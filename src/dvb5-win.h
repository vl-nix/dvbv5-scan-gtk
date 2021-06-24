/*
* Copyright 2021 Stepan Perun
* This program is free software.
*
* License: Gnu General Public License GPL-2
* file:///usr/share/common-licenses/GPL-2
* http://www.gnu.org/licenses/gpl-2.0.html
*/

#pragma once

#include "dvb5-app.h"

#define DVB5_TYPE_WIN dvb5_win_get_type ()

G_DECLARE_FINAL_TYPE ( Dvb5Win, dvb5_win, DVB5, WIN, GtkWindow )

Dvb5Win * dvb5_win_new ( Dvb5App * );

