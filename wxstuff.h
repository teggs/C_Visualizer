//  Written by Chris.McDonald@uwa.edu.au, 2016-

//  For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

//  for all others, include the necessary headers (this file is usually all
//  you need because it includes almost all "standard" wxWidgets headers)
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <ctype.h>
#include <math.h>
#include <wx/tokenzr.h>
#include <map>

#if	defined(__linux__)
extern	char			*strdup(const char *s);
#endif

#define	UNUSED(identifier)	/* identifier */

#define larrow                  L"\u2190"
#define rarrow                  L"\u2192"

// vim:set sw=4 ts=8: 
