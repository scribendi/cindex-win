#include "stdafx.h"
#include "commands.h"
#include "errors.h"
#include "typestuff.h"
#include "util.h"
#include "text.h"
#include "files.h"
#include "print.h"
#include "strings.h"
#include "modify.h"

#define MAXTEXT 100000		/* max number of characters in rich edit control */

static MARGINCOLUMN t_mc = 
{		/* margins ... */
	72,		/* top */
	72,		/* bottom */
	72,		/* left */
	72,		/* right */
};

static TCHAR sfilters[] =
TEXT("Plain Text\0*.txt\0")\
TEXT("Rich Text Format (RTF)\0*.rtf\0");

static void tdomenu(HWND hwnd, HMENU hMenu, UINT item, BOOL fSystemMenu);	/* sets menus */
static BOOL tcreate(HWND hwnd,LPCREATESTRUCT cs);	/* initializes text window */
static void tactivate(HWND hwnd, BOOL fActive, HWND hwndActivate, HWND hwndDeactivate);		/* activates window */
static int tactivateonmouse(HWND hwnd, HWND topwind, UINT hitcode, UINT msg);	/* does right kind of activation on window click */
static void tsetfocus(HWND hwnd, HWND oldfocus);	/* gets focus shift message */
static void tkillfocus(HWND hwnd, HWND newfocus);	/* gets focus shift message */
static void tsetpage(HWND hwnd);		/* sets up after page setup */
static void getformrect(HWND hwnd, RECT * drect);		/* sets formatting rect for edit control */
static void tprint(HWND hwnd);	/* forms & prints text pages */
static void tsave(HWND hwnd);		/* saves text */
static INT_PTR CALLBACK tsavehook(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static DWORD CALLBACK esproc(DWORD_PTR cookie, unsigned char * buff,long wcount, long * written);
static void tselall(HWND hwnd);		/* selects all text */
static short tclose(HWND wptr);		/* closes text window */
static void tnotify(HWND hwnd, UINT msg, HWND hwndChild, UINT idChild); 
static void tsize(HWND hwnd, UINT state, int cx, int cy);	/* sizes */
static void tcommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);	/* does menu tasks */
static void dobuttons(HWND hwnd);		/* checks/sets sets text buttons */

/*********************************************************************************/
HWND txt_setwindow(TCHAR * title, RECT * drptr, TEXTPARS * tp)	/*  sets up text edit window */

{
	HWND hwnd;
	
	adjustWindowRect(g_hwclient,drptr,WS_CLIPCHILDREN,FALSE,WS_EX_MDICHILD);	/* now get window rect that has right client area */
	scaleRectForDpi(g_hwclient, drptr);
	hwnd = CreateWindowEx(WS_EX_MDICHILD|WS_EX_TOOLWINDOW,g_textclass,title,WS_CLIPCHILDREN,
		drptr->left, drptr->top, drptr->right, drptr->bottom,g_hwclient,NULL,g_hinst,tp);
	if (hwnd)	{
		SendMessage(EX(hwnd,hwed),EM_SETRECT,0,(LPARAM)&EX(hwnd,prect));	/* set formatting rectangle (need this here) */
		ShowWindow(hwnd,SW_SHOWNORMAL);
	}
	return (hwnd);
}
/*********************************************************************************/
void txt_append(HWND hwnd, TCHAR * string, CHARFORMAT2 *cfp)	/*  adds text to specified rich edit control */

{
	CHARRANGE cr;
	HWND ew;

	ew = EX(hwnd,hwed);
	cr.cpMin = cr.cpMax = -1;
	SendMessage(ew,EM_EXSETSEL,0,(LPARAM)&cr);	/* set selection pt to end */
	if (cfp)	/* if want formatting */
		SendMessage(ew,EM_SETCHARFORMAT,SCF_SELECTION,(LPARAM)cfp);	/* set changed style */
	else		/* default formatting */
		SendMessage(ew,EM_SETCHARFORMAT,SCF_SELECTION,(LPARAM)&EX(hwnd,df));	/* restore default format */
	SendMessage(ew,EM_REPLACESEL,TRUE,(LPARAM)string);
}
/*********************************************************************************/
void txt_appendctext(HWND hwnd, TCHAR * rtext)	/*  replaces selection with cindex styled text */

{
	CHARRANGE cr;
	CHARFORMAT2 cf;
	short codes;
	long baseheight, smallc, step;
	HWND ew;
	TCHAR tstring[MAXREC], *rptr,*tptr;

	ew = EX(hwnd,hwed);
#if 0
	SendMessage(ew,EM_HIDESELECTION,TRUE,0);	/* hide selection while fiddling */
	oldmask = SendMessage(ew,EM_SETEVENTMASK,0,ENM_NONE);	/* turn off selchange/update events */		
#endif
	cf = EX(hwnd,cf);			/* get default cindex text format */
	baseheight = cf.yHeight;	/* use for scaling */
	step = baseheight/5;
	smallc = 4*step;
	cr.cpMin = cr.cpMax = -1;
	SendMessage(ew,EM_EXSETSEL,0,(LPARAM)&cr);	/* set selection pt to end */
	SendMessage(ew,EM_SETCHARFORMAT,SCF_SELECTION,(LPARAM)&cf);	/* start with plain style */
	for (rptr = rtext, tptr = tstring; *rptr; rptr++)	{	/* prepare segments */
		if (iscodechar(*tptr = *rptr))	{
			*tptr = '\0';
			SendMessage(ew,EM_REPLACESEL,TRUE,(LPARAM)tstring);
			codes = *++rptr;	/* get code */
			if (*tptr == FONTCHR)	{	/* if a font spec */
//				if ((codes&FX_FONTMASK) >= FONTLIMIT)
//					senderr(ERR_INTERNALERR, WARN, "local font id too large");
				;
			}
			else {		/* a type style spec */
				if (codes&FX_OFF)	{	/* if turning off */
					if (codes&FX_BOLD)
						cf.dwEffects &= ~CFE_BOLD;
					if (codes&FX_ITAL)
						cf.dwEffects &= ~CFE_ITALIC;
					if (codes&FX_ULINE)
						cf.dwEffects &= ~CFE_UNDERLINE;
					if (codes&FX_SMALL)	{
//						cf.yHeight = cf.yOffset ? smallc : baseheight;
						cf.yHeight = baseheight;
						cf.dwEffects &= ~CFE_SMALLCAPS;
					}
#if 0
					if (codes&FX_SUPER || codes&FX_SUB)		{
						cf.yOffset = 0;
						cf.yHeight = cf.dwEffects&CFE_SMALLCAPS ?  smallc : baseheight;
					}
#else
					if (codes&FX_SUPER)
						cf.dwEffects &= ~CFE_SUPERSCRIPT;
					if (codes&FX_SUB)
						cf.dwEffects &= ~CFE_SUBSCRIPT;
#endif
				}
				else {					/* turning on */
					if (codes&FX_BOLD)
						cf.dwEffects |= CFE_BOLD;
					if (codes&FX_ITAL)
						cf.dwEffects |= CFE_ITALIC;
					if (codes&FX_ULINE)
						cf.dwEffects |= CFE_UNDERLINE;
					if (codes&FX_SMALL)	{
						cf.yHeight = smallc;
						cf.dwEffects |= CFE_SMALLCAPS;
					}
#if 0
					if (codes&FX_SUPER)	{
						cf.yHeight = smallc;
						cf.yOffset = step;
					}
					if (codes&FX_SUB)	{	
						cf.yHeight = smallc;
						cf.yOffset = -step;
					}
#else
					if (codes&FX_SUPER)	{
						cf.dwEffects |= CFE_SUPERSCRIPT;
					}
					if (codes&FX_SUB)	{	
						cf.dwEffects |= CFE_SUBSCRIPT;
					}
#endif
				}
			}
			SendMessage(ew,EM_SETCHARFORMAT,SCF_SELECTION,(LPARAM)&cf);	/* set changed style */
			tptr = tstring;	/* reset buffer */
			continue;
		}
		tptr++;
	}
	*tptr = '\0';
	SendMessage(ew,EM_REPLACESEL,FALSE,(LPARAM)tstring);	/* add residual text */
#if 0
	SendMessage(ew,EM_SETEVENTMASK,0,oldmask);	/* restore event mask */
	SendMessage(ew,EM_HIDESELECTION,FALSE,0);	/* show selection again */
#endif
}
/************************************************************************/
void txt_clearall(HWND hwnd)		/* clears text */

{
	HWND ew;
	int oldmask;

	ew = EX(hwnd,hwed);
	oldmask = SendMessage(ew,EM_SETEVENTMASK,0,ENM_NONE);	/* turn off selchange/update events */		
	Edit_SetSel(ew,0,-1);		/* select all */
	SendMessage(ew,EM_SETCHARFORMAT,0,(LPARAM)&EX(hwnd,cf));	/* set default style */
	SendMessage(ew,WM_CLEAR,0,0);	/* clear */
	Edit_SetSel(ew,0,0);
	SendMessage(ew,EM_SETPARAFORMAT,0,(LPARAM)&EX(hwnd,pf));	/* set para formatting */
	SendMessage(ew,EM_SETEVENTMASK,0,oldmask);	/* restore event mask */		
}
/************************************************************************/
void txt_setparaformat(HWND hwnd, PARAFORMAT2 * pf)	/* sets paragraph format */

{
	HWND ew;

	ew = EX(hwnd,hwed);
	EX(hwnd,pf) = *pf;
	Edit_SetSel(ew,0,-1);
	SendMessage(ew,EM_SETPARAFORMAT,0,(LPARAM)pf);	/* set para formatting */
}
/************************************************************************/
void txt_findpara(HWND ew, int curpos, int *selstart, int *selend, int *line, BOOL mflag)	/* finds para limits */

{
	FINDTEXT ft;

	*line = SendMessage(ew,EM_EXLINEFROMCHAR,0,curpos);	/* get line it's in */
	while (*line)	{	/* check if first line of para */
		int linestart = Edit_LineIndex(ew,*line);

		if (mod_getcharatpos(ew,linestart-1) == RETURN)
			break;
		(*line)--;
	}
	*selstart = Edit_LineIndex(ew,*line);
	ft.chrg.cpMin = curpos;	/* find end of para */
	ft.chrg.cpMax = -1;
	ft.lpstrText = TEXT("\r");
	*selend = SendMessage(ew,EM_FINDTEXT,FR_DOWN,(LPARAM)&ft);
	if (mflag)	{
		Edit_SetSel(ew,*selstart,*selend);	/* select para */
		SendMessage(ew,EM_SCROLLCARET,0,0);	// make sure viewable
	}
}
/************************************************************************/
int txt_selectpara(HWND ew, int shift)	/* selects -1,0,1 para, returns first line */

{
	int selstart, selend, line;
	CHARRANGE cr;

	SendMessage(ew,EM_EXGETSEL,0,(LPARAM)&cr);
	if (shift)	{
		txt_findpara(ew,cr.cpMin,&selstart,&selend,&line,FALSE);
		cr.cpMin = shift >0 ? selend + 2 : selstart -2;
		if (cr.cpMin < 0)
			cr.cpMin = 0;
	}
	txt_findpara(ew,cr.cpMin,&selstart,&selend,&line,TRUE);
	return (line);
}
/************************************************************************/
LRESULT CALLBACK txt_proc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)

{
	switch (msg)	{
		HANDLE_MSG(hwnd,WM_CREATE,tcreate);
		HANDLE_MSG(hwnd,WM_SIZE,tsize);
		HANDLE_MSG(hwnd,WM_INITMENUPOPUP,tdomenu);
		HANDLE_MSG(hwnd,WM_COMMAND,tcommand);
		HANDLE_MSG(hwnd,WM_NOTIFY,txt_notify);
		HANDLE_MSG(hwnd,WM_MDIACTIVATE,tactivate);
		HANDLE_MSG(hwnd,WM_MOUSEACTIVATE,tactivateonmouse);
		HANDLE_MSG(hwnd,WM_SETFOCUS,tsetfocus);
		HANDLE_MSG(hwnd,WM_KILLFOCUS,tkillfocus);
		HANDLE_MSG(hwnd,WM_PARENTNOTIFY,tnotify);
		case (WM_CLOSE):
			return (tclose(hwnd));
		case WM_HELP:
			dowindowhelp(EX(hwnd,hcontext));
			return (0);
		case WM_CONTEXTMENU:
			return (0);				/* stops recursive call through frameproc, via DefMDIChildProc */
	}
	return (DefMDIChildProc(hwnd,msg,wParam,lParam));
}
/************************************************************************/
static void tcommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)	/* does menu tasks */

{
	switch (id)	{
		case IDM_FILE_CLOSE:
			tclose(hwnd);
			return;
		case IDM_FILE_SAVE:
		case IDM_FILE_SAVEAS:
			tsave(hwnd);
			return;
		case IDM_FILE_PAGESETUP:
			tsetpage(hwnd);
			return;
		case IDM_FILE_PRINT:
			tprint(hwnd);
			return;
		case IDM_EDIT_UNDO:
			SendMessage(EX(hwnd,hwed),EM_UNDO,0,0);
			return;
		case IDM_EDIT_REDO:
			SendMessage(EX(hwnd,hwed),EM_REDO,0,0);
			return;
		case IDM_EDIT_CUT:
			SendMessage(EX(hwnd,hwed),WM_CUT,0,0);
			return;
		case IDM_EDIT_COPY:
			SendMessage(EX(hwnd,hwed),WM_COPY,0,0);
			return;
		case IDM_EDIT_PASTE:
			SendMessage(EX(hwnd,hwed),WM_PASTE,0,0);
			return;
		case IDM_EDIT_CLEAR:
			SendMessage(EX(hwnd,hwed),WM_CLEAR,0,0);
			return;
		case IDM_EDIT_SELECTALL:
			tselall(hwnd);
			return;
		case IDM_STYLE_REGULAR:
		case IDM_STYLE_BOLD:
		case IDM_STYLE_ITALIC:
		case IDM_STYLE_UNDERLINE:
		case IDM_STYLE_SMALLCAPS:
		case IDM_STYLE_SUPER:
		case IDM_STYLE_SUB:
		case IDM_STYLE_INITIALCAPS:
		case IDM_STYLE_UPPERCASE:
		case IDM_STYLE_LOWERCASE:
			mod_dostyle(EX(hwnd,hwed),&EX(hwnd,cf),id);
			return;
	}
	FORWARD_WM_COMMAND(hwnd,id,hwndCtl,codeNotify,DefMDIChildProc);
}
/************************************************************************/
int txt_notify(HWND hwnd, int id, NMHDR * hdr)	/* does notification tasks */

{
	if (id == RE_ID)	{

		switch (hdr->code)	{
			case EN_SELCHANGE:
				SendMessage(hdr->hwndFrom,EM_SCROLLCARET,0,0);	/* make sure caret visible */
				dobuttons(hwnd);		/* check text buttons */
				return (FALSE);
			case EN_MSGFILTER:
				switch (((MSGFILTER *)hdr)->msg)	{
					case WM_LBUTTONDOWN:
						;
				}
		}
	}
	return FORWARD_WM_NOTIFY(hwnd,id,hdr,DefMDIChildProc);
}
/************************************************************************/
static void tdomenu(HWND hwnd, HMENU hMenu, UINT item, BOOL fSystemMenu)	/* sets menus */

{
	CHARRANGE cr;
	HWND ew = EX(hwnd,hwed);

	com_setenable(ETEXT,XONLY,ON);		/* enable set for text */
	if (IsWindowEnabled(ew))	{
		com_set(IDM_EDIT_SELECTALL,ON);	/* enable select all */
		SendMessage(ew,EM_EXGETSEL,0,(LPARAM)&cr);	/* get selection info */
		if (SendMessage(ew,EM_GETOPTIONS,0,0)&ECO_READONLY)		/* if read only */
			com_set(IDM_EDIT_COPY,cr.cpMin != cr.cpMax);	/* enable copy by selection */
		else	{
			com_setenable(ESCONT,ONLY, cr.cpMin != cr.cpMax);	/* enable cut copy clear by selection */
			com_set(IDM_EDIT_PASTE,SendMessage(ew,EM_CANPASTE,0,0));	/* enable if can paste */
			com_set(IDM_EDIT_UNDO,SendMessage(ew,EM_CANUNDO,0,0));	/* enable if can undo */
			com_set(IDM_EDIT_REDO,SendMessage(ew,EM_CANREDO,0,0));	/* enable if can redo */
		}
	}
}
/******************************************************************************/
static BOOL tcreate(HWND hwnd,LPCREATESTRUCT cs)	/* initializes text window */

{
	EFLIST * tflist;
	INDEX * FF;
	int height;
	TEXTPARS * tp;

	if (tflist = getmem(sizeof(EFLIST)))	{	/* if can get memory for our window structure */
		setdata(hwnd,tflist);
		tp = getMDIparam(cs);
		if (WX(hwnd,owner) = FF = tp->owner)	{	/* recover param passed (FF) */
			if (FF->twind)		/* if already have a text window */
				SendMessage(FF->twind,WM_CLOSE,0,0);	/* close it */
		}
		tflist->hwp = tp->hwp;	/* address of ultimate handle */
		tflist->hcontext = tp->helpcontext;
		*tflist->hwp = hwnd;	/* set handle */
		tflist->mc = t_mc;		/* copy margin and column settings */
		tflist->pi = g_prefs.formpars.pf.pi;	/* and paper settings */
		getformrect(hwnd,&tflist->prect);		/* get formatting rectangle for page */
		scalerect(&tflist->prect,g_slpix,72);
		// now install header control
		tflist->hwh = CreateWindowEx(0,WC_HEADER,NULL,WS_CHILD|HDS_HORZ/*|HDS_NOSIZING */|tp->headerstyle,0,0,0,0,hwnd,(HMENU)HD_ID,g_hinst,0);
//		SendMessage(tflist->hwh,WM_SETFONT,(WPARAM)GetStockObject(DEFAULT_GUI_FONT),0);
		SendMessage(tflist->hwh, WM_SETFONT, (WPARAM)CreateFontIndirect(&getconfigdpi(hwnd)->lfStatusFont), MAKELPARAM(FALSE, 0));
		if (tp->hitems)	{	// set header items
			int index = 0;
			while (tp->hitems->title)	{
				HDITEM hdi; 
				
				hdi.mask = HDI_TEXT | HDI_FORMAT | HDI_WIDTH; 
				hdi.pszText = tp->hitems->title; 
				hdi.cxy = scaleForDpi(tflist->hwh,tp->hitems->width);
				hdi.cchTextMax = nstrlen(tp->hitems->title); 
				hdi.fmt = HDF_LEFT | HDF_STRING |tp->hitems->format; 
				SendMessage(tflist->hwh, HDM_INSERTITEM,(WPARAM)index++, (LPARAM) &hdi); 
				tp->hitems++;
			}
		}
		/* now install rich edit */
		tp->style |= WS_VSCROLL|ES_AUTOVSCROLL|/* WS_HSCROLL| */WS_CHILD|WS_VISIBLE|ES_MULTILINE|ES_DISABLENOSCROLL|WS_CLIPSIBLINGS|ES_SAVESEL|WS_CLIPCHILDREN;
		tflist->hwed = CreateWindowEx(0,RICHEDIT_CLASS,NULL,tp->style,0,0,0,0,hwnd,(HMENU)RE_ID,g_hinst,0);
		SendMessage(tflist->hwed, EM_SETTYPOGRAPHYOPTIONS, TO_ADVANCEDTYPOGRAPHY, TO_ADVANCEDTYPOGRAPHY);	// allows right tabs ?
		SendMessage(tflist->hwed,EM_EXLIMITTEXT,0, MAXTEXT);		/* set limit on size of control */
		tflist->df.cbSize = sizeof(tflist->df);
		SendMessage(tflist->hwed,EM_GETCHARFORMAT,TRUE,(LPARAM)&tflist->df);	/* recover default info */
		tflist->cf.cbSize = sizeof(tflist->cf);
		tflist->cf.bPitchAndFamily = DEFAULT_PITCH;
		tflist->cf.bCharSet = DEFAULT_CHARSET;
		tflist->df.yHeight = scaleForDpi(tflist->hwed, tflist->df.yHeight);
//		nstrcpy(tflist->cf.szFaceName,FF ? toNative(FF->head.fm[0].name) : toNative(g_prefs.gen.fm[0].name));
//		nstrcpy(tflist->cf.szFaceName, FF ? toNative("Arial") : toNative("Arial"));
		nstrcpy(tflist->cf.szFaceName, TEXT("Arial"));
		height = FF ? FF->head.privpars.size : g_prefs.privpars.size;
		height *= scaleForDpi(tflist->hwed, TWIPS_PER_POINT);
//		if (height > tflist->df.yHeight)
//			height = tflist->df.yHeight;
		tflist->cf.yHeight = height > tflist->df.yHeight ? tflist->df.yHeight : height;
		tflist->cf.dwMask = CFM_BOLD|CFM_CHARSET|CFM_FACE|CFM_SIZE|CFM_ITALIC|CFM_SUPERSCRIPT|CFM_SUBSCRIPT|CFM_UNDERLINE|CFM_SMALLCAPS;
		if (tp->hook)	/* if subclassing */
			(LONG_PTR)tflist->tproc = SetWindowLongPtr(hwnd, GWLP_WNDPROC,(LONG_PTR)tp->hook);		/* set subclass handler */
		SetFocus(tflist->hwed);
		return (TRUE);
	}
	return (FALSE);
}
/******************************************************************************/
static void tactivate(HWND hwnd, BOOL fActive, HWND hwndActivate, HWND hwndDeactivate)		/* activates window */

{
	if (hwnd == hwndActivate)	{	/* if being activated */
#if 0
		SendMessage(g_hwtoolbar,TB_ENABLEBUTTON,IDM_FILE_NEW,TRUE);
		SendMessage(g_hwtoolbar,TB_ENABLEBUTTON,IDM_FILE_OPEN,TRUE);
		SendMessage(g_hwtoolbar,TB_ENABLEBUTTON,IDM_FILE_SAVE,TRUE);	/* set button */
		SendMessage(g_hwtoolbar,TB_ENABLEBUTTON,IDM_FILE_PRINT,TRUE);	/* set button */
		SendMessage(g_hwtoolbar,TB_ENABLEBUTTON,IDM_EDIT_FIND,FALSE);		/* set button */
		SendMessage(g_hwtoolbar,TB_ENABLEBUTTON,IDM_EDIT_REPLACE,FALSE);	/* set button */
		SendMessage(g_hwtoolbar,TB_ENABLEBUTTON,IDM_EDIT_NEWRECORD,FALSE);
		SendMessage(g_hwtoolbar,TB_ENABLEBUTTON,IDM_EDIT_DELETE,FALSE);
		SendMessage(g_hwtoolbar,TB_ENABLEBUTTON,IDM_EDIT_TAG,FALSE);
		SendMessage(g_hwtoolbar,TB_ENABLEBUTTON,IDM_VIEW_ALLRECORDS,FALSE);
		SendMessage(g_hwtoolbar,TB_ENABLEBUTTON,IDM_VIEW_FULLFORMAT,FALSE);
		SendMessage(g_hwtoolbar,TB_ENABLEBUTTON,IDM_VIEW_DRAFTFORMAT,FALSE);
		SendMessage(g_hwtoolbar,TB_ENABLEBUTTON,IDB_VIEW_INDENTED,FALSE);
		SendMessage(g_hwtoolbar,TB_ENABLEBUTTON,IDB_VIEW_RUNIN,FALSE);
		SendMessage(g_hwtoolbar,TB_ENABLEBUTTON,IDB_VIEW_SORTALPHA,FALSE);
		SendMessage(g_hwtoolbar,TB_ENABLEBUTTON,IDB_VIEW_SORTPAGE,FALSE);
		dobuttons(hwnd);
#endif
		com_settextmenus(getowner(hwnd),OFF,OFF);
	}
	FORWARD_WM_MDIACTIVATE(hwnd,fActive,hwndActivate, hwndDeactivate,DefMDIChildProc);
}
/*******************************************************************************/
static int tactivateonmouse(HWND hwnd, HWND topwind, UINT hitcode, UINT msg)	/* does right kind of activation on window click */

{
	if (hitcode == HTCLIENT && getmdiactive(hwnd))	{/* if first new hit in client area */
		setmdiactive(hwnd,FALSE);		// clear discard first click
		return (MA_ACTIVATEANDEAT);
	}
	return (MA_ACTIVATE);
}
/************************************************************************/
static void tsetfocus(HWND hwnd, HWND oldfocus)	/* gets focus shift message */

{
	if (getdata(hwnd))	{	/* if we're receiving focus */
		SetFocus(EX(hwnd,hwed));	/* need to trap this so that we can use keys, etc */
		dobuttons(hwnd);
	}
	FORWARD_WM_SETFOCUS(hwnd,oldfocus,DefMDIChildProc);
}
/************************************************************************/
static void tkillfocus(HWND hwnd, HWND newfocus)	/* gets focus shift message */

{
	com_setdefaultbuttons(FALSE);
	FORWARD_WM_KILLFOCUS(hwnd,newfocus,DefMDIChildProc);
}
/******************************************************************************/
static void tsetpage(HWND hwnd)		/* sets up after page setup */

{
	RECT drect;

	if (print_setpage(&EX(hwnd,mc),&EX(hwnd,pi)))	{
		getformrect(hwnd,&drect);
		scalerect(&drect,g_slpix,72);
		EX(hwnd,prect) = drect;		/* set formatting rectangle */
		SendMessage(EX(hwnd,hwed),EM_SETRECT,0,(LPARAM)&drect);	/* set formatting rectangle */
	}
}
/******************************************************************************/
static void getformrect(HWND hwnd, RECT * drect)		/* sets formatting rect for edit control */

{
	drect->left = drect->top = 0;
	drect->right = EX(hwnd,pi).pwidthactual-(EX(hwnd,mc).right+EX(hwnd,mc).left);
	drect->bottom = EX(hwnd,pi).pheightactual-(EX(hwnd,mc).top+EX(hwnd,mc).bottom);
}
/******************************************************************************/
static void tprint(HWND hwnd)	/* forms & prints text pages */

{
	DWORD flags;
	CHARRANGE cr;
	FORMATRANGE fr;
	HWND ew;
	TCHAR tstring[STSTRING];
	int index, tottext,pcount, firstpage, lastpage;
	RECT prect;
	GETTEXTLENGTHEX exl;
	
	ew = EX(hwnd,hwed);
	SendMessage(ew,EM_EXGETSEL,0,(LPARAM)&cr);	/* get selection info */
	GetWindowText(hwnd,tstring,STSTRING);
	flags = cr.cpMin != cr.cpMax ? 0 : PD_NOSELECTION;
	if (print_begin(flags, tstring,&EX(hwnd,pi),ew) > 0)	{	/* if OK */
		p_dc = p_dlg.hDC;		// need this for text metrics
//		SetMapMode(p_dlg.hDC,MM_TEXT);
		memset(&fr,0,sizeof(fr));
		fr.rcPage.right = EX(hwnd,pi).pwidthactual;
		fr.rcPage.bottom = EX(hwnd,pi).pheightactual;
		scalerect(&fr.rcPage,TWIPS_PER_POINT,1);
		getformrect(hwnd,&prect);		/* get size */
		OffsetRect(&prect,-EX(hwnd,pi).xoffset,-EX(hwnd,pi).yoffset);	/* offsets for unprintable area */
		fr.rc = prect;
		OffsetRect(&fr.rc,EX(hwnd,mc).left,EX(hwnd,mc).top);	/* positions page rect for margins */
		scalerect(&fr.rc,TWIPS_PER_POINT,1);	/* scale to twips */
		fr.hdc = fr.hdcTarget = p_dlg.hDC;
		firstpage = 1;					/* default all pages */
		lastpage = SHRT_MAX;
		if (p_dlg.Flags&PD_SELECTION)	/* selected records */
			fr.chrg = cr;
		else
			fr.chrg.cpMax = -1;		/* whole text (start already initialized) */
		if (p_dlg.Flags&PD_PAGENUMS) {	/* if want specified pages */
			firstpage = p_dlg.nFromPage;
			lastpage = p_dlg.nToPage;
		}
		exl.flags = GTL_DEFAULT;
		exl.codepage = 1200;
		tottext = SendMessage(ew,EM_GETTEXTLENGTHEX,(WPARAM)&exl,0);
		for (pcount = 1, index = 0; pcount <= lastpage && index < tottext && p_err >0; pcount++)	{
			if (pcount >= firstpage)	{
				if ((p_err = StartPage(p_dlg.hDC)) <= 0)
					break;
				pr_defaulthead(p_dlg.hDC,tstring,pcount,&EX(hwnd,mc),&prect);
			}
			index = SendMessage(ew,EM_FORMATRANGE,pcount >= firstpage, (LPARAM)&fr);
			fr.chrg.cpMin = index;
			if (pcount >= firstpage)	{
				if ((p_err = EndPage(p_dlg.hDC)) <= 0)	/* close page */
					break;
			}
		}
		SendMessage(ew,EM_FORMATRANGE,0,(LPARAM)NULL);	/* cleanup caches */
		print_end(TRUE,ew);
	}
}
/*******************************************************************************/
static void tsave(HWND hwnd)		/* saves text */

{
	TCHAR path[_OFN_NAMELEN], title[_MAX_FNAME+_MAX_EXT];
	HCURSOR ocurs;
	OPENFILENAME ofn;
	HANDLE fid;
	EDITSTREAM es;
	int err;
	
	memset(&ofn,0,sizeof(ofn));
	*path = '\0';
	*title = '\0';
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = g_hwframe;
	ofn.lpstrFilter = sfilters;
	ofn.lpstrFile = path;		/* holds path on return */
	ofn.nMaxFile =MAX_PATH;
	ofn.lpstrFileTitle = title;	/* holds file title on return */
	ofn.nMaxFileTitle = _MAX_FNAME+_MAX_EXT;
	ofn.Flags = OFN_OVERWRITEPROMPT|OFN_HIDEREADONLY|OFN_PATHMUSTEXIST
		|OFN_EXPLORER|OFN_ENABLEHOOK|OFN_NOCHANGEDIR|OFN_ENABLESIZING;
	ofn.lpfnHook = tsavehook;

	if (GetSaveFileName(&ofn))	{	/* if want to use */
		if ((fid = CreateFile(ofn.lpstrFile,GENERIC_READ|GENERIC_WRITE,0,NULL,OPEN_ALWAYS,FILE_FLAG_RANDOM_ACCESS,0)) != INVALID_HANDLE_VALUE)	{
			ocurs = SetCursor(g_waitcurs);
			es.dwCookie = (DWORD_PTR)fid;
			es.pfnCallback = esproc;
			es.dwError = 0;
			SendMessage(EX(hwnd,hwed),EM_STREAMOUT,ofn.nFilterIndex==1 ? SF_TEXT : SF_RTF,(LPARAM)&es);
			CloseHandle(fid);
			SetCursor(ocurs);
		}
	}
	err = CommDlgExtendedError();
}
/**********************************************************************************/	
static INT_PTR CALLBACK tsavehook(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)

{
	HWND parent;
	OPENFILENAME * ofp;

	parent = GetParent(hwnd);
	ofp = (OPENFILENAME*)lParam;
	switch (msg)	{

		case WM_INITDIALOG:
			centerwindow(parent,0);
			CommDlg_OpenSave_SetDefExt(parent, str_xatindex((char *)ofp->lpstrFilter,1)+2);
			return (TRUE);
		case WM_NOTIFY:
			ofp = ((LPOFNOTIFY)lParam)->lpOFN;
			if (((LPOFNOTIFY)lParam)->hdr.code == CDN_TYPECHANGE)
				CommDlg_OpenSave_SetDefExt(parent, str_xatindex((char *)ofp->lpstrFilter,(ofp->nFilterIndex-1)*2+1)+2);
	}
	return (FALSE);
}
/**********************************************************************************/	
static DWORD CALLBACK esproc(DWORD_PTR cookie, unsigned char * buff,long wcount, long * written)

{
	WriteFile((HANDLE)cookie,buff,wcount,written,NULL);
	return (*written < wcount ? 0 : (DWORD) *written);
}
/******************************************************************************/
void tselall(HWND hwnd)		/* selects all text */

{
	CHARRANGE cr;

	cr.cpMin = 0;
	cr.cpMax = -1;
	SendMessage(EX(hwnd,hwed),EM_EXSETSEL,0,(LPARAM)&cr);
}
/******************************************************************************/
static short tclose(HWND hwnd)		/* closes text window */

{
	*EX(hwnd,hwp) = NULL;	/* clear the ultimate handle */
	if (EX(hwnd,tproc))	/* if have subclassed */
		SetWindowLongPtr(hwnd, GWLP_WNDPROC,(LONG_PTR)EX(hwnd,tproc));	/* restore procedure */
	DestroyWindow(EX(hwnd,hwed));	/* destroy edit window */
	freedata(hwnd);		/* window now has nothing */
	SendMessage(g_hwclient,WM_MDIDESTROY,(WPARAM)hwnd,0);	/* close */
	return (TRUE);
}
/******************************************************************************/
static void tnotify(HWND hwnd, UINT msg, HWND hwndChild, UINT idChild) 

{
	if (msg == WM_LBUTTONDOWN)	{	// if left click
		HWND active = (HWND)SendMessage(g_hwclient,WM_MDIGETACTIVE,0,(LPARAM)NULL);
		if (active != hwnd)	// if active window isn't us
			setmdiactive(hwnd,TRUE);	// need discard first click
	}
	FORWARD_WM_PARENTNOTIFY(hwnd, msg, hwndChild, idChild, DefMDIChildProc);
}
/************************************************************************/
static void tsize(HWND hwnd, UINT state, int cx, int cy)

{
	RECT crect;
	EFLIST * efp;
	HDLAYOUT hdl;
	WINDOWPOS wp;

	if (efp = getdata(hwnd))	{	/* might be re-sized during destruction */
		GetClientRect(hwnd,&crect);
		hdl.prc = &crect;		// set up to size/position header
        hdl.pwpos = &wp; 
        SendMessage(efp->hwh, HDM_LAYOUT, 0, (LPARAM) &hdl); 
        SetWindowPos(efp->hwh, wp.hwndInsertAfter, wp.x, wp.y,wp.cx, wp.cy, wp.flags | SWP_SHOWWINDOW); 
//		GetWindowRect(efp->hwh,&hrect);		/* get header window rect */
//		crect.top += hrect.bottom-hrect.top;	/* and by toobar */
//		crect.top += wp.cy;
		MoveWindow(efp->hwed,crect.left,crect.top,crect.right-crect.left,crect.bottom-crect.top,TRUE);	/* size riched */
// (1/10/8)		SendMessage(efp->hwed,EM_SETRECT,0,(LPARAM)&efp->prect);	/* set formatting rectangle */
//		SendMessage(efp->hwed,EM_SETRECT,0,(LPARAM)&crect);	/* set formatting rectangle */
	}
	FORWARD_WM_SIZE(hwnd,state,cx,cy,DefMDIChildProc);
}
/******************************************************************************/
static void dobuttons(HWND hwnd)		/* checks/sets sets text buttons */

{	
	CHARRANGE cr;
	HWND ew;

	ew = EX(hwnd,hwed);
	if (IsWindowEnabled(ew))	{
//		SendMessage(g_hwtoolbar,TB_ENABLEBUTTON,IDM_FILE_SAVE,TRUE);	/* set button */
//		SendMessage(g_hwtoolbar,TB_ENABLEBUTTON,IDM_FILE_PRINT,TRUE);	/* set button */
		SendMessage(ew,EM_EXGETSEL,0,(LPARAM)&cr);	/* get selection info */
		SendMessage(g_hwtoolbar,TB_ENABLEBUTTON,IDM_EDIT_COPY,cr.cpMin < cr.cpMax);	/* set button */
		SendMessage(g_hwtoolbar,TB_ENABLEBUTTON,IDM_EDIT_CUT,!(SendMessage(ew,EM_GETOPTIONS,0,0)&ECO_READONLY) && cr.cpMin < cr.cpMax);	/* set button */
		SendMessage(g_hwtoolbar,TB_ENABLEBUTTON,IDM_EDIT_PASTE,SendMessage(ew,EM_CANPASTE,0,0));	/* set button */
	}
	else {
		SendMessage(g_hwtoolbar,TB_ENABLEBUTTON,IDM_EDIT_COPY,FALSE);	/* set button */
		SendMessage(g_hwtoolbar,TB_ENABLEBUTTON,IDM_EDIT_CUT,FALSE);	/* set button */
		SendMessage(g_hwtoolbar,TB_ENABLEBUTTON,IDM_EDIT_PASTE,FALSE);	/* set button */
	}
}
