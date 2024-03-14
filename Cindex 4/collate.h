/*
 *  collate.h
 *  Cindex
 *
 *  Created by Peter Lennie on 12/31/10.
 *  Copyright 2010 Peter Lennie. All rights reserved.
 *
 */

#pragma once

#include "attributedstrings.h"

enum {		/*  definitions of sort types */
	RAWSORT,
	LETTERSORT,
	LETTERSORT_CMS,
	LETTERSORT_ISO,
	WORDSORT,
	WORDSORT_CMS,
	WORDSORT_ISO,
	LETTERSORT_SBL,
	WORDSORT_SBL
};

#define iswordsort(A) ((A) >= WORDSORT && (A) != LETTERSORT_SBL)
#define islettersort(A) ((A) >= LETTERSORT && (A) < WORDSORT || (A) == LETTERSORT_SBL)

enum {	/* text comparison flags */
	MATCH_LOOKUP = 1,	// lookup match (up to length of first string)
	MATCH_CHECKPREFIX = 2,	/* does cross-ref and prefix checks */
	MATCH_IGNORECASE = 4,	/* accepts match in either case */
	MATCH_IGNOREACCENTS = 8,	// ignores accent differences
	MATCH_IGNORECODES = 16,	/* ignores codes for comparing residues */
	MATCH_IGNOREPUNCT = 32,	/* ignores residues that are entirely punctuation */
	MATCH_FIRSTCHAR = 64	// stops loading COLLATORTEXT after first primary character
};

enum {	/* definitions of character priorities */
	SORT_SYMBOL,
	SORT_NUMBER,
	SORT_LETTER
};

enum  {		// collator modes
	COL_FULL = 0,
	COL_LOOKUP
};

typedef struct {
	int offset;	// position in source string
	int base;		// base in secondary string
} SECONDARYSET;

typedef struct {
	int index;		// position in string
	int seccount;	// number of secondaries before break
} BREAKSET;

typedef struct {
	unichar * string;
	unsigned int length;
	CODESET * codesets;
	unsigned int codecount;
	unichar * sectext;
	unsigned int seclength;
	SECONDARYSET * secondaries;
	unsigned int secondarycount;
	int crossrefvalue;
	int hasdigits;
	BREAKSET * breaks;
	int breakcount;
	char * scratch;
} COLLATORTEXT;

typedef struct {
	char id[4];
	char localeID[32];
	char name[64];
	char script[32];
	int direction;
} LANGDESCRIPTOR;

extern LANGDESCRIPTOR * cl_languages;
extern int cl_languagecount;
extern TCHAR * cl_sorttypes[];

void col_findlocales(void);		// finds list of locales with sorting rules
LANGDESCRIPTOR * col_fixLocaleInfo(SORTPARAMS * sgp);	// gets and fixes locale info for sort language (needed to handle sort params before LOCALESORTVERSION)
LANGDESCRIPTOR * col_getLocaleInfo(SORTPARAMS * sgp);	// retrieves locale info from locale id
void col_init(SORTPARAMS * sgp, INDEX * FF);		// initializes collator
COLLATORTEXT * col_createstringforsize(int length);
void col_loadUTF8string(COLLATORTEXT * as,INDEX * FF, SORTPARAMS * sgp, char *string, int flags);	// loads COLLATORTEXT from xstring
void col_free(COLLATORTEXT * cs);
short col_match(INDEX * FF, SORTPARAMS * sgp, char *s1, char *s2, short flags); /* compares text field by current rules */
void col_buildkey(INDEX * FF, char *key, char * string);	// builds sort key as utf-8 string
int col_collatablelength(INDEX * FF, char * string);	// returns length of primary string
BOOL col_newlead(INDEX * FF, char * string1, char * string2, unichar * lead);	// returns TRUE if s2 and s1 have different lead
void col_describe(COLLATORTEXT * col,SORTPARAMS * sgp);	// describes content 
