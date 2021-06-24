/*
* Copyright 2021 Stepan Perun
* This program is free software.
*
* License: Gnu General Public License GPL-2
* file:///usr/share/common-licenses/GPL-2
* http://www.gnu.org/licenses/gpl-2.0.html
*/

#include "zap.h"
#include "file.h"

#include <linux/dvb/dmx.h>

#include <libdvbv5/dvb-file.h>

enum cols_n
{
	COL_NUM,
	COL_CHL,
	NUM_COLS
};

typedef struct _OutDemux OutDemux;

struct _OutDemux
{
	uint8_t descr_num;
	const char *name;
};

const OutDemux out_demux_n[] =
{
	{ DMX_OUT_DECODER, 	"DMX_OUT_DECODER" 	},
	{ DMX_OUT_TAP, 		"DMX_OUT_TAP"		},
	{ DMX_OUT_TS_TAP, 	"DMX_OUT_TS_TAP" 	},
	{ DMX_OUT_TSDEMUX_TAP, 	"DMX_OUT_TSDEMUX_TAP" 	}

};

struct _Zap
{
	GtkBox parent_instance;

	GtkTreeView *treeview;
	GtkEntry *entry_file;
	GtkComboBoxText *combo_dmx;
};

G_DEFINE_TYPE ( Zap, zap, GTK_TYPE_BOX )

static void zap_treeview_append ( const char *channel, Zap *zap )
{
	GtkTreeIter iter;
	GtkTreeModel *model = gtk_tree_view_get_model ( zap->treeview );

	int ind = gtk_tree_model_iter_n_children ( model, NULL );
	if ( ind >= UINT16_MAX ) return;

	gtk_list_store_append ( GTK_LIST_STORE ( model ), &iter );
	gtk_list_store_set    ( GTK_LIST_STORE ( model ), &iter,
				COL_NUM, ind + 1,
				COL_CHL, channel,
				-1 );
}

static void zap_signal_trw_act ( GtkTreeView *tree_view, GtkTreePath *path, G_GNUC_UNUSED GtkTreeViewColumn *column, Zap *zap )
{
	GtkTreeIter iter;
	GtkTreeModel *model = gtk_tree_view_get_model ( tree_view );

	if ( !gtk_tree_model_get_iter ( model, &iter, path ) ) return;

	uint8_t num_dmx = (uint8_t)gtk_combo_box_get_active ( GTK_COMBO_BOX ( zap->combo_dmx ) );

	uint8_t descr_num = out_demux_n[num_dmx].descr_num; // enum dmx_output

	const char *file = gtk_entry_get_text ( zap->entry_file );

	g_autofree char *channel = NULL;
	gtk_tree_model_get ( model, &iter, COL_CHL, &channel, -1 );

	g_signal_emit_by_name ( zap, "zap-set-data", descr_num, channel, file );
}

static GtkScrolledWindow * zap_create_treeview_scroll ( Zap *zap )
{
	GtkScrolledWindow *scroll = (GtkScrolledWindow *)gtk_scrolled_window_new ( NULL, NULL );
	gtk_scrolled_window_set_policy ( scroll, GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC );
	gtk_widget_set_visible ( GTK_WIDGET ( scroll ), TRUE );

	GtkListStore *store = gtk_list_store_new ( NUM_COLS, G_TYPE_UINT, G_TYPE_STRING );

	zap->treeview = (GtkTreeView *)gtk_tree_view_new_with_model ( GTK_TREE_MODEL ( store ) );
	gtk_drag_dest_set ( GTK_WIDGET ( zap->treeview ), GTK_DEST_DEFAULT_ALL, NULL, 0, GDK_ACTION_COPY );
	gtk_drag_dest_add_uri_targets  ( GTK_WIDGET ( zap->treeview ) );
	gtk_widget_set_visible ( GTK_WIDGET ( zap->treeview ), TRUE );

	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	struct Column { const char *name; const char *type; uint8_t num; } column_n[] =
	{
		{ "Num",        "text",   COL_NUM },
		{ "Channel",    "text",   COL_CHL }
	};

	uint8_t c = 0; for ( c = 0; c < G_N_ELEMENTS ( column_n ); c++ )
	{
		renderer = gtk_cell_renderer_text_new ();

		column = gtk_tree_view_column_new_with_attributes ( column_n[c].name, renderer, column_n[c].type, column_n[c].num, NULL );
		gtk_tree_view_append_column ( zap->treeview, column );
	}

	gtk_container_add ( GTK_CONTAINER ( scroll ), GTK_WIDGET ( zap->treeview ) );
	g_object_unref ( G_OBJECT (store) );

	return scroll;
}

static void zap_combo_dmx_add ( uint8_t n_elm, Zap *zap )
{
	uint8_t c = 0; for ( c = 0; c < n_elm; c++ )
		gtk_combo_box_text_append_text ( zap->combo_dmx, out_demux_n[c].name );

	gtk_combo_box_set_active ( GTK_COMBO_BOX ( zap->combo_dmx ), 2 );
}

static gboolean zap_signal_parse_dvb_file ( const char *file, Zap *zap )
{
	if ( file == NULL ) return FALSE;
	if ( !g_file_test ( file, G_FILE_TEST_EXISTS ) ) return FALSE;

	struct dvb_file *dvb_file;
	struct dvb_entry *entry;

	dvb_file = dvb_read_file_format ( file, 0, FILE_DVBV5 );

	if ( !dvb_file )
	{
		g_critical ( "%s:: Read file format ( only DVBV5 ) failed.", __func__ );
		return FALSE;
	}

	gtk_list_store_clear ( GTK_LIST_STORE ( gtk_tree_view_get_model ( zap->treeview ) ) );

	for ( entry = dvb_file->first_entry; entry != NULL; entry = entry->next )
	{
		if ( entry->channel  ) zap_treeview_append ( entry->channel, zap );
		if ( entry->vchannel ) zap_treeview_append ( entry->vchannel, zap );
	}

	dvb_file_free ( dvb_file );

	gtk_entry_set_text ( zap->entry_file, file );

	return TRUE;
}

static void zap_signal_file_open ( GtkEntry *entry, GtkEntryIconPosition icon_pos, G_GNUC_UNUSED GdkEventButton *event, Zap *zap )
{
	if ( icon_pos == GTK_ENTRY_ICON_SECONDARY )
	{
		GtkWindow *window = GTK_WINDOW ( gtk_widget_get_toplevel ( GTK_WIDGET ( entry ) ) );

		g_autofree char *file = file_open ( g_get_home_dir (), window );

		zap_signal_parse_dvb_file ( file, zap );
	}
}

static void zap_signal_drag_in ( G_GNUC_UNUSED GtkTreeView *tree_view, GdkDragContext *ct, G_GNUC_UNUSED int x, G_GNUC_UNUSED int y,
        GtkSelectionData *s_data, G_GNUC_UNUSED uint info, guint32 time, Zap *zap )
{
	char **uris = gtk_selection_data_get_uris ( s_data );

	g_autofree char *file = uri_get_path ( uris[0] );

	zap_signal_parse_dvb_file ( file, zap );

	g_strfreev ( uris );

	gtk_drag_finish ( ct, TRUE, FALSE, time );
}

static void zap_clicked_clear ( G_GNUC_UNUSED GtkButton *button, Zap *zap )
{
	gtk_entry_set_text ( zap->entry_file, "" );
	gtk_list_store_clear ( GTK_LIST_STORE ( gtk_tree_view_get_model ( zap->treeview ) ) );
}

static void zap_init ( Zap *zap )
{
	GtkBox *box = GTK_BOX ( zap );
	gtk_orientable_set_orientation ( GTK_ORIENTABLE ( box ), GTK_ORIENTATION_VERTICAL );
	gtk_box_set_spacing ( box, 10 );
	gtk_widget_set_visible ( GTK_WIDGET ( box ), TRUE );

	gtk_widget_set_margin_top    ( GTK_WIDGET ( box ), 10 );
	gtk_widget_set_margin_bottom ( GTK_WIDGET ( box ), 10 );
	gtk_widget_set_margin_start  ( GTK_WIDGET ( box ), 10 );
	gtk_widget_set_margin_end    ( GTK_WIDGET ( box ), 10 );

	gtk_box_pack_start ( box, GTK_WIDGET ( zap_create_treeview_scroll ( zap ) ), TRUE, TRUE, 0 );

	g_signal_connect ( zap->treeview, "drag-data-received", G_CALLBACK ( zap_signal_drag_in ), zap );
	g_signal_connect ( zap->treeview, "row-activated",      G_CALLBACK ( zap_signal_trw_act ), zap );

	GtkBox *h_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );
	gtk_box_set_spacing ( h_box, 5 );
	gtk_widget_set_visible ( GTK_WIDGET ( h_box ), TRUE );


	GtkButton *button_clear = (GtkButton *)gtk_button_new_from_icon_name ( "edit-clear", GTK_ICON_SIZE_MENU );
	gtk_widget_set_visible ( GTK_WIDGET ( button_clear ), TRUE );

	zap->entry_file = (GtkEntry *)gtk_entry_new ();
	gtk_entry_set_text ( zap->entry_file, "dvb_channel.conf" );
	g_object_set ( zap->entry_file, "editable", FALSE, NULL );
	gtk_entry_set_icon_from_icon_name ( zap->entry_file, GTK_ENTRY_ICON_SECONDARY, "folder" );
	g_signal_connect ( zap->entry_file, "icon-press", G_CALLBACK ( zap_signal_file_open ), zap );

	g_signal_connect ( button_clear, "clicked", G_CALLBACK ( zap_clicked_clear ), zap );

	const char *icon = "info";
	gtk_entry_set_icon_from_icon_name ( zap->entry_file, GTK_ENTRY_ICON_PRIMARY, icon );
	gtk_entry_set_icon_tooltip_text ( GTK_ENTRY ( zap->entry_file ), GTK_ENTRY_ICON_PRIMARY, "Format only DVBV5" );
	gtk_box_pack_start ( h_box, GTK_WIDGET ( zap->entry_file ), TRUE, TRUE, 0 );

	zap->combo_dmx = (GtkComboBoxText *) gtk_combo_box_text_new ();
	zap_combo_dmx_add ( G_N_ELEMENTS ( out_demux_n ), zap );
	gtk_box_pack_start ( h_box, GTK_WIDGET ( zap->combo_dmx ), TRUE, TRUE, 0 );

	gtk_widget_set_visible ( GTK_WIDGET ( zap->entry_file ), TRUE );
	gtk_widget_set_visible ( GTK_WIDGET ( zap->combo_dmx  ), TRUE );

	gtk_box_pack_end   ( h_box, GTK_WIDGET ( button_clear ), FALSE, FALSE, 0 );

	gtk_box_pack_start ( box, GTK_WIDGET ( h_box ), FALSE, FALSE, 0 );
}

static void zap_finalize ( GObject *object )
{
	G_OBJECT_CLASS (zap_parent_class)->finalize (object);
}

static void zap_class_init ( ZapClass *class )
{
	G_OBJECT_CLASS (class)->finalize = zap_finalize;

	g_signal_new ( "zap-set-data", G_TYPE_FROM_CLASS ( class ), G_SIGNAL_RUN_LAST,
		0, NULL, NULL, NULL, G_TYPE_NONE, 3, G_TYPE_UINT, G_TYPE_STRING, G_TYPE_STRING );
}

Zap * zap_new (void)
{
	return g_object_new ( ZAP_TYPE_BOX, NULL );
}

