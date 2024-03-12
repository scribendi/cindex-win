#pragma once
#include "reole.h"
#include <TOM.h>

#define HMARGIN 36
#define VMARGIN 48

enum {		/* control ids in text window */
	RE_ID = 100,		/* rich edit control */
	HD_ID = 101
};

typedef struct {	// header item info
	TCHAR * title;	// title
	int width;		// width
	int format;		// button format flags
} HEADERITEM;

typedef struct {		/* setup parameters for text window */
	LRESULT (CALLBACK *hook)(HWND, UINT, WPARAM, LPARAM);
	HANDLE * hwp;		/* address of ultimate window handle */
	int style;
	INDEX * owner;
	TCHAR * helpcontext;	// help context string
	HEADERITEM *hitems;
	int headerstyle;
}TEXTPARS;

typedef struct {	/* contains data/functions for window */
	WFLIST wstuff;		/* basic set of window functions */
	HWND hwed;			/* rich edit handle */
	RECALLBACK reole;	/* ole callback stuff */
	IRichEditOle* ole;		// ole
	ITextDocument * itd;	// iTextDocument
	CHARFORMAT2 df;		/* system default character format */
	PARAFORMAT2 pf;		/* our paragraph format */
	LRESULT (CALLBACK *tproc)(HWND, UINT, WPARAM, LPARAM );	/* saved window procedure */
	HANDLE * hwp;		/* address of ultimate window handle */
	HWND hwh;			// header control
	CHARFORMAT2 cf;		/* our default character format */
	PAPERINFO pi;		/* paper info */
	MARGINCOLUMN mc;	/* margin info */
	RECT prect;			/* formatting rectangle for page */
	HANDLE handle1;		/* general-purpose handle */
	int flag1;			/* general purpose int */
	TCHAR * hcontext;	// help context
}EFLIST;

#define EX(WW,II) (((EFLIST *)GetWindowLongPtr(WW,GWLP_USERDATA))->II)	/* for addressing items */

HWND txt_setwindow(TCHAR * title, RECT * drptr, TEXTPARS * tp);	/*  sets up text edit window */
void txt_append(HWND hwnd, TCHAR * string, CHARFORMAT2 *cfp);	/*  adds text to specified rich edit control */
void txt_appendctext(HWND hwnd, TCHAR * rtext);	/*  replaces selection with cindex styled text */
void txt_clearall(HWND hwnd);		/* clears text */
int txt_notify(HWND hwnd, int id, NMHDR * hdr);	/* does notification tasks */
void txt_findpara(HWND ew, int curpos, int *selstart, int *selend, int *line,BOOL mflag);	/* finds para limits */
int txt_selectpara(HWND ew, int shift);	/* selects -1,0,1 para, returns first line */
void txt_setparaformat(HWND hwnd, PARAFORMAT2 * pf);	/* sets paragraph format */
LRESULT CALLBACK txt_proc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
