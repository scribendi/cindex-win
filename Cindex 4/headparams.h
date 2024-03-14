#pragma once

typedef struct {
	char endian;		// byte order
	char cinkey;		// cindex key
	long bspare;		// spare
	long headsize;				/* size of header */
	unsigned short version;		/* version # */
	RECN rtot;					/* total number of records */
	unsigned long groupsize;	// space required for groups
	time_c elapsed;				/* time spent executing commands */
	time_c createtime; 			/* time of creation  */
	short resized;				/* true if resized */
	short spare1;				/* spare */
	time_c squeezetime;			/* time of last squeeze */
	INDEXPARAMS indexpars;		/* index structure pars */
	SORTPARAMS sortpars;		/* sort parameters */
	REFPARAMS refpars;			/* reference parameters */
	PRIVATEPARAMS privpars;		/* private preferences */
	FORMATPARAMS formpars;		/* format parameters */
	char stylestrings[STYLESTRINGLEN];		/* strings for auto style */
	char flipwords[STSTRING];		// flipping prefixes for transposition
	char headnote[HEADNOTELEN];		// headnote
	FONTMAP fm[FONTLIMIT];		/* mapping of local IDs to font names (0 is default) */
	RECN root;					/* number of root of alpha tree */
	short dirty;				/* TRUE if any record written before close */
	// Mac only
	IRRect mainviewrect;
	IRRect recordviewrect;
	// Win only
	RECT vrect;					// view rect
	unsigned int mmstate;		// max min state
	//
	char spare[1024];		/* !!spare */
} HEAD;
#define HEADSIZE (sizeof(HEAD))
