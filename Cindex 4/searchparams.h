#pragma once

#include "unicode/uregex.h"

#define MAXLISTS 4
#define LISTSTRING 256

typedef struct {		/* structure for finding text in records */
	char string[LISTSTRING];	/* string being sought (or expression) */
	short field;		/* field to search */
	short spare1;		// spare
	char patflag;		/* is pattern */
	char caseflag;		/* is case sensitive */
	char notflag;		/* logical not */
	char andflag;		/* logical and */
	char evalrefflag;	/* evaluate references (if page field) */
	char wordflag;		/* match whole word */
	unsigned char style;	/* style, etc */
	unsigned char font;
	unsigned char forbiddenstyle;
	unsigned char forbiddenfont;
	short spare0;
#if (defined __LP64__ || defined _M_X64)
	URegularExpression * regex;	// regular expression handler
	char * auxptr;		/* pointer to auxiliary buffer */
	char * ref2ptr;		/* pointer to second ref in a range */
	CSTATE entrycodes;
	CSTATE exitcodes;
	int spare[27];		// spare
#else
	URegularExpression * regex;	// regular expression handler
	char * auxptr;		/* pointer to auxiliary buffer */
	char * ref2ptr;		/* pointer to second ref in a range */
	CSTATE entrycodes;
	CSTATE exitcodes;
	int spare[30];		// spare
#endif
} LIST;

typedef struct {		/* set of list structures */
	char userid[5];		// user id
	short lflags;		/* flags that say something about selection */
	short size;			/* number in set */
	char revflag;		/* search backwards */
	char excludeflag;	// excludes records with specified attributes
	char newflag;		/* search for new */
	char modflag;		/* search for modified */
	char markflag;		/* search for marked */
	char delflag;		/* search for deleted */
	char genflag;		/* search for generated */
	char sortmode;		/* search with sort on/off */
	char tagflag;		/* search for flagged */
	int tagvalue;		// kind of tags to search for
	RECN firstr;		/* first record to find */
	RECN lastr;			/* record to stop on */
	time_c firstdate;	/* first date */
	time_c lastdate;	/* last date */
	char range0[FTSTRING];		// range start string
	char range1[FTSTRING];		// range end string
	long spare[64];
	LIST lsarray[MAXLISTS];
} LISTGROUP;

#define COUNTTABSIZE 512

typedef struct {
	unichar lead;
	RECN total;
}LEAD;

enum {			/* delete management flags for count */
	CO_ALL,
	CO_NODEL,
	CO_ONLYDEL
};

typedef struct {	/* structure containing pars for counting */
	char smode;			/* sort on or off */
	RECN firstrec;		/* first record examined */
	RECN lastrec;		/* last record examined */
	char delflag;		/* how to handle deleted records */
	char markflag;		/* TRUE if want only marked records */
	char genflag;		/* TRUE if want only generated */
	char tagflag;		/* TRUE if want only tagged records */
	char modflag;		/* TRUE if want only modified records */
	char firstref[FSSTRING];	/* string for first ref */
	char lastref[FSSTRING];		/* string for last ref */
	LEAD leads[COUNTTABSIZE];	// array of leads
	short deepest;		/* depth of deepest in index (fields) */
	RECN deeprec;
	short longest;		/* length of longest in index (chars) */
	short longestdepth;	// depth of longest record
	RECN longrec;
	unsigned long totaltext;	/* total length of text */
	short fieldlen[FIELDLIM];	/* length of longest in index */
	RECN modified;		/* modified total */
	RECN deleted;		/* deleted total */
	RECN marked;		/* marked total */
	RECN generated;		/* generated total */
	RECN labeled[FLAGLIMIT];		/* tagged total */
	long spare[128];
} COUNTPARAMS;

enum {					/* list flags */
	LF_GROUP = 1		/* from search in group */
};

#define SREPLIM 10		/* max number of replacement sequences */

typedef struct {		/* structure for replacement component */
	char *start;	/* start of literal replacement */
	short len;		/* length of literal replacement */
	short index;	/* index for substring replacement */
	short flag;		/* flag for case change */
} REPLACE;

typedef struct	{	/* structure for text attribute replacement */
	unsigned char onstyle;	/* holds add style info */
	unsigned char offstyle;	/* holds remove style info */
	short fontchange;		/* TRUE if setting font */
	unsigned char font[FSSTRING];
} REPLACEATTRIBUTES;

typedef struct {		/* set of replacement structures */
	short maxlen;		/* maximum length of text to work with */ 		
	char repstring[MAXREC];	/* assembled replacement string */
	short reptot;		/* number of replacement components */
	long failcount;		/* count of failed replacements */
	char sourcestring[STSTRING];	/* source of replacement parts */
	REPLACE rep[SREPLIM];	/* array of component structures */
	REPLACEATTRIBUTES ra;		/* attrib structure */
	URegularExpression * regex;	// regular expression handler
} REPLACEGROUP;

enum {			/* verification error types */
	V_TOOFEW = 1,		/* too few references */
	V_CIRCULAR,		/* circular reference */
	V_MISSING,		/* no target entry */
	V_CASE,			// case/accent error
	V_TYPEERR = 64	/* wrong type of reference */
};

typedef struct	{		/* offset, length of cross-ref text */
	short offset;
	short length;
	short error;
	short matchlevel;	/* heading level in target down to which we match */
	RECN num;			/* number of first target hit */
} VERIFY;

#define VREFLIMIT 10 
typedef struct  {		/* for managing verification */
	char * t1;			/* temp string (can hold whole record) */
	short lowlim;		/* min # matches we need */
	short fullflag;		/* need exact match */
	short locatoronly;	// look only in locator field
	short eflags;		/* error flag */
	short eoffset;		/* offset of last character of body of entry */
	short tokens;		// # tokens in prefix;
	VERIFY cr[VREFLIMIT];	/* offsets, etc of matches */
} VERIFYGROUP;

typedef struct {	/* for managing autogenerated ross-refs */
	RECN skipcount;		/* counts potential records that would have been too long */
	short maxneed;		/* size of entry needed to accommodate longest */
	short seeonly;		/* TRUE when generating 'see' refs only */
} AUTOGENERATE;
