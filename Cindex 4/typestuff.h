#pragma once

#include "attributedstrings.h"

#define VOLATILEFONTS 1	// V3 base id of changeable fonts
#define OLDVOLATILEFONTS 2	/* base id of changeable fonts in earlier versions */

enum {		/* case conversion codes */
	FC_NONE,
	FC_INITIAL,
	FC_UPPER,
	FC_AUTO,
	FC_TITLE
};

typedef struct {
	int fcount;
	ENUMLOGFONTEX *lf;
} NLFLIST;

typedef struct {
	char * name;
	unichar *ucode;
} SPECIALFONT;

typedef struct {
	int height;
	int emwidth;
} FONTMETRICS;

#define type_available(A) (type_findindex(A) >= 0)

extern NLFLIST t_fontlist;
extern SPECIALFONT t_specialfonts[];	// fonts that require character conversion

void type_doattributes(HDC dc,ATTRIBUTES * as, TCHAR codes);	/* set up attributes from code byte */
void type_clearattributes(HDC dc,ATTRIBUTES * as);	/* clears outstanding attributes */
short type_findcodestate(char * start, char * end, char *attr, char * font);	/*finds active codes/fonts at end of span */
int type_findindex(char * font);		/* finds table index of LOGFONT for named font */
BOOL type_checkfonts(FONTMAP * fm);	/* checks that preferred or substitute fonts exist */
BOOL type_scanfonts(INDEX * FF, short * farray);	/* identifies fonts used in index */
void type_trimfonts(INDEX * FF);	// removes unused fonts from font list
void type_findlostfonts(INDEX * FF);	// finds/marks dead fonts from references in records
void type_tagfonts(char * text, short * farray);	  /* tags index of fonts used in xstring */
void type_adjustfonts(INDEX * FF, short * farray);	/* adjusts font ids used in index */
BOOL type_setfontids(char * text, short * farray);	  /* adjusts ids of  fonts used in xstring */
short type_ispecialfont(char *name);		//TRUE if not letter font
short type_maplocal(FONTMAP * fmp, char * fname, int base);	// finds local id for preferred font
short type_findlocal(FONTMAP * fmp, char * fname, int base);	/* finds/assigns local id for font */
short type_makelocal(FONTMAP * fmp, char * pname, char * fname, int base);	// finds/assigns local id for import font
void type_findfonts(HWND hwnd);	/* finds fonts */
void type_setfont(HDC dc, char * name,short size, short style);	/* sets appropriate font in DC */
void type_releasefonts(void);	/* releases all fonts we've accumulated */
char * type_pickcodes(char *sptr, char * eptr, struct codeseq * cs);	/* picks up loose codes at end point */
char * type_dropcodes(char *sptr, struct codeseq * cs);	/* drops codes, advances ptr */
char * type_balancecodes(char *sptr, char * eptr, struct codeseq * cs, short match);	/* finds loose codes (and balances) at end of string */
char * type_matchcodes(char *sptr, struct codeseq * cs, short free);	/* sets codes on string */
FONTMETRICS type_getfontmetrics(INDEX * FF, char *font, int size);	// returns line spacing and em width for font
int type_setfontsizecombo(HWND cb, int size);	/* sets combo box font sizes */
int type_setfontnamecombo(HWND hwnd, int item, char *name);	/* sets combo box font names */
HFONT type_makewindowfont(HWND hwnd, TCHAR * fname);	/* makes window font at current size */
void type_settextboxfont(HWND tw);	/* set edit text font */
