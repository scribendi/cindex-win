#pragma once

#include "mfile.h"

enum {
	W_DELFLAG = 1,
	W_TAGFLAG = 2,
	W_GENFLAG = 4,
	W_NEWTAGS = 8+16,
	W_PUSHLAST = 128
};

typedef struct index INDEX;

typedef unichar LMONTH[20];

typedef struct {
	UCollator * ucol;
	int mode;
	LMONTH longmonths[12];
	LMONTH shortmonths[12];
	UNormalizer2 * unorm;
} COLLATOR;

#define MAXTSTRINGS 8	/* maximum number of translations for ASCII chars */

typedef struct fxc {		/* file writing control struct */
	short filetype;			// type of file to write
	char * esptr;		/* output string ptr */
	BOOL (*efstart) (INDEX *FF, struct fxc * fxp);	/* setup function */
	void (*efend) (INDEX * FF, struct fxc * fxp);	/* cleanup function */
	void (*efembed)(INDEX * FF, struct fxc * fxp, RECORD * recptr);	/* forms embedded entry */
	void (*efwriter) (struct fxc * fxp, unichar uc);	// emits unicode character
	char **structtags;		/* heading style tags */
	char **styleset;		/* table of style code strings */
	char **fontset;			/* table of font code strings */
	char **auxtags;			// strings for auxiliary tags
	char *newline;			/* optional line (not para) break */
	char *newpara;			/* string that defines paragraph break */
	char *newlinestring;	/* obligatory new line (mac or dos) */
	char *tab;				/* tab code */
	char * protected;		/* string of protected characters */
	char * pstrings[MAXTSTRINGS];		/* translation strings for protected characters */
	BOOL usetabs;			/* TRUE if to define lead indent with tabs */
	BOOL suppressrefs;		/* TRUE if suppressing ref lead/end when tagging */
	BOOL individualrefs;	/* TRUE if tagging refs individually */
	short individualcrossrefs;	/* TRUE if tagging refs individually */
	BOOL nested;			// nested tags
	short internal;			/* code set is internal (RTF, etc) */
	char stylenames[FIELDLIM][FNAMELEN];	// style names
} FCONTROLX;

struct index {		/* runtime index structure */
	HEAD head;				/* index header */
	TCHAR iname[_MAX_FNAME+_MAX_EXT];	/* index name */
	struct index * inext;	/* pointer to next index */
//	char readonly;			/* TRUE if index is read only */
	char ishidden;			/* can't be used explicitly */
	time_c opentime;		/* time at which index opened */
	time_c lastflush;		/* time of last flush */
	FILETIME modtime;		/* time file last modified */
	RECN lastfound;			/* last record found in search */
	RECN startnum;			/* no of records in index when opened */
	RECN lastedited;		/* last record edited */
	short wholerec;			/* size of record including header */
	HWND vwind;				/* index viewing window */
	HWND rwind;				/* record editing window */
	HWND twind;				/* text edit window */
	HWND cwind;				// container window
	HWND rcwind;			// container window
	HWND currentfocus;		// window with current focus
	short viewtype;			/* flags indcate what view we have */
	GROUPHANDLE lastfile;	/* file for most recent search */
	GROUPHANDLE curfile;	/* file used for skip, etc */
	int groupcount;			/* holds count of number of groups */
	RECN curfilepos;		/* index of current record in group */
	TCHAR pfspec[MAX_PATH];	/* file spec from which index read/saved */
	TCHAR backupspec[MAX_PATH];	// path to backup file
	MFILE mf;				// file map
	RECN recordlimit;		// highest numbered record we can accommodate
	GROUP * gbase;				// base of groups
	struct index * sumsource;	/* child index for managing summary entries */
	char wasdirty;			/* TRUE if index was ever dirty */
	char wasedited;			/* TRUE if record window ever used */
	PRINTFORMAT pf;			/* page format structure */
	short stylecount;		/* count of styled strings */
	CSTR slist[AUTOSIZE];	/* parsed styled strings */
	BOOL continued;			// TRUE when working on continued heading
	unsigned int singlerefcount;	// holds count of locators
	BOOL overlappedrefs;		// TRUE when some page reference has overlapping range
	BOOL nostats;			// suppress collection of entry stats
	int acount;				// activation count (find/rep/spell)
	BOOL holdsizechange;	// prevents main view size change on opening record window
	COLLATOR collator;
	FCONTROLX * typesetter;
	BOOL needsresort;		// TRUE when needs resort;
	BOOL righttoleftreading;	// right to left reading
};

typedef struct wstruct {			/* contains data/functions for window */
	INDEX *owner;					/* parent index */
} WFLIST;

#define WX(WW,II) (((WFLIST *)GetWindowLongPtr(WW,GWLP_USERDATA))->II)	/* for addressing items */
