/*
* Copyright 2021 Stepan Perun
* This program is free software.
*
* License: Gnu General Public License GPL-2
* file:///usr/share/common-licenses/GPL-2
* http://www.gnu.org/licenses/gpl-2.0.html
*/

#include "dvb.h"

struct _Dvb
{
	GObject parent_instance;

	struct dvb_device *dvb_scan, *dvb_zap, *dvb_fe;
	struct dvb_open_descriptor *video_fd, *audio_fd;

	char *demux_dev;
	char *input_file, *output_file;
	enum dvb_file_formats input_format, output_format;

	uint8_t adapter, frontend, demux, time_mult, diseqc_wait;
	uint8_t new_freqs, get_detect, get_nit, other_nit;
	int8_t lna, lnb, sat_num;

	uint8_t descr_num;
	uint16_t pids[3]; // 0 - sid, 1 - vpid, 2 - apid

	GMutex mutex;
	GThread *thread;

	uint8_t thread_stop;
	uint32_t freq_scan, progs_scan;

	gboolean exit;
};

G_DEFINE_TYPE ( Dvb, dvb, G_TYPE_OBJECT )

static void dvb_info_stats ( Dvb *dvb );

static uint8_t _get_delsys ( struct dvb_v5_fe_parms *parms )
{
	uint8_t sys = SYS_UNDEFINED;

	switch ( parms->current_sys )
	{
		case SYS_DVBT:
		case SYS_DVBS:
		case SYS_DVBC_ANNEX_A:
		case SYS_ATSC:
			sys = parms->current_sys;
			break;
		case SYS_DVBC_ANNEX_C:
			sys = SYS_DVBC_ANNEX_A;
			break;
		case SYS_DVBC_ANNEX_B:
			sys = SYS_ATSC;
			break;
		case SYS_ISDBT:
		case SYS_DTMB:
			sys = SYS_DVBT;
			break;
		default:
			sys = SYS_UNDEFINED;
			break;
	}

	return sys;
}

static int _check_frontend ( G_GNUC_UNUSED void *__args, G_GNUC_UNUSED struct dvb_v5_fe_parms *parms )
{
	return 0;
}

static gpointer dvb_scan_thread ( Dvb *dvb_base )
{
	struct dvb_device *dvb = dvb_base->dvb_scan;
	struct dvb_v5_fe_parms *parms = dvb->fe_parms;
	struct dvb_file *dvb_file = NULL, *dvb_file_new = NULL;
	struct dvb_entry *entry;
	struct dvb_open_descriptor *dmx_fd;

	g_mutex_init ( &dvb_base->mutex );

	int count = 0, shift;
	uint32_t freq = 0, sys = _get_delsys ( parms );
	enum dvb_sat_polarization pol;

	dvb_file = dvb_read_file_format ( dvb_base->input_file, sys, dvb_base->input_format );

	if ( !dvb_file )
	{
		dvb_dev_free ( dvb );
		dvb_base->dvb_scan  = NULL;
		dvb_base->demux_dev = NULL;

		g_mutex_clear ( &dvb_base->mutex );
		g_critical ( "%s:: Read file format failed.", __func__ );
		return NULL;
	}

	dmx_fd = dvb_dev_open ( dvb, dvb_base->demux_dev, O_RDWR );

	if ( !dmx_fd )
	{
		dvb_file_free ( dvb_file );
		dvb_dev_free ( dvb );
		dvb_base->dvb_scan  = NULL;
		dvb_base->demux_dev = NULL;

		g_mutex_clear ( &dvb_base->mutex );
		perror ( "opening demux failed" );
		return NULL;
	}

	for ( entry = dvb_file->first_entry; entry != NULL; entry = entry->next )
	{
		struct dvb_v5_descriptors *dvb_scan_handler = NULL;
		uint32_t stream_id;

		if ( dvb_retrieve_entry_prop ( entry, DTV_FREQUENCY, &freq ) ) continue;

		shift = dvb_estimate_freq_shift ( parms );

		if ( dvb_retrieve_entry_prop ( entry, DTV_POLARIZATION, &pol ) ) pol = POLARIZATION_OFF;
		if ( dvb_retrieve_entry_prop ( entry, DTV_STREAM_ID, &stream_id ) ) stream_id = NO_STREAM_ID_FILTER;
		if ( !dvb_new_entry_is_needed ( dvb_file->first_entry, entry, freq, shift, pol, stream_id ) ) continue;

		count++;
		dvb_log ( "Scanning frequency #%d %d", count, freq );

		g_mutex_lock ( &dvb_base->mutex );
			dvb_base->freq_scan = freq;
		g_mutex_unlock ( &dvb_base->mutex );

		dvb_scan_handler = dvb_dev_scan ( dmx_fd, entry, &_check_frontend, NULL, dvb_base->other_nit, dvb_base->time_mult );

		g_mutex_lock ( &dvb_base->mutex );
			if ( dvb_scan_handler ) dvb_base->progs_scan += dvb_scan_handler->num_program;
			if ( dvb_base->thread_stop ) parms->abort = 1;
		g_mutex_unlock ( &dvb_base->mutex );

		if ( parms->abort )
		{
			dvb_scan_free_handler_table ( dvb_scan_handler );
			break;
		}

		if ( !dvb_scan_handler ) continue;

		dvb_store_channel ( &dvb_file_new, parms, dvb_scan_handler, dvb_base->get_detect, dvb_base->get_nit );

		if ( !dvb_base->new_freqs )
			dvb_add_scaned_transponders ( parms, dvb_scan_handler, dvb_file->first_entry, entry );

		dvb_scan_free_handler_table ( dvb_scan_handler );
	}

	if ( dvb_file_new ) dvb_write_file_format ( dvb_base->output_file, dvb_file_new, parms->current_sys, dvb_base->output_format );

	dvb_file_free ( dvb_file );
	if ( dvb_file_new ) dvb_file_free ( dvb_file_new );

	dvb_dev_close ( dmx_fd );

	g_mutex_lock ( &dvb_base->mutex );
		dvb_base->thread_stop = 1;
	g_mutex_unlock ( &dvb_base->mutex );

	g_mutex_clear ( &dvb_base->mutex );

	dvb_dev_free ( dvb );
	dvb_base->dvb_scan  = NULL;
	dvb_base->demux_dev = NULL;

	return NULL;
}

static const char * dvb_scan ( Dvb *dvb )
{
	dvb->thread_stop = 0;

	dvb->dvb_scan = dvb_dev_alloc ();

	if ( !dvb->dvb_scan ) return "Allocates memory failed.";

	dvb_dev_set_log ( dvb->dvb_scan, 0, NULL );
	dvb_dev_find ( dvb->dvb_scan, NULL, NULL );

	struct dvb_dev_list *dvb_dev = dvb_dev_seek_by_adapter ( dvb->dvb_scan, dvb->adapter, dvb->demux, DVB_DEVICE_DEMUX );

	if ( !dvb_dev )
	{
		dvb_dev_free ( dvb->dvb_scan );
		dvb->dvb_scan = NULL;

		g_critical ( "%s:: Couldn't find demux device node.", __func__ );
		return "Couldn't find demux device.";
	}

	dvb->demux_dev = dvb_dev->sysname;
	g_message ( "%s:: demux_dev: %s ", __func__, dvb->demux_dev );

	dvb_dev = dvb_dev_seek_by_adapter ( dvb->dvb_scan, dvb->adapter, dvb->frontend, DVB_DEVICE_FRONTEND );

	if ( !dvb_dev )
	{
		dvb_dev_free ( dvb->dvb_scan );
		dvb->dvb_scan = NULL;

		g_critical ( "%s:: Couldn't find frontend device.", __func__ );
		return "Couldn't find frontend device.";
	}

	if ( !dvb_dev_open ( dvb->dvb_scan, dvb_dev->sysname, O_RDWR ) )
	{
		dvb_dev_free ( dvb->dvb_scan );
		dvb->dvb_scan = NULL;

		perror ( "Opening device failed" );
		return "Opening device failed.";
	}

	struct dvb_v5_fe_parms *parms = dvb->dvb_scan->fe_parms;

	if ( dvb->lnb >= 0 ) parms->lnb = dvb_sat_get_lnb ( dvb->lnb );
	if ( dvb->sat_num >= 0 ) parms->sat_number = dvb->sat_num;
	parms->diseqc_wait = dvb->diseqc_wait;
	parms->lna = dvb->lna;
	parms->freq_bpf = 0;

	dvb->freq_scan  = 0;
	dvb->progs_scan = 0;

	dvb_info_stats ( dvb );

	dvb->thread = g_thread_new ( "scan-thread", (GThreadFunc)dvb_scan_thread, dvb );
	g_thread_unref ( dvb->thread );

	return NULL;
}

static void dvb_handler_scan ( Dvb *dvb, uint8_t a, uint8_t f, uint8_t d, uint8_t t, uint8_t q, uint8_t c, uint8_t n, uint8_t o, 
	int8_t sn, uint8_t dq, const char *lnb_name, const char *lna, const char *fi, const char *fo, const char *fmi, const char *fmo )
{
	if ( dvb->dvb_scan || dvb->dvb_zap ) { g_signal_emit_by_name ( dvb, "dvb-scan-info", "It works ..." ); return; }

	dvb->adapter   = a;
	dvb->frontend  = f;
	dvb->demux     = d;
	dvb->time_mult = t;

	dvb->new_freqs  = q;
	dvb->get_detect = c;
	dvb->get_nit    = n;
	dvb->other_nit  = o;

	dvb->lna = -1; // "On", "Off", "Auto"
	dvb->lnb = (int8_t)dvb_sat_search_lnb ( lnb_name );
	dvb->sat_num = sn;
	dvb->diseqc_wait = dq;

	if ( g_str_equal ( lna, "On"  ) ) dvb->lna = 0;
	if ( g_str_equal ( lna, "Off" ) ) dvb->lna = 1;

	const char *fmt[] = { "DVBV5", "VDR", "CHANNEL", "ZAP" };
	enum dvb_file_formats dfm[] = { FILE_DVBV5, FILE_VDR, FILE_CHANNEL, FILE_ZAP };

	uint8_t z = 0;
	for ( z = 0; z < 4; z++ ) { if ( g_str_equal ( fmi, fmt[z] ) ) dvb->input_format  = dfm[z]; }
	for ( z = 0; z < 4; z++ ) { if ( g_str_equal ( fmo, fmt[z] ) ) dvb->output_format = dfm[z]; }

	if ( dvb->input_file  ) free ( dvb->input_file  );
	if ( dvb->output_file ) free ( dvb->output_file );

	dvb->input_file  = g_strdup ( fi );
	dvb->output_file = g_strdup ( fo );

	const char *ret_str = dvb_scan ( dvb );

	if ( ret_str ) g_signal_emit_by_name ( dvb, "dvb-scan-info", ret_str );
}

static void dvb_handler_scan_stop ( Dvb *dvb )
{
	if ( dvb->dvb_scan ) dvb->thread_stop = 1;
}

static uint8_t dvb_zap_parse ( const char *file, const char *channel, uint8_t frm, struct dvb_v5_fe_parms *parms, uint16_t pids[] )
{
	struct dvb_file *dvb_file;
	struct dvb_entry *entry;

	uint8_t i = 0;
	uint32_t sys = _get_delsys ( parms );

	dvb_file = dvb_read_file_format ( file, sys, frm );

	if ( !dvb_file )
	{
		g_critical ( "%s:: Read file format failed.", __func__ );
		return 0;
	}

	for ( entry = dvb_file->first_entry; entry != NULL; entry = entry->next )
	{
		if ( entry->channel && !strcmp ( entry->channel, channel ) ) break;
		if ( entry->vchannel && !strcmp ( entry->vchannel, channel ) ) break;
	}

	if ( !entry )
	{
		for ( entry = dvb_file->first_entry; entry != NULL; entry = entry->next )
		{
			if ( entry->channel && !strcasecmp ( entry->channel, channel ) ) break;
		}
	}

	if ( !entry )
	{
		uint32_t f, freq = (uint32_t)atoi ( channel );

		if ( freq )
		{
			for ( entry = dvb_file->first_entry; entry != NULL; entry = entry->next )
			{
				dvb_retrieve_entry_prop ( entry, DTV_FREQUENCY, &f );

				if ( f == freq ) break;
			}
		}
	}

	if ( !entry )
	{
		g_critical ( "%s:: channel %s | file %s | Can't find channel.", __func__, channel, file );

		dvb_file_free ( dvb_file );
		return 0;
	}

	if ( entry->lnb )
	{
		int lnb = dvb_sat_search_lnb ( entry->lnb );

		if ( lnb == -1 )
		{
			g_warning ( "%s:: Unknown LNB %s", __func__, entry->lnb );
			dvb_file_free ( dvb_file );
			return 0;
		}

		parms->lnb = dvb_sat_get_lnb (lnb);
	}

	// pids[3];  0 - sid, 1 - vpid, 2 - apid
	if ( entry->service_id ) pids[0] = entry->service_id;
	if ( entry->video_pid  ) pids[1] = entry->video_pid[0];
	if ( entry->audio_pid  ) pids[2] = entry->audio_pid[0];
	if ( entry->sat_number >= 0 ) parms->sat_number = entry->sat_number;

	dvb_retrieve_entry_prop (entry, DTV_DELIVERY_SYSTEM, &sys );
	dvb_set_compat_delivery_system ( parms, sys );

	/* Copy data into parms */
	for ( i = 0; i < entry->n_props; i++ )
	{
		uint32_t data = entry->props[i].u.data;

		/* Don't change the delivery system */
		if ( entry->props[i].cmd == DTV_DELIVERY_SYSTEM ) continue;

		dvb_fe_store_parm ( parms, entry->props[i].cmd, data );

		if ( parms->current_sys == SYS_ISDBT )
		{
			dvb_fe_store_parm ( parms, DTV_ISDBT_PARTIAL_RECEPTION,  0 );
			dvb_fe_store_parm ( parms, DTV_ISDBT_SOUND_BROADCASTING, 0 );
			dvb_fe_store_parm ( parms, DTV_ISDBT_LAYER_ENABLED,   0x07 );

			if ( entry->props[i].cmd == DTV_CODE_RATE_HP )
			{
				dvb_fe_store_parm ( parms, DTV_ISDBT_LAYERA_FEC, data );
				dvb_fe_store_parm ( parms, DTV_ISDBT_LAYERB_FEC, data );
				dvb_fe_store_parm ( parms, DTV_ISDBT_LAYERC_FEC, data );
			}
			else if ( entry->props[i].cmd == DTV_MODULATION )
			{
				dvb_fe_store_parm ( parms, DTV_ISDBT_LAYERA_MODULATION, data );
				dvb_fe_store_parm ( parms, DTV_ISDBT_LAYERB_MODULATION, data );
				dvb_fe_store_parm ( parms, DTV_ISDBT_LAYERC_MODULATION, data );
			}
		}

		if ( parms->current_sys == SYS_ATSC && entry->props[i].cmd == DTV_MODULATION )
		{
			if ( data != VSB_8 && data != VSB_16 )
				dvb_fe_store_parm ( parms, DTV_DELIVERY_SYSTEM, SYS_DVBC_ANNEX_B );
		}
	}

	dvb_file_free ( dvb_file );

	return 1;
}

static uint32_t dvb_zap_setup_frontend ( struct dvb_v5_fe_parms *parms )
{
	uint32_t freq = 0;

	int rc = dvb_fe_retrieve_parm ( parms, DTV_FREQUENCY, &freq );

	if ( rc < 0 ) return 0;

	rc = dvb_fe_set_parms ( parms );

	if ( rc < 0 ) return 0;

	return freq;
}

static uint8_t dvb_zap_set_pes_filter ( struct dvb_open_descriptor *fd, uint16_t pid, dmx_pes_type_t type, dmx_output_t dmx, uint32_t buf_size )
{
	if ( dvb_dev_dmx_set_pesfilter ( fd, pid, type, dmx, (int)buf_size ) < 0 ) return 0;

	return 1;
}

static void dvb_zap_set_dmx ( Dvb *dvb )
{
	// dvb->pids[3];  0 - sid, 1 - vpid, 2 - apid

	uint32_t bsz = ( dvb->descr_num == DMX_OUT_TS_TAP || dvb->descr_num == 4 ) ? 64 * 1024 : 0;

	if ( dvb->pids[1] )
	{
		dvb->video_fd = dvb_dev_open ( dvb->dvb_zap, dvb->demux_dev, O_RDWR );

		if ( dvb->video_fd )
			dvb_zap_set_pes_filter ( dvb->video_fd, dvb->pids[1], DMX_PES_VIDEO, dvb->descr_num, bsz );
		else
			g_critical ( "%s:: VIDEO: failed opening %s", __func__, dvb->demux_dev );
	}

	if ( dvb->pids[2] )
	{
		dvb->audio_fd = dvb_dev_open ( dvb->dvb_zap, dvb->demux_dev, O_RDWR );

		if ( dvb->audio_fd )
			dvb_zap_set_pes_filter ( dvb->audio_fd, dvb->pids[2], DMX_PES_AUDIO, dvb->descr_num, bsz );
		else
			g_critical ( "%s:: AUDIO: failed opening %s", __func__, dvb->demux_dev );
	}
}


static const char * dvb_zap ( uint8_t a, uint8_t f, uint8_t d, uint8_t num, const char *channel, const char *file, Dvb *dvb )
{
	dvb->dvb_zap = dvb_dev_alloc ();

	if ( !dvb->dvb_zap ) return "Allocates memory failed.";

	dvb_dev_set_log ( dvb->dvb_zap, 0, NULL );
	dvb_dev_find ( dvb->dvb_zap, NULL, NULL );
	struct dvb_v5_fe_parms *parms = dvb->dvb_zap->fe_parms;

	struct dvb_dev_list *dvb_dev = dvb_dev_seek_by_adapter ( dvb->dvb_zap, a, d, DVB_DEVICE_DEMUX );

	if ( !dvb_dev )
	{
		dvb_dev_free ( dvb->dvb_zap );
		dvb->dvb_zap = NULL;

		g_critical ( "%s: Couldn't find demux device node.", __func__ );
		return "Couldn't find demux device.";
	}

	dvb->demux_dev = dvb_dev->sysname;

	dvb_dev = dvb_dev_seek_by_adapter ( dvb->dvb_zap, a, d, DVB_DEVICE_DVR );

	if ( !dvb_dev )
	{
		dvb_dev_free ( dvb->dvb_zap );
		dvb->dvb_zap = NULL;

		g_critical ( "%s: Couldn't find dvr device node.", __func__ );
		return "Couldn't find dvr device.";
	}

	dvb_dev = dvb_dev_seek_by_adapter ( dvb->dvb_zap, a, f, DVB_DEVICE_FRONTEND );

	if ( !dvb_dev )
	{
		dvb_dev_free ( dvb->dvb_zap );
		dvb->dvb_zap = NULL;

		g_critical ( "%s: Couldn't find frontend device node.", __func__ );
		return "Couldn't find frontend device.";
	}

	if ( !dvb_dev_open ( dvb->dvb_zap, dvb_dev->sysname, O_RDWR ) )
	{
		dvb_dev_free ( dvb->dvb_zap );
		dvb->dvb_zap = NULL;

		perror ( "Opening device failed" );
		return "Opening device failed.";
	}

	parms->diseqc_wait = 0;
	parms->freq_bpf = 0;
	parms->lna = -1;

	dvb->descr_num = num;

	if ( !dvb_zap_parse ( file, channel, FILE_DVBV5, parms, dvb->pids ) )
	{
		dvb_dev_free ( dvb->dvb_zap );
		dvb->dvb_zap = NULL;

		g_critical ( "%s:: Zap parse failed.", __func__ );
		return "Zap parse failed.";
	}

	uint32_t freq = dvb_zap_setup_frontend ( parms );

	if ( freq )
	{
		dvb->freq_scan = freq;

		dvb_zap_set_dmx ( dvb );

		g_message ( "%s:: Zap Ok.", __func__ );
	}
	else
	{
		dvb_dev_free ( dvb->dvb_zap );
		dvb->dvb_zap = NULL;

		g_warning ( "%s:: Zap failed.", __func__ );
		return "Zap failed.";
	}

	dvb_info_stats ( dvb );

	return NULL;
}

static void dvb_handler_zap ( Dvb *dvb, uint8_t a, uint8_t f, uint8_t d, uint8_t num, const char *channel, const char *file )
{
	if ( dvb->dvb_scan || dvb->dvb_zap ) { g_signal_emit_by_name ( dvb, "dvb-scan-info", "It works ..." ); return; }

	dvb->freq_scan = 0;

	const char *ret_str = dvb_zap ( a, f, d, num, channel, file, dvb );

	if ( ret_str ) g_signal_emit_by_name ( dvb, "dvb-scan-info", ret_str );
}

static void dvb_handler_zap_stop ( Dvb *dvb )
{
	if ( dvb->dvb_zap )
	{
		dvb->pids[0] = 0;
		dvb->pids[1] = 0;
		dvb->pids[2] = 0;

		if ( dvb->audio_fd ) dvb_dev_close ( dvb->audio_fd );
		if ( dvb->video_fd ) dvb_dev_close ( dvb->video_fd );

		dvb->audio_fd = NULL;
		dvb->video_fd = NULL;

		dvb_dev_free ( dvb->dvb_zap );
		dvb->dvb_zap = NULL;
		dvb->demux_dev = NULL;
	}
}

static int _frontend_stats ( struct dvb_v5_fe_parms *parms, Dvb *dvb )
{
	char buf[512], *p;
	int rc, i, len, show;

	rc = dvb_fe_get_stats ( parms );
	if ( rc ) return -1;

	p = buf;
	len = sizeof(buf);
	// dvb_fe_snprintf_stat ( parms, DTV_STATUS, NULL, 0, &p, &len, &show );

	for ( i = 0; i < MAX_DTV_STATS; i++ )
	{
		show = 1;

		dvb_fe_snprintf_stat ( parms, DTV_QUALITY, "Quality ", i, &p, &len, &show );
		dvb_fe_snprintf_stat ( parms, DTV_STAT_SIGNAL_STRENGTH, "  Signal ", i, &p, &len, &show );
		dvb_fe_snprintf_stat ( parms, DTV_STAT_CNR, "  C/N ", i, &p, &len, &show );
/*
		dvb_fe_snprintf_stat ( parms, DTV_STAT_ERROR_BLOCK_COUNT, "UCB", i,  &p, &len, &show );
		dvb_fe_snprintf_stat ( parms, DTV_BER, "postBER", i,  &p, &len, &show );
		dvb_fe_snprintf_stat ( parms, DTV_PRE_BER, "preBER", i,  &p, &len, &show );
		dvb_fe_snprintf_stat ( parms, DTV_PER, "PER", i,  &p, &len, &show );
*/
		if ( p != buf )
		{
			// g_message ( "%d: %s ", i, buf );

			g_signal_emit_by_name ( dvb, "stats-org", i, buf );

			p = buf;
			len = sizeof(buf);
		}
	}

	return 0;
}

static void dvb_fe_stat_get ( Dvb *dvb )
{
	struct dvb_v5_fe_parms *parms = dvb->dvb_fe->fe_parms;

	int rc = dvb_fe_get_stats ( parms );

	if ( rc ) { g_warning ( "%s:: failed.", __func__ ); return; }

	uint32_t freq = 0, qual = 0;
	gboolean fe_lock = FALSE;

	fe_status_t status;
	dvb_fe_retrieve_stats ( parms, DTV_STATUS,  &status );
	dvb_fe_retrieve_stats ( parms, DTV_QUALITY,   &qual );
	dvb_fe_retrieve_parm  ( parms, DTV_FREQUENCY, &freq );

	if ( status & FE_HAS_LOCK ) fe_lock = TRUE;

	uint32_t sgl = 0, snr = 0;
	dvb_fe_retrieve_stats ( parms, DTV_STAT_CNR, &snr );
	dvb_fe_retrieve_stats ( parms, DTV_STAT_SIGNAL_STRENGTH, &sgl );

	uint8_t sgl_p = (uint8_t)(sgl * 100 / 65535), snr_p = (uint8_t)(snr * 100 / 65535);

	char sgl_s[256];
	sprintf ( sgl_s, "Signal:  %u%% ", sgl_p );

	char snr_s[256];
	sprintf ( snr_s, "Signal:  %u%% ", snr_p );

	g_signal_emit_by_name ( dvb, "stats-update", dvb->freq_scan, qual, sgl_s, snr_s, sgl_p, snr_p, fe_lock );

	_frontend_stats ( parms, dvb );
}

static const char * dvb_info ( uint8_t adapter, uint8_t frontend, Dvb *dvb )
{
	dvb->dvb_fe = dvb_dev_alloc ();

	if ( !dvb->dvb_fe ) return "Allocates memory failed.";

	dvb_dev_set_log ( dvb->dvb_fe, 0, NULL );
	dvb_dev_find ( dvb->dvb_fe, NULL, NULL );

	struct dvb_dev_list *dvb_dev_fe = dvb_dev_seek_by_adapter ( dvb->dvb_fe, adapter, frontend, DVB_DEVICE_FRONTEND );

	if ( !dvb_dev_fe )
	{
		g_critical ( "%s: Couldn't find demux device.", __func__ );
		return "Couldn't find demux device.";
	}

	if ( !dvb_dev_open ( dvb->dvb_fe, dvb_dev_fe->sysname, O_RDONLY ) )
	{
		g_critical ( "%s: Opening device failed.", __func__ );
		return "Opening device failed.";
	}

	return NULL;
}

static void dvb_handler_dvb_info ( Dvb *dvb, uint8_t adapter, uint8_t frontend )
{
	const char *error = dvb_info ( adapter, frontend, dvb );

	if ( error )
	{
		g_signal_emit_by_name ( dvb, "dvb-name", "Undefined" );
	}
	else
	{
		struct dvb_v5_fe_parms *parms = dvb->dvb_fe->fe_parms;

		g_autofree char *ret = g_strdup ( parms->info.name );

		g_signal_emit_by_name ( dvb, "dvb-name", ret );
	}

	if ( dvb->dvb_fe ) { dvb_dev_free ( dvb->dvb_fe ); dvb->dvb_fe = NULL; }
}

static gboolean dvb_info_show_stats ( Dvb *dvb )
{
	if ( dvb->exit ) return FALSE;

	if ( dvb->dvb_scan == NULL && dvb->dvb_zap == NULL )
	{
		if ( dvb->dvb_fe ) { dvb_dev_free ( dvb->dvb_fe ); dvb->dvb_fe = NULL; }

		g_signal_emit_by_name ( dvb, "stats-update", 0, 0, "Signal", "Snr", 0, 0, FALSE );

		return FALSE;
	}

	dvb_fe_stat_get ( dvb );

	return TRUE;
}

static void dvb_info_stats ( Dvb *dvb )
{
	const char *error = dvb_info ( dvb->adapter, dvb->frontend, dvb );

	if ( error )
		g_signal_emit_by_name ( dvb, "dvb-scan-info", error );
	else
		g_timeout_add ( 250, (GSourceFunc)dvb_info_show_stats, dvb );
}

static void dvb_init ( Dvb *dvb )
{
	dvb->exit = FALSE;

	dvb->dvb_fe = NULL;
	dvb->dvb_zap = NULL;
	dvb->dvb_scan = NULL;

	dvb->adapter   = 0;
	dvb->frontend  = 0;
	dvb->demux     = 0;
	dvb->time_mult = 2;

	dvb->new_freqs  = 0;
	dvb->get_detect = 0;
	dvb->get_nit    = 0;
	dvb->other_nit  = 0;

	dvb->lna = -1;
	dvb->lnb = -1;
	dvb->sat_num = -1;
	dvb->diseqc_wait = 0;

	dvb->pids[0] = 0;
	dvb->pids[1] = 0;
	dvb->pids[2] = 0;

	dvb->audio_fd = NULL;
	dvb->video_fd = NULL;

	dvb->descr_num = 0;
	dvb->freq_scan = 0;

	dvb->input_file  = NULL;
	dvb->output_file = NULL;

	dvb->input_format  = FILE_DVBV5;
	dvb->output_format = FILE_DVBV5;

	g_signal_connect ( dvb, "dvb-info",      G_CALLBACK ( dvb_handler_dvb_info  ), NULL );
	g_signal_connect ( dvb, "dvb-zap",       G_CALLBACK ( dvb_handler_zap       ), NULL );
	g_signal_connect ( dvb, "dvb-zap-stop",  G_CALLBACK ( dvb_handler_zap_stop  ), NULL );
	g_signal_connect ( dvb, "dvb-scan-stop", G_CALLBACK ( dvb_handler_scan_stop ), NULL );
	g_signal_connect ( dvb, "dvb-scan-set-data", G_CALLBACK ( dvb_handler_scan  ), NULL );
}

static void dvb_finalize ( GObject *object )
{
	Dvb *dvb = DVB_OBJECT ( object );

	dvb->exit = TRUE;

	if ( dvb->input_file ) free ( dvb->input_file  );
	if ( dvb->input_file ) free ( dvb->output_file );

	if ( dvb->dvb_fe   ) dvb_dev_free ( dvb->dvb_fe   );
	if ( dvb->dvb_zap  ) dvb_dev_free ( dvb->dvb_zap  );
	if ( dvb->dvb_scan ) dvb_dev_free ( dvb->dvb_scan );

	dvb->dvb_fe = NULL;
	dvb->dvb_zap = NULL;
	dvb->dvb_scan = NULL;

	G_OBJECT_CLASS (dvb_parent_class)->finalize (object);
}

static void dvb_class_init ( DvbClass *class )
{
	GObjectClass *oclass = G_OBJECT_CLASS (class);

	oclass->finalize = dvb_finalize;

	g_signal_new ( "stats-org", G_TYPE_FROM_CLASS ( class ), G_SIGNAL_RUN_LAST,
		0, NULL, NULL, NULL, G_TYPE_NONE, 2, G_TYPE_INT, G_TYPE_STRING );

	g_signal_new ( "dvb-name", G_TYPE_FROM_CLASS ( class ), G_SIGNAL_RUN_LAST,
		0, NULL, NULL, NULL, G_TYPE_NONE, 1, G_TYPE_STRING );

	g_signal_new ( "dvb-info", G_TYPE_FROM_CLASS ( class ), G_SIGNAL_RUN_LAST,
		0, NULL, NULL, NULL, G_TYPE_NONE, 2, G_TYPE_UINT, G_TYPE_UINT );

	g_signal_new ( "dvb-scan-stop", G_TYPE_FROM_CLASS ( class ), G_SIGNAL_RUN_LAST,
		0, NULL, NULL, NULL, G_TYPE_NONE, 0 );

	g_signal_new ( "dvb-scan-info", G_TYPE_FROM_CLASS ( class ), G_SIGNAL_RUN_LAST,
		0, NULL, NULL, NULL, G_TYPE_NONE, 1, G_TYPE_STRING );

	g_signal_new ( "dvb-zap", G_TYPE_FROM_CLASS ( class ), G_SIGNAL_RUN_LAST,
		0, NULL, NULL, NULL, G_TYPE_NONE, 6, G_TYPE_UINT, G_TYPE_UINT, G_TYPE_UINT, G_TYPE_UINT, G_TYPE_STRING, G_TYPE_STRING );

	g_signal_new ( "dvb-zap-stop", G_TYPE_FROM_CLASS ( class ), G_SIGNAL_RUN_LAST,
		0, NULL, NULL, NULL, G_TYPE_NONE, 0 );

	g_signal_new ( "dvb-scan-set-data", G_TYPE_FROM_CLASS ( class ), G_SIGNAL_RUN_LAST,
		0, NULL, NULL, NULL, G_TYPE_NONE, 16, G_TYPE_UINT, G_TYPE_UINT, G_TYPE_UINT, G_TYPE_UINT, G_TYPE_UINT, G_TYPE_UINT, G_TYPE_UINT, G_TYPE_UINT, 
		G_TYPE_INT, G_TYPE_UINT, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING );

	g_signal_new ( "stats-update", G_TYPE_FROM_CLASS ( class ), G_SIGNAL_RUN_LAST,
		0, NULL, NULL, NULL, G_TYPE_NONE, 7, G_TYPE_UINT, G_TYPE_UINT, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_UINT, G_TYPE_UINT, G_TYPE_BOOLEAN );
}

Dvb * dvb_new ( void )
{
	Dvb *dvb = g_object_new ( DVB_TYPE_OBJECT, NULL );

	return dvb;
}
