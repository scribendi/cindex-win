//
//  locales.m
//  Cindex
//
//  Created by Peter Lennie on 1/6/11.
//  Copyright 2011 Peter Lennie. All rights reserved.
//

#include "stdafx.h"
#include "locales.h"
#include "unicode/uloc.h"
#include "util.h"

/****************************************************************************************/
const char * loc_currentLocale(void)

{
#if defined CIN_MAC_OS
	return[[[NSLocale autoupdatingCurrentLocale] localeIdentifier] UTF8String];
#else
	UErrorCode error = U_ZERO_ERROR;
	TCHAR buffer[200];
	static char dest[200];

	GetLocaleInfoEx(LOCALE_NAME_USER_DEFAULT, LOCALE_SNAME,buffer,sizeof(buffer));
	u_strToUTF8(dest, 200, NULL, buffer, -1, &error);
	return dest;
#endif
}
/****************************************************************************************/
char * displayNameForLocale(char * identifier)

{
	static char displayname[100];
	unichar buffer[100];
	UErrorCode error = U_ZERO_ERROR;
	int dlength;
	
	uloc_getDisplayName(identifier,"en_US",buffer,100,&error);
//	NSLog(@"%s:%S",identifier,buffer);
	error = U_ZERO_ERROR;
	 u_strToUTF8(displayname,100,&dlength,buffer,-1,&error);
//	NSLog(@"%s",displayname);
	if (U_SUCCESS(error))
		return displayname;
	return identifier;
}
/****************************************************************************************/
BOOL localeSameLanguage(char * root1, char * root2)

{
	int index;
	
	for (index = 0; isalpha(root1[index]); index++)
		;
#if defined CIN_MAC_OS 
	return !strncasecmp(root1, root2, index);
#else
	return !_strnicmp(root1, root2, index);
#endif
}
/****************************************************************************************/
uint32_t LCIDforCurrentLocale(void)	// returns windows language code

{
#if defined CIN_MAC_OS 
	NSLocale *loc = [NSLocale autoupdatingCurrentLocale];
	const char * identifier = [[loc localeIdentifier] UTF8String];
	
	return uloc_getLCID(identifier);
#else
	return GetThreadLocale();
#endif
}
#if 1
/****************************************************************************************/
uint32_t LCIDforLocale(char * sortLocale)	// returns windows language code for named locale

{
	uint32_t threadLCID = GetThreadLocale();
	uint32_t sortLCID = uloc_getLCID(sortLocale);

// do this because LCID for unadorned 'en', etc does not include sort component code, and then MS Word doesn't check spelling
	if ((threadLCID&0xFF) == (sortLCID&0xFF))	// if same base language
		sortLCID = threadLCID;	// return thread locale (has laguage subtypes)
	return sortLCID;
}
#else
/****************************************************************************************/
uint32_t LCIDforLocale(char * sortLocale)	// returns windows language code for named locale

{
	TCHAR lname[LOCALE_NAME_MAX_LENGTH];

	GetUserDefaultLocaleName(lname,LOCALE_NAME_MAX_LENGTH);
	
// do this because LCID for unadorned 'en', etc does not include sort component code, and then MS Word doesn't check spelling
	if (!strncmp(sortLocale, identifier, strlen(sortLocale)))	// if current locale is same or more specific than sort
		sortLocale = identifier;
	return uloc_getLCID(sortLocale);
}
#endif