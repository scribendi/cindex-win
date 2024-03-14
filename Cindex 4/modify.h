#pragma once


#define OLDTEXT ((char *)1)
#define NOTEXT NULL
#define NEWREC ULONG_MAX
#define OLDREC 0

#define LEFTCHAR -2
#define RIGHTCHAR -1

enum {		/* text recovery flags */
	MREC_SELECT = 1,
	MREC_NOTRIM = 2,
	MREC_SINGLE = 4,
	MREC_NOITD = 8
};
enum {
	MREC_FORCEDISCARD = -1,
	MREC_ALWAYSACCEPT = 0,
	MREC_ALLOWCHECK = 1
};

HWND mod_setwindow(INDEX * FF, int totop);	/*  sets up modify window */
LRESULT CALLBACK mod_proc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL mod_canenterrecord(HWND hwnd, BOOL flag);	// returns TRUE if window can close
BOOL mod_close(HWND wptr, int checkflag);		/* closes text window */
void mod_settext(INDEX *FF, char * text, RECN rnum, HWND behind);	/* loads new text for editing */
int mod_getselection(HWND hwnd, char * sp, FONTMAP * fmp);	/* retrieves selected text as cin xstring */
int mod_gettestring(HWND ew, char * string, FONTMAP * fmp, int flags);	/* recovers coded cindex string */
short mod_dostyle(HWND ew, CHARFORMAT2 *df, int style);	/* sets style on selected text */
TCHAR mod_getcharatpos(HWND ew, int charpos);	/* gets character at position */
void mod_selectfield(HWND hwnd, int field);	// selects specified field in record
