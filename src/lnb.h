/*
* Copyright 2021 Stepan Perun
* This program is free software.
*
* License: Gnu General Public License GPL-2
* file:///usr/share/common-licenses/GPL-2
* http://www.gnu.org/licenses/gpl-2.0.html
*/

#pragma once

#include <stdint.h>

enum lnb_type
{
	LNB_EXT,
	LNB_UNV,
	LNB_DBS,
	LNB_STD,
	LNB_L170,
	LNB_L175,
	LNB_L113,
	LNB_ENH,
	LNB_QPH,
	LNB_CBD,
	LNB_CMT,
	LNB_DSH,
	LNB_BSJ,
	LNB_SBRS,
	LNB_OBRS,
	LNB_AMZ3,
	LNB_AMZ2,
	LNB_GBRS,
	LNB_ALL
};

const char * lnb_get_abr  ( uint8_t );

const char * lnb_get_name ( uint8_t );

const char * lnb_get_desc ( const char * );

