#pragma once
enum {		/* V1.0 structure tag indexes */
	STR_1BEGIN = 0,
	STR_1END,
	STR_1AHEAD,
	STR_1MAIN,
	STR_1MAINEND = STR_1MAIN + 15,
	STR_1PAGE = STR_1MAIN + 30,
	STR_1CROSS = STR_1PAGE + 1
};

enum {		// tag types
	XMLTAGS = 0,
	SGMLTAGS
};

enum {		/* structure tag indexes */
	STR_BEGIN = 0,
	STR_END,
	STR_GROUP,
	STR_GROUPEND,
	STR_AHEAD,
	STR_AHEADEND,
	STR_MAIN,
	STR_MAINEND = STR_MAIN + 15,
	STR_PAGE = STR_MAIN + 30,
	STR_PAGEND,
	STR_CROSS,
	STR_CROSSEND,

	T_STRUCTCOUNT	// number of structure tags
};

enum {		/* 'other' tag indexes */
	OT_PR1,			/* first protected character */
	OT_ENDLINE = MAXTSTRINGS * 2,	/* end of line (after sets of protected chars) */
	OT_PARA,		/* end of para */
	OT_TAB,			/* tab */
	OT_UPREFIX,		// unicode prefix
	OT_USUFFIX,		// suffix
	// following 2 structure tags stuck here so that tagsets can be backward compatible with Cindex 3
	// reading tags reads to the end of the EOCS, so V3 ignores these
	OT_STARTTEXT,	// start heading text
	OT_ENDTEXT,		// end heading text

	T_OTHERCOUNT	// number of aux tags
};

#define TAGMAXSIZE 5000			/* max size of tag set */

#define V10_STRUCTCOUNT 35		/* number of structure tags */
#define V10_STYLEBASE  V10_STRUCTCOUNT	/* index to first style string */

//#define T_STRUCTCOUNT 40		/* number of structure tags */
#define T_STYLECOUNT 14			/* number of style tags */
#define T_FONTCOUNT 24			/* number of font tags */
//#define T_OTHERCOUNT 21			/* miscellaneous tags */

#define T_STRUCTBASE 0			/* index of first structure string */
#define T_STYLEBASE (T_STRUCTBASE+T_STRUCTCOUNT)	/* index to first style string */
#define T_FONTBASE (T_STYLEBASE+T_STYLECOUNT)	/* index to first font string */
#define T_OTHERBASE (T_FONTBASE+T_FONTCOUNT)	/* index to first other string */
#define T_NUMTAGS (T_OTHERBASE+T_OTHERCOUNT)	/* index to first character code */
#define T_NUMFONTS (T_FONTCOUNT/2)				/* number of fonts */

typedef struct {	/* tagset structure */
	short tssize;	/* size of TAGSET */
	long total;		/* complete size of object */
	short version;	/* version of tags */
	char extn[4];	/* default extension for output file */	
	char readonly;	/* TRUE if set unmodifiable */
	char suppress;	/* TRUE if suppressing ref leader & trailer */
	char hex;		// true when hex; otherwise decimal
	char nested;	// TRUE when nested
	char fontmode;	// font tag type
	char individualrefs;	// TRUE if references coded individually
	char levelmode;	// heading level tag type
	char useUTF8;	// encode uchars as utf8 (SGML only)
	long spare[5];
	char xstr[];	/* base of compound string */
} TAGSET;

#define TS_VERSION 2	/* tag set version */

TCHAR * ts_getactivetagsetname(int type);	// gets active tagset name
TCHAR * ts_getactivetagsetpath(int type);	// gets active tagset for type
void ts_managetags(HWND hwnd);	/* manages tag set */
TAGSET * ts_openset(TCHAR * tagname);		/* returns pointer to tag set */
TAGSET * ts_open(TCHAR * tagname);		/* (for api) returns pointer to named tag set */
