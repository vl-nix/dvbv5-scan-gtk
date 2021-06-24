/*
* Copyright 2021 Stepan Perun
* This program is free software.
*
* License: Gnu General Public License GPL-2
* file:///usr/share/common-licenses/GPL-2
* http://www.gnu.org/licenses/gpl-2.0.html
*/

#pragma once

#include <linux/dvb/dmx.h>
#include <libdvbv5/dvb-dev.h>
#include <libdvbv5/dvb-scan.h>
#include <libdvbv5/dvb-file.h>
#include <libdvbv5/dvb-demux.h>
#include <libdvbv5/dvb-v5-std.h>

#include <glib-object.h>

#define DVB_TYPE_OBJECT dvb_get_type ()

G_DECLARE_FINAL_TYPE ( Dvb, dvb, DVB, OBJECT, GObject )

Dvb * dvb_new ( void );

