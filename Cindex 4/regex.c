#include "stdafx.h"
#include "regex.h"

/***************************************************************************************/
BOOL regex_validexpression(char * string, int flags)		// validates pattern

{
	URegularExpression * re = regex_build(string,flags);
	if (re)	{
		uregex_close(re);
		return TRUE;
	}
	return FALSE;
}
/***************************************************************************************/
URegularExpression * regex_build(char * string, int flags)		// sets up regex from string

{
	UErrorCode error = U_ZERO_ERROR;
	UParseError pe;
	URegularExpression * re;
	
	re = uregex_openC(string,flags,&pe,&error);
	return re;
}
/***************************************************************************************/
char * regex_find(URegularExpression *regex, char * text, int offset, short * length)		// find regex match

{
	static UText ut = UTEXT_INITIALIZER;		// keep permanent Utext for reuse
	UErrorCode error = U_ZERO_ERROR;
	char * sptr = NULL;
	
	utext_openUTF8(&ut,text, -1, &error);
	if (U_SUCCESS(error)) {
		error = U_ZERO_ERROR;
		uregex_setUText(regex,&ut,&error);
		if (U_SUCCESS(error)) {
			error = U_ZERO_ERROR;
			if (uregex_find(regex,offset,&error))	{	// if a match
				int startindex, endindex;
				
				startindex = uregex_start(regex,0,&error);
				sptr = text + startindex;
				if (length)	{
					endindex = uregex_end(regex,0,&error);
					*length = endindex-startindex;
				}
			}
		}
	}
	return sptr;
}
/***************************************************************************************/
int regex_groupcount(URegularExpression *regex)		// returns number of capture groups

{
	UErrorCode error = U_ZERO_ERROR;
	
	int count =  uregex_groupCount(regex,&error);
	if (U_SUCCESS(error))
		return count;
	return -1;
}
/***************************************************************************************/
char * regex_textforgroup(URegularExpression *regex, int cgroup, char * source, int * matchlen)		// returns start & length of specified capture group

{
	UText ut = UTEXT_INITIALIZER;
	UErrorCode error = U_ZERO_ERROR;
	int64_t length = 0;
	
	*matchlen = 0;
	uregex_groupUText(regex,cgroup,&ut,&length,&error);
	if (U_SUCCESS(error))	{
		*matchlen = (int)length;
		return source+utext_getNativeIndex(&ut);
	}
	*matchlen = -1;
	return NULL;
}
/***************************************************************************************/
BOOL regex_replace(URegularExpression *regex, char * text, char * replacement)		// replaces all matches in source with replacement

{
	static UText uts = UTEXT_INITIALIZER;		// keep permanent Utext for reuse
	static UText utr = UTEXT_INITIALIZER;		// keep permanent Utext for reuse
	UErrorCode error = U_ZERO_ERROR;
	UText * utdp;

	utext_openUTF8(&uts, text, -1, &error);
	utext_openUTF8(&utr, replacement, -1, &error);
	if (U_SUCCESS(error)) {
		error = U_ZERO_ERROR;
		uregex_setUText(regex, &uts, &error);
		if (U_SUCCESS(error)) {
			error = U_ZERO_ERROR;
			utdp = uregex_replaceAllUText(regex, &utr, NULL, &error);
			if (U_SUCCESS(error)) {
				UChar tdest[MAXREC * 2];
				int32_t length = utext_extract(utdp, 0, UINT_MAX, tdest, MAXREC * 2, &error);
				u_strToUTF8(text, MAXREC, &length, tdest, length, &error);
				utext_close(utdp);
			}
		}
	}
	return U_SUCCESS(error);
}
