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

#define DVB5_TYPE_APP dvb5_app_get_type ()

G_DECLARE_FINAL_TYPE ( Dvb5App, dvb5_app, DVB5, APP, GtkApplication )

Dvb5App * dvb5_app_new ( void );

