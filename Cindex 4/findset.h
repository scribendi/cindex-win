#pragma once

typedef struct fwstruct {	/* contains data/functions for window */
	WFLIST wstuff;			/* basic set of window functions */
	INDEX * lastindex;		/* ptr to last touched */
	LISTGROUP lg;	/* array of list structures */
	long vacount;			/* view window activation count */
	short restart;			/* TRUE if changed some search parameter */
	short scope;			/* scope of search */
	short dateflag;			/* true if selected dates */
	LRESULT (CALLBACK *edproc)(HWND, UINT, WPARAM, LPARAM );	/* saved combo procedure */
	void (*init)(HWND dptr, short resume, BOOL protect);	/* initializing function */
	RECN target;			/* target for replacement */
	short resetting;		/* TRUE when resetting box */
	int heightstep;			/* gap between sets of bool fields */
	int baseheight;		/* base height of window */
	int activegroup;	// index of current LIST
} FFLIST;

#define FX(WW,II) (((FFLIST *)GetWindowLongPtr(WW,GWLP_USERDATA))->II)	/* for addressing items */

enum {		/* field menu fixed items */
	FP_ALL = 0,
	FP_ALLTEXT,
	FP_LASTTEXT,
	FP_PAGE,
	FP_MAIN
};

#define MAXLISTS 4				/* number of groups */

extern HWND f_lastwind;

void fs_findagain(INDEX * FF, HWND hwnd);	/* finds next matching record */
short fs_testok(INDEX * FF);	/* checks if can do a blnd find */
HWND find_setwindow(HWND hwnd);	/*  sets up find window */
LONG_PTR fs_installcombohook(HWND hwnd,int item);		// install combo hook
void fs_setpos(HWND hwnd);	/* sets the position of the find/rep dialog box */
void fs_setfieldmenu(INDEX * FF, HWND hwnd);	/* sets field menu */
int fs_getfieldindex(HWND ctl);		/* gets field index from list box */
void fs_hotkey(HWND hwnd, int keyid);      /* use hotkey text for search */
void fs_setwordenable(HWND hwnd, int item, LIST * ls);	/* sets word checkbox by text contents */
int fs_findnext(INDEX * FF, HWND hwnd, LISTGROUP *lg, short * offset, short * lptr, short rflag);
int fs_getstyle(HWND hwnd, INDEX * FF, LIST * ll);	// gets find attributes
void fs_showstyle(HWND hwnd, int item, unsigned char style, unsigned char font, unsigned char forbiddenstyle, unsigned char forbiddenfont);	/* displays style settings */
void fs_reset(HWND hwnd, LISTGROUP * lg, int findflag);	/* resets search sturct & common window items to start condition */
void fs_setlabelcombo(HWND hwnd);		// set label combo box
void fs_configureforpattern(HWND hwnd, int base, boolean state);	// configures group for pattern enabled/disabled
