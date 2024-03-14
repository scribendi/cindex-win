#include "stdafx.h"
#include "errors.h"
#include "viewset.h"
#include "draftstuff.h"
#include "formstuff.h"
#include "records.h"
#include "sort.h"
#include "search.h"
#include "strings.h"
#include "collate.h"
#include "util.h"

static TCHAR lrMark[1];

static void putline(HDC dc, TCHAR * source, ATTRIBUTES * as, TCHAR * eptr, short visible, short free);      /* return first printable char */
static TCHAR * measurewidth(HDC dc, TCHAR * base, TCHAR * source, ATTRIBUTES *as, long limitwidth, long *freespace, struct selstruct * slptr, short lcount);       /*finds posn of last printable char within width */
static TCHAR * buildentry(INDEX * FF, RECORD * recptr, short * ulevel, int *baseoffset);		/* forms entry for record */
static short makelead(TCHAR * lptr, INDEX * FF, RECORD * recptr);		/* makes lead */

/**********************************************************************/
TCHAR * draft_build(INDEX * FF, RECORD * recptr, short * hlevel)	/* returns text of formed entry */

{
	int baseoffset;

	return (buildentry(FF,recptr, hlevel,&baseoffset));	/* form entry */
}
/**********************************************************************/
RECORD * draft_skip(INDEX * FF, RECORD * recptr, short dir)	/* skips in draft display mode */

{
	if (recptr = sort_skip(FF,recptr,dir))	{	/* if have a record */
		if (FF->head.privpars.hidebelow != ALLFIELDS || FF->sumsource)	{	/* if some restriction on display */
			short ulevel;
			int baseoffset;
			while (recptr && !*buildentry(FF,recptr, &ulevel,&baseoffset))	/* while entry would be invisible */
				recptr = sort_skip(FF,recptr,dir);	/* skip again */
		}
	}
	return (recptr);
}
/**********************************************************************/
short draft_disp(HWND wptr, HDC dc, short line, short lcount)	/* displays lines of record */
	/* stops at end of record, or lcount, or end of screen (whichever first) */
	/* returns number of lines written */

{
	BOOL active = GetFocus()== wptr;
	RECORD * recptr;
	register short totlines;
	TCHAR tstring[30];	/* processed string */
	INDEX * FF;
	DLINE * dlptr;
	ATTRIBUTES ast;
	RECT trect;
	long height, linepos;
	int baseoffset;
	short ulevel, indent;
	TCHAR * tbuff;
//	POINT xy;
	LFLIST * lfp;
	
	FF = getowner(wptr);
	*lrMark = col_getLocaleInfo(&FF->head.sortpars)->direction == ULOC_LAYOUT_RTL ? RLM : LRM;	// set mark for reading order
	SaveDC(dc);
	lfp = getdata(wptr);
	dlptr = &lfp->lines[line];
	totlines = 0;
	memset(&ast,0,sizeof(ast));				/* clear attribute struct */
	ast.cursize = ast.nsize = FF->head.privpars.size;		/* temp standard font size */
	ast.fmp = FF->head.fm;
	if (recptr = rec_getrec(FF,dlptr->rnum))	{	/* get its record */
		tbuff = buildentry(FF,recptr, &ulevel,&baseoffset);	
		fixsmallcaps(tbuff);
		putline(dc,tbuff,&ast,tbuff+dlptr->base, FALSE,0);	/* just to fill codearray */
		height = lfp->lheight;
		trect.top = height*line;
		trect.bottom = trect.top + height;
		linepos = lfp->drect.top+height*line+lfp->linedrop;
		while (line < lfp->nlines && dlptr->rnum == recptr->num && lcount--)	{	/* while on screen and in logical record */
			MoveToEx(dc,lfp->drect.left,linepos,NULL);	/* start of line */
			indent = (dlptr->indent&FO_HMASK)*lfp->indent + (dlptr->indent&FO_RFLAG ? lfp->indent : 0);
			if (dlptr->flags&VSELECT) {		/* if highlighted */
				if (FF->righttoleftreading)	{
					trect.left = lfp->nspace + lfp->drect.left + (dlptr->selstart >= 0 ? indent+dlptr->selstart : 0);
					trect.right = dlptr->selend >= 0 ? lfp->nspace + lfp->drect.left + indent+dlptr->selend : lfp->vrect.right;
//					NSLog("S: %d, E: %d -- L: %d; R:%d",dlptr->selstart,dlptr->selend, trect.left,trect.right);
				}
				else {
					trect.left = lfp->nspace + lfp->drect.left + (dlptr->selstart >= 0 ? indent+dlptr->selstart : 0);
					trect.right = dlptr->selend >= 0 ? lfp->nspace + lfp->drect.left + indent+dlptr->selend : lfp->vrect.right;
				}
				FillRect(dc,&trect, active ? g_highlightbrush : GetStockObject(LTGRAY_BRUSH));
			}
			if (dlptr->flags&VNUMLINE)		{	/* if at line to display number */
				COLORREF prevcolor = GetTextColor(dc);
				long len;

				type_setfont(dc,"Arial",ast.nsize,0);
				len = makelead(tstring,FF,recptr);
				SetTextColor(dc,0x00FF0000);
				TextOut(dc,0,0,tstring,len);			/* write leader */
				SetTextColor(dc,prevcolor);
				type_setfont(dc,ast.fmp[currentfont(&ast)].name,ast.nsize,ast.attr);
#ifdef PUBLISH
				if (recptr->label && FF->pf.labelmark)	{
					MoveToEx(dc,lfp->drect.left-lfp->emwidth,linepos,NULL);		/* set to mark position */
					TextOut(dc,0,0,TEXT("•"),1);
				}
#endif //PUBLISH
				if (recptr->label)
					SetTextColor(dc,g_prefs.gen.flagcolor[recptr->label]);
			}
			MoveToEx(dc,indent+lfp->nspace+lfp->drect.left,linepos,NULL);	/* offset by lead indent */
			putline(dc,tbuff+dlptr->base,&ast,tbuff+dlptr->base+dlptr->count, TRUE,dlptr->freespace);
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
static void putline(HDC dc, TCHAR * source, ATTRIBUTES * as, TCHAR * eptr, short visible, short free)      /* return first printable char */

{
	TCHAR cc, *scan, *cbase;
	
	cbase = scan = source;
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
			case FO_LEVELBREAK:
				if (visible)
					TextOut(dc,0,0,cbase,scan-cbase-1);
				if (cc == FO_LEVELBREAK)		/* if a field break */
					type_clearattributes(dc,as);	/* clear font attributes */
				break;
			case ELIPSIS:		// Aug 2020; insert reading order mark
				if (visible) {		// insert reading order mark
					TextOut(dc, 0, 0, cbase, scan - cbase);
					TextOut(dc, 0, 0, lrMark, 1);
				}
			break;
			default:
				if (as->attr&FX_SMALL)		/* if need small caps */
					*(scan-1) = u_toupper(*(scan-1));	/* make it so */
				continue;
		}
		cbase = scan;
	}
	if (visible)
		TextOut(dc,0,0,cbase,scan-cbase);
}
/*******************************************************************************/
short draft_measurelines(HWND wptr, HDC dc, RECORD * recptr, LSET *array, struct selstruct * slptr)	
		/* finds lines needed for display, fills array of start positions */
		/* last array element is set to starting offset in record */
					
{
	INDEX * FF;
	short rflag, ulevel, lcount, indent, ulevelcopy;
	long maxwidth;
	int baseoffset;
	ATTRIBUTES ast;
	TCHAR * tbuff, *tpos;
	
	FF = getowner(wptr);
	SaveDC(dc);
	tbuff = buildentry(FF,recptr,&ulevel,&baseoffset);		/* build entry */
	fixsmallcaps(tbuff);
	ulevelcopy = ulevel;		// need to preserve for stats
	array[0].offset = 0;
	memset(&ast,0,sizeof(ast));				/* clear attribute struct */
	ast.cursize = ast.nsize = FF->head.privpars.size;		/* temp standard font size */
	ast.fmp = FF->head.fm;
	maxwidth = !FF->head.privpars.vmode && !FF->head.privpars.wrap ? LONG_MAX : LX(wptr,width);
	for (lcount = rflag = 0, tpos = tbuff; *tpos && lcount < LLIMIT-1;)	{
		array[lcount].indent = ulevel + rflag;	/* indent isn't real -- codes level + runon */
		indent = LX(wptr,indent)*(ulevel + (rflag ? 1 : 0));
		tpos = measurewidth(dc,tbuff-baseoffset,tpos, &ast, maxwidth-indent-LX(wptr,nspace),&array[lcount].free, slptr, lcount);	/* find last char for current line */
		array[++lcount].offset = tpos-tbuff;
		if (*tpos == FO_LEVELBREAK)	{	/* if line ended on a break */
			tpos++;
			ulevel++;				/* advance level for indent */
			rflag = FALSE;
		}
		else
			rflag = FO_RFLAG;
	}
	if (lcount >= LLIMIT-1)
		senderronstatusline(ERR_LONGENTRYERR,WARN,recptr->num);
 	if (!FF->nostats)	{		/* if want to use the stats */
		FF->pf.entries++;	/* count a record */
		FF->pf.lines += lcount;
		if (!ulevelcopy)	// if main heading
			FF->pf.uniquemain++;
	}
	if (LX(wptr,fillEnabled))	{
		LX(wptr,filledRecords) += 1;
		LX(wptr,filledLines) += lcount;
	}
	FF->nostats = FALSE;
	RestoreDC(dc,-1);
	return (lcount);
}
/******************************************************************************/
static TCHAR * measurewidth(HDC dc, TCHAR * base, TCHAR * source, ATTRIBUTES *as, long limitwidth, long *freespace, struct selstruct * slptr, short lcount)       /*finds posn of last printable char within width */

{
	TCHAR *mark, *scan, *cbase;
	long totwidth, curseglen, markwidth, mcount;
	SIZE size;
	
	if (limitwidth < 0)		/* if no room for anything (lead is too large) */
		return base;
	for (curseglen = totwidth = 0, cbase = mark = scan = source; *scan;) {       /* while not at end or length lim  */
		if (scan-base == slptr->startpos )	{	// if at start and haven't already picked up end
			GetTextExtentPoint32(dc,cbase,scan-cbase,&size);
			slptr->shighlight = totwidth+size.cx;	/* start offset of highlight */
			slptr->sline = lcount;
		}
		if (scan-base+1 == slptr->startpos+slptr->length || scan-base+1 == slptr->startpos+slptr->length-1 && iscodechar(*scan))		{	/* if last char */
			GetTextExtentPoint32(dc,cbase,scan-cbase+1,&size);
			slptr->ehighlight = totwidth+size.cx;	/* include end char */
			slptr->eline = lcount;
		}
		switch (*scan++) {
			case CODECHR:
				GetTextExtentPoint32(dc,cbase,scan-cbase-1,&size);
				curseglen = size.cx;	/* count cur segment up to this */
				type_doattributes(dc,as,*scan++);
				break;
			case FONTCHR:
				GetTextExtentPoint32(dc,cbase,scan-cbase-1,&size);
				curseglen = size.cx;	/* count cur segment up to this */
				type_doattributes(dc,as,(*scan++)|FX_AUTOFONT);
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
				break;
			default:
				if (as->attr&FX_SMALL)		/* if need small caps */
					*(scan-1) = u_toupper(*(scan-1));	/* make it so */
				continue;
		}
		if (curseglen+totwidth >= limitwidth)	/* if current segment doesn't fit */
			goto toolong;
		totwidth += curseglen;		/* add to total */
		cbase = scan;	/* reset base for next segment */
	}
done:
	GetTextExtentPoint32(dc,cbase,scan-cbase,&size);
	curseglen = size.cx;	/* measure final segment */
	if (curseglen+totwidth < limitwidth)	/* if can add last segment */
		totwidth += curseglen;
	else {
toolong:
		if (mark > source)	{	/* if have a break somewhere */
			scan = mark;		/* go back to last space/dash */
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
/**********************************************************************/
static TCHAR * buildentry(INDEX * FF, RECORD * recptr, short * ulevel, int * baseoffset)		/* forms entry for record */

{
	char lookup[20];
	RECORD * crossptr, * sourceptr;
	CSTR flist[FIELDLIM];
	CSTR scur[FIELDLIM];
	RECN sourcenum;
	short length, count, ccount, fcount, curcount, baseshift;
	char * sptr, *eptr, *lptr, *string, pagecodechar;
	short sprlevel, hidelevel, clevel;
	
	string = f_entrybuff;
	str_xcpy(string, recptr->rtext);
	if (FF->sumsource)	{		/* if are working with summary */
		sptr = string+strlen(string)+1;		/* start pos is beginning of second field */
		eptr = string+EBUFSIZE-6;			/* limit */
		sprintf(lookup, "%ld$", recptr->num);		/* form target name */
		str_extend(lookup);
		if (crossptr = search_findbylead(FF->sumsource,lookup))	{	/* if there's an entry */
			ccount = 0;
			do {
				str_xparse(crossptr->rtext,flist);
				sourcenum = atol(flist[1].str);		/* number of source record */
				length = (short)strtoul(flist[2].str, &lptr,10);	/* length of source body (eptr has level) */
				if (*lptr == 'B')		/* if target was matched at subhead */
					sptr += strlen(sptr++);		/* show another target field */
				if (sourceptr = rec_getrec(FF, sourcenum))	{	/* if have source */
					if (ccount++)		/* if not the first ref */
						*sptr++ = SPACE;
					*sptr++ = '[';
					for (count = 0; count < length && sptr < eptr; count++, sptr++)	{
						if (!(*sptr = sourceptr->rtext[count]))	{
							*sptr++ = ';';
							*sptr = SPACE;
						}
					}
				    *sptr++ = ']';
				}
			} while ((crossptr = sort_skip(FF->sumsource,crossptr,1)) && !strcmp(crossptr->rtext, lookup) && sptr < eptr);
			*sptr++ = '\0';	/* terminate */
			*sptr = EOCS;	/* terminate after  */
		}
		else if (!str_xfindcross(FF,recptr->rtext,FALSE))	{	/* if doesn't have cross-ref */
			*sptr++ = '\0';		/* set empty page field */
			*sptr = EOCS;	/* terminate after  */
		}
	}
	curcount = str_xparse(string,scur);	/* parse it */
	pagecodechar = FO_ELIPSIS;
	baseshift = 0;					/* set to 1 if want to show elipsis before page field */
	if (!FF->head.privpars.vmode)	/* if unformatted */
		*ulevel = 0;				/* don't suppress repeated headings */
	else	{
		rec_uniquelevel(FF, recptr, ulevel,&sprlevel,&hidelevel, &clevel);	/* find level at which heading unique */
		if (FF->head.privpars.vmode == VM_SUMMARY)	{	/* if summary */
			if (*ulevel >= curcount-1)		/* suppress fields beyond lowest */
				*ulevel = curcount-1;
		}
		else if (*ulevel == PAGEINDEX)		/* if page field in normal view */
			*ulevel = curcount-1;		/* position to it */
		if (((!*ulevel && curcount == 2 && FF->head.formpars.ef.cf.mainposition == CP_FIRSTSUB)	/* if want main head cross-ref as first subhead */
			 || (curcount > 2 && FF->head.formpars.ef.cf.subposition == CP_FIRSTSUB))	/* or subhead cross-ref as subhead */
			 && str_crosscheck(FF,scur[curcount-1].str))	/* and crossref */
				pagecodechar = FO_LEVELBREAK;		/* page field cross-ref for display as first sub */
		else if (*ulevel == curcount-1 && (FF->head.privpars.vmode != VM_SUMMARY || *scur[curcount-1].str) &&
			(FF->head.formpars.ef.cf.mainposition != CP_FIRSTSUB || !str_crosscheck(FF,scur[curcount-1].str)))	/* if not to format as a subhead cross-ref */
			baseshift = 1;
	}
	*(scur[curcount-2].str+scur[curcount-2].ln) = pagecodechar;	/* set page field lead (newline or elipsis) */
	for (fcount = 0; fcount < curcount-2; fcount++)		/* for fields to display */
		*(scur[fcount].str+scur[fcount].ln) = FF->head.privpars.vmode ? FO_LEVELBREAK : FO_ELIPSIS;	/* change null to elipsis */
	if (FF->head.privpars.hidebelow != ALLFIELDS && FF->head.privpars.vmode != VM_SUMMARY)	{	/* if some restriction on display */
		if (FF->head.privpars.hidebelow == ALLBUTPAGE)
			fcount = curcount-2;					/* not showing page */
		else if (FF->head.privpars.hidebelow < curcount-1)	/* if want a level above last field */
			fcount = FF->head.privpars.hidebelow-1;
		*(scur[fcount].str+scur[fcount].ln) = '\0';		/* terminate entry */
		if (*ulevel > fcount)	{	/* if unique in suppressed part */
			*ulevel = fcount;		/* ensures text copy is indented properly */
			return (TEXT(""));		/* nothing to display */
		}
	}
	UErrorCode error = U_ZERO_ERROR;
	int32_t olength;

	u_strFromUTF8(f_widebuff, EBUFSIZE, &olength, scur[*ulevel].str - baseshift, -1, &error);
	for (int index = 0; f_widebuff[index]; index++) {
		if (f_widebuff[index] == FO_ELIPSIS)
			f_widebuff[index] = ELIPSIS;
	}
	*baseoffset = u8_countU(string, scur[*ulevel].str - string - baseshift);	// unichar offset of first field constructed (for selection offset)
	return f_widebuff;
}
/**********************************************************************/
static short makelead(TCHAR * lptr, INDEX * FF, RECORD * recptr)		/* makes lead */

{
	TCHAR *base = lptr;
	TCHAR fspace = FGSPACE;
	WORD gspace;
	TCHAR *tptr;
	
	GetGlyphIndices(LX(FF->vwind,hdc),&fspace,1,&gspace,GGI_MARK_NONEXISTING_GLYPHS);	// figure space supported?
	if (gspace == 0xFFFF)	// figure space is unsupported
		fspace = FSPACE;
	if (FF->head.privpars.shownum)	{
		lptr += u_sprintf(lptr, "%*lu", LX(FF->vwind,ndigits),recptr->num);
		for (tptr = base; *tptr == SPACE; tptr++)
			*tptr = fspace;
	}
	*lptr++ = recptr->time > FF->opentime ? BULLET : fspace;
	*lptr++ = recptr->isdel ? L'x' : fspace;
	*lptr++ = recptr->ismark ? L'#' : fspace;
	*lptr = 0;
	return (lptr-base);
}
