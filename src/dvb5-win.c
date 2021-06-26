/*
* Copyright 2021 Stepan Perun
* This program is free software.
*
* License: Gnu General Public License GPL-2
* file:///usr/share/common-licenses/GPL-2
* http://www.gnu.org/licenses/gpl-2.0.html
*/

#include "dvb5-win.h"
#include "dvb.h"
#include "zap.h"
#include "scan.h"
#include "file.h"
#include "status.h"

#include <locale.h>

struct _Dvb5Win
{
	GtkWindow  parent_instance;
	GtkNotebook *notebook;

	Zap *zap;
	Scan *scan;
	Status *status;

	Dvb *dvb;
	gboolean fe_lock;
	uint8_t adapter, frontend, demux;
};

G_DEFINE_TYPE ( Dvb5Win, dvb5_win, GTK_TYPE_WINDOW )

static void dvbv5_about ( Dvb5Win *win )
{
	GtkAboutDialog *dialog = (GtkAboutDialog *)gtk_about_dialog_new ();
	gtk_window_set_transient_for ( GTK_WINDOW ( dialog ), GTK_WINDOW ( win ) );

	gtk_window_set_icon_name ( GTK_WINDOW ( dialog ), "display" );
	gtk_about_dialog_set_logo_icon_name ( dialog, "display" );

	const char *authors[] = { "Stepan Perun", " ", NULL };
	const char *issues[]  = { "Mauro Carvalho Chehab", "zampedro", "Ro-Den", " ", NULL };

	gtk_about_dialog_add_credit_section ( dialog, "Issues ( github.com )", issues );

	gtk_about_dialog_set_program_name ( dialog, "Dvbv5-Gtk" );
	gtk_about_dialog_set_version ( dialog, VERSION );
	gtk_about_dialog_set_license_type ( dialog, GTK_LICENSE_GPL_2_0 );
	gtk_about_dialog_set_authors ( dialog, authors );
	gtk_about_dialog_set_website ( dialog,   "https://github.com/vl-nix/dvbv5-gtk" );
	gtk_about_dialog_set_copyright ( dialog, "Copyright 2021 Dvbv5-Gtk" );
	gtk_about_dialog_set_comments  ( dialog, "Gtk+3 interface to DVBv5 tool" );

	gtk_dialog_run ( GTK_DIALOG (dialog) );
	gtk_widget_destroy ( GTK_WIDGET (dialog) );
}

static void dvb5_win_create ( Dvb5Win *win )
{
	setlocale ( LC_NUMERIC, "C" );

	GtkWindow *window = GTK_WINDOW ( win );

	gtk_window_set_title ( window, "Dvbv5-Gtk");
	gtk_window_set_icon_name ( window, "terminal" );

	GtkBox *main_vbox = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL, 0 );
	gtk_box_set_spacing ( main_vbox, 5 );
	gtk_widget_set_visible ( GTK_WIDGET ( main_vbox ), TRUE );

	win->notebook = (GtkNotebook *)gtk_notebook_new ();
	gtk_notebook_set_scrollable ( win->notebook, TRUE );
	gtk_widget_set_visible ( GTK_WIDGET ( win->notebook ), TRUE );

	gtk_notebook_append_page ( win->notebook, GTK_WIDGET ( win->scan   ), gtk_label_new ( "Scan"   ) );
	gtk_notebook_append_page ( win->notebook, GTK_WIDGET ( win->zap    ), gtk_label_new ( "Zap"    ) );
	gtk_notebook_append_page ( win->notebook, GTK_WIDGET ( win->status ), gtk_label_new ( "Status" ) );

	gtk_notebook_set_tab_pos ( win->notebook, GTK_POS_TOP );
	gtk_box_pack_start ( main_vbox, GTK_WIDGET (win->notebook), TRUE, TRUE, 0 );

	gtk_container_add  ( GTK_CONTAINER ( window ), GTK_WIDGET ( main_vbox ) );

	gtk_window_present ( GTK_WINDOW ( win ) );
}

static void dvb5_handler_win_info ( G_GNUC_UNUSED Status *status, Dvb5Win *win )
{
	dvbv5_about ( win );
}

static void dvb5_handler_win_close ( G_GNUC_UNUSED Status *status, Dvb5Win *win )
{
	gtk_widget_destroy ( GTK_WIDGET ( win ) );
}

static void dvb5_handler_scan_start ( G_GNUC_UNUSED Status *status, Dvb5Win *win )
{
	g_signal_emit_by_name ( win->scan, "scan-get-data" );
}

static void dvb5_handler_scan_stop ( G_GNUC_UNUSED Status *status, Dvb5Win *win )
{
	g_signal_emit_by_name ( win->zap, "zap-stop"      );
	g_signal_emit_by_name ( win->dvb, "dvb-zap-stop"  );
	g_signal_emit_by_name ( win->dvb, "dvb-scan-stop" );

	win->fe_lock = FALSE;
	g_signal_emit_by_name ( win->status, "status-update", 0, NULL, 0, "Signal", "Snr", 0, 0, FALSE );
}

static void dvb5_handler_scan_data ( G_GNUC_UNUSED Scan *scan, uint8_t a, uint8_t f, uint8_t d, uint8_t t, gboolean q, gboolean c, gboolean n, gboolean o, 
	int8_t sn, uint8_t dq, const char *lnb, const char *lna, const char *fi, const char *fo, const char *fmi, const char *fmo, Dvb5Win *win )
{
	uint8_t adapter = a, frontend = f, demux = d, time_mult = t;
	uint8_t new_freqs = ( q ) ? 1 : 0, get_detect = ( c ) ? 1 : 0, get_nit = ( n ) ? 1 : 0, other_nit = ( o ) ? 1 : 0;

	if ( !g_file_test ( fi, G_FILE_TEST_EXISTS ) )
	{
		dvb5_message_dialog ( fi, g_strerror ( errno ), GTK_MESSAGE_ERROR, GTK_WINDOW ( win ) );
		return;
	}

	g_signal_emit_by_name ( win->dvb, "dvb-scan-set-data", adapter, frontend, demux, time_mult, new_freqs, get_detect, get_nit, other_nit, 
		sn, dq, lnb, lna, fi, fo, fmi, fmo );
}

static void dvb5_handler_zap_data ( G_GNUC_UNUSED Zap *zap, uint8_t dmx_out, const char *channel, const char *file, Dvb5Win *win )
{
	g_signal_emit_by_name ( win->dvb, "dvb-zap", win->adapter, win->frontend, win->demux, dmx_out, channel, file );
}

static gboolean dvb5_handler_zap_lock ( G_GNUC_UNUSED Zap *zap, Dvb5Win *win )
{
	return win->fe_lock;
}

static uint8_t dvb5_handler_zap_adapter ( G_GNUC_UNUSED Zap *zap, Dvb5Win *win )
{
	return win->adapter;
}

static uint8_t dvb5_handler_zap_demux ( G_GNUC_UNUSED Zap *zap, Dvb5Win *win )
{
	return win->demux;
}

static void dvb5_handler_scan_info ( G_GNUC_UNUSED Dvb *dvb, const char *ret_str, Dvb5Win *win )
{
	dvb5_message_dialog ( "", ret_str, GTK_MESSAGE_ERROR, GTK_WINDOW ( win ) );
}

static void dvb5_handler_dvb_name ( G_GNUC_UNUSED Dvb *dvb, const char *dvb_name, Dvb5Win *win )
{
	g_signal_emit_by_name ( win->status, "set-dvb-name", dvb_name );
}

static void dvb5_handler_stats_upd ( G_GNUC_UNUSED Dvb *dvb, uint32_t freq, uint8_t qual, char *sgl, char *snr, uint8_t sgl_p, uint8_t snr_p, gboolean fe_lock, Dvb5Win *win )
{
	char *size = NULL;
	g_signal_emit_by_name ( win->zap, "zap-get-size", &size );

	win->fe_lock = fe_lock;
	g_signal_emit_by_name ( win->status, "status-update", freq, size, qual, sgl, snr, sgl_p, snr_p, fe_lock );

	free ( size );
}

static void dvb5_handler_stats_org ( G_GNUC_UNUSED Dvb *dvb, int num, char *text, Dvb5Win *win )
{
	g_signal_emit_by_name ( win->status, "status-org", num, text );
}

static void dvb5_handler_scan_af ( G_GNUC_UNUSED Scan *scan, const char *name_a, uint8_t a, const char *name_f, uint8_t f, const char *name_d, uint8_t d, Dvb5Win *win )
{
	g_signal_emit_by_name ( win->dvb, "dvb-info", a, f );

	win->adapter = a;
	win->frontend = f;
	win->demux = d;

	g_message ( "%s:: %s = %u, %s = %u, %s = %u ", __func__, name_a, a, name_f, f,name_d, d );
}

static void dvb5_win_init ( Dvb5Win *win )
{
	win->dvb = dvb_new ();
	win->fe_lock = FALSE;
	win->adapter = 0, win->demux = 0, win->frontend = 0;

	g_signal_connect ( win->dvb, "dvb-name",      G_CALLBACK ( dvb5_handler_dvb_name  ), win );
	g_signal_connect ( win->dvb, "dvb-scan-info", G_CALLBACK ( dvb5_handler_scan_info ), win );
	g_signal_connect ( win->dvb, "stats-update",  G_CALLBACK ( dvb5_handler_stats_upd ), win );
	g_signal_connect ( win->dvb, "stats-org",     G_CALLBACK ( dvb5_handler_stats_org ), win );

	win->zap    = zap_new  ();
	win->scan   = scan_new ();
	win->status = status_new ();

	g_signal_connect ( win->zap,    "zap-set-data",    G_CALLBACK ( dvb5_handler_zap_data    ), win );
	g_signal_connect ( win->zap,    "zap-get-felock",  G_CALLBACK ( dvb5_handler_zap_lock    ), win );
	g_signal_connect ( win->zap,    "zap-get-adapter", G_CALLBACK ( dvb5_handler_zap_adapter ), win );
	g_signal_connect ( win->zap,    "zap-get-demux",   G_CALLBACK ( dvb5_handler_zap_demux   ), win );
	g_signal_connect ( win->scan,   "scan-set-af",     G_CALLBACK ( dvb5_handler_scan_af     ), win );
	g_signal_connect ( win->scan,   "scan-set-data",   G_CALLBACK ( dvb5_handler_scan_data   ), win );
	g_signal_connect ( win->status, "scan-stop",       G_CALLBACK ( dvb5_handler_scan_stop   ), win );
	g_signal_connect ( win->status, "scan-start",      G_CALLBACK ( dvb5_handler_scan_start  ), win );
	g_signal_connect ( win->status, "win-info",        G_CALLBACK ( dvb5_handler_win_info    ), win );
	g_signal_connect ( win->status, "win-close",       G_CALLBACK ( dvb5_handler_win_close   ), win );

	dvb5_win_create ( win );

	g_signal_emit_by_name ( win->dvb, "dvb-info", win->adapter, win->frontend );
}

static void dvb5_win_finalize ( GObject *object )
{
	Dvb5Win *win = DVB5_WIN ( object );

	g_object_unref ( win->dvb );

	G_OBJECT_CLASS ( dvb5_win_parent_class )->finalize ( object );
}

static void dvb5_win_class_init ( Dvb5WinClass *class )
{
	GObjectClass *oclass = G_OBJECT_CLASS (class);

	oclass->finalize = dvb5_win_finalize;
}

Dvb5Win * dvb5_win_new ( Dvb5App *app )
{
	Dvb5Win *win = g_object_new ( DVB5_TYPE_WIN, "application", app, NULL );

	return win;
}
