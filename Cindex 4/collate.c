/*
 *  collate.m
 *  Cindex
 *
 *  Created by Peter Lennie on 12/31/10.
 *  Copyright 2010 Peter Lennie. All rights reserved.
 *
 */

#include "stdafx.h"
#include "collate.h"
#include "sort.h"
#include "strings.h"
#include "unicode/utypes.h"
#include "unicode/ucal.h"
#include "unicode/udat.h"
#include "util.h"

#define LOWCHR0 0xA
#define LOWCHR1 0xB
#define HIGHCHR0 0xE000
#define HIGHCHR1 0xE001

#define REORDER 1

enum {
	SBL_CHAPTER = 0,
	SBL_VERSE = 1
};

LANGDESCRIPTOR * cl_languages;
int cl_languagecount;
TCHAR * cl_sorttypes[] = {
	TEXT("Simple"),
	TEXT("Letter"),
	TEXT("Letter—CMS"),
	TEXT("Letter—ISO"),
	TEXT("Word"),
	TEXT("Word—CMS"),
	TEXT("Word—ISO"),
	TEXT("Letter—SBL"),
	TEXT("Word—SBL")
};

static char * scripts[] = {
	"Latn","Cyrl","Grek","Armn", "Geor",
	"Arab","Hebr",
	"Kore", "Hans","Hant","Jpan","Thai","Mymr",
	"Deva","Beng","Taml","Knda","Gujr","Mlym","Telu"
};
#define SCRIPTCOUNT (sizeof(scripts)/sizeof(char *))

static unichar specials[] = {'.',':',';',',',KEEPCHR,ESCCHR,SPACE,OBRACE,CBRACE,OBRACKET,OPAREN,DASH,SLASH,0};

static unichar longmonth[] = {'L','L','L','L',0};
static unichar shortmonth[] = {'L','L','L',0};

static COLLATORTEXT * scol1;
static COLLATORTEXT * scol2;

static short lookup(COLLATOR * col, int flags, int mode);	// truncated text comparison
static short rawcomp(COLLATOR * col, short flags);		/* raw text comparison */
static short textcomp(COLLATOR * col, short flags);		/* raw text comparison */
static int comparecodes(void);	// compares codes in otherwise identical strings
static int capturecodes(COLLATORTEXT * as, char *string, int *indexptr);
static unichar capturechar(COLLATORTEXT * as, unichar cc, SORTPARAMS * sgp);
static void capturesecondary(COLLATORTEXT * as, char * string, int *indexptr, int length);
static void capturesecondarystring(COLLATORTEXT * as, char * string, int *indexptr, unichar cc, SORTPARAMS * sgp);
static int lcompare(const void * s1, const void * s2);	// compare for qsort
static char * prepstring(char * dest, char * source, char * slist);	// makes dest from source, with substitutions from list

#if 0
/***************************************************************************/
void col_findlocales(void)		// finds list of locales with collators

{
	char codeName[100];
	char fullocaleID[100];
	char * localeID;
	int length;
	UEnumeration * ee;
    UErrorCode error = U_ZERO_ERROR;
	
	cl_languagecount = 0;
	cl_languages = malloc(ucol_countAvailable()*sizeof(LANGDESCRIPTOR));
	ee = ucol_openAvailableLocales(&error);
	while ((localeID = uenum_next(ee,&length, &error)))	{
		error = U_ZERO_ERROR;
		uloc_addLikelySubtags(localeID,fullocaleID,sizeof(fullocaleID),&error);
//		if (U_FAILURE(error))
//			NSLog(@"%s",u_errorName(error));
		error = U_ZERO_ERROR;
		uloc_getScript(fullocaleID,codeName,sizeof(codeName),&error);
//		if (U_FAILURE(error))
//			NSLog(@"Bad Script: %s",u_errorName(error));
		if (!strcmp(codeName,"Latn") || !strcmp(codeName,"Cyrl") || !strcmp(codeName,"Arab") || !strcmp(codeName,"Hebr") || !strcmp(codeName,"Kore") )	{	// if valid script
			int lcount;
			
			error = U_ZERO_ERROR;
			uloc_getLanguage(localeID,codeName,sizeof(codeName),&error);
			for (lcount = 0; lcount < cl_languagecount; lcount++)	{
				if (!strcmp(cl_languages[lcount].id,codeName))
					break;
			}
			if (lcount == cl_languagecount)	{	// if need to add language
				unichar buffer[100];
				int namesize;
				
				strcpy(cl_languages[lcount].id,codeName);
				error = U_ZERO_ERROR;
				namesize = uloc_getDisplayName(localeID,"en_US",buffer,sizeof(buffer),&error);
				cl_languages[lcount].name = malloc(namesize+1);
				error = U_ZERO_ERROR;
				u_strToUTF8(cl_languages[lcount].name,100,&namesize,buffer,-1,&error);	// set name in UTF-8
//				NSLog(@"Language: %s, name: %s",codeName,cl_languages[lcount].name);
				cl_languagecount++;
			}
		}
	}
	qsort(cl_languages, cl_languagecount, sizeof(LANGDESCRIPTOR), lcompare);  /* sort substrings */
	uenum_close(ee);
}
#else
/***************************************************************************/
void col_findlocales(void)		// finds list of locales with collators

{
	char codeName[100];
	char fullocaleID[100];
	const char * localeID;
	int length;
	UEnumeration * ee;
    UErrorCode error = U_ZERO_ERROR;
	
	cl_languagecount = 0;
	cl_languages = malloc(ucol_countAvailable()*sizeof(LANGDESCRIPTOR));
	ee = ucol_openAvailableLocales(&error);
	while ((localeID = uenum_next(ee,&length, &error)))	{
		int i;
		error = U_ZERO_ERROR;
		uloc_addLikelySubtags(localeID,fullocaleID,sizeof(fullocaleID),&error);
		error = U_ZERO_ERROR;
		uloc_getScript(fullocaleID,codeName,sizeof(codeName),&error);
//		NSLog(@"LocaleID: %s;\tFull: %s;\tScript: %s",localeID,fullocaleID,codeName);
		if (!strcmp(localeID,"en_US") || !strcmp(localeID,"en_US_POSIX"))	// skip special English locales
			continue;
		for (i = 0; i < SCRIPTCOUNT; i++)	{	// for all allowed scripts
			if (!strcmp(codeName,scripts[i]))	{	// if a hit
				int lcount;
				
				for (lcount = 0; lcount < cl_languagecount; lcount++)	{
					if (!strcmp(cl_languages[lcount].localeID,localeID))
						break;
				}
				if (lcount == cl_languagecount)	{	// if need to add language
					unichar buffer[100];
					char lbuffer[100];
					int namesize;
					
					strcpy(cl_languages[lcount].localeID,localeID);	// localeID
					strcpy(cl_languages[lcount].script,codeName);	// script code
					error = U_ZERO_ERROR;
					cl_languages[lcount].direction = uloc_getCharacterOrientation(localeID,&error);
					error = U_ZERO_ERROR;
					uloc_getLanguage(localeID,lbuffer,sizeof(buffer),&error);
					strcpy(cl_languages[lcount].id,lbuffer);	// language code
					error = U_ZERO_ERROR;
					namesize = uloc_getDisplayName(localeID,"en_US",buffer,sizeof(buffer),&error);	// language display name
					error = U_ZERO_ERROR;
					u_strToUTF8(cl_languages[lcount].name,sizeof(cl_languages[lcount].name),&namesize,buffer,-1,&error);	// set name in UTF-8
//					NSLog(@"Locale: %s, ID: %s,Script: %s, Display Name: %s, dir: %d",localeID,cl_languages[lcount].id, cl_languages[lcount].script,cl_languages[lcount].name, cl_languages[lcount].direction);
					cl_languagecount++;
				}
			}
		}
	}
	qsort(cl_languages, cl_languagecount, sizeof(LANGDESCRIPTOR), lcompare);  /* sort substrings */
	uenum_close(ee);
}
#endif
/***************************************************************************/
static int lcompare(const void * s1, const void * s2)	// compare for qsort

{
	return strcmp(((LANGDESCRIPTOR*)s1)->name,((LANGDESCRIPTOR*)s2)->name);
}
/***************************************************************************/
LANGDESCRIPTOR * col_fixLocaleInfo(SORTPARAMS * sgp)	// fixes locale info for sort language (needed to fix sort params before LOCALESORTVERSION)

{
	int lcount;

	sgp->sversion = SORTVERSION;		// always set this because params inherited from early versions of prefs might not have had it set
	for (lcount = 0; lcount < cl_languagecount; lcount++)	{
		// test sgp->language because that was the only locale identifier before LOCALESORTVERSION
		if (*sgp->localeID && !strcmp(sgp->localeID,cl_languages[lcount].localeID) || !*sgp->localeID && !strcmp(sgp->language,cl_languages[lcount].id)) {	// if our locale (try full first)
			if (!*sgp->localeID)	// if don't have localeID
				strcpy(sgp->localeID,cl_languages[lcount].localeID);	// set it
			if (!strcmp(cl_languages[lcount].script, "Latn"))	// if latin
				sgp->nativescriptfirst = TRUE;	// force native script first
			return &cl_languages[lcount];
		}
	}
	return NULL;
}
/***************************************************************************/
LANGDESCRIPTOR * col_getLocaleInfo(SORTPARAMS * sgp)	// retrieves locale info from locale id

{
	int lcount;
	for (lcount = 0; lcount < cl_languagecount; lcount++)	{
		if (!strcmp(sgp->localeID,cl_languages[lcount].localeID))	// if our locale
			return &cl_languages[lcount];
	}
	return NULL;
}
/***************************************************************************/
void col_init(SORTPARAMS * sgp, INDEX * FF)		// initializes collator

{
	UColAttributeValue numbermode = sgp->type > RAWSORT ? UCOL_ON : UCOL_OFF;
	UErrorCode error = U_ZERO_ERROR;
	int i, mindex, sorder;
	short *iptr;
	UDateFormat * dfl,* dfs;
	UCalendar * cal;
	int32_t codes[50];
	
	if (FF->collator.ucol)	{
		ucol_close(FF->collator.ucol);
		FF->collator.ucol = NULL;
	}
	// opens en_US_POSIX if have called with invalid locale
	FF->collator.ucol = ucol_open(sgp->localeID, &error);
#if 0
	{
		UVersionInfo vinfo;
		char vstring[U_MAX_VERSION_STRING_LENGTH];
		
		ucol_getVersion(FF->collator.ucol,vinfo);
		u_versionToString(vinfo,vstring);
		NSLog(@"ICU Collator Version %s",vstring);
		error = U_ZERO_ERROR;
		NSLog(@"Locale: %s",ucol_getLocaleByType(FF->collator.ucol,ULOC_ACTUAL_LOCALE,&error));
	}
#endif
	if (U_FAILURE(error))
		NSLog("Cannot open collator %s",u_errorName(error));
	if (!scol1)
		scol1 = col_createstringforsize(MAXREC+1);
	if (!scol2)
		scol2 = col_createstringforsize(MAXREC+1);

	ucol_setAttribute(FF->collator.ucol,UCOL_NUMERIC_COLLATION, numbermode,&error);
//	ucol_setAttribute(FF->collator.ucol,UCOL_CASE_FIRST, UCOL_UPPER_FIRST,&error);
	if (U_FAILURE(error))
		NSLog("%s",u_errorName(error));

	error = U_ZERO_ERROR;
	FF->collator.unorm = unorm2_getInstance(NULL,"nfc",UNORM2_DECOMPOSE,&error);	// this is singleton; doesn't need release
	if (U_FAILURE(error))
		NSLog("%s",u_errorName(error));

	error = U_ZERO_ERROR;
	dfl = udat_open(UDAT_NONE,UDAT_FULL,FF->head.sortpars.language,0,-1,longmonth,-1,&error);
	udat_applyPattern(dfl,FALSE,longmonth,-1);	// don't understand why pattern must be set here; doesn't take in 'open' call
	error = U_ZERO_ERROR;
	dfs = udat_open(UDAT_NONE,UDAT_FULL,FF->head.sortpars.language,0,-1,shortmonth,-1,&error);
	udat_applyPattern(dfs,FALSE,shortmonth,-1);
	
	error = U_ZERO_ERROR;
	cal = ucal_open(0,-1,FF->head.sortpars.language,UCAL_GREGORIAN,&error);
	error = U_ZERO_ERROR;
	ucal_setDate(cal,2011,UCAL_JANUARY,1,&error);
	for (mindex = 0; mindex < 12; mindex++)	{		// for all months
		UDate date;
		
		error = U_ZERO_ERROR;
		date = ucal_getMillis(cal,&error);
		error = U_ZERO_ERROR;
		udat_format(dfl,date,FF->collator.longmonths[mindex],sizeof(LMONTH),NULL,&error);
		error = U_ZERO_ERROR;
		udat_format(dfs,date,FF->collator.shortmonths[mindex],sizeof(LMONTH),NULL,&error);
//		NSLog(@"%S",FF->collator.longmonths[mindex]);
//		NSLog(@"%S",FF->collator.shortmonths[mindex]);
		error = U_ZERO_ERROR;
		ucal_add(cal,UCAL_MONTH,1,&error);
	}
	ucal_close(cal);
	udat_close(dfl);
	udat_close(dfs);
	
	// now set character class precedence
#if REORDER
	codes[0] = UCOL_REORDER_CODE_SPACE;
	for (sorder = 1, i = 0; i < 3; i++)	{
		if (sgp->charpri[i] == SORT_SYMBOL)	{
			codes[sorder++] = UCOL_REORDER_CODE_PUNCTUATION;
			codes[sorder++] = UCOL_REORDER_CODE_SYMBOL;
			codes[sorder++] = UCOL_REORDER_CODE_CURRENCY;
		}
		else if (sgp->charpri[i] == SORT_NUMBER)	{
			codes[sorder++] = UCOL_REORDER_CODE_DIGIT;
		}
		else if (sgp->charpri[i] == SORT_LETTER)	{
			int lcount;
			if (sgp->nativescriptfirst)	{	// if want native script first
				for (lcount = 0; lcount < cl_languagecount; lcount++)	{
					if (!strcmp(cl_languages[lcount].localeID,sgp->localeID))	{	// if our localeID
						// checks per http://userguide.icu-project.org/collation/customization#TOC-Values-for-Reorder-Codes
						char * script = cl_languages[lcount].script;
						if (!strcmp("Kore",script))	{	// if Korean
							codes[sorder++] = u_getPropertyValueEnum(UCHAR_SCRIPT, "Hang");
							codes[sorder++] = u_getPropertyValueEnum(UCHAR_SCRIPT, "Hani");
						}
						else if (!strcmp("Jpan",script))	{	// Japanese
							codes[sorder++] = u_getPropertyValueEnum(UCHAR_SCRIPT, "Kana");
							codes[sorder++] = u_getPropertyValueEnum(UCHAR_SCRIPT, "Hani");
						}
						else if (!strcmp("Hans",script) || !strcmp("Hant",script))	// Chinese
							codes[sorder++] = u_getPropertyValueEnum(UCHAR_SCRIPT, "Hani");
						else
							codes[sorder++] = u_getPropertyValueEnum(UCHAR_SCRIPT, script);	// get our script code
						break;
					}
				}
			}
			else		// default
				codes[sorder++] = USCRIPT_LATIN;
			codes[sorder++] = UCOL_REORDER_CODE_OTHERS;
		}
	}
	error = U_ZERO_ERROR;
	ucol_setReorderCodes(FF->collator.ucol,codes,sorder,&error);
#else
	for (i = 0; i < 3; i++)	{
		if (sgp->charpri[i] == SORT_SYMBOL)	{
			if (i == 0)		// position 0
				sgp->symcode = LOWCHR0;
			else if (i == 1)	// position 1
				sgp->symcode = sgp->charpri[0] == SORT_LETTER ? HIGHCHR0 : LOWCHR1; // depends on letter position
			else		// position 2
				sgp->symcode = HIGHCHR1;
		}
		else if (sgp->charpri[i] == SORT_NUMBER)	{
			if (i == 0)		// position 0
				sgp->numcode = LOWCHR0;
			else if (i == 1)	// position 1
				sgp->numcode = sgp->charpri[0] == SORT_LETTER ? HIGHCHR0 : LOWCHR1; // depends on letter position
			else		// position 2
				sgp->numcode = HIGHCHR1;
		}
	}
#endif
	memset(sgp->reftab, -1, sizeof(sgp->reftab));	/* all ref types start invalid */
	for (iptr = sgp->refpri; *iptr >= 0; iptr++)		/* for each entry in priority list */
		sgp->reftab[*iptr] = iptr - sgp->refpri;		/* set priority index in table */
	memset(sgp->styletab,0,sizeof(sgp->styletab));
	for (i = 0; i < STYLETYPES; i++)	{	// build style priority table
		switch (sgp->styleorder[i]) {
			case 0:		// plain
				sgp->styletab[0] = i;
				break;
			case 1:		// bold
				sgp->styletab[FX_BOLD] = i;
				sgp->styletab[FX_BOLD|FX_ITAL] = i;
				sgp->styletab[FX_BOLD|FX_ULINE] = i;
				sgp->styletab[FX_BOLD|FX_ITAL|FX_ULINE] = i;
				break;
			case 2:		// ital
				sgp->styletab[FX_ITAL] = i;
				sgp->styletab[FX_ITAL|FX_BOLD] = i;
				sgp->styletab[FX_ITAL|FX_ULINE] = i;
				sgp->styletab[FX_ITAL|FX_BOLD|FX_ULINE] = i;
				break;
			case 3:		// uline
				sgp->styletab[FX_ULINE] = i;
				sgp->styletab[FX_ULINE|FX_BOLD] = i;
				sgp->styletab[FX_ULINE|FX_ITAL] = i;
				sgp->styletab[FX_ULINE|FX_BOLD|FX_ITAL] = i;
				break;
		}
	}
}
#if 0
/***************************************************************************/
void col_loadUTF8string(COLLATORTEXT * as,INDEX * FF, SORTPARAMS * sgp, char *string, int flags)	// loads COLLATORTEXT from xstring

{
	BOOL wordsort = iswordsort(sgp->type);
	BOOL cmssort = sgp->type == WORDSORT_CMS || sgp->type == LETTERSORT_CMS;
	BOOL sblsort = sgp->type == WORDSORT_SBL || sgp->type == LETTERSORT_SBL;
	int sbltype = SBL_CHAPTER;
	int size = strlen(string);
	int sindex = 0;
	int spaceindex = 0;
	unichar cc, lastcc;
	
	as->codecount = 0;
	as->secondarycount = 0;
	as->length = 0;
	as->seclength = 0;
	as->crossrefvalue = 0;
	as->breakcount = 0;
	as->hasdigits = 0;
	
	if (flags&MATCH_CHECKPREFIX)  {      /* if want prefix checks */
		if (str_crosscheck(FF,string))		    /* if a cross reference */
			as->crossrefvalue = FF->head.formpars.ef.cf.mainposition >= CP_LASTSUB ? 1 : -1;
		else {
			int length;
			short tokens;
			length = str_skiplist(string,sgp->ignore, &tokens) - string;     /* skip words to be ignored */
			if (length)
				capturesecondary(as,string,&sindex,length);
		}
	}
	for (lastcc = cc = '\0'; sindex < size; lastcc = cc)	{	// build text string
		if (!capturecodes(as,string,&sindex))	{	// if haven't captured any codes
			char * s1;
			
			U8_NEXT_UNSAFE(string,sindex,cc);	// convert char, increment sindex
			if (sgp->type != RAWSORT)	{
				switch (cc)	{
					case SPACE:
						spaceindex = as->length;	// position of space if it were to be added
						if (!wordsort || (as->length && as->string[as->length-1] == SPACE))	// ignore unless word sort & not redundant space
							continue;
						break;
					case ESCCHR:		/* escape char */
						if (sindex == size)	// if don't have following char
							continue;
						U8_NEXT_UNSAFE(string,sindex,cc);	// get protected char
						break;
					case KEEPCHR:		    /* char after this needs to be kept */
						if (sindex == size)	// if don't have following char
							continue;
						U8_NEXT_UNSAFE(string,sindex,cc);	// get protected char
#if !REORDER
						// following needed while ICU doesn't allow setting character class precedence
						if (!u_isalpha(cc) && !(sgp->evalnums && u_isdigit(cc)) && !(flags&MATCH_FIRSTCHAR))	// if not doing lead char && it wouldn't be evaluated
							cc = capturechar(as,cc,sgp);	// save char as secondary and get token value
#endif
						break;
					case OBRACKET:			 /* if start of string to be ignored */
						s1 = str_skiptoclose(&string[sindex],CBRACKET)-1;	// skip bracket contents
						capturesecondary(as,string,&sindex,s1-&string[sindex]);	// opening and closing <> discarded
						sindex++;		// index now beyond closing bracket 
						continue;
					case OBRACE:			/* just ignore these altogether */
//						if (flags&MATCH_FIRSTCHAR)	// if seeking lead char
//							sindex = str_skiptoclose(string+sindex, CBRACE)-string;	// skip any leading braced text
					case CBRACE:
						continue;
					case DASH:
					case SLASH:
						if (wordsort && !sgp->ignoreslash)    {	/* if word sort and using -/ */
							cc = SPACE; 			/* make a space */
							break;
						}
						continue;
						// following cases fall-through unless trapped
					case '.':
					case ':':
					case ';':
					case ',':
						if (sgp->ignorepunct != TRUE && *str_skipcodes(string+sindex) == SPACE)  {	// if not ignoring && next is space
							// cms && sbl break only on comma; ignore other punct
							if (cc == ',' || (!cmssort && !sblsort))	{
								as->breaks[as->breakcount].index = as->length; 	    // allow break
								as->breaks[as->breakcount++].seccount = as->secondarycount; 	    // save secondary count
								continue;
							}
						}
						else if (sblsort && cc != ';' && u_isdigit(lastcc))	{	// keep special ',' ':' ',' for SBL
							sbltype = SBL_VERSE;
							break;
						}
						// fall through
					default:
						if (cc == OPAREN)	{	// handle parens per settings
							BOOL parentype = spaceindex == as->length || as->string[as->length-1] == SPACE;
							if (sgp->ignoreparenphrase && parentype || sgp->ignoreparen && !parentype)	{	// if ignoring all text in parens, or this can't be paren ending
								int ocount = 0;
								for (s1 = &string[sindex], ocount = 0; *s1; s1++) {		/* until end of string */
									if (*s1 == CPAREN && --ocount < 0)	/* if got matching closing paren */
										break;
									else if (*s1 == OPAREN)	/* one more opening paren */
										ocount++;
								}
#if 0
								if (cmssort)	{	// if CMS sort
									as->breaks[as->breakcount].index = spaceindex; 	    // force break at space preceding opener
									as->breaks[as->breakcount++].seccount = as->secondarycount; 	    // save secondary count
								}
#endif
								capturesecondary(as,string,&sindex,s1-&string[sindex]);	// opening and closing () discarded
								sindex++;		// index now beyond closing paren
								continue;
							}
						}
						if (sgp->evalnums && u_isdigit(cc))	{	// if digit to use
							if (!u_isdigit(lastcc))		{	// if last char wasn't digit
								if (sblsort)	{	// if SBL
									if (lastcc == ENDASH || lastcc == '-')	{	// if starting range
										int numindex = sindex;
										int xindex;
										
										U8_BACK_1_UNSAFE(string,numindex);	// get offset of first digit
										xindex = strspn(string+numindex,"0123456789");											
										if (xindex){	// if non-numeral follows
											char next = string[numindex+xindex];
											if (next == ':')	// if end of range is explicitly chapter
												sbltype = SBL_CHAPTER;	// set it
										}
										as->string[as->length++] = sbltype == SBL_CHAPTER ? LOWCHR1 : LOWCHR0;	// lower vale connector for verse
									}
								}
								if (lastcc && u_isdigit(as->string[as->length-1]))	// if preceding sortable character is digit
									as->string[as->length++] = lastcc;	// preserve discarded character as separator
							}
						}
						else if (!u_isalpha(cc))	{	// capture all chars in the ignored category
							capturesecondarystring(as,string,&sindex,cc, sgp);
							continue;	// must be at end of input
						}
						else if (sbltype == SBL_VERSE && cc == 'f' && u_isdigit(lastcc))	{	// if sbl sorting verse and open range char follows digit
							int tindex = sindex-1;		// step back to capture char
							capturesecondary(as,string,&tindex,1);	// capture terminating alpha
							sbltype = SBL_CHAPTER;	// reset type
							continue;
						}
				}
			}
			as->string[as->length++] = cc;
			if (flags&MATCH_FIRSTCHAR)	{
#if !REORDER
				if (as->length == 2)	// if 2 chars (first can only be symcode or numcode)
					as->string[0] = as->string[1];	// get actual lead char
#endif
				as->breakcount = 0;		// discard all special text/codes
				as->secondarycount = 0;
				break;
			}
		}
	}
	while (as->length && as->string[as->length-1] == SPACE)	// remove trailing spaces
		as->length--;
#if 1
	for (sindex = 0; sindex < as->secondarycount; sindex++)	{	// for all secondaries
		if (as->secondaries[sindex].offset >= as->length)	{	// if any beyond length
			as->breaks[as->breakcount].index = as->length; 	    // add new break so that these secondaries are implicitly after it
			as->breaks[as->breakcount++].seccount = sindex; 	// save secondary count at break
			break;
		}
	}
#endif
	as->breaks[as->breakcount].index = as->length; 	    // mark end as break
	as->breaks[as->breakcount++].seccount = as->secondarycount; 	    // save secondary count
}
#else
/***************************************************************************/
void col_loadUTF8string(COLLATORTEXT * as, INDEX * FF, SORTPARAMS * sgp, char *sstring, int flags)	// loads COLLATORTEXT from xstring

{
	BOOL wordsort = iswordsort(sgp->type);
	BOOL cmssort = sgp->type == WORDSORT_CMS || sgp->type == LETTERSORT_CMS;
	BOOL sblsort = sgp->type == WORDSORT_SBL || sgp->type == LETTERSORT_SBL;
	char *string;
	int sbltype = SBL_CHAPTER;
	int size;
	int sindex = 0;
	int spaceindex = 0;
	unichar cc, lastcc;

	as->codecount = 0;
	as->secondarycount = 0;
	as->length = 0;
	as->seclength = 0;
	as->crossrefvalue = 0;
	as->breakcount = 0;
	as->hasdigits = 0;

	if (*sgp->substitutes != EOCS && sgp->type != RAWSORT)	// if have potential substitutions to make
		string = prepstring(as->scratch + 2, sstring, sgp->substitutes);	// prep the string
	else
		string = sstring;
	size = strlen(string);
	if (flags&MATCH_CHECKPREFIX) {      /* if want prefix checks */
		if (str_crosscheck(FF, string))		    /* if a cross reference */
			as->crossrefvalue = FF->head.formpars.ef.cf.mainposition >= CP_LASTSUB ? 1 : -1;
		else {
			int length;
			short tokens;
#if 0
			char * smark = string;
			cc = u8_nextU(&smark);	// find lead character
			if (u_strchr(openers, cc)) {	// if it's opening quote [NB: sometime we should check for codes preceding quote]
				capturechar(as, cc, sgp);	// save as secondary
				string = smark;
			}
#endif
			length = str_skiplist(string, sgp->ignore, &tokens) - string;     /* skip words to be ignored */
			if (length)
				capturesecondary(as, string, &sindex, length);
		}
	}
	for (lastcc = cc = '\0'; sindex < size; lastcc = cc) {	// build text string
		if (!capturecodes(as, string, &sindex)) {	// if haven't captured any codes
			char * s1;

			U8_NEXT_UNSAFE(string, sindex, cc);	// convert char, increment sindex
			if (sgp->type != RAWSORT) {
				switch (cc) {
				case SPACE:
					spaceindex = as->length;	// position of space if it were to be added
					if (!wordsort || (as->length && as->string[as->length - 1] == SPACE))	// ignore unless word sort & not redundant space
						continue;
					break;
				case ESCCHR:		/* escape char */
					if (sindex == size)	// if don't have following char
						continue;
					U8_NEXT_UNSAFE(string, sindex, cc);	// get protected char
					if (u_strchr(specials, cc)) {	// if char for special handling
						capturechar(as, cc, sgp);	// save as secondary
						continue;
					}
					break;		// not special; treat as normal char
				case KEEPCHR:		    /* char after this needs to be kept */
					if (sindex == size)	// if don't have following char
						continue;
					U8_NEXT_UNSAFE(string, sindex, cc);	// get protected char
#if !REORDER
														// following needed while ICU doesn't allow setting character class precedence
					if (!u_isalpha(cc) && !(sgp->evalnums && u_isdigit(cc)) && !(flags&MATCH_FIRSTCHAR))	// if not doing lead char && it wouldn't be evaluated
						cc = capturechar(as, cc, sgp);	// save char as secondary and get token value
#endif
					break;
				case OBRACKET:			 /* if start of string to be ignored */
					s1 = str_skiptoclose(&string[sindex], CBRACKET) - 1;	// skip bracket contents
					capturesecondary(as, string, &sindex, s1 - &string[sindex]);	// opening and closing <> discarded
					sindex++;		// index now beyond closing bracket
					continue;
				case OBRACE:			/* just ignore these altogether */
										//						if (flags&MATCH_FIRSTCHAR)	// if seeking lead char
										//							sindex = str_skiptoclose(string+sindex, CBRACE)-string;	// skip any leading braced text
				case CBRACE:
					continue;
				case DASH:
				case SLASH:
					if (wordsort && !sgp->ignoreslash) {	/* if word sort and using -/ */
						cc = SPACE; 			/* make a space */
						break;
					}
					continue;
					// following cases fall-through unless trapped
				case '.':
				case ':':
				case ';':
				case ',':
					if (sgp->ignorepunct != TRUE && *str_skipcodes(string + sindex) == SPACE) {	// if not ignoring && next is space
																								// cms && sbl break only on comma; ignore other punct
						if (cc == ',' || (!cmssort && !sblsort)) {
							as->breaks[as->breakcount].index = as->length; 	    // allow break
							as->breaks[as->breakcount++].seccount = as->secondarycount; 	    // save secondary count
							continue;
						}
					}
					else if (sblsort && cc != ';' && u_isdigit(lastcc)) {	// keep special ',' ':' ',' for SBL
						sbltype = SBL_VERSE;
						break;
					}
					// fall through
				default:
					if (cc == OPAREN) {	// handle parens per settings
						BOOL parentype = spaceindex == as->length || as->string[as->length - 1] == SPACE;
						if (sgp->ignoreparenphrase && parentype || sgp->ignoreparen && !parentype) {	// if ignoring all text in parens, or this can't be paren ending
							int ocount = 0;
							for (s1 = &string[sindex], ocount = 0; *s1; s1++) {		/* until end of string */
								if (*s1 == CPAREN && --ocount < 0)	/* if got matching closing paren */
									break;
								else if (*s1 == OPAREN)	/* one more opening paren */
									ocount++;
							}
#if 0
							if (cmssort) {	// if CMS sort
								as->breaks[as->breakcount].index = spaceindex; 	    // force break at space preceding opener
								as->breaks[as->breakcount++].seccount = as->secondarycount; 	    // save secondary count
							}
#endif
							capturesecondary(as, string, &sindex, s1 - &string[sindex]);	// opening and closing () discarded
							sindex++;		// index now beyond closing paren
							continue;
						}
					}
					if (sgp->evalnums && u_isdigit(cc)) {	// if digit to use
						if (!u_isdigit(lastcc)) {	// if last char wasn't digit
							if (sblsort) {	// if SBL
								if (lastcc == ENDASH || lastcc == '-') {	// if starting range
									int numindex = sindex;
									int xindex;

									U8_BACK_1_UNSAFE(string, numindex);	// get offset of first digit
									xindex = strspn(string + numindex, "0123456789");
									if (xindex) {	// if non-numeral follows
										char next = string[numindex + xindex];
										if (next == ':')	// if end of range is explicitly chapter
											sbltype = SBL_CHAPTER;	// set it
									}
									as->string[as->length++] = sbltype == SBL_CHAPTER ? LOWCHR1 : LOWCHR0;	// lower vale connector for verse
							}
						}
#if 0
							if (lastcc && u_isdigit(as->string[as->length - 1]))	// if preceding sortable character is digit
								as->string[as->length++] = lastcc;	// preserve discarded character as separator
#else
							if (as->length && u_isdigit(as->string[as->length - 1]))	// if preceding sortable character is digit
								as->string[as->length++] = lastcc;	// add last char as separator
#endif
					}
				}
					else if (!u_isalpha(cc)) {	// capture all chars in the ignored category
						capturesecondarystring(as, string, &sindex, cc, sgp);
						continue;	// must be at end of input
			}
					else if (sbltype == SBL_VERSE && cc == 'f' && u_isdigit(lastcc)) {	// if sbl sorting verse and open range char follows digit
						int tindex = sindex - 1;		// step back to capture char
						capturesecondary(as, string, &tindex, 1);	// capture terminating alpha
						sbltype = SBL_CHAPTER;	// reset type
						continue;
					}
		}
	}
			as->string[as->length++] = cc;
			if (flags&MATCH_FIRSTCHAR) {
#if !REORDER
				if (as->length == 2)	// if 2 chars (first can only be symcode or numcode)
					as->string[0] = as->string[1];	// get actual lead char
#endif
				as->breakcount = 0;		// discard all special text/codes
				as->secondarycount = 0;
				break;
			}
}
	}
	while (as->length && as->string[as->length - 1] == SPACE)	// remove trailing spaces
		as->length--;
#if 1
	for (sindex = 0; sindex < as->secondarycount; sindex++) {	// for all secondaries
		if (as->secondaries[sindex].offset >= as->length) {	// if any beyond length
			as->breaks[as->breakcount].index = as->length; 	    // add new break so that these secondaries are implicitly after it
			as->breaks[as->breakcount++].seccount = sindex; 	// save secondary count at break
			break;
		}
	}
#endif
	as->breaks[as->breakcount].index = as->length; 	    // mark end as break
	as->breaks[as->breakcount++].seccount = as->secondarycount; 	    // save secondary count
}
#endif
/***************************************************************************/
COLLATORTEXT * col_createstringforsize(int length)

{
	COLLATORTEXT * as;

	as = calloc(1, sizeof(COLLATORTEXT));
	as->string = malloc(length * sizeof(unichar));
	as->codesets = malloc(length / 2 * sizeof(CODESET));	// max is half length since 2 chars per code
	as->sectext = malloc(length * sizeof(unichar));
	as->secondaries = malloc(length / 2 * sizeof(SECONDARYSET));
	as->breaks = malloc(length / 2 * sizeof(BREAKSET));
	as->scratch = calloc(length * 2, sizeof(char));
	return as;
}
/***************************************************************************/
void col_free(COLLATORTEXT * as)

{
	free(as->breaks);
	free(as->secondaries);
	free(as->sectext);
	free(as->codesets);
	free(as->string);
	free(as->scratch);
	free(as);
}
/*****************************************************************************/
short col_match(INDEX * FF, SORTPARAMS * sgp, char *s1, char *s2, short flags) /* compares text field by current rules */

{		
	col_loadUTF8string(scol1,FF,sgp,s1,flags);
//	col_describe(scol1,sgp);
	col_loadUTF8string(scol2,FF,sgp,s2,flags);
//	col_describe(scol2,sgp);

	if (flags & MATCH_LOOKUP)
		return lookup(&FF->collator,flags, sgp->type);
	if (sgp->type == RAWSORT)	/* raw sort */
		return (rawcomp(&FF->collator,flags));
	else 
		return (textcomp(&FF->collator,flags));
}
/*****************************************************************************/
void col_buildkey(INDEX * FF, char *key, char * string)	// builds sort key as utf-8 string

{
	UErrorCode error = U_ZERO_ERROR;
	int length;
	
	col_loadUTF8string(scol1,FF,&FF->head.sortpars,string,0);
	u_strToUTF8(key,MAXREC,&length,scol1->string,scol1->length,&error);
}
//*****************************************************************************/
int col_collatablelength(INDEX * FF, char * string)		// returns length of primary string

{
	col_loadUTF8string(scol1, FF, &FF->head.sortpars, string, MATCH_FIRSTCHAR);
	return scol1->length;
}
/*****************************************************************************/
BOOL col_newlead(INDEX * FF, char * string1, char * string2, unichar * lead)	// returns TRUE if s2 and s1 have different lead

{
	unichar cc = 0;
	BOOL diffclass;
	
	col_loadUTF8string(scol2,FF, &FF->head.sortpars,string2,MATCH_FIRSTCHAR);
	*lead = scol2->length ? scol2->string[0] : 0;
	if (u_islower(*lead))
		*lead = u_toupper(*lead);
	if (string1)	{	// if there's a prior string
		col_loadUTF8string(scol1,FF, &FF->head.sortpars,string1,MATCH_FIRSTCHAR);
		if (scol1->length)	{	// if have prior lead
			if (scol2->length)	{	// if have two leads
				if (*scol1->string == *scol2->string)	// if identical
					return FALSE;
				if (u_isalpha(*scol1->string) && u_isalpha(*scol2->string))	// both alphas; check primary value
					return lookup(&FF->collator, MATCH_IGNOREACCENTS|MATCH_IGNORECODES, FF->head.sortpars.type) ? TRUE : FALSE;
			}
			cc = scol1->string[0];
		}
	}
	// if get here, then can't both be alphas
	if (*lead == 0 && cc == 0)	// empty matches
		return FALSE;
	if (u_isalpha(*lead) || u_isalpha(cc))
		return TRUE;
	switch (FF->head.formpars.ef.eg.method)	{
		case 0:				// separate group for each number and each symbol
			return TRUE;
		case 1:				// all symbols together; numbers separate
			diffclass = !cc || u_isdigit(*lead) || u_isdigit(cc);
			if (diffclass && !u_isdigit(*lead))		// lead is grouped symbol
				*lead = SYMBIT;
			return diffclass;
		case 2:				// all numbers together, symbols separate
			diffclass = !cc || !u_isdigit(*lead) || cc && !u_isdigit(cc);
			if (diffclass && u_isdigit(*lead))			// if lead is grouped digit
				*lead = NUMBIT;
			return diffclass;
		case 3:				// all symbols & numbers in same group
			diffclass =  !cc || u_isalpha(*lead) && u_isalpha(cc);
			if (diffclass)			// if lead is grouped num/symbol
				*lead = (SYMBIT|NUMBIT);
			return diffclass;
		case 4:				// all number in one group; symbols in another
			diffclass =  !cc || u_isdigit(*lead) && !u_isdigit(cc) || !u_isdigit(*lead) &&u_isdigit(cc);
			if (diffclass)		// if lead is grouped num/symbol
				* lead = u_isdigit(*lead) ? NUMBIT : SYMBIT;
			return diffclass;
	}
	return TRUE;
}
/*****************************************************************************/
void col_describe(COLLATORTEXT * col,SORTPARAMS * sgp)	// describes content 

{
	unichar buffer[2000];
	int sindex;

	u_strncpy(buffer,col->string,col->length);
	buffer[col->length] = 0;
	NSLog(*buffer ? "%S" :"__",buffer);
	if (col->breakcount > 1)	{
		for (sindex = 0; sindex < col->breakcount; sindex++)	{
			int offset = (sindex ? col->breaks[sindex-1].index : 0);
			int length = col->breaks[sindex].index-offset;
			u_strncpy(buffer,col->string+offset,length);
			buffer[length] = 0;
			NSLog("  B [%d: %d]%S",offset, col->breaks[sindex].seccount, buffer);
		}
	}
	for (sindex = 0; sindex < col->secondarycount; sindex++)	{
#if !REORDER
		int offset = col->secondaries[sindex].base == sgp->numcode || col->secondaries[sindex].base == sgp->symcode ? 1 : 0;
		int length = col->secondaries[sindex+1].base-col->secondaries[sindex].base-offset;	// offset char is class byte
		u_strncpy(buffer,&col->sectext[col->secondaries[sindex].base+offset],length);
#else
		int length = col->secondaries[sindex+1].base-col->secondaries[sindex].base;	// offset char is class byte
		u_strncpy(buffer,&col->sectext[col->secondaries[sindex].base],length);
#endif
		buffer[length] = 0;
		NSLog("  S [%d, %d]%S",col->secondaries[sindex].offset, length,buffer);
	}
}
#if 0
/*****************************************************************************/
static short lookup(COLLATOR * col, int flags, int mode)	// truncated text comparison

{
	UCollationResult result = UCOL_EQUAL;
	UCollationStrength limitlevel;
	
	if (flags&MATCH_IGNOREACCENTS)	// ending strength
		limitlevel = UCOL_PRIMARY;
	else if (flags&MATCH_IGNORECASE)
		limitlevel = UCOL_SECONDARY;
	else
		limitlevel = UCOL_TERTIARY;
	if (!scol2->length && scol1->length)	// if testing against empty string
		return UCOL_GREATER;
	if (mode == RAWSORT)	{
		ucol_setStrength(col->ucol, limitlevel);
		result = ucol_strcoll(col->ucol,scol1->string,scol1->length,scol2->string,scol1->length < scol2->length ? scol1->length : scol2->length);
	}
	else {
		int level = UCOL_PRIMARY;	// starting strength
		
		while (level <= limitlevel)	{
			int breaks = 0;
			int sindex = 0;
			int breakCount;

			ucol_setStrength(col->ucol, level);
			for (breakCount = 0; scol1->breaks[breakCount].index < scol1->length; breakCount++)	// to ensure we ignore secondaries after end of scol1
				;
			breakCount++;
			do {	// while breaks to do
				// set break in string 2 at earlier of natural position or length of string 1
				int s2breakindex;
				if (breaks == breakCount - 1 && scol1->breaks[breaks].index < scol2->breaks[breaks].index)	// if end of s1 is before end of s2
					s2breakindex = scol1->breaks[breaks].index;	// shorten s2
				else
					s2breakindex = scol2->breaks[breaks].index;
				result = ucol_strcoll(col->ucol, scol1->string, scol1->breaks[breaks].index, scol2->string, s2breakindex);
				if (result != UCOL_EQUAL)	// if different 
					return result;
				while (sindex < scol1->breaks[breaks].seccount && sindex < scol2->breaks[breaks].seccount) {	// while secondaries to do before break
					SECONDARYSET * ss1ptr = &scol1->secondaries[sindex];
					SECONDARYSET * ss2ptr = &scol2->secondaries[sindex];

					if (ss1ptr->offset == ss2ptr->offset) {	// if secondary offsets are the same, compare
						int length1 = (ss1ptr + 1)->base - ss1ptr->base;
						result = ucol_strcoll(col->ucol, &scol1->sectext[ss1ptr->base], length1, &scol2->sectext[ss2ptr->base], length1);
						if (result != UCOL_EQUAL)
							return result;
					}
					else if (ss2ptr->offset < s2breakindex)		// if s2 secondary lies within span of s1; otherwise ignore
						return ss2ptr->offset - ss1ptr->offset;	// return *inverted* difference between offsets
					sindex++;
				}
			} while (++breaks < breakCount && breaks < scol2->breakcount);
			if (breakCount > scol2->breakcount)	// if s1 has more text
				return 1;
			level++;
		}
	}
	if (!(flags&MATCH_IGNORECODES))			// if not ignoring codes
		return comparecodes();
	return result;
}
#else
/*****************************************************************************/
static short lookup(COLLATOR * col, int flags, int mode)	// truncated text comparison

{
	UCollationResult result = UCOL_EQUAL;
	UCollationStrength limitlevel;

	if (flags&MATCH_IGNOREACCENTS)	// ending strength
		limitlevel = UCOL_PRIMARY;
	else if (flags&MATCH_IGNORECASE)
		limitlevel = UCOL_SECONDARY;
	else
		limitlevel = UCOL_TERTIARY;
	if (!scol1->length)
		return UCOL_LESS;
	else if (!scol2->length)	// if testing against empty string
		return UCOL_GREATER;
	if (mode == RAWSORT) {
		ucol_setStrength(col->ucol, limitlevel);
		result = ucol_strcoll(col->ucol, scol1->string, scol1->length, scol2->string, scol1->length < scol2->length ? scol1->length : scol2->length);
	}
	else {
		int level = UCOL_PRIMARY;	// starting strength

		while (level <= limitlevel) {
			int breaks = 0;
			int sindex = 0;
			int breakCount;

			ucol_setStrength(col->ucol, level);
			for (breakCount = 0; scol1->breaks[breakCount].index < scol1->length; breakCount++)	// to ensure we ignore secondaries after end of scol1
				;
			breakCount++;
			do {	// while breaks to do
				// set break in string 2 at earlier of natural position or length of string 1
				int s2breakindex;
				if (breaks == breakCount - 1 && scol1->breaks[breaks].index < scol2->breaks[breaks].index)	// if end of s1 is before end of s2
					s2breakindex = scol1->breaks[breaks].index;	// shorten s2
				else
					s2breakindex = scol2->breaks[breaks].index;
				result = ucol_strcoll(col->ucol, scol1->string, scol1->breaks[breaks].index, scol2->string, s2breakindex);
				if (result != UCOL_EQUAL)	// if different 
					return result;
				while (sindex < scol1->breaks[breaks].seccount && sindex < scol2->breaks[breaks].seccount) {	// while secondaries to do before break
					SECONDARYSET * ss1ptr = &scol1->secondaries[sindex];
					SECONDARYSET * ss2ptr = &scol2->secondaries[sindex];

					if (ss1ptr->offset == ss2ptr->offset) {	// if secondary offsets are the same, compare
						int length1 = (ss1ptr + 1)->base - ss1ptr->base;
						int length2 = (ss2ptr + 1)->base - ss2ptr->base;
						result = ucol_strcoll(col->ucol, &scol1->sectext[ss1ptr->base], length1, &scol2->sectext[ss2ptr->base], length2);
						if (result != UCOL_EQUAL)
							return result;
					}
					else if (ss2ptr->offset < s2breakindex)		// if s2 secondary lies within span of s1; otherwise ignore
						return ss2ptr->offset - ss1ptr->offset;	// return *inverted* difference between offsets
					sindex++;
				}
			} while (++breaks < breakCount && breaks < scol2->breakcount);
			if (breakCount > scol2->breakcount)	// if s1 has more text
				return 1;
			level++;
		}
	}
	if (!(flags&MATCH_IGNORECODES))			// if not ignoring codes
		return comparecodes();
	return result;
}
#endif
/*****************************************************************************/
static short rawcomp(COLLATOR * col, short flags)		/* raw text comparison */

{
	UCollationResult result;
	UCollationStrength level;
	
	if (flags&MATCH_IGNOREACCENTS)
		level = UCOL_PRIMARY;
	else if (flags&MATCH_IGNORECASE)
		level = UCOL_SECONDARY;
	else
		level = UCOL_TERTIARY;	// ending strength
	ucol_setStrength(col->ucol, level);
	result = ucol_strcoll(col->ucol,scol1->string,scol1->length,scol2->string,scol2->length);
	if (result != UCOL_EQUAL)	// if different
		return result;
	if (!(flags&MATCH_IGNORECODES))		// if not ignoring codes
		return comparecodes();
	return 0;
}
/*****************************************************************************/
static short textcomp(COLLATOR * col, short flags)		/* full text comparison */

{
	int level;
	int limitlevel;
	UCollationResult result;
	
	if (scol1->crossrefvalue != scol2->crossrefvalue)	// if diff crossref values
		return scol1->crossrefvalue-scol2->crossrefvalue;
	if (flags&MATCH_IGNOREACCENTS)
		limitlevel = UCOL_PRIMARY;
	else if (flags&MATCH_IGNORECASE)
		limitlevel = UCOL_SECONDARY;
	else
		limitlevel = UCOL_TERTIARY;	// ending strength
	level = scol1->breakcount == 1 && scol2->breakcount == 1 ? limitlevel : UCOL_PRIMARY;	// starting strength primary if contains breaks
	while (level <= limitlevel)	{
		int breaks = 0;
		int sindex = 0;
		
		ucol_setStrength(col->ucol, level);
		do	{	// while breaks to do
			result = ucol_strcoll(col->ucol,scol1->string,scol1->breaks[breaks].index,scol2->string,scol2->breaks[breaks].index);
			if (result != UCOL_EQUAL)	// if different 
				return result;
			if (scol1->length || scol2->length)	{	// compare secondaries if either field has content (to ensure that when <...> occupies whole field, whole field is ignored)
				while (sindex < scol1->breaks[breaks].seccount && sindex < scol2->breaks[breaks].seccount )		{	// while secondaries to do before break
					SECONDARYSET * ss1ptr = &scol1->secondaries[sindex];
					SECONDARYSET * ss2ptr = &scol2->secondaries[sindex];

					if (ss1ptr->offset == ss2ptr->offset)	{	// if secondary offsets are the same
						int length1 = (ss1ptr+1)->base - ss1ptr->base;
						int length2 = (ss2ptr+1)->base - ss2ptr->base;
						result = ucol_strcoll(col->ucol,&scol1->sectext[ss1ptr->base],length1,&scol2->sectext[ss2ptr->base],length2);
						if (result != UCOL_EQUAL)
							return result;
					}
					else 
						// !! NB return *inverted* difference [smaller value for string with secondary at highest offset]
						return ss2ptr->offset - ss1ptr->offset;	// return difference between offsets
					sindex++;
				}
			}
			// all secondaries exhausted up to break
			if (scol1->breaks[breaks].seccount != scol2->breaks[breaks].seccount)	// of one has more secondaries left before break
				return scol1->breaks[breaks].seccount - scol2->breaks[breaks].seccount;	// return diff
		} while (++breaks < scol1->breakcount && breaks < scol2->breakcount);
		// if we've exhausted all breaks in one string and some remain in the other...
		if (scol1->breakcount != scol2->breakcount)
			return scol1->breakcount - scol2->breakcount;
		level++;
	}
	if (!(flags&MATCH_IGNORECODES))		// if not ignoring codes
		return comparecodes();
	return 0;
}
/***************************************************************************/
static int comparecodes(void)	// compares codes in otherwise identical strings

{
	int sindex;

	for (sindex = 0; sindex < scol1->codecount && scol2->codecount; sindex++)	{
		if (scol1->codesets[sindex].offset != scol2->codesets[sindex].offset)	// if one has code before the other
			return scol2->codesets[sindex].offset - scol1->codesets[sindex].offset;		// return !!inverted
		if (scol1->codesets[sindex].code != scol1->codesets[sindex].code)	// if different codes at same position
			return scol1->codesets[sindex].code-scol1->codesets[sindex].code;	// precedence: color, font, style
	}
	return scol1->codecount-scol2->codecount;
}
/***************************************************************************/
static int capturecodes(COLLATORTEXT * as, char *string, int *indexptr)

{
	int codecount = as->codecount;
	int sindex = *indexptr;
	
#if 0
	while (string[sindex] == CODECHR || string[sindex] == CODEOCHR)	{	// while in code chars
		as->codesets[codecount].offset = as->length;	// position of code
		if (string[sindex++] == CODEOCHR)
			as->codesets[codecount].code = string[sindex]|FX_OFF;
		else
			as->codesets[codecount].code = string[sindex];
		codecount++;
		sindex++;
	}
#else
	while (iscodechar(string[sindex]))	{	// while in code chars
		as->codesets[codecount].offset = as->length;	// position of code
		if (string[sindex++] == FONTCHR)
			as->codesets[codecount].code = string[sindex]|FX_AUTOFONT;
		else
			as->codesets[codecount].code = string[sindex];
		codecount++;
		sindex++;
	}
#endif
	if (codecount > as->codecount)	{
		* indexptr = sindex;
		as->codecount = codecount;
		return TRUE;
	}
	return FALSE;
}
#if !REORDER
/***************************************************************************/
static unichar capturechar(COLLATORTEXT * as, unichar cc, SORTPARAMS * sgp)

{
	unichar charcode = u_isdigit(cc) ? sgp->numcode : sgp->symcode;	// get class marker
	as->secondaries[as->secondarycount].offset = as->length;	// position in main string
	as->secondaries[as->secondarycount].base = as->seclength;	// base of segment
	as->sectext[as->seclength++] = charcode;			// add class marker
	as->sectext[as->seclength++] = cc;
	as->secondarycount++;
	as->secondaries[as->secondarycount].base = as->seclength;	// base of what will be next segment (need it set for textmatch)
	return charcode;
}
#else
/***************************************************************************/
static unichar capturechar(COLLATORTEXT * as, unichar cc, SORTPARAMS * sgp)

{
//	unichar charcode = u_isdigit(cc) ? sgp->numcode : sgp->symcode;	// get class marker
	as->secondaries[as->secondarycount].offset = as->length;	// position in main string
	as->secondaries[as->secondarycount].base = as->seclength;	// base of segment
//	as->sectext[as->seclength++] = charcode;			// add class marker
	as->sectext[as->seclength++] = cc;
	as->secondarycount++;
	as->secondaries[as->secondarycount].base = as->seclength;	// base of what will be next segment (need it set for textmatch)
	return cc;
}
#endif
/***************************************************************************/
static void capturesecondary(COLLATORTEXT * as, char * string, int *indexptr, int length)

{
	int limit = *indexptr+length;
	int startlength = as->seclength;

	as->secondaries[as->secondarycount].offset = as->length;	// position in main string
	as->secondaries[as->secondarycount].base = as->seclength;	// base of segment
	while (*indexptr < limit)	{	// add to secondary text string
		if (!capturecodes(as,string,indexptr))	{	// if haven't captured any codes
			unichar c;
			U8_NEXT_UNSAFE(string,*indexptr,c);	// convert char, increment sindex
			as->sectext[as->seclength++] = c;
		}
	}
	if (as->seclength > startlength)	{
		as->secondarycount++;
		as->secondaries[as->secondarycount].base = as->seclength;	// base of what will be next segment (need it set for textmatch)
	}
}
/***************************************************************************/
static void capturesecondarystring(COLLATORTEXT * as, char * string, int *indexptr, unichar cc, SORTPARAMS * sgp)

{
//	static unichar specials[] = {'.',':',';',',',KEEPCHR,ESCCHR,SPACE,OBRACE,CBRACE,OBRACKET,OPAREN,DASH,SLASH,0};
	
	as->secondaries[as->secondarycount].offset = as->length;	// position in main string
	as->secondaries[as->secondarycount].base = as->seclength;	// base of segment
#if !REORDER
	as->sectext[as->seclength++] = u_isdigit(cc) ? sgp->numcode : sgp->symcode;	// add class marker
#endif
	as->sectext[as->seclength++] = cc;
	as->secondarycount++;
	while (string[*indexptr])	{	// while not exhausted string
		if (!capturecodes(as,string,indexptr))	{	// if haven't captured any codes
			U8_GET_UNSAFE(string,*indexptr,cc);	// convert char at current position
			if (u_strchr(specials,cc))		// if char for special handling
				break;
			if (u_isalpha(cc) || sgp->evalnums && u_isdigit(cc))
				break;
			U8_FWD_1_UNSAFE(string,*indexptr);	// was secondary char, so increment position
			as->sectext[as->seclength++] = cc;
		}
	}
	as->secondaries[as->secondarycount].base = as->seclength;	// base of what will be next segment (need it set for textmatch)
}
/******************************************************************************/
static char * prepstring(char * dest, char * source, char * slist)	// makes dest from source, with substitutions from list

																	// substitution list is compound string containing runs of pairs <target><replacement>
{
	long matchlen, replen;

	strcpy(dest, source);
	for (char * slptr = slist; *slptr != EOCS; slptr += matchlen + replen + 2) {	// for all sub strings
		char * dptr = dest, *mptr, *repptr;
		matchlen = strlen(slptr);
		repptr = slptr + matchlen + 1;
		replen = strlen(repptr);
		while (mptr = strstr(dptr, slptr)) {
			int uindex = 0;
			unichar cc;
			char * xptr;
			if (iscodechar(*(mptr - 1))) {	// if got accidental pickup on code value
				dptr = mptr + 1;		// skip it
				continue;
			}
			for (xptr = mptr; iscodechar(*(xptr - 2)); xptr -= 2)	// now skip back cleanly over any codes
				;
			U8_PREV_UNSAFE(xptr, uindex, cc);	// get unichar before match
			if (!u_isalnum(cc)) {	// if prior char not alnum
									// check whether match is inside {..} or <..>
				int bracket = 0, brace = 0;
				for (char * cptr = mptr; cptr > dptr; cptr--) {
					xptr = cptr - 1;
					if (*cptr == '{' && *xptr != '\\' && !iscodechar(*xptr))
						brace++;
					else if (*cptr == '}' && *xptr != '\\' && !iscodechar(*xptr))
						brace--;
					else if (*cptr == '<' && *xptr != '\\' && !iscodechar(*xptr))
						bracket++;
					else if (*cptr == '>' && *xptr != '\\' && !iscodechar(*xptr))
						bracket--;
				}
				if (!bracket && !brace) {	// if not enclosed, replace match with rep<match>
					memmove(mptr + matchlen + replen + 2, mptr + matchlen, strlen(mptr + matchlen) + 1);
					strncpy(mptr, repptr, replen);
					mptr += replen;
					*mptr++ = '<';
					strncpy(mptr, slptr, matchlen);
					mptr += matchlen;
					if (!replen && cc == SPACE && *(mptr + 1) == SPACE)	// if no replacement text and removing target would leave 2 successive spaces
						*mptr++ = SPACE;		// force trailing space inside <>
					*mptr++ = '>';
					dptr = mptr;
				}
				else	// skip enclosed target
					dptr = mptr + matchlen;
			}
			else	// skip false target
				dptr = mptr + matchlen;
		}
	}
//	NSLog(dest);
	return dest;
}
