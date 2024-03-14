#include "stdafx.h"
#include "errors.h"
#include "commands.h"
#include "files.h"
#include "records.h"
#include "index.h"
#include "strings.h"
#include "sort.h"
#include "viewset.h"
#include "indexset.h"
#include "formstuff.h"
#include "import.h"
#include "translate.h"
#include "typestuff.h"
#include "util.h"
#include "v2headers.h"

#define OLDFSPACE  ((unsigned char)160)	/* Windows fixed space */
#define OLDENDASH ((unsigned char)0x96)	/* Windows en dash */

static const DWORD wh_ctransid[] = {
	IDC_CODETRANSG, HIDC_CODETRANSG,
	IDC_CODETRANSH, HIDC_CODETRANSH,
	0,0
};

struct sizestruct	{	/* for record size dialog box */
	int maxlen;
	int cursize;
};

static unsigned char styleids[] = {
	0,
	FX_ULINE,
	FX_ITAL,
	FX_BOLD,
	FX_SUPER,
	FX_SUB,
	FX_SMALL,
	0,			/* g and h codes in any format settings are discarded */
	0
};

static char esclist[] = "libudaghLIBUDAGH";	/* for indexing codes in DOS header format settings */

char doslow_to_win[] =	/* DOS low characters that can be mapped */
"\0\206\206\251\250\247\252\267\206\206\206\206\206\206\206\206\
\206\206\206\206\266\247\206\206\206\206\206\206\206\206\206\206";

char dos_to_win[] =
"ÇüéâäàåçêëèïîìÄÅ\
ÉæÆôöòûùÿÖÜ¢£¥€ƒ\
áíóúñÑªº¿€¬½¼¡«»\
€€€€€€€€€€€€€€€€\
€€€€–€€€€€€€€€€€\
€€€€€€€€€€€€€€€€\
abGpSsµtFQWd€Ø€€\
€±€€€€÷€°•·€€²€€";

char dos_greek[] = "abdFGmpQSstW";	/* characters that in symbol font match DOS greek */
char dos_fromsymbol[] = {224,225,235,232,226,230,227,233,228,229,231,234};	/* DOS extend chars that match chars from the symbol font */

char tr_escname[] = "bBiIlLaAuUdDgGhH";	/* key chars for DOS escape sequences */
unsigned char tr_attrib[] = {FX_BOLD,FX_ITAL,FX_ULINE,FX_SMALL,FX_SUPER,FX_SUB,FX_FONT,FX_FONT};		/* text attribute flags */

static short crosscheck(char *str);    /* checks string to see if begins with 'see' */
static void convertDOSheader(V2HEAD *v2p, V61_HEAD * ohp, int maxfields);	// converts DOS header
static BOOL scanrecords(V61_HEAD * ohp, char * base, struct ghstruct *ghp, int * maxlen, int *maxfields);     /* scans records */
static INT_PTR CALLBACK transproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static char * fixxstring(char * string);     /* converts EOCS from DOS format */
static void dosstripcpy(char * dptr, char * sptr, short *code);     /* copies, stripping dos style codes */
static char * dosextendedtowin(char * sptr, int flags);     /* translates dos extended char */
static char * doslowtowin(char * sptr);     /* translates dos low char */

/******************************************************************************/
BOOL tr_DOStoV2(MFILE * mf)     /* converts DOS v6 index to v2 format */

{
	BOOL ok = FALSE;
	RECN markcount = 0;
	FONTMAP fm[FONTLIMIT] = {{"Arial","Arial",0},{"Symbol","Symbol",CHARSETSYMBOLS}};
	V61_HEAD ohp;
	V2HEAD v2h;
	struct ghstruct gh;
	int maxlen, maxfields;

//	int v1r = V61_RECSIZE;
//	int v2r = V2RECSIZE;
	memset(&gh,0,sizeof(gh));
	gh.fmp = fm;
	copymappeddata(&ohp,mf->base,V61_HEADSIZE);		// copy original header
	if (ohp.version < 600)	{
		int major = ohp.version/100;
		int minor = ohp.version%100;
		senderr(ERR_FILEVERSERR, WARN, major,minor,major,minor);	/* early version */
	}
	else if (scanrecords(&ohp,mf->base+V61_HEADSIZE,&gh,&maxlen,&maxfields))	{	/* if scan is ok */
		int index;
		if (!(gh.flags&READ_HASGH) || tr_ghok(&gh))		{	/* if no gh codes or want translated */
			int newrecsize, newdocsize;

			memset(&v2h,0,V2HEADSIZE);		// set up v2 header
			convertDOSheader(&v2h,&ohp, maxfields);	// convert header
			for (index = 0; index < FONTLIMIT; index++)	{	// need to copy this way because font maps different sizes
				strcpy(v2h.fm[index].pname, fm[index].pname);
				strcpy(v2h.fm[index].name, fm[index].name);
			}
			if (maxlen < ohp.indexpars.recsize)
				maxlen = ohp.indexpars.recsize;
			if (maxlen&1)
				maxlen++;
			newrecsize = V2RECSIZE+maxlen;
			newdocsize = V2HEADSIZE+ohp.rtot*newrecsize;
			v2h.indexpars.recsize = maxlen;
			if (mfile_resize(mf,newdocsize))	{	// if can extend file
				int count, mflag;
				for (count = ohp.rtot-1; count >= 0; count--)	{		// move all records to new positions
					V61_RECORD * ovptr = (V61_RECORD *)(mf->base+V61_HEADSIZE+count*(ohp.indexpars.recsize+V61_RECSIZE));
					V2RECORD * nvptr = (V2RECORD *)(mf->base+V2HEADSIZE+count*newrecsize);

					fixxstring(ovptr->rtext);		// fix EOCS
					movemappeddata(nvptr,ovptr,ohp.indexpars.recsize+V61_RECSIZE);
					mflag = tr_dosxstring(nvptr->rtext,&gh,TR_DOFONTS|TR_DOSYMBOLS);
					markcount += mflag;
					nvptr->ismark = mflag;		// add any mark
					tr_movesubcross(nvptr->rtext);	// move any subhead cross-ref
//					NSLog("%d, %s_%s",count,nvptr->rtext,nvptr->rtext+strlen(nvptr->rtext)+1);
				}
				memcpy(mf->base,&v2h,V2HEADSIZE);	// copy back new header
				ok = TRUE;
			}
		}
	}
	if (markcount)		/* if have marked any records */
		sendinfo(INFO_IMPORTMARKED,markcount);
	return ok;
}
/*******************************************************************************/
BOOL tr_ghok(struct ghstruct * ghp)	/* gets DOS font translation specs */

{
	if (sendwarning(WARN_CODETRANSLATION))
		return (DialogBoxParam(g_hinst,MAKEINTRESOURCE(IDD_CODETRANS),g_hwframe,transproc,(LPARAM)ghp) > 0);
	return (FALSE);
}
/******************************************************************************/
static INT_PTR CALLBACK transproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)

{
	struct ghstruct * ghp;
	HWND cbh;
	TCHAR fname[FSSTRING];

	ghp = getdata(hwnd);
	switch (msg)	{

		case WM_INITDIALOG:
			ghp = setdata(hwnd,(void *)lParam);	/* set data */
			type_setfontnamecombo(hwnd,IDC_CODETRANSG,NULL);
			type_setfontnamecombo(hwnd,IDC_CODETRANSH,NULL);
			centerwindow(hwnd,1);
			return FALSE;
		case WM_COMMAND:
			switch (LOWORD(wParam))	{
				case IDOK:
					cbh = GetDlgItem(hwnd,IDC_CODETRANSG);
					if (ComboBox_GetCurSel(cbh))	{	/* if want other than default font */
						GetWindowText(cbh,fname,FSSTRING);
						ghp->fcode[0] = type_findlocal(ghp->fmp,fromNative(fname),1);
					}
					cbh = GetDlgItem(hwnd,IDC_CODETRANSH);
					if (ComboBox_GetCurSel(cbh))	{	/* if want other than default font */
						GetWindowText(cbh,fname,FSSTRING);
						ghp->fcode[1] = type_findlocal(ghp->fmp,fromNative(fname),1);
					}
					ghp->flags |= READ_TRANSGH;
				case IDCANCEL:
					EndDialog(hwnd,LOWORD(wParam) == IDOK ? TRUE : FALSE);
					return TRUE;
			}
			break;
		case WM_HELP:
			return (dodialoghelp(hwnd,NULL,(HELPINFO *)lParam,wh_ctransid));
	}
	return FALSE;
}
/******************************************************************************/
BOOL tr_dosxstring(char * buff,struct ghstruct *ghp, int flags)	/* in-place translation of DOS CINDEX control codes in string */

	/* assumes dest string has room for any expansion */
{
	char * sptr, *tptr;
	short index;
	int xcode;
		
	for (xcode = FALSE, sptr = buff; *sptr != EOCS; sptr++)	{
		if (*sptr < 0)	{	/* extended character */
			sptr = dosextendedtowin(sptr,flags);
			if ((unsigned char)*sptr == DOSUNKNOWNCHAR)
				xcode = TRUE;
		}
		else if (*sptr < SPACE)
			sptr = doslowtowin(sptr);
		else if (*sptr == KEEPCHR && *(sptr+1))
			sptr++;			/* ensure don't treat following \ specially */
		else if (*sptr == ESCCHR && *(sptr+1))	{
		 	if (strchr(tr_escname,*(sptr+1)))	{
				*sptr++ = CODECHR;
				index = strchr(tr_escname,*sptr)-tr_escname;
				*sptr = tr_attrib[index >>1]|(index&1 ? FX_OLDOFF : 0);
				if (*sptr&FX_FONT)	{	/* if a g or h code */
					if (!(index&1) && flags&TR_DOFONTS)	{	/* if 'on' code and might want translated */
						ghp->flags |= READ_HASGH;			/* flag presence of codes (first pass) */
						if (ghp->flags & READ_TRANSGH)		/* if want translated */
							*sptr += ghp->fcode[(index>>1)-6];	/* add right font id */
					}
					else	/* off GH code or no translation */
						*sptr &= ~FX_OFF;	/* clear any 'off' style code -> default font */
					if (ghp->flags&READ_TRANSGH && !ghp->fcode[((index&~1)>>1)-6])	{	/* if have specified translation to default font */
						sptr--;				/* move to CODECHR */
						str_xshift(sptr+2,-2);	/* remove code altogether */
					}
				}
			}
			else if (*(sptr+1) == SPACE)	{	/* non-breaking space */
				str_xshift(sptr+1,-1);	/* move over code */
				*sptr = OLDFSPACE;			/* replace character */
			}
			else if (*(sptr+1) == '-')	{	/* non-breaking hyphen */
				str_xshift(sptr+1,-1);	/* move over code */
				*sptr = OLDENDASH;			/* replace character with en-dash */
			}
			else if (isdigit(*(sptr+1)))	{	/* encoded character */
				tptr = sptr+1;
				*sptr = str_transnum(&tptr);
				str_xshift(tptr,sptr-tptr+1);	/* move over the code */
				if (*sptr < 0)	{			/* if extended char */
					sptr = dosextendedtowin(sptr,flags);	/* translate */
					if ((unsigned char)*sptr == DOSUNKNOWNCHAR)
						xcode = TRUE;
				}
				else if (*sptr < SPACE)
					sptr = doslowtowin(sptr);	/*  translate */
				else if (*sptr == EOCS)		/* if unacceptable char */
					str_xshift(sptr+1,-1);	/* get rid of it */
			}
			else		/* some other character to protect */
				sptr += 2;	/* preserve slash and what follows */
		}
	}
	return (xcode);
}
/******************************************************************************/
void tr_movesubcross(char * buff)	/* moves any subhead cross-ref to page field */

{
	CSTR sarray[FIELDLIM];
	int scount;

	scount = str_xparse(buff, sarray);
	if (scount > 2 && !sarray[scount-1].ln)	{	/* if at least one subhead && empty page field */
//		if (str_crosscheck(FF,sarray[scount-2].str))	/* if last subhead is cross-ref */
		if (crosscheck(sarray[scount-2].str))	/* if last subhead is cross-ref */
			*sarray[scount-1].str = EOCS;	/* clip off page field */
	}
}
/******************************************************************************/
static short crosscheck(char *str)    /* checks string to see if begins with 'see' */

{
	static char *ptr = "see also under above";
	register char c1;
	char c2;

//	ptr = FF->head.refpars.crosstart;
	if (*(str-1) != KEEPCHR && *(str-1) != ESCCHR && (!isalpha(*(str-1)) || *(str-2) == CODECHR))	{	/* if not preceded by unescaped alpha */
		while (*str == '(' || *str == '[' || *str == OBRACE || *str == CODECHR && *++str)	/* while acceptable lead */
			str++;				/* skip it */
		do {
			if (isupper(c1 = *ptr++))       /* if upper case */
				c1 =  _tolower(c1);
			if (*str == CODECHR && *++str)		/* if code char */
				str++;                /* skip it */
			if (isupper(c2 = *str++))       /* if upper */
				c2 = _tolower(c2);
		} while (c1 == c2 && c1 != SPACE && *ptr && c1);  /* while match and not at end of elist */
		return (!(c1-c2));			/* return true if a match */
	}
	return (FALSE);	
}
/******************************************************************************/
static void convertDOSheader(V2HEAD *v2p, V61_HEAD * ohp, int maxfields)	// converts DOS header

{
	int count, index, len;
	short dummy;
	char * sptr, tstring[V61_STYLELEN];

	v2p->headsize = V2HEADSIZE;
	v2p->version = 100;
	v2p->elapsed = ohp->elapsed;
	v2p->createtime = ohp->createtime;
	v2p->squeezetime = ohp->squeezetime;
	v2p->rtot = ohp->rtot;

	// index pars
	v2p->indexpars.recsize = ohp->indexpars.recsize;
	v2p->indexpars.minfields = ohp->indexpars.minfields;
	if (maxfields < g_prefs.indexpars.maxfields)	/* if actual max less than preferred */
		maxfields = g_prefs.indexpars.maxfields;	/* set preferred */
	v2p->indexpars.maxfields = maxfields;

	for (count = 0; count < FIELDLIM; count++)	{	/* for all fields */
		if (count == ohp->indexpars.maxfields-1)
			strcpy(v2p->indexpars.field[count].name, "Page");
		else
			sprintf(v2p->indexpars.field[count].name, count ? "Sub%d" : "Main", count);
	}
	/* sort pars */
	v2p->sortpars.type = ohp->sortpars.type;
	count = 0;
	do {
		v2p->sortpars.fieldorder[count] = ohp->sortpars.fieldorder[count];
	} while (v2p->sortpars.fieldorder[count++] >= 0);
	if (v2p->sortpars.fieldorder[count-1] == PAGEINDEX)	/* if last field was page field */
		v2p->sortpars.fieldorder[v2p->indexpars.maxfields-1] = PAGEINDEX;	/* set to last field in converted index */
	v2p->sortpars.fieldorder[v2p->indexpars.maxfields] = -1;	/* always cap at maxfields */
	for (count = 0; count < V61_CHARPRI+1; count++)	/* for all reftypes */
		v2p->sortpars.charpri[count] = ohp->sortpars.charpri[count];
	v2p->sortpars.ignorepunct = ohp->sortpars.ignorepunct;
	v2p->sortpars.ignoreslash = ohp->sortpars.ignoreslash;
	v2p->sortpars.ignoreparen = ohp->sortpars.ignoreparen;
	v2p->sortpars.evalnums = ohp->sortpars.evalnums;
//	v2p->sortpars.crossfirst = ohp->sortpars.crossfirst;
	dosstripcpy(v2p->sortpars.ignore,ohp->sortpars.ignore,&dummy);
	v2p->sortpars.skiplast = ohp->sortpars.skiplast;
	v2p->sortpars.ordered = ohp->sortpars.ordered;
	v2p->sortpars.ascendingorder = ohp->sortpars.ascendingorder;
	v2p->sortpars.ison = ohp->sortpars.ison;
	for (count = 0; count < V61_REFTYPES+1; count++)	/* for all reftypes */
		v2p->sortpars.refpri[count] = ohp->sortpars.refpri[count];
	for (count = 0; count < V61_COMPMAX+1; count++)		/* for all parts */
		v2p->sortpars.partorder[count] = ohp->sortpars.partorder[count];

	/* ref pars */
	dosstripcpy(v2p->refpars.crosstart,ohp->refpars.crosstart,&dummy);
	dosstripcpy(v2p->refpars.crossexclude,ohp->refpars.crossexclude,&dummy);
	v2p->refpars.csep = ohp->refpars.csep;
	v2p->refpars.psep = ohp->refpars.psep;
	v2p->refpars.rsep = ohp->refpars.rsep;

	/* private pars */
	v2p->privpars.vmode = VM_DRAFT;
	v2p->privpars.wrap = TRUE;
	v2p->privpars.shownum = FALSE;
	v2p->privpars.hidedelete = FALSE;
	v2p->privpars.hidebelow = ALLFIELDS;
	v2p->privpars.size = 12;
	v2p->privpars.eunit = U_INCH;

	/* format pars (page) */
	v2p->formpars.pf.mc.top = 72;
	v2p->formpars.pf.mc.bottom = 72;
	v2p->formpars.pf.mc.left = 72;
	v2p->formpars.pf.mc.right = 72;
	v2p->formpars.pf.mc.ncols = ohp->formpars.pf.mc.ncols;
	v2p->formpars.pf.mc.gutter = 18;
	v2p->formpars.pf.mc.reflect = ohp->formpars.pf.reflect;
	if (*ohp->formpars.pf.mc.continued)	{	/* if had continued text */
		v2p->formpars.pf.mc.pgcont = 1;
		dosstripcpy(v2p->formpars.pf.mc.continued,ohp->formpars.pf.mc.continued,&v2p->formpars.pf.mc.cstyle.style);
	}
	else
		v2p->formpars.pf.mc.pgcont = 0;	/* no continuation stuff */
	v2p->formpars.pf.linespace = ohp->formpars.pf.linespace-1;	/* convert from line spacing to index */
	if (v2p->formpars.pf.linespace > FS_DOUBLE)
		v2p->formpars.pf.linespace = FS_DOUBLE;
	v2p->formpars.pf.entryspace = ohp->formpars.pf.entryspace;
	v2p->formpars.pf.above = ohp->formpars.pf.above;
	v2p->formpars.pf.firstpage = ohp->formpars.pf.firstpage;

	v2p->formpars.pf.lineheight = 16;
	v2p->formpars.pf.lineunit = U_INCH;
	v2p->formpars.pf.autospace = TRUE;
	v2p->formpars.pf.dateformat = DF_LONG;
	v2p->formpars.pf.pi.porien = DMORIENT_PORTRAIT;
	v2p->formpars.pf.pi.psize = DMPAPER_LETTER;
	v2p->formpars.pf.pi.pwidth = 0;
	v2p->formpars.pf.pi.pheight = 0;
	v2p->formpars.pf.pi.pwidthactual = 612;
	v2p->formpars.pf.pi.pheightactual = 792;

		/* format pars (entries) */
	v2p->formpars.ef.runlevel = ohp->formpars.ef.runlevel;
	v2p->formpars.ef.style = ohp->formpars.ef.style;
	v2p->formpars.ef.itype = ohp->formpars.ef.itype;
	v2p->formpars.ef.adjustpunct = ohp->formpars.ef.adjustpunct;

//	v2p->formpars.ef.runlevel = v2fp->ef.runlevel;
//	v2p->formpars.ef.collapselevel = v2fp->ef.collapselevel;
//	v2p->formpars.ef.style = v2fp->ef.style;
//	v2p->formpars.ef.itype = v2fp->ef.itype;
//	v2p->formpars.ef.adjustpunct = v2fp->ef.adjustpunct;
	v2p->formpars.ef.adjstyles = TRUE;
//	v2p->formpars.ef.fixedunit = v2fp->ef.fixedunit;
//	v2p->formpars.ef.autounit = v2fp->ef.autounit;
	v2p->formpars.ef.autolead = 1.;
	v2p->formpars.ef.autorun = 2.5;

		/* entry groups */
	v2p->formpars.ef.eg.method = 3;	
	if (*ohp->formpars.ef.eg.title)	/* if have group title */
		dosstripcpy(v2p->formpars.ef.eg.title,ohp->formpars.ef.eg.title,&v2p->formpars.ef.eg.gstyle.style);
	else
		strcpy(v2p->formpars.ef.eg.title,"%");	// set default

		/* fields */
	for (count = 0; count < FIELDLIM; count++)	{	/* individual fields */
		v2p->formpars.ef.field[count].style.style = styleids[ohp->formpars.ef.field[count].style];
		v2p->formpars.ef.field[count].style.cap = ohp->formpars.ef.field[count].capstype;	/* assume each char is an em space */
//		v2p->formpars.ef.field[count].leadindent = (float)strlen(ohp->formpars.ef.field[count].leadtext);	/* assume each char is an em space */
//		v2p->formpars.ef.field[count].runindent = (float)strlen(ohp->formpars.ef.field[count].runtext);	/* assume each char is an em space */
		v2p->formpars.ef.field[count].leadindent = (float)count;
		v2p->formpars.ef.field[count].runindent = (float)count+(float)1.5;
		dosstripcpy(v2p->formpars.ef.field[count].trailtext,ohp->formpars.ef.field[count].trailtext,&dummy);
	}
		/* cross refs */
	if (ohp->version == 600)	{	/* 6.0 */
		struct v6_crossform *cfptr;
		cfptr = (struct v6_crossform *)&ohp->formpars.ef.cf;
		dosstripcpy(v2p->formpars.ef.cf.level[0].cleada,cfptr->clead,&dummy);
		dosstripcpy(v2p->formpars.ef.cf.level[0].cleadb,cfptr->clead,&dummy);
		dosstripcpy(v2p->formpars.ef.cf.level[0].cenda,cfptr->cend,&dummy);
		dosstripcpy(v2p->formpars.ef.cf.level[0].cendb,cfptr->cend,&dummy);
		dosstripcpy(v2p->formpars.ef.cf.level[1].cleada,cfptr->clead,&dummy);
		dosstripcpy(v2p->formpars.ef.cf.level[1].cleadb,cfptr->clead,&dummy);
		dosstripcpy(v2p->formpars.ef.cf.level[1].cenda,cfptr->cend,&dummy);
		dosstripcpy(v2p->formpars.ef.cf.level[1].cendb,cfptr->cend,&dummy);
		v2p->formpars.ef.cf.mainposition = CP_AFTERPAGE;	/* default always after heading */
//		v2p->sortpars.crossfirst = TRUE;				/* requires crossref first */
	}
	else	{	/* v 6.1 */
		dosstripcpy(v2p->formpars.ef.cf.level[0].cleada,ohp->formpars.ef.cf.level[0].cleada,&dummy);
		dosstripcpy(v2p->formpars.ef.cf.level[0].cleadb,ohp->formpars.ef.cf.level[0].cleadb,&dummy);
		dosstripcpy(v2p->formpars.ef.cf.level[0].cenda,ohp->formpars.ef.cf.level[0].cenda,&dummy);
		dosstripcpy(v2p->formpars.ef.cf.level[0].cendb,ohp->formpars.ef.cf.level[0].cendb,&dummy);
		dosstripcpy(v2p->formpars.ef.cf.level[1].cleada,ohp->formpars.ef.cf.level[1].cleada,&dummy);
		dosstripcpy(v2p->formpars.ef.cf.level[1].cleadb,ohp->formpars.ef.cf.level[1].cleadb,&dummy);
		dosstripcpy(v2p->formpars.ef.cf.level[1].cenda,ohp->formpars.ef.cf.level[1].cenda,&dummy);
		dosstripcpy(v2p->formpars.ef.cf.level[1].cendb,ohp->formpars.ef.cf.level[1].cendb,&dummy);
		v2p->formpars.ef.cf.mainposition = ohp->formpars.ef.cf.position;
	}
	v2p->formpars.ef.cf.leadstyle.style = styleids[ohp->formpars.ef.cf.leadstyle];
	v2p->formpars.ef.cf.bodystyle.style = styleids[ohp->formpars.ef.cf.bodystyle];
	v2p->formpars.ef.cf.sortcross = ohp->formpars.ef.cf.sortcross;

		/* locators */
	v2p->formpars.ef.lf.sortrefs = ohp->formpars.ef.lf.sortrefs;
	v2p->formpars.ef.lf.rjust = strchr(ohp->formpars.ef.lf.llead1,'*') ? TRUE : FALSE;
	dosstripcpy(v2p->formpars.ef.lf.llead1,ohp->formpars.ef.lf.llead1,&dummy);
	dosstripcpy(v2p->formpars.ef.lf.lleadm,ohp->formpars.ef.lf.lleadm,&dummy);
	dosstripcpy(v2p->formpars.ef.lf.connect,ohp->formpars.ef.lf.connect,&dummy);
	v2p->formpars.ef.lf.conflate = ohp->formpars.ef.lf.conflate;
	v2p->formpars.ef.lf.abbrevrule = ohp->formpars.ef.lf.abbrevrule;
	v2p->formpars.ef.lf.suppressparts = *ohp->formpars.ef.lf.suppress ? TRUE : FALSE;
	dosstripcpy(v2p->formpars.ef.lf.suppress,ohp->formpars.ef.lf.suppress,&dummy);
	dosstripcpy(v2p->formpars.ef.lf.concatenate,ohp->formpars.ef.lf.concatenate,&dummy);
		/* !! can we translate locator style sequences ? */

	fixxstring(ohp->stylestrings);	/* convert EOCS to new form */
	count = str_xcount(ohp->stylestrings);
	for (index = 0, sptr = v2p->stylestrings; index < count; index++, sptr += strlen(sptr++))	{		/* for all styled strings */
		dosstripcpy(tstring,str_xatindex(ohp->stylestrings,(short)index),&dummy);	/* clean up string */
		len = strlen(tstring);
		if (sptr+len+2 >= v2p->stylestrings+STYLESTRINGLEN)	
			break;
		*sptr++ = styleids[ohp->formpars.ef.autostyle]|FX_OLDOFF;
		strcpy(sptr,tstring);
	}
	*sptr = EOCS;
}
/******************************************************************************/
static BOOL scanrecords(V61_HEAD * ohp, char * base, struct ghstruct *ghp, int * maxlen, int *maxfields)     /* scans records */

{
	char buff[MAXREC+V61_RECSIZE];
	RECN rcount;
	V61_RECORD * vptr, *tvptr;
	int len,  fields;

	vptr = (V61_RECORD *)buff;
	for (*maxlen = *maxfields = rcount = 0; rcount < ohp->rtot; rcount++)	{
		tvptr = (V61_RECORD *)(base+(rcount*(V61_RECSIZE+ohp->indexpars.recsize)));
		memcpy(vptr,tvptr,V61_RECSIZE+ohp->indexpars.recsize);
		fixxstring(vptr->rtext);
		tr_dosxstring(vptr->rtext,ghp,TR_DOFONTS|TR_DOSYMBOLS);
		len = str_xlen(vptr->rtext)+1;	/* holds count of full need */
		if (len > *maxlen)
			*maxlen = len;
		fields = str_xcount(vptr->rtext);
		if (fields > *maxfields)
			*maxfields = fields;
	}
	return (rcount == ohp->rtot);
}
/******************************************************************************/
static char * fixxstring(char * string)     /* converts EOCS from DOS format */

{
	char * eptr = NULL;

	if (eptr = memchr(string,DOSEOCS,MAXREC))
		*eptr = EOCS;
	return (eptr);
}
/******************************************************************************/
static void dosstripcpy(char * dptr, char * sptr, short *code)     /* copies, stripping DOS style codes */

{
	char * tptr, base[2*STSTRING], *bptr;

	*code = '\0';		/* assume no codes */
	bptr = base;
	while (*bptr = *sptr++)	{
		if (*bptr == ESCCHR && (tptr = strchr(esclist,*sptr)))	{
			if (!*code && islower(*sptr))		/* if haven't already captured a code */
				*code = styleids[tptr-esclist+1];	/* translate it (add 1 for empty code at head of styleids) */
			*sptr++;
		}
		else
			bptr++;
	}
	*++bptr = EOCS;	/* make compound string */
	tr_dosxstring(base,NULL,FALSE);	/* now translate any remaining codes */
	strcpy(dptr,base);		/* copy fixed string */
}
/******************************************************************************/
static char * dosextendedtowin(char * sptr, int flags)     /* translates dos extended char */

{
	*sptr = dos_to_win[(unsigned char)(*sptr)-128];
	if (strchr(dos_greek,*sptr) && flags&TR_DOSYMBOLS)	{	/* if needs to go to symbol font */
		char gchar = *sptr;
		str_xshift(sptr,4);		/* make room for font codes */
		*sptr++ = CODECHR;
		*sptr++ = FX_FONT|1;	/* symbol font */
		*sptr++ = gchar;		/* symbol */
		*sptr++ = CODECHR;
		*sptr = FX_FONT;		/* default font */
	}
	return (sptr);
}
/******************************************************************************/
static char * doslowtowin(char * sptr)     /* translates dos low char */

{
	char cc;

	if (*sptr)	{
		cc = doslow_to_win[*sptr];
		if (*sptr <= 7)	{		/* if want symbol translation */
			str_xshift(sptr,4);		/* make room for font codes */
			*sptr++ = CODECHR;
			*sptr++ = FX_FONT|1;	/* symbol font */
			*sptr++ = cc;		/* symbol */
			*sptr++ = CODECHR;
			*sptr = FX_FONT;		/* default font */
		}
		else		/* take in text font */
			*sptr = cc;
	}
	return (sptr);
}
/******************************************************************************/
unsigned char str_transnum(char **iptr)	   /* translates (up to three) decimal digits to char */
			/* returns with altered pointer to input string */

{
	register char c, *index;
	unsigned short count;

	for (index = *iptr, count = 3, c = 0; isdigit(*index) && count--;)	       /* while a digit */
		c = c*10 + *index++ -'0';	/* convert */
	*iptr = index;                /* advance input pointer */
	return (c);
}
