//
//  utime.m
//  Cindex
//
//  Created by Peter Lennie on 4/23/18.
//
#include "stdafx.h"
#include "utime.h"
#include "locales.h"
#include "unicode/udat.h"
#include "unicode/udatpg.h"

static UDateFormat * df0, *df1, *df2, *df3, *dfx;
/***************************************************************************************/
UDate time_dateValue(const char * dateString)	// parse dates in multiple formats, structured by locale

{
	const char * locale = loc_currentLocale();
	UErrorCode error = U_ZERO_ERROR;
	unichar uString[200];
	int32_t parsepos = 0;
	UDate parsedDate = 0;
	UCalendar * cal;
	
	error = U_ZERO_ERROR;
	u_strFromUTF8(uString,200,NULL,dateString,-1,&error);
	
	// try standard short first
	if (df0 == NULL)	// standard short date parser
		df0 = udat_open(UDAT_NONE,UDAT_SHORT,locale,0,-1,NULL,-1,&error);
	parsedDate = udat_parse(df0, uString, u_strlen(uString), &parsepos, &error);
	if (U_SUCCESS(error))
		return parsedDate;
	
	// try relative short
	error = U_ZERO_ERROR;
	if (df3 == NULL)
		df3 = udat_open(UDAT_NONE,UDAT_SHORT_RELATIVE,locale,0,-1,NULL,-1,&error);
	parsepos = 0;
	parsedDate = udat_parse(df3, uString, u_strlen(uString), &parsepos, &error);
	if (U_SUCCESS(error)) {
		cal = ucal_open(0,-1,locale,UCAL_GREGORIAN,&error);
		ucal_setMillis(cal,parsedDate,&error);
		ucal_clearField(cal,UCAL_AM_PM);
		ucal_clearField(cal,UCAL_HOUR);
		ucal_clearField(cal,UCAL_HOUR_OF_DAY);
		ucal_clearField(cal,UCAL_MINUTE);
		ucal_clearField(cal,UCAL_SECOND);
		parsedDate = ucal_getMillis(cal,&error);
		ucal_close(cal);
		if (U_SUCCESS(error))
			return parsedDate;
	}
	
	// try month-day (missing year, defaults to current)
	error = U_ZERO_ERROR;
	if (df1 == NULL) {		// month-day parser
		const UChar skeleton[]= {'M', 'M', 'M', 'd', 0};
		UDateTimePatternGenerator * generator = udatpg_open(locale, &error);
		unichar pattern[20];
		int plength = udatpg_getBestPattern(generator, skeleton, 4, pattern, 20, &error);
		df1 = udat_open(UDAT_PATTERN,UDAT_PATTERN,locale,0,-1,pattern,plength,&error);
		udatpg_close(generator);
	}
	cal = ucal_open(0,-1,locale,UCAL_GREGORIAN,&error);
	int year = ucal_get(cal,UCAL_YEAR,&error);
	ucal_setDateTime(cal,year,UCAL_JANUARY,1,0,0,0,&error);
//	NSLog(@"BaseDate: %s",time_stringFromDate(ucal_getMillis(cal,&error)));
	parsepos = 0;
	udat_parseCalendar(df1,cal,uString ,u_strlen(uString), &parsepos, &error);
	parsedDate = ucal_getMillis(cal,&error);
	ucal_close(cal);
	if (U_SUCCESS(error))
		return parsedDate;
	
	// try standard short time
	error = U_ZERO_ERROR;
	if (df2 == NULL)	{
		df2 = udat_open(UDAT_SHORT,UDAT_NONE,locale,0,-1,NULL,-1,&error);
	}
	cal = ucal_open(0,-1,locale,UCAL_GREGORIAN,&error);
	ucal_clearField(cal,UCAL_SECOND);
//	NSLog(@"BaseMinute: %s",time_stringFromDate(ucal_getMillis(cal,&error)));
	parsepos = 0;
	udat_parseCalendar(df2,cal,uString ,u_strlen(uString), &parsepos, &error);
	parsedDate = ucal_getMillis(cal,&error);
	ucal_close(cal);
	return U_SUCCESS(error) ? parsedDate : 0;
}
/***************************************************************************************/
char * time_stringFromDate(UDate date)

{
	const char * locale = loc_currentLocale();
	UErrorCode error = U_ZERO_ERROR;
	unichar dest[200];
	static char cdest[200];
	
	if (dfx == NULL)
		dfx = udat_open(UDAT_MEDIUM,UDAT_MEDIUM,locale,0,-1,NULL,-1,&error);
	error = U_ZERO_ERROR;
	udat_format(dfx,date,dest,200,NULL,&error);
	error = U_ZERO_ERROR;
	u_strToUTF8(cdest,200,NULL,dest,-1,&error);
	return cdest;
}
/******************************************************************************/
char * time_stringFromTime(time_c time, BOOL local)	// returns time string

{
	static char ts[40];
	time_t ttime = time;
	struct tm * rectime = local ? localtime(&ttime) : gmtime(&ttime);

	if (rectime) {
		sprintf(ts, "%d-%02d-%02dT%02d:%02d:%02d", rectime->tm_year + 1900, rectime->tm_mon + 1, rectime->tm_mday, rectime->tm_hour, rectime->tm_min, rectime->tm_sec);
		return ts;
	}
	return "1970-01-01T00:00:00";
}
