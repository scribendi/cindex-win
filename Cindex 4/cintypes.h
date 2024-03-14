#pragma once

#define ON -1
#define OFF 0

#define MAXREC 2000     /* size of largest record */
#define FIELDLIM 16		/* max number of fields in record */
#define RECLIMIT 0xFFFFFFFF

#ifndef TOPREC			/* if not a demo */
#define TOPREC RECLIMIT
#endif

#define MAXKEYS 10		/* max # hot keys */

#define EOCS '\177'	 /* char at end of compound string */

#define EXRCHR '\023'	/* introduction to extended field in data file */

#define FONTCHR '\031'	// char that introduces font code
#define CODECHR '\032'	/* char that introduces type/font code */
#define BREAKCHR '\033'	/* char that stands for sort break */
#define NUMCHR '\034'   /* char that stands for a number (used in smatch) */
#define CROSSCHR '\035' /* char that stands for cross ref (in smatch) */
//#define MINSCHAR '\036'  /* a small character in smatch */
//#define BASECHR '\037'  /* character with which all sort fields begin by default */
//#define MAXSCHAR ((unsigned char)'\376')	/* a large character in smatch */

#define LINEBREAK "\xe2\x80\xa8"	// unicode line break character
#define utf8BOM "\xef\xbb\xbf"	// utf-8 BOM
#define REPLACECHAR 0xFFFD

#define NEWLINE '\n'	/* linefeed */
#define RETURN '\r'		/* return */

#define SPACE ' '
#define DASH '-'		/* dash */
#define SLASH '/'		/* slash */
#define	SORTBREAK '|'	/* sort break character */
#define ESCCHR '\\'		/* escape sequence introducer */
#define KEEPCHR '~'		/* char that forces next (normally ignored) char to be used in sorting */
#define OBRACKET '<'	/* opens string to ignored in sort */
#define CBRACKET '>'	/* closes string to ignored in sort */
#define OBRACE '{'		/* opens string to force sort */
#define CBRACE '}'		/* closes string to force sort */
#define OPAREN '('
#define CPAREN ')'
#define FORCEDRUN '|'	// triggers ad hoc run-on

#define OQUOTE 8220		// opening double quote
#define CQUOTE 8221		// closing double quote
#define ELIPSIS 0x2026	/* elipsis */
#define FGSPACE 0x2007	// figure space 
#define FSPACE 0x00A0	// fixed space (non-break space because figure not often available)
#define ENDASH 0x2013	/* en dash */
#define EMDASH 0x2014	/* em dash */
#define BULLET 0x2022	// bullet
#define NEQUAL 0x2260	// not equal
#define ENSPACE 0x2002	// en space
#define EMSPACE 0x2003	// em space

#define LRI 0x2066		// left-to-right isolate
#define RLI 0x2067		// right-to-left isolate
#define PDI 0x2069		// pop isolate
#define LRM 0x200E	// l-t-r mark
#define RLM 0x200F	// r-t-l mark
#define LRE	0x202A		// lr embedding
#define RLE 0x202B		// rl embedding
#define LRO	0x202D		// lr override
#define RLO 0x202E		// rl override
#define PDF 0x202C		// pop embedding
#define LRMARK "\xe2\x80\x8e"
#define LRISOLATE "\xe2\x81\xa6"
#define LRISOLATE "\xe2\x81\xa6"
#define POPISOLATE "\xe2\x81\xa9"
#define LROVERRIDE "\xe2\x80\xad"
#define LREMBED "\xe2\x80\xaa"
#define POPOVERRIDE "\xe2\x80\xac"

#define AUTOSIZE 50		/* max # of auto style strings */

#if 0
#define STSTRING 256	/* basic standard string */
#define FTSTRING 128
#define FSSTRING 32
#define FMSTRING 16
#define STYLESTRINGLEN 256

#else
// lengths of standard strings doubled between V2 and V3
#define STSTRING 512	
#define FTSTRING 256
#define FSSTRING 64
#define FMSTRING 32
#define STYLESTRINGLEN 512
#define HEADNOTELEN 1024

//end standard strings
#endif

#define STYLETYPES 4	// MAX number of page refs styles to deal with
#define REFTYPES 4      /* number of valid page reference types */
#define COMPMAX 10	   /* max # components evaluated in individual ref */
#define CHARPRI 3		/* number of character classes */

# if 0		// version 3
#define BASEVERSION 290
#define TOPVERSION 330
#define CINVERSION 300
#define FORMVERSION 300	/* version of format structure */
#else			// version 4
#define BASEVERSION 290
#define TOPVERSION 390
#define CINVERSION 310
#define FORMVERSION 300	/* version of format structure */
#endif

#define MINRECENT 4		/* min # recent files */
#ifdef PUBLISH
#define MAXRECENT 12	/* max # recent files */
#else
#define MAXRECENT 10
#endif //PUBLISH

enum	{	/* field selection flags */
	SUMMARYFIELDS = -4,
	ALLFIELDS = -3,
	ALLBUTPAGE = -2,
	LASTFIELD = -1,
	PAGEINDEX = (FIELDLIM-1)
};

enum {	/* date formats */
	DF_LONG,
	DF_ABBREV,
	DF_SHORT
};

enum {		/* format modes */
	VM_NONE,
	VM_DRAFT,
	VM_FULL,
	VM_SUMMARY
};

enum {				/* unit indices */
	U_INCH,
	U_MM,
	U_POINT,
	U_PICA
};

enum {		/* command scope flags */
	COMR_ALL,
	COMR_SELECT,
	COMR_RANGE
};
enum {			/* view types */
	VIEW_ALL,
	VIEW_NEW,
	VIEW_GROUP,
	VIEW_TEMP
};

#define FX_OLDOFF 128

enum {		/* style codes in control byte */
	FX_BOLD = 1,
	FX_ITAL = 2,
	FX_ULINE = 4,
	FX_SMALL = 8,
	FX_SUPER = 16,
	FX_SUB = 32,
	FX_OFF = 64,	/* off code */
	FX_STYLEMASK = (FX_OFF-1),
	FX_BOLDITAL = (FX_BOLD|FX_ITAL)
};

enum {		/* font codes in control byte */
	FX_COLOR = 32,	/* is color code */
	FX_FONT = 64,	/* is a font code */
	FX_AUTOFONT = 128,	/* valid only with FX_FONT; signifies setting base font */
	FX_FONTMASK = (FX_COLOR-1),
	FX_COLORMASK = (FX_COLOR-1)
};
