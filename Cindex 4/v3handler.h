//
//  v3handler.h
//  Cindex
//
//  Created by Peter Lennie on 1/17/11.
//  Copyright 2011 Peter Lennie. All rights reserved.
//

#pragma once

#include "iconv.h"

//TCHAR * v3_warnings(TCHAR * warnstring);	// forms warning message
char * v3_warnings(char * warnstring);	// forms warning message
BOOL v3_convertabbrev(TCHAR * path, int type);	// converts abbrev file
BOOL v3_convertstylesheet(TCHAR * path, int type);	// converts style sheet
BOOL v3_convertindex(TCHAR * path, int type);		// converts preunicode index to V3
int v3_convertrecord(char * xstring, FONTMAP * fm, BOOL replace);	// converts xstring to UTF-8
iconv_t v3_openconverter(char * ctype);	// opens converter
