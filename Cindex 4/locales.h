//
//  locales.h
//  Cindex
//
//  Created by Peter Lennie on 1/6/11.
//  Copyright 2011 Peter Lennie. All rights reserved.
//

#pragma once
typedef struct {
	char * BCPid;
	char * name;
} LOCALEDATA;

const char * loc_currentLocale(void);
char * displayNameForLocale(char *identifier);
BOOL localeSameLanguage(char * root1, char * root2);
uint32_t LCIDforCurrentLocale(void);	// returns windows language code
uint32_t LCIDforLocale(char * sortLocale);	// returns windows language code for named locale
