/*
* Copyright 2021 Stepan Perun
* This program is free software.
*
* License: Gnu General Public License GPL-2
* file:///usr/share/common-licenses/GPL-2
* http://www.gnu.org/licenses/gpl-2.0.html
*/

#define _LARGEFILE64_SOURCE

#define BUF_SIZE ( 8 * 128 * 188 )

#include "file.h"

#include <time.h>
#include <poll.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>

typedef struct _DwrRec DwrRec;

struct _DwrRec
{
	int dvr_fd;
	int rec_fd;

	GMutex mutex;
	DwrRecMonitor *drm;
};

static gpointer dvr_rec_thread ( DwrRec *dvr_rec )
{
	g_mutex_init ( &dvr_rec->mutex );

	gboolean stop = FALSE;
	uint32_t buf[BUF_SIZE];
	ssize_t r = 0, w = 0;
	uint64_t total = 0;

	time_t t_start, t_cur;
	time ( &t_start );

	struct pollfd pfd;
	pfd.fd = dvr_rec->dvr_fd;
	pfd.events = POLLIN | POLLPRI | POLLERR;

	while ( !stop )
	{
		int ret = poll ( &pfd, 1, 10 );

		if ( ret == -1 )
		{
			if ( errno == EINTR ) continue;

			printf ( "Dvr device poll failure \n" );
			break;
		}

		if ( pfd.revents == 0 ) continue;

		r = read ( dvr_rec->dvr_fd, buf, sizeof(buf) );

		if ( r < 0 )
		{
			perror ( "Read" );

			if ( errno == EOVERFLOW ) continue;

			printf ( "Read error \n" );
			break;
		}

		w = write ( dvr_rec->rec_fd, buf, (size_t)r );

		if ( w == -1 )
		{
			if ( errno != EINTR ) { printf ( "Write error: %m \n" ); break; }
		}

		total += (uint32_t)r;

		time ( &t_cur );

		if ( t_cur > t_start )
		{
			g_mutex_lock ( &dvr_rec->mutex );

			if ( dvr_rec->drm->stop_rec ) { stop = TRUE; dvr_rec->drm->total_rec = 0; total = 0; } else dvr_rec->drm->total_rec = total;

			g_mutex_unlock ( &dvr_rec->mutex );

			time ( &t_start );
		}
	}

	close ( dvr_rec->dvr_fd );
	close ( dvr_rec->rec_fd );

	g_mutex_clear ( &dvr_rec->mutex );
	free ( dvr_rec );

	return NULL;
}

const char * dvr_rec_create ( uint8_t adapter, const char *rec, DwrRecMonitor *dm )
{
	char dvrdev[PATH_MAX];
	sprintf ( dvrdev, "/dev/dvb/adapter%d/dvr0", adapter );

	int dvr_fd = open ( dvrdev, O_RDONLY );

	if ( dvr_fd == -1 )
	{
		perror ( "Cannot open dvr device" );

		return "Cannot open dvr device";
	}

	int rec_fd = open ( rec, O_WRONLY | O_CREAT | O_TRUNC | O_LARGEFILE, 0664 );

	if ( rec_fd == -1 )
	{
		perror ( "Cannot open rec file" );
		close ( dvr_fd );

		return "Cannot open rec file";
	}

	DwrRec *dvr_rec = g_new0 ( DwrRec, 1 );

	dvr_rec->drm = dm;
	dvr_rec->dvr_fd = dvr_fd;
	dvr_rec->rec_fd = rec_fd;

	GThread *thread = g_thread_new ( "dmx-rec-thread", (GThreadFunc)dvr_rec_thread, dvr_rec );
	g_thread_unref ( thread );

	return NULL;
}

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

char * file_save ( const char *dir, const char *file_save, GtkWindow *window )
{
	char *file = file_open_save ( dir, file_save, "gtk-save", "document-save", GTK_FILE_CHOOSER_ACTION_SAVE, window );

	return file;
}

char * time_to_str ( void )
{
	GDateTime *date = g_date_time_new_now_local ();

	char *str_time = g_date_time_format ( date, "%j-%Y-%T" );

	g_date_time_unref ( date );

	return str_time;
}


