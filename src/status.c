/*
* Copyright 2021 Stepan Perun
* This program is free software.
*
* License: Gnu General Public License GPL-2
* file:///usr/share/common-licenses/GPL-2
* http://www.gnu.org/licenses/gpl-2.0.html
*/

#include "status.h"
#include "level.h"

struct _Status
{
	GtkBox parent_instance;

	Level *level;

	GtkLabel *dvb_name;
	GtkLabel *freq_scan;
};

G_DEFINE_TYPE ( Status, status, GTK_TYPE_BOX )

static void status_handler_update ( Status *status, uint32_t freq, uint8_t qual, char *sgl, char *snr, uint8_t sgl_gd, uint8_t snr_gd, gboolean fe_lock )
{
	char text[100];
	sprintf ( text, "Freq:  %d ", freq );

	gtk_label_set_text ( status->freq_scan, ( freq ) ? text : "" );

	g_signal_emit_by_name ( status->level, "level-update", qual, sgl, snr, sgl_gd, snr_gd, fe_lock );
}

static void status_handler_set_dvb_name ( Status *status, const char *dvb_name )
{
	gtk_label_set_text ( status->dvb_name, dvb_name );
}

static void status_clicked_scan ( G_GNUC_UNUSED GtkButton *button, Status *status )
{
	g_signal_emit_by_name ( status, "scan-start" );
}

static void status_clicked_stop ( G_GNUC_UNUSED GtkButton *button, Status *status )
{
	g_signal_emit_by_name ( status, "scan-stop" );
}

static void status_clicked_info ( G_GNUC_UNUSED GtkButton *button, Status *status )
{
	g_signal_emit_by_name ( status, "win-info" );
}

static void status_clicked_exit ( G_GNUC_UNUSED GtkButton *button, Status *status )
{
	g_signal_emit_by_name ( status, "win-close" );
}

static void status_init ( Status *status )
{
	GtkBox *box = GTK_BOX ( status );
	gtk_orientable_set_orientation ( GTK_ORIENTABLE ( box ), GTK_ORIENTATION_VERTICAL );
	gtk_widget_set_visible ( GTK_WIDGET ( box ), TRUE );

	gtk_widget_set_margin_top    ( GTK_WIDGET ( box ), 10 );
	gtk_widget_set_margin_bottom ( GTK_WIDGET ( box ), 10 );
	gtk_widget_set_margin_start  ( GTK_WIDGET ( box ), 10 );
	gtk_widget_set_margin_end    ( GTK_WIDGET ( box ), 10 );

	status->dvb_name = (GtkLabel *)gtk_label_new ( "Dvb Device" );
	gtk_box_pack_start ( box, GTK_WIDGET ( status->dvb_name ), FALSE, FALSE, 0 );
	gtk_widget_set_visible (  GTK_WIDGET ( status->dvb_name ), TRUE );

	GtkBox *h_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );
	gtk_box_set_spacing ( h_box, 5 );
	gtk_widget_set_visible (  GTK_WIDGET ( h_box ), TRUE );

	GtkButton *bscan = (GtkButton *)gtk_button_new_with_label ( "⏵" );
	GtkButton *bstop = (GtkButton *)gtk_button_new_with_label ( "⏹" );
	GtkButton *binfo = (GtkButton *)gtk_button_new_with_label ( "🛈" );
	GtkButton *bexit = (GtkButton *)gtk_button_new_with_label ( "⏻" );

	g_signal_connect ( bscan, "clicked", G_CALLBACK ( status_clicked_scan ), status );
	g_signal_connect ( bstop, "clicked", G_CALLBACK ( status_clicked_stop ), status );
	g_signal_connect ( binfo, "clicked", G_CALLBACK ( status_clicked_info ), status );
	g_signal_connect ( bexit, "clicked", G_CALLBACK ( status_clicked_exit ), status );

	gtk_widget_set_visible (  GTK_WIDGET ( bscan ), TRUE );
	gtk_widget_set_visible (  GTK_WIDGET ( bstop ), TRUE );
	gtk_widget_set_visible (  GTK_WIDGET ( binfo ), TRUE );
	gtk_widget_set_visible (  GTK_WIDGET ( bexit ), TRUE );

	gtk_box_pack_start ( h_box, GTK_WIDGET ( bscan  ), TRUE, TRUE, 0 );
	gtk_box_pack_start ( h_box, GTK_WIDGET ( bstop  ), TRUE, TRUE, 0 );
	gtk_box_pack_start ( h_box, GTK_WIDGET ( binfo  ), TRUE, TRUE, 0 );
	gtk_box_pack_start ( h_box, GTK_WIDGET ( bexit  ), TRUE, TRUE, 0 );

	gtk_box_pack_end ( box, GTK_WIDGET ( h_box ), FALSE, FALSE, 5 );

	status->level = level_new ();
	gtk_box_pack_end ( box, GTK_WIDGET ( status->level ), FALSE, FALSE, 5 );

	status->freq_scan = (GtkLabel *)gtk_label_new ( "" );
	gtk_widget_set_halign ( GTK_WIDGET ( status->freq_scan ), GTK_ALIGN_START );
	gtk_box_pack_end ( box, GTK_WIDGET ( status->freq_scan ), FALSE, FALSE, 0 );
	gtk_widget_set_visible (  GTK_WIDGET ( status->freq_scan ), TRUE );

	g_signal_connect ( status, "set-dvb-name",  G_CALLBACK ( status_handler_set_dvb_name  ), NULL );
	g_signal_connect ( status, "status-update", G_CALLBACK ( status_handler_update ), NULL );	
}

static void status_finalize ( GObject *object )
{
	G_OBJECT_CLASS (status_parent_class)->finalize (object);
}

static void status_class_init ( StatusClass *class )
{
	G_OBJECT_CLASS (class)->finalize = status_finalize;

	g_signal_new ( "scan-start", G_TYPE_FROM_CLASS ( class ), G_SIGNAL_RUN_LAST,
		0, NULL, NULL, NULL, G_TYPE_NONE, 0 );

	g_signal_new ( "scan-stop", G_TYPE_FROM_CLASS ( class ), G_SIGNAL_RUN_LAST,
		0, NULL, NULL, NULL, G_TYPE_NONE, 0 );

	g_signal_new ( "win-info", G_TYPE_FROM_CLASS ( class ), G_SIGNAL_RUN_LAST,
		0, NULL, NULL, NULL, G_TYPE_NONE, 0 );

	g_signal_new ( "win-close", G_TYPE_FROM_CLASS ( class ), G_SIGNAL_RUN_LAST,
		0, NULL, NULL, NULL, G_TYPE_NONE, 0 );

	g_signal_new ( "set-dvb-name", G_TYPE_FROM_CLASS ( class ), G_SIGNAL_RUN_LAST,
		0, NULL, NULL, NULL, G_TYPE_NONE, 1, G_TYPE_STRING );

	g_signal_new ( "status-update", G_TYPE_FROM_CLASS ( class ), G_SIGNAL_RUN_LAST,
		0, NULL, NULL, NULL, G_TYPE_NONE, 7, G_TYPE_UINT, G_TYPE_UINT, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_UINT, G_TYPE_UINT, G_TYPE_BOOLEAN );
}

Status * status_new (void)
{
	return g_object_new ( STATUS_TYPE_BOX, NULL );
}

