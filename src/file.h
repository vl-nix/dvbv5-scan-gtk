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

typedef long long unsigned int lluint;

char * uri_get_path ( const char * );

char * file_open ( const char *, GtkWindow * );

char * file_save ( const char *, const char *, GtkWindow * );

void dvb5_message_dialog ( const char *, const char *, GtkMessageType , GtkWindow * );

void dvr_rec_stop   ( void );

lluint dvr_rec_get_size ( void );

const char * dvr_rec_create ( uint8_t , const char * );
