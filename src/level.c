/*
* Copyright 2021 Stepan Perun
* This program is free software.
*
* License: Gnu General Public License GPL-2
* file:///usr/share/common-licenses/GPL-2
* http://www.gnu.org/licenses/gpl-2.0.html
*/

#include "level.h"

struct _Level
{
	GtkBox parent_instance;

	GtkLabel *sgn_snr;
	GtkProgressBar *bar_sgn;
	GtkProgressBar *bar_snr;
};

G_DEFINE_TYPE ( Level, level, GTK_TYPE_BOX )

static void level_handler_update ( Level *level, uint8_t qual, char *sgl, char *snr, uint8_t sgl_gd, uint8_t snr_gd, gboolean fe_lock )
{
	gtk_progress_bar_set_fraction ( level->bar_sgn, (double)sgl_gd / 100 );
	gtk_progress_bar_set_fraction ( level->bar_snr, (double)snr_gd / 100 );

	const char *text_q = "bfbfbf";
	if ( qual == 3 ) text_q = "ff00ff"; // Good - Magenta
	if ( qual == 2 ) text_q = "00ffff"; // Ok   - Aqua
	if ( qual == 1 ) text_q = "ff9000"; // Poor - Orange

	const char *text_l = "bfbfbf";
	if ( fe_lock  ) text_l = "00ff00"; else text_l = "ff0000";
	if ( sgl_gd == 0 && snr_gd == 0 ) text_l = "bfbfbf";

	g_autofree char *markup = g_markup_printf_escaped 
		( "Quality<span foreground=\"#%s\">  ◉  </span>%s<span foreground=\"#%s\">  ◉  </span>%s", text_q, sgl, text_l, snr );

	gtk_label_set_markup ( level->sgn_snr, markup );
}

static void level_init ( Level *level )
{
	GtkBox *box = GTK_BOX ( level );
	gtk_orientable_set_orientation ( GTK_ORIENTABLE ( box ), GTK_ORIENTATION_VERTICAL );
	gtk_box_set_spacing ( box, 3 );

	level->sgn_snr = (GtkLabel *)gtk_label_new ( "Quality  ◉  Signal  ◉  C/N" );
	level->bar_sgn = (GtkProgressBar *)gtk_progress_bar_new ();
	level->bar_snr = (GtkProgressBar *)gtk_progress_bar_new ();

	gtk_widget_set_visible ( GTK_WIDGET ( level->sgn_snr ), TRUE );
	gtk_widget_set_visible ( GTK_WIDGET ( level->bar_sgn ), TRUE );
	gtk_widget_set_visible ( GTK_WIDGET ( level->bar_snr ), TRUE );
	gtk_widget_set_visible ( GTK_WIDGET ( box ), TRUE );

	gtk_box_pack_start ( box, GTK_WIDGET ( level->sgn_snr ), FALSE, FALSE, 0 );
	gtk_box_pack_start ( box, GTK_WIDGET ( level->bar_sgn ), FALSE, FALSE, 0 );
	gtk_box_pack_start ( box, GTK_WIDGET ( level->bar_snr ), FALSE, FALSE, 0 );

	g_signal_connect ( level, "level-update", G_CALLBACK ( level_handler_update ), NULL );
}

static void level_finalize ( GObject *object )
{
	G_OBJECT_CLASS (level_parent_class)->finalize (object);
}

static void level_class_init ( LevelClass *class )
{
	G_OBJECT_CLASS (class)->finalize = level_finalize;

	g_signal_new ( "level-update", G_TYPE_FROM_CLASS ( class ), G_SIGNAL_RUN_LAST,
		0, NULL, NULL, NULL, G_TYPE_NONE, 6, G_TYPE_UINT, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_UINT, G_TYPE_UINT, G_TYPE_BOOLEAN );
}

Level * level_new ( void )
{
	return g_object_new ( LEVEL_TYPE_BOX, NULL );
}

