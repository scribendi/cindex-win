#include "stdafx.h"
#include "commands.h"
#include "index.h"
#include "records.h"
#include "group.h"
#include "sort.h"
#include "strings.h"
#include "group.h"
#include "errors.h"
#include "findset.h"
#include "replaceset.h"
#include "viewset.h"
#include "util.h"
#include "modify.h"
#include "regex.h"

static const DWORD wh_repid[] = {
	IDC_FIND_ALL, HIDC_FIND_ALL,
	IDC_FIND_SELECTED, HIDC_FIND_SELECTED,
	IDC_FIND_RANGE, HIDC_FIND_RANGE,
	IDC_FIND_RANGESTART, HIDC_FIND_RANGE,
	IDC_FIND_ALLDATES, HIDC_FIND_ALLDATES,
	IDC_FIND_RANGEEND, HIDC_FIND_RANGE,
	IDC_FIND_DATERANGE, HIDC_FIND_DATERANGE,
	IDC_FIND_DATESTART, HIDC_FIND_DATERANGE,
	IDC_FIND_DATEEND, HIDC_FIND_DATERANGE,
	IDC_FIND_ONLYAMONG,HIDC_FIND_ONLYAMONG,
	IDC_FIND_NOTAMONG,HIDC_FIND_NOTAMONG,
	IDC_FIND_NEWREC, HIDC_FIND_NEWREC,
	IDC_FIND_MODREC, HIDC_FIND_MODREC,
	IDC_FIND_DELREC, HIDC_FIND_DELREC,
	IDC_FIND_MARKREC, HIDC_FIND_MARKREC,
	IDC_FIND_GENREC, HIDC_FIND_GENREC,
	IDC_FIND_TAGREC, HIDC_FIND_TAGREC,
	IDC_FIND_TAGRECLEVEL,HIDC_FIND_TAGRECLEVEL,
	IDC_FIND_FINDSTART, HIDC_FIND_FINDSTART,
	IDC_REPLACE_REP, HIDC_REPLACE_REP,
	IDC_REPLACE_REPALL, HIDC_REPLACE_REPALL,
	IDC_FIND_FINDSTOP, HIDC_FIND_FINDSTOP,
	IDC_FIND_UP, HIDC_FIND_UP,
	IDC_FIND_DOWN, HIDC_FIND_DOWN,
	IDC_FIND_TEXT1, HIDC_FIND_TEXT1,
	IDC_FIND_STYLE1, HIDC_FIND_STYLE1,
	IDC_FIND_STYLESHOW1, HIDC_FIND_STYLESHOW1,
	IDC_FIND_FIELDS1, HIDC_FIND_FIELDS1,
	IDC_FIND_WORD1, HIDC_FIND_WORD1,
	IDC_FIND_CASE1, HIDC_FIND_CASE1,
	IDC_FIND_PATTERN1, HIDC_FIND_PATTERN1,
	IDC_REPLACE_TEXT, HIDC_REPLACE_TEXT,
	IDC_REPLACE_STYLE, HIDC_REPLACE_STYLE,
	IDC_REPLACE_STYLESHOW, HIDC_REPLACE_STYLESHOW,
	0,0
};

static const DWORD wh_rstyleid[] = {
	IDC_REPSTYLE_BOLDAP, HIDC_REPSTYLE_BOLDAP,
	IDC_REPSTYLE_BOLDIG, HIDC_REPSTYLE_BOLDIG,
	IDC_REPSTYLE_BOLDREM, HIDC_REPSTYLE_BOLDREM,
	IDC_REPSTYLE_ITALAP, HIDC_REPSTYLE_BOLDAP,
	IDC_REPSTYLE_ITALIG, HIDC_REPSTYLE_BOLDIG,
	IDC_REPSTYLE_ITALREM, HIDC_REPSTYLE_BOLDREM,
	IDC_REPSTYLE_ULINEAP, HIDC_REPSTYLE_BOLDAP,
	IDC_REPSTYLE_ULINEIG, HIDC_REPSTYLE_BOLDIG,
	IDC_REPSTYLE_ULINEREM, HIDC_REPSTYLE_BOLDREM,
	IDC_REPSTYLE_SMALLAP, HIDC_REPSTYLE_BOLDAP,
	IDC_REPSTYLE_SMALLIG, HIDC_REPSTYLE_BOLDIG,
	IDC_REPSTYLE_SMALLREM, HIDC_REPSTYLE_BOLDREM,
	IDC_REPSTYLE_SUPAP, HIDC_REPSTYLE_BOLDAP,
	IDC_REPSTYLE_SUPIG, HIDC_REPSTYLE_BOLDIG,
	IDC_REPSTYLE_SUPREM, HIDC_REPSTYLE_BOLDREM,
	IDC_REPSTYLE_SUBAP, HIDC_REPSTYLE_BOLDAP,
	IDC_REPSTYLE_SUBIG, HIDC_REPSTYLE_BOLDIG,
	IDC_REPSTYLE_SUBREM, HIDC_REPSTYLE_BOLDREM,
	IDC_REPSTYLE_FONT, HIDC_REPSTYLE_FONTSET,
	IDC_REPSTYLE_FONTIG, HIDC_REPSTYLE_FONTIG,
	IDC_REPSTYLE_FONTSET, HIDC_REPSTYLE_FONTSET,
	0,0
};

static int sidarray[] =  {
	IDC_REPSTYLE_BOLDIG,
	IDC_REPSTYLE_ITALIG,
	IDC_REPSTYLE_ULINEIG,
	IDC_REPSTYLE_SMALLIG,
	IDC_REPSTYLE_SUPIG,
	IDC_REPSTYLE_SUBIG
};
#define STYLEIDS (sizeof(sidarray)/sizeof(int))

static LRESULT CALLBACK repproc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static int rcommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);	/* does menu tasks */
static BOOL setattrib(HWND hwnd);	/* gets attributes */
static INT_PTR CALLBACK attribproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static void setstylebuttons(HWND hwnd, int idindex, RFLIST * rfp);	/* sets id buttons */
static void recoverstylebuttons(HWND hwnd, int idindex, RFLIST * rfp);	/* recovers style info */
static void ractivate(HWND hwnd, UINT state, HWND hwndActDeact, BOOL fMinimized);	/* activates/deactivates */
static BOOL rinit(HWND hwnd, HWND hwndFocus, LPARAM lParam);	/* initializes dialog */
static short rclose(HWND hwnd);		/* closes text window */
static void initreplace(HWND hwnd,short resume, BOOL protect);	/* clears things for new search */
static void testsearchok(HWND hwnd);	/* enables ok if fields completed */
static void setallbuttons(HWND hwnd);	/* sets all buttons */
static short cleanuprep(HWND hwnd);	/* cleans up after finishing replacements */
/*********************************************************************************/
HWND rep_setwindow(HWND hwnd)	/*  sets up rep window */

{
	if (g_repw || (g_repw = CreateDialogParam(g_hinst,MAKEINTRESOURCE(IDD_REPLACE),g_hwframe,repproc,(LPARAM)hwnd)))
		fs_setpos(g_repw);		/* set its position if necessary */
	return (g_repw);
}
/************************************************************************/
static LRESULT CALLBACK repproc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)

{
	switch (msg)	{
		HANDLE_MSG(hwnd,WM_INITDIALOG,rinit);
#if 0
		HANDLE_MSG(hwnd,WM_CLOSE,rclose);
#endif
		HANDLE_MSG(hwnd,WM_ACTIVATE,ractivate);
		case WM_COMMAND:
			return (HANDLE_WM_COMMAND(hwnd, wParam, lParam, rcommand));
		case WM_HOTKEY:
			fs_hotkey(hwnd,wParam);
			return (TRUE); 
		case WM_HELP:
			return (dodialoghelp(hwnd,TEXT("_Find & Replace\\Usingreplace.htm"),(HELPINFO *)lParam,wh_repid));
	}
	return (FALSE);
}
/************************************************************************/
static int rcommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)	/* does menu tasks */

{
	INDEX * FF;
	int titem, changed, pcount;
	LISTGROUP * lg;
	char istring[STSTRING];
	char dupcopy[MAXREC], *sptr, *tptr;
	RECORD * recptr;
	HCURSOR ocurs;
	RFLIST * rfp;
	BOOL protect = FALSE;

	if ((rfp = getdata(hwnd)) && (FF = rfp->lastindex) && (hwndCtl || id == IDCANCEL) && !rfp->resetting)	{	/* if data, index, & child message */
		lg = &rfp->lr;
		changed = FALSE;

		/* need following in case records moved between find and replace */
		/* might leave dialog then come back */
		if (rfp->target && (recptr = rec_getrec(FF,rfp->target)))	/* if already have a target */
			sptr = recptr->rtext+rfp->offset;
		else
			sptr = NULL;		/* default no target */
	
		switch (id)	{
		case IDC_REPLACE_REP:		/* replace this occurrence */
			getDItemText(hwnd,IDC_REPLACE_TEXT,istring,sizeof(istring));
			if (rfp->nptr || (rfp->nptr = rep_setup(FF,lg,&rfp->rg,&rfp->ra,istring)))	{	/* if have/can set up structures */
				str_xcpy(dupcopy, recptr->rtext);		/* save copy */
				if (sptr = search_reptext(FF,recptr, sptr, rfp->mlength, &rfp->rg, &lg->lsarray[0])) {		// if replaced
					long oxlen = str_xlen(recptr->rtext);
					long xlen = str_adjustcodes(recptr->rtext, CC_TRIM | (g_prefs.gen.remspaces ? CC_ONESPACE : 0));	/* clean up codes */
					sptr -= oxlen - xlen;	// adjust ptr for any stripped codes (assumes any redundant codes will have been added in this replacement)
					if (!*sptr)	// if at end of field
						sptr++;	// force skip to next
					rec_strip(FF,recptr->rtext);		/* remove empty fields */
					sort_addtolist(rfp->nptr,recptr->num);	/* add to sort list */
					pcount = rec_propagate(FF,recptr,dupcopy, rfp->nptr);	/* propagate */
					view_clearselect(FF->vwind);
					view_resetrec(FF,recptr->num);	/* redisplay */
					rfp->repcount += pcount+1;
					rfp->mlength = 0;	/* clear this to enable decent search */
//					if (!*sptr)			// if we have an empty field after replacement, don't look for more in this record
//						sptr = NULL;
				}
				else	{	/* record too long */
					rfp->markcount++;
					MessageBeep(MB_ICONHAND);
				}
			}
			else
				break;
			case IDC_FIND_FINDSTART:	/* falls through from replacement */
				if (sptr && (sptr = search_findbycontent(FF,recptr, sptr+rfp->mlength, lg, &rfp->mlength)))	{	/* if any more in this record */
					if (*sptr)	{	// if matched real text
						view_selectrec(FF, recptr->num, VD_SELPOS,sptr-recptr->rtext,rfp->mlength ? rfp->mlength : -1);
						rfp->offset = sptr-recptr->rtext;	/* offset for next search or replace */
						setallbuttons(hwnd);
						break;
					}
				}
				if (fs_findnext(FF,hwnd,lg, &rfp->offset,&rfp->mlength, TRUE))	/* if found the next target */
					setallbuttons(hwnd);		/* ok */
				else
					changed = TRUE;				/* nothing doing */
				break;
			case IDC_FIND_FINDSTOP:
#if 0
				if (!rfp->nptr)	{		/* if no outstanding replacements (if showing Reset) */
					fs_reset(hwnd,lg,FALSE);
					memset(&rfp->ra,0,sizeof(rfp->ra));		/* clear replace attrib structure */
					initreplace(hwnd,FALSE,FALSE);	/* set up for new find */
					rfp->scope = COMR_ALL;
					rfp->dateflag = FALSE;	/* all dates */
					break;
				}
#else
				cleanuprep(hwnd);
				fs_reset(hwnd,lg,FALSE);
				memset(&rfp->ra,0,sizeof(rfp->ra));		/* clear replace attrib structure */
				initreplace(hwnd,FALSE,FALSE);	/* set up for new find */
				rfp->scope = COMR_ALL;
				rfp->dateflag = FALSE;	/* all dates */
				break;
#endif
			case IDCANCEL:
				rclose(hwnd);
				if (rfp->target)	/* if have target */
					view_selectrec(FF,rfp->target,VD_SELPOS,-1,-1);	/* select it */
				return FALSE;
			case IDC_REPLACE_REPALL:
				if (!sptr && fs_findnext(FF,hwnd,lg, &rfp->offset,&rfp->mlength, TRUE)) 	{	/* if don't have, but can find target */
					if ((recptr = rec_getrec(FF,rfp->target)))		/* if can get target */
						sptr = recptr->rtext+rfp->offset;
				}
				getDItemText(hwnd,IDC_REPLACE_TEXT,istring,sizeof(istring));
				if (sptr && (rfp->nptr || (rfp->nptr = rep_setup(FF,lg,&rfp->rg,&rfp->ra,istring))))	{	/* if have target & have/can set up structures */
					ocurs = SetCursor(g_waitcurs);
					do {
						while (sptr && (tptr = search_reptext(FF,recptr, sptr, rfp->mlength, &rfp->rg, &lg->lsarray[0])))	{	/* while can replace a target */
							long oxlen = str_xlen(recptr->rtext);
							long xlen = str_adjustcodes(recptr->rtext, CC_TRIM | (g_prefs.gen.remspaces ? CC_ONESPACE : 0));	/* clean up codes */
							tptr -= oxlen - xlen;	// adjust ptr for any stripped codes (assumes any redundant codes will have been added in this replacement)
							if (!*tptr)	// if at end of field
								tptr++;	// force skip to next
							rfp->repcount++;
							sort_addtolist(rfp->nptr,recptr->num);		/* add to sort list */
//							if (!*sptr)	// if we have an empty field after replacement, don't look for more
//								break;
				 			sptr = search_findbycontent(FF, recptr,tptr, lg, &rfp->mlength); /* more in current record? */
						}
						if (sptr && !tptr)		/* if couldn't make a replacement */
							rfp->markcount++;
//						str_adjustcodes(recptr->rtext,CC_TRIM|(g_prefs.gen.remspaces ? CC_ONESPACE : 0));	/* adjust codes etc */
						rec_strip(FF,recptr->rtext);		/* remove empty fields */
					} while ((recptr = search_findfirst(FF,lg,FALSE,&sptr, &rfp->mlength)));
					cleanuprep(hwnd);		/* resort */
					SetCursor(ocurs);
					if (rfp->markcount)	/* if some marked records */
						sendinfo(INFO_REPLACEMARKED,rfp->repcount,rfp->markcount);
					else	
						sendinfo(INFO_REPLACECOUNT,rfp->repcount);
					changed = TRUE;
				}
				break;
			case IDC_FIND_ALL:
			case IDC_FIND_SELECTED:
			case IDC_FIND_RANGE:
				if (id == IDC_FIND_RANGE)
					selectitext(hwnd,IDC_FIND_RANGESTART);
				if (rfp->scope != id-IDC_FIND_ALL)	{
					changed = TRUE;
					rfp->scope = id-IDC_FIND_ALL;
				}
				break;
			case IDC_FIND_RANGESTART:
			case IDC_FIND_RANGEEND:
				if (codeNotify == EN_CHANGE)	{
					CheckRadioButton(hwnd,IDC_FIND_ALL,IDC_FIND_RANGE, IDC_FIND_RANGE);
					changed = TRUE;
					protect = TRUE;		/* stops init selection of text */
					rfp->scope = COMR_RANGE;
				}
				break;
			case IDC_FIND_ALLDATES:
			case IDC_FIND_DATERANGE:
				if (id == IDC_FIND_DATERANGE)
					selectitext(hwnd,IDC_FIND_DATESTART);
				if (rfp->dateflag != id-IDC_FIND_ALLDATES)	{
					changed = TRUE;
					rfp->dateflag = id-IDC_FIND_ALLDATES;
				}
				break;
			case IDC_FIND_DATESTART:
			case IDC_FIND_DATEEND:
				if (codeNotify == EN_CHANGE)	{
					CheckRadioButton(hwnd,IDC_FIND_ALLDATES,IDC_FIND_DATERANGE,IDC_FIND_DATERANGE);
					changed = TRUE;
					protect = TRUE;		/* stops init selection of text */
					rfp->dateflag = TRUE;
				}
#if 1
				else if (codeNotify == EN_KILLFOCUS) {
					checkdate(hwnd, id);
				}
#endif
				break;
			case IDC_FIND_ONLYAMONG:
			case IDC_FIND_NOTAMONG:
				lg->excludeflag = id-IDC_FIND_ONLYAMONG;
				changed = TRUE;
				break;
			case IDC_FIND_NEWREC:
				checkcontrol(hwndCtl,lg->newflag ^= 1);
				changed = TRUE;
				break;
			case IDC_FIND_MODREC:
				checkcontrol(hwndCtl,lg->modflag ^= 1);
				changed = TRUE;
				break;
			case IDC_FIND_GENREC:
				checkcontrol(hwndCtl,lg->genflag ^= 1);
				changed = TRUE;
				break;
			case IDC_FIND_TAGREC:
				checkcontrol(hwndCtl,lg->tagflag ^= 1);
				changed = TRUE;
				break;
			case IDC_FIND_TAGRECLEVEL:
				if (codeNotify == CBN_SELENDOK)	{
					lg->tagvalue = ComboBox_GetCurSel(GetDlgItem(hwnd,IDC_FIND_TAGRECLEVEL));
					checkcontrol(GetDlgItem(hwnd,IDC_FIND_TAGREC),lg->tagflag = 1);
					changed = TRUE;
				}
				break;
			case IDC_FIND_DELREC:
				checkcontrol(hwndCtl,lg->delflag ^= 1);
				changed = TRUE;
				break;
			case IDC_FIND_MARKREC:
				checkcontrol(hwndCtl,lg->markflag ^= 1);
				changed = TRUE;
				break;
			case IDC_FIND_UP:
			case IDC_FIND_DOWN:
				lg->revflag = IDC_FIND_DOWN-id;
				changed = TRUE;
				break;
			case IDC_FIND_TEXT1:
				if (codeNotify == CBN_EDITCHANGE || codeNotify == CBN_SELENDOK)	{
					TCHAR tt[LISTSTRING];
					if (codeNotify == CBN_EDITCHANGE)
						GetWindowText(hwndCtl,tt,LISTSTRING);
					else
						ComboBox_GetLBText(hwndCtl,ComboBox_GetCurSel(hwndCtl),tt);
					strcpy(lg->lsarray[0].string,fromNative(tt));
					changed = TRUE;
					protect = TRUE;		/* stops init selection of text */
					fs_setwordenable(hwnd,IDC_FIND_WORD1,&lg->lsarray[0]);	/* if can't do word match */
				}
				break;
			case IDC_FIND_STYLE1:
				if (fs_getstyle(hwnd,FF,&lg->lsarray[0]))	{
					fs_showstyle(hwnd,IDC_FIND_STYLESHOW1,lg->lsarray[0].style,lg->lsarray[0].font, lg->lsarray[0].forbiddenstyle, lg->lsarray[0].forbiddenfont);
					rfp->ra.offstyle &= lg->lsarray[0].style;	/* adjust permitted removal styles */
					fs_showstyle(hwnd,IDC_REPLACE_STYLESHOW,rfp->ra.onstyle,rfp->ra.fontchange, rfp->ra.offstyle, 0);
					changed = TRUE;
				}
				break;
			case IDC_REPLACE_STYLE:
				if (setattrib(hwnd))	{
					fs_showstyle(hwnd,IDC_REPLACE_STYLESHOW, rfp->ra.onstyle, rfp->ra.fontchange, rfp->ra.offstyle, 0);
					changed = TRUE;
				}
				break;
			case IDC_FIND_FIELDS1:
				if (codeNotify == CBN_SELENDOK)	{
					changed = TRUE;
					titem = fs_getfieldindex(hwndCtl);
					lg->lsarray[0].field = titem;
				}
				break;
			case IDC_FIND_WORD1:
				checkcontrol(hwndCtl,lg->lsarray[0].wordflag ^= 1);
				changed = TRUE;
				break;
			case IDC_FIND_CASE1:
				checkcontrol(hwndCtl,lg->lsarray[0].caseflag ^= 1);
				changed = TRUE;
				break;
			case IDC_FIND_PATTERN1:
				fs_configureforpattern(hwnd, IDC_FIND_NOT1, lg->lsarray[0].patflag ^ 1);
		}
		testsearchok(hwnd);
		if (changed && !rfp->restart){		/* if need to reset (and not already reset) */
			cleanuprep(hwnd);			/* force resort of changed records */
			initreplace(hwnd,FALSE,protect);
		}
		return (TRUE);
	}
	return (FALSE);	
}
/*******************************************************************************/
static BOOL setattrib(HWND hwnd)	/* gets attributes */

{
	if (DialogBoxParam(g_hinst,MAKEINTRESOURCE(IDD_REPLACE_STYLE),hwnd,attribproc,(LPARAM)getdata(hwnd)))
		return (TRUE);
	return (FALSE);
}
/*******************************************************************************/
static INT_PTR CALLBACK attribproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)

{
	RFLIST * rfp;
	HWND cbh;
	int count;

	rfp = getdata(hwnd);
	switch (msg)	{

		case WM_INITDIALOG:
			if (rfp = setdata(hwnd,(void *)lParam))	{	/* set data */
				for (count = 0; count < STYLEIDS; count++)	/* for all styles */
					setstylebuttons(hwnd, count,rfp);	/* set style button */
				CheckRadioButton(hwnd,IDC_REPSTYLE_FONTIG,IDC_REPSTYLE_FONTSET,IDC_REPSTYLE_FONTIG+rfp->ra.fontchange);
				type_setfontnamecombo(hwnd,IDC_REPSTYLE_FONT,NULL);
				cbh = GetDlgItem(hwnd, IDC_REPSTYLE_FONT);
				if (!rfp->ra.fontchange) {	/* if ignoring font */
					EnableWindow(cbh,0);
					ComboBox_SetCurSel(cbh, -1);
				}
				else
					ComboBox_SelectString(cbh,-1,toNative(rfp->ra.font));
				centerwindow(hwnd,0);
			}
			return FALSE;
		case WM_COMMAND:
			switch (LOWORD(wParam))	{
				case IDOK:
					for (count = 0; count < STYLEIDS; count++)	/* for all styles */
						recoverstylebuttons(hwnd, count,rfp);	/* recover style */
					rfp->ra.fontchange = (short)isitemchecked(hwnd,IDC_REPSTYLE_FONTSET);
					if (rfp->ra.fontchange)	{	/* if want to change a font */
						cbh = GetDlgItem(hwnd,IDC_REPSTYLE_FONT);
						if (ComboBox_GetCurSel(cbh) > 0)	/* if not default font */
							getDItemText(hwnd,IDC_REPSTYLE_FONT,rfp->ra.font,FSSTRING);
						else
							*rfp->ra.font = '\0';
					}
				case IDCANCEL:
					EndDialog(hwnd,LOWORD(wParam) == IDOK ? TRUE : FALSE);
					return TRUE;
				case IDC_REPSTYLE_FONTSET:
				case IDC_REPSTYLE_FONTIG:
					cbh = GetDlgItem(hwnd, IDC_REPSTYLE_FONT);
					EnableWindow(cbh, isitemchecked(hwnd, IDC_REPSTYLE_FONTSET));
					ComboBox_SetCurSel(cbh, IsWindowEnabled(cbh) ? 0 : -1);	// enable/disable combo per mode
					return (TRUE);
				case IDC_REPSTYLE_SUPAP:
					if (isitemchecked(hwnd,IDC_REPSTYLE_SUBAP))
						CheckRadioButton(hwnd,IDC_REPSTYLE_SUBIG,IDC_REPSTYLE_SUBREM,IDC_REPSTYLE_SUBIG);
					return TRUE;
				case IDC_REPSTYLE_SUBAP:
					if (isitemchecked(hwnd,IDC_REPSTYLE_SUPAP))
						CheckRadioButton(hwnd,IDC_REPSTYLE_SUPIG,IDC_REPSTYLE_SUPREM,IDC_REPSTYLE_SUPIG);
					return TRUE;
			}
			break;
		case WM_HELP:
			return (dodialoghelp(hwnd,TEXT("_Find & Replace\\Replace_styleandfont.htm"),(HELPINFO *)lParam,wh_rstyleid));
	}
	return (FALSE);
}
/*******************************************************************************/
static void setstylebuttons(HWND hwnd, int idindex, RFLIST * rfp)	/* sets id buttons */

{
	int action;

	action = 0;		/* default action is ignore */
	if (!(rfp->lr.lsarray[0].style&(1 << idindex)))	{	/* if not searching for a style */
		disableitem(hwnd, sidarray[idindex]+2);	/* disable remove */
		rfp->ra.offstyle &= ~(1 << idindex);	/* clear remove style bit */
	}
	if (rfp->ra.offstyle&(1 << idindex))	/* if want removed */
		action = 2;
	else if (rfp->ra.onstyle&(1 << idindex))	/* if want add */
		action = 1;
	CheckRadioButton(hwnd,sidarray[idindex],sidarray[idindex]+2,sidarray[idindex]+action);
}
/*******************************************************************************/
static void recoverstylebuttons(HWND hwnd, int idindex, RFLIST * rfp)	/* recovers style info */

{
	int action;

	rfp->ra.offstyle &= ~(1 << idindex);	/* clear style bit */
	rfp->ra.onstyle &= ~(1 << idindex);		/* clear style bit */
	action = findgroupcheck(hwnd,sidarray[idindex],sidarray[idindex]+2)-sidarray[idindex];
	if (action == 2)		/* if want removed */
		rfp->ra.offstyle |= (1 << idindex);	/* set style bit */
	else if (action == 1)	/* if want add */
		rfp->ra.onstyle |= (1 << idindex);	/* set style bit */
}
/******************************************************************************/
static BOOL rinit(HWND hwnd, HWND hwndFocus, LPARAM lParam)	/* initializes dialog */

{
	RFLIST * rflist;

	if (rflist = getmem(sizeof(RFLIST)))	{	/* if can get memory for our window structure */
		setdata(hwnd,rflist);		/* install our private data */
		fs_setlabelcombo(hwnd);
		(LONG_PTR)rflist->edproc = fs_installcombohook(hwnd,IDC_FIND_TEXT1);
		type_settextboxfont(GetDlgItem(hwnd,IDC_FIND_TEXT1));
		rflist->lr.size = 1;		/* size of group */
		rflist->init = initreplace;	/* function for setting up new find */
		checkitem(hwnd,IDC_FIND_CASE1, TRUE);
		centerwindow(hwnd,-1);
		return (FALSE);
	}
	DestroyWindow(hwnd);
	return (FALSE);
}
/************************************************************************/
static void ractivate(HWND hwnd, UINT state, HWND hwndActDeact, BOOL fMinimized)	/* activates/deactivates */

{
	INDEX * FF;
	RFLIST * rfp;

	rfp = getdata(hwnd);
	if (state != WA_INACTIVE)	{	/* being activated */
		g_mdlg = hwnd;
		if (FF = index_front())	{			/* if have any index window */
			if (FF != rfp->lastindex)	{	/* if changed index in which searching */
				rfp->lr.lsarray[0].andflag = FALSE;
				rfp->lr.lsarray[0].field = ALLFIELDS;
				rfp->lr.lsarray[0].caseflag = TRUE;
				fs_setfieldmenu(FF, GetDlgItem(hwnd,IDC_FIND_FIELDS1));	/* get current menu */
				rfp->lastindex = FF;
				initreplace(hwnd,FALSE,FALSE);			/* set up for new find */
				rfp->scope = COMR_ALL;
				rfp->dateflag = FALSE;		/* all dates */
				rfp->lr.excludeflag = FALSE;	// only among
				CheckRadioButton(hwnd,IDC_FIND_ONLYAMONG,IDC_FIND_NOTAMONG, IDC_FIND_ONLYAMONG);
				CheckRadioButton(hwnd,IDC_FIND_UP,IDC_FIND_DOWN, IDC_FIND_DOWN);
				CheckRadioButton(hwnd,IDC_FIND_ALL,IDC_FIND_RANGE, IDC_FIND_ALL);
				CheckRadioButton(hwnd,IDC_FIND_ALLDATES,IDC_FIND_DATERANGE,IDC_FIND_ALLDATES);
				ComboBox_SetCurSel(GetDlgItem(hwnd,IDC_FIND_TAGRECLEVEL),0);
			}
			else	{		/* back in last-searched index */
//				if (FF->rwind)	/* don't permit open record window */
//					mod_close(FF->rwind,MREC_ALWAYSACCEPT);
				if (rfp->target)		{	/* if we have a target */
					if (FF->acount != rfp->vacount)	{	/* if have had view window active */
						initreplace(hwnd,TRUE,FALSE);			/* make resume settings */
#if 1
						view_clearselect(FF->vwind);	/* clear selection in case come in with wrong starter highlighted */
#endif
					}
					else		/* just reset condition buttons */
						setallbuttons(hwnd);
				}
				else		/* starting again in existing index */
					initreplace(hwnd,FALSE,FALSE);	/* force new search */
			}
			if (FF->startnum == FF->head.rtot)	{	/* if no new records */
				disableitem(hwnd,IDC_FIND_NEWREC);
				checkitem(hwnd,IDC_FIND_NEWREC,0);	/* clear check box */
				rfp->lr.newflag = 0;	/* clear flag */
			}
			else
				enableitem(hwnd,IDC_FIND_NEWREC);
			if (FF->head.privpars.vmode == VM_FULL){		/* if formatted view */
				disableitem(hwnd,IDC_FIND_DELREC);
				checkitem(hwnd,IDC_FIND_DELREC,0);	/* clear check box */
				rfp->lr.delflag = 0;	/* clear flag */
			}
			else
				enableitem(hwnd,IDC_FIND_DELREC);
			if (view_recordselect(FF->vwind))
				enableitem(hwnd,IDC_FIND_SELECTED);
			else	{
				disableitem(hwnd,IDC_FIND_SELECTED);
				if (rfp->scope == COMR_SELECT)	{	/* if had wanted selction */
					rfp->scope = COMR_ALL;		/* set for all */
					CheckRadioButton(hwnd,IDC_FIND_ALL,IDC_FIND_RANGE, IDC_FIND_ALL);
				}
			}
			testsearchok(hwnd);
		}
		else
			rfp->lastindex = NULL;
	}
	else {
		g_mdlg = NULL;
		f_lastwind = hwnd;		/* this was last-used of find/replace */
		if (getdata(hwnd) && rfp->lastindex)	{	/* being deactivated */
			cleanuprep(hwnd);		/* if had to resort some records */
			rfp->vacount = rfp->lastindex->acount;	/* save v window activation count */
		}
	}
}
/******************************************************************************/
static short rclose(HWND hwnd)		/* closes replace window */

{
	HWND nhw;

#if 0
	cleanuprep(hwnd);
	freedata(hwnd);		/* window now has nothing */
	g_repw = g_mdlg = NULL;
	DestroyWindow(hwnd);	/* destroy it */
#else
	cleanuprep(hwnd);
#if 0
	if (g_mdlg == hwnd)	/* if active window (active might be Find) */
		g_mdlg = NULL;
#endif
	nhw = GetWindow(hwnd,GW_HWNDPREV);	/* see if window on top */
	if (hwnd == GetWindow(nhw,GW_OWNER))	/* if we have one, it's style */
		SendMessage(nhw,WM_CLOSE,0,0);	/* close it */
	ShowWindow(hwnd,SW_HIDE);	/* hide it */
#if 0
	RX(hwnd,lastindex) = NULL;	/* no last index */
#endif
#endif
	return (0);
}
/******************************************************************************/
static void initreplace(HWND hwnd,short resume, BOOL protect)	/* clears things for new search */

{
	HWND cb;
	RFLIST * rfp;

	rfp = getdata(hwnd);
	if (resume)
		SetWindowText(GetDlgItem(hwnd,IDC_FIND_FINDSTART),TEXT("Resume"));	/* set new title */
	else {
		SetWindowText(GetDlgItem(hwnd,IDC_FIND_FINDSTART),TEXT("&Find"));		/* set new title */
		rfp->target = 0;		/* no record in which to replace */
		rfp->restart = TRUE;
		rfp->repcount = 0;		/* no replacements */
		rfp->markcount = 0;		/* no marked records */
		SetWindowText(GetDlgItem(hwnd,IDC_FIND_FINDSTOP),TEXT("Re&set"));	/* set new title */
		cb = GetDlgItem(hwnd,IDC_FIND_TEXT1);
		if (!protect)
			SetFocus(cb);
		rfp->offset = 0;		/* no next search position */
		disableitem(hwnd,IDC_REPLACE_REP);
	}
	rfp->mlength = 0;		/* no replacement length */
	testsearchok(hwnd);
}
/******************************************************************************/
static void testsearchok(HWND hwnd)	/* enables ok if fields completed */

{
	int tlen,r1len,r2len,d1len,d2len, trlen;
	char istring[STSTRING];
	RFLIST * rfp;
	BOOL checkready;

	rfp = getdata(hwnd);
	tlen = strlen(rfp->lr.lsarray[0].string);
	trlen = getDItemText(hwnd,IDC_REPLACE_TEXT,istring,sizeof(istring));
	r1len = getDItemText(hwnd,IDC_FIND_RANGESTART,istring,sizeof(istring));
	r2len = getDItemText(hwnd,IDC_FIND_RANGEEND,istring,sizeof(istring));
	d1len = getDItemText(hwnd,IDC_FIND_DATESTART,istring,sizeof(istring));
	d2len = getDItemText(hwnd,IDC_FIND_DATEEND,istring,sizeof(istring));
	checkready = (tlen && (trlen || !rfp->ra.onstyle && !rfp->ra.offstyle && !rfp->ra.fontchange)		/* find target & (rep target || no style or font) */
		|| !tlen && (rfp->lr.lsarray[0].style || rfp->lr.lsarray[0].font || rfp->lr.lsarray[0].forbiddenstyle || rfp->lr.lsarray[0].forbiddenfont) && !trlen
		&& (rfp->ra.onstyle || rfp->ra.offstyle || rfp->ra.fontchange))	/* or no targets and just fonts/styles */
		&& (rfp->scope != COMR_RANGE || r1len || r2len)
		&& (!rfp->dateflag || d1len || d2len);
	if (checkready && g_mdlg && rfp->lastindex && !rfp->lastindex->rwind)	{	// if checks ok, and no record window
		/* if have text in first search field or style or font, and range ok and dates ok */
		enableitem(hwnd,IDC_FIND_FINDSTART);
		enableitem(hwnd,IDC_REPLACE_REPALL);
	}
	else {
		disableitem(hwnd,IDC_FIND_FINDSTART);
		disableitem(hwnd,IDC_REPLACE_REPALL);
	}
}
/******************************************************************************/
static void setallbuttons(HWND hwnd)	/* sets all buttons */

{	
	if (RX(hwnd,target))	{
		SetWindowText(GetDlgItem(hwnd,IDC_FIND_FINDSTART),TEXT("&Find Again"));	/* set new title */
		SetWindowText(GetDlgItem(hwnd,IDC_FIND_FINDSTOP),RX(hwnd,nptr) ? TEXT("Done") : TEXT("Re&set"));	/* set new title */
		enableitem(hwnd,IDC_REPLACE_REP);
	}
}
#if 0
/******************************************************************************/
struct numstruct * rep_setup(INDEX * FF, LISTGROUP * lg, REPLACEGROUP * rg, REPLACEATTRIBUTES *rap, char * replace)	/* sets up structures */

{
	char *tptr, c;
	
	memset(rg,0,sizeof(REPLACEGROUP));	/* initialize */
	strcpy(rg->sourcestring, replace);	/* replacement string. We'll tinker with it */
	rg->regex = lg->lsarray[0].regex;
	rg->ra = *rap;		/* copy text attributes */
	if ((rg->ra.onstyle || rg->ra.offstyle || rg->ra.fontchange) && !*lg->lsarray[0].string)	{	/* if replacing style or font & no search target */
		rg->reptot = 1;		/* mark one replacement */
		rg->rep[0].index = '&'-'1';	/* set replacement to whatever's matched */
	}
	else {	/* some actual text/pattern being replaced */
		for (tptr = rg->sourcestring; *tptr && rg->reptot < SREPLIM; rg->reptot++)	{	/* extract parts of substitution string */
			for (rg->rep[rg->reptot].start = tptr; *tptr; tptr++)	{ /* while in string */
				if (lg->lsarray[0].patflag && *tptr == ESCCHR)	{	/* if pattern & escape with following char */
					if (isdigit(c = *(tptr+1)) && c != '0' || c == '&') {	/* if special replacement	*/
						if (!rg->rep[rg->reptot].len)	 {	/* if haven't been building an ordinary string */
//							if ((rg->rep[rg->reptot].index = c - '1') >= lg->lsarray[0].expcount && c != '&')	 { /* put in index; if not that many subexpressions */
							if ((rg->rep[rg->reptot].index = c - '1') >= regex_groupcount(lg->lsarray[0].regex) && c != '&')	 { /* put in index; if not that many subexpressions */
								senderr(ERR_BADREPERR, WARN);
								return (NULL);
							}
							tptr += 2;	/* now points to char beyond string designator */
							if (*tptr == '+' || *tptr == '-')		  /* if want case change */
								rg->rep[rg->reptot].flag = *tptr++ == '+' ? 1 : -1;	  /* put in flag and skip over sign */
							rg->rep[rg->reptot].start = NULL;		/* indicates a special replacement */
						}
						break;	/* force up one component */
					}
					else {
						if (!*(tptr+1))		/* if dangling \ on end of line */
							continue;		/* will ignore */
						memmove(tptr, tptr+1, strlen(tptr));	/* shift over esc char */
					}
				}
				rg->rep[rg->reptot].len++;		/* count one char in replacement string */
			}
		}
	}
	rg->maxlen = FF->head.indexpars.recsize-1;
    return (sort_setuplist(FF));		/* if can set up sort list */
}
#else
/******************************************************************************/
struct numstruct * rep_setup(INDEX * FF, LISTGROUP * lg, REPLACEGROUP * rg, REPLACEATTRIBUTES *rap, char * replace)	/* sets up structures */

{
	char *tptr, c;
	
	memset(rg,0,sizeof(REPLACEGROUP));	/* initialize */
	strcpy(rg->sourcestring, replace);	/* replacement string. We'll tinker with it */
	u8_normalize(rg->sourcestring, strlen(rg->sourcestring)+1);		// normalize to composed characters
	rg->regex = lg->lsarray[0].regex;
	rg->ra = *rap;		/* copy text attributes */
	if ((rg->ra.onstyle || rg->ra.offstyle || rg->ra.fontchange) && !*lg->lsarray[0].string)	{	/* if replacing style or font & no search target */
		rg->reptot = 1;		/* mark one replacement */
		rg->rep[0].index = 0;	// set replacement to capture group 0 (full match)
	}
	else {	/* some actual text/pattern being replaced */
		for (tptr = rg->sourcestring; *tptr && rg->reptot < SREPLIM; rg->reptot++)	{	/* extract parts of substitution string */
			for (rg->rep[rg->reptot].start = tptr; *tptr; tptr++)	{ /* while in string */
				if (lg->lsarray[0].patflag && *tptr == ESCCHR)	{	/* if pattern & escape with following char */
					if (isdigit(c = *(tptr+1)) && c != '0' || c == '&') {	/* if special replacement	*/
						if (!rg->rep[rg->reptot].len)	 {	/* if haven't been building an ordinary string */
							if (c == '&')
								c = '0';	// capture group 0 is whole string
							if ((rg->rep[rg->reptot].index = c - '0') > regex_groupcount(lg->lsarray[0].regex))	 { /* put in index; if not that many subexpressions */
								senderr(ERR_BADREPERR, WARN);
								return (NULL);
							}
							tptr += 2;	/* now points to char beyond string designator */
							if (*tptr == '+' || *tptr == '-')		  /* if want case change */
								rg->rep[rg->reptot].flag = *tptr++ == '+' ? 1 : -1;	  /* put in flag and skip over sign */
							rg->rep[rg->reptot].start = NULL;		/* indicates a special replacement */
						}
						break;	/* force up one component */
					}
					else {
						if (!*(tptr+1))		/* if dangling \ on end of line */
							continue;		/* will ignore */
						memmove(tptr, tptr+1, strlen(tptr));	/* shift over esc char */
					}
				}
				rg->rep[rg->reptot].len++;		/* count one char in replacement string */
			}
		}
	}
	rg->maxlen = FF->head.indexpars.recsize-1;
    return sort_setuplist(FF);		/* if can set up sort list */
}
#endif
/******************************************************************************/
static short cleanuprep(HWND hwnd)	/* cleans up after finishing replacements */

{
	HCURSOR ocurs;

	if (RX(hwnd,nptr))	{		/* if have some replacements */
		view_clearselect(RX(hwnd,lastindex)->vwind);
		ocurs = SetCursor(g_waitcurs);
		sort_resortlist(RX(hwnd,lastindex),RX(hwnd,nptr));		/* make new nodes */
		freemem(RX(hwnd,nptr));
		RX(hwnd,nptr) = NULL;
		view_redisplay(RX(hwnd,lastindex),0,VD_CUR|VD_IMMEDIATE);
		SetCursor(ocurs);
		return (TRUE);		/* did something */
	}
	return (FALSE);			/* did nothing */
}
