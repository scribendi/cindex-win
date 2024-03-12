#include "stdafx.h"
#include <string.h>
#include "strings.h"
#include "errors.h"
#include "records.h"
#include "sort.h"
#include "collate.h"
#include "typestuff.h"
#include "viewset.h"
#include "util.h"
#include "refs.h"
#include "formstuff.h"
#include "import.h"
#include "tagstuff.h"

#define SAFETYMARGIN 200		/* safety margin used by buildentry */

#define PARRAYSIZ 3000   /* no of discrete page refs that can be buffered */
#define CBUFFLEN 7500
#define PBUFFLEN 15000

static INDEX * s_index;
static SORTPARAMS * s_sg;

static char *f_commaspace = ", ";
static char *f_semispace = "; ";
static char *f_colonspace = ": ";
static char *f_periodspace = ". ";
static char f_newline[] = {FO_LEVELBREAK,0};
static char f_newlevel[] = {FO_NEWLEVEL,0};
static char f_pgc[] = {FO_EPAGE,0};
static char *f_titleskiplist;

#if 0
static char *f_openset = "\'\"‘“";
static char *f_closeset = "\'\"’”";
#else
static unichar f_openset[] = {34,39,8220,8216/*,8249*/,0}; // last char is angle quote
static unichar f_closeset[] = {34,39,8221,8217/*,8250*/,0};
#endif
static TCHAR * _labelpos;
static unsigned char entrybasebuff[EBUFSIZE+2];		/* big array for building entries */
static char _trefbuff[PBUFFLEN];		/* buffer for sorting page/cross-refs */
static char _rrefbuff[PBUFFLEN];		// for holding pos-merge replacement references
static char _cbuff[CBUFFLEN], _pbuff[PBUFFLEN+PARRAYSIZ];	/* NB: _pbuff needs extra when copying back */

char f_stringbase[1500];	/* holds strings that define dynamically-built formatting tags */
unsigned char *f_entrybuff = entrybasebuff+2;	// offset to cope with pos addressing before start (e.g., by transposepunct())

TCHAR f_widebuff[EBUFSIZE+2];	// unicode buffer

static TCHAR * measurewidth(HDC dc, TCHAR * base, TCHAR * source, ATTRIBUTES *as, long limitwidth, long *freespace, struct selstruct * slptr, short lcount, short * lptr);       /*finds posn of last printable char within width */
static short addblanks(INDEX * FF, LSET * array, short lcount, short offset, short lscount);	/* sets up blank lines */
static short findrunlevel(INDEX * FF, RECORD * recptr, short dir);	/* finds level for running-in */
static void inscode(char ** pos, short codes, short type);	/* inserts style code */
static void setcolor(char ** pos, unsigned char color);	// inserts color if needed
static void clearcolor(char ** pos, unsigned char color);	// clears color if needed
static void setcaps(char * tpos, short type);	/* fixes capitalization */
static short switchfont(char * pos, short localid, short mode);	/* inserts font/style code */
static char *copystep(char * str1, char *str2);		/* adds string (with protection), returns ptr to end */
static short findindent(INDEX * FF, short indent);     /* returns indent for this field */
static int buildrefs(INDEX * FF, char * tbuff, RECORD ** xptr, char * epos, short * prcount, short *crcount, short level, short allowcbreak, int * pagerecords);	/* builds page string */
static char *placecrossref(char * tpos, short runlevel, short level, short mpos, short spos, short lastplevel);	/* sets up cross-ref placement */
static short insertlead(char * lptr, char * base);	/* inserts lead before refs */
static short buildcrossprefix(INDEX * FF, char * dest, char ** source, struct codeseq * cs, short * tokens);	/* gather & formats lead */
static short parserefs(char * tempbuff, char * pptr, char ** ptrarray, char sepchar, struct codeseq * cs);	/* organizes reference string */
static void doautostyles(INDEX * FF, char * tbuff);	/* inserts auto styles */
static void adjustcodes(INDEX * FF, char * tbuff);	/* adjusts positions of style codes */
static BOOL isletterfont(INDEX * FF, char * base, char * tpos);	/* returns TRUE if ansi font in use */
static void transposepunct(char * tbuff);	/* fixes quote punctuation */
static int fixpage(INDEX * FF, char * pgbuf,short *rount);	/* organizes page ref string */
static int pcompare (const void * s1, const void * s2);       /* compares page refs for qsort */
static int fixcross(INDEX * FF, char * pgbuf, short *rcount, short sflag, struct codeseq *cs);	/* organizes cross-ref string */
static int tcompare(const void * s1, const void * s2);	/* text compare for qsort */
static char *suppressmatch(char *s1, char *s2, char * suppress);	/* finds if refs match through suppression string */
static char * setpagestyle (INDEX * FF, char *tbuff);	/* styles page refs */
static BOOL conflate(char *dest, char *source, char connect, BOOL overlap);		/* conflates refs */
static BOOL bnacont(char * uptr);		/* finds if need BNA continuation */

/**********************************************************************/
static BOOL bnacont(char * uptr)		/* finds if need BNA continuation */

{	
#ifdef PUBLISH
	return (*uptr == ';' || *uptr =='<' && *(uptr+1) ==';');	/* if a run-on */
#else
	return FALSE;
#endif
}
/**********************************************************************/
RECORD * form_getprec(INDEX * FF, RECN rnum)	/* returns ptr to record (or parent) */

{
	RECORD * recptr, *curptr;
	short hlevel, hidelevel,sprlevel,runlevel,clevel;
	char * uptr;
	
	curptr = NULL;
	if (recptr = rec_getrec(FF,rnum))	{
		runlevel = findrunlevel(FF,recptr,-1);
		do	{				/* until get to heading level that requires break */
			curptr = recptr;
			uptr = rec_uniquelevel(FF, recptr, &hlevel,&sprlevel,&hidelevel,&clevel);	/* find level at which heading unique */
		} while ((hlevel >= runlevel || hlevel >= hidelevel || hlevel >= clevel || sort_isignored(FF,recptr) || sprlevel <= hlevel || bnacont(uptr)) && (recptr = sort_skip(FF,recptr,-1)));
	}
	return (curptr);
}
/**********************************************************************/
RECORD * form_skip(INDEX * FF, RECORD * recptr, short dir)	/* skips in formatted mode */

{
	short hlevel, hidelevel, sprlevel,runlevel,clevel;
	char * uptr;
	
	runlevel = findrunlevel(FF,recptr,dir);
	do	{		/* until get to heading level that requires break */
		if (recptr = sort_skip(FF,recptr,dir))		/* if have a record */
			uptr = rec_uniquelevel(FF, recptr, &hlevel,&sprlevel,&hidelevel,&clevel);	/* find level at which heading unique */
	} while (recptr && (hlevel >= runlevel || hlevel >= hidelevel || hlevel >= clevel || sprlevel <= hlevel || bnacont(uptr)));	// while hidden or run-on
	return (recptr);
}
/******************************************************************************/
char * form_formatcross(INDEX * FF, char * source)	/* formats cross-ref into static string */

{
	short tokens, crcount;
	struct codeseq cs;
	char * dest;

	dest = buildcrossprefix(FF,_cbuff,&source,&cs,&tokens)+_cbuff;	/* rebuild prefix */
	strcpy(dest,source);		/* copy body */
	fixcross(FF,dest,&crcount,FALSE, &cs);	/* in-place conversion */
	return (_cbuff);
}
/**********************************************************************/
short form_disp(HWND wptr, HDC dc, short line, short lcount)	/* displays lines of entry */
	/* stops at end of entry, or lcount, or end of screen (whichever first) */
	/* returns number of lines written */

{
	BOOL active = GetFocus()== wptr;
	RECORD * recptr;
	TCHAR *tbuff;
	short totlines;
	INDEX * FF;
	DLINE * dlptr;
	ENTRYINFO es;
	ATTRIBUTES ast;
	RECT trect;
	long height, linepos;
	short conttype, indent = 0;
	LFLIST * lfp;
	
	FF = getowner(wptr);
	SaveDC(dc);
	lfp = getdata(wptr);
	dlptr = &lfp->lines[line];
	conttype = FF->continued = dlptr->indent&FO_CONTIN;	/* specify build type (only ever TRUE if printing) */ 
	totlines = 0;
	memset(&ast,0,sizeof(ast));				/* clear attribute struct */
	ast.cursize = ast.nsize = FF->head.privpars.size;		/* temp standard font size */
	ast.fmp = FF->head.fm;
	if (recptr = rec_getrec(FF,dlptr->rnum))	{	/* get its record */
		tbuff = form_buildentry(FF,recptr, &es);		/* if can get text */
		fixsmallcaps(tbuff);
		form_putline(dc,tbuff,&ast,tbuff+dlptr->base, FALSE,0,0);	/* just to fill codearray */
		height = lfp->lheight;
		trect = lfp->vrect;			/* basic rect for highlighting */
		trect.top = height*line;
		trect.bottom = trect.top + height;
		linepos = lfp->drect.top+height*line+lfp->linedrop;
		while (line < lfp->nlines && dlptr->rnum == recptr->num && lcount-- && conttype == (dlptr->indent&FO_CONTIN))	{	/* while on screen and in logical record */
			MoveToEx(dc,lfp->drect.left,linepos,NULL);	/* start of line */
			if (dlptr->flags&VSELECT) {		/* if highlighted */
				trect.left = lfp->drect.left + (dlptr->selstart >= 0 ? indent+dlptr->selstart : 0);
				trect.right = dlptr->selend >= 0 ? lfp->drect.left + indent+dlptr->selend : lfp->vrect.right;
				FillRect(dc,&trect, active ? g_highlightbrush : GetStockObject(LTGRAY_BRUSH));
			}
			if (!(dlptr->indent&FO_BLANK))	{ 		/* if not a blank line */
#ifdef PUBLISH
				if (dlptr->flags&VLABELED && FF->pf.labelmark)		{	/* if at line to display number && want label mark */
					MoveToEx(dc,lfp->drect.left-lfp->emwidth,linepos,NULL);		/* set to mark position */
					TextOut(dc,0,0,TEXT("•"),1);
				}
#endif // PUBLISH
				indent = findindent(FF, dlptr->indent);	/* get the indent */
				MoveToEx(dc,lfp->drect.left+indent,linepos,NULL);		/* set lead indent */
				form_putline(dc,tbuff+dlptr->base,&ast,tbuff+dlptr->base+dlptr->count, TRUE,dlptr->freespace, FF->head.formpars.ef.lf.leader);
	        }
			trect.top += height;
			trect.bottom += height;
			dlptr++;
	 		line++;
	 		totlines++;
			linepos += lfp->lheight;
		}
	}
	RestoreDC(dc,-1);
	return (totlines);
}
/******************************************************************************/
void form_putline(HDC dc,TCHAR * source, ATTRIBUTES * as, TCHAR * eptr, short visible, short free, char leadtype)      /* return first printable char */

{
	TCHAR cc, *scan, *cbase;
	POINT pos;
	
	cbase = scan = source;
	GetCurrentPositionEx(dc,&pos);
	MoveToEx(dc,pos.x,pos.y-as->soffset,NULL);		/* move to any offset base posn */
	while ((cc = *scan) && scan < eptr) {       /* while not at end or length lim  */
		switch (*scan++) {
			case CODECHR:
				if (visible)		/* want to change style */
					TextOut(dc,0,0,cbase,scan-cbase-1);
				type_doattributes(dc,as,*scan++);
				break;
			case FONTCHR:
				if (visible)		/* want to change style */
					TextOut(dc,0,0,cbase,scan-cbase-1);
				type_doattributes(dc,as,(*scan++)|FX_AUTOFONT);
				break;
			case ESCCHR:			/* if escape sequence */
			case KEEPCHR:			/* if keep marker */
				if (visible)
					TextOut(dc,0,0,cbase,scan-cbase-1);
				cbase = scan++;		/* next char tested is one after protected one */
				continue;
			case FO_RPADCHAR:
#ifdef SBREAK
			case SORTBREAK:			/* sort break char */
#endif
			case FO_LEVELBREAK:
			case FO_NEWLEVEL:
				if (visible)
					TextOut(dc,0,0,cbase,scan-cbase-1);
				if (cc == FO_LEVELBREAK  || cc == FO_NEWLEVEL)	/* if a field change */
					type_clearattributes(dc,as);	/* clear font attributes */
				break;
			default:
				if (as->attr&FX_SMALL)		/* if need small caps */
					*(scan-1) = u_toupper(*(scan-1));	/* make it so */
				continue;
		}
		cbase = scan;
		if (cc == FO_RPADCHAR)	{	/* if wanted to right-justify the next text */
			int limit;

			GetCurrentPositionEx(dc,&pos);
			limit = abs(pos.x)+free;
			if (leadtype && visible)	{	/* if dot leader */
				while (abs(pos.x) < limit) {
					TextOut(dc,0,0,TEXT("."),1);
					GetCurrentPositionEx(dc,&pos);
				}
			}
			MoveToEx(dc,limit,pos.y,NULL);	/* capture remaining space */
		}
	}
	if (visible)
		TextOut(dc,0,0,cbase,scan-cbase);
}
/*******************************************************************************/
short form_measurelines(HWND wptr, HDC dc, RECORD * recptr, LSET *array, struct selstruct *slptr)	
		/* finds lines needed for display, fills array of start positions */
		/* last array element is set to starting offset in record */
					
{
	LFLIST *lfp = getdata(wptr);
	short lcount;
	INDEX * FF;
	short  rflag, indent,cflag, textlines, currentlevel;
	long maxwidth;
	TCHAR * tbuff, * tpos;
	ATTRIBUTES ast;
	ENTRYINFO es;
	
	textlines = lcount = 0;
	FF = getowner(wptr);
	SaveDC(dc);
	cflag = FF->continued ? FO_CONTIN : FALSE;
	if (!recptr->isdel)	{	/* if not del && can get text */
		tbuff = form_buildentry(FF,recptr, &es);
		fixsmallcaps(tbuff);
		currentlevel = es.ulevel;		// set working level
		maxwidth = lfp->width ? lfp->width : LONG_MAX;
		array[0].offset = 0;
		memset(&ast,0,sizeof(ast));				/* clear attribute struct */
		ast.cursize = ast.nsize = FF->head.privpars.size;		/* temp standard font size */
		ast.fmp = FF->head.fm;
		if (es.leadchange && !es.firstrec)	/* if need group space (&& not first record) */
			lcount = addblanks(FF, array, lcount, 0, FF->head.formpars.pf.above+1);
		for (rflag = 0, tpos = tbuff; *tpos && lcount < LLIMIT-1; textlines++)	{
			while (*tpos == FO_NEWLEVEL)	{	// accumulate any level shifts
				currentlevel++;		// increment level
				tpos++;
			}
			if (currentlevel == 0 && !rflag && (!es.firstrec || textlines))	// if not first displayed line of main heading and (not first record or have emitted ahead)
				lcount = addblanks(FF, array, lcount, tpos-tbuff, FF->head.formpars.pf.entryspace+1);
			if (currentlevel > 0 && FF->head.formpars.ef.itype == FI_SPECIAL && currentlevel == es.llevel)		/* if special indent */
				currentlevel = FF->head.indexpars.maxfields-2;		/* drive lowest-level indent */
//			array[lcount].indent = currentlevel >= 0 ? currentlevel + rflag + cflag  : 0;	/* indent isn't real -- codes level + runon + contin + forced run level */
			array[lcount].indent = currentlevel >= 0 ? currentlevel + rflag + cflag + es.forcedrun : 0;	/* indent isn't real -- codes level + runon + contin + forced run level */
			indent = findindent(FF, array[lcount].indent);		/* set lead indent */
			tpos = measurewidth(dc,tbuff,tpos, &ast, maxwidth-indent,&array[lcount].free, slptr, lcount, &currentlevel);	/* find last char for current line */
#ifdef PUBLISH
			if (_labelpos && _labelpos < tpos)
				array[lcount].indent |= FO_LABELED;
#endif // PUBLISH
			array[++lcount].offset = tpos-tbuff;
			if (*tpos == FO_LEVELBREAK)	{	/* if line ended on a break */
				*tpos++;		// discard it
				currentlevel++;	// increment level
				rflag = FALSE;	// no runover
			}
			else
				rflag = FO_RFLAG;
		}
		if (!cflag)	{	/*  if no contin */
		 	if (!FF->nostats)	{	/* if want to use the stats */
				FF->pf.prefs += es.prefs;				/* accumulate stats */
				FF->pf.crefs += es.crefs;
				FF->pf.entries += es.drecs;
				FF->pf.lines += textlines;
				if (!es.ulevel)	// if main heading
					FF->pf.uniquemain++;
			}
			FF->nostats = FALSE;
		}
		if (lcount >= LLIMIT-1)
			senderronstatusline(ERR_LONGENTRYERR,WARN,recptr->num);
		if (lfp->fillEnabled)	{
			lfp->filledRecords += es.consumed;
			lfp->filledLines += lcount;
		}
	}
	RestoreDC(dc,-1);
	return (lcount);
}
/******************************************************************************/
static TCHAR * measurewidth(HDC dc, TCHAR * base, TCHAR * source, ATTRIBUTES *as, long limitwidth, long *freespace, struct selstruct * slptr, short lcount,short * lptr)       /*finds posn of last printable char within width */

{
	TCHAR *mark, *scan, *cbase;
	long totwidth, curseglen, markwidth,mcount;
	SIZE size;
	
	if (limitwidth < 0)		/* if no room for anything (lead is too large) */
		return base;
	_labelpos = NULL;	// used to mark position of turning on color
	for (curseglen = totwidth = 0, cbase = mark = scan = source; *scan;) {       /* while not at end or length lim  */
		if (scan-base == slptr->startpos)	{
			GetTextExtentPoint32(dc,cbase,scan-cbase,&size);
			slptr->shighlight = totwidth+size.cx;	/* start offset of highlight */
			slptr->sline = lcount;
		}
		if (scan-base == slptr->startpos+slptr->length)		{			/* if last char */
			GetTextExtentPoint32(dc,cbase,scan-cbase+1,&size);
			slptr->ehighlight = totwidth+size.cx;	/* include end char */
			slptr->eline = lcount;
		}
		switch (*scan++) {
			case CODECHR:
				GetTextExtentPoint32(dc,cbase,scan-cbase-1,&size);
				curseglen = size.cx;	/* count cur segment up to this */
				type_doattributes(dc,as,*scan++);
				cbase = scan;	/* reset base for next segment */
				break;
			case FONTCHR:
				GetTextExtentPoint32(dc,cbase,scan-cbase-1,&size);
				curseglen = size.cx;	/* count cur segment up to this */
				if ((*scan&(FX_COLOR|FX_COLORMASK)) > FX_COLOR)	/* if turning on a color */
					_labelpos = scan;			// currently used only by PUBLISH
				type_doattributes(dc,as,(*scan++)|FX_AUTOFONT);
				cbase = scan;	/* reset base for next segment */
				break;
			case ESCCHR:			/* if escape sequence */
			case KEEPCHR:			/* if keep marker */
				GetTextExtentPoint32(dc,cbase,scan-cbase-1,&size);
				curseglen = size.cx;	/* count cur segment up to this */
				cbase = scan++;		/* next char tested is one after protected one */
				break;
#ifdef SBREAK
			case SORTBREAK:			/* sort break char */
#endif
			case FO_NEWLEVEL:
				*lptr += 1;
			case FO_PAGE:
			case FO_EPAGE:
			case FO_CROSS:
			case FO_ECROSS:
			case FO_RPADCHAR:			/* right pad char */
				GetTextExtentPoint32(dc,cbase,scan-cbase-1,&size);
				curseglen = size.cx;	/* count cur segment up to this */
				cbase = scan;	/* reset base for next segment */
				break;
			case FO_LEVELBREAK:
				scan--;		/* will return pointing at newline */
				goto done;
			case SPACE:
			case DASH:
				GetTextExtentPoint32(dc,cbase,scan-cbase,&size);
				curseglen = size.cx;	/* count cur segment up to this */
				if (curseglen + totwidth < limitwidth)	{	/* if this is potentially a good break */
					mark = scan;
					markwidth = totwidth+curseglen;
					continue;		/* don't accumulate yet, for better fractional measure */
				}
				cbase = scan;	/* reset base for next segment */
				break;
			default:
				if (as->attr&FX_SMALL)		/* if need small caps */
					*(scan-1) = u_toupper(*(scan-1));	/* make it so */
				continue;
		}
		if (curseglen+totwidth >= limitwidth)	/* if current segment doesn't fit */
			goto toolong;
		totwidth += curseglen;		/* add to total */
	}
done:
	GetTextExtentPoint32(dc,cbase,scan-cbase,&size);
	curseglen = size.cx;	/* measure final segment */
	if (curseglen+totwidth < limitwidth)	/* if can add last segment */
		totwidth += curseglen;
	else {
toolong:
		if (mark > source)	{	/* if have a break somewhere */
			scan = mark;	/* go back to last space/dash */
			totwidth = markwidth;
		}
		else	{		/* no natural break within line */
			GetTextExtentExPoint(dc,source,nstrlen(source),limitwidth,&mcount,NULL,&size);
			scan = source+mcount-1;	
		}
	}
	while (*scan == SPACE)
		scan++;
	*freespace = limitwidth-totwidth;	/* freespace on line */
	return (scan);	/* first character not covered */
}
/******************************************************************************/
static short addblanks(INDEX * FF, LSET * array, short lcount, short offset, short lscount)	/* sets up blank lines */

{
	while (--lscount && lcount < LLIMIT-1)	{	/* while need extra line spaces */
		array[lcount].indent = FO_BLANK;		/* set blank elements */
		array[++lcount].offset = offset;
	}
	return (lcount);
}
/******************************************************************************/
TCHAR * form_buildentry(INDEX * FF, RECORD * recptr, ENTRYINFO* esp)	/* builds entry text */

{
	CSTR scur[FIELDLIM];
	short curcount, hlevel, lastlevel, runlevel, prcount, crcount, stoplevel;
	short sprlevel, hidelevel,clevel;
	int requiredlevel, labellevel, reccount;
	short puncttype;
	unichar leadchar;
	ENTRYFORMAT *efp;
	FIELDFORMAT *fp;
	char * tbuff, * tpos, *epos, *cpos, *mark;
	RECORD * curptr;
	struct codeseq cs;
	BOOL specialrunon = FALSE;
	unsigned char tagcode;
	char * locatormark = NULL;
	
	s_index = FF;				/* index ptr (need static for qsort) */
	s_sg = &FF->head.sortpars;	/* sort group ptr */
	efp = &FF->head.formpars.ef;
	memset(esp,0,sizeof(ENTRYINFO));
	tbuff = f_entrybuff;
	f_titleskiplist = FF->head.flipwords;
	curcount = str_xparse(recptr->rtext, scur);		/* parse string */
	requiredlevel = FF->head.indexpars.required ? curcount-2 : INT_MAX;
	if (efp->style == FL_NOSUPPRESS)	{	// don't suppress repeated headings
		hlevel = 0;
		sprlevel = hidelevel = clevel = PAGEINDEX;
	}
	else
		rec_uniquelevel(FF, recptr, &hlevel,&sprlevel,&hidelevel,&clevel);	/* find level at which heading unique */
	runlevel = findrunlevel(FF,recptr, 1);
	if (FF->continued)	{		/* if looking for continuation heading */
		stoplevel = hlevel-1;	/* go as far as unique level */
		if (stoplevel < 0)		/* if a main head contin */
			stoplevel = curcount-2;	/* go to lowest level heading */
		if (stoplevel >= runlevel)	/* if would be run-on */
			stoplevel = runlevel-1;	/* set limit to field before run-on */
		if (stoplevel > FF->head.formpars.pf.mc.clevel)
			stoplevel = FF->head.formpars.pf.mc.clevel;
		hlevel = 0;
	}
	esp->ulevel = hlevel;    				/* set unique level */
	esp->llevel = curcount-2;				/* set lowest level */
	if (!(curptr = sort_skip(FF,recptr,-1)))	// if no last lead
		esp->firstrec = TRUE;		/* first record in view */
	esp->leadchange = col_newlead(FF,curptr ? curptr->rtext : NULL,recptr->rtext, &leadchar);
	curptr = recptr;
	tpos = tbuff;
	if (esp->leadchange && !FF->continued && FF->head.sortpars.fieldorder[0] != PAGEINDEX)	{
		char * leadtext;
		if (leadchar == SYMBIT)
			leadtext = efp->eg.sinsert;
		else if (leadchar == NUMBIT)
			leadtext = efp->eg.ninsert;
		else if (leadchar == BOTHBIT)
			leadtext = efp->eg.nsinsert;
		else
			leadtext = efp->eg.title;
		esp->ahead = FALSE;	// assume no heading text
		if (*leadtext)	{	/* if want to do title */
			tpos += switchfont(tpos,type_findlocal(FF->head.fm,efp->eg.gfont,0),ON);		/* change font if necess */
			inscode(&tpos,efp->eg.gstyle.style,0);	/* insert any 'on' code before text */
			copystep(tpos, leadtext);
			setcaps(tpos,efp->eg.gstyle.cap);
			tpos += strlen(tpos);
			inscode(&tpos,efp->eg.gstyle.style,FX_OFF);	/* insert any 'off' code after text */
			tpos += switchfont(tpos,0,OFF);			/* restore default font */
			*tpos ='\0';
			if ((epos = strchr(tbuff,'%')) && u_isgraph(leadchar))	{
				int length;
				UChar decomp[10];
				UErrorCode errorCode = U_ZERO_ERROR;
					
				length = unorm2_getDecomposition(FF->collator.unorm,leadchar,decomp,sizeof(decomp),&errorCode);
				u8_insertU(epos,length > 0 ? decomp[0] : leadchar,1);	// use decomposed character if there is one
			}
			tpos += strlen(tpos);	// now points to end of expanded string
			esp->ahead = TRUE;		// indicate heading has text
			*tpos++ = FO_LEVELBREAK;		/* add line break */
			esp->ulevel = -1;		// level flags heading to be emitted
		}
	}
	do	{		/* until get to heading level that requires break */
		if (g_prefs.gen.showlabel && curptr->label && !*scur[curcount-1].str)	{	// if should label lowest displayed field
			for (labellevel = curcount-2; labellevel > 0 && (labellevel > clevel || !*scur[labellevel].str); labellevel--)	// skip back while field invisible or empty
				;
		}
		else
			labellevel = INT_MAX;
		for (lastlevel = -1; hlevel < curcount-1; hlevel++)		{	/* while not in page field */
			fp = &efp->field[hlevel == requiredlevel ? FF->head.indexpars.maxfields-2 : hlevel];	/* set ptr to field parameters */
			if (hlevel < clevel && hlevel < hidelevel)	{	// if not collapsing or suppressing
				tagcode = hlevel == labellevel ? FX_COLOR|curptr->label : 0;
				if (tpos+scur[hlevel].ln+SAFETYMARGIN >= tbuff+EBUFSIZE)	{	/* if not enough room */
					senderronstatusline(ERR_LONGENTRYERR,WARN,recptr->num);
					goto noroom;
				}
				mark = tpos;		// default base for possible discard of this field
				if (lastlevel >= 0)	{	// if second or subsequent field
					if (efp->style == FL_RUNBACK)
						tpos = copystep(tpos,f_commaspace);
#if 0
					else if (hlevel < runlevel && hlevel < sprlevel)	{	// if not running on
						tpos = copystep(tpos,efp->field[lastlevel].trailtext);	// set trailing text for last-emitted field
						tpos = copystep(tpos,f_newline);		// set newline before start of this field
					}
					else 	// punctuation required before adding next level as run-on
						tpos = copystep(tpos,*efp->field[lastlevel].trailtext ? efp->field[lastlevel].trailtext : f_colonspace);
#else
					else if (hlevel >= runlevel || hlevel >= sprlevel)	/* if run-on from one in earlier record */
						tpos = copystep(tpos,*efp->field[lastlevel].trailtext ? efp->field[lastlevel].trailtext : f_colonspace);
					else if (!bnacont(scur[hlevel].str)){
						tpos = copystep(tpos,efp->field[lastlevel].trailtext);	// set trailing text for last-emitted field
						tpos = copystep(tpos,f_newline);		// set newline before start of this field
					}
#endif
				}
				else if (!bnacont(scur[hlevel].str)) {	// first field to be formatted; if not BNA special
					if (hlevel >= runlevel || hlevel >= sprlevel)	{	/* if run-on from one in earlier record */
						if ((hlevel != sprlevel || specialrunon))	// if not special run-on, or it needs formatted prefix
							tpos = copystep(tpos,str_crosscheck(FF,scur[hlevel].str) ? f_periodspace : (puncttype ? f_semispace : f_colonspace));
						else
							mark = tpos;	// reset discard base so that we don't discard lead to first emitted field (even if empty)
					}			
				}
				if (hlevel == requiredlevel && hlevel < runlevel)	{	// if required and not yet at runon level
					int findex;
					for (findex = hlevel; findex < FF->head.indexpars.maxfields-2; findex++)
						tpos = copystep(tpos,f_newlevel);		// just insert level shift marker instead of field
				}
				tpos += switchfont(tpos,type_findlocal(FF->head.fm,fp->font,0),ON);		/* change font if necess */
				inscode(&tpos,fp->style.style,0);	/* insert any 'on' code before text */
				setcolor(&tpos,tagcode);
				if (hlevel < runlevel && hlevel < sprlevel && !specialrunon)	// if not running on
					tpos = copystep(tpos,fp->leadtext);		// add any lead text
				epos = form_stripcopy(tpos,scur[hlevel].str);	/* copy current field */
				if (!esp->forcedrun && bnacont(scur[hlevel].str))		/* level for forced run-on */
					esp->forcedrun = FO_FORCERUN;
				if (epos == tpos)	{	/* didn't have any displayable text in the field */
					// change made 9/3/11 to deal with bad indentation of fully suppressed field (Fave R). Side effects?
//					if (hlevel < runlevel && hlevel < sprlevel && lastlevel >= 0) // if not run-on and have emitted something from this record
					if (hlevel < runlevel && hlevel < sprlevel && hlevel > 0) // if not run-on and beyond main head
						tpos = copystep(mark,f_newlevel);		// just insert level shift marker instead of field
					else
						tpos = mark;	// clear everything from start of field
					if (!locatormark)			// if haven't noted first empty field
						locatormark = mark;		// becomes base position for adding page/cross-refs
					continue;			// skip field
				}
				locatormark = NULL;		// reset possible mark
				if (*(epos-1) == FORCEDRUN)	{// if field has special ending to force run-on
					*--epos = '\0';		// overwrite last char to clear (formatting supplied automatically)
					specialrunon = TRUE;
				}
				if (!FF->head.refpars.clocatoronly && (cpos = str_xfindcross(FF,tpos,CSINGLE)))	{	/* if have cross-refs somewhere */
					char * cptr;
					short tokens;

					while (cpos >= tpos+2 && *(cpos-2) == CODECHR && !(*(cpos-1)&FX_OFF))
						cpos -= 2;		/* skip back over leading on style codes */
					strcpy(_cbuff,tpos);		/* make a temporary copy of the field */
					cptr = _cbuff+(cpos-tpos);	/* point to cross-ref in copy */
					cpos += buildcrossprefix(FF,cpos,&cptr,&cs,&tokens);	/* rebuild prefix */
					strcpy(cpos,cptr);		/* copy body */
					cpos += fixcross(FF,cpos,&crcount,FALSE, &cs);	/* in-place conversion */
					esp->crefs += crcount;
					epos = cpos;
				}
				setcaps(tpos,fp->style.cap);
#ifdef GROLIER
				if (!hlevel && (tpos = strchr(tpos, '|')))	{	/* if need an off code */
					if (fp->style.style)	{		/* if there's a style to insert */
						memmove(tpos+1, tpos, epos-tpos);
						inscode(&tpos,fp->style.style, FX_OFF);
						tpos = epos+1;
					}
					else	{			/* just discard the marker */
						memmove(tpos, tpos+1, epos-(tpos+1));
						tpos = epos-1;
					}
				}
				else	{
					tpos = epos;
					clearcolor(&tpos,tagcode);
					inscode(&tpos,fp->style.style,FX_OFF);	/* insert any 'off' code after text */
				}				
#else				
				tpos = epos;
				clearcolor(&tpos,tagcode);
				inscode(&tpos,fp->style.style,FX_OFF);	/* insert any 'off' code after text */
#endif				
				tpos += switchfont(tpos,0,OFF);		/* restore font if necess */
				if (FF->continued)	{		/* building contin heading */
					*tpos++ = SPACE;
					inscode(&tpos,FF->head.formpars.pf.mc.cstyle.style,0);	/* insert any 'off' code after text */
					copystep(tpos,FF->head.formpars.pf.mc.continued);
					setcaps(tpos,FF->head.formpars.pf.mc.cstyle.cap);
					tpos += strlen(tpos);
					inscode(&tpos,FF->head.formpars.pf.mc.cstyle.style,FX_OFF);	/* insert any 'off' code after text */
					if (hlevel >= stoplevel)	/* stop before field before page refs */
						goto noroom;
				}
				lastlevel = hlevel;
			}	/* collapse/suppress */
		}
		if (hlevel < hidelevel)	{	// if want to handle refs
			if (locatormark)	// if have empty field(s) to overwrite
				tpos = locatormark;
			tpos += buildrefs(FF,tpos, &curptr,tbuff+EBUFSIZE-SAFETYMARGIN, &prcount, &crcount, lastlevel >= 0 ? lastlevel : hlevel-1,lastlevel >= 0,&reccount); 	/* add page refs (or cross, if no page) */
			esp->prefs += prcount;
			esp->crefs += crcount;
			esp->consumed += reccount;
			esp->drecs++;		/* count a record */
			puncttype = prcount || hlevel-1;	/* FALSE only when no page refs from main heading (implies cross-ref only) */
		}
		else
			curptr = sort_skip(FF,curptr,1);
		if (curptr) {	/* if have a record */
			curcount = str_xparse(curptr->rtext,scur);
			rec_uniquelevel(FF, curptr, &hlevel,&sprlevel,&hidelevel,&clevel);	/* find level at which heading unique */
		}
	} while (curptr && (hlevel >= runlevel || hlevel >= sprlevel));	/* while headings to be runon */
	noroom:
	*tpos = '\0';		/* guaranteed termination in case of trouble */
	if (FF->stylecount)
		doautostyles(FF,tbuff);
	if (efp->adjstyles)			/* if want styles adjusted around punctuation */
		adjustcodes(FF,tbuff);	/* adjust typecode arrangement */
	if (efp->adjustpunct)		/* if want punct adjusted around quotes, etc */
		transposepunct(tbuff);	/* do it */
	esp->epos = tpos;	/* return pointer to end */
	FF->continued = FALSE;
	{
		UErrorCode error = U_ZERO_ERROR;
		int32_t olength;

		u_strFromUTF8(f_widebuff,sizeof(f_widebuff),&olength,f_entrybuff,-1,&error);
		return f_widebuff;
	}
}
/******************************************************************************/
static short findrunlevel(INDEX * FF, RECORD * recptr, short direction)	/* finds level for running-in */

{
	short tlevel, runlevel, sprlevel,hidelevel,clevel;
	
	if (!(runlevel = FF->head.formpars.ef.runlevel))	/* if unspecified */
		runlevel = PAGEINDEX;		/* i.e., never runon */
	if (FF->head.formpars.ef.style == FL_MODRUNIN)	{
		RECORD * trptr;
		
		trptr = recptr;
		if (direction < 0)			/* if want to start one record back */
			recptr = sort_skip(FF,recptr,-1);
		while (recptr)	{		/* while still have records */
			rec_uniquelevel(FF, recptr, &tlevel,&sprlevel,&hidelevel,&clevel);
			if (tlevel < runlevel)		/* if gone back too far */
				break;
			recptr = sort_skip(FF, recptr,-1);
		}
		if (recptr)	{
			while (recptr = sort_skip(FF, recptr,1))	{
				if (!recptr->isdel)	{
					rec_uniquelevel(FF, recptr, &tlevel, &sprlevel,&hidelevel,&clevel);
					if (tlevel != runlevel && tlevel != PAGEINDEX)	{	/* some level change */
						if (tlevel > runlevel)
							return (tlevel);
						break;
					}
				}
			}
		}
	}
	return (runlevel);
}
/******************************************************************************/
static void doautostyles(INDEX * FF, char * tbuff)	/* inserts auto styles */

{
	int index, len;
	char * mptr, *eptr;
	CSTR sptr;
	
	for (index = FF->stylecount-1; index >= 0; index--)	{	// search backwards so that capture longest match
		sptr = FF->slist[index];
		if (*sptr.str&FX_STYLEMASK)	{	/* if the string has any style codes attached */
			char code = *sptr.str&~FX_OFF;	// remove dummy off
			for (len = sptr.ln-1,mptr = tbuff; mptr = strstr(mptr,sptr.str+1);)		{	/* while a match */
				if ((mptr == tbuff || !u_isalpha(u8_toU(mptr-1)) && *(mptr-1) != KEEPCHR && *(mptr-1) != ESCCHR) && !u_isalpha(u8_toU(mptr+len)))	{	/* if ok for insertion */
					eptr = mptr+len;
					memmove(eptr+4,eptr,strlen(eptr)+1);	/* make gap above string */
					memmove(mptr+2,mptr,len);				/* & gap before string */
					inscode(&mptr,code,0);	
					mptr = eptr+2;
					inscode(&mptr,code,FX_OFF);
				}
				else
					mptr += len;	/* skip length of match */
			}
		}
	}
}
/******************************************************************************/
static void adjustcodes(INDEX * FF, char * tbuff)	/* adjusts positions of style codes */

{
	unsigned char * tpos, cc, *ppos;
	char *pset = "([:;.,";
	short pcount;

	for (tpos = tbuff; tpos = strpbrk(tpos,pset);tpos++)	{	/* for whole string */
		cc = *tpos;
		if (cc == '(' || cc == '[')		{	/* opening parens, etc */
			if (*(tpos+1) == CODECHR && *(tpos+2) == FX_ITAL /* && !(*tpos&FX_FONT) */ || tpos > tbuff+2 && *(tpos-2) == CODECHR && *(tpos-1) == FX_ITAL)	{	/* if on italics abuts opener */
				for (pcount = 1, ppos = tpos+1; *ppos && pcount; ppos++)	{	/* find closer */
					if (*ppos == cc)		/* if more opening */
						pcount++;
					else if (cc == '(' && *ppos == ')' || cc == '[' && *ppos == ']')	{	/* if found a closer */
						if (!--pcount && (*(ppos-2) == CODECHR && *(ppos-1) == (FX_ITAL|FX_OFF) || *(ppos+1) == CODECHR && *(ppos+2)==(FX_ITAL|FX_OFF)))	{	/* if matching closer has codes */
//%%						if (!--pcount && (*(ppos-2) == CODEOCHR && *(ppos-1) == FX_ITAL || *(ppos+1) == CODEOCHR && *(ppos+2)==FX_ITAL))	{	/* if matching closer has codes */
							if (*(tpos+1) == CODECHR && *(tpos+2) == FX_ITAL)	{	/* if opening codes occur after character */
								*tpos = CODECHR;	/* transpose opener */
								*(tpos+1) = FX_ITAL;
								*(tpos+2) = cc;
							}
#if 1
							if (*(ppos-2) == CODECHR && *(ppos-1) == (FX_ITAL|FX_OFF))	{		/* if closer codes in front */
								*(ppos-2) = *ppos;			/* transpose closer */
								*(ppos-1) = CODECHR;
								*ppos = (FX_ITAL|FX_OFF);
							}
#else
							if (*(ppos-2) == CODEOCHR && *(ppos-1) == FX_ITAL)	{		/* if closer codes in front */
								*(ppos-2) = *ppos;			/* transpose closer */
								*(ppos-1) = CODEOCHR;
								*ppos = FX_ITAL;
							}
#endif
							break;	/* fixed this pair */
						}
					}
				}
			}
		}
		else {	/* punct mark */
			char cch = *(tpos-2);
			char tc = *(tpos-1);
//$$			if (*(tpos-2) == CODECHR && ((tc = *(tpos-1))&FX_OFF || tc&FX_FONT && isletterfont(FF,tbuff,tpos-2))) 	{	/* off or font change precedes target char */
//%%			if (cch == CODEOCHR || cch == CODECHR && tc&FX_FONT /* && isletterfont(FF,tbuff,tpos-2) */) 	{	/* off or font change precedes target char */
				if (cch == CODECHR && tc&FX_OFF || cch == FONTCHR /*&& isletterfont(FF,tbuff,tpos-2))*/) 	{	/* off or font change precedes target char */
//				if (tc&FX_FONT || !(tc&(FX_SUPER|FX_SUB|FX_SMALL)))	{	/* if not super/sub/small caps */
				if (cch == FONTCHR || !(tc&(FX_SUPER|FX_SUB|FX_SMALL)))	{	/* if not super/sub/small caps */
					*(tpos-2) = cc;
					*(tpos-1) = cch;
					*tpos = tc;
				}
			}
		}
	}
}
/******************************************************************************/
static void transposepunct(char * base)	/* fixes quote punctuation */

{
	char * sptr = base;
	unichar uc, lc, llc;
	unichar * tpos;
	
	for (llc = lc = 0; (uc = u8_nextU(&sptr)); llc = lc, lc = uc)			{
		if ((tpos = u_strchr(f_openset,uc)) && !u_isalpha(lc) && !iscodechar(llc))	{	// if candidate opener
			unichar cc = f_closeset[tpos-f_openset];		// set target closer
			char * tptr = sptr;		// save position after opener
			
			while ((uc = u8_nextU(&tptr)) && uc != cc)	// while not reached target closer
				lc = uc;
			if (uc == cc)	{		// if found target
				char * bpos = u8_back1(tptr);	// hold position of found char
				unichar tc = u8_nextU(&tptr);	// get char following target
				if (tc)	{	// if not at end of string
					if ((tc == '.' || tc == ',') /* && !u_ispunct(lc) */)	{	/* if period or comma follows, and closer not preceded by punct */
						bpos = u8_appendU(bpos, tc);
						u8_appendU(bpos, cc);
					}
				}
			}
		}
	}
}
/******************************************************************************/
static void inscode(char ** pos, short codes, short type)	/* inserts style code */

{	
	if (codes)	{		/* if code to insert */
		**pos = CODECHR;
		*(*pos+1) = codes|type;	/* type can be 0 (on), or FX_OFF */
		*pos += 2;
	}
}
/******************************************************************************/
static void setcolor(char ** pos, unsigned char color)	// inserts color if needed

{	
	if (color)	{		// if color
		**pos = FONTCHR;
		*(*pos+1) = FX_COLOR|color;	
		*pos += 2;
	}
}
/******************************************************************************/
static void clearcolor(char ** pos, unsigned char color)	// inserts color if needed

{	
	if (color)	{		// if color
		**pos = FONTCHR;
		*(*pos+1) = FX_COLOR;	
		*pos += 2;
	}
}
/******************************************************************************/
static void setcaps(char * base, short type)	/* fixes capitalization */

{
	if (type == FC_INITIAL) {
		unichar uc;

		char * tptr = str_skiptoword(base);
		uc = u8_toU(tptr);
		if (u_islower(uc) && (tptr == base || *(tptr - 1) != KEEPCHR))	// if first testable char is lc
			u8_appendU(tptr, u_toupper(uc));	// assumes uc has same number of bytes as lc
	}
	else if (type == FC_UPPER)
		str_upr(base);
	else if (type == FC_TITLE)	// title case
		str_title(base, f_titleskiplist);
}
/******************************************************************************/
static short switchfont(char * pos, short localid, short mode)	/* inserts font/style code */

{
	static short lastid;
	short count = 0;
	
	if (mode == ON && localid)	{	/* if want to switch out of default */
		*pos++ = FONTCHR;	
		*pos = FX_FONT|localid;		
		lastid = TRUE;
		count += 2;
	}
	else if (mode == OFF && lastid)	{	/* need switch back to default */
		*pos++ = FONTCHR;
//		*pos = FX_FONT|FX_AUTFONT;
		*pos = FX_FONT;
		lastid = FALSE;
		count += 2;
	}
	else			/* didn't insert any font code */
		lastid = 0;
	return (count);
}
/******************************************************************************/
static int buildrefs(INDEX * FF, char * tbuff, RECORD ** xptr, char * epos, short * prcount, short *crcount, short level, short allowcbreak, int * pagerecords)	/* builds page string */

{
	char * tpos, *curpos, *lptr, *eptr;
	short hlevel, ulevel, tokens;
	RECORD * curptr, *firstptr;
	char nplead[FMSTRING];
	static char padstr[] = {FO_RPADCHAR,0};
	char * cptr, *pptr, tagcode;
	int len, cperr, pperr, cprefixlen;
	char cprefixbuff[100];
	struct codeseq cs;

	*crcount = *prcount = cperr = pperr = 0;
	*pagerecords = 0;
	firstptr = curptr = *xptr;
	curpos = str_xlast(curptr->rtext);
	tpos = tbuff;
	cptr = _cbuff;
	pptr = _pbuff;
	*pptr = *cptr = '\0';
	hlevel = level;
	
	do {
		(*pagerecords)++;
		tagcode = curptr->label && g_prefs.gen.showlabel ? FX_COLOR|curptr->label : 0;
		if (str_crosscheck(FF,curpos))		{	/* if crossref */
			if (!FF->head.formpars.ef.cf.suppressall)	{	/* if not suppressing */
				if (!*_cbuff)	/* if first ref */
					cprefixlen = buildcrossprefix(FF,cprefixbuff,&curpos,&cs,&tokens);	/* catch prefix */
				else	{
					curpos = str_skiplist(curpos, FF->head.refpars.crosstart,&tokens);	/* skip prefix */
					while (*curpos == CODECHR && *(curpos+1)&FX_OFF)		/* while we've got trailing style codes */
						curpos += 2;			/* advance over the trailing code */
					/* NB: this loses any styles implicitly carried from prefix into body */
				}
				if (tokens > 1 || hlevel == PAGEINDEX || !FF->head.formpars.ef.collapselevel || FF->head.formpars.ef.collapselevel > hlevel)	{	/* if not a 'See' ref from a collapsed heading */
					len = strlen(curpos);
					if (len) {
						if (cptr - _cbuff + len < CBUFFLEN - 10) {	// if there's content, and enough room
							if (*_cbuff)	/* if not first ref */
								*cptr++ = FF->head.refpars.csep;	/* add a separator */
							setcolor(&cptr, tagcode);
							strcpy(cptr, curpos);		/* add to current string */
							cptr += len;
							clearcolor(&cptr, tagcode);
							*cptr = '\0';
						}
						else
							cperr = TRUE;		/* error */
					}
				}
			}
		}
		else {		/* page refs */
			if (!FF->head.formpars.ef.lf.suppressall)	{
				len = strlen(curpos);
				if (len) {
					if (pptr - _pbuff + len < PBUFFLEN - 10) {	// if there's content, and enough room
						if (*_pbuff)		/* if not first ref */
							*pptr++ = FF->head.refpars.psep;
						setcolor(&pptr, tagcode);
						strcpy(pptr, curpos);		/* add to current string */
						pptr += len;
						clearcolor(&pptr, tagcode);
						*pptr = '\0';
					}
					else
						pperr = TRUE;		/* error */
				}
			}
		}
		if (curptr = sort_skip(FF,curptr,1))	{	/* if have a record */
			short clevel, sprlevel, hidelevel;
			
			curpos = rec_uniquelevel(FF, curptr, &hlevel,&sprlevel,&hidelevel,&clevel);	/* find level at which heading unique */
			ulevel = hlevel;
			if (ulevel < PAGEINDEX && ulevel >= clevel)	{	/* if need to collapse this */
				curpos = str_xlast(curptr->rtext);
				ulevel = PAGEINDEX;
			}
		}
	} while (curptr && ulevel == PAGEINDEX);	/* while more references to accumulate */
	if (!cperr && !pperr)	{		/* if no overflow */
		*xptr = curptr;
		if (*_pbuff)	{	/* if any page refs */
			if (len = fixpage(FF, _pbuff, prcount)) {	/* do them */
				if (FF->head.sortpars.forceleftrightorder) {
					strcpy(tpos, LRMARK);
					tpos += strlen(LRMARK);
				}
				strcpy(nplead, *prcount > 1 ? FF->head.formpars.ef.lf.lleadm : FF->head.formpars.ef.lf.llead1);
				if (FF->head.formpars.ef.lf.rjust && !FF->head.formpars.ef.runlevel)	/* if want right justification on indented */
		    		strcat(nplead,padstr);
				allowcbreak |= CP_HASPAGE;		/* there is page ref, so we can break before cross-ref */
				if (tpos+len < epos)	{		/* if enough room */
					if (!FF->typesetter || !FF->typesetter->suppressrefs)	/* if not typesetting or not suppressing */
						tpos += insertlead(nplead,tpos);
					if (FF->typesetter && !FF->typesetter->individualrefs)	// if need spanning open tag
						*tpos++ = FO_PAGE;
					strcpy(tpos,_pbuff);
					tpos += len;		/* now ready for any cross-ref */
					if (FF->typesetter && !FF->typesetter->individualrefs)	// if need spanning close tag
						*tpos++ = FO_EPAGE;
//					strcpy(tpos, POPISOLATE);
//					tpos += strlen(POPISOLATE);
				}
				else
					goto err;
			}
		}
		if (*_cbuff)	{	/* if any cross-refs */
			if (len = fixcross(FF,_cbuff,crcount,FF->head.formpars.ef.cf.sortcross, &cs))	{	/* do cross refs */
				if (tpos+len < epos)	{		/* if enough room */
					CROSSPUNCT * puptr = &FF->head.formpars.ef.cf.level[level ? 1 : 0];	// get ptr to punct struct
					if (tokens > 1)		{	/* open (see also) ref */
						short position = level ? FF->head.formpars.ef.cf.subposition : FF->head.formpars.ef.cf.mainposition;
						char * breakpos = tpos;

	    				tpos = placecrossref(tpos,FF->head.formpars.ef.runlevel,level,FF->head.formpars.ef.cf.mainposition,FF->head.formpars.ef.cf.subposition,allowcbreak);
						if (tpos == breakpos && allowcbreak || (position != CP_HEADNOPAGE && position != CP_LASTSUBNOSUB))	{ // if not formatting as conditional subhead
							lptr = puptr->cleada;	// use lead 
	    					eptr = puptr->cenda;
						}
						else	// conditional subhead; suppress punctuation
							lptr = eptr = g_nullstr;
	    			}
	    			else	{	/* see ref */
	    				tpos = placecrossref(tpos,FF->head.formpars.ef.runlevel,level,FF->head.formpars.ef.cf.mainseeposition,FF->head.formpars.ef.cf.subseeposition,allowcbreak|=CP_HASPAGE);
						lptr = puptr->cleadb;
	    				eptr = puptr->cendb;
	    			}
					if (!FF->typesetter || !FF->typesetter->suppressrefs)	/* if not typesetting or not suppressing */
						tpos += insertlead(lptr,tpos);	/* add lead punctuation */
					if (FF->typesetter && !FF->typesetter->individualcrossrefs)
						*tpos++ = FO_CROSS;
					if (FF->head.formpars.ef.cf.leadstyle.cap == FC_AUTO)	{	// if want auto cap
						unsigned char * cpptr = cprefixbuff;
						cpptr = str_skipcodes(cpptr);	// and codes
						if (*lptr == '.' || !*lptr)	{	// if first lead char is period, or there's no lead at all
							unichar uc = u8_toU(cpptr);		// convert to upper case regardless
							u8_appendU(cpptr,u_toupper(uc));	// assume upper case same number bytes as lower case
						}
						else if (*(lptr+strlen(lptr)-1) == '(' || *lptr == ',' )	{	// if last lead char is opening paren, or first is comma
							unichar uc = u8_toU(cpptr);		// convert to lower case regardless
							u8_appendU(cpptr,u_tolower(uc));	// assume lower case same number bytes as upper case
						}
					}
					strcpy(tpos,cprefixbuff);		/* add prefix text with codes */
					tpos += cprefixlen;			
					strcpy(tpos,_cbuff);			/* add body */
					tpos += len;
					if (!FF->typesetter || !FF->typesetter->suppressrefs)	{	/* if not typesetting or not suppressing */
						strcpy(tpos,eptr);			/* add trailing punct */
						tpos += strlen(eptr);
					}
					if (FF->typesetter && !FF->typesetter->individualcrossrefs)	// if need spanning close tag
						*tpos++ = FO_ECROSS;	/* add end tag */
				}
				else
					goto err;
			}
		}
	}
	else
err:	
//		senderr(ERR_LONGENTRYERR,WARN,firstptr->num);
		senderronstatusline(ERR_LONGENTRYERR,WARN,firstptr->num);
return (tpos-tbuff);
}
/******************************************************************************/
static char * placecrossref(char * tpos, short runlevel, short level, short mpos, short spos, short enable)	/* sets up cross-ref placement */

{
	/* enable is bit 1 if have displayed heading from this rec, 
			bit 2 if there are page refs from prev identical rec */
	if (enable && (!runlevel || runlevel > level+1))	{
		short pos = level ? spos : mpos;
		if (pos == CP_FIRSTSUB || pos == CP_LASTSUB || pos == CP_HEADNOPAGE && (enable&CP_HASPAGE)) 	/* if want as some kind of subhead */
			*tpos++ = FO_LEVELBREAK;	/* insert break char */
	}
	return tpos;
}
/******************************************************************************/
static short insertlead(char * lptr, char * base)	/* inserts lead before refs */

{
	short tlen;
	char * tbase = base-1;
	
    if (tlen = strlen(lptr))	{	/* length of lead */
		if (*tbase == FO_EPAGE)		/* if an end page code is trailing */
			tbase--;				/* pass over it */
//$$		while (*(tbase-1) == CODECHR)	/* while trailing codes on pre field */
		while (iscodechar(*(tbase-1)))	/* while trailing codes on pre field */
			tbase -= 2;
		if (*lptr == *tbase)	{	/* if first char of lead == last of preceding text (e.g., period) */
    		tlen--;					/* reduce length of lead to be appended */
    		lptr++;					/* skip first character */
		}
		strcpy(base,lptr);		/* insert lead */
	}
	return (tlen);
}
/******************************************************************************/
static short buildcrossprefix(INDEX * FF, char * dest, char ** source, struct codeseq * cs, short *tokens)	/* gather & format lead */

{
	char ccode = FF->head.formpars.ef.cf.leadstyle.style;
	char *curpos, *epos, *cptr, *dptr = dest;
	short alen;
	struct codeseq xs;
	
	curpos = str_skiplist(*source, FF->head.refpars.crosstart, tokens);	/* skip prefix in source */
	while (*curpos == CODECHR && *(curpos+1)&FX_OFF)	// pass over trailing style codes
		curpos += 2;		// skip them
	epos = curpos;		// mark end of trailing style codes
	while (*curpos == CODECHR && *++curpos || *curpos == SPACE)	// pass over opening codes in body text
		curpos++;
	cs->font = cs->style = cs->color = '\0';
	for (xs.style = xs.font = xs.color = '\0', cptr = *source; cptr < curpos; cptr++)	{	// accumulate style to be transferred to body
		if (*cptr == FONTCHR)	{	/* if a font change */
			if (*++cptr&FX_FONT)
				xs.font = *cptr;
			else
				xs.color = *cptr;
			cptr++;
		}
		if (*cptr == CODECHR)	{
			if (*++cptr&FX_OFF)		/* if off */
				xs.style &= ~(*cptr&FX_STYLEMASK);	/* clear the style */
			else
				xs.style |= *cptr;	/* set the style */
			cptr++;
		}
		if (cptr <= epos)	// until end of prefix, build style to carry forward
			*cs = xs;
	}
	if (FF->head.formpars.ef.cf.suppressifbodystyle)	// if want to suppress styles that match body
		ccode  ^= (xs.style&ccode);		// clear any auto styles that match those at start of body
	if (**source == CODECHR && !(*(*source+1)&FX_OFF))	// if prefix starts with style code */
		ccode |= *(*source+1);	// pick up code and add to auto style
	if (ccode)
		inscode(&dptr,ccode,0);		// insert 'on' code before prefix
	str_textcpylimit(dptr, *source,epos);	// copy, stripping codes
	if (*tokens == 2)	{	/* check if really 'see also' (might be see under) */
		char *tsptr, *teptr;
		if (tsptr = strchr(FF->head.refpars.crosstart,SPACE))	{
			while (*tsptr == SPACE)
				tsptr++;
			for (teptr = tsptr; u_isalnum(u8_toU(teptr)); teptr = u8_forward1(teptr))
				;
			if (!str_xfind(dest,tsptr,CSINGLE,teptr-tsptr,&alen))	/* if not see also */
				*tokens = 1;		/* assume 'see under' -- treat like 'See' */
		}
	}
	setcaps(dptr,FF->head.formpars.ef.cf.leadstyle.cap);	/* do any caps on prefix */
	dptr += strlen(dptr);
	while(*(dptr-1) == SPACE)	// remove terminal spaces
		dptr--;
	if (ccode)	{			// if inserted a start code
		*dptr++ = CODECHR;	// turn off
		*dptr++ = ccode|FX_OFF;
	}
	*dptr++ = SPACE;	// add terminal space
	*dptr = '\0';		// terminate string
	*source = epos;	/* leave source pointer at right place */
	return (dptr-dest);
}
/******************************************************************************/
static short parserefs(char * tempbuff, char * pptr, char ** ptrarray, char sepchar, struct codeseq * cs)	/* organizes reference string */

{
	char * xptr, *limit, *nptr;
	int count, ccount;
	
	for (xptr = tempbuff, count = 0, limit = tempbuff+PBUFFLEN-9; *pptr && count < PARRAYSIZ && xptr < limit; )	{	/* copy string, cleaning up */
		while (*pptr == SPACE || iscodechar(*pptr) || *pptr == sepchar)	{	/* while before ref proper */
			char c = *pptr++;
			if (c == FONTCHR)	{	// capture most recent font/color
				if (*pptr&FX_COLOR)
					cs->color = *pptr++;
				else
					cs->font = *pptr++;
			}
			else if (c == CODECHR)	{	/* accumulate any codes */
				if (*pptr&FX_OFF)		/* if off code */
					cs->style &= ~(*pptr&FX_STYLEMASK);	
				else						/* on code */
					cs->style |= *pptr;
				pptr++;
			}
		}
		ptrarray[count] = xptr;
		if (cs->font)	{		/* if carrying font into ref */
			*xptr++ = FONTCHR;
			*xptr++ = cs->font;	/* set font */
		}
		if (cs->color)	{		/* if carrying color into ref */
			*xptr++ = FONTCHR;
			*xptr++ = cs->color;	/* set font */
		}
		if (cs->style)	{		/* if carrying an attribute into the ref */
			*xptr++ = CODECHR;
			*xptr++ = cs->style;
		}
		for (ccount = 0, nptr = ref_next(pptr,sepchar); *pptr && (!nptr || pptr < nptr) && xptr < limit; ccount++)	{	/* copy ref */
			char c = *xptr++ = *pptr++;
			if (c == FONTCHR)	{	/* accumulate any codes */
				if (*pptr&FX_COLOR)
					cs->color = *pptr;
				else
					cs->font = *pptr;
				ccount -= 2;			/* reduce real character count */
			}
			else if (c == CODECHR)	{
				if (*pptr&FX_OFF)		/* if off code */
					cs->style &= ~(*pptr&FX_STYLEMASK);	
				else						/* on code */
					cs->style |= *pptr;
				ccount -= 2;			/* reduce real character count */
			}
		}
		if (ccount)	{	/* if have copied a ref (stuff other than codes) */
			if (cs->style)	{
				*xptr++ = CODECHR;
				*xptr++ = cs->style|FX_OFF;
			}
			if (cs->color&FX_COLORMASK)	{	/* if a color to turn off */
				*xptr++ = FONTCHR;
				*xptr++ = cs->color&~FX_COLORMASK;	/* default color */
			}
			if (cs->font&FX_FONTMASK)	{	/* if a font to turn off */
				*xptr++ = FONTCHR;
				*xptr++ = cs->font&~FX_FONTMASK;	/* default font  */
			}
			count++;			/* count it */
			*xptr++ = '\0';		/* terminate it */
		}
	}
	if (*pptr)	/* if overflow */
//		senderr(ERR_TOOMANYREFS,WARN,f_entrybuff);
		senderronstatusline(ERR_TOOMANYREFS,WARN,f_entrybuff);
	return (count);
}
/******************************************************************************/
static int fixpage(INDEX * FF, char * pgbuf, short *rcount)	/* organizes page ref string */

{
	char *j, *p1, *tj, *repptr;;
	char *ptrarray[PARRAYSIZ], *p2, *cpos;
	char *lastj, **i, **ti;
	size_t len, len1; 
	short count, tcount,ccount;
	long val;
	LOCATORFORMAT * lfp = &FF->head.formpars.ef.lf; 
	struct codeseq cs;
	int overwritelength = 2;		// length of separator text overwritten when suppressing repeated parts

	cs.font = cs.style = cs.color = '\0';
	*rcount = count = parserefs(_trefbuff, pgbuf, ptrarray, FF->head.refpars.psep,&cs);
	
	if (FF->typesetter)	{	// if typesetting, adjust overwritelength used in suppressing repeated parts
		if (FF->typesetter->individualrefs && !FF->typesetter->suppressrefs)	// tagging individual refs, and allowing lead / end strings
			overwritelength = 4;	// end tag code + space, + page ref sep + start tag code
		// if tagging individual refs *and* suppressing lead / end strings, overwritelength is 2, because only end tag code and start tag code are emitted
	}
	if (FF->head.formpars.ef.lf.sortrefs)		/* if want ordered */
		qsort(ptrarray, count, sizeof(char *), pcompare);  /* sort substrings */
	for (i = ptrarray, j = pgbuf, repptr = _rrefbuff, lastj = j; count--; i++) {   /* while there are entries in sort table */
		if (count && (lfp->noduplicates || lfp->conflate) && FF->head.sortpars.ascendingorder) {		// check / remove overlaps (always remove them if conflating), but (6/4/21) only if refs in ascending order
			// conflate() doesn't do in-place fix
			strcpy(repptr, *i);
			if (conflate(repptr, *(i + 1), FF->head.refpars.rsep, TRUE)) {	// if suppressed an overlap
				FF->overlappedrefs++;
				*(i + 1) = repptr;		// pointer to second ref now contains replacement ref
				repptr += strlen(repptr) + 1;	// advance position in replacement buffer, ready for next replacement
				(*rcount)--;
				continue;	// will skip the first ref
			}
		}
		if (FF->typesetter && FF->typesetter->individualrefs)
			*j++ = FO_PAGE;
		form_stripcopy(j, *i);			/* copy string back in sort order */
			
		if (lfp->conflate && FF->head.sortpars.ascendingorder)	 {			/* if want to conflate (6/4/21 only for refs in ascending order) */
			for (ccount = 0,tcount = count, ti = i, tj = j; count && conflate(j, *(i+1), FF->head.refpars.rsep,FALSE);)   {  /* while there are fields to check */
				ccount++;		/* one conflated */
				if (strchr(*i,FF->head.refpars.rsep))	/* if a range */
					ccount++;			/* count one extra */
				count--;
				i++;
			}
			if (ccount >= lfp->conflate)	// if conflation reached threshold
				*rcount -= tcount-count;	// adjust ref count
			else	{		// restore old pointers and count
				count = tcount;	
				i = ti;
				j = tj;
				form_stripcopy(j,*i);  	/* restore original string */
			}
		}
		setpagestyle(FF, j);		/* apply auto style */
		/* suppress leading parts of reference by template */

		if (lfp->suppressparts && strlen(lfp->suppress))	{		/* if want to suppress common beginnings (and have a string to suppress to. 5/14/2017) */
			struct codeseq tcs;
			
			if ((p1 = strchr(j, FF->head.refpars.rsep)) && (p2 = suppressmatch(j, ++p1,lfp->suppress))) 	/* if ref has second seg that can be suppressed */
				strcpy(p1, p2); 			/* copy over the suppressed part */
			if (j > lastj && (p2 = suppressmatch(lastj,j,lfp->suppress))) {	/* if already suppressed some && start of ref can be suppressed */
				p2 = type_pickcodes(j,p2,&tcs);	// pick codes continuing out of suppressed part
				j = strcpy(j-overwritelength,lfp->concatenate);	// replace preceding separators, etc. with suppression string
				j += strlen(j);
				j = type_dropcodes(j,&tcs);	// drop any continuing codes
				strcpy(j,p2);		// add unsuppressed parts of this ref
//				j += strlen(p2);	// set ptr beyond end
			}
			else
				lastj = j;		/* insist on new one */
		}

		/* now abbrev strings */

		if (lfp->abbrevrule && (cpos = strchr(j, FF->head.refpars.rsep)))	{	 /* if want numbers abbreviated  */
			char * p1Suffix, *p2Suffix, * sepPos;		// changes to handle suffixes 4/29/2017
				
			sepPos = cpos;		// mark position of separator
			for (p1 = cpos++; p1 > j+2 && iscodechar(*(p1-2)); p1 -= 2)	/* skip trailing codes at end of first range */
				;
			for (p1Suffix = p1; p1Suffix > j+2 && !isdigit(*(p1Suffix-1)); p1Suffix--)		// find any suffix to first part
				;
			p1 = p1Suffix;	// p1 starts at char beyond first digit (start of suffix, or separator if no suffix)
			while (isdigit(*--p1) && p1 >= j)		/* working backwards while in digits */
				;
			while (iscodechar(*cpos))	/* skip any leading codes in second part of range */
				cpos += 2;
			p2 = cpos;
			for (p2Suffix = p2+strspn(p2,r_nlist); p1Suffix < sepPos && *p2Suffix == *p1Suffix; p1Suffix++, p2Suffix++)
				;
			if ((len1 = len = strspn(p2,r_nlist)) == strspn(++p1,r_nlist) && p1Suffix == sepPos)	{	// if ranges same length and first suffix matches second up to separator
				if (lfp->abbrevrule == FAB_CHICAGO)	{	/* if Chicago rules (13th edn) */
					val = (unsigned)atol(p1);   /* get first val */
					if (val > 100 && val%100)	{	/* if >= 3 digits and not multiple of 100 */
						for (; *p2 == *p1 && (len > 2 || *p2 == '0'); p2++, p1++)	/* now skip as required */
							len--;	   /* count down */
						if (len1 != len && !(len1 == 4 && len==3))	/* if want to truncate and not a special 4-digit # */
							memmove(cpos,p2,strlen(p2)+1);		/* shift string downwards */
					}
				}
				else if (lfp->abbrevrule == FAB_HART) {		/* must be Hart's rules (39 ed, p 19) */
					for (; *p2 == *p1 && len > 1 && (len != 2 || *p2 != '1'); p2++, p1++)	/* now skip as required */
						len--;	   /* count down */
					if (len1 != len)	/* if want to truncate */
						memmove(cpos,p2,strlen(p2)+1);		/* shift string downwards */
				}
				else {		/* must be full abbrev (remove all shared numbers, as per Chicago 14 edn 8.70) */
					for (; *p2 == *p1 && len > 1; p2++, p1++)	/* now skip as required */
						len--;	   /* count down */
					if (len1 != len)	/* if want to truncate */
						memmove(cpos,p2,strlen(p2)+1);		/* shift string downwards */
				}
			}
		}
		if (count)	{
			j += strlen(j);
			if (FF->typesetter && FF->typesetter->individualrefs)
				*j++ = FO_EPAGE;
			if (!FF->typesetter || !FF->typesetter->individualrefs || !FF->typesetter->suppressrefs)	{
				*j++ = FF->head.refpars.psep;		/* restore separator */
				*j++ = SPACE;
			}
		}
	}
	len = strlen(lfp->connect);
	for (j = pgbuf; (j = strchr(j,FF->head.refpars.rsep)); j += len)	{	/* for all page refs */
		if (len)	{		/* if need to replace */
			if (len != 1)	/* if string <> token */
				memmove(j+len,j+1,strlen(j));		/* fix space */
			strncpy(j,lfp->connect,len);
		}
		else    /* skip over connector */
			j++;
		if (!FF->singlerefcount)	// if a range counts as 1
			(*rcount)++;		/* adjust ref count for range (each range == 2 refs) */
	}
	if (FF->typesetter && FF->typesetter->individualrefs)
		strcat(pgbuf,f_pgc);		// add page tag end as string
	if (*lfp->trail && (!FF->typesetter || !FF->typesetter->suppressrefs))		/* if want trailing punctuation */
		strcat(pgbuf, lfp->trail);
	str_extend(pgbuf);
	return (str_adjustcodes(pgbuf,CC_INPLACE)-1);	/* adjust codes, return # of chars in buffer */
}
/******************************************************************************/
static int pcompare (const void * s1, const void * s2)       /* compares page refs for qsort */

{
	short result;
	
	if (!(result = ref_match(s_index,*(char **)s1, *(char **)s2, s_sg->partorder, PMEXACT|PMSENSE|PMSTYLE)))	/* if match */
		result = strlen(*(char **)s1)-strlen(*(char **)s2);	/* order by length  (copes with type codes) */
	return (result);
}
/******************************************************************************/
static int fixcross(INDEX * FF, char * pgbuf, short *rcount, short sflag, struct codeseq *cs)	/* organizes cross-ref string */

{
	char *j, **i;
	char *ptrarray[PARRAYSIZ];
	short count, skip, tokens;

	*rcount = count = parserefs(_trefbuff, pgbuf, ptrarray, FF->head.refpars.csep,cs);
	j = pgbuf;
	if (*pgbuf)	{		/* if actually any ref following lead */
		if (sflag)		/* if need to sort */
			qsort(ptrarray, count, sizeof(char *), tcompare);  /* sort substrings */
		for (i = ptrarray; count--; i++) {   /* while there are entries in sort table */
			if (!count || strcmp(*i, *(i+1)))	{	/* if not succeeding identical refs */
				skip = str_skiplist(*i, FF->head.refpars.crossexclude, &tokens) != *i;	/* find if general cross-ref */
				if (!skip)		/* if not general */
					inscode(&j,FF->head.formpars.ef.cf.bodystyle.style,0);	/* insert 'on' code before lead */
				if (FF->typesetter && FF->typesetter->individualcrossrefs)
					*j++ = FO_CROSS;
				form_stripcopy(j, *i);
				if (!skip)
					setcaps(j,FF->head.formpars.ef.cf.bodystyle.cap);	/* set caps if needed */
				j += strlen(j);
				if (!skip)	/* if not general */
					inscode(&j,FF->head.formpars.ef.cf.bodystyle.style,FX_OFF);	/* insert 'off' code */
				if (count)	{	/* if not the last ref */
					if (FF->typesetter && FF->typesetter->individualcrossrefs)
						*j++ = FO_ECROSS;
					if (!FF->typesetter || !FF->typesetter->individualcrossrefs || !FF->typesetter->suppressrefs)	{
						*j++ = FF->head.refpars.csep;
						*j++ = SPACE;
					}
				}
			}
		}
		if (FF->typesetter && FF->typesetter->individualcrossrefs)
			*j++ = FO_ECROSS;
		*j = '\0';
	}		
	return (j-pgbuf);
}
/******************************************************************************/
static int tcompare(const void * s1, const void * s2)	/* text compare for qsort */

{
	return (sort_crosscompare(s_index,s_sg,*(char **)s1, *(char **)s2));
}
/******************************************************************************/
char *form_stripcopy(char *dptr, char *sptr)	 /* copies, skipping over braced text */

{
	char *tptr;

	while ((*dptr = *sptr++))	{	/* copy chars */
		if (*dptr == ESCCHR || *dptr == KEEPCHR)	{
			if (*sptr)	{	/* if something follows */
				tptr = str_skipcodes(sptr);
				if (tptr > sptr)	{	/* if we've got code seq in wrong place */
					char tchar;
					tchar = *dptr;
					memcpy(dptr,sptr,tptr-sptr);
					dptr += tptr-sptr;
					*dptr = tchar;
					sptr = tptr;
				}	
				dptr++;	/* advance over special char */
				*dptr++ = *sptr++;	/* pass the next char so it can't be tested */
			}
			else		/* has dangling ESCCHR || KEEPCHR */
				*dptr = '\0';
			continue;
		}
		if (*dptr == OBRACKET || *dptr == CBRACKET)
			continue;				/* drop this character */
		if (*dptr++ == OBRACE)	{	/* if opening brace */
			dptr--;		/* reset to position of brace */
			while (tptr = strchr(sptr,CBRACE))	 {
				sptr = tptr+1;
				if (*--tptr != KEEPCHR && (*tptr != ESCCHR || *--tptr == ESCCHR)) /* if not escaped closing brace */
					break;		  /* O.K. return at char beyond */
				
			}
			if (!tptr)			/* if unterminated */
				sptr += strlen(sptr);	/* no closing brace; skip to end of field */
		}
	}			
	return (dptr);
}
/*******************************************************************************/
static char * copystep(char * str1, char *str2)		/* adds string (with protection), returns ptr to end */

{
	while (*str2)	{
		if (*str2 == ESCCHR || *str2 == KEEPCHR)
			*str1++ = ESCCHR;
		*str1++ = *str2++;
	}
	*str1 = '\0';
	return (str1);
}
#if 0
/******************************************************************************/
static short findindent(INDEX * FF, short indent)     /* returns indent for this field */

{
	ENTRYFORMAT * efp;
	short level, runflag;
	float points;
	
	efp = &FF->head.formpars.ef;
	if (efp->itype && !(indent&FO_AHEAD))	{	/* if want any indent */
		level = indent&FO_HMASK;
		runflag = indent&FO_RFLAG;
//		if (efp->itype == FI_AUTO || efp->itype == FI_SPECIAL && level < L_SPECIAL)	{	/* auto */
		if (efp->itype == FI_AUTO || efp->itype == FI_SPECIAL && level < FF->head.indexpars.maxfields-2)	{	/* auto */
			points = level*efp->autolead;	/* indent (points) */
			if (runflag)			/* if runover */
				points += efp->autorun;
			if (!efp->autounit)		/* if unit is ems */
				points *= LX(FF->vwind,emwidth);
		}
		else 	{	/* fixed, or special */
			if (FF->head.formpars.ef.itype == FI_SPECIAL && level == FF->head.indexpars.maxfields-2)
				level = L_SPECIAL;
			points = runflag ? efp->field[level].runindent : efp->field[level].leadindent;
			if (!efp->fixedunit)		/* if unit is ems */
				points *= LX(FF->vwind,emwidth);
		}
	}
	else
		points = 0;
	return ((short)points);                       
}
#else
/******************************************************************************/
static short findindent(INDEX * FF, short indent)     /* returns indent for this field */

{
	ENTRYFORMAT * efp;
	short level, runflag;
	float points;
	
	efp = &FF->head.formpars.ef;
	if (efp->itype)	{	/* if want any indent */
		level = indent&FO_HMASK;
		runflag = indent&FO_RFLAG;
//		if (efp->itype == FI_AUTO || efp->itype == FI_SPECIAL && level < L_SPECIAL)	{	/* auto */
		if (efp->itype == FI_AUTO || efp->itype == FI_SPECIAL && level < FF->head.indexpars.maxfields-2)	{	/* auto */
			points = level*efp->autolead;	/* indent (points) */
			if (runflag)			/* if runover */
				points += efp->autorun;
			if (!efp->autounit)		/* if unit is ems */
				points *= LX(FF->vwind,emwidth);
		}
		else 	{	/* fixed, or special */
			if (FF->head.formpars.ef.itype == FI_SPECIAL && level == FF->head.indexpars.maxfields-2)
				level = L_SPECIAL;
			points = runflag ? efp->field[level].runindent : efp->field[level].leadindent;
			if (!efp->fixedunit)		/* if unit is ems */
				points *= LX(FF->vwind, emwidth);
		}
	}
	else
		points = 0;
	return ((short)points);                       
}
#endif
/******************************************************************************/
static char *suppressmatch(char *s1, char *s2, char * suppress)	/* finds if refs match through suppression string */

{
	char *cpos;

	for (cpos = suppress; *s1 == *s2 && *cpos; s1++, s2++)	 {	/* match and not at limit */
//$$		if (*s1 == CODECHR)	{
		if (iscodechar(*s1))	{
			s1++;
			s2++;
		}
		else if (*s1 == *cpos)	/* if a field delimiter */
			cpos++;
	}
	if (*s1 == CODECHR && *s2 == CODECHR && *(s1+1)&FX_OFF && *(s2+1)&FX_OFF)	/* skip any codes after match */
		s2 += 2;
	return (*cpos ? NULL : s2);/* return beyond match in second string */
}
/******************************************************************************/
static char * setpagestyle(INDEX * FF, char *sptr)	/* styles page refs */

{
	char *tptr;
	short index, scount;
	unichar uc;
	
	/* one day we might improve this to check, at point where we insert a code, whether 
	a code already exists that we can use or augment */

	for (index = 0; index < COMPMAX && *sptr;)	{	/* for all characters */
		if (iscodechar(*sptr))	/* if code sequence */
			sptr += 2;			/* skip it */
		else if (*sptr == FF->head.refpars.rsep)	{	/* if range connector */
			sptr++;				/* skip it */
			for (scount = 0, tptr = sptr; *tptr;)	{	/* count segments in second part */
				uc = u8_nextU(&tptr);
				if (u_isalpha(uc))	{
					scount++;
					while (*tptr && u_isalpha(u8_toU(tptr)))
						tptr = u8_forward1(tptr);
				}
				if (u_isdigit(uc))	{
					scount++;
					while (*tptr && u_isdigit(u8_toU(tptr)))
						tptr = u8_forward1(tptr);
				}
				while (*tptr && !u_isalnum(u8_toU(tptr)))
					tptr = u8_forward1(tptr);
			}
			if (scount < index)		/* if fewer segs in second part */
				index = index-scount;	/* assume initial segments suppressed */
			else
				index = 0;
		}
		else if (!u_isalnum(u8_toU(sptr))) {		/* punctuation of some sort */
			if (FF->head.formpars.ef.lf.lstyle[index].punct.style)	{
				memmove(sptr+2, sptr, strlen(sptr)+1);
				inscode(&sptr, FF->head.formpars.ef.lf.lstyle[index].punct.style,0);
			}
			while (*sptr && !iscodechar(*sptr) && *sptr != FF->head.refpars.rsep && !u_isalnum(u8_toU(sptr)))	/* while some lead */
				sptr = u8_forward1(sptr);
			if (FF->head.formpars.ef.lf.lstyle[index].punct.style)	{
				memmove(sptr+2, sptr, strlen(sptr)+1);
				inscode(&sptr, FF->head.formpars.ef.lf.lstyle[index].punct.style,FX_OFF);
			}
		}
		else	{	/* must be a locator segment */
			if (FF->head.formpars.ef.lf.lstyle[index].loc.style)	{
				memmove(sptr+2, sptr, strlen(sptr)+1);
				inscode(&sptr, FF->head.formpars.ef.lf.lstyle[index].loc.style,0);
			}
			if (u_isalpha(u8_toU(sptr)))	{
				do {
					uc = u8_nextU(&sptr);
				} while (u_isalpha(uc));
				sptr = u8_back1(sptr);
			}
			else while (u_isdigit(u8_toU(sptr)))
				sptr = u8_forward1(sptr);
			if (FF->head.formpars.ef.lf.lstyle[index].loc.style)	{
				memmove(sptr+2, sptr, strlen(sptr)+1);
				inscode(&sptr, FF->head.formpars.ef.lf.lstyle[index].loc.style,FX_OFF);
			}
			index++;
		}
	}
	return (sptr);
}
#if 0
/******************************************************************************/
static BOOL conflate(char *dest, char *source, char connect, BOOL overlap)		/* conflates refs */

{
	unsigned char *p1, *p2, *e1, *e2, *base1, *base2, *p0;
	unsigned long val0 = 0, val1 = 0, val2 = 0, val3 = 0;

	e1 = dest + strlen(dest);			// end of first ref
	e2 = source + strlen(source);		// end of second ref
	while (e1 > dest + 1 && iscodechar(*(e1 - 2))) {	// check trailing codes
		if (e2 > source + 1 && iscodechar(*(e2 - 2)) && *--e1 == *--e2) {	// if match, skip back
			e1--;
			e2--;
		}
		else		// mismatch, so fail
			return FALSE;
	}
	p1 = e1;		// end of ref, before any trailing codes
	if (base1 = strchr(dest, connect)) {	/* if first ref has second segment */
											/* this little bit gets val of first seg of first ref */
		for (p0 = base1; !isdigit(*--p0) && p0 >dest;)		/* skip non-numerical suffix to first seg */
			;
		while (isdigit(*--p0) && p0 >= dest)	   /* move back through digits */
			;
		val0 = atol(++p0);		/* get value of first seg */
		base1++;				/* set base to second seg */
	}
	else {
		val0 = 0;			/* no initial segment to worry about (assume refs already ordered) */
		base1 = dest;		/* set base to start */
	}
	if (base2 = strchr(source, connect)) {		/* if second ref has second seg */
		for (p2 = base2 + strlen(base2); !isdigit(*--p2) && p2 >base2;)		/* skip non-numerical suffix to second seg */
			;
		while (isdigit(*--p2) && p2 >= base2)	   /* move back through digits */
			;
		val3 = atol(++p2);		/* get value of second seg */
		p2 = base2; 			/* set mark to end of first */
	}
	else
		p2 = e2;			/* set to end of whole ref */
	while (*--p1 == *--p2 && !isdigit(*p1))		/* while non-numerical suffixes match */
		;
	if (!isdigit(*p1) || !isdigit(*p2))	/* if first has suffix that differs from second */
		return (FALSE);
	while (isdigit(*--p1) && p1 >= dest)
		;
	while (isdigit(*--p2))
		;
	p1++;
	p2++;		 /* ptrs sit at beginning of last numerical seg	*/

	int refgap = overlap ? 0 : 1;	// threshold distance between highest component of first ref and lowest component of second

	if (((base1 == dest || p1 > base1) && (p1 - base1 != p2 - source || strncmp(base1, source, p1 - base1)))
		|| p1 == base1 && strncmp(dest, source, p2 - source) || (val2 = atol(p2)) > (val1 = atol(p1)) + refgap
		|| val2 < val0)
		return (FALSE);
	/* if (only 1 seg in first ref || second seg has part before number) && early parts differ
	|| second seg of first ref is abbreviated and first doesn't match second ref up to corresponding part
	|| second ref > first +1 || second ref < first part of first */

	//	NSLog(@"Comparing: %s[%ld], %s[%ld]",dest,val1,source,val2);
	//	if (base2 && (val2 <= val1) || !base2 && val1 >= val2) {
	//		NSLog(@"Overlap: %s, %s",dest, source);
	//	}

	/* if (second ref has 2 segs && (second is > higher part of first ref || < ^^lower in second)) || second has one that isn't wholly contained in first */
	// ^^ this test is kludge to allow cases where multi-part second refs have numerical suffix, e.g., 140, 141-144t7.1
	if (base2 && (val3 > val1 || val3 < val2) || !base2 && (overlap ? val1 < val2 : val1 <= val2)) {
		if (base1 == dest)	/* if only one part to first ref */
			*e1++ = connect;
		else
			e1 = base1;
		//		NSLog(@"[%d] %s -< %s",overlap,e1, base2 ? base2+1 : source);
		strcpy(e1, base2 ? base2 + 1 : source);		/* copy second part if any, otherwise first */
	}
	return (TRUE);
}
#else
/******************************************************************************/
static BOOL conflate(char *dest, char *source, char connect, BOOL overlap)		/* conflates refs */

{
	unsigned char *p0, *p1, *p2, *p3, *e1, *e2, *base1, *base2, *px;
	unsigned long val0 = 0, val1 = 0, val2 = 0, val3 = 0;

#if 1		// 8 Apr 2020
	if (overlap && !strcmp(dest, source))		// if identical refs to be suppressed
		return TRUE;
#endif
	e1 = dest + strlen(dest);			// end of first ref
	e2 = source + strlen(source);		// end of second ref
	while (e1 > dest + 1 && iscodechar(*(e1 - 2))) {	// check trailing codes
		if (e2 > source + 1 && iscodechar(*(e2 - 2)) && *--e1 == *--e2) {	// if match, skip back
			e1--;
			e2--;
		}
		else		// mismatch, so fail
			return FALSE;
	}
	p1 = e1;		// end of ref, before any trailing codes
	if (base1 = strchr(dest, connect)) {	/* if first ref has second segment */
		/* this little bit gets val of first seg of first ref */
		px = base1;	// end of second segment
		for (p0 = base1; !isdigit(*--p0) && p0 > dest;)		/* skip non-numerical suffix to first seg */
			;
		while (isdigit(*--p0) && p0 >= dest)	   /* move back through digits */
			;
		val0 = atol(++p0);		/* get value of first seg */
		base1++;				/* set base to second seg */
	}
	else {
		val0 = 0;			/* no initial segment to worry about (assume refs already ordered) */
		base1 = dest;		/* set base to start */
		px = p1;
	}
	if (base2 = strchr(source, connect)) {		/* if second ref has second seg */
		p3 = e2;	// end of second segment
		for (p2 = base2 + strlen(base2); !isdigit(*--p2) && p2 > base2;)		/* skip non-numerical suffix to second seg */
			;
		while (isdigit(*--p2) && p2 >= base2)	   /* move back through digits */
			;
		val3 = atol(++p2);		/* get value of second seg */
		p2 = base2; 			/* set mark to end of first */
	}
	else {
		p2 = e2;			/* set to end of whole ref */
		p3 = p2;
	}
	// px now set to end of suffix to first segment of first ref
	// p1 now set to end of suffix to first (or, if present, second) segment of first ref
	// p2 now set to end of suffix to first segment of second ref
	// p3 now set to end of suffix to second seg of second ref (or same as p2 if none)
	// px is same as p1 if no second seg to first ref; p3 is same as p2 if no second seg to second ref
	do {	// pass back through all suffixes while they're identical
		p1--;
		p2--;
		p3--;
		px--;
	} while (*p1 == *p2 && *p1 == *p3 && *px == *p1 && !isdigit(*p1));
	if (!isdigit(*p1) || !isdigit(*p2) || !isdigit(*p3) || !isdigit(*px))	// if any relevant suffixes differ
		return (FALSE);
	while (isdigit(*--p1) && p1 >= dest)
		;
	while (isdigit(*--p2))
		;
	p1++;
	p2++;		 /* ptrs sit at beginning of last numerical seg	*/

	int refgap = overlap ? 0 : 1;	// threshold distance between highest component of first ref and lowest component of second

	if (((base1 == dest || p1 > base1) && (p1 - base1 != p2 - source || strncmp(base1, source, p1 - base1)))
		|| p1 == base1 && strncmp(dest, source, p2 - source) || (val2 = atol(p2)) > (val1 = atol(p1)) + refgap
		|| val2 < val0)
		return (FALSE);
	/* if (only 1 seg in first ref || second seg has part before number) && early parts differ
	 || second seg of first ref is abbreviated and first doesn't match second ref up to corresponding part
	 || second ref > first +1 || second ref < first part of first */

//	NSLog(@"Comparing: %s[%ld], %s[%ld]",dest,val1,source,val2);
//	if (base2 && (val2 <= val1) || !base2 && val1 >= val2) {
//		NSLog(@"Overlap: %s, %s",dest, source);
//	}

	 /* if (second ref has 2 segs && (second is > higher part of first ref || < ^^lower in second)) || second has one that isn't wholly contained in first */
	 // ^^ this test is kludge to allow cases where multi-part second refs have numerical suffix, e.g., 140, 141-144t7.1
	if (base2 && (val3 > val1 || val3 < val2) || !base2 && (overlap ? val1 < val2 : val1 <= val2)) {
		if (base1 == dest)	/* if only one part to first ref */
			*e1++ = connect;
		else
			e1 = base1;
//		NSLog(@"[%d] %s -< %s",overlap,e1, base2 ? base2+1 : source);
		strcpy(e1, base2 ? base2 + 1 : source);		/* copy second part if any, otherwise first */
	}
	return (TRUE);
}
#endif
