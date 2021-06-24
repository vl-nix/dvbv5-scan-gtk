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

#define SCAN_TYPE_GRID scan_get_type ()

G_DECLARE_FINAL_TYPE ( Scan, scan, SCAN, GRID, GtkGrid )

Scan * scan_new ( void );

