#pragma once

enum {			// export file types !! must match efilter list
	E_NATIVE,
	E_STATIONERY,
	E_XMLRECORDS,
	E_ARCHIVE,
	E_TAB,
//	E_DOS,			// restore if we want to export
	E_TEXTNOBREAK,
	E_RTF,
	E_XPRESS,
	E_INDESIGN,
	E_XMLTAGGED,
	E_TAGGED
};

typedef struct {
	char levelnames[512];
	int type;
	int spare;
} STYLENAMES;

typedef struct  {		/* export structure */
	short type;		/* file type */
	RECN first;		/* first record */
	RECN last;
	int firstpage;	// first page
	int lastpage;
	int encoding;	// character encoding
	BOOL includedeleted;	/* include deleted */
	BOOL extendflag;	/* write extended info */
	short minfields;	/* min # fields in tab/quote write */
	BOOL appendflag;	/* append to existing file */
	BOOL sorted;
	long records;	/* records written */
	short longest;	/* chars required by longest record */
	short usetabs;		/* TRUE when formatted write defines indents with tabs */
	RECN errorcount;	// count of records with character conversion errors
	STYLENAMES stylenames;
} EXPORTPARAMS;

struct expfile {		/* organizes info about export files */
	OPENFILENAME ofn;
	INDEX * FF;
	char rangestr1[STSTRING];
	char rangestr2[STSTRING];
	short rangemethod;
	short type;			/* file type */
	short override;		/* used when appending to existing file & circumventing replace box */
	EXPORTPARAMS exp;
};

enum {		/* formatted export range methods */
	RR_ALL,
	RR_PAGES,
	RR_SELECTION,
	RR_RECORDS
};

extern struct outfile extype[];

short exp_export(HWND wptr);	/* exports index */
short exp_rawexport(INDEX * FF, struct expfile * ef);	/* opens & writes raw export file */
