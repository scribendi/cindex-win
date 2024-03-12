#pragma once

enum {	/* index style modifier */
	FL_NONE,   		/* indented */
	FL_RUNBACK,		/* runback if no references */
	FL_MODRUNIN,	/* runin unless some head at this level has lower level */
	FL_NOSUPPRESS	/* don't suppress repeated headings */
};

enum {	/* indent types */
	FI_NONE,		/* none */
	FI_AUTO,		/* auto */
	FI_FIXED,		/* fixed */
	FI_SPECIAL		/* special last indent */
};

enum {	/* line spacing modes */
	FS_SINGLE,		/* single */
	FS_ONEANDHALF,	/* 1.5 */
	FS_DOUBLE		/* double */
};

enum {		/* heading continuation flags */
	RH_NEVER,		/* no repeated headings */
	RH_PAGE,		/* after page break only */
	RH_COL			/* after column break */
};

enum	{		/* field control flags */
	FH_SUPPRESS = 1
};

#define L_SPECIAL FIELDLIM-2		/* token heading level for special indent */

enum	{		/* keys for grouping entries */
	SYMBIT = 1, 
	NUMBIT = 2,
	BOTHBIT = (SYMBIT|NUMBIT)
};

enum	{	/* see also cross-ref placement flag */
	CP_HASPAGE = 2
};

enum {		/* abbreviation rules */
	FAB_NONE,
	FAB_CHICAGO,
	FAB_HART,
	FAB_FULL
};

enum {				/* special values for flags on indent field */
	FO_RFLAG = 0x10,	/* identifies runover line */
	FO_BLANK = 0x20,	/* identifies blank line */
	FO_CONTIN = 0x40,	/* identifies continuation entry */
	FO_AHEAD = 0x80,	/* identifies group header */
	FO_FORCERUN = 0x100,	/* flag forced forced run-on */
	FO_LABELED = 0x200,	/* labeled line */
	FO_HMASK = 0xF		/* mask for bits that define heading level */
};

enum	{				/* format control characters */
	FO_LINEBREAK = 14,	/* line break without increasing level count */
	FO_LEVELBREAK,	/* line break with increasing level count */
	FO_NEWLEVEL,	/* line break with increasing level count */
	FO_RPADCHAR,	/* character denotes need for right padding */
	FO_PAGE,		/* insertion pt for page ref tag */
	FO_CROSS,		/* insertion pt for cross-ref tag */
	FO_EPAGE,		/* insertion pt for end page ref tag */
	FO_ECROSS,		/* insertion pt for end cross-ref tag */
	FO_ELIPSIS		// denotes elipsis in formatted output
};

typedef struct {	/* character style/position structure */
	short style;	/* character style */
	short cap;		/* capitalization type */
	char spare[4];	/* !!spare */
} CSTYLE;

typedef struct	{		/* margins & columns */
	unsigned short top;
	unsigned short bottom;
	unsigned short left;
	unsigned short right;
	unsigned short ncols;
	unsigned short gutter;
	short reflect;
	short pgcont;		/* repeat broken head after break */
	char continued[FSSTRING];	/* 'continued' text */
	CSTYLE cstyle;
	short clevel;		/* level down to which want continuation */
	long spare[128];		/* !!spare */
} MARGINCOLUMN;

typedef struct {	/* locator segment style structure */
	CSTYLE loc;		/* locator itself */
	CSTYLE punct;	/* trailing punctuation */
	long spare[16];		/* !!spare */
} LSTYLE;

typedef struct	{	/* header & footer content */
	char left[FTSTRING];
	char center[FTSTRING];
	char right [FTSTRING];
	CSTYLE hfstyle;		/* style */
	char hffont[FSSTRING];		/* font */
	short size;			/* size */
	long spare[64];	/* !!spare */
} HEADERFOOTER;

typedef struct {	/* for the moment just a placeholder */
	short porien;			/* paper orientation */
	short psize;			/* encoded paper size */
	short pwidth;			/* override width in 1/10 mm */
	short pheight;			/* override length in 1/10 mm */
	short pwidthactual;		/* width in points */
	short pheightactual;	/* height in points */
	short xoffset;			/* offset from edge to printable area */
	short yoffset;			/* offset from top to printable area */
	long spare[64];
} PAPERINFO;

typedef struct {		/*	page format */
	MARGINCOLUMN mc;
	HEADERFOOTER lefthead;
	HEADERFOOTER leftfoot;
	HEADERFOOTER righthead;
	HEADERFOOTER rightfoot;
	short linespace;		/* line spacing (single, double, etc.) */
	short firstpage;
	short lineheight;		/* line height */
	short entryspace;		/* space between entries*/
	short above;			/* space above group header */
	char lineunit;			/* unit of measurement for vertical spacing */
	char autospace;			/* TRUE if line-spacing derived automatically */
	char dateformat;		/* date format index */
	char timeflag;			/* TRUE when to add time to date */
	PAPERINFO pi;			/* paper info */
	char numformat;			/* page numbering format */
	char orientation;		// 0 portrait; 1 landscape
	char alignment;			// text alignment
	char cspare;			/* !!spare */
	long spare[128];		/* !!spare */
} PAGEFORMAT;

typedef struct {			/* cross-ref punctuation structure */
	char cleada[FMSTRING];	/* lead text for open ref (see also) */
	char cenda[FMSTRING];	/* end text */
	char cleadb[FMSTRING];	/* lead text for blind ref (see) */
	char cendb[FMSTRING];	/* end text */
	long spare[64];
} CROSSPUNCT;

typedef struct {			/* cross-reference formatting */
	CROSSPUNCT level[2];	/* array of 2 punctuation structures (head & subhead) */
	CSTYLE leadstyle;		/* lead style */
	CSTYLE bodystyle;		/* body style */
	char subposition;		/* how subhead 'see also' placed in entry */
	char mainposition;		/* how main head 'see also' placed in entry */
	char sortcross;			/* TRUE: arrange refs */
	char suppressall;		/* suppress all cross refs */
	char subseeposition;	/* how subhead 'see' placed in entry */
	char mainseeposition;	/* how main head 'see' placed in entry */
	char suppressifbodystyle;	// suppresses lead style if present in body style
	char spare[127];			/* !!spare */
} CROSSREFFORMAT;

typedef struct	{		/* locator formatting */
	char sortrefs;			/* TRUE: arrange refs */
	char rjust;				/* right justify */
	char suppressall;		/* suppress whole ref */
	char suppressparts;		/* suppress repeated parts */
	char llead1[FMSTRING];	/* lead text for single ref */
	char lleadm[FMSTRING];	/* lead text for multiple refs */
	char trail[FMSTRING];	/* trailing text for refs */
	char connect[FMSTRING];	/* connecting sequence */
	short conflate;			/* threshold for conflation */
	short abbrevrule;		/* abbreviation rule */
	char suppress[FMSTRING];	/* suppress to last of these chars */
	char concatenate[FMSTRING];	/* concatenate with this sequence */
	LSTYLE lstyle[COMPMAX];	/* segment style sequence */
	char leader;			/* leader type */
	char noduplicates;		// hides duplicate refs
	char sparex[2];
	long spare[64];			/* !!spare */
} LOCATORFORMAT;

typedef struct	{	/* grouping of entries */
	short method;		/* grouping method */
	char gfont[FSSTRING];	/* font */
	CSTYLE gstyle;		/* style */
	short gsize;		/* text size */
	char title[FSSTRING];	/* title/format string */
	char ninsert[FSSTRING];	// generic number token
	char sinsert[FSSTRING];	// generic symbol token
	char nsinsert[FSSTRING];// generic symbol and number token
	long spare[128];			/* !!spare */
} GROUPFORMAT;

typedef struct	{	/* individual field layout */
	char font[FSSTRING];	/* font */
	short size;			/* size */
	CSTYLE style;		/* style */
	float leadindent;	/* first line indent (points) */
	float runindent;	/* runover indent (points) */
	char trailtext[FMSTRING];	/* trailing text for when no ref */
	long flags;
	char leadtext[FMSTRING];	/* leading text */
	long spare[32];			/* !!spare */
} FIELDFORMAT;

typedef struct	{		/* overall heading layout */
	short runlevel;			/* runon level */
	short collapselevel;	/* level at which entries collapsed */
	char style;				/* index style modifiers */
	char itype;				/* indentation type (auto, etc) */
	char adjustpunct;		/* adjust punctuation */
	char adjstyles;			/* adjust style codes around punctuation */
	char fixedunit;			/* unit for fixed spacing (0 is em) */
	char autounit;			/* unit for auto spacing (0 is em) */
	float autolead;			/* space for auto lead */
	float autorun;			/* space for autorunin */
	GROUPFORMAT eg;
	CROSSREFFORMAT cf;
	LOCATORFORMAT lf;
	FIELDFORMAT field[FIELDLIM-1];	/* field info */
	long spare[128];			/* !!spare */
} ENTRYFORMAT;

typedef struct	{		/* overall format structure */
	unsigned long fsize;	/* size of structure */
	unsigned short version;	/* version number */
	PAGEFORMAT pf; 
	ENTRYFORMAT ef;
	long spare[256];		/* !!spare */
} FORMATPARAMS;

typedef struct {		/* stylesheet */
	char endian;
	FORMATPARAMS fg;
	FONTMAP fm;
	short fontsize;
	long spare[32];
} STYLESHEET;
