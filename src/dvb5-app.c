/*
* Copyright 2021 Stepan Perun
* This program is free software.
*
* License: Gnu General Public License GPL-2
* file:///usr/share/common-licenses/GPL-2
* http://www.gnu.org/licenses/gpl-2.0.html
*/

#include "dvb5-app.h"
#include "dvb5-win.h"

struct _Dvb5App
{
	GtkApplication  parent_instance;
};

G_DEFINE_TYPE ( Dvb5App, dvb5_app, GTK_TYPE_APPLICATION )

static void dvb5_app_activate ( GApplication *app )
{
	dvb5_win_new ( DVB5_APP ( app ) );
}

static void dvb5_app_init ( G_GNUC_UNUSED Dvb5App *dvb5_app )
{
	
}

static void dvb5_app_finalize ( GObject *object )
{
	G_OBJECT_CLASS (dvb5_app_parent_class)->finalize (object);
}

static void dvb5_app_class_init ( Dvb5AppClass *class )
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);

	G_APPLICATION_CLASS (class)->activate = dvb5_app_activate;

	object_class->finalize = dvb5_app_finalize;
}

Dvb5App * dvb5_app_new ( void )
{
	return g_object_new ( DVB5_TYPE_APP, /*"application-id", "org.gnome.dvbv5",*/ "flags", G_APPLICATION_NON_UNIQUE, NULL );
}
