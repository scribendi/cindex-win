#pragma once

#include "viewset.h"

#define EBUFSIZE (100*1024)	/* size of entry build buffer */

#define ESCSIZE 8			/* number of diff type styles */

#define MAXCOLS 6			/* max numbre of columns */

typedef struct {		/* information about entry */
	short ulevel;		/* unique level */
	short llevel;		/* lowest level text field */
	short ahead;		/* TRUE if this is ahead */
	short leadchange;	/* TRUE if lead changes */
	short firstrec;		/* TRUE if first record in view */
	char * epos;		/* end position */
	short prefs;		/* number of page refs */
	short crefs;		/* number of cross-refs */
	long drecs;			/* number of unique records consumed in building it */
	short forcedrun;	/* level of forced run-on heading */
	int consumed;	// number of records consumed (including non-unique)
} ENTRYINFO;

extern char f_stringbase[];	/* holds strings that define dynamically-built formatting tags */
extern unsigned char *f_entrybuff;	/* big array for building entries (grabbed at startup) */
extern TCHAR f_widebuff[];	// unicode buffer

TCHAR * form_buildentry(INDEX * FF, RECORD * recptr, ENTRYINFO* esp);	/* builds entry text */
RECORD * form_getprec(INDEX * FF, RECN rnum);	/* returns ptr to record (or parent) */
RECORD * form_skip(INDEX * FF, RECORD * recptr, short dir);	/* skips in formatted mode */
char *form_formatcross(INDEX * FF, char * source);	/* formats cross-ref into dest string */
short form_disp(HWND wptr, HDC dc, short line, short lcount);	/* displays lines of record */
short form_measurelines(HWND wptr, HDC dc, RECORD * recptr, LSET *array, struct selstruct * slptr);	
char *form_stripcopy(char *dptr, char *sptr);	 /* copies, skipping over braced text */
void form_putline(HDC dc,TCHAR * source, ATTRIBUTES * as, TCHAR * eptr, short visible, short free, char leadtype);      /* return first printable char */
