/*
* Copyright 2021 Stepan Perun
* This program is free software.
*
* License: Gnu General Public License GPL-2
* file:///usr/share/common-licenses/GPL-2
* http://www.gnu.org/licenses/gpl-2.0.html
*/

#include "lnb.h"

#include <string.h>

typedef struct _DvbLnb DvbLnb;

struct _DvbLnb
{
	uint8_t descr_num;
	const char *abr;
	const char *name;
	const char *desc;
};

const DvbLnb dvb_lnb_type_n[] =
{
	{ LNB_EXT,  "EXT",   "EXTENDED", "Freqs: 10700 to 11700 MHz, LO: 9750 MHz\nFreqs: 11700 to 12750 MHz, LO: 10600 MHz" },
	{ LNB_UNV,  "UNV",   "UNIVERSAL", "Freqs: 10800 to 11800 MHz, LO: 9750 MHz\nFreqs: 11600 to 12700 MHz, LO: 10600 MHz" },
	{ LNB_DBS,  "DBS",   "DBS", "Freqs: 12200 to 12700 MHz, LO: 11250 MHz" },
	{ LNB_STD,  "STD",   "STANDARD", "Freqs: 10945 to 11450 MHz, LO: 10000 MHz" },
	{ LNB_L170, "L1070", "L10700", "Freqs: 11750 to 12750 MHz, LO: 10700 MHz" },
	{ LNB_L175, "L1075", "L10750", "Freqs: 11700 to 12200 MHz, LO: 10750 MHz" },
	{ LNB_L113, "L1130", "L11300", "Freqs: 12250 to 12750 MHz, LO: 11300 MHz" },
	{ LNB_ENH,  "ENH",   "ENHANCED", "Freqs: 10700 to 11700 MHz, LO: 9750 MHz" },
	{ LNB_QPH,  "QPH",   "QPH031", "Freqs: 11700 to 12200 MHz, LO: 10750 MHz\nFreqs: 12200 to 12700 MHz, LO: 11250 MHz" },
	{ LNB_CBD,  "CBD",   "C-BAND", "Freqs: 3700 to 4200 MHz, LO: 5150 MHz" },
	{ LNB_CMT,  "CMT",   "C-MULT", "Right: 3700 to 4200 MHz, LO: 5150 MHz\nLeft: 3700 to 4200 MHz, LO: 5750 MHz" },
	{ LNB_DSH,  "DSH",   "DISHPRO", "Vertical: 12200 to 12700 MHz, LO: 11250 MHz\nHorizontal: 12200 to 12700 MHz, LO: 14350 MHz" },
	{ LNB_BSJ,  "BSJ",   "110BS", "Freqs: 11710 to 12751 MHz, LO: 10678 MHz" },
	{ LNB_SBRS, "STBRS", "STACKED-BRASILSAT", "Horizontal: 10700 to 11700 MHz, LO: 9710 MHz\nHorizontal: 10700 to 11700 MHz, LO: 9750 MHz" },
	{ LNB_OBRS, "OIBRS", "OI-BRASILSAT", "Freqs: 10950 to 11200 MHz, LO: 10000 MHz\nFreqs: 11800 to 12200 MHz, LO: 10445 MHz" },
	{ LNB_AMZ3, "AMZ3",  "AMAZONAS", "Vertical: 11037 to 11450 MHz, LO: 9670 MHz\nHorizontal: 11770 to 12070 MHz, LO: 9922 MHz\nHorizontal: 10950 to 11280 MHz, LO: 10000 MHz" },
	{ LNB_AMZ2, "AMZ2",  "AMAZONAS", "Vertical: 11037 to 11360 MHz, LO: 9670 MHz\nHorizontal: 11780 to 12150 MHz, LO: 10000 MHz\nHorizontal: 10950 to 11280 MHz, LO: 10000 MHz" },
	{ LNB_GBRS, "GVBRS", "GVT-BRASILSAT", "Vertical: 11010 to 11067 MHz, LO: 12860 MHz\nVertical: 11704 to 11941 MHz, LO: 13435 MHz\nHorizontal: 10962 to 11199 MHz, LO: 13112 MHz\nHorizontal: 11704 to 12188 MHz, LO: 13138 MHz" }
};

const char * lnb_get_abr ( uint8_t num )
{
	return dvb_lnb_type_n[num].abr;

}

const char * lnb_get_name ( uint8_t num )
{
	return dvb_lnb_type_n[num].name;

}

const char * lnb_get_desc ( const char *lnb_name )
{
	uint8_t c = 0; for ( c = 0; c < LNB_ALL; c++ )
	{
		if ( strcmp ( dvb_lnb_type_n[c].name, lnb_name ) == 0 ) return dvb_lnb_type_n[c].desc;
	}

	return "None";
}

