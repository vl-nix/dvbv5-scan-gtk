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

#define ZAP_TYPE_BOX zap_get_type ()

G_DECLARE_FINAL_TYPE ( Zap, zap, ZAP, BOX, GtkBox )

Zap * zap_new ( void );

