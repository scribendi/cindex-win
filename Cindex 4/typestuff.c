#include "stdafx.h"
#include <string.h>
#include "errors.h"
#include "typestuff.h"
#include "records.h"
#include "index.h"
#include "strings.h"
#include "print.h"
#include "util.h"
#include <math.h>

struct linkfont {	/* active font structure */
	struct linkfont * nextf;
	LOGFONT lf;
	HFONT fh;
};

static struct linkfont *t_fbase;

// http://www.alanwood.net/demos/symbol.html
// http://www.kostis.net/charsets/symbol.htm

static unichar symbolsource[256] = {		// unicode values for symbol font
0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,	// 15
0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,	// 31

32,33,8704,35,8707,37,38,8717,  40,41,8727,43,44,8722,46,47,	// 47
48,49,50,51,52,53,54,55,  56,57,58,59,60,61,62,63,	// 63

8773,913,914,935,916,917,934,915,  919,921,977,922,923,924,925,927,	// 79 [O]
928,920,929,931,932,933,962,937,  926,936,918,91,8756,93,8869,95,	// 95

8254,945,946,967,948,949,966,947,  951,953,981,954,955,956,957,959,	// 111 [o]
960,952,961,963,964,965,982,969,  958,968,950,123,124,125,8764,0,	// 127
0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,	// 143
0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,	// 159
0,978,8242,8804,8260,8734,402,9827,  9830,9829,9824,8596,8592,8593,8594,8595,	// 175
176,177,8243,8805,215,8733,8706,8729,  247,8800,8801,8776,8230,9168,9135,8629,	// 191
8501,8465,8476,8472,8855,8853,8709,8745,  8746,8835,8839,8836,8834,8838,8712,8713,	// 207
8736,8711,174,169,8482,8719,8730,8901,  172,8743,8744,8660,8656,8657,8658,8659,	// 223
9674,9001,174,169,8482,8721,9115,9116,  9117,9121,9122,9123,9127,9128,9129,9130,	// 239
8364,9002,8747,8992,9134,8993,9118,9119,  9120,9124,9125,9126,9131,9132,9133,0		// 255
};

#if 0
static unichar wingdings[256] = {		// unicode values for wingdings font
0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,	// 15
0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,	// 31
0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,	// 47
0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,	// 63
0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,	// 79
0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,	// 95
0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,	// 111
0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,	// 127
0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,	// 143
0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,	// 159
0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,	// 175
0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,	// 191
0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,	// 207
0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,	// 223
0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,	// 239
0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0	// 255
};
#endif
static unichar wingdings[256] = {		// unicode values for wingdings font
0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,	// 15
0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,	// 31
32,9999,9986,9985,0,0,0,0,  9742,9990,9993,0,0,0,0,0,	// 47
0,0,0,0,0,0,8987,9000,  0,0,0,0,0,0,9991,9997,	// 63
0,9996,0,0,0,9756,9758,9757,  9759,0,9786,0,9785,0,9760,9872,	// 79
0,9992,9788,0,10052,0,10014,0,  10016,10017,9770,9775,2384,9784,9800,9801,	// 95
9802,9803,9804,9805,9806,9807,9808,9809,  9810,9811,38,38,9679,10061,9632,9633,	// 111
0,10065,10066,11047,10731,9670,10070,11045,  8999,9043,8984,10048,10047,10077,10078,9647,	// 127
9450,9312,9313,9314,9315,9316,9317,9318,  9319,9320,9321,9471,10102,10103,10104,10105,	// 143
10106,10107,10108,10109,10110,10111,0,0,  0,0,0,0,0,0,183,8226,	// 159
9642,9675,11093,0,9673,9678,0,9642,  9723,0,10022,9733,10038,10036,10041,10037,	// 175
0,8982,10209,8977,0,10026,10032,0,  0,0,0,0,0,0,0,0,	// 191
0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,	// 207
0,0,0,0,0,9003,8998,0,  10146,0,0,0,10162,0,0,0,	// 223
0,0,0,0,0,0,0,0,  10132,0,0,0,0,0,0,8678,	// 239
8680,8679,8681,11012,8691,11008,11009,11011,  11010,9645,9643,10007,10003,9746,9745,0	// 255
};

static unichar dingbats[256] = {		// unicode values for dingbats font
0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,	// 15
0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,	// 31
32,0x2701,0x2702,0x2703,0x2704,0x260e,0x2706,0x2707,  0x2708,0x2709, 0x261b, 0x261e, 0x270c,0x270d,0x270e,0x270f,	// 47
0x2710,0x2711,0x2712,0x2713,0x2714,0x2715,0x2716,0x2717,  0x2718,0x2719,0x271a,0x271b,0x271c,0x271d,0x271e,0x271f,	// 63
0x2720,0x2721,0x2722,0x2723,0x2724,0x2725,0x2726,0x2727,  0x2605,0x2729,0x272a,0x272b,0x272c,0x272d,0x272e,0x272f,	// 79
0x2730,0x2731,0x2732,0x2733,0x2734,0x2735,0x2736,0x2737,  0x2738,0x2739,0x273a,0x273b,0x273c,0x273d,0x273e,0x273f,	// 95
0x2740,0x2741,0x2742,0x2743,0x2744,0x2745,0x2746,0x2747,  0x2748,0x2749,0x274a,0x274b,0x25cf,0x274d,0x25a0,0x274f,	// 111
0x2750,0x2751,0x2752,0x25b2,0x25bc,0x25c6,0x2756,0x25d7,  0x2758,0x2759,0x275a,0x275b,0x275c,0x275d,0x275e,0,	// 127
0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,	// 143
0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,	// 159
0,0x2761,0x2762,0x2763,0x2764,0x2765,0x2766,0x2767,  0x2663,0x2666,0x2665,0x2660,0x2460,0x2461,0x2462,0x2463,	// 175
0x2464,0x2465,0x2466,0x2467,0x2468,0x2469,0x2776,0x2777,  0x2778,0x2779, 0x277a,0x277b,0x277c,0x277d,0x277e,0x277f,	// 191
0x2780,0x2781,0x2782,0x2783,0x2784,0x2785,0x2786,0x2787,  0x2788,0x2789,0x278a,0x278b,0x278c,0x278d,0x278e,0x278f,	// 207
0x2790,0x2791,0x2792,0x2793,0x2794,0x2192,0x2194,0x2195,  0x2798,0x2799,0x279a,0x279b,0x279c,0x279d,0x279e,0x279f,	// 223
0x27a0,0x27a1,0x27a2,0x27a3,0x27a4,0x27a5,0x27a6,0x27a7,  0x27a8,0x27a9,0x27aa,0x27ab,0x27ac,0x27ad,0x27ae,0x27af,	// 239
0,0x27b1,0x27b2,0x27b3,0x27b4,0x27b5,0x27b6,0x27b7,  0x27b8,0x27b9,0x27ba,0x27bb,0x27bc,0x27bd,0x27be,0	// 255
};

static TCHAR *fontstrings[] = {TEXT(" 8"),TEXT(" 9"),TEXT("10"),TEXT("12"),TEXT("14"),TEXT("18"),TEXT("24")};
#define FSTRINGCOUNT (sizeof(fontstrings)/sizeof(char *))

NLFLIST t_fontlist;

SPECIALFONT t_specialfonts[] = {	// fonts that require character conversion
	{"Symbol",symbolsource},
	{"Zapf Dingbats", dingbats},
	{"Wingdings",wingdings},
	{NULL,NULL}
};

static int CALLBACK enumfont(ENUMLOGFONTEX * lf, NEWTEXTMETRICEX * tm, int fonttype, LPARAM lParam);	/* finds fonts */
static int fcompare( const void *arg1, const void *arg2 );
static HFONT getfont(LOGFONT * lfptr);	/* finds (or creates) font matching spec */
/**********************************************************************/
void type_doattributes(HDC dc,ATTRIBUTES * as, TCHAR codes)	/* set up attributes from code byte */

{
	if (codes&FX_AUTOFONT)	{	/* if a font */
		codes &= ~FX_AUTOFONT;
		if (codes&FX_COLOR)	{
			codes &= FX_COLORMASK;
			if (!codes)	/* if restoring */
				SetTextColor(dc,GetSysColor(COLOR_WINDOWTEXT));
			else
				SetTextColor(dc,g_prefs.gen.flagcolor[codes]);
		}
		else {
			short fcode;
			int currfont, newbase;

			currfont = currentfont(as);
			newbase = codes&FX_AUTOFONT;		/* flags change of base font */
			codes &= FX_FONTMASK;
			if (!codes)	{				/* if want to go back to current base */
				while (!atbasefont(as))	/* while not at base font */
					getprevfont(as);
				fcode = currentfont(as);
			}
			else if (codes == secondfont(as))	/* if reverting to last font */
				fcode = getprevfont(as);		/* pop current; get previous */
			else
				fcode = pushfont(as,codes);
			if (newbase)
				setbasefont(as);		/* set new base font */
			if (currfont != fcode)		/* if changed font */
				type_setfont(dc,as->fmp[fcode].name,as->nsize,as->attr);
		}
	}
	else {				/* a style code */
		POINT pos;

		if (codes&FX_OFF)	{	/* if turning off */
			if (codes&FX_BOLD && as->attcount[FX_BOLDX] && !--as->attcount[FX_BOLDX])
				as->attr &= ~FX_BOLD;
			if (codes&FX_ITAL && as->attcount[FX_ITALX] && !--as->attcount[FX_ITALX])
				as->attr &= ~FX_ITAL;
			if (codes&FX_ULINE && as->attcount[FX_ULINEX] && !--as->attcount[FX_ULINEX])
				as->attr &= ~FX_ULINE;
			if (codes&FX_SMALL && as->attcount[FX_SMALLX] && !--as->attcount[FX_SMALLX])	{
				as->attr &= ~FX_SMALL;
				as->cursize = as->nsize;	/* restore normal size */
			}
			if (codes&(FX_SUPER|FX_SUB))	{	/* if sub or superscript */
				GetCurrentPositionEx(dc,&pos);
				MoveToEx(dc,pos.x,pos.y-(int)as->soffset,NULL);		/* move back to base posn (undo offset) */
				as->soffset = 0;
				as->cursize = as->nsize;	/* restore normal size */
			}
		}
		else {					/* turning on */
			if (codes&FX_BOLD && !as->attcount[FX_BOLDX]++)
				as->attr |= FX_BOLD;
			if (codes&FX_ITAL && !as->attcount[FX_ITALX]++)
				as->attr |= FX_ITAL;
			if (codes&FX_ULINE && !as->attcount[FX_ULINEX]++)
				as->attr |= FX_ULINE;
			if (codes&FX_SMALL && !as->attcount[FX_SMALLX]++)	{
				as->attr |= FX_SMALL;
				as->cursize = as->nsize*SMALLCAPSCALE;	/* set small caps */
			}
			if (codes&FX_SUPER && as->soffset <= 0)	{	/* if not already in super position */
				as->soffset = -as->nsize*SUPERRISE;
				GetCurrentPositionEx(dc,&pos);
				MoveToEx(dc,pos.x,pos.y+(int)as->soffset,NULL);	/* move to super posn */
				as->cursize = as->nsize*SUPSUBCALE;	/* set small caps */
			}
			if (codes&FX_SUB && as->soffset >= 0)	{	/* if not already in sub position */
				as->soffset = as->nsize*SUBDROP;	// down 0.2 height
				GetCurrentPositionEx(dc,&pos);
				MoveToEx(dc,pos.x,pos.y+(int)as->soffset,NULL);	/* move to subscript posn */
				as->cursize = as->nsize*SUPSUBCALE;	/* set small caps */
			}
		}
		type_setfont(dc,NULL,as->cursize,as->attr);
	}
}
/**********************************************************************/
void type_clearattributes(HDC dc,ATTRIBUTES * as)	/* clears outstanding attributes */
	/* called at end of field */

{
	if (as->findex)		{	/* if outstanding fonts */
		as->findex = 0;		/* clear count */
		memset(as->fstack,0,sizeof(as->fstack));	/* clear stack */
		type_setfont(dc,as->fmp[0].name,as->nsize,as->attr);
	}
	/* NB: should we also have code to clear style/face attribs ? */
}
/**********************************************************************/
short type_findcodestate(char * start, char * end, char *attr, char * font)	/*finds active codes/fonts at end of span */

{
	short ret;
	
	ret = *attr = *font = '\0';
	while (start < end)	{
		char c = *start++;
		if (c == FONTCHR && *start)	{
			*font = *start & ~FX_FONT;
			start++;
		}
		if (c == CODECHR && *start)	{
			if (*start&FX_OFF)
				*attr &= ~*start;	/* clear code */
			else
				*attr |= *start;
			start++;
		}
	}
	if (*attr)
		ret++;
	if (*font)
		ret++;
	return (ret);	/* return number of code bytes needed */
}
/**********************************************************************/
int type_findindex(char * font)		/* finds table index of LOGFONT for named font */

{
	int fcount;
	
	for (fcount = 0; fcount < t_fontlist.fcount; fcount++)	{
		if (!nstrcmp(t_fontlist.lf[fcount].elfLogFont.lfFaceName, toNative(font)))
			return (fcount);
	}
	return (-1);
}
/**********************************************************************/
BOOL type_checkfonts(FONTMAP * fm)	/* checks that preferred or substitute fonts exist */

{
	int index;
	char estring[LF_FACESIZE*FONTLIMIT], * eptr;

	for (index = 0, eptr = estring; *fm[index].pname && index < FONTLIMIT; index++)	{	/* for all preferred fonts */
		if (type_available(fm[index].pname))		/* if have preferred font */
			strcpy(fm[index].name,fm[index].pname);		/* make substitute match real thing */
		else if (!type_available(fm[index].name))	{	/* if can't find sub font */
			eptr += sprintf(eptr,"\"%s\"\n",fm[index].pname);	/* save name of preferred */
			*fm[index].name = '\0';						/* no substitute font */
		}
	}
	*eptr = '\0';
	if (*estring)	{
		senderr(ERR_FONTMISSING,WARN,estring);
		return (FALSE);
	}
	return (TRUE);
}
/**********************************************************************/
BOOL type_scanfonts(INDEX * FF, short * farray)	/* identifies fonts used in index */

{
	RECORD * recptr;
	RECN rnum;
	int index, count;

	memset(farray,0,FONTLIMIT * sizeof(short));
	for (rnum = 1; recptr = rec_getrec(FF,rnum); rnum++)	/* for all records */
		type_tagfonts(recptr->rtext,farray);		/* marks fonts used */
	if (index = type_findlocal(FF->head.fm,FF->head.formpars.pf.lefthead.hffont,VOLATILEFONTS))
		farray[index] = TRUE;
	if (index = type_findlocal(FF->head.fm,FF->head.formpars.pf.righthead.hffont,VOLATILEFONTS))
		farray[index] = TRUE;
	if (index = type_findlocal(FF->head.fm,FF->head.formpars.pf.leftfoot.hffont,VOLATILEFONTS))
		farray[index] = TRUE;
	if (index = type_findlocal(FF->head.fm,FF->head.formpars.pf.rightfoot.hffont,VOLATILEFONTS))
		farray[index] = TRUE;
	if (index = type_findlocal(FF->head.fm,FF->head.formpars.ef.eg.gfont,VOLATILEFONTS))
		farray[index] = TRUE;
	for (count = 0; count < FF->head.indexpars.maxfields-1; count++)	{
		if (index = type_findlocal(FF->head.fm,FF->head.formpars.ef.field[count].font,VOLATILEFONTS))
			farray[index] = TRUE;
	}
	for (index = VOLATILEFONTS; *FF->head.fm[index].name && index < FONTLIMIT; index++)	/* scan  */
		if (!farray[index])		/* if unused */
			return (FALSE);
	return (TRUE);
}
/******************************************************************************/
void type_trimfonts(INDEX * FF)	// removes unused fonts from font list

{
	short farray[FONTLIMIT];
	if (!type_scanfonts(FF, farray))	// if not all fonts used
		type_adjustfonts(FF, farray);

}
/**********************************************************************/
void type_findlostfonts(INDEX * FF)	// finds/marks dead fonts from references in records

{
	RECORD * recptr;
	RECN rnum;
	short farray[FONTLIMIT];
	int index;
	
	memset(farray,0,FONTLIMIT * sizeof(short));
	for (rnum = 1; (recptr = rec_getrec(FF,rnum)); rnum++)	/* for all records */
		type_tagfonts(recptr->rtext,farray);		/* marks fonts used */
	for (index = VOLATILEFONTS; index < FONTLIMIT; index++)	{	/* scan  */
		if (farray[index] && !*FF->head.fm[index].name)	// if no map entry for font
			strcpy(FF->head.fm[index].pname,"_Unknown Font");	// force pickup as missing font
	}
}
/******************************************************************************/
void type_tagfonts(char * text, short * farray)	  /* tags index of fonts used in xstring */

{
	char * cptr, *xptr;

	for (cptr = text; *cptr != EOCS; cptr += strlen(cptr++)) {
		xptr = cptr;
		while ((xptr = strchr(xptr, FONTCHR))) {	// while codes to check
			if (*++xptr&FX_FONT)		/* if font */
				farray[*xptr&FX_FONTMASK] = TRUE;		/* tag its entry */
		}
	}
}
/**********************************************************************/
void type_adjustfonts(INDEX * FF, short * farray)	/* adjusts font ids used in index */

{
	RECORD * recptr;
	RECN rnum;
	int nindex, nfindex,index;
	short nfarray[FONTLIMIT];

	memset(nfarray,0,FONTLIMIT*sizeof(short));
	for (nfindex = nindex = VOLATILEFONTS; nfindex < FONTLIMIT; nfindex++)	{	/* for every font index */
		if (farray[nfindex])				/* if current local id is used */
			nfarray[nfindex] = nindex++;	/* set new local id in its slot */
	}
	for (index = VOLATILEFONTS;index < FONTLIMIT; index++)	{	/* for all fonts */
		for (nfindex = VOLATILEFONTS; nfindex < FONTLIMIT; nfindex++)	{		/* for all current map entries */
			if (nfarray[nfindex] == index)		/* if found entry to replace current one */
				break;
		}
		if (nfindex < FONTLIMIT)	/* if found a match */
			FF->head.fm[index] = FF->head.fm[nfindex];	/* set new entry */
		else
//			*FF->head.fm[nindex].name = *FF->head.fm[nindex].pname = '\0';	/* clear entry */		5/26/20
			*FF->head.fm[index].name = *FF->head.fm[index].pname = '\0';	/* clear entry */

	}
	for (rnum = 1; recptr = rec_getrec(FF,rnum); rnum++)	{	/* for all records */
		type_setfontids(recptr->rtext,nfarray);		/* sets new font ids */
	}
	index_flush(FF);
}
/******************************************************************************/
BOOL type_setfontids(char * text, short * farray)	  /* adjusts ids of fonts used in xstring */

{
	char *cptr;
	BOOL missing = FALSE;

	for (cptr = text; *cptr != EOCS; cptr += strlen(cptr++)) {
		char * xptr = cptr;
		while ((xptr = strchr(xptr, FONTCHR)))	{	/* if code */
			if (*++xptr&FX_FONT)	{
				char fid = *xptr&FX_FONTMASK;
				if (fid >= VOLATILEFONTS)	{	// if discretionary font
					if (!farray[fid])	// if didn't have name for this font
						missing = TRUE;
					*xptr = (char)farray[fid]|FX_FONT;	// set its new id
				}
			}
		}
	}
	return missing;
}
/**********************************************************************/
short type_ispecialfont(char *name)		//TRUE if charset font

{
	int index;

	for (index = 0; t_specialfonts[index].name; index++)	{	/* for all special fonts */
		if (!strcmp(t_specialfonts[index].name,name))	// if this is one
			return TRUE;
	}
	return FALSE;
}
/**********************************************************************/
short type_maplocal(FONTMAP * fmp, char * fname, int base)	// finds local id for preferred font

{	
	if (!*fname)		/* if want default font */
		return (0);
	for (; base < FONTLIMIT && *fmp[base].pname; base++)		{	/* for all local fonts */
		if (!strcmp(fmp[base].pname,fname))	/* if have a match */
			return (base);
	}
	senderr (ERR_INTERNALERR, WARN, "missing font");
	return (0);			/* always return default font on error */
}
#if 0
/**********************************************************************/
short type_findlocal(FONTMAP * fmp, char * fname, int base)	/* finds/assigns local id for font */

{	
	if (!*fname)		/* if want default font */
		return (0);
	for (; base < FONTLIMIT; base++)		{	/* for all local fonts */
		if (!*fmp[base].name)	{			/* an empty slot */
			strcpy(fmp[base].pname,fname);	/* load preferred name */
			strcpy(fmp[base].name,fname);	/* set sub/actual name */
			fmp[base].flags = type_ispecialfont(fname) ? CHARSETSYMBOLS : 0;
		}
		if (!strcmp(fmp[base].name,fname))	/* if have a match */
			return (base);
	}
	senderr (ERR_INTERNALERR, WARN, "missing font");
	return (0);			/* always return default font on error */
}
/**********************************************************************/
short type_findlocal(FONTMAP * fmp, char * fname, int base)	/* finds/assigns local id for font */

{	
	if (*fname)		{	// if want other than default font
		for (; base < FONTLIMIT; base++)		{	/* for all local fonts */
			if (!strcmp(fmp[base].pname,fname))	// if have a match to preferred name
				return (base);
			if (!*fmp[base].name)	{			/* an empty slot */
				strcpy(fmp[base].pname,fname);	/* load preferred name */
				if (type_findindex(fname) >= 0)		// if this is an available font
					strcpy(fmp[base].name,fname);	// set preferred name as alt name
				else
					strcpy(fmp[base].name,fmp[0].name);	// set default font alt as alt name
				fmp[base].flags = type_ispecialfont(fname) ? CHARSETSYMBOLS : 0;
				return base;
			}
		}
		senderr(ERR_INTERNALERR, WARN, "missing font");	// will return default font
	}
	return (0);
}
#else
/**********************************************************************/
short type_findlocal(FONTMAP * fmp, char * fname, int base)	/* finds/assigns local id for font */

{	
	if (*fname)		{	// if want other than default font
		for (; base < FONTLIMIT; base++)		{	/* for all local fonts */
			if (!strcmp(fmp[base].pname,fname) || !strcmp(fmp[base].name,fname))	// if have a match to preferred or alt name name
				return (base);
			if (!*fmp[base].name)	{			/* an empty slot */
				strcpy(fmp[base].pname,fname);	/* load preferred name */
				if (type_available(fname))		// if this is an available font
					strcpy(fmp[base].name,fname);	// set preferred name as alt name
				else
					strcpy(fmp[base].name,fmp[0].name);	// set default font alt as alt name
				fmp[base].flags = type_ispecialfont(fname) ? CHARSETSYMBOLS : 0;
				return base;
			}
		}
		senderr(ERR_INTERNALERR, WARN, "missing font");	// will return default font
	}
	return (0);
}
#endif
/**********************************************************************/
short type_makelocal(FONTMAP * fmp, char * pname, char * fname, int base)	// finds/assigns local id for import font

{	
	for (; base < FONTLIMIT; base++)		{	/* for all local fonts */
		if (!strcmp(fmp[base].name,pname))	/* if have a match */
			return (base);
		if (!*fmp[base].name)	{			/* an empty slot */
			strcpy(fmp[base].pname,pname);	/* load preferred name */
			if (type_available(pname))	/* if we actually have the font */
				strcpy(fmp[base].name,pname);	/* set actual name */
			else
				strcpy(fmp[base].name,fname);	/* set substitute name */
			return (base);
		}
	}
	senderr (ERR_INTERNALERR, WARN, "missing font");
	return (-1);
}
/******************************************************************************/
void type_findfonts(HWND hwnd)	/* finds fonts */

{
	LOGFONT lf;
	HDC dc;

	if (dc = GetDC(hwnd))	{	
		memset(&lf,0,sizeof(lf));
		lf.lfCharSet = ANSI_CHARSET; /* (defined as 0) */
		EnumFontFamiliesEx(dc,&lf,(FONTENUMPROC)enumfont,(LPARAM)&t_fontlist,0);	/* counting */
		lf.lfCharSet = SYMBOL_CHARSET;
		EnumFontFamiliesEx(dc,&lf,(FONTENUMPROC)enumfont,(LPARAM)&t_fontlist,0);	/* counting */
		if (t_fontlist.lf = getmem(t_fontlist.fcount*sizeof(ENUMLOGFONTEX)))	{	/* if can get memory */
			t_fontlist.fcount = 0;
			lf.lfCharSet = ANSI_CHARSET; /* (defined as 0) */
			EnumFontFamiliesEx(dc,&lf,(FONTENUMPROC)enumfont,(LPARAM)&t_fontlist,0);
			lf.lfCharSet = SYMBOL_CHARSET;
			EnumFontFamiliesEx(dc,&lf,(FONTENUMPROC)enumfont,(LPARAM)&t_fontlist,0);
			qsort(t_fontlist.lf,t_fontlist.fcount,sizeof(ENUMLOGFONTEX),fcompare);
		}
		ReleaseDC(hwnd,dc);
	}
	else
		senderr(ERR_INTERNALERR, WARN, "can't find fonts");
}
/******************************************************************************/
static int CALLBACK enumfont(ENUMLOGFONTEX * elf, NEWTEXTMETRICEX * tm, int fonttype, LPARAM lParam)	/* finds fonts */

{
	NLFLIST * lfp = (NLFLIST *)lParam;

	if (fonttype&(TRUETYPE_FONTTYPE|DEVICE_FONTTYPE) && *elf->elfLogFont.lfFaceName != L'@')	{	/* if it's a printer font && truetype  && not vertical */
		if (lfp->lf)	/* if we've got an array */
			lfp->lf[lfp->fcount++] = *elf;	/* load info */
		else
			lfp->fcount++;
	}
	return (TRUE);		/* FALSE stops enumeration */
}
/******************************************************************************/
static int fcompare( const void *arg1, const void *arg2 )

{
	return nstricmp(((ENUMLOGFONTEX*)arg1)->elfLogFont.lfFaceName,((ENUMLOGFONTEX*)arg2)->elfLogFont.lfFaceName);
}
/******************************************************************************/
void type_setfont(HDC dc, char * name, short size, short style)	/* sets appropriate font in DC */

{
	LOGFONT lf;
	HFONT fh;
	TCHAR fname[LF_FACESIZE];

	memset(&lf,0,sizeof(lf));
//	if (GetMapMode(dc) == MM_TEXT || GetMapMode(dc) == MM_ANISOTROPIC)	/* if in text mapping mode (if rtl context then always return anisotropic)*/
	if (dc != p_dc)	// assume text mapping if not printer dc
		lf.lfHeight = -MulDiv(size, GetDeviceCaps(dc, LOGPIXELSY), 72);	/* scale font to right size */
	else			/* some logical mode */
		lf.lfHeight = -size;
	lf.lfWeight = (style&FX_BOLD) ? FW_BOLD : FW_NORMAL;
	lf.lfItalic = (style&FX_ITAL) ? TRUE : FALSE;
	lf.lfUnderline = (style&FX_ULINE) ? TRUE : FALSE;
	if (name)
		nstrcpy(lf.lfFaceName,toNative(name));
	else {
		GetTextFace(dc,LF_FACESIZE,fname);
		nstrcpy(lf.lfFaceName,fname);
	}
	if (fh = getfont(&lf))	/* if can get font */
		SelectFont(dc,fh);
	else
		senderr(ERR_INTERNALERR,WARN,"Can't Load Font");
}
/******************************************************************************/
static HFONT getfont(LOGFONT * lfptr)	/* finds (or creates) font matching spec */

{
	struct linkfont * tfp, **lfp;

	for (tfp = t_fbase, lfp = &t_fbase; tfp; tfp = tfp->nextf)	{	/* for all fonts in list */
		if (lfptr->lfWeight == tfp->lf.lfWeight
			&& lfptr->lfItalic == tfp->lf.lfItalic
			&& lfptr->lfUnderline == tfp->lf.lfUnderline
			&& lfptr->lfHeight == tfp->lf.lfHeight
			&& !nstrcmp(lfptr->lfFaceName,tfp->lf.lfFaceName))
			return (tfp->fh);
		lfp = &tfp->nextf;		/* hold last ptr */
	}
	if (tfp = getmem(sizeof(struct linkfont)))	{	/* if can get mem for new struct */
		*lfp = tfp;				/* install link */
		tfp->lf = *lfptr;		/* copy font spec */
		tfp->lf.lfOutPrecision = OUT_TT_ONLY_PRECIS;	/* extras */
		tfp->lf.lfPitchAndFamily = DEFAULT_PITCH;
		tfp->lf.lfCharSet = DEFAULT_CHARSET;
		tfp->fh = CreateFontIndirect(&tfp->lf);	/* create font */
		return (tfp->fh);
	}
	return (NULL);
}
/******************************************************************************/
void type_releasefonts(void)	/* releases all fonts we've accumulated */

{
	struct linkfont * tfp;

	for (tfp = t_fbase; tfp; tfp = tfp->nextf)	/* for all fonts in list */
		DeleteFont(tfp->fh);
}
/******************************************************************************/
char * type_pickcodes(char *sptr, char * eptr, struct codeseq * cs)	/* picks up loose codes at end point */

{
	cs->style = cs->font = 0;
	while (sptr < eptr)	{
		char c = *sptr++;
		if (c == FONTCHR && *sptr)	{	/* if code char with something after */
			cs->font = *sptr;				/* switch font */
			sptr++;
		}
		if (c == CODECHR && *sptr)	{	/* if code char with something after */
			if (*sptr&FX_OFF)
				cs->style &= ~(*sptr&FX_STYLEMASK);	/* set style off */
			else
				cs->style |= *sptr;				/* set style on */
			sptr++;
		}
	}
	return (sptr);
}
/******************************************************************************/
char * type_dropcodes(char *sptr, struct codeseq * cs)	/* drops codes, advances ptr */

{
	if (cs->font)	{		/* if need font */
		*sptr++ = FONTCHR;	/* insert */
		*sptr++ = cs->font;
	}
	if (cs->style)	{		/* if need style */
		*sptr++ = CODECHR;	/* insert */
		*sptr++ = cs->style;
	}
	return (sptr);
}
/******************************************************************************/
char * type_balancecodes(char *sptr, char * eptr, struct codeseq * cs, short match)	/* finds loose codes (and balances) at end of string */

{
	sptr = type_pickcodes(sptr,eptr,cs);	/* gather codes */
	if (match)	{		/* if want to balance unclosed codes */
		if (cs->style)	{	/* need to append closing style */
			*sptr++ = CODECHR;
			*sptr++ = cs->style|FX_OFF;
		}
		if (cs->font)	{
			*sptr++ = FONTCHR;
			*sptr++ = FX_FONT;	/* default font */
		}
		*sptr = '\0';
	}
	return (sptr);		/* end of string */
}
/******************************************************************************/
char * type_matchcodes(char *sptr, struct codeseq * cs, short free)	/* sets codes on string */

{
	short shiftcount = 0;
	char * base = sptr;
	
	if (cs->style)
		shiftcount += 2;
	if (cs->font)
		shiftcount += 2;
	if (free >= shiftcount*2)	{	/* if guaranteed enough room for both open and close */
		if (shiftcount)
			memmove(sptr+shiftcount, sptr,strlen(sptr)+1);	/* shift string */
		sptr = type_dropcodes(sptr,cs);	/* insert continuing codes */
		return (type_balancecodes(base,sptr+strlen(sptr),cs,TRUE));	/* balance by inserting any closing codes */
	}
	else
		return (sptr+strlen(sptr));
}
/******************************************************************************/
FONTMETRICS type_getfontmetrics(INDEX * FF, char *font, int size)	// returns line spacing and em width for font

{
	HDC dc = GetDC(FF->vwind);
	int tdc = SaveDC(dc);
	TEXTMETRIC tmetric;
	FONTMETRICS fi;

	type_setfont(dc,FF->head.fm[0].name,size,0);	
	GetTextMetrics(dc,&tmetric);
	fi.height = tmetric.tmHeight+tmetric.tmExternalLeading;
	GetCharWidth32(dc,EMSPACE,EMSPACE,&fi.emwidth);	// get width of em space
	RestoreDC(dc,tdc);
//	ReleaseDC(FF->vwind,dc);		// ?? need this
	return fi;
}
/*******************************************************************************/
int type_setfontsizecombo(HWND cbh, int size)	/* sets combo box font sizes */

{
	int count;
	char cs[32];

	ComboBox_LimitText(cbh,3);
	for (count = 0; count < FSTRINGCOUNT; count++)
		ComboBox_AddString(cbh,fontstrings[count]);
	_itoa(size,cs,10);
	SetWindowText(cbh,toNative(cs));	/* set size in view */
	return (count);
}
/*******************************************************************************/
int type_setfontnamecombo(HWND hwnd, int item, char *name)	/* sets combo box font names */

{
	int count;
	HWND cbh;

	cbh = GetDlgItem(hwnd,item);
	for (count = 0; count < t_fontlist.fcount; count++)
		ComboBox_AddString(cbh,t_fontlist.lf[count].elfLogFont.lfFaceName);
	if (!name)	/* if no font flagged  */	{
		ComboBox_InsertString(cbh,0,TEXT("<Default>"));	/* add default item */
		return (ComboBox_SetCurSel(cbh,0));
	}
	else
		return ComboBox_SetCurSel(cbh,ComboBox_FindStringExact(cbh,-1,toNative(name)));	/* select named font */
//		return ComboBox_SetCurSel(cbh,SendMessage(cbh,CB_FINDSTRINGEXACT,-1,(LPARAM)toNative(name)));	// select named font (exact match)
}
/**********************************************************************************/
HFONT type_makewindowfont(HWND hwnd, TCHAR * fname)	/* makes window font at current size */

{
	LOGFONT lf;
	HDC dc;
	HFONT tfh;
	TEXTMETRIC tm;

	if (dc = GetDC(hwnd))	{
		GetTextMetrics(dc,&tm);
		memset(&lf,0,sizeof(lf));
		lf.lfHeight = tm.tmHeight;	/* get right size */
		lf.lfOutPrecision = OUT_TT_ONLY_PRECIS;
		lf.lfPitchAndFamily = DEFAULT_PITCH;
		lf.lfCharSet = DEFAULT_CHARSET;
		nstrcpy(lf.lfFaceName,fname);
		tfh = CreateFontIndirect(&lf);
		ReleaseDC(hwnd,dc);
		return (tfh);
	}
	return (NULL);
}
/******************************************************************************/
void type_settextboxfont(HWND tw)	/* makes edit text font with full unicode charset*/

{
#if 0
	static HFONT boxFont;
	if (!boxFont)	// this assumes all edit control boxes are same size
		boxFont = type_makewindowfont(tw,g_basefont);
	SendMessage(tw,WM_SETFONT,(WPARAM)boxFont,MAKELPARAM(FALSE,0));	/* set font */
#endif
}
