#pragma once
#include "typestuff.h"
#include "recole.h"

#define SCREENBUFFSIZE 130	/* max # of lines in screen buffer */

struct selstruct {		/* text selection struct */
	RECN first;			/* first selected record in range */
	RECN last;			/* last selected record in range */
	short dir;			/* direction in which selection made */
	short startpos;		/* start char of range */
	short length;		/* length of range */
	short sline;		/* record line on which range starts */
	short shighlight;	/* start pixel pos on line */
	short eline;		/* record line on which range ends */
	short ehighlight;	/* end pixel pos on line */
};

enum	{		/* DLINE display flags */
	VNUMLINE = 1,	/* line on which to number the record */
	VSELECT = 2,	/* selected record (reverse video) */
	VFORCERUN = 4,	/* forced run-on */
	VLABELED = 8	/* line is labeled */
};

enum 	{		/* view_redisplay flags */
	VD_CUR = 1,		/* redisplay from record at top of screen */
	VD_TOP = 2,		/* position target at top of screen (default unless screen empty at bottom) */
	VD_SELPOS = 4,	/* put in standard selection position */
	VD_MIDDLE = 8,	/* position target record in middle of screen */
	VD_BOTTOM = 16, /* position target at bottom of screen */
	VD_RESET = 32,	/* reset of all display params */
	VD_IMMEDIATE = 64,	/* redraw immediately */
	VD_STABLESCROLL = 128	// force to center of screen	
};

typedef struct {			/* lineset structure */
	int offset;
	unsigned short indent;
	long free;
} LSET;

typedef struct {	/* struct for line display info */
	RECN rnum;		/* record number */
	unsigned char flags;	/* misc flags */
	short lnum;		/* line number of record */
	int base;		/* offset from start of string */
	int count;		/* # of chars on line */
	short indent;	/* lead indent (pixels) (or indicator for formatted) */
	short freespace;/* free space on line (points) */
	short selstart;	/* start pos of any selection */
	short selend;	/* end pos of selection */
} DLINE;

#define SEARCHLIMIT 32
typedef struct {	/* contains data/functions for window */
	WFLIST wstuff;	/* basic set of window functions */
	long acount;	/* activation count */
	HDC hdc;		/* device context */
	int printflag;	/* set when printing or writing to file */
	RECN vrange;	/* rec range applied to vert scroll bar */
	RECN stoprec;	/* record at which setlines stops layout */
	struct selstruct sel;	/* structure to specify selection */
	DLINE lines[SCREENBUFFSIZE];	/* array of line structs */
	short foffset;	/* offset line into top record for next display */
	RECT vrect;		/* view rect */
	RECT drect;		/* dest rect */
	RECT srect;		// view rect in screen coordinates
	RECORD * (*skip)(INDEX * FF, RECORD * recptr, short dir);	/* skips in appropriate mode */
	short (*measurelines)(HWND wptr, HDC dc,RECORD * recptr, LSET *array, struct selstruct * slptr);
	short (*disp)(HWND wptr, HDC dc, short line, short lcount);	/* display function */
	RECORD * (*getrec)(INDEX * FF, RECN rnum);	/* finds record (or parent) */
	long linedrop;	/* drop from top of window before drawing first line */
	long lspace;	/* natural spacing unit for font */
	short lheight;	/* line spacing used */
	short widows;	/* number of lines of entry to be put at top of next display */
	short maxindent;	/* lowest level heading in formatted record */
	int cwidth;		/* width of fixed space */
	int emwidth;	/* width of uppercase M (screen units) */
	int empoints;	/* width of uppercase M (points) 12/1/22 */
	short nlines;	/* display height in lines */
	short ncols;	/* number of columns (if formatted for printer) */
	short width;	/* display width in pixels */
	short nspace;	/* lead number gap (pixels) for display */
	short ndigits;	/* number of digit spaces used when showing numbers */
	long indent;	/* size of 2 em space indent */
	short target;	/* target line */
	UINT_PTR scrolltimer;/* is non-zero if a scroll timer is active */
	int lastthumbpos;	// last tracked thumb position
	int keytimer;	/* is non-zero when keystroke timer is active */
	UINT_PTR savetimer;	/* is non-zero when autosave timer is active */
	DTRGOBJ trgobj;	/* target object for drag/drop */
	BOOL dragsource;/* TRUE if we're the source of the drag */
	long lastkeytime;		/* time of last keystroke */
	int wheelresidue;	// fraction step for accumulating wheel movement
	char searchstring[SEARCHLIMIT];	/* keystroke buffer */
	char *sptr;
	char *hold;
	BOOL badstring;		/* stops attempts to find string */
	int selectdir;	// keystroke selection direction
	BOOL fillEnabled;	// allow counting filled records
	RECN visibleRecords;	// records potentially visible in view
	RECN filledRecords;	// number of records consumed in filling the screen
	int filledLines;
}LFLIST;

#define LX(WW,II) (((LFLIST *)GetWindowLongPtr(WW,GWLP_USERDATA))->II)	/* for addressing items */

#define view_recordselect(WP) (LX(WP,sel).first && LX(WP,sel).ehighlight < 0)

#ifdef PUBLISH
#define LLIMIT 500		/* max # display lines that can be occupied by one record */
#else
#define LLIMIT 250		/* max # display lines that can be occupied by one record */
#endif //PUBLISH

struct vcontext {		/* essential parts of viewing context for saving/restoring */
	char sort;			/* sort state */
	PRIVATEPARAMS priv;	/* private pars */
	DLINE topline;		/* contents of top line */
};

void view_allrecords(HWND hwnd);	/* displays all records */
short view_setfontsize(HWND wptr, HMENU mh, int item);	/* sets font size */
void view_showgroup(HWND wptr, char * gname);	/* displays group */
void view_savecontext(INDEX * FF, struct vcontext * vc);	/* saves view context */
void view_restorecontext(INDEX * FF, struct vcontext * vc, int restoreflag);	/* restores view context */
HWND view_setwindow(INDEX * FF, short visflag);	/*  sets up view window */
void view_formsilentimages(HWND hwnd);		/* formats pages silently */
//short view_writeformatted(INDEX * FF);	/* formatted output to file */
void view_changedisplaysize(HWND hwnd);		/* sets up after page/margin change */
RECN view_selectrec(INDEX * FF, RECN rnum, int posflag, short startoffset, short length);		/* selects record */
void view_resetrec(INDEX * FF, RECN rnum);		/* redisplays record that's on screen */
void view_appendrec(INDEX * FF, RECN rnum);		/* displays record on first empty screen line */
void view_redisplay(INDEX * FF, RECN rnum, short flags);	/*  redisplays view window */
void view_updatestatus(HWND wptr, BOOL marginonly); /* updates status display for selected records */
void view_clearselect(HWND hwnd); /* clears selection */
void view_setstatus(HWND wptr);	/* sets window title */
short view_close(HWND hwnd);		/* closes index */
RECN view_getsellimit(HWND wptr);	/* finds record immed beyond selection */
LRESULT CALLBACK view_proc (HWND, UINT, WPARAM, LPARAM);
