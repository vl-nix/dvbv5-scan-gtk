/*
* Copyright 2021 Stepan Perun
* This program is free software.
*
* License: Gnu General Public License GPL-2
* file:///usr/share/common-licenses/GPL-2
* http://www.gnu.org/licenses/gpl-2.0.html
*/

#include "file.h"

void dvb5_message_dialog ( const char *error, const char *file_or_info, GtkMessageType mesg_type, GtkWindow *window )
{
	GtkMessageDialog *dialog = ( GtkMessageDialog *)gtk_message_dialog_new (
		window, GTK_DIALOG_MODAL, mesg_type, GTK_BUTTONS_CLOSE, "%s\n%s", error, file_or_info );

	gtk_dialog_run     ( GTK_DIALOG ( dialog ) );
	gtk_widget_destroy ( GTK_WIDGET ( dialog ) );
}

char * uri_get_path ( const char *uri )
{
	char *path = NULL;

	GFile *file = g_file_new_for_uri ( uri );

	path = g_file_get_path ( file );

	g_object_unref ( file );

	return path;
}

static char * file_open_save ( const char *path, const char *file, const char *accept, const char *icon, uint8_t num, GtkWindow *window )
{
	GtkFileChooserDialog *dialog = ( GtkFileChooserDialog *)gtk_file_chooser_dialog_new (
		" ", window, num, "gtk-cancel", GTK_RESPONSE_CANCEL, accept, GTK_RESPONSE_ACCEPT, NULL );

	gtk_window_set_icon_name ( GTK_WINDOW ( dialog ), icon );

	gtk_file_chooser_set_current_folder ( GTK_FILE_CHOOSER ( dialog ), path );

	if ( file )
		gtk_file_chooser_set_current_name ( GTK_FILE_CHOOSER ( dialog ), file );
	else
		gtk_file_chooser_set_select_multiple ( GTK_FILE_CHOOSER ( dialog ), FALSE );

	char *filename = NULL;

	if ( gtk_dialog_run ( GTK_DIALOG ( dialog ) ) == GTK_RESPONSE_ACCEPT )
		filename = gtk_file_chooser_get_filename ( GTK_FILE_CHOOSER ( dialog ) );

	gtk_widget_destroy ( GTK_WIDGET ( dialog ) );

	return filename;
}

char * file_open ( const char *dir, GtkWindow *window )
{
	char *file = file_open_save ( dir, NULL, "gtk-open", "document-open", GTK_FILE_CHOOSER_ACTION_OPEN, window );

	return file;
}

char * file_save ( const char *dir, GtkWindow *window )
{
	char *file = file_open_save ( dir, "dvb_channel.conf", "gtk-save", "document-save", GTK_FILE_CHOOSER_ACTION_SAVE, window );

	return file;
}
