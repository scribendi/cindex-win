#pragma once
#include "search.h"

typedef struct {		/* contains data/functions for window */
	WFLIST wstuff;			/* basic set of window functions */
	INDEX * lastindex;		/* ptr to last touched */
	LISTGROUP lr;	/* group of list structures */
	long vacount;			/* view window activation count */
	short restart;			/* TRUE if changed some search parameter */
	short scope;			/* scope of search */
	short dateflag;			/* true if selected dates */
	LRESULT (CALLBACK *edproc)(HWND, UINT, WPARAM, LPARAM );	/* saved combo procedure */
	void (*init)(HWND hwnd, short resume, BOOL protect);	/* initializing function */
	RECN target;			/* target for replacement */
	short resetting;		/* TRUE when resetting box */
	REPLACEGROUP rg;		/* replacement structure */
	REPLACEATTRIBUTES ra;		/* attrib structure */
	short offset;			/* offset of match or search position */
	short mlength;			/* length of matching text */
	RECN repcount;			/* replacement count */
	RECN markcount;			/* count of marked records */
	struct numstruct *nptr;	/* numstruct array for resorting */
} RFLIST;

#define RX(WW,II) (((RFLIST *)GetWindowLongPtr(WW,GWLP_USERDATA))->II)	/* for addressing items */

HWND rep_setwindow(HWND hwnd);	/*  sets up rep window */
struct numstruct * rep_setup(INDEX * FF, LISTGROUP * lg, REPLACEGROUP * rg, REPLACEATTRIBUTES *rap,char * replace);	/* sets up structures */
