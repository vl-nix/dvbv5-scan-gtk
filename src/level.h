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

#define LEVEL_TYPE_BOX level_get_type ()

G_DECLARE_FINAL_TYPE ( Level, level, LEVEL, BOX, GtkBox )

Level * level_new ( void );
