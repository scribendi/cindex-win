#include "stdafx.h"
#include <htmlhelp.h>
#include "strings.h"
#include "commands.h"
#include "files.h"
#include "errors.h"
#include "typestuff.h"
#include "abbrev.h"
#include "util.h"
#include "text.h"
#include "print.h"
#include "modify.h"
#include "registry.h"

static const DWORD wh_abbid[] = {
	IDC_ABBREV_NAME, HIDC_ABBREV_NAME,
	0,0
};

struct nlist {
	struct nlist *next;
	char *name;
	char *string;
};

#define HTABSIZ 101     /* size of hash table */

enum {		/* view mode */
	A_BYNAME = 0,
	A_BYEXPR
};

#if 0
char *a_prefix = " (['\"“‘";		/* prefixes */
char *a_suffix = " )],.;:'\"”’";   /* suffixes that can trigger expansion */
#else
TCHAR a_prefix[] = {'\n',SPACE,'(','[','\"','\'',8220,8216,0};		/* prefixes */
TCHAR a_suffix[] = {SPACE,')',']',',','.',';',':','\"','\'', 8221,8217,0};		// suffixes that trigger expansion
#endif
short a_dirty;      			/* flag for changed abbreviation list */
short a_tot;					/* total number of abbreviations */
char a_abname[ABBNAMELEN+1];		/* name of currently selected abbrev */

TCHAR *NEWLSTRING = TEXT("\r");

static struct nlist * hashtab[HTABSIZ];
static struct nlist *lp;				/* pointer to previous node */

static HEADERITEM _abbrevheader[] = {
	{TEXT("Abbreviation"),80,0},
	{TEXT("Expansion"),1000,0},
	{NULL,0,0}
};
static TCHAR * a_abbhelp = TEXT("base\\Abbreviations.htm");
static TCHAR afilter[] = TEXT("Abbreviations (*.cbr)\0*.cbr\0");

static void builddisplay(HWND hwnd);		/* sorts abbrevs and displays text */
static void setnew(HWND hwnd);		/* sets up structure for new abbrev */
static HWND setwindow(void);			/* sets up abbrev window */
static LRESULT CALLBACK abbrevproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static void acommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);	/* does menu tasks */
static int anotify(HWND hwnd, int id, NMHDR * hdr);	/* does notification tasks */
static BOOL checkchange(ENPROTECTED * enp);		/* checks if change permissible */
static int expansionused(HWND ew);	/* returns length of unselected text */
static void adopaste(HWND hwnd);		/* does paste if poss */
static LRESULT checkdroppaste(RECALLBACK * reptr, int mode, TCHAR * string);	/* CALLBACK from riched ole; returns paste/drop error */
static LRESULT checkdropsite(RECALLBACK * reptr);	/* CALLBACK from riched ole; returns error */
static void advancefield(NMHDR * hdr);	/* advance field in display */
static void adomenu(HWND hwnd, HMENU hMenu, UINT item, BOOL fSystemMenu);	/* sets menus */
static void setstylemenu(CHARFORMAT2 * df, CHARFORMAT2 * cf);		/* checks items on style menu */
static void aactivate(HWND hwnd, BOOL fActive, HWND hwndActivate, HWND hwndDeactivate);		/* activates window */
static BOOL recoverabbrev(HWND hwnd);	/* recovers abbrevs */
static short aclose(HWND hwnd);	/* recovers abbrevs; closes window */
static INT_PTR CALLBACK abbproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static int abcomp(const void * pnp1, const void * pnp2); /* comparison function for qsort on hash table */
static struct nlist *lookup(register char *str);     /* returns pointer to abbrev struct, or NULL */
static struct nlist * entname (register char *name, char *string);    /* enters abbrev in hash table */
static void setabtitle(void);		/* sets window title */
static void deleteabbrev(char * name);		/* deletes abbrev */
static void deleteallabbrevs(void);		/* clears all abbreviations */
static BOOL writeabbrevs(TCHAR * path, short force, short cleardirty);	  /* writes abbrevs to file */
static unsigned hash(register char *str);           /* hashes string */
static void sortdisplay(HWND hwnd,int sorttype);	/* switches abbrev view mode */
static BOOL absaveas(BOOL copyflag);		/* saves the current abbreviations */
static void absetpath(TCHAR * path);	// saves path, and sets menu display

/******************************************************************************/
char *abbrev_checkentry(char *str)     /* returns pointer to string, or NULL */

{
	struct nlist *np;
	unsigned hindex;
	int mflag;

	for (hindex = hash(str), np = hashtab[hindex]; np; np = np->next) {		/* for all entries with this hash val */
		if ((mflag = strcmp(str, np->name)) < 0)		/* if lower than current entry in list */
			return (NULL);
		if (!mflag)
			return (np->string);
	}
	return (NULL);      /* not found */
}
/*******************************************************************************/
void abbrev_view(void)		/* displays abbreviations */

{
	if (g_abbw || setwindow())	{	/* if have window */
		sortdisplay(g_abbw,g_prefs.hidden.absort);		/* build display */
		setabtitle();			/* sets title on window */
		SendMessage(g_hwclient,WM_MDIACTIVATE,(WPARAM)g_abbw,0);	/* make sure it's active */
	}
}
/*******************************************************************************/
static void setabtitle(void)		/* sets window title */

{
	TCHAR path[MAX_PATH], tstring[STSTRING];

	file_getuserpath(path,ALIS_ABBREV);	/* get current abbrev path */
	u_sprintf(tstring,"Abbreviations: %S",GetFileAttributes(path) != INVALID_FILE_ATTRIBUTES ? file_getname(path) : TEXT("Unsaved"));
	SetWindowText(g_abbw,tstring);
}
/*******************************************************************************/
static void builddisplay(HWND hwnd)		/* sorts abbrevs and displays text */

{
	struct nlist * * sorttab, *np; 	  /* buffer for sorting */
	int tot, count, len, oldmask;
	TCHAR tstring[STSTRING];
	CHARFORMAT2 df, cf;
	CHARRANGE cr;
	HWND ew = EX(hwnd,hwed);
	
	if (sorttab = getmem(sizeof(struct nlist *)*a_tot))	{		/* if can get array */
		for (tot = count = 0; count < HTABSIZ; count++)	 {		/* for all cells in table */
			for (np = hashtab[count]; np; np = np->next, tot++) /* for all entries in list */
				sorttab[tot] = np;     /* enter in table */
		}
		qsort(sorttab, tot, sizeof(struct nlist *), abcomp);
		ShowWindow(ew,SW_HIDE);
		txt_clearall(hwnd);		/* remove text and set para formatting */
		df = EX(hwnd,df);		/* get default format */
		df.dwMask = CFM_BOLD|CFM_COLOR|CFM_FACE|CFM_SIZE|CFM_ITALIC|CFM_SUPERSCRIPT|CFM_SUBSCRIPT|CFM_UNDERLINE|CFM_SMALLCAPS|CFM_PROTECTED;		/* add protection for names */
		df.dwEffects |= CFE_PROTECTED;
		cf = EX(hwnd,cf);
		cf.dwMask |= CFM_PROTECTED;
		cf.dwEffects |= CFE_PROTECTED;
		oldmask = SendMessage(ew,EM_SETEVENTMASK,0,ENM_NONE);	/* disable notifications */
		for (count = 0; count < tot; count++)	{		/* for all abbrevs */
			len = u_sprintf(tstring, "%S",toNative(sorttab[count]->name));
			txt_append(hwnd,tstring,&df);		/* prompt in system font */
			txt_append(hwnd,TEXT("\t"),&cf);	/* tab and what follows in default font */
			u_sprintf(tstring,"%S",toNative(sorttab[count]->string));
			txt_appendctext(hwnd,tstring);
			if (count < tot-1)		/* if not last line */
				txt_append(hwnd,TEXT("\r"),&df);/* newline, etc, in system font */
		}
		freemem(sorttab);
		cr.cpMin = cr.cpMax = 0;
		SendMessage(ew,EM_EXSETSEL,0,(LPARAM)&cr);	/* set caret to start */
		SendMessage(ew,EM_SETEVENTMASK,0,oldmask);	/* set event mask */
		SendMessage(ew,EM_SETMODIFY,FALSE,0);		/* unmodified */
		ShowWindow(ew,SW_SHOW);
	}
}
/*******************************************************************************/
static void setnew(HWND hwnd)		/* sets up structure for new abbrev */

{
	CHARFORMAT2 df;
	CHARRANGE cr;
	int oldmask, length;
	HWND ew = EX(hwnd,hwed);

	oldmask = SendMessage(ew,EM_SETEVENTMASK,0,ENM_NONE);	/* disable notifications */
	df = EX(hwnd,df);		/* get system format */
	df.dwMask = CFM_BOLD|CFM_COLOR|CFM_FACE|CFM_SIZE|CFM_ITALIC|CFM_SUPERSCRIPT|CFM_SUBSCRIPT|CFM_UNDERLINE|CFM_SMALLCAPS|CFM_PROTECTED;		/* add protection for names */
	df.dwEffects |= CFE_PROTECTED;
	length = SendMessage(ew,WM_GETTEXTLENGTH,0,0);	/* find where we're adding */
	cr.cpMin = cr.cpMax = -1;
	SendMessage(ew,EM_EXSETSEL,0,(LPARAM)&cr);	/* set caret pos */
	if (mod_getcharatpos(ew,LEFTCHAR) != '\t')	/* if we're not sitting just beyond abbrev name */
		txt_append(hwnd,length ? TEXT("\rNEW\t") : TEXT("NEW\t"),&df);	/* newline stuff */
//	SendMessage(ew,EM_SETCHARFORMAT,SCF_SELECTION,(LPARAM)&EX(hwnd,cf));	/* start abbrev with plain style */
	SendMessage(ew,EM_EXGETSEL,0,(LPARAM)&cr);	/* get caret pos */
	cr.cpMin = SendMessage(ew,EM_LINEINDEX,-1,0);	/* start of line */
	cr.cpMax -= 1;	/* position before tab */
	SendMessage(ew,EM_EXSETSEL,0,(LPARAM)&cr);	/* select title */
	SendMessage(ew,EM_SETEVENTMASK,0,oldmask);	/* restore notifications */
}
/*******************************************************************************/
static HWND setwindow(void)			/* sets up abbrev window */

{
	RECT trect;
	PARAFORMAT2 pf;					/* paragraph format */
	HWND hwnd, ew;
	TEXTPARS tpars;

	SetRect(&trect,0,0,600,200);
	memset(&tpars,0,sizeof(tpars));
	tpars.hook = abbrevproc;
	tpars.hwp = &g_abbw;
	tpars.hitems = _abbrevheader;
	tpars.headerstyle = HDS_BUTTONS;
	if (hwnd = txt_setwindow(TEXT(""),&trect,&tpars))	{
		EFLIST * tfp = getdata(hwnd);

		memset(&pf,0, sizeof(pf));	/* set up for para formatting */
		pf.cbSize = sizeof(pf);
		pf.dwMask = PFM_OFFSET|PFM_TABSTOPS;
		pf.dxOffset = 80*TWIPS_PER_POINT;
		pf.cTabCount = 2;
		pf.rgxTabs[0] = 60*TWIPS_PER_POINT;
		pf.rgxTabs[1] = 120*TWIPS_PER_POINT;
		txt_setparaformat(hwnd,&pf);
		ew = tfp->hwed;
		SendMessage(ew,EM_EMPTYUNDOBUFFER,0,0);	/* disable undo */
		tfp->reole = rw_callback;
		tfp->reole.droptest = checkdroppaste;
		tfp->reole.dropsite = checkdropsite;
		tfp->reole.hwnd = hwnd;
		SendMessage(ew,EM_SETOLECALLBACK,0,(LPARAM)&tfp->reole);
		SendMessage(ew,EM_GETOLEINTERFACE,0,(LPARAM)&tfp->ole);
		tfp->ole->lpVtbl->QueryInterface(tfp->ole,&IID_ITextDocument, &tfp->itd);
		SendMessage(ew,EM_SETEVENTMASK,0,ENM_PROTECTED|ENM_KEYEVENTS|ENM_SELCHANGE|ENM_MOUSEEVENTS);	/* set event mask */		
	}
	return (hwnd);
}
/************************************************************************/
static LRESULT CALLBACK abbrevproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)

{
	switch (msg)	{
		HANDLE_MSG(hwnd,WM_NOTIFY,anotify);
		HANDLE_MSG(hwnd,WM_COMMAND,acommand);
		HANDLE_MSG(hwnd,WM_INITMENUPOPUP,adomenu);
		HANDLE_MSG(hwnd,WM_MDIACTIVATE,aactivate);
		case (WM_CLOSE):
			return(aclose(hwnd));
		case WM_HELP:
			dowindowhelp(a_abbhelp);
			return (0);
	}
	return (txt_proc(hwnd,msg,wParam,lParam));	/* default handling */
}
/************************************************************************/
static void acommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)	/* does commands */

{
	switch (id)	{
		case IDM_FILE_CLOSE:
			aclose(hwnd);
			return;
		case IDM_FILE_SAVE:
			if (recoverabbrev(hwnd))
				abbrev_save();
			return;
		case IDM_FILE_SAVEAS:
			if (recoverabbrev(hwnd))
				absaveas(FALSE);
			return;
		case IDM_FILE_SAVEACOPYAS:
			if (recoverabbrev(hwnd))
				absaveas(TRUE);
			return;
		case IDM_EDIT_PASTE:
			adopaste(hwnd);
			return;
		case IDM_EDIT_NEWABBREV:
			setnew(hwnd);
			return;
	}
	FORWARD_WM_COMMAND(hwnd,id,hwndCtl,codeNotify,txt_proc);
}
/************************************************************************/
static int anotify(HWND hwnd, int id, NMHDR * hdr)	/* does notification tasks */

{
	int oldmask;

	switch (hdr->code)	{
		case HDN_ITEMCLICK:	// header control item
			if (recoverabbrev(hwnd))		/* force recovery */
				sortdisplay(hwnd,((NMHEADER *)hdr)->iItem);
			return TRUE;
		case EN_SELCHANGE:
			oldmask = SendMessage(hdr->hwndFrom,EM_SETEVENTMASK,0,ENM_NONE);	/* turn off selchange/update events */		
			if (mod_getcharatpos(hdr->hwndFrom,LEFTCHAR) == '\t' && mod_getcharatpos(hdr->hwndFrom,RIGHTCHAR) == '\0')	/* if immediately beyond tab and at end of text */
				SendMessage(hdr->hwndFrom,EM_SETCHARFORMAT,SCF_SELECTION,(LPARAM)&EX(hwnd,cf));	// plain style
			SendMessage(hdr->hwndFrom,EM_SETEVENTMASK,0,oldmask);	/* turn on events */		
			return TRUE;		
		case EN_PROTECTED:
			if (((ENPROTECTED *)hdr)->msg == WM_KEYDOWN || ((ENPROTECTED *)hdr)->msg == WM_CHAR
				|| ((ENPROTECTED *)hdr)->msg == WM_PASTE || ((ENPROTECTED *)hdr)->msg == WM_CLEAR)
				if (checkchange((ENPROTECTED *)hdr))	/* if permissible */
					return (FALSE);
			return TRUE;		/* forbid all others */
		case EN_MSGFILTER:
			switch (((MSGFILTER *)hdr)->msg)	{
				case WM_KEYDOWN:
					if (((MSGFILTER *)hdr)->wParam == VK_RETURN)	{
						setnew(hwnd);
						return (TRUE);		/* discard */
					}
					return FALSE;
				case WM_CHAR:
					if (((MSGFILTER *)hdr)->wParam == VK_TAB)	{	/* advance field */
						advancefield(hdr);
						return (TRUE);
					}
					if (expansionused(hdr->hwndFrom) >= ABBREVLEN-1)	/* if too long */
						return (TRUE);		/* too long */
					return (FALSE);
				default:
					;
			}
	}
	return FORWARD_WM_NOTIFY(hwnd,id,hdr,txt_proc);	/* default notify handler */
}
/************************************************************************/
static BOOL checkchange(ENPROTECTED * enp)		/* checks if change permissible */

{
	int line, endpara;
	TEXTRANGE tr;
	CHARRANGE cr;
	TCHAR tstring[STSTRING], *tptr;

	SendMessage(((NMHDR *)enp)->hwndFrom,EM_EXGETSEL,0,(LPARAM)&cr);
	txt_findpara(((NMHDR *)enp)->hwndFrom,cr.cpMin,&tr.chrg.cpMin,&endpara,&line,FALSE);
	if ((enp->msg == WM_CLEAR || (enp->msg == WM_CHAR || enp->msg == WM_KEYDOWN) && (enp->wParam == VK_BACK || enp->wParam == VK_DELETE))
		&& cr.cpMin == tr.chrg.cpMin && cr.cpMax == endpara+1)
		return (TRUE);	/* allow deletion if whole para(s) selected */
	tr.lpstrText = tstring;
	tr.chrg.cpMax = tr.chrg.cpMin+ABBREVLEN+1;
	SendMessage(((NMHDR *)enp)->hwndFrom,EM_GETTEXTRANGE,0,(LPARAM)&tr);	/* recover enough to get tab */
	tptr = nstrchr(tstring,VK_TAB);
	if (cr.cpMin <= tr.chrg.cpMin + (tptr-tstring) && cr.cpMax <= tr.chrg.cpMin + (tptr-tstring))	{	/* if wholly in abbrev */
		if (cr.cpMin == cr.cpMax && (enp->wParam == VK_BACK && tr.chrg.cpMin == cr.cpMin || enp->wParam == VK_DELETE && tr.chrg.cpMin+(tptr-tstring) == cr.cpMax))	/* if would delete marginal character */
			return (FALSE);
		if (enp->wParam == VK_TAB || enp->msg == WM_CHAR && (nstrchr(a_prefix,enp->wParam) || nstrchr(a_suffix,enp->wParam)) || enp->wParam == VK_RETURN
			|| (tptr-tstring >= ABBNAMELEN && enp->wParam != VK_DELETE && enp->wParam != VK_BACK))	/* if would cause trouble */
			return (FALSE);		/* discard */
	}
	else if (cr.cpMin > tr.chrg.cpMin + (tptr-tstring) && cr.cpMax <= endpara) {	/* if wholly in expansion segment */
		if (cr.cpMin == cr.cpMax && (enp->wParam == VK_BACK && tr.chrg.cpMin+(tptr-tstring)+1 == cr.cpMin || enp->wParam == VK_DELETE && endpara == cr.cpMax))	/* if would delete marginal character */
			return (FALSE);
	}
	else	/* selection crosses segments */
		return (FALSE);
	return (TRUE);
}
/************************************************************************/
static int expansionused(HWND ew)	/* returns length of unselected text */

{
	int line, endpara;
	TEXTRANGE tr;
	CHARRANGE cr;
	TCHAR tstring[STSTRING], *tptr;

	SendMessage(ew,EM_EXGETSEL,0,(LPARAM)&cr);
	txt_findpara(ew,cr.cpMin,&tr.chrg.cpMin,&endpara,&line,FALSE);
	tr.lpstrText = tstring;
	tr.chrg.cpMax = tr.chrg.cpMin+ABBREVLEN+1;
	SendMessage(ew,EM_GETTEXTRANGE,0,(LPARAM)&tr);
	tptr = nstrchr(tstring,VK_TAB);
	return (endpara-tr.chrg.cpMin-(tptr-tstring)-(cr.cpMax-cr.cpMin));
}
/************************************************************************/
static void advancefield(NMHDR * hdr)	/* advance field in display */

{
	FINDTEXT ft;
	int findtab, findnl;

	SendMessage(hdr->hwndFrom,EM_EXGETSEL,0,(LPARAM)&ft.chrg);
	ft.chrg.cpMax = -1;
	ft.lpstrText = TEXT("\t");
	findtab = SendMessage(hdr->hwndFrom,EM_FINDTEXT,FR_DOWN,(LPARAM)&ft)+1;
	ft.lpstrText = NEWLSTRING;
	findnl = SendMessage(hdr->hwndFrom,EM_FINDTEXT,FR_DOWN,(LPARAM)&ft)+1;
	ft.chrg.cpMin = ft.chrg.cpMax = findtab < findnl ? findtab : findnl;
	SendMessage(hdr->hwndFrom,EM_EXSETSEL,0,(LPARAM)&ft.chrg);
}
/************************************************************************/
static void adopaste(HWND hwnd)		/* does paste if poss */

{
	int breaks, length;

	length = getcliplength(hwnd,&breaks);
	if (!breaks && length+expansionused(EX(hwnd,hwed)) < ABBREVLEN-1)
		SendMessage(EX(hwnd,hwed),WM_PASTE,0,0);
	else
		senderr(ERR_ABBBADPASTE,WARNNB);
}
/************************************************************************/
static LRESULT checkdroppaste(RECALLBACK * reptr, int mode, TCHAR * string)	/* CALLBACK from riched ole; returns paste/drop error */

{
	if (reptr->bcount || reptr->length+expansionused(EX(reptr->hwnd,hwed)) >= ABBREVLEN-1)
		return (ERR_ABBBADPASTE);
	return (0);
}
/******************************************************************************/
static LRESULT checkdropsite(RECALLBACK * reptr)	/* CALLBACK from riched ole; returns error */

{
	POINT pt;
	int curpos, line, startpos;
	HWND ew;
	TCHAR tstring[STSTRING], *tabptr;

	ew = EX(reptr->hwnd,hwed);
	GetCursorPos(&pt);
	ScreenToClient(ew,&pt);
	curpos = SendMessage(ew,EM_CHARFROMPOS,0,(LPARAM)&pt);	/* get char pos */
	line = Edit_LineFromChar(ew,curpos);
	startpos = Edit_LineIndex(ew,line);
	Edit_GetLine(ew,line,tstring,STSTRING);
	if (tabptr = nstrchr(tstring,'\t'))	{	/* if have a tab on the line */
		if (curpos-startpos > tabptr-tstring && reptr->length+nstrlen(tabptr+1) < ABBREVLEN-1)	/* if over decent drop zone */
			return (0);
	}
	return (TRUE);
}
/************************************************************************/
static void adomenu(HWND hwnd, HMENU hMenu, UINT item, BOOL fSystemMenu)	/* sets menus */

{
	CHARFORMAT2 cf;
	CHARRANGE cr;
	TEXTRANGE tr;
	HWND ew = EX(hwnd,hwed);
	TCHAR tstring[STSTRING], *tptr;
	int line, endpara, tflag;

	cf.cbSize = sizeof(cf);
	SendMessage(ew,EM_GETCHARFORMAT,TRUE,(LPARAM)&cf);	/* recover selection info */
	setstylemenu(&EX(hwnd,cf),&cf);
	com_setenable(EABBREV,XONLY,ON);	/* enable set for abbreviations */
//	com_check(IDM_VIEW_SHOWBYEXPANSION,g_prefs.hidden.absort);	/* set to show */
	SendMessage(ew,EM_EXGETSEL,0,(LPARAM)&cr);	/* get selection range */
	txt_findpara(ew,cr.cpMin,&tr.chrg.cpMin,&endpara,&line,FALSE);
	tr.lpstrText = tstring;
	tr.chrg.cpMax = tr.chrg.cpMin+ABBREVLEN+1;
	SendMessage(ew,EM_GETTEXTRANGE,0,(LPARAM)&tr);	/* recover enough to get tab */
	tptr = nstrchr(tstring,VK_TAB);
	if (cr.cpMin <= tr.chrg.cpMin + (tptr-tstring) && cr.cpMax <= tr.chrg.cpMin + (tptr-tstring))	/* if wholly in abbrev */
		tflag = -1;
	else if (cr.cpMin > tr.chrg.cpMin + (tptr-tstring) && cr.cpMax <= endpara)	/* if wholly in expansion segment */
		tflag = 1;
	else	/* selection crosses segments */
		tflag = 0;
	com_set(IDM_EDIT_CUT,tflag && cr.cpMin != cr.cpMax);	/* cut */
	com_set(IDM_EDIT_COPY,tflag && cr.cpMin != cr.cpMax);	/* copy */
	com_set(IDM_EDIT_CLEAR,tflag && cr.cpMin != cr.cpMax || cr.cpMin == tr.chrg.cpMin && cr.cpMax == endpara+2);	/* clear */
	com_set(IDM_EDIT_PASTE,tflag && SendMessage(ew,EM_CANPASTE,0,0));	/* enable if can paste */
	com_set(IDM_EDIT_UNDO,SendMessage(ew,EM_CANUNDO,0,0));	/* enable if can undo */
	com_set(IDM_EDIT_REDO,SendMessage(ew,EM_CANREDO,0,0));	/* enable if can redo */
}
/******************************************************************************/
static void setstylemenu(CHARFORMAT2 * df, CHARFORMAT2 * cf)		/* checks items on style menu */

{
	com_check(IDM_STYLE_BOLD,cf->dwEffects&CFE_BOLD && cf->dwMask&CFM_BOLD);	/* check by state */
	com_check(IDM_STYLE_ITALIC,cf->dwEffects&CFE_ITALIC && cf->dwMask&CFM_ITALIC);	/* check by state */
	com_check(IDM_STYLE_UNDERLINE,cf->dwEffects&CFE_UNDERLINE && cf->dwMask&CFM_UNDERLINE);	/* check by state */
	com_check(IDM_FONT_DEFAULT, cf->dwMask&CFM_FACE && !nstrcmp(df->szFaceName,cf->szFaceName));	/* check by state */
	com_check(IDM_STYLE_SMALLCAPS,cf->dwEffects&CFE_SMALLCAPS && cf->dwMask&CFM_SMALLCAPS);	/* check by state */
	com_check(IDM_STYLE_SUPER,cf->dwEffects&CFE_SUPERSCRIPT && cf->dwMask&CFM_SUPERSCRIPT);	/* check by state */
	com_check(IDM_STYLE_SUB,cf->dwEffects&CFE_SUBSCRIPT && cf->dwMask&CFM_SUBSCRIPT);	/* check by state */
	com_check(IDM_STYLE_REGULAR,!(cf->dwEffects&(CFE_BOLD|CFE_ITALIC|CFE_UNDERLINE|CFE_SMALLCAPS|CFE_SUPERSCRIPT|CFE_SUBSCRIPT)));
}
/******************************************************************************/
static void aactivate(HWND hwnd, BOOL fActive, HWND hwndActivate, HWND hwndDeactivate)		/* activates window */

{
	if (getdata(hwnd) && hwnd != hwndActivate)	{	/* if being deactivated */
		if (!recoverabbrev(hwnd))	/* if error recovering */
			return;					/* don't forward */
	}
	FORWARD_WM_MDIACTIVATE(hwnd,fActive,hwndActivate,hwndDeactivate,txt_proc);
}
/************************************************************************/
static BOOL recoverabbrev(HWND hwnd)	/* recovers abbrevs */

{
	FINDTEXT ft;
	TEXTRANGE tr;
	CHARRANGE cr;
	int findpos, oldmask;
	TCHAR abname[ABBNAMELEN*2], *eptr;
	char tstring[STSTRING];
	HWND ew = EX(hwnd,hwed);

	if (SendMessage(ew,EM_GETMODIFY,0,0))	{	/* if dirty */
		oldmask = SendMessage(ew,EM_SETEVENTMASK,0,ENM_NONE);	/* turn off selchange/update events */		
		SendMessage(ew,EM_EXGETSEL,0,(LPARAM)&cr);	/* get current selection */
		SendMessage(ew,EM_HIDESELECTION,TRUE,0);	/* hide selection while fiddling */
		deleteallabbrevs();
		tr.lpstrText = abname;
		ft.chrg.cpMin = 0;	/* find end of para */
		ft.chrg.cpMax = -1;
		ft.lpstrText = TEXT("\r");
		while ((findpos = SendMessage(ew,EM_FINDTEXT,FR_DOWN,(LPARAM)&ft)) != -1)	{	/* while we have hits */
			tr.chrg.cpMin = ft.chrg.cpMin;		/* start of range to get */
			tr.chrg.cpMax = ft.chrg.cpMin+ABBNAMELEN+2;			/* end of range (must be beyond tab) */
			SendMessage(ew,EM_GETTEXTRANGE,0,(LPARAM)&tr);
			eptr = nstrchr(abname,'\t');
			if (!eptr)		/* no name here */
				break;
			if (eptr-abname)	{	/* if have a name */
				*eptr++ = '\0';		/* terminate abbrev */
				tr.chrg.cpMin += eptr-abname;	/* start of range to select */
				tr.chrg.cpMax = findpos;		// end of range (include \r)
				SendMessage(ew,EM_EXSETSEL,0,(LPARAM)&tr.chrg);	/* set selection for recovery */
				if (mod_gettestring(ew,tstring,NULL,MREC_SELECT) > 1)	{	/* if have an expansion */
					char * key = fromNative(abname);
					if (lookup(key))	{	/* if we've defined it already */
						senderr(ERR_DUPABBREV,WARN,abname);
						return (FALSE);
					}
					else {
						if (!entname(key,tstring))
							return (FALSE);
						a_dirty = TRUE;				/* mark new */
					}
				}
			}
			ft.chrg.cpMin = findpos+1;			/* pass over target */
		}
		SendMessage(ew,EM_SETMODIFY,FALSE,0);	/* mark as unmodified */
		SendMessage(ew,EM_EXSETSEL,0,(LPARAM)&cr);	/* restore old selection */
		SendMessage(ew,EM_HIDESELECTION,FALSE,0);	/* make visible */
		SendMessage(ew,EM_SETEVENTMASK,0,oldmask);	/* restore selchange/update events */		
	}
	return (TRUE);
}
/************************************************************************/
static short aclose(HWND hwnd)	/* recovers abbrevs; closes window */

{
	if (recoverabbrev(hwnd))	{
		EFLIST * efp = getdata(hwnd);
		efp->itd->lpVtbl->Release(efp->itd);
		efp->ole->lpVtbl->Release(efp->ole);
		FORWARD_WM_CLOSE(hwnd,txt_proc);
		return (TRUE);
	}
	return (FALSE);
}
/*******************************************************************************/
void abbrev_makenew(HWND hwnd)	/* makes new abbrev */

{
	char abname[ABBNAMELEN+1], abtext[MAXREC];
	struct nlist *nptr;

	if (DialogBoxParam(g_hinst,MAKEINTRESOURCE(IDD_ABBREV),hwnd,abbproc,(LPARAM)abname))	{
		mod_getselection(hwnd,abtext,NULL);
		if (nptr = lookup(abname))	/* if exists */
			deleteabbrev(abname);	/* delete old abbrev */
		if (entname(abname,abtext))	{	/* enter it */
			a_dirty = TRUE;				/* mark new */
			if (g_abbw)				/* if have open window */
				builddisplay(g_abbw);	/* redisplay */
		}
	}
}
/******************************************************************************/
static INT_PTR CALLBACK abbproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)

{
	char * tsp = getdata(hwnd);
	HWND eh;
	struct nlist *nptr;

	
	switch (msg)	{

		case WM_INITDIALOG:
			setdata(hwnd,(void *)lParam);/* set data */
			eh = GetDlgItem(hwnd,IDC_ABBREV_NAME);
			SendMessage(eh,EM_SETLIMITTEXT,ABBNAMELEN,0);	/* limits text */
			SetFocus(eh);	/* set focus to text */
			centerwindow(hwnd,1);
			return FALSE;
		case WM_COMMAND:
			switch (LOWORD(wParam))	{
				case IDOK:
					getDItemText(hwnd,IDC_ABBREV_NAME,tsp,ABBNAMELEN+1);
					if (nstrpbrk(toNative(tsp),a_prefix) || nstrpbrk(toNative(tsp),a_suffix))	{	/* if ab contains forbidden char */
						senderr(ERR_BADABBREVERR,WARNNB);
						return (TRUE);
					}
					else if (nptr = lookup(tsp))	{
						if (!sendwarning(WARN_EXISTABBREV, tsp, nptr->string))
							return (TRUE);
					}
				case IDCANCEL:
					EndDialog(hwnd,LOWORD(wParam) == IDOK && *tsp ? TRUE : FALSE);
					return TRUE;
			}
			break;
		case WM_HELP:
			return (dodialoghelp(hwnd,TEXT("_Adding and Editing\\Add_usingabbreviations.htm"),(HELPINFO *)lParam,wh_abbid));
	}
	return FALSE;
}
/*******************************************************************************/
BOOL abbrev_hasactiveset(void)		// closes existing abrevviation set

{
	TCHAR path[MAX_PATH];

	if (file_getuserpath(path,ALIS_ABBREV))	/* if can get spec of current file */
		return TRUE;
	return FALSE;
}
/*******************************************************************************/
void abbrev_close(HWND hw)		// closes existing abrevviation set

{
	if (abbrev_checkok())	{
		deleteallabbrevs();
		if (g_abbw)
			aclose(g_abbw);
		absetpath(NULL);	/* no saved set */
	}
}
/*******************************************************************************/
short abbrev_checkok(void)	/* checks/saves abbrevs */

{
	short action;
	
	if (!g_abbw || recoverabbrev(g_abbw))	{	/* if no window or recovered properly */
		if (a_dirty)	{	/* if have unsaved abbreviations */
			if (!(action = savewarning(WARN_SAVEABBREV)))
				return (FALSE);		/* cancelled */
			else if (action > 0)	/* if want to save */
				return abbrev_save();
		}
	}
	return (TRUE);
}
/******************************************************************************/
BOOL abbrev_save(void)	  /* saves abbrevs */

{
	TCHAR path[MAX_PATH];

	if (file_getuserpath(path,ALIS_ABBREV))	/* if can get spec of current file */
		return writeabbrevs(path, FALSE,TRUE);
	else
		return absaveas(FALSE);
}
/*******************************************************************************/
static BOOL absaveas(BOOL copyflag)		/* saves the current abbreviations */

{
	TCHAR path[_OFN_NAMELEN], title[_MAX_FNAME+_MAX_EXT], dir[MAX_PATH];

	ofn.hwndOwner = g_hwframe;
	file_getdefpath(dir,FOLDER_ABR, TEXT(""));
	*path = '\0';
	*title = '\0';
	ofn.lpstrFilter = afilter;
	ofn.lpstrFile = path;		/* holds path on return */
	ofn.lpstrInitialDir = dir;
	ofn.lpstrFileTitle = title;	/* holds file title on return */
	ofn.lpstrTitle = TEXT("Save Abbreviations");
	ofn.Flags = OFN_OVERWRITEPROMPT|OFN_HIDEREADONLY|OFN_PATHMUSTEXIST
			|OFN_EXPLORER|OFN_ENABLEHOOK|OFN_NOCHANGEDIR|OFN_ENABLESIZING;
	ofn.lpstrDefExt = file_extensionfortype(FTYPE_ABBREV)+1;
	ofn.lpfnHook = file_generichook;
	ofn.hInstance = NULL;
	ofn.lCustData = FOLDER_ABR;
	ofn.lpTemplateName = NULL;

	if (GetSaveFileName(&ofn))	{	/* if want to use */
		if (writeabbrevs(path,TRUE,!copyflag) && !copyflag)	{	/* if can write, and not saving copy */
			absetpath(path);	/* save path */
			if (g_abbw)
				setabtitle();
		}
		return (TRUE);
	}
	return (FALSE);
}
/******************************************************************************/
static BOOL writeabbrevs(TCHAR * path, short force, short cleardirty)	  /* writes abbrevs to file */

{
	struct nlist ** sorttab, *np; 	  /* buffer for sorting */
	unsigned short count, ok;
	size_t tot;
	FILE *tempfptr;

	if (force || a_dirty)	{		/* if unconditional or dirty */
		ok = FALSE;
		if (sorttab = getmem(sizeof(struct nlist *)*a_tot))	{	/* if can get array */
			for (tot = count = 0; count < HTABSIZ; count++)	 {		 /* for all cells in table */
				for (np = hashtab[count]; np; np = np->next, tot++)     /* for all entries in list */
					sorttab[tot] = np;     /* enter in table */
			}
			qsort(sorttab, tot, sizeof(struct nlist *), abcomp);
			if (tempfptr = nfopen(path, TEXT("wb")))	{
				fputs("AAAA\r\n",tempfptr);	/* write abbrev identifier */
				for (count = 0; count < tot && !ferror(tempfptr); count++)
					fprintf(tempfptr, "%s\t%s\r\n",sorttab[count]->name,sorttab[count]->string); 
				if (!ferror(tempfptr))	{	/* if no errors */
					if (cleardirty)
						a_dirty = FALSE;
					ok = TRUE;
				}
				fclose(tempfptr);
			}
			freemem(sorttab);
		}
		return (ok);	/* TRUE if OK */
	}
	return (TRUE);
}
#if 0
/******************************************************************************/
BOOL abbrev_open(TCHAR * path, BOOL visflag)	 /* loads abbreviations from file */

{
	char sline[(ABBREVLEN+ABBNAMELEN+1)*2];	/* allow long path so we don't have to worry about encoded styles */
	char *sptr;
	FILE *tempfptr;
	BOOL err;
	unsigned long length;

	err = TRUE;
	if (abbrev_checkok()) 	{	 /* if ok clearing existing ones */
		if (tempfptr = nfopen(path,TEXT("rb"))) 	{
			deleteallabbrevs();
			str_getline(sline,(ABBREVLEN+ABBNAMELEN+1)*2,tempfptr,&length);	/* get and discard key id */
			while (str_getline(sline, (ABBREVLEN+ABBNAMELEN+1)*2, tempfptr,&length))	{	/* while strings to read */
				sptr = strchr(sline,'\t');	/* to end of name */
				*sptr++ = '\0';		 /* terminate first */
				if (!entname(sline, sptr))	   /* if error entering in table */
					break;		/* stop reading */
			}
			if (!ferror(tempfptr))   {	/* if ok */
				file_saveuserpath(path,ALIS_ABBREV);	/* save path */
#if 0
				if (g_abbw)		{	/* if window is open */
					builddisplay(g_abbw);	/* set up new set */
					SetWindowText(g_abbw,file_getname(path));
				}
#else
				if (visflag)	/* if want to see them */
					abbrev_view();
#endif
				err = FALSE;
			}
			fclose(tempfptr);
		}
		else	{	/* can't open file */
			senderr(ERR_ABBOPEN,WARN,path);
			file_saveuserpath(TEXT(""),ALIS_ABBREV);	/* make sure we don't persist on abbrevs */
		}
	}
	return (err);
}
#else
/******************************************************************************/
BOOL abbrev_open(TCHAR * path, BOOL visflag)	 /* loads abbreviations from file */

{
	char sline[(ABBREVLEN+ABBNAMELEN+1)*2];	/* allow long path so we don't have to worry about encoded styles */
	char *sptr;
	BOOL err;
	unsigned long length;

	err = TRUE;
	if (abbrev_checkok()) 	{	 /* if ok clearing existing ones */
		MFILE mf;
		if (mfile_open(&mf,path,GENERIC_READ,FILE_SHARE_READ,OPEN_EXISTING,0,0))	{
			deleteallabbrevs();
			str_setgetlimits(mf.base, mf.size);	// set up reader
			str_getline(sline,(ABBREVLEN+ABBNAMELEN+1)*2,&length);	/* get and discard key id */
			while (str_getline(sline, (ABBREVLEN+ABBNAMELEN+1)*2,&length))	{	/* while strings to read */
				sptr = strchr(sline,'\t');	/* to end of name */
				*sptr++ = '\0';		 /* terminate first */
				if (!entname(sline, sptr))	   /* if error entering in table */
					break;		/* stop reading */
			}
			absetpath(path);	/* save path */
#if 0
			if (g_abbw)		{	/* if window is open */
				builddisplay(g_abbw);	/* set up new set */
				SetWindowText(g_abbw,file_getname(path));
			}
#else
			if (visflag)	/* if want to see them */
				abbrev_view();
#endif
			err = FALSE;
			mfile_close(&mf);
		}
		else	{	/* can't open file */
			senderr(ERR_ABBOPEN,WARN,path);
			absetpath(NULL);	/* make sure we don't persist on abbrevs */
		}
	}
	return (err);
}
#endif
/******************************************************************************/
BOOL abbrev_new(TCHAR * path)	 /* starts new set of abbrevs */

{
	if (abbrev_checkok())	{	/* if ok saving existing */
		deleteallabbrevs();
		absetpath(path);	// set path
		writeabbrevs(path,TRUE, TRUE);		/* force creation of file */
		abbrev_view();			/* open window */
		return (TRUE);
	}
	return (FALSE);
}
/******************************************************************************/
static int abcomp(const void * pnp1, const void * pnp2) /* comparison function for qsort on hash table */

{
	char *fs1, *fs2;
	struct nlist ** np1, ** np2;
	static UChar str1[512], str2[512];
	UErrorCode error = U_ZERO_ERROR;
	int32_t olength;

	np1 = (struct nlist * *)pnp1;
	np2 = (struct nlist * *)pnp2;	
	if (g_prefs.hidden.absort == A_BYNAME) {		/* if want ordered by abbreviation */
		fs1 = (*np1)->name;
		fs2 = (*np2)->name;
	}
	else   {                     /* want ordered by string */
		fs1 = (*np1)->string;
		fs2 = (*np2)->string;
	}
	u_strFromUTF8(str1,sizeof(str1),&olength,fs1,-1,&error);
	error = U_ZERO_ERROR;
	u_strFromUTF8(str2,sizeof(str2),&olength,fs2,-1,&error);
	return u_strcasecmp(str1,str2,U_FOLD_CASE_DEFAULT);
}
/******************************************************************************/
static struct nlist *lookup(register char *str)     /* returns pointer to abbrev struct, or NULL */

{
	struct nlist *np;
	register unsigned short hindex;

	if (*str)	{
		for (hindex = hash(str), np = hashtab[hindex], lp = (struct nlist *)&hashtab[hindex]; np; lp = np, np = np->next)	/* for all entries with this has val */
			if (!strcmp(str, np->name))		   /* if match */
				return (np);
	}
	return (NULL);      /* not found */
}
/******************************************************************************/
static struct nlist * entname (register char *name, char *string)    /* enters abbrev in hash table */

{
	struct nlist * np;
	struct nlist * * tp;
	char * stemp;

	if (!(np = lookup(name)))  {       /* if no entry */
		if ((np = getmem(sizeof(struct nlist))) && (np->name = strdup(name)) && (np->string = strdup(string)))	{	/* if no memory */
			tp = &hashtab[hash(name)];        /* get address of table entry */
			while (*tp && strcmp(name, (*tp)->name) > 0)	   /* while names in list lower than new one */
				tp = &((*tp)->next);      /* advance through list to one that's greater than new one */
			np->next = *tp;        /* current entry comes after new one */
			*tp = np;              /* new one displaces current one */
			a_tot++;				/* count it */
			return (np);
		}
	}
	else if (stemp = strdup(string))    {	 /* if can duplicate */
		free(np->string);         /* get rid of old one */
		np->string = stemp;         /* install new one */
		return (np);
	}
	return (NULL);		/* error */
}
/******************************************************************************/
static void deleteabbrev(char * name)		/* deletes abbrev */

{
	struct nlist * np;

	if (np = lookup(name))  {       /* if no entry */
		lp->next = np->next;		/* put child in parent */
		free(np->name);
		free(np->string);   		/* remove entry  */
		freemem(np);
		a_tot--;
	}
	a_dirty = TRUE;
}
/******************************************************************************/
static void deleteallabbrevs(void)		/* clears all abbreviations */

{
	register unsigned short count;
	struct nlist *np, *nextp;

	for (count = 0; count < HTABSIZ;)   {       /* for all cells in table */
		for (np = hashtab[count]; np; np = nextp)     {     /* for all entries in list */
			nextp = np->next;
			free(np->name);
			free(np->string);   /* remove entry */
			freemem(np);         /* and structure */
		}
		hashtab[count++] = NULL;   /* clear base entry */
	}
	a_dirty = a_tot = 0;
}
/******************************************************************************/
static unsigned hash(register char *str)           /* hashes string */

{
	register unsigned short val;

	for (val = 0; *str; str++)
		val = *str +31*val;
	return (val%HTABSIZ);
}
/**********************************************************************************/	
static void sortdisplay(HWND hwnd, int sorttype)	/* switches abbrev view mode */

{	
//	if (recoverabbrev(hwnd))	{		/* force recovery */
		HWND hw = EX(hwnd,hwh);
		HDITEM hdi;
		int index;

		for (index = 0; index < 2; index++)	{
			hdi.mask = HDI_FORMAT; 
			hdi.fmt = HDF_LEFT | HDF_STRING;
			if (sorttype == index)
				hdi.fmt |= HDF_SORTUP;
			SendMessage(hw, HDM_SETITEM,index, (LPARAM) &hdi);
		}
		g_prefs.hidden.absort = sorttype;
		builddisplay(hwnd);		/* redisplay */
//	};
}
/******************************************************************************/
static void absetpath(TCHAR * path)	// saves path, and sets menu display

{
	TCHAR name[100] = TEXT("Close ");

	file_saveuserpath(path,ALIS_ABBREV);	// save path
	if (path)
		nstrcat(name,file_getname(path));
	com_setname(IDM_TOOLS_ABBREV_DETACH,name);
}
