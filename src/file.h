/*
* Copyright 2021 Stepan Perun
* This program is free software.
*
* License: Gnu General Public License GPL-2
* file:///usr/share/common-licenses/GPL-2
* http://www.gnu.org/licenses/gpl-2.0.html
*/

#pragma once

#include <gtk/gtk.h>

char * uri_get_path ( const char * );

char * file_open ( const char *, GtkWindow * );

char * file_save ( const char *, GtkWindow * );

void dvb5_message_dialog ( const char *, const char *, GtkMessageType , GtkWindow * );

