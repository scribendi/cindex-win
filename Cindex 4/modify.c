#include "stdafx.h"
#include <htmlhelp.h>
#include <TOM.h>
#include "errors.h"
#include "records.h"
#include "sort.h"
#include "collate.h"
#include "search.h"
#include "refs.h"
#include "regex.h"
#include "commands.h"
#include "strings.h"
#include "util.h"
#include "modify.h"
#include "edit.h"
#include "viewset.h"
#include "formstuff.h"
#include "indexset.h"
#include "replaceset.h"
#include "spellset.h"
#include "findset.h"
#include "abbrev.h"
#include "registry.h"
#include "reole.h"
#include "macros.h"
#include "containerset.h"

#define smallcHeight(A) (4*(A/5))

#define KEY_EXTENDED 0x1000000

#define DEFINE_GUIDXXX(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
        EXTERN_C const GUID CDECL name \
                = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }

DEFINE_GUIDXXX(IID_ITextDocument,0x8CC497C0,0xA1DF,0x11CE,0x80,0x98,
                0x00,0xAA,0x00,0x47,0xBE,0x5D);


enum {
	BUTTONS_NONE = 0,
	BUTTONS_BASE = 1,
	BUTTONS_SELECT = 2,
	BUTTONS_STATE = 4,

	BUTTONS_ALL = (BUTTONS_BASE|BUTTONS_SELECT|BUTTONS_STATE)
};

enum {			/* flags for findfield */
	STARTFIELD = -1,
	ENDFIELD = -2
};

enum {		/* save rules */
	M_SAVE,
	M_ASK,
	M_DISCARD
};

enum {
	M_WORDCHAR = 1,
	M_PUNCT,
	M_CONTROL
};

enum {		/* alarm flags */
	AL_OFF,
	AL_WARN,
	AL_REQUIRED
};

enum {		/* alarm indices */
	A_TEMPLATE,	/* template */
	A_PAGE,		/* page */
	A_CROSS,	// cross ref
	A_CODE,		/* code */
	A_PAREN,	/* paren */
	A_SQBR,		/* square brackets */
	A_QUOTE,	// fancy quote
	A_DQUOTE,	// simple double quote

	ALARMTYPES	/* number of alarm types */
};

enum {		/* field errors */
	KEEPCS = 1,		/* bad code after ~ */
	ESCS,		/* bad code after \ */
	BRACES,		/* mismatched {} */
	BRACKETS,	/* mismatched <> */
	PAREN,		/* mismatched () */
	SQBR,		/* mismatched [] */
	QUOTE,		// mismatched fancy quotes
	DQUOTE,		// mismatched double quote
	CCODES		/* some incomplete code */
};

#define LMAPSIZE 100

typedef struct {
	char * base;
	int offset;
} REREC;

typedef struct {
	char * font;	/* font name string */
	int style;		/* style codes */
} RTSTYLE;

typedef struct mwstruct {	/* contains data/functions for window */
	WFLIST wstuff;			/* basic set of window functions */
	HWND hwed;				/* rich edit handle */
	RECALLBACK reole;		/* ole callback stuff */
	IRichEditOle* ole;		// ole
	ITextDocument * itd;	// iTextDocument
	CHARFORMAT2 df;			/* default character format */
	CHARFORMAT2 compf;		/* completion character format */
	CHARFORMAT2 curf;		// current format at selection pt
	PARAFORMAT2 pf;			/* paragraph format */
	HWND hwpr;				/* prompt edit */
	HWND hwst;				/* status window handle */
	HWND hwtb;				/* tab control at top */
	HWND hwstatic;			// static text
	RECN recnum;			/* number of record being edited */
	RECN prevnum;			/* number of previous */
	RECN nextnum;			/* number of next */
	TCHAR map[LMAPSIZE];	/* prompt map */
	int pageindex;			/* index of line on which page field starts */
	int protectindex;		//
	int fcount;				/* field count in editor */
//	TCHAR lastpastechar;	// last char dropped or pasted
	LRESULT (CALLBACK *edproc)(HWND, UINT, WPARAM, LPARAM );	/* saved riched procedure */
	char rstring[MAXREC];	/* copy of original source text for reversion */
	char revstring[MAXREC];	/* copy of current record text */
	short loffset;			/* left offset of dest rect in window */
	short propstate;		/* TRUE if propagating edit changes */
	short delstate;			/* TRUE if deleted */
	short tagvalue;			/* TRUE if labeled */
	short isovr;			/* TRUE if overwrite mode */
	short totlen;			/* # chars required to hold all info for record (including styles) */
	BOOL nocount;			/* true when want to inhibit whole record count */
	BOOL insertspace;		/* needs space inserted after change */
	short alarms[ALARMTYPES];	/* alarm flags for checks */
	struct numstruct * nptr;	/* sort list for changed records */
	short addflag;			/* true in conventional ADD mode */
	struct vcontext vcont;	/* initial context of view window */
	URegularExpression * regex[FIELDLIM];
	short statuschanged;	// some edit change to be noted
	short difflag;			// old/new text comparison
//	BOOL protectedSelection;	// true if selection change is within completion
	BOOL completingSelection;	// true if selection results from completion
	int startfield;			// field in which selection starts
	int endfield;			// field in which selection ends
	BOOL doingpastedrop;	// true when doing a paste/drop
	BOOL internalpaste;		// true when clipboard source within Cindex
	CHARRANGE currentselection;	// current selection
	FONTMAP fm[FONTLIMIT];	// font map
	HIMAGELIST imagelist;	// toolbar image list
	HIMAGELIST imagelistdisabled;	// toolbar image list (disabled)
} MFLIST;

#define MX(WW,II) (((MFLIST *)GetWindowLongPtr(WW,GWLP_USERDATA))->II)	/* for addressing items */

enum {	/* mod window child id's */
	IDM_TOOL = 200,
	IDM_STATUS,
	IDM_EDIT,
	IDM_PROMPT
};

static TCHAR NEWLINESTRING[] = TEXT("\r");
static char *m_string[] = {		/* mismatch table */
	"{...}",
	"<...>",
	"(...)",
	"[...]",
	"\223...\224",
	"\"...\"",
	"control codes"
};

static int statwidths[] = {	/* widths of segments of status window */
	37,		/* used */
	74,		/* free */
	180,	/* time */
	220,	/* user */
	270,	/* deleted */
	330,	/* labeled */
	380,	/* marked */
	410,	/* insert/overwrite */
	-1
};
#define STAT_NPARTS (sizeof(statwidths)/sizeof(int))

enum {		/* mod window button image ids */
	MB_PROPAGATE = 0,
	MB_BRACES,
	MB_BRACKETS,
	MB_FLIP,
	MB_HALFFLIP,
	MB_SWAPPAREN,
	MB_DEFFONT,
	MB_BOLD,
	MB_ITAL,
	MB_ULINE,
	MB_PREV,
	MB_NEXT,
	MB_DUPNEW,
	MB_REVERT,

	MB_MTOTAL		/* total number of buttons in mod set */
};

#define BASESEPARATOR 100
#define SEPARATOR 10
static TBBUTTON tbb[] = {
	{BASESEPARATOR,0,0,BTNS_SEP,{0},0,-1},
	{MB_PROPAGATE,IDB_MOD_PROPAGATE,TBSTATE_ENABLED|TBSTATE_CHECKED,TBSTYLE_CHECK,{0},0,0},
	{SEPARATOR,0,0,BTNS_SEP,{0},0,-1},
	{MB_BRACES,IDB_MOD_BRACES,TBSTATE_ENABLED,TBSTYLE_BUTTON,{0},0,0},
	{MB_BRACKETS,IDB_MOD_BRACKETS,TBSTATE_ENABLED,TBSTYLE_BUTTON,{0},0,0},
	{SEPARATOR,0,0,BTNS_SEP,{0},0,-1},
	{MB_FLIP,IDB_MOD_FLIP,TBSTATE_ENABLED,TBSTYLE_BUTTON,{0},0,0},
	{MB_HALFFLIP,IDB_MOD_HALFFLIP,TBSTATE_ENABLED,TBSTYLE_BUTTON,{0},0,0},
	{MB_SWAPPAREN,IDB_MOD_SWAPPAREN,TBSTATE_ENABLED,TBSTYLE_BUTTON,{0},0,0},
	{SEPARATOR,0,0,BTNS_SEP,{0},0,-1},
	{MB_DEFFONT,IDM_FONT_DEFAULT,TBSTATE_ENABLED,TBSTYLE_BUTTON,{0},0,0},
	{MB_BOLD,IDM_STYLE_BOLD,TBSTATE_ENABLED,TBSTYLE_CHECK,{0},0,0},
	{MB_ITAL,IDM_STYLE_ITALIC,TBSTATE_ENABLED,TBSTYLE_CHECK,{0},0,0},
	{MB_ULINE,IDM_STYLE_UNDERLINE,TBSTATE_ENABLED,TBSTYLE_CHECK,{0},0,0},
	{SEPARATOR,0,0,BTNS_SEP,{0},0,-1},
	{MB_REVERT,IDB_MOD_REVERT,TBSTATE_ENABLED,TBSTYLE_BUTTON,{0},0,0},
	{SEPARATOR,0,0,BTNS_SEP,{0},0,-1},
	{MB_PREV,IDB_MOD_PREV,TBSTATE_ENABLED,TBSTYLE_BUTTON,{0},0,0},
	{MB_NEXT,IDB_MOD_NEXT,TBSTATE_ENABLED,TBSTYLE_BUTTON,{0},0,0},
	{MB_DUPNEW,IDB_MOD_DUPNEW,TBSTATE_ENABLED,TBSTYLE_BUTTON,{0},0,0},
};
#define TBBSIZE (sizeof(tbb)/sizeof(TBBUTTON))
#define TBSTANDARDSIZE TBBSIZE		/* marks end of standard set */


static struct ttmatch ttstrings[] = {		/* tooltip strings */
	{IDB_MOD_PROPAGATE,TEXT("Propagate (Ctrl+PageDown)")},
	{IDB_MOD_BRACES,TEXT("Sort Hidden Text (Ctrl+Shift+T)")},
	{IDB_MOD_BRACKETS,TEXT("Hide from Sort (Ctrl+Shift+H)")},
	{IDB_MOD_FLIP,TEXT("Full Flip (Ctrl++)")},
	{IDB_MOD_HALFFLIP,TEXT("Half Flip (Ctrl+-)")},
	{IDB_MOD_SWAPPAREN,TEXT("Swap Text in Parens (Ctrl+Alt+S)")},
	{IDM_FONT_DEFAULT,TEXT("Default Font (Ctrl+Shift+D)")},
	{IDM_STYLE_BOLD,TEXT("Bold (Ctrl+B)")},
	{IDM_STYLE_ITALIC,TEXT("Italic (Ctrl+I)")},
	{IDM_STYLE_UNDERLINE,TEXT("Underline (Ctrl+U)")},
	{IDB_MOD_REVERT,TEXT("Restore Original (Ctrl+Shift+O)")},
	{IDB_MOD_PREV,TEXT("Previous (PageUp)")},
	{IDB_MOD_NEXT,TEXT("Next (PageDown)")},
	{IDB_MOD_DUPNEW,TEXT("Save and Copy (Shift+PageDown)")}
};
#define TTMAX (sizeof(ttstrings)/sizeof(struct ttmatch))

#define MODBASEX 450	/* initial height */
#define MODBASEY 160	/* initial width */
#define MODMINX 430		/* min width */
#define MODMINY 120
#define PROMPTWIDTH (BASESEPARATOR-10)	/* width of prompt field (same as first toolbar sep) */
#define EDGE 4			/* vertical offset for client edge */
#define PGAP 4			/* gap between edit boxes */
#define MINHEIGHT 6		/* min text height */
#define MAXHEIGHT 24	/* max text height */

typedef struct {
	CHARFORMAT2 cf;
	CHARRANGE cr;
	int mask;
	long freezecount;
} RECONTEXT;

void saverecontext(HWND ew,RECONTEXT *recp);		//	saves RicheEdit context
void restorerecontext(HWND ew,RECONTEXT *recp);		//	restores context
HWND createtoolbar(HWND hWndParent);

static TCHAR * m_addhelp = TEXT("_Adding and Editing\\Add_newrecord.htm");
static TCHAR * m_edithelp = TEXT("_Adding and Editing\\Add_editingentries.htm");

static void mdomenu(HWND hwnd, HMENU hMenu, UINT item, BOOL fSystemMenu);	/* sets menus */
static BOOL mcreate(HWND hwnd,LPCREATESTRUCT cs);	/* initializes text window */
static void normalizetext(HWND hwnd);		// normalizes text after paste/drop
static LRESULT CALLBACK hook(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
//static void mdoactivate(HWND hwnd, BOOL fActive, HWND hwndActivate, HWND hwndDeactivate);		/* activates window */
static void mselall(HWND hwnd);		/* selects all text */
static void mpaste(HWND hwnd);		/* pastes */
static LRESULT checkdroppaste(RECALLBACK * reptr, int mode, TCHAR * string);	/* returns paste/drop error */
static LRESULT checkdropsite(RECALLBACK * reptr);	/* CALLBACK from riched ole; returns error */
static LRESULT getcontextmenu(RECALLBACK * reptr);	/* gets context menu */
static void mlimits(HWND hwnd, LPMINMAXINFO lpMinMaxInfo);	/* limits tracking */
static void msize(HWND hwnd, UINT state, int cx, int cy);	/* sizes */
//static void msyscommand(HWND hwnd, UINT cmd, int x, int y);	/* does system commands */
static void mcommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);	/* does menu tasks */
static void msetfocus(HWND hwnd, HWND oldfocus);	/* gets focus shift message */
static int mnotify(HWND hwnd, int id, NMHDR * hdr);	/* does notification tasks */
static BOOL doeditkey(HWND hwnd, UINT msg,WPARAM wParam, LPARAM lParam );	/* sets menus */
static int checkdeletions(HWND hwnd, int code, int rangeonly);	/* permits/forbids deletion */
static void delpartfield(HWND ew, CHARRANGE * sr, int direction);	/* deletes to start or end of field */
static void doprompts(HWND hwnd);	/* sets up prompts */
static void setbasetoolbar(HWND hwnd);	/* updates all toobar buttons */
static void dobuttons(HWND hwnd, int set);	/* does style buttons */
static void checkpromptpos(HWND hwnd);		/* checks/adjusts scrolled position of prompts */
static int getprevtext(HWND hwnd, short keyindex);	/* loads identified field from previous record */
static void doprevnext(HWND wptr, BOOL mod, BOOL next);	/* enters record and moves forward/backward */
static void loadfields(HWND wptr, char * rtext, RECN rnum);		/* loads text for editing */
static void settestring(HWND wptr, char * tstring, FONTMAP * fmp, int selflag);	/* builds TE text from xstring */
static short enterrecord(HWND wptr);		/* enters new/modified record */
//static char * recoverstyle(char * sptr, CHARFORMAT2 * newsptr, CHARFORMAT2 * oldsptr, FONTMAP * fmp, CHARFORMAT2 * dfptr);	/* generates record string from style el */
static short entryerr(HWND hwnd, const int errnum, ...);
static short checkerrors(HWND wptr, char * rtext);	/* checks record */
static short checkfield(unsigned char * source, short *alarms);		/* checks record field */
static int gettotlen(HWND hwnd);	/* gets length of record text */
static void findfieldlimits(HWND hwnd, int index, CHARRANGE * cr);	/* finds field limits */
static int findfield(HWND hwnd, int charpos);	/* finds field in which char lies */
static int findtemprange(HWND ew, TCHAR * base, CHARRANGE *cr, CHARRANGE *ct, CHARFORMAT2 *cf, int * oldmask);	/* gets selection range stuff */
static void undotemprange(HWND ew, CHARRANGE *cr, CHARRANGE *ct, int oldmask);	/* restores old range if necessary */
static void mdofont(HWND hwnd,HWND cb, int code);	/* sets font */
static void setinfont(HWND hwnd, char * fname);	/* sets selection in named font */
static void drawstatstring(HWND hwnd);		/* builds & draws status string */
static void displaycount(HWND hwnd);		/* displays char count info */
static void enclosetext(HWND hwnd, char c1, char c2);	/* encloses selection in chars */
static int countbreaks(HWND ew,CHARRANGE * cr);		/* counts field breaks in selection */
static void flipfield(HWND hwnd, int mode);	/* flips field */
static void swapparens(HWND hwnd);	// swaps contents of parens
static void incdec(HWND hwnd, int mode);	// increments/decrements page number
static BOOL extendref(HWND hwnd);	// adds upper element of range as lower+1
static void mswitchdel(HWND hwnd);		/* deletes/restores */
static void mswitchtag(HWND hwnd,int tagid);		/* tags/untags */
static BOOL getabbrev(HWND hwnd);      /* inserts expanded string for abbreviation */
static BOOL mhotkey(HWND hwnd, int keyid);      /* inserts expanded string hotkey */

static int getstyleflags(HWND ew, CHARFORMAT2 * cfp, TCHAR * fname);		// gets styles at selection or from specified char format
static void setstylemenu(HWND ew, FONTMAP * fmp);		/* checks items on style menu */

static BOOL canabandon(HWND hwnd);		// returns true if can abandon record
static void copyfontmap(FONTMAP * to,FONTMAP * from);

/**********************************************************************************/
void mod_settext(INDEX *FF, char * text, RECN rnum, HWND behind)	/* loads new text for editing */

{
#ifndef READER
	RECORD * recptr;

	if (!FF->mf.readonly && (FF->rwind || mod_setwindow(FF, rnum == NEWREC)))	{	/* if have edit window */
		if (FF->rcwind)		{// if we're in separate window
			if (behind)	// set behind calling window
				SetWindowPos(FF->rcwind, behind,0,0,0,0,SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOSIZE);
			else
				BringWindowToTop(FF->rcwind);
		}
		else if (!behind)
			SetFocus(FF->rwind);
		if (mod_canenterrecord(FF->rwind,MREC_ALWAYSACCEPT))	{		/* if can flush what's already in window */
			if (rnum == NEWREC)		/* want new record */
				loadfields(FF->rwind, text,rnum);	/* loads field structures */
			else if (recptr = rec_getrec(FF,rnum))	{
				loadfields(FF->rwind, recptr->rtext,rnum);	/* loads field structures */
				view_selectrec(FF,rnum,FF->rcwind ? VD_MIDDLE : VD_BOTTOM,-1,-1);	/* get it in right position */
//				view_selectrec(FF,rnum,VD_BOTTOM,-1,-1);	/* get it in right position */
			}
		}
	}
#endif //!READER
}
/**********************************************************************************/
void mod_selectfield(HWND hwnd, int field)	// selects specified field in record

{
	HWND ew = MX(hwnd, hwed);
	CHARRANGE cr;

	findfieldlimits(hwnd, field, &cr);	/* find limits of field */
	cr.cpMax -= 1;	// preserve end of line
	SendMessage(ew, EM_EXSETSEL, 0, (LPARAM)&cr);	/* set selection info */
}
/**********************************************************************************/
HWND mod_setwindow(INDEX * FF, int addflag)	/*  sets up modify window */

{
	HWND hwnd;
	hwnd = CreateWindowEx(WS_EX_WINDOWEDGE,g_modclass,TEXT(""),WS_CLIPCHILDREN|WS_VISIBLE|WS_CHILD|WS_VISIBLE,
		0,0,0,0,FF->cwind,NULL,g_hinst,FF);
	if (hwnd)	{
		container_installrecordwindow(FF->cwind);
		MX(hwnd,addflag) = addflag;
		view_savecontext(FF,&MX(hwnd,vcont));		/* save view context */
		if (addflag && !g_prefs.gen.track)	{		/* if adding new record and not tracking (NB: addflag FALSE if adding in group) */
			RECORD * recptr;
			RECN rnum;

			FF->head.sortpars.ison = FALSE;
			FF->head.privpars.hidebelow = ALLFIELDS;	/* 9/2/01 ensures every record displayed */
			if (g_prefs.gen.switchview && FF->head.privpars.vmode == VM_FULL)	/* if in full format & want switch */
				FF->head.privpars.vmode = VM_DRAFT;
			if (recptr = sort_bottom(FF))	/* find last displayable record */
				rnum = recptr->num;
			else
				rnum = 0;
			view_redisplay(FF,rnum,VD_RESET|VD_BOTTOM);	// displays last rec at bottom of screen
			view_selectrec(FF,FF->head.rtot,FF->rcwind ? VD_MIDDLE : VD_BOTTOM,-1,0);	// force last record to right display position
		}
		else if (g_prefs.gen.switchview && FF->head.privpars.vmode == VM_FULL)		{/* if in full format & want switch */
			FF->head.privpars.vmode = VM_DRAFT;
			view_redisplay(FF,0,VD_RESET|VD_CUR);	/* displays in draft */
		}
		view_setstatus(FF->vwind);	// show any status change, per context
	}
	return (hwnd);
}
/************************************************************************/
LRESULT CALLBACK mod_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)

{
	switch (msg)	{
		HANDLE_MSG(hwnd,WM_CREATE,mcreate);
		HANDLE_MSG(hwnd,WM_SIZE,msize);
		HANDLE_MSG(hwnd,WM_COMMAND,mcommand);
		HANDLE_MSG(hwnd,WM_INITMENUPOPUP,mdomenu);
		HANDLE_MSG(hwnd,WM_NOTIFY,mnotify);
		HANDLE_MSG(hwnd,WM_SETFOCUS,msetfocus);
		HANDLE_MSG(hwnd,WM_GETMINMAXINFO,mlimits);
		case WM_CLOSE:
			return (mod_close(hwnd,MREC_ALLOWCHECK));
		case WM_HOTKEY:
			return (mhotkey(hwnd,wParam));
		case WM_HELP:
			dowindowhelp((MX(hwnd, addflag) ? m_addhelp: m_edithelp));
			return (0);
		case WM_CONTEXTMENU:
			return (0);				/* stops recursive call through frameproc, via DefMDIChildProc */
		case WMM_GETCURREC:
			return(MX(hwnd,recnum));
		case WMM_UPDATESTATUS:
			drawstatstring(hwnd);	/* force update of status line */
			return 0;
//		case WMM_UPDATETOOLBARS:
//			mupdatetoolbars(hwnd);
//			return (0);
	}
	return (DefWindowProc(hwnd,msg,wParam,lParam));
}
/************************************************************************/
static void mcommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)	/* does menu tasks */

{
	MFLIST * mfp = getdata(hwnd);
	INDEX * FF = getowner(hwnd);

	if (mfp && hwndCtl == mfp->hwed)	{	/* if from edit window */
		switch (codeNotify)	{
			case EN_UPDATE:
			// use EN_UPDATE because all changes happen before redisplay	
				if (mfp->doingpastedrop) {		// true when doing paste/drop
#if 0
					if (mfp->lastpastechar) {	// check/remove unwanted chars on end of drop/paste
						CHARRANGE cr;
						TEXTRANGE tr;
						TCHAR buff[2];

						SendMessage(hwndCtl, EM_EXGETSEL, 0, (LPARAM)&cr);	// get selection info (position at end of paste/drop)
						tr.chrg = cr;
						tr.lpstrText = buff;

						do {
							tr.chrg.cpMin = tr.chrg.cpMax - 1;	// char to left of current pos
							SendMessage(mfp->hwed, EM_GETTEXTRANGE, 0, (LPARAM)&tr);
						} while (*buff != mfp->lastpastechar && --tr.chrg.cpMax > mfp->currentselection.cpMin);
						if (tr.chrg.cpMax < cr.cpMax) {	// if need to discard characters
							int oldmask = SendMessage(hwndCtl, EM_SETEVENTMASK, 0, ENM_NONE);	/* turn off selchange/update events */

							cr.cpMin = tr.chrg.cpMax;	// set to remove unwanted chars
							SendMessage(hwndCtl, EM_EXSETSEL, 0, (LPARAM)&cr);	// set selection info
							SendMessage(hwndCtl, WM_CLEAR, 0, 0);	// clear last char
							SendMessage(hwndCtl, EM_SETEVENTMASK, 0, oldmask);	/* turn on events */
						}
						mfp->lastpastechar = 0;
					}
#endif
					mfp->doingpastedrop = FALSE;
					normalizetext(hwnd);
				}
				return;
			case EN_CHANGE:
				if (!mfp->nocount)		/* if we want to count */
					mfp->totlen = gettotlen(hwnd);		/* set new length */
				mfp->nocount = FALSE;	/* clear nocount flag */
				if (mfp->insertspace)	{	/* if we want to insert space */
					mfp->insertspace = FALSE;		/* clear */
					if (mod_getcharatpos(hwndCtl,LEFTCHAR) != SPACE)	{	/* if not space to left */
						SendMessage(hwndCtl,WM_CHAR,(TCHAR)SPACE,0);
						mfp->nocount = TRUE;	/* set nocount flag */
					}
				}
				if (mfp->totlen >= FF->head.indexpars.recsize)	{	/* if has become too long */
					SendMessage(hwndCtl,EM_UNDO,0,0);
					entryerr(mfp->hwst,ERR_RECOVERFLOW);
					mfp->totlen = gettotlen(hwnd);		/* set new length */
				}
				dobuttons(hwnd, BUTTONS_SELECT);	/* do style buttons */
				doprompts(hwnd);	/* check/set prompts */
				displaycount(hwnd);
				return;
		}
	}
	else {
		switch (id)	{
			case IDM_EDIT_UNDO:
				SendMessage(mfp->hwed,EM_UNDO,0,0);
				return;
			case IDM_EDIT_REDO:
				SendMessage(mfp->hwed,EM_REDO,0,0);
				return;
			case IDM_EDIT_CUT:
#if 0
				SendMessage(mfp->hwed,WM_CUT,0,0);
#else
				SendMessage(mfp->hwed,WM_COPY,0,0);	/* don't use WM_CUT, because we then can't check deletions */
				checkdeletions(hwnd, VK_DELETE, -1);
#endif
				return;
			case IDM_EDIT_COPY:
				SendMessage(mfp->hwed,WM_COPY,0,0);
				return;
			case IDM_EDIT_PASTE:
				mpaste(hwnd);
				return;
			case IDM_EDIT_CLEAR:
				checkdeletions(hwnd, VK_DELETE, -1);
				return;
			case IDM_EDIT_SELECTALL:
				mselall(hwnd);
				return;
			case IDM_EDIT_NEWRECORD:
				mod_settext(FF,NOTEXT,NEWREC,NULL);
				return;
			case IDM_EDIT_DUPLICATE:
				edit_duplicate(hwnd);			
				return;
			case IDM_EDIT_DELETE:
				mswitchdel(hwnd);
				return;
			case IDM_EDIT_LABEL_SWITCH:
			case IDM_EDIT_LABEL0:
			case IDM_EDIT_LABEL1:
			case IDM_EDIT_LABEL2:
			case IDM_EDIT_LABEL3:
			case IDM_EDIT_LABEL4:
			case IDM_EDIT_LABEL5:
			case IDM_EDIT_LABEL6:
			case IDM_EDIT_LABEL7:
			case IDM_EDIT_LABEL8:
				mswitchtag(hwnd,id-IDM_EDIT_LABEL0);
				return;
			case IDM_EDIT_NEWABBREV:
				abbrev_makenew(hwnd);
				return;
			case IDM_EDIT_SWAPPARENS:
			case IDB_MOD_SWAPPAREN:
				swapparens(hwnd);
				return;
			case IDM_COMBO_FONT:
				mdofont(hwnd,hwndCtl,codeNotify);
				return;
			case IDB_MOD_PROPAGATE:
				if (mfp->recnum <= FF->head.rtot)	/* if not new record */
					SendMessage(mfp->hwtb,TB_CHECKBUTTON,IDB_MOD_PROPAGATE,mfp->propstate ^= 1);	/* set propagation */
				return;
			case IDB_MOD_BRACES:
				enclosetext(hwnd,OBRACE,CBRACE);
				return;
			case IDB_MOD_BRACKETS:
				enclosetext(hwnd,OBRACKET,CBRACKET);
				return;
			case IDB_MOD_FLIP:
			case IDB_MOD_FLIPX:
			case IDB_MOD_HALFFLIP:
			case IDB_MOD_HALFFLIPX:
				flipfield(hwnd,id);
				return;
			case IDM_MOD_INCREMENT:
			case IDM_MOD_DECREMENT:
				incdec(hwnd,id);
				return;
			case IDB_MOD_DUPNEW:
				doprevnext(hwnd, TRUE, TRUE);	/* enter and move */
				return;
			case IDB_MOD_PREV:
			case IDB_MOD_NEXT:
				doprevnext(hwnd, FALSE, id == IDB_MOD_NEXT);	/* enter and move */
				return;
			case IDB_MOD_REVERT:
				loadfields(hwnd,OLDTEXT, OLDREC);	/* revert to original */
				return;
			case IDM_FONT_DEFAULT:
				setinfont(hwnd,FF->head.fm[0].name);
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
				mod_dostyle(mfp->hwed,&mfp->df,id);
				return;
		}
	}
	FORWARD_WM_COMMAND(hwnd,id,hwndCtl,codeNotify, DefWindowProc);
}
/************************************************************************/
static int mnotify(HWND hwnd, int id, NMHDR * hdr)	/* does notification tasks */

{
	MFLIST * mfp = getdata(hwnd);
	int tindex;
	POINT mouseloc;
	HWND mwnd;
	
	switch (hdr->code)	{
		case EN_SELCHANGE:
			{
				CHARRANGE cr = ((SELCHANGE *)hdr)->chrg;
				int startfield = findfield(hwnd, STARTFIELD);
				int endfield = findfield(hwnd, ENDFIELD);

				int oldmask = SendMessage(mfp->hwed, EM_SETEVENTMASK, 0, ENM_NONE);	// turn off all events
				if (startfield < FIELDLIM && endfield < FIELDLIM && (startfield != mfp->startfield || endfield != mfp->endfield)) {	// if changed field (FIELDLIM check needed because first pass can deliver bad value)
					mfp->completingSelection = FALSE;	// kill any autocomplete
					if (startfield == endfield) {	// if within single field
						CHARRANGE cs;
						findfieldlimits(hwnd, startfield, &cs);
						Edit_SetSel(mfp->hwed, cs.cpMin, cs.cpMin);	// select start char
						SendMessage(mfp->hwed, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&mfp->compf);	// catch format at beginning of field
						Edit_SetSel(mfp->hwed, cr.cpMin, cr.cpMax);	/* restore selection */
					}
					mfp->startfield = startfield;
					mfp->endfield = endfield;
				}
				if (cr.cpMax == cr.cpMin+1)	{	// if single char selected
					TCHAR lastchar = mod_getcharatpos(mfp->hwed,cr.cpMin);

					if (!lastchar || lastchar == RETURN)	{	// destroy selection if it's single newline character
						cr.cpMax = cr.cpMin;
						SendMessage(mfp->hwed,EM_EXSETSEL,0,(LPARAM)&cr);	// reset selection
					}
				}
				SendMessage(mfp->hwed, EM_SETEVENTMASK, 0, oldmask);	// restore allowed events
			}
			dobuttons(hwnd, BUTTONS_SELECT);
			checkpromptpos(hwnd);	/* check for prompt alignment */
#if 0
			if (!mfp->protectedSelection)	// if not a change within completion selection
				mfp->completingSelection = FALSE;
			else
				mfp->protectedSelection = FALSE;
#endif
			return TRUE;
		case EN_MSGFILTER:
			switch (((MSGFILTER *)hdr)->msg)	{
				case WM_CHAR:
				case WM_KEYDOWN:
					return (doeditkey(hwnd,((MSGFILTER *)hdr)->msg,((MSGFILTER *)hdr)->wParam,((MSGFILTER *)hdr)->lParam));
				case WM_MOUSEWHEEL:
					mouseloc.x = LOWORD(((MSGFILTER *)hdr)->lParam);
					mouseloc.y = HIWORD(((MSGFILTER *)hdr)->lParam);
					mwnd = WindowFromPoint(mouseloc);		// get window from mouse posn
					if (mwnd == getowner(hwnd)->vwind) {		// if view window
						SendMessage(mwnd,WM_MOUSEWHEEL,((MSGFILTER *)hdr)->wParam,((MSGFILTER *)hdr)->lParam);	// forward it
						return TRUE;
					}
			}
			return FALSE;
		case TTN_NEEDTEXT:
			for (tindex = 0; tindex < TTMAX; tindex++)	{	/* while not found our string */
				if (ttstrings[tindex].id == ((LPTOOLTIPTEXT)hdr)->hdr.idFrom)	{
					((LPTOOLTIPTEXT)hdr)->lpszText = ttstrings[tindex].string;
					return (TRUE);
				}
			}
			return TRUE;
		case TBN_QUERYINSERT:
			if (((LPNMTOOLBAR)hdr)->iItem < 1)	// if lead separator
				return FALSE;
			return TRUE;
		case TBN_QUERYDELETE:
			if (((LPNMTOOLBAR)hdr)->iItem < 1)	// if lead separator
				return FALSE;
			return TRUE;
		case TBN_GETBUTTONINFO:
			if (((LPNMTOOLBAR)hdr)->iItem < TBBSIZE)	{
				tbreturninfo((LPNMTOOLBAR)hdr,tbb,TTMAX,ttstrings);
				return TRUE;
			}
			return FALSE;
		case TBN_BEGINADJUST:
			return FALSE;	// essential to return 0
		case TBN_ENDADJUST:
			com_tbsaverestore(mfp->hwtb,hwnd,TRUE, M_TBREGISTERC);	/* saves */
			return TRUE;
		case TBN_RESET:
			com_tbsaverestore(mfp->hwtb,hwnd,FALSE, M_TBREGISTERD);	/* recover default */
			dobuttons(hwnd, BUTTONS_ALL);
			return TRUE;
		case TBN_CUSTHELP:
			dowindowhelp(TEXT("Toolbars"));
			return TRUE;
		case TBN_RESTORE:
			if (((LPNMTBRESTORE)hdr)->iItem == 0)	// if first button (big separator)
				((LPNMTBRESTORE)hdr)->tbButton.iBitmap = scaleForDpi(hwnd,BASESEPARATOR);
			return 0;
	}
	return FORWARD_WM_NOTIFY(hwnd,id,hdr, DefWindowProc);
}
/************************************************************************/
static BOOL doeditkey(HWND hwnd, UINT msg,WPARAM wParam, LPARAM lParam )	/* handles keys */

{
	MFLIST * mfp = getdata(hwnd);
	HWND ew = mfp->hwed;
	int lindex, newfield, oldpos, curfield, tsort;
	CHARRANGE cr;
	INDEX * FF;
	TCHAR rightchar, leftchar;

	FF = getowner(hwnd);
	SendMessage(ew,EM_EXGETSEL,0,(LPARAM)&cr);	/* get selection info */
	curfield = mfp->startfield;
	rightchar = mod_getcharatpos(ew,RIGHTCHAR);
	leftchar = mod_getcharatpos(ew,LEFTCHAR);

	if (msg == WM_KEYDOWN)	{
		switch (wParam)	{		/* key code */
			case VK_ESCAPE:		/* discard */
				mod_close(hwnd,MREC_FORCEDISCARD);
				return (TRUE);
			case VK_DELETE:
			case VK_BACK:
				return (checkdeletions(hwnd,wParam,FALSE));
			case VK_UP:
				if (GetKeyState(VK_CONTROL) < 0)	{	/* if want first field */
					Edit_SetSel(ew,0,0);	/* set start */
					return (TRUE);
				}
				return (FALSE);
			case VK_DOWN:
				if (rightchar == RETURN)
					getabbrev(hwnd);
				if (GetKeyState(VK_CONTROL) < 0)	{	/* if want last field */
					findfieldlimits(hwnd,mfp->fcount-1,&cr);	/* find limits of field */
					Edit_SetSel(ew,cr.cpMin,cr.cpMin);	/* set it */
					return (TRUE);
				}
				return (FALSE);
			case VK_TAB:
				if (rightchar == RETURN || !rightchar)	/* if at end of text field or page field */
					getabbrev(hwnd);
				if (GetKeyState(VK_SHIFT) < 0)	{	/* if want to move backwards */
					newfield = curfield-1;	/* find next field */
					if (newfield < 0)	/* or last */
						newfield = mfp->fcount-1;
				}
				else	{		/* move forwards */
					newfield = curfield+1;	/* find next field */
					if (newfield >= mfp->fcount)	/* or first */
						newfield = 0;
				}
				findfieldlimits(hwnd,newfield,&cr);	/* find limits of field */
				Edit_SetSel(ew,cr.cpMin,cr.cpMin);	/* set it */
				return (TRUE);
			case VK_PRIOR:		/* prev record */
			case VK_NEXT:		/* next record */
				doprevnext(hwnd, FALSE, wParam == VK_NEXT);	/* enter and move */
				return (TRUE);
			case VK_HOME:
				oldpos = cr.cpMax;
				newfield = curfield;	/* get field for selection point */
				if (leftchar == RETURN && newfield >0 && cr.cpMax == cr.cpMin)	/* if already at start of field, & no sel */
					newfield--;
				findfieldlimits(hwnd,newfield,&cr);	/* find limits of field */
				if (GetKeyState(VK_SHIFT) < 0)		/* if extending selection to start */
					cr.cpMax = oldpos;
				else
					cr.cpMax = cr.cpMin;
				Edit_SetSel(ew,cr.cpMin,cr.cpMax);	/* set it */
				return (TRUE);
			case VK_END:
				oldpos = cr.cpMin;
				newfield = mfp->endfield;	/* get field for end of selection range */
				if (rightchar == RETURN && newfield < mfp->fcount && cr.cpMax == cr.cpMin)	/* if already at end of field, & no sel */
					newfield++;
				findfieldlimits(hwnd,newfield,&cr);	/* find limits of field */
				if (GetKeyState(VK_SHIFT) < 0)		/* if extending selection to end */
					cr.cpMin = oldpos;
				else
					cr.cpMin= cr.cpMax-1;
				Edit_SetSel(ew,cr.cpMin,cr.cpMax-1);	/* set it */
				return (TRUE);
			case VK_RETURN:
				if (GetKeyState(VK_SHIFT) < 0)	// ignore line feed
					return TRUE;
				if (GetKeyState(VK_CONTROL) < 0 || (lParam&KEY_EXTENDED))	{	/* if we want to close */
					mod_close(hwnd,MREC_ALWAYSACCEPT);
					return TRUE;
				}
#if 0
				if (cr.cpMin == cr.cpMax)	/* if no range */
					getabbrev(hwnd);		/* expand any abbreviation */
//				else if (g_prefs.gen.autoextend && mfp->recnum > FF->head.rtot && (rightchar == RETURN || !rightchar) && curfield == findfield(hwnd, ENDFIELD))	{
				else if (mfp->completingSelection && (rightchar == RETURN || !rightchar))	{
					cr.cpMin = cr.cpMax;
					SendMessage(ew,EM_EXSETSEL,0,(LPARAM)&cr);	// set selection to end of line
				}
				lindex = SendMessage(ew,EM_LINEFROMCHAR,(WPARAM)-1,0);	/* get line of selection pt */
				if (lindex >= mfp->protectindex || mfp->fcount == FF->head.indexpars.maxfields)	/* if can't break */
					return (TRUE);		/* would make too many fields */
				SendMessage(ew,EM_EXGETSEL,0,(LPARAM)&cr);	// get current selection (might have expanded abbrev)
				if (leftchar == SPACE)	// if should catch space
					cr.cpMin--;		// expand selection
				if (rightchar == SPACE)		// if should catch space
					cr.cpMax++;
				SendMessage(ew,EM_EXSETSEL,0,(LPARAM)&cr);	// set selection to end of line
				if (!cr.cpMin || rightchar == RETURN || leftchar == RETURN)	{	/* if would be making a clean new field */
					mod_dostyle(ew,&mfp->df,IDM_STYLE_REGULAR);		/* set style to plain */
					setinfont(hwnd,FF->head.fm[0].name);	/* and default font */
				}
#else
				if (mfp->completingSelection && (rightchar == RETURN || !rightchar))	{
					cr.cpMin = cr.cpMax;
					SendMessage(ew,EM_EXSETSEL,0,(LPARAM)&cr);	// set selection to end of line
				}
				lindex = SendMessage(ew,EM_LINEFROMCHAR,(WPARAM)-1,0);	/* get line of selection pt */
				if (lindex >= mfp->protectindex || mfp->fcount == FF->head.indexpars.maxfields)	/* if can't break */
					return (TRUE);		/* would make too many fields */
				if (cr.cpMin == cr.cpMax)	/* if no range */
					getabbrev(hwnd);		/* expand any abbreviation */
				SendMessage(ew,EM_EXGETSEL,0,(LPARAM)&cr);	// get current selection (might have expanded abbrev)
				rightchar = mod_getcharatpos(ew, RIGHTCHAR);	// Aug 2 2019: reset left and right chars
				leftchar = mod_getcharatpos(ew, LEFTCHAR);
				if (leftchar == SPACE)	// if should catch space
					cr.cpMin--;		// expand selection
				if (rightchar == SPACE)		// if should catch space
					cr.cpMax++;
				if (!cr.cpMin || rightchar == RETURN || leftchar == RETURN)		/* if would be making a clean new field */
					SendMessage(ew,EM_SETCHARFORMAT,SCF_SELECTION,(LPARAM)&mfp->df);	// set default format
				SendMessage(ew,EM_EXSETSEL,0,(LPARAM)&cr);	// set selection to end of line
#endif
				return FALSE;
			case VK_INSERT:
				if (GetKeyState(VK_CONTROL) >= 0 && GetKeyState(VK_SHIFT) >= 0)	{
					mfp->isovr ^= 1;
					SendMessage(mfp->hwst,SB_SETTEXT,7,(LPARAM)(mfp->isovr ? TEXT("OVR") : TEXT("INS")));	/* status */
				}
				return (FALSE);
			default:
				if ((char)wParam >= '0' && (char)wParam <= '9' && GetKeyState(VK_CONTROL) < 0)	{
					getprevtext(hwnd,wParam);
					return(TRUE);	// changed for win2000: don't process event
				}
		}
	}
	else if (msg == WM_CHAR)	{	/* checks/deals with ordinary chars */
		if (wParam == RETURN || wParam == VK_TAB || wParam == VK_BACK || wParam == VK_ESCAPE || wParam == '\177')	/* we've dealt with these already */
			return (TRUE);
		if (checkdeletions(hwnd,wParam,TRUE))	/* if would have caused field trouble */
			return (TRUE);
		if (curfield == mfp->fcount-1 && wParam == FF->head.refpars.rsep && g_prefs.gen.autorange && extendref(hwnd))	// if in page field && autoextend ref
			return TRUE;
		if (g_prefs.gen.autoextend && mfp->recnum > FF->head.rtot	/* if auto-complete && new record  */
			&& /* curfield <= mfp->fcount-2 && */ curfield == mfp->endfield && (rightchar == RETURN
				|| !rightchar))	{	/* and selection in one text field, up to end */
			
			char tstring[MAXREC], *xptr;
			RECORD * hitptr;
			CHARRANGE cx;
			short s1count, sourcefield;
			CSTR s1[FIELDLIM], s2[FIELDLIM];
			char * baseptr;
			CHARFORMAT2 cf;
			
			if (leftchar && leftchar != RETURN && (cr.cpMin != cr.cpMax && (mod_getcharatpos(ew,cr.cpMin) == (char)wParam /* if not at start of field... */
				|| g_prefs.gen.autoignorecase && _tolower(mod_getcharatpos(ew,cr.cpMin)) == _tolower(wParam))))		{/* if matches start of selection */
				Edit_SetSel(ew,++cr.cpMin,cr.cpMax);	/* advance selection */
			}
			else {
				int oldmask;
				BOOL isCrossref = FALSE;

				if (cr.cpMin && cr.cpMax > cr.cpMin)	{	/* if have active completion */
					if (leftchar != RETURN)	{
						Edit_SetSel(ew,cr.cpMin-1,cr.cpMin);	/* set selection to prev char */
						cf.cbSize = sizeof(cf);
						SendMessage(ew,EM_GETCHARFORMAT,TRUE,(LPARAM)&cf);	/* recover format info */
						Edit_SetSel(ew,cr.cpMin,cr.cpMax);	/* restore selection */
						SendMessage(ew,EM_SETCHARFORMAT,SCF_SELECTION,(LPARAM)&cf);	/* set new format on selection */
					}
				}
				else if ((!cr.cpMin || leftchar == RETURN) && rightchar == RETURN || !rightchar) 	// if field is empty
					SendMessage(ew, EM_GETCHARFORMAT, TRUE, (LPARAM)&mfp->compf);	// recover any initial format settings
//				else
//					memcpy(&mfp->compf,&mfp->df,sizeof(CHARFORMAT2));	// will set default format
				if (nstrchr(a_suffix,wParam))	{	/* if poss character to trigger abbrev expansion */
					getabbrev(hwnd);			/* check & insert expansion string */
					SendMessage(ew,EM_EXGETSEL,0,(LPARAM)&cr);	/* get new selection info */
				}
//				SendMessage(ew,EM_REPLACESEL,TRUE,(LPARAM)key);
				// do this rather then EM_REPLACESEL, because WM_CHAR character hangs around and can be spuriously entered later
				oldmask = SendMessage(ew,EM_SETEVENTMASK,0,ENM_CHANGE);	// turn off all but selchange events 		
				SendMessage(ew,WM_CHAR,wParam,lParam);
				SendMessage(ew,EM_SETEVENTMASK,0,oldmask);	// reenable notifications
				copyfontmap(mfp->fm,FF->head.fm);
				mod_gettestring(ew, tstring, mfp->fm, FALSE);	/* retrieve record text */
				str_xparse(tstring,s1);			/* parse string */
				s1count = curfield;
				if (curfield == mfp->fcount-1)	{	// if page field
					if (str_crosscheck(FF,s1[s1count].str))	{	/* if cross ref */
						isCrossref = TRUE;
						baseptr = str_skiplistmax(s1[s1count].str, FF->head.refpars.crosstart);
					}
					else
						baseptr = g_nullstr;
				}
				else
					baseptr = tstring;
				xptr = s1[s1count].str+s1[s1count].ln;		/* NULL at end of target field */
				if (s1[s1count].ln > 1)	{			/* if could have had trailing codes */
					while (iscodechar(*(xptr-2)))	/* pass back over any codes */
						xptr -= 2;
				}
				if ((char)wParam == SPACE)		/* if field had terminal space (now stripped) */
					*xptr++ = (char)wParam;		/* restore it */
				s1[s1count].ln = xptr - s1[s1count].str;	/* reset length */
				*xptr++ = '\0';			/* terminate string */
				*xptr = EOCS;
				tsort = FF->head.sortpars.ison;
				FF->head.sortpars.ison = TRUE;
				hitptr = *baseptr ? search_treelookup(FF,baseptr) : NULL;	/* always lookup with sort enabled */
				FF->head.sortpars.ison = tsort;
				findfieldlimits(hwnd,s1count,&cx);	/* find bounds of current field */
				if (hitptr)	{	// if not leading space && content matches a record
					str_xparse(hitptr->rtext,s2);		/* parse new text */
					sourcefield = -1;		/* presume will find no match */
					if (curfield < mfp->fcount-1)	{	/* if not page field */
						if ((g_prefs.gen.autoignorecase && !str_texticmp(s1[s1count].str,s2[s1count].str) || !strncmp(s1[s1count].str,s2[s1count].str,s1[s1count].ln))	/* if match on last text field */
							&& (!s1count || !col_match(FF,&FF->head.sortpars,s1[s1count-1].str,s2[s1count-1].str,MATCH_IGNORECODES|MATCH_IGNORECASE)))	{ /* and is first field or prev is full match */
							sourcefield = s1count;
						}
					}
					else if (g_prefs.gen.autoignorecase && !str_texticmp(baseptr,s2[0].str) || !strncmp(baseptr,s2[0].str,strlen(baseptr)))	{/* if match on last text field */
						*baseptr = '\0';	/* terminate lead to cross-ref */
						cx.cpMin += str_utextlen(s1[s1count].str,0);		/* offset start by length of cross-ref lead */
						sourcefield = 0;
					}
					if (sourcefield >= 0)	{		/* if matched field */
						int xsattr;
						TCHAR name[LF_FACESIZE];
						char * xsptr;

						oldmask = SendMessage(mfp->hwed, EM_SETEVENTMASK, 0, ENM_NONE);	// turn off all events
						xsattr = getstyleflags(ew, &mfp->compf, name);	// recover style flags
						if (xsattr) {
							xsptr = tstring;
							*xsptr++ = CODECHR;
							*xsptr++ = xsattr;
							strcpy(xsptr, s2[sourcefield].str);
							xsptr += strlen(xsptr);
							*xsptr++ = CODECHR;
							*xsptr++ = xsattr | FX_OFF;
							*xsptr = '\0';
						}
						else
							strcpy(tstring, s2[sourcefield].str);	/* copy source field */
						*(tstring + strlen(tstring) + 1) = EOCS;	/* terminate as EOCS */
						Edit_SetSel(ew,cx.cpMin,cx.cpMax-1);	/* advance selection in current field */
						settestring(hwnd, tstring, FF->head.fm, TRUE);	/* insert new text with any styles */
						SendMessage(ew, EM_EXGETSEL, 0, (LPARAM)&cx);	/* get selection info */
						Edit_SetSel(ew,++cr.cpMin,cx.cpMax);		/* advance selection */
						SendMessage(mfp->hwed, EM_SETEVENTMASK, 0, oldmask);	// restore allowed events
						mfp->completingSelection = TRUE;
						if (g_prefs.gen.tracksource && !sort_isignored(FF,hitptr))	/* if tracking source && record viewable */
							view_selectrec(FF,hitptr->num,VD_SELPOS,-1,-1);	/* put source rec in right position */
					}
				}
				else if (mfp->completingSelection)	{		// no match after having found one; make sure any text has default attributes restored
					if (g_prefs.gen.autoignorecase) {		// restore attributes only if ignoring style and case
						if (curfield < mfp->fcount - 1)	// if not in page field
							cx.cpMax--;	// stop before newline
						else if (isCrossref) {
							*baseptr = '\0';
							cx.cpMin += str_utextlen(s1[s1count].str, 0);		/* offset start by length of cross-ref lead */
						}
						Edit_SetSel(ew, cx.cpMin, cx.cpMax);
						mfp->compf.dwMask = mfp->df.dwMask;		// set mode for *replacing* styles
						SendMessage(ew, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&mfp->compf);
						Edit_SetSel(ew, cx.cpMax, cx.cpMax);
					}
					mfp->completingSelection = FALSE;
//					NSLog("++++EndingCompletion");
				}
				SendMessage(hwnd,WM_COMMAND,MAKELONG(IDM_EDIT,EN_CHANGE),(LPARAM)ew);	/* flag change */
			}
			return TRUE;			/* forbid further processing */
		}
		if (cr.cpMin == cr.cpMax){		/* if inserting single char */
			if (nstrchr(a_suffix,wParam))	/* if poss character to trigger expansion */
				getabbrev(hwnd);	   /* check & insert expansion string */
			if (mfp->totlen+1 >= FF->head.indexpars.recsize)	{	/* if would be too long */
				entryerr(mfp->hwst,ERR_RECOVERFLOW);
				return (TRUE);	/* forbid */
			}
			else	{		/* add char */
				mfp->totlen += U8_LENGTH(wParam);		// add 
				mfp->nocount = TRUE;	/* we don't do whole count on this keystroke */
			}
		}
		else {		/* replacing range */
			if (mod_getcharatpos(ew,cr.cpMax-1) == SPACE)		{	/* if last char is space */
				cr.cpMax -= 1;	/* leave it */
				SendMessage(ew,EM_EXSETSEL,0,(LPARAM)&cr);	/* adjust selection */
			}
		}
	}
	return (FALSE);
}
/************************************************************************/
static int checkdeletions(HWND hwnd, int code, int rangeonly)	/* permits/forbids replacement */

{
	int lindex, loss, start, end, need, findpos, length;
	INDEX * FF;
	HWND ew;
	FINDTEXT ft;
	CHARRANGE cr;
	GETTEXTLENGTHEX tl;
	TCHAR rep[FIELDLIM*2], *rptr;
	MFLIST * mfp;

	mfp = getdata(hwnd);
	ew = mfp->hwed;
	FF = getowner(hwnd);
	SendMessage(ew,EM_EXGETSEL,0,(LPARAM)&cr);	/* get selection info */
	if (cr.cpMin != cr.cpMax)	{	/* if a range */
		ft.chrg = cr;
		ft.lpstrText = NEWLINESTRING;
		for (loss = 0; (findpos = SendMessage(ew,EM_FINDTEXT,FR_DOWN,(LPARAM)&ft)) != -1; loss++)	/* while we have hits */
			ft.chrg.cpMin = findpos+1;			/* pass over target */
		tl.codepage = 1200 ;
		tl.flags = GTL_DEFAULT;
		length = SendMessage(ew,EM_GETTEXTLENGTHEX,(WPARAM)&tl,0);	//get character count
//		if (ft.chrg.cpMin >= length && findfield(ew,cr.cpMin) >0)		// if end on last char (return) of last field, and don't start in main heading
//			loss--;			/* reduce lost field count */
		if (loss && (mod_getcharatpos(ew,cr.cpMax-1) == RETURN || ft.chrg.cpMin >= length))	{	/* if line break at end of selection, or sel reaches end of text */
			cr.cpMax -= 1;		/* don't remove last newline */
			SendMessage(ew,EM_EXSETSEL,0,(LPARAM)&cr);	/* set new selection info */
			loss--;
			lindex = Edit_LineFromChar(ew,cr.cpMax);
			if (cr.cpMax <= cr.cpMin && lindex == mfp->protectindex-1)	/* if would delete only newline before page or required field */
				return (TRUE);		/* do nothing */
		}
		need = FF->head.indexpars.minfields-(mfp->fcount-loss);
		if (need < 0)
			need = 0;
		if (loss && !need && Edit_LineFromChar(ew,cr.cpMax) == mfp->protectindex)	/* if selection runs across page field and no new breaks */
			need++;		/* protect against loss of page field break */
		if (need > 0)	{	/* if we'd leave a need */
			rptr = rep;
			if (code != RETURN && code != VK_DELETE && code != VK_BACK) /* if not a character to remove */
				*rptr++ = code;
			while (need--)
				*rptr++ = RETURN;
			*rptr++ = '\0';
			SendMessage(ew,EM_REPLACESEL,TRUE,(LPARAM)rep);	/* replace with new lines */
			SendMessage(ew,EM_SETSEL,cr.cpMin,cr.cpMin);	// set cursor to start of prev selection
			return (TRUE);		/* we've handled it */
		}
		if (rangeonly < 0)	{	/* if want cut or clear from menu, and not already trapped */
			SendMessage(ew,WM_CLEAR,0,0);
			return (TRUE);
		}
		if ((code == VK_DELETE || code == VK_BACK) && GetKeyState(VK_CONTROL) < 0 && GetKeyState(VK_SHIFT) < 0)	{	/* if want part field deletion */
			delpartfield(hwnd,&cr,1);	/* need this 'cause ctrl with del or bsp will do richedit word action */
			return (TRUE);
		}
		lindex = Edit_LineFromChar(ew,cr.cpMin);
		if (code == VK_BACK && GetKeyState(VK_CONTROL) < 0 && lindex == mfp->protectindex && mod_getcharatpos(ew,LEFTCHAR) == RETURN)	{	/* if ctrl del at start of protected field */
			SendMessage(ew,WM_CLEAR,0,0);
			return (TRUE);
		}
	}
	else if (!rangeonly)	{	/* if want to check single character deletion */
		lindex = SendMessage(ew,EM_LINEFROMCHAR,cr.cpMin,0);	/* get line of selection pt */
		start = SendMessage(ew,EM_LINEINDEX,lindex,0);		/* get index at start of line */
		end = SendMessage(ew,EM_LINEINDEX,lindex+1,0);		/* get index at start of next line */
		if (code == VK_DELETE)	{
			if (end == cr.cpMax+1 && (lindex+1 >= mfp->protectindex
				|| mfp->map[lindex+1] != (TCHAR)0xFF && mfp->fcount == FF->head.indexpars.minfields))
				/* if at end of field before any field whose removal would cause trouble */
				return (TRUE);	/* forbid deletion */
			if (GetKeyState(VK_CONTROL) < 0)	{	/* if some extended deletion */
				if (GetKeyState(VK_SHIFT) < 0)	{	/* want to delete to end of field */
					delpartfield(hwnd,&cr,1);
					return (TRUE);
				}
				return (FALSE);
			}
			if (end > start+2 && end == cr.cpMax+1)	/* if at end of non-empty line */
				mfp->insertspace = TRUE;	/* will force space insertion after */
		}
		else if (code == VK_BACK)	{
			if (start == cr.cpMin && (lindex >= mfp->protectindex
				|| mfp->map[lindex] != (TCHAR)0xFF && mfp->fcount == FF->head.indexpars.minfields))
				/* if at start of page field or any field whose removal would cause trouble */
				return (TRUE);	/* forbid deletion */
			if (GetKeyState(VK_CONTROL) < 0)	{	/* some extended deletion */
				if (GetKeyState(VK_SHIFT) < 0)	{	/* want to delete to start of field */
					delpartfield(hwnd,&cr,-1);
					return (TRUE);
				}
				return (FALSE);
			}
			if (start == cr.cpMin && start > 2 && mod_getcharatpos(ew,start-2) != RETURN && mod_getcharatpos(ew,start-1) != SPACE)	/* need to insert space before */
				mfp->insertspace = TRUE;	/* will force space insertion after */
		}
		mfp->totlen -= U8_LENGTH(mod_getcharatpos(ew,code == VK_DELETE ? RIGHTCHAR : LEFTCHAR));			/* quick count of deletion */
		mfp->nocount = TRUE;	/* we don't do whole count on this keystroke */
	}
	return (FALSE);
}
/************************************************************************/
static void delpartfield(HWND hwnd, CHARRANGE * sr, int direction)	/* deletes to start or end of field */

{
	int curfield;
	CHARRANGE cr;
	HWND ew = MX(hwnd,hwed);

	curfield = findfield(hwnd,STARTFIELD);
	findfieldlimits(hwnd,curfield,&cr);	/* find limits of field */
	cr.cpMax -=1;	/* can't delete end of line */
	if (direction == -1)	/* if deleting to start of field */
		cr.cpMax = sr->cpMax;			
	else					/* to end */
		cr.cpMin = sr->cpMin;
	SendMessage(ew,EM_HIDESELECTION,TRUE,0);	/* hide selection while fiddling */
	SendMessage(ew,EM_EXSETSEL,0,(LPARAM)&cr);	/* set selection info */
	SendMessage(ew,EM_REPLACESEL,TRUE,(LPARAM)TEXT(""));	/* replace with empty */
	SendMessage(ew,EM_HIDESELECTION,FALSE,0);	/* enable */
}
/************************************************************************/
static void mdomenu(HWND hwnd, HMENU hMenu, UINT item, BOOL fSystemMenu)	/* sets menus */

{
	HWND ew = MX(hwnd,hwed);
	INDEX * FF = getowner(hwnd);
	int menuid = menu_getmid(hMenu);
	
	com_setenable(ERVIEW,XONLY,ON);		/* enable set for text */
	mcr_setmenu(FALSE);					/* sets macro menus */
	if (menuid == IDPM_ABBREVIATIONS)	/* abbrevs submenu menu */
		com_set(IDM_TOOLS_ABBREV_DETACH,abbrev_hasactiveset());
	if (menuid == IDPM_LABEL)	{
		CheckMenuRadioItem(hMenu,IDM_EDIT_LABEL0,IDM_EDIT_LABEL0+FLAGLIMIT,IDM_EDIT_LABEL0+MX(hwnd,tagvalue),MF_BYCOMMAND);
		menu_setlabelcolors(hMenu);
	}
	else if (menuid == IDPM_EDIT)	{		/* if edit menu */
		CHARRANGE cr;

		com_set(IDM_EDIT_FINDAGAIN,fs_testok(FF));
		SendMessage(ew,EM_EXGETSEL,0,(LPARAM)&cr);	/* get selection info */
		com_setenable(ESCONT,ONLY, cr.cpMin != cr.cpMax);	/* enable cut copy clear by selection */
		com_set(IDM_EDIT_PASTE,SendMessage(ew,EM_CANPASTE,0,0));	/* enable if can paste */
		com_set(IDM_EDIT_UNDO,SendMessage(ew,EM_CANUNDO,0,0));	/* enable if can undo */
		com_set(IDM_EDIT_REDO,SendMessage(ew,EM_CANREDO,0,0));	/* enable if can redo */
		com_set(IDM_EDIT_NEWABBREV,cr.cpMin != cr.cpMax && !countbreaks(ew,NULL));	/* abbrev if sel in single field */
		com_check(IDM_EDIT_DELETE,MX(hwnd,delstate));
		if (MX(hwnd,recnum) <= FF->head.rtot)		/* if not a new record */
			com_set(IDM_EDIT_DUPLICATE, ON);
	}
//	else				/* clear check marks */
//		com_check(IDM_EDIT_DELETE,FALSE);
	if (menuid == IDPM_CHARACTERS)		/* if char menu */
		setstylemenu(ew,FF->head.fm);
}
/******************************************************************************/
static void setstylemenu(HWND ew, FONTMAP * fmp)		/* checks items on style menu */

{
	TCHAR name[LF_FACESIZE];
	int style = getstyleflags(ew,NULL,name);

	com_check(IDM_STYLE_BOLD,style&FX_BOLD);	/* check by state */
	com_check(IDM_STYLE_ITALIC,style&FX_ITAL);	/* check by state */
	com_check(IDM_STYLE_UNDERLINE,style&FX_ULINE);	/* check by state */
	com_check(IDM_FONT_DEFAULT,*name && !strcmp(fmp[0].name,fromNative(name)));	/* check by state */
	com_check(IDM_STYLE_SMALLCAPS,style&FX_SMALL);	/* check by state */
	com_check(IDM_STYLE_SUPER,style&FX_SUPER);	/* check by state */
	com_check(IDM_STYLE_SUB,style&FX_SUB);	/* check by state */
	com_check(IDM_STYLE_REGULAR,!(style&(FX_BOLD|FX_ITAL|FX_ULINE|FX_SMALL|FX_SUB|FX_SUPER|FX_OFF)));
}
/******************************************************************************/
static BOOL mcreate(HWND hwnd,LPCREATESTRUCT cs)	/* initializes text window */

{
	MFLIST * mfp;
	DWORD style;
	INDEX * FF;
	int count, height, index;
	
	if (mfp = getmem(sizeof(MFLIST)))	{	/* if can get memory for our window structure */
		setdata(hwnd,mfp);			/* install our memory */
		FF = WX(hwnd,owner) = (INDEX *)cs->lpCreateParams;
		FF->rwind = hwnd;
		mfp->nptr = sort_setuplist(FF);	/* set up sort list */
		for (count = 0; count < FIELDLIM; count++)	{	/* compile any regexes */
			if (*FF->head.indexpars.field[count].matchtext)		/* if need regex as template */
				mfp->regex[count] = regex_build(FF->head.indexpars.field[count].matchtext,0);
		}
		mfp->pf.cbSize = sizeof(mfp->pf);
		mfp->pf.dwMask = PFM_OFFSET| PFM_SPACEBEFORE| PFM_SPACEAFTER| PFM_LINESPACING;
		mfp->pf.dxOffset = LX(FF->vwind,emwidth)*TWIPS_PER_POINT;
		mfp->pf.dySpaceBefore = 0;
		mfp->pf.dySpaceAfter = 0;
		mfp->pf.bLineSpacingRule = 0;
		mfp->curf.cbSize = sizeof(mfp->curf);	// pre paste/drop format
		mfp->df.cbSize = sizeof(mfp->df);
		nstrcpy(mfp->df.szFaceName,toNative(FF->head.fm[0].name));
		mfp->df.bCharSet = ANSI_CHARSET;				/* just in case can't get char set from below */
		if ((index = type_findindex(FF->head.fm[0].name)) >= 0)	/* if font OK */
			mfp->df.bCharSet = t_fontlist.lf[index].elfLogFont.lfCharSet;	/* specify char set */
		mfp->df.bPitchAndFamily = DEFAULT_PITCH;
		height = g_prefs.gen.recordtextsize ? g_prefs.gen.recordtextsize : FF->head.privpars.size;
		if (height < MINHEIGHT)
			height = MINHEIGHT;
		else if (height > MAXHEIGHT)
			height = MAXHEIGHT;
		mfp->df.yHeight = height*TWIPS_PER_POINT;
		mfp->df.dwMask = CFM_BOLD|CFM_CHARSET|CFM_COLOR|CFM_FACE|CFM_SIZE|CFM_ITALIC|CFM_SUPERSCRIPT|CFM_SUBSCRIPT|CFM_UNDERLINE|CFM_SMALLCAPS;
		mfp->df.crTextColor = GetSysColor(COLOR_ACTIVECAPTION);

		style = WS_CHILD|WS_VISIBLE|ES_MULTILINE|ES_RIGHT|WS_DISABLED; 
		mfp->hwpr = CreateWindowEx(0,RICHEDIT_CLASS,NULL,style,0,0,0,0,hwnd,(HMENU)IDM_PROMPT,g_hinst,0);
		SendMessage(mfp->hwpr,EM_SETBKGNDCOLOR,FALSE,GetSysColor(COLOR_BTNFACE));
		mfp->df.dwEffects = 0;			/* bold is char style */
		SendMessage(mfp->hwpr,EM_SETSEL,(WPARAM)-1,-1);	/* set insertion pt to end */
		SendMessage(mfp->hwpr,EM_SETCHARFORMAT,0,(LPARAM)&mfp->df);	/* set default character formatting */

		style = WS_VSCROLL|WS_CHILD|WS_VISIBLE|ES_MULTILINE|ES_AUTOVSCROLL|ES_DISABLENOSCROLL|ES_SAVESEL;
		mfp->hwed = CreateWindowEx(WS_EX_CLIENTEDGE,RICHEDIT_CLASS,NULL,style,0,0,0,0,hwnd,(HMENU)IDM_EDIT,g_hinst,0);
		SendMessage(mfp->hwed, EM_SETLIMITTEXT,FF->head.indexpars.recsize*2,0);
		SendMessage(mfp->hwed,EM_SETSEL,(WPARAM)-1,-1);	/* set insertion pt to end */
		SendMessage(mfp->hwed, EM_SETEDITSTYLE, SES_XLTCRCRLFTOCR, SES_XLTCRCRLFTOCR);	// translate all cr/lf to cr
//		SendMessage(mfp->hwed,EM_SETEDITSTYLE,SES_XLTCRCRLFTOCR| SES_SMARTDRAGDROP,SES_XLTCRCRLFTOCR| SES_SMARTDRAGDROP);	// translate all cr/lf to cr
		// default lang options:
		SendMessage(mfp->hwed,EM_SETLANGOPTIONS,0,(LPARAM)(IMF_AUTOFONT|IMF_DUALFONT|IMF_AUTOKEYBOARD));	

//		mfp->df.dwEffects = CFE_AUTOCOLOR;		/* default color */
		SendMessage(mfp->hwed,EM_SETCHARFORMAT,0,(LPARAM)&mfp->df);	/* set default character formatting */
		mfp->reole = rw_callback;
		mfp->reole.droptest = checkdroppaste;
		mfp->reole.dropsite = checkdropsite;
		mfp->reole.contextmenu = getcontextmenu;
		mfp->reole.hwnd = hwnd;
		SendMessage(mfp->hwed,EM_SETOLECALLBACK,0,(LPARAM)&mfp->reole);
		SendMessage(mfp->hwed,EM_GETOLEINTERFACE,0,(LPARAM)&mfp->ole);
		mfp->ole->lpVtbl->QueryInterface(mfp->ole,&IID_ITextDocument, &mfp->itd);

		SendMessage(mfp->hwed,EM_SETEVENTMASK,0,ENM_SELCHANGE|ENM_KEYEVENTS|ENM_MOUSEEVENTS|ENM_CHANGE|ENM_DRAGDROPDONE);	/* set event mask */
		(LONG_PTR)mfp->edproc = SetWindowLongPtr(mfp->hwed, GWLP_WNDPROC,(LONG_PTR)hook);		/* set subclass handler */
//		mfp->hwst = CreateStatusWindow(WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|SBARS_SIZEGRIP,TEXT(""),hwnd,IDM_STATUS);
//		SendMessage(mfp->hwst,SB_SETPARTS,STAT_NPARTS,(LPARAM)statwidths);
		mfp->hwst = CreateWindowEx(
			0,                       // no extended styles
			STATUSCLASSNAME,         // name of status bar class
			(PCTSTR)NULL,           // no text when first created
			SBARS_SIZEGRIP |         // includes a sizing grip
			WS_CHILD | WS_VISIBLE,   // creates a visible child window
			0, 0, 0, 0,              // ignores size and position
			hwnd,					// handle to parent window
			(HMENU)IDM_STATUS,       // child window identifier
			g_hinst,                   // handle to application instance
			NULL);                   // no window creation data
		configurestatusbar(mfp->hwst, STAT_NPARTS, statwidths);
		SendMessage(g_hwstatus, WM_SETFONT, (WPARAM)CreateFontIndirect(&getconfigdpi(hwnd)->lfStatusFont), MAKELPARAM(FALSE, 0));
		SendMessage(mfp->hwst,SB_SETTEXT,7,(LPARAM)TEXT("INS"));	/* status */
#if 0
		mfp->hwtb = CreateToolbarEx(hwnd,TBSTYLE_TOOLTIPS|TBSTYLE_ALTDRAG|
			WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|CCS_TOP|CCS_NODIVIDER|CCS_ADJUSTABLE,
			IDM_TOOL,
			MB_MTOTAL,
			g_hinst,
			IDR_MODTOOLS,
			tbb,
			TBSTANDARDSIZE,
			0,0,0,0,
			sizeof(TBBUTTON));
#else
		mfp->hwtb =createtoolbar(hwnd);
#endif

		if (!reg_getkeyvalue(K_TOOLBARS,M_TBREGISTERD,NULL,NULL))	/* if don't have a default setting */
			com_tbsaverestore(mfp->hwtb,hwnd,TRUE, M_TBREGISTERD);	/* save it */
		mfp->hwstatic = CreateWindowEx(0,TEXT("STATIC"),TEXT(""),
			WS_CHILD|WS_VISIBLE|SS_LEFT,
			0,0,0,0,		/* xpos,ypos, wid, height */
			mfp->hwtb,
			NULL,
			g_hinst,
			NULL);
		layoutForDpi(mfp->hwstatic,4,4,90,16);
//		SendMessage(mfp->hwstatic,WM_SETFONT,(WPARAM)GetStockObject(DEFAULT_GUI_FONT),0);
		SendMessage(mfp->hwstatic, WM_SETFONT, (WPARAM)CreateFontIndirect(&getconfigdpi(hwnd)->lfStatusFont), MAKELPARAM(FALSE, 0));
		com_tbsaverestore(mfp->hwtb,hwnd,FALSE, M_TBREGISTERC);	/* retrieves current config */
		if (FF->head.privpars.rwrect.top == FF->head.privpars.rwrect.bottom)	{	/* if never been shown before */
			RECT mrect,vcrect;
			GetClientRect(FF->vwind,&vcrect);	// set up size & position based on view window
			mrect.bottom = vcrect.bottom;
#if 0
			mrect.top = vcrect.bottom-scaleForDpi(hwnd,MODBASEY);
			mrect.left = vcrect.left+((vcrect.right-vcrect.left)-scaleForDpi(hwnd, MODBASEX))/2;
			mrect.right = mrect.left+scaleForDpi(hwnd, MODBASEX);
#else
			mrect.top = vcrect.bottom - MODBASEY;
			mrect.left = vcrect.left + ((vcrect.right - vcrect.left) - MODBASEX) / 2;
			mrect.right = mrect.left + MODBASEX;
#endif
			FF->head.privpars.rwrect = mrect;
		}
		SetFocus(GetParent(hwnd));	// set focus via relevant container
		return (TRUE);
	}
	return (FALSE);
}
/************************************************************************/
static void normalizetext(HWND hwnd)	// normalizes text after paste/drop

{
	MFLIST * mfp = getdata(hwnd);
	CHARFORMAT2 df = mfp->df;	// get default format
	CHARFORMAT2 cf;
	long baseheight, smallc;
	CHARRANGE  prange;
	TEXTRANGE tr;
	TCHAR buff[2];

	memset(&cf,0,sizeof(cf));		// set up char format struct
	cf.cbSize = sizeof(cf);
	df.dwMask = CFM_SIZE|CFM_COLOR;		// set only size and color
	df.crTextColor = g_prefs.gen.flagcolor[mfp->tagvalue];	// set label color if any
	baseheight = df.yHeight;	//  height of normal text
	smallc = smallcHeight(baseheight);	// height of small caps

	int oldmask = SendMessage(mfp->hwed, EM_SETEVENTMASK, 0, ENM_NONE);	/* turn off selchange/update events */
	SendMessage(mfp->hwed, EM_EXGETSEL, 0, (LPARAM)&prange);	// get selection info (end position of paste/drop)
	prange.cpMin = mfp->currentselection.cpMin;			// set start of paste range
	mfp->itd->lpVtbl->Undo(mfp->itd, tomSuspend, NULL);
	SendMessage(mfp->hwed, EM_EXSETSEL, 0, (LPARAM)&prange);	// set selection range as whole of pasted text
//	if (GetAsyncKeyState(VK_MENU) && g_prefs.gen.pastemode != PASTEMODE_STYLEONLY || !GetAsyncKeyState(VK_MENU) && g_prefs.gen.pastemode == PASTEMODE_STYLEONLY) {
//	if (GetAsyncKeyState(VK_MENU) < 0) {	// if want to discard font info
	if (mfp->reole.pastemode == PASTEMODE_STYLEONLY) {	// if want to discard font info
		char * fname = getowner(hwnd)->head.fm[0].name;
		int index = type_findindex(fname) >= 0;		// get logfont index
		cf.dwMask = CFM_FACE | CFM_CHARSET;
		nstrcpy(cf.szFaceName, toNative(fname));
		cf.bCharSet = t_fontlist.lf[index].elfLogFont.lfCharSet;	/* specify char set */
		SendMessage(mfp->hwed, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);	/* set font */
	}
	SendMessage(mfp->hwed, EM_SETPARAFORMAT, 0, (LPARAM)&mfp->pf);	// set default para formatting
	for (; prange.cpMin < prange.cpMax; prange.cpMin++)	{	// for all characters in pasted range
		SendMessage(mfp->hwed,EM_SETSEL,(WPARAM)prange.cpMin,(LPARAM)prange.cpMin+1);	// select char				
		SendMessage(mfp->hwed,EM_GETCHARFORMAT,SCF_SELECTION,(LPARAM)&cf);
		if (cf.dwEffects&CFE_SMALLCAPS)
			df.yHeight = smallc;
		else
			df.yHeight = baseheight;
		SendMessage(mfp->hwed,EM_SETCHARFORMAT,SCF_SELECTION,(LPARAM)&df);
	}
	// check/remove unwanted chars on end of drop/paste
	tr.chrg = prange;
	tr.lpstrText = buff;
	do {
		tr.chrg.cpMin = tr.chrg.cpMax - 1;	// char to left of current pos
		SendMessage(mfp->hwed, EM_GETTEXTRANGE, 0, (LPARAM)&tr);
	} while (u_iscntrl(*buff) && --tr.chrg.cpMax > mfp->currentselection.cpMin);
	if (tr.chrg.cpMax < prange.cpMax) {		// if need to discard characters
		tr.chrg.cpMin = tr.chrg.cpMax;	// set range to be removed
		tr.chrg.cpMax = prange.cpMax;
		SendMessage(mfp->hwed, EM_EXSETSEL, 0, (LPARAM)&tr.chrg);	// set selection info
		SendMessage(mfp->hwed, EM_REPLACESEL, 1, (LPARAM)"");	// clear last char
		prange.cpMin = prange.cpMax = tr.chrg.cpMin;	// reset end of pasted range
	}
	mfp->itd->lpVtbl->Undo(mfp->itd, tomResume, NULL);
	SendMessage(mfp->hwed, EM_EXSETSEL, 0, (LPARAM)&prange);
	SendMessage(mfp->hwed, EM_SETEVENTMASK, 0, oldmask);	/* turn on events */
	SendMessage(mfp->hwed, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&mfp->curf);	// restore format after paste
	SendMessage(hwnd, WM_COMMAND, MAKELONG(IDM_EDIT, EN_CHANGE), (LPARAM)mfp->hwed);	/* flag change to force other updates */
}
/************************************************************************/
static LRESULT CALLBACK hook(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)

{
	HWND parent = GetParent(hwnd);
	LRESULT result;

	if (msg == WM_KILLFOCUS)	{	/* if losing focus */
		if ((HWND)wParam == g_hwfcb)	/* if focus going to font combo */
			SendMessage(hwnd,EM_HIDESELECTION,FALSE,0);	/* make sure selection remains shown */
		else
			dobuttons(parent,BUTTONS_NONE);
	}
	else if (msg == WM_SETFOCUS)	{
		SendMessage(hwnd,EM_HIDESELECTION,TRUE,0);	/* normal state is to hide selection when deactivated */
		getowner(parent)->currentfocus = parent;
		setbasetoolbar(parent);
	}
	else if (msg == WM_HSCROLL)	// never allow horizontal scroll (could be from wheel)
		return 0;
	result = CallWindowProc((WNDPROC)MX(parent,edproc),hwnd,msg,wParam,lParam);	/* pass to ordinary handler */
	if (msg == WM_VSCROLL || msg == WM_MOUSEWHEEL)		/* so that we can scroll prompt control (do this after riched has seen it) */
		checkpromptpos(parent);	/* check for prompt alignment */
	return result;
}
/************************************************************************/
static void msetfocus(HWND hwnd, HWND oldfocus)	/* gets focus shift message */

{
	if (getdata(hwnd))	{		/* if we're receiving focus */
		SetFocus(MX(hwnd,hwed));	/* pass on to edit window */
//		getowner(hwnd)->currentfocus = hwnd;
	}
}
/******************************************************************************/
void mselall(HWND hwnd)		/* selects all text */

{
	CHARRANGE cr;

	cr.cpMin = 0;
	cr.cpMax = -1;
	SendMessage(MX(hwnd,hwed),EM_EXSETSEL,0,(LPARAM)&cr);
}
/******************************************************************************/
static void mpaste(HWND hwnd)		/* pastes */

{
	SendMessage(MX(hwnd,hwed),WM_PASTE,0,0);
}
#if 0
/******************************************************************************/
static LRESULT checkdroppaste(RECALLBACK * reptr, int mode, TCHAR * string)	/* CALLBACK from riched ole; returns paste/drop error */

{
	INDEX * FF = getowner(reptr->hwnd);
	MFLIST * mfp = getdata(reptr->hwnd);
	int fcount, selbreaks, moveflag;
	CHARRANGE cr;

	selbreaks = countbreaks(mfp->hwed,&cr);
	if (mfp->hwed == GetFocus() && mode == RECO_DROP && GetAsyncKeyState(VK_LCONTROL) >= 0)	/* if we're moving in our own window */
		moveflag = TRUE;
	else
		moveflag = FALSE;		/* no allowance for selection */
	fcount = reptr->bcount + mfp->fcount - (moveflag ? 0 : selbreaks);	/* count number of field breaks in selection */
	if (fcount > FF->head.indexpars.maxfields)
		return (entryerr(mfp->hwst,ERR_TOOMANYFIELDS));
	else if (fcount < FF->head.indexpars.minfields)
		return (entryerr(mfp->hwst,ERR_TOOFEWFIELDS));
	else if (FF->head.indexpars.recsize <= mfp->totlen + (moveflag ? 0 : reptr->length)
		-(mode == RECO_DROP ? 0 : cr.cpMax-cr.cpMin))
		return (entryerr(mfp->hwst,ERR_RECOVERFLOW));
	if (reptr->length)	{
		TCHAR * lastcptr = string+reptr->length-1;

		if ((*lastcptr == '\n' || !*lastcptr) && lastcptr > string)	// if last char will be discarded
			lastcptr--;
		mfp->lastpastechar = *lastcptr;	 // save real last char for checking
	}
	else
		mfp->lastpastechar = 0;
	if (mode == RECO_PASTE)		{
		if (cr.cpMax > cr.cpMin && mod_getcharatpos(mfp->hwed,cr.cpMax-1) == '\r' && mfp->lastpastechar != '\r')	{	// if sel ends with newline and paste doesn't
			cr.cpMax--;
			SendMessage(mfp->hwed,EM_EXSETSEL,0,(LPARAM)&cr);	// remove last char from selection, to preserve newline
		}
		if (reptr->bcount && !selbreaks)	{	/* if pasting with breaks */
			int pastefield = findfield(reptr->hwnd,cr.cpMin);
			if (pastefield == mfp->fcount-1 || pastefield == mfp->fcount-2 && FF->head.indexpars.required)	/* if solely in protected field */
				return (entryerr(mfp->hwst,ERR_BREAKPROTECTEDFIELD));
		}
	}
	SendMessage(mfp->hwed, EM_EXGETSEL, 0, (LPARAM)&mfp->currentselection);	// save selection info
	mfp->doingpastedrop = TRUE;
	return (0);
}
#else
/******************************************************************************/
static LRESULT checkdroppaste(RECALLBACK * reptr, int mode, TCHAR * string)	/* CALLBACK from riched ole; returns paste/drop error */

{
	INDEX * FF = getowner(reptr->hwnd);
	MFLIST * mfp = getdata(reptr->hwnd);
	int fcount, selbreaks, moveflag;
	CHARRANGE cr;

	SendMessage(mfp->hwed, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&mfp->curf);	// save format at paste point
	selbreaks = countbreaks(mfp->hwed, &cr);
	if (mfp->hwed == GetFocus() && mode == RECO_DROP && GetAsyncKeyState(VK_LCONTROL) >= 0)	/* if we're moving in our own window */
		moveflag = TRUE;
	else
		moveflag = FALSE;		/* no allowance for selection */
	fcount = reptr->bcount + mfp->fcount - (moveflag ? 0 : selbreaks);	/* count number of field breaks in selection */
	if (fcount > FF->head.indexpars.maxfields)
		return (entryerr(mfp->hwst, ERR_TOOMANYFIELDS));
	else if (fcount < FF->head.indexpars.minfields)
		return (entryerr(mfp->hwst, ERR_TOOFEWFIELDS));
	else if (FF->head.indexpars.recsize <= mfp->totlen + (moveflag ? 0 : reptr->length)
		- (mode == RECO_DROP ? 0 : cr.cpMax - cr.cpMin))
		return (entryerr(mfp->hwst, ERR_RECOVERFLOW));
#if 1
	if (reptr->length) {
		TCHAR * lastcptr = string + reptr->length - 1;
		while (lastcptr > string && (*lastcptr == '\n' || *lastcptr == '\r' || !*lastcptr)) {
			if (*lastcptr == '\n')	// if removing a newline
				reptr->bcount--;	// remove a break count
			lastcptr--;
		}
	}
#endif
	if (mode == RECO_PASTE) {
		if (cr.cpMax > cr.cpMin && mod_getcharatpos(mfp->hwed, cr.cpMax - 1) == '\r') {	// if sel ends with newline and paste doesn't
			cr.cpMax--;
			SendMessage(mfp->hwed, EM_EXSETSEL, 0, (LPARAM)&cr);	// remove last char from selection, to preserve newline
		}
		if (reptr->bcount && !selbreaks) {	/* if pasting with breaks */
			int pastefield = findfield(reptr->hwnd, cr.cpMin);
			if (pastefield == mfp->fcount - 1 || pastefield == mfp->fcount - 2 && FF->head.indexpars.required)	/* if solely in protected field */
				return (entryerr(mfp->hwst, ERR_BREAKPROTECTEDFIELD));
		}
	}
	SendMessage(mfp->hwed, EM_EXGETSEL, 0, (LPARAM)&mfp->currentselection);	// save selection info

	HWND cbo = GetClipboardOwner();
	DWORD cbid, frameid;
	GetWindowThreadProcessId(cbo, &cbid);
	GetWindowThreadProcessId(g_hwframe, &frameid);
	BOOL plainmode = GetAsyncKeyState(VK_LSHIFT) < 0 && GetAsyncKeyState(VK_MENU) >= 0;
	BOOL stylemode = GetAsyncKeyState(VK_LSHIFT) >= 0 && GetAsyncKeyState(VK_MENU) < 0;

	reptr->pastemode = PASTEMODE_ALL;	// default all
	if (cbid != frameid) {		// if external paste, toggle by global settings
		if (!stylemode && (plainmode && g_prefs.gen.pastemode != PASTEMODE_PLAIN || !plainmode && g_prefs.gen.pastemode == PASTEMODE_PLAIN))
			reptr->pastemode = PASTEMODE_PLAIN;
		else if (stylemode && g_prefs.gen.pastemode != PASTEMODE_STYLEONLY || !stylemode && g_prefs.gen.pastemode == PASTEMODE_STYLEONLY)
			reptr->pastemode = PASTEMODE_STYLEONLY;
	}
	else {	// internal paste always in full style unless overridden
		if (plainmode)
			reptr->pastemode = PASTEMODE_PLAIN;
		else if (stylemode)
			reptr->pastemode = PASTEMODE_STYLEONLY;
	}
	mfp->doingpastedrop = TRUE;
	return (0);
}
#endif
/******************************************************************************/
static LRESULT checkdropsite(RECALLBACK * reptr)	/* CALLBACK from riched ole; returns error */

{
	if (reptr->bcount)	{		/* if drop contains breaks */
		MFLIST * mfp = getdata(reptr->hwnd);
		INDEX * FF = getowner(reptr->hwnd);
		POINT pt;
		int curpos, dropfield;

		GetCursorPos(&pt);
		ScreenToClient(mfp->hwed,&pt);
		curpos = SendMessage(mfp->hwed,EM_CHARFROMPOS,0,(LPARAM)&pt);	/* get char pos */
		dropfield = findfield(reptr->hwnd,curpos);
		if (dropfield == mfp->fcount-1 || dropfield == mfp->fcount-2 && FF->head.indexpars.required)	/* if page field */
			return (entryerr(mfp->hwst,ERR_BREAKPROTECTEDFIELD));
	}
	return (0);
}
/************************************************************************/
static LRESULT getcontextmenu(RECALLBACK * reptr)	/* does context menu */

{
	HMENU mh;
	HMENU mhs;
	HWND ew;
	CHARRANGE cr;
	int style;
	TCHAR name[LF_FACESIZE];
	FONTMAP *fmp;

	if (mh = LoadMenu(g_hinst,MAKEINTRESOURCE(IDR_POPUPS)))	{
		if (mhs = GetSubMenu(mh,0))	{		/* if can get submenu */
			ew = MX(reptr->hwnd,hwed);
			fmp = WX(reptr->hwnd,owner)->head.fm;
			SendMessage(ew,EM_EXGETSEL,0,(LPARAM)&cr);	/* get selection info */
			EnableMenuItem(mh,IDM_EDIT_CUT,cr.cpMax > cr.cpMin ? MF_BYCOMMAND|MF_ENABLED : MF_BYCOMMAND|MF_GRAYED);
			EnableMenuItem(mh,IDM_EDIT_COPY,cr.cpMax > cr.cpMin ? MF_BYCOMMAND|MF_ENABLED : MF_BYCOMMAND|MF_GRAYED);
			EnableMenuItem(mh,IDM_EDIT_PASTE,SendMessage(ew,EM_CANPASTE,0,0) ? MF_BYCOMMAND|MF_ENABLED : MF_BYCOMMAND|MF_GRAYED);
			EnableMenuItem(mh,IDM_EDIT_NEWABBREV,cr.cpMax > cr.cpMin && !countbreaks(ew,NULL) ? MF_BYCOMMAND|MF_ENABLED : MF_BYCOMMAND|MF_GRAYED);
			style = getstyleflags(ew,NULL,name);
			CheckMenuItem(mh,IDM_FONT_DEFAULT, *name && !strcmp(fmp[0].name,fromNative(name)) ? MF_BYCOMMAND|MF_CHECKED : MF_BYCOMMAND|MF_UNCHECKED);	/* set it */
			CheckMenuItem(mh,IDM_STYLE_BOLD, style&FX_BOLD ? MF_BYCOMMAND|MF_CHECKED : MF_BYCOMMAND|MF_UNCHECKED);	/* set it */
			CheckMenuItem(mh,IDM_STYLE_ITALIC, style&FX_ITAL ? MF_BYCOMMAND|MF_CHECKED : MF_BYCOMMAND|MF_UNCHECKED);	/* set it */
			CheckMenuItem(mh,IDM_STYLE_UNDERLINE, style&FX_ULINE ? MF_BYCOMMAND|MF_CHECKED : MF_BYCOMMAND|MF_UNCHECKED);	/* set it */
			CheckMenuItem(mh,IDM_STYLE_SMALLCAPS, style&FX_SMALL ? MF_BYCOMMAND|MF_CHECKED : MF_BYCOMMAND|MF_UNCHECKED);	/* set it */
			CheckMenuItem(mh,IDM_STYLE_SUPER, style&FX_SUPER ? MF_BYCOMMAND|MF_CHECKED : MF_BYCOMMAND|MF_UNCHECKED);	/* set it */
			CheckMenuItem(mh,IDM_STYLE_SUB, style&FX_SUB ? MF_BYCOMMAND|MF_CHECKED : MF_BYCOMMAND|MF_UNCHECKED);	/* set it */
			CheckMenuItem(mh,IDM_STYLE_REGULAR,!(style&(FX_BOLD|FX_ITAL|FX_ULINE|FX_SMALL|FX_SUB|FX_SUPER|FX_OFF))? MF_BYCOMMAND|MF_CHECKED : MF_BYCOMMAND|MF_UNCHECKED);
			return (LRESULT)mhs;
		}
	}
	return (0);		/* something wrong */
}
/******************************************************************************/
BOOL mod_close(HWND hwnd,int check)		/* closes modify window */

{
	if (check == MREC_FORCEDISCARD || mod_canenterrecord(hwnd,check))	{
		MFLIST * mfp = getdata(hwnd);
		INDEX * FF = getowner(hwnd);
		int returnflag, redisplay, count;

		for (count = 0; count < FF->head.indexpars.maxfields; count++)
			uregex_close(mfp->regex[count]);
		returnflag = g_prefs.gen.vreturn || mfp->addflag && !g_prefs.gen.track;
		redisplay = returnflag || mfp->vcont.priv.vmode == VM_FULL && g_prefs.gen.switchview || mfp->nptr->tot;
		view_restorecontext(FF,&mfp->vcont,returnflag);		/* restore view context */
		FF->rwind = NULL;		/* remove us */
		GetWindowRect(hwnd,&FF->head.privpars.rwrect);
		ScreenToClient(g_hwclient,(POINT *)&FF->head.privpars.rwrect.left);	/* now in MDI client co-ordinates */
		ScreenToClient(g_hwclient,(POINT *)&FF->head.privpars.rwrect.right);	/* now in MDI client co-ordinates */
		if (!FF->rcwind)	// if not a floating window
			FF->head.privpars.rwrect.bottom -= 4;	// adjustment for band width
		scaleRectFromDpi(hwnd, &FF->head.privpars.rwrect);
		sort_resortlist(FF,mfp->nptr);	/* make new nodes */
		freemem(mfp->nptr);		/* free sort list */
		mfp->itd->lpVtbl->Release(mfp->itd);
		mfp->ole->lpVtbl->Release(mfp->ole);
		ImageList_Destroy(mfp->imagelist);
		ImageList_Destroy(mfp->imagelistdisabled);
		SetWindowLongPtr(mfp->hwed, GWLP_WNDPROC,(LONG_PTR)mfp->edproc);
		DestroyWindow(mfp->hwed);
		freedata(hwnd);		/* releases window data */
		DestroyWindow(hwnd);	// destroy
		container_removerecordwindow(FF->cwind);
		SetFocus(FF->vwind);
		if (redisplay)
			view_redisplay(FF,0,VD_CUR|VD_RESET);	/* redisplay whole screen, perhaps changed */
//		SendMessage(FF->vwind,WMM_UPDATETOOLBARS,0,0);		// get toolbars reset for absence of record window
		return TRUE;
	}
	return FALSE;
}
/************************************************************************/
static void mlimits(HWND hwnd, LPMINMAXINFO lpMinMaxInfo)	/* limits tracking */

{
	lpMinMaxInfo->ptMinTrackSize.x = scaleForDpi(hwnd,MODMINX);
	lpMinMaxInfo->ptMinTrackSize.y = scaleForDpi(hwnd,MODMINY);
	FORWARD_WM_GETMINMAXINFO(hwnd,lpMinMaxInfo, DefWindowProc);
}
/************************************************************************/
static void msize(HWND hwnd, UINT state, int cx, int cy)

{
	RECT crect, srect, trect;
	MFLIST *mfp = getdata(hwnd);
	int _PROMPTWIDTH = scaleForDpi(hwnd, PROMPTWIDTH);
	int _PGAP = scaleForDpi(hwnd, PGAP);
//	int _EDGE = scaleForDpi(hwnd, EDGE);
	int _EDGE = EDGE;

	GetClientRect(hwnd,&crect);
	GetWindowRect(mfp->hwst,&srect);		/* get status window rect */
	GetWindowRect(mfp->hwtb,&trect);		/* get toolbar window rect */
	crect.bottom -= srect.bottom-srect.top;		/* reduce by height of status window */
	crect.top += trect.bottom-trect.top;		/* and by toobar */
	MoveWindow(mfp->hwed,crect.left+_PROMPTWIDTH+_PGAP,crect.top,crect.right-crect.left-_PROMPTWIDTH-_PGAP,crect.bottom-crect.top,TRUE);	/* size riched */
	MoveWindow(mfp->hwpr,crect.left-_PROMPTWIDTH,crect.top+_EDGE,_PROMPTWIDTH*2,crect.bottom-crect.top-_EDGE,TRUE);	/* size prompt */
	doprompts(hwnd);		/* make sure prompts are right */
	SendMessage(mfp->hwtb,WM_SIZE,0,0);	/* size toolbar */
	SendMessage(mfp->hwst,WM_SIZE,0,0);	/* and status bar */
}
/******************************************************************************/
static void doprompts(HWND hwnd)	/* sets up prompts */

{
	MFLIST * mfp = getdata(hwnd);
	INDEX * FF = getowner(hwnd);
	HWND ew,pw;
	int elcount, lcount, dirtyline, length, fcount, selstart, lasttextindex;
	TCHAR line[MAXREC];
	TCHAR map[LMAPSIZE];

	ew = mfp->hwed;
	pw = mfp->hwpr;
	elcount = SendMessage(ew,EM_GETLINECOUNT,0,0);
	map[0] = 0;		/* first line always has prompt */
	for (lasttextindex = fcount = 0,dirtyline = -1, lcount = 0; lcount < elcount; lcount++)	{	/* for all beyond first line */
		length = Edit_GetLine(ew,lcount,line,MAXREC);		/* retrieve line */
		if (line[length-1] == RETURN)		{	/* if next line is start of field */
			map[lcount+1] = ++fcount;		/* set prompt index */
			if (lcount < elcount-1)	{	/* if not on very last line (which has spurious newline) */
				lasttextindex = mfp->pageindex;	// what was page field line becomes lastfield index
				mfp->pageindex = lcount+1;	/* page field begins on this line */
			}
		}
		else
			map[lcount+1] = (TCHAR)0xFF;		/* invalid prompt on this line */
		if (dirtyline < 0 && map[lcount] != mfp->map[lcount])	/* if differs from current state */
			dirtyline = lcount;
		mfp->map[lcount] = map[lcount];	/* copy new value */
	}
	mfp->protectindex = FF->head.indexpars.required ? lasttextindex : mfp->pageindex;
	if (fcount != mfp->fcount)	/* if changed number of fields */
		dirtyline = 0;			/* we've no idea which might be gained/lost field */
	if (dirtyline < 0 && lcount < SendMessage(pw,EM_GETLINECOUNT,0,0)-1)	/* if we're displaying more lines than we need */
		dirtyline = lcount;
	if (dirtyline >= 0)	{	/* if need change */
		selstart = SendMessage(pw,EM_LINEINDEX,dirtyline,0);
		SendMessage(pw,EM_SETSEL,selstart,-1);	/* select all bad text */
		SendMessage(pw,WM_CLEAR,0,0);			/* remove it */
		while (dirtyline < elcount)	{			/* from invalid line until end */
			if (map[dirtyline] != (TCHAR)0xFF)	{	/* if need prompt */
				int index = map[dirtyline];
				if (index == fcount-1)	// if page field
					index = PAGEINDEX;
				else if (index == fcount-2 && FF->head.indexpars.required)	// if required field
					index = FF->head.indexpars.maxfields-2;
				SendMessage(pw,EM_REPLACESEL,FALSE,(LPARAM)toNative(FF->head.indexpars.field[index].name));
			}
			SendMessage(pw,EM_REPLACESEL,FALSE,(LPARAM)NEWLINESTRING);
			dirtyline++;
		}
	}
	mfp->fcount = fcount;		/* save field count */
	checkpromptpos(hwnd);
}
/******************************************************************************/
static void checkpromptpos(HWND hwnd)		/* checks/adjusts scrolled position of prompts */

{	
	int pwtop, ewtop;

	ewtop = SendMessage(MX(hwnd,hwed),EM_GETFIRSTVISIBLELINE,0,0);	/* get top line in edit window */
	pwtop = SendMessage(MX(hwnd,hwpr),EM_GETFIRSTVISIBLELINE,0,0);	/* get top line in prompt window */
	if (pwtop != ewtop)		/* if prompt doesn't match */
		SendMessage(MX(hwnd,hwpr),EM_LINESCROLL,0,ewtop-pwtop);
}
/************************************************************************/
static void setbasetoolbar(HWND hwnd)	/* updates all toolbar buttons */

{
	HWND tb = MX(hwnd,hwtb);

	SendMessage(g_hwtoolbar,TB_ENABLEBUTTON,IDM_EDIT_NEWRECORD,TRUE);
	SendMessage(g_hwtoolbar,TB_ENABLEBUTTON,IDM_EDIT_DELETE,TRUE);	/* enable */
	SendMessage(g_hwtoolbar,TB_ENABLEBUTTON,IDM_EDIT_TAG,TRUE);	/* enable flag */
	SendMessage(tb,TB_ENABLEBUTTON,IDB_MOD_PREV,TRUE);
	SendMessage(tb,TB_ENABLEBUTTON,IDB_MOD_NEXT,TRUE);
	SendMessage(tb,TB_ENABLEBUTTON,IDM_STYLE_BOLD,TRUE);
	SendMessage(tb,TB_ENABLEBUTTON,IDM_STYLE_ITALIC,TRUE);
	SendMessage(tb,TB_ENABLEBUTTON,IDM_STYLE_UNDERLINE,TRUE);
	com_settextmenus(getowner(hwnd),ON,OFF);
	dobuttons(hwnd, BUTTONS_ALL);
}
/******************************************************************************/
static void dobuttons(HWND hwnd, int set)		/* checks/sets sets some buttons */

{	
	HWND ew = MX(hwnd,hwed);
	HWND tb = MX(hwnd,hwtb);

	if (GetFocus() != ew)	{
		com_setdefaultbuttons(FALSE);
		SendMessage(tb,TB_ENABLEBUTTON,IDB_MOD_PROPAGATE,FALSE);
		SendMessage(tb,TB_ENABLEBUTTON,IDB_MOD_BRACES,FALSE);
		SendMessage(tb,TB_ENABLEBUTTON,IDB_MOD_BRACKETS,FALSE);
		SendMessage(tb,TB_ENABLEBUTTON,IDB_MOD_FLIP,FALSE);
		SendMessage(tb,TB_ENABLEBUTTON,IDB_MOD_HALFFLIP,FALSE);
		SendMessage(tb,TB_ENABLEBUTTON,IDB_MOD_SWAPPAREN,FALSE);
		SendMessage(tb,TB_ENABLEBUTTON,IDM_FONT_DEFAULT,FALSE);
		SendMessage(tb,TB_ENABLEBUTTON,IDM_STYLE_BOLD,FALSE);
		SendMessage(tb,TB_ENABLEBUTTON,IDM_STYLE_ITALIC,FALSE);
		SendMessage(tb,TB_ENABLEBUTTON,IDM_STYLE_UNDERLINE,FALSE);
		SendMessage(tb,TB_ENABLEBUTTON,IDB_MOD_REVERT,FALSE);
		SendMessage(tb,TB_ENABLEBUTTON,IDB_MOD_PREV,FALSE);
		SendMessage(tb,TB_ENABLEBUTTON,IDB_MOD_NEXT,FALSE);
		SendMessage(tb,TB_ENABLEBUTTON,IDB_MOD_DUPNEW,FALSE);
	}
	else {
		MFLIST * mfp = getdata(hwnd);
		INDEX * FF = WX(hwnd,owner);
		int breakcount;
		TCHAR name[LF_FACESIZE];
		CHARRANGE cr;
		int style, enable;

		breakcount = countbreaks(ew,NULL);

		SendMessage(mfp->hwst,SB_SIMPLE,FALSE,0);	/* kills simple status window */
		SendMessage(g_hwtoolbar,TB_CHECKBUTTON,IDM_EDIT_DELETE,mfp->delstate);	/* set */
		SendMessage(g_hwtoolbar,TB_CHECKBUTTON,IDM_EDIT_TAG,mfp->tagvalue);	/* set */

		SendMessage(ew,EM_EXGETSEL,0,(LPARAM)&cr);	/* get selection info */
		SendMessage(g_hwtoolbar,TB_ENABLEBUTTON,IDM_EDIT_CUT,cr.cpMin < cr.cpMax);	/* set button */
		SendMessage(g_hwtoolbar,TB_ENABLEBUTTON,IDM_EDIT_COPY,cr.cpMin < cr.cpMax);	/* set button */
		SendMessage(g_hwtoolbar,TB_ENABLEBUTTON,IDM_EDIT_PASTE,SendMessage(ew,EM_CANPASTE,0,0));	/* set button */
		if (cr.cpMin < cr.cpMax && !breakcount)	{		/* if can embrace text */
			SendMessage(tb,TB_ENABLEBUTTON,IDB_MOD_BRACES,TRUE);	/* set button */
			SendMessage(tb,TB_ENABLEBUTTON,IDB_MOD_BRACKETS,TRUE);	/* set button */
		}
		else	{
			SendMessage(tb,TB_ENABLEBUTTON,IDB_MOD_BRACES,FALSE);	/* set button */
			SendMessage(tb,TB_ENABLEBUTTON,IDB_MOD_BRACKETS,FALSE);	/* set button */
		}
		style = getstyleflags(ew,NULL,name);
		SendMessage(tb,TB_ENABLEBUTTON,IDM_FONT_DEFAULT,!*name || strcmp(FF->head.fm[0].name,fromNative(name)));	/* set/clear */
		SendMessage(tb,TB_CHECKBUTTON,IDM_STYLE_BOLD,style&FX_BOLD);	/* set/clear */
		SendMessage(tb,TB_CHECKBUTTON,IDM_STYLE_ITALIC,style&FX_ITAL);	/* set/clear */
		SendMessage(tb,TB_CHECKBUTTON,IDM_STYLE_UNDERLINE,style&FX_ULINE);	/* set/clear */
		if (!*name || ComboBox_SelectString(g_hwfcb,-1,name) == CB_ERR)		// if no font, or bad font, or not continuous
			ComboBox_SetCurSel(g_hwfcb,-1);		/* clear selection */
		if (mfp->recnum <= FF->head.rtot)	{	/* if editing existing record */
			SendMessage(tb,TB_ENABLEBUTTON,IDB_MOD_PROPAGATE,TRUE);	/* enable */
			SendMessage(tb,TB_CHECKBUTTON,IDB_MOD_PROPAGATE,mfp->propstate);	/* set propagation */
		}
		else {
			SendMessage(tb,TB_ENABLEBUTTON,IDB_MOD_PROPAGATE,FALSE);	/* disable */
			SendMessage(tb,TB_CHECKBUTTON,IDB_MOD_PROPAGATE,FALSE);	/* no propagation */
		}
		enable = !breakcount && mfp->startfield != mfp->fcount-1;
		SendMessage(tb,TB_ENABLEBUTTON,IDB_MOD_FLIP,enable);
		SendMessage(tb,TB_ENABLEBUTTON,IDB_MOD_HALFFLIP,enable);
		SendMessage(tb,TB_ENABLEBUTTON,IDB_MOD_SWAPPAREN,enable);
		SendMessage(tb,TB_ENABLEBUTTON,IDB_MOD_REVERT,SendMessage(ew,EM_GETMODIFY,0,0));	/* can revert */
		SendMessage(mfp->hwtb,TB_ENABLEBUTTON,IDB_MOD_DUPNEW,!(!SendMessage(ew,EM_GETMODIFY,0,0) && mfp->recnum == FF->head.rtot+1));/* no duplicate if already a duplicate */
	}
}
/******************************************************************************/
static int getstyleflags(HWND ew, CHARFORMAT2 * cfp, TCHAR * fname)		// gets styles at selection or from specified char format

{
//	ITextDocument * itp = MX(GetParent(ew),itd);
//	long freezecount;
	CHARFORMAT2 cf;
	int sflags = 0;

//	itp->lpVtbl->Freeze(itp,&freezecount);	// freeze display
	if (cfp != NULL)	// if want styles from format
		cf = *cfp;
	else {	// else from current selection
		TCHAR tstring[MAXREC];
		CHARRANGE cr, ct;
		int oldmask;
		findtemprange(ew, tstring, &cr, &ct, &cf, &oldmask);
		undotemprange(ew, &cr, &ct, oldmask);	/* restore old selection */
	}
//	itp->lpVtbl->Unfreeze(itp,&freezecount);
	if (cf.dwEffects&CFE_BOLD && cf.dwMask&CFM_BOLD)
		sflags |= FX_BOLD;
	if (cf.dwEffects&CFE_ITALIC && cf.dwMask&CFM_ITALIC)
		sflags |= FX_ITAL;
	if (cf.dwEffects&CFE_UNDERLINE && cf.dwMask&CFM_UNDERLINE)
		sflags |= FX_ULINE;
	if (cf.dwEffects&CFE_SMALLCAPS && cf.dwMask&CFM_SMALLCAPS)
		sflags |= FX_SMALL;
	if (cf.dwEffects&CFE_SUPERSCRIPT && cf.dwMask&CFM_SUPERSCRIPT)
		sflags |= FX_SUPER;
	if (cf.dwEffects&CFE_SUBSCRIPT && cf.dwMask&CFM_SUBSCRIPT)
		sflags |= FX_SUB;
	if (cf.dwMask&CFM_FACE)
		nstrcpy(fname,cf.szFaceName);
	else
		*fname = '\0';
	return sflags;
}
/******************************************************************************/
static void doprevnext(HWND wptr, BOOL mod, BOOL next)	/* enters record and moves forward/backward */

{	
	if (mod_canenterrecord(wptr,MREC_ALWAYSACCEPT))	{		/* if can load record */
		INDEX * FF = getowner(wptr);
		MFLIST * mfp = getdata(wptr);
		RECORD * recptr;
		RECN newnum;

		if (mod && next)		/* if want to duplicate current record */
			loadfields(wptr,OLDTEXT, NEWREC);		/* reuse text */
		else	{									/* move forward or backwards */
			newnum = next ? mfp->nextnum : mfp->prevnum;
			if (newnum && (recptr = rec_getrec(FF,newnum)))	{	/* if have previous or next */
				loadfields(wptr,recptr->rtext, recptr->num);	/* set it up */
//				view_selectrec(FF,recptr->num, MX(wptr,addflag)? VD_BOTTOM : VD_MIDDLE,-1,-1);
				view_selectrec(FF,recptr->num, VD_BOTTOM,-1,-1);
			}
			else if (next && mfp->recnum == FF->head.rtot)	/* if just added new record */
				loadfields(wptr,NOTEXT, NEWREC);		/* allow new empty */
		}
	}
}
/******************************************************************************/
static int getprevtext(HWND hwnd, short keyindex)	/* loads identified field from previous record */

{
	MFLIST * mfp = getdata(hwnd);
	INDEX * FF = getowner(hwnd);
	char * sptr;
	int len, fcount;
	RECORD * recptr;
	char tstring[MAXREC];
	CSTR fstrings[FIELDLIM];
	CHARRANGE cr;

	keyindex -= '1';

	if (recptr = rec_getrec(FF,FF->lastedited))	{	/* if can get record */
		fcount = str_xparse(recptr->rtext,fstrings);	/* parse search string */
		if (keyindex < fcount-1)	{	/* if desired field exists */
			if (keyindex < 0)			/* if want page field */
				keyindex = fcount-1;	/* fix its index */
			sptr = fstrings[keyindex].str;
			strcpy(tstring, sptr);
			len = fstrings[keyindex].ln;
			if (mfp->fcount - countbreaks(mfp->hwed,&cr) < FF->head.indexpars.minfields)
				entryerr(mfp->hwst,ERR_TOOFEWFIELDS);
			else if (FF->head.indexpars.recsize < mfp->totlen + len - (cr.cpMax-cr.cpMin))
				entryerr(mfp->hwst,ERR_RECOVERFLOW);
			else	{	/* if enough room */
				tstring[len+1] = EOCS;
				settestring(hwnd, tstring, FF->head.fm, TRUE);	/* insert text */
				return (TRUE);
			}
		}
	}
	return (FALSE);
}
/**********************************************************************************/
static void loadfields(HWND wptr, char * rtext, RECN rnum)		/* loads text for editing */

/* usage:	if rnum = 0, then revert to stored;
			if rnum = NEWREC && rtext = NULL, then new empty record ; 
			if rnum = NEWREC && rtext != NULL, new record from existing;
			else do as specified */

{
	MFLIST * mfp = getdata(wptr);
	INDEX * FF = getowner(wptr);
	char tstring[MAXREC];
	TCHAR wstring[STSTRING];
	RECORD * recptr, *lrptr;

	mfp->completingSelection = FALSE;	// clear any dangling autocomplete flag
	if (recptr = rec_getrec(FF,rnum))	{	/* if record exists */
		rec_getprevnext(FF,recptr,&mfp->prevnum,&mfp->nextnum, LX(FF->vwind,skip));
		str_xcpy(mfp->rstring, recptr->rtext);
		mfp->recnum = rnum;				/* saved number */
	}
	else {
		if (rnum == NEWREC)	{		/* new record */
			if (!rtext)	{		/* make a new empty record */
				str_xcpy(tstring, g_nullrec);
				rec_pad(FF,tstring);
				if (g_prefs.gen.carryrefs && mfp->addflag && (lrptr = rec_getrec(FF,FF->head.rtot)))	/* if to carry page refs */
					str_xcpy(str_xlast(tstring),str_xlast(lrptr->rtext));	/* transfer locator & terminate */
				str_xcpy(mfp->rstring,tstring);		/* saved text */
			}
			if (mfp->addflag)	{		/* if in add mode */
				if (!g_prefs.gen.track && (lrptr = sort_bottom(FF)))		/* use sort-bottom, in case sorted view, or group */
					mfp->prevnum = lrptr->num;		/* set previous to be current last record */
				mfp->nextnum = 0;			/* there is never a next */
			}
			else
				mfp->prevnum = mfp->recnum;		// default prev becomes last edited; next is unchanged
			mfp->recnum = FF->head.rtot+1;		/* saved number */
			view_clearselect(FF->vwind);		/* clears selection */
		}
		// else recnum 0 means revert current to inital state
	}
	u_sprintf(wstring, mfp->recnum <= FF->head.rtot ? "Record %lu" : "New Record",mfp->recnum);
	SendMessage(mfp->hwstatic,WM_SETTEXT,0,(LPARAM)wstring);
	mfp->df.crTextColor = GetSysColor(COLOR_WINDOWTEXT);	// set default color
	mfp->compf = mfp->df;
	if (recptr = rec_getrec(FF,mfp->recnum))	{		/* if record exists */
		mfp->tagvalue = recptr->label;
		mfp->delstate = recptr->isdel;
		mfp->df.crTextColor = g_prefs.gen.flagcolor[recptr->label];	// set label color
	}
	else						/* new record */
		mfp->tagvalue = mfp->delstate = FALSE;
	mfp->statuschanged = FALSE;
	mfp->propstate = g_prefs.gen.propagate;		/* fix inijtial state of propagation */
	settestring(wptr,mfp->rstring, FF->head.fm,FALSE);	// set up text and context
	mfp->startfield = mfp->endfield = 0;	// set start and end fields to 0
	memset(&mfp->alarms,0,sizeof(mfp->alarms));		/* clear alarm fields */
	drawstatstring(wptr);	/* force update of status line */
}
/**********************************************************************************/
static void settestring(HWND wptr, char * rtext, FONTMAP * fmp, int selflag)	/* builds TE text from xstring */
	/* always discards NULL byte before EOCS; treats others as newlines */

{
	MFLIST * mfp = getdata(wptr);
	HWND ew = mfp->hwed;
	CHARFORMAT2 cf;
	short codes;
	long baseheight, smallc;
	TCHAR tstring[MAXREC], *tptr;
	char * rptr;
	RECONTEXT rec;

	saverecontext(ew,&rec);
	memset(mfp->map,(TCHAR)0xFF,LMAPSIZE);		/* nullify prompt map */
	cf = mfp->df;			/* get default format */
	baseheight = cf.yHeight;	/* use for scaling */
	smallc = smallcHeight(baseheight);
	if (!selflag)	{	// if replacing all the text
		SendMessage(ew,EM_SETSEL,0,-1);	/* set selection to all */
		SendMessage(ew,EM_SETCHARFORMAT,SCF_SELECTION,(LPARAM)&cf);	/* set default format (need if window had prev record) */
		SendMessage(ew,WM_CLEAR,0,0);
		SendMessage(ew,EM_SETSEL,0,0);
		SendMessage(ew,EM_SETPARAFORMAT,0,(LPARAM)&mfp->pf);	/* set para formatting */
	}
	else	{
//		mfp->itd->lpVtbl->Undo(mfp->itd,tomSuspend,NULL);	// suspend  undo
		cf.dwMask &= ~CFM_COLOR;	// don't change current color
	}
	SendMessage(ew,EM_SETCHARFORMAT,SCF_SELECTION,(LPARAM)&cf);	/* set default format (need if window had prev record) */
	for (rptr = rtext, tptr = tstring; *rptr != EOCS;)	{	/* prepare segments */
		unichar uc = u8_nextU(&rptr);		
		if (iscodechar(uc))	{
			*tptr = '\0';
			SendMessage(ew,EM_REPLACESEL,TRUE,(LPARAM)tstring);
			codes = *rptr++;	// get code and advance
			if (uc == FONTCHR)	{	/* if a font spec */
				int index;
//				if ((codes&FX_FONTMASK) >= FONTLIMIT)
//					senderr(ERR_INTERNALERR, WARN, "local font id too large");
				nstrcpy(cf.szFaceName,toNative(fmp[codes&FX_FONTMASK].name));
				if ((index = type_findindex(fmp[codes&FX_FONTMASK].name)) >= 0)	/* if have font */
					cf.bCharSet = t_fontlist.lf[index].elfLogFont.lfCharSet;	/* specify char set */
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
						cf.yHeight = baseheight;
						cf.dwEffects &= ~CFE_SMALLCAPS;
					}
					if (codes&FX_SUPER)
						cf.dwEffects &= ~CFE_SUPERSCRIPT;
					if (codes&FX_SUB)
						cf.dwEffects &= ~CFE_SUBSCRIPT;
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
					if (codes&FX_SUPER)	{
						cf.dwEffects |= CFE_SUPERSCRIPT;
					}
					if (codes&FX_SUB)	{	
						cf.dwEffects |= CFE_SUBSCRIPT;
					}
				}
			}
			SendMessage(ew,EM_SETCHARFORMAT,SCF_SELECTION,(LPARAM)&cf);	/* set changed style */
			tptr = tstring;	/* reset buffer */
			continue;
		}
		else if (!uc)	{	/* if end of field */
			*tptr++ = RETURN;	/* set end of line */
			*tptr++ = NEWLINE;	// NEED THIS
		}
		else
			*tptr++ = uc;
	}
	*(tptr-2) = '\0';		/* terminate string */
	SendMessage(ew,EM_REPLACESEL,TRUE,(LPARAM)tstring);	/* add residual text (permit undo) */
	if (!selflag)	{	// if replacing all text
//		SendMessage(ew,EM_SETSEL,0,0);			/* set to start */
		rec.cr.cpMin = rec.cr.cpMax = 0;		// set to start
		SendMessage(ew,EM_SCROLLCARET,0,0);		/* force into view */
		SendMessage(ew,EM_SETMODIFY,FALSE,0);	/* mark unmodified */
		SendMessage(ew,EM_EMPTYUNDOBUFFER,0,0);	/* clear undo */
	}
	else	{	// selection to restore is current insertion pt
#if 0
		CHARRANGE tcr = rec.cr;
		GETTEXTEX gtx;
		SETTEXTEX stx;
		TCHAR tstring[MAXREC*2];

		SendMessage(ew,EM_EXGETSEL,0,(LPARAM)&tcr);	// get current selection
		tcr.cpMin = rec.cr.cpMin;			// set start to original
		SendMessage(ew,EM_EXSETSEL,0,(LPARAM)&tcr);	// set selection to span replacement
		gtx.cb = MAXREC*2;
		gtx.flags = GT_SELECTION;
		gtx.codepage = 1200;
		gtx.lpDefaultChar = NULL;
		gtx.lpUsedDefChar = NULL;
		SendMessage(ew,EM_GETTEXTEX,(WPARAM)&gtx,(LPARAM)tstring);	// set selection to span replacement
		mfp->itd->lpVtbl->Undo(mfp->itd,tomResume,NULL);	// enable undo
		stx.flags = ST_DEFAULT;
		stx.codepage = 1200;
		SendMessage(ew,EM_SETTEXTEX,(WPARAM)&stx,(LPARAM)tstring);	// set selection to span replacement
#endif
		SendMessage(ew,EM_EXGETSEL,0,(LPARAM)&rec.cr);	// set selection to restore
	}
	restorerecontext(ew,&rec);
	SendMessage(wptr,WM_COMMAND,MAKELONG(IDM_EDIT,EN_CHANGE),(LPARAM)ew);	// flag change to force cleanup
}
/******************************************************************************/
BOOL mod_canenterrecord(HWND hwnd, BOOL flag)	// returns TRUE if window can close

{
	if (!canabandon(hwnd))	{	// if can't abandon
		int action = -1;		/* default action check is discard */
		if (flag == MREC_ALWAYSACCEPT || g_prefs.gen.saverule == M_SAVE || g_prefs.gen.saverule == M_ASK && (action = savewarning(WARN_RECCHANGED)) == 1)
			return enterrecord(hwnd);
		else if (action == 0)
			return (FALSE);
	}
	return TRUE;
}
/******************************************************************************/
static BOOL canabandon(HWND hwnd)		// returns true if can abandon record

{
	MFLIST * mfp = getdata(hwnd);
	INDEX * FF = getowner(hwnd);

	mfp->difflag = 0;		// assume no difference
	if (SendMessage(mfp->hwed,EM_GETMODIFY,0,0))	{	/* if changed */
		int fcount;

		copyfontmap(mfp->fm,FF->head.fm);
		mod_gettestring(mfp->hwed, mfp->revstring, mfp->fm, FALSE);	/* temp copy of retrieved record text */
		fcount = rec_strip(FF,mfp->revstring);	/* strip/pad empty fields */
		if (fcount < FF->head.indexpars.minfields)	{	/* if for any reason too few fields */
			senderr (ERR_INTERNALERR, WARN, "Lost Headings");
			return (TRUE);	// abandon record
		}
		mfp->difflag = str_xcmp(mfp->revstring, mfp->rstring);		/* could be the same even though dirty */
	}
	return !mfp->difflag && (!mfp->statuschanged || mfp->recnum > FF->head.rtot);
}
/******************************************************************************/
static short enterrecord(HWND wptr)		/* enters new/modified record */

{
	MFLIST * mfp = getdata(wptr);
	INDEX * FF = getowner(wptr);
	RECORD * recptr;
	short newflag;
	
	if (checkerrors(wptr,mfp->revstring))
		return (FALSE);
	if (str_xlen(mfp->revstring) > FF->head.indexpars.recsize-1)	{	/* if for any reason string too long */
		/* this can happen if using autocomplete and try to enter too-long completed text */
		entryerr(mfp->hwst,ERR_RECOVERFLOW);
		return (FALSE);
	}
	if (mfp->recnum > FF->head.rtot)	{	/* if a new record */
		if (recptr = rec_writenew(FF,mfp->revstring))	{
			view_setstatus(FF->vwind);			/* revise main window title */
			newflag = TRUE;
		}
		else
			return FALSE;
	}
	else if (recptr = rec_getrec(FF,mfp->recnum))	{	/* if existing record */
		str_xcpy(recptr->rtext,mfp->revstring);
		recptr->ismark = FALSE;			/* remove any mark */
		recptr->isgen = FALSE;			/* clear autogen flag */
		if (mfp->difflag || recptr->isdel != mfp->delstate || recptr->label != mfp->tagvalue && g_prefs.gen.labelsetsdate)
			rec_stamp(FF,recptr);
		newflag = FALSE;
	}
	else
		return (FALSE);
	copyfontmap(FF->head.fm,mfp->fm);	// set changed font map
	recptr->isdel = mfp->delstate;		// transfer status
	recptr->label = mfp->tagvalue;
	if (mfp->difflag)				// if text of record changed
		FF->lastedited = recptr->num;			/* save num as last-edited record */
	sort_addtolist(mfp->nptr,recptr->num);	/* add to sort list */
	if (newflag)	{							/* if a new record */
		sort_resortlist(FF,mfp->nptr);		/* bring sort up to date */
		if (!FF->curfile)	{				/* if group not active */
			if (mfp->addflag)	{	// if in add mode
				if (g_prefs.gen.track)	{	/* if tracking new records */
					rec_getprevnext(FF,recptr,&mfp->prevnum,&mfp->nextnum, LX(FF->vwind,skip));
					mfp->nextnum = 0;
					view_redisplay(FF,FF->head.rtot,VD_BOTTOM);	/* redisplay whole screen, perhaps changed */
				}
				else
					view_appendrec(FF,recptr->num);		/* append to existing set */
				view_selectrec(FF,FF->head.rtot,VD_BOTTOM,-1,0);	/* put new rec in right position */
			}
			else
				view_redisplay(FF,0,VD_CUR);	/* redisplay whole screen, perhaps changed */
		}
	}
	else {
		if (SendMessage(mfp->hwtb,TB_ISBUTTONCHECKED,IDB_MOD_PROPAGATE,0))	/* if wanting to propagate changes */
			rec_propagate(FF,recptr,mfp->rstring, mfp->nptr); 	/* do it */
		view_resetrec(FF,mfp->recnum);		/* just the modified one and other changed ones that follow */
	}
	str_xcpy(mfp->rstring, mfp->revstring);		/* keep a copy of new record */
	return (TRUE);
}
/**********************************************************************************/
int mod_getselection(HWND hwnd, char * sp, FONTMAP * fmp)	/* retrieves selected text as cin xstring */
 
{
	return mod_gettestring(MX(hwnd,hwed), sp,fmp, MREC_SELECT);
}
/******************************************************************************/
static short entryerr(HWND hwnd, const int errnum, ...)

{
	TCHAR text[STSTRING];
	TCHAR tbuff[MAXREC];
	va_list aptr;

	LoadString(g_hinst, errnum,text,STSTRING);
	va_start(aptr, errnum);		 /* initialize arg pointer */
	u_vsprintf_u(tbuff, text, aptr); /* get string */
	va_end(aptr);
//	MessageBeep(MB_ICONHAND);
	MessageBeep(MB_ICONEXCLAMATION);
//	SendMessage(hwnd,SB_SETTEXT,8,(LPARAM)tbuff);	// show message
	SendMessage(hwnd,SB_SIMPLE,TRUE,0);	/* set simple status window */
	SendMessage(hwnd,SB_SETTEXT,255|SBT_NOBORDERS,(LPARAM)tbuff);	/* display on status line */
	err_eflag = -1;
	return err_eflag;
}
/***************************************************************************/
static short checkerrors(HWND wptr, char * rtext)	/* checks record */

{
	short err, fcount, findex;
	CSTR field[FIELDLIM];
	FIELDPARAMS * fiptr;
	INDEX * FF;
	CHARRANGE cr;
	MFLIST * mfp;
		
	FF = getowner(wptr);
	mfp = getdata(wptr);
	fcount = str_xparse(rtext, field);
	for (fiptr = FF->head.indexpars.field, findex = 0; findex < fcount; findex++, fiptr++)	{
		if (findex == fcount-1)		/* if last field */
			fiptr = &FF->head.indexpars.field[PAGEINDEX];	/* point to page field pars */
		if (fiptr->minlength && str_textlen(field[findex].str) < fiptr->minlength)	{	/* too few chars */
			err = entryerr(mfp->hwst,ERR_TOOFEWCHARFIELD);
			break;
		}
		if (fiptr->maxlength && str_textlen(field[findex].str) > fiptr->maxlength)	{	/* too many chars */
			err = entryerr(mfp->hwst, ERR_TOOMANYCHARFIELD);
			break;
		}
#if 0
		if (*fiptr->matchtext && !re_exec(field[findex].str, &mfp->regex[findex == fcount-1 ? PAGEINDEX : findex][0])
			&& (g_prefs.gen.templatealarm == AL_REQUIRED || g_prefs.gen.templatealarm == AL_WARN && !mfp->alarms[A_TEMPLATE]++))	{	/* if bad pattern match */
			err = entryerr(mfp->hwst, ERR_BADPATTERNFIELD);
			break;
		}
#else
		if (*fiptr->matchtext && !regex_find(mfp->regex[findex == fcount-1 ? PAGEINDEX : findex],field[findex].str,0,NULL)
			&& (g_prefs.gen.templatealarm == AL_REQUIRED || g_prefs.gen.templatealarm == AL_WARN && !mfp->alarms[A_TEMPLATE]++))	{	/* if bad pattern match */
			err = entryerr(mfp->hwst, ERR_BADPATTERNFIELD);
			break;
		}
#endif
		if (err = checkfield(field[findex].str,mfp->alarms))	{	/* flag error */
			if (err == KEEPCS || err == ESCS)
				entryerr(mfp->hwst, ERR_BADCODEFIELD,err == KEEPCS ? '~' : '\\');
			else
				entryerr(mfp->hwst, ERR_MISMATCHFIELD,m_string[err-BRACES]);
			break;
		}
	}
	if (!err)	{	/* if individual fields OK */
		if (str_xlen(rtext) > FF->head.indexpars.minfields)	{	/* check main head & page field if not empty rec */
			if (field[fcount-1].str-rtext == FF->head.indexpars.minfields-1) {	/* if empty as far as page */
				if (strcmp(field[fcount-1].str,str_xlast(mfp->rstring)))	{	/* will be error if page field altered */
					err = entryerr(mfp->hwst, ERR_EMPTYMAINFIELD);
					findex = 0;
				}
			}
			if (!err)	{		/* check page field */
#if 1
				findex = fcount-1;
				if (str_xfindcross(FF,rtext,FALSE))	{
					char * estring;
					if (estring = search_testverify(FF,rtext))	{	// if bad crossref
						if (g_prefs.gen.crossalarm == AL_REQUIRED || g_prefs.gen.crossalarm == AL_WARN && !mfp->alarms[A_CROSS]++)	// if an error						
							err = entryerr(mfp->hwst, ERR_MISSINGCROSSREF,toNative(estring));		// missing target
					}
				}
				else if ((!*field[fcount-1].str || ref_isinrange(FF,field[fcount-1].str, g_nullstr, g_nullstr,&err))
					&& (g_prefs.gen.pagealarm == AL_REQUIRED 	/* check missing page only on key */
					|| g_prefs.gen.pagealarm == AL_WARN && !mfp->alarms[A_PAGE]++))	{	/* if missing page needs flagging */
					err = entryerr(mfp->hwst, *field[fcount-1].str ? err : ERR_EMPTYPAGEFIELD);		/* bad/missing refs */
				}
				else			/* need to clear errors in case err flag set and we're actually ignoring */
					err = 0;
#else
				if (!str_xfindcross(FF,rtext,FALSE) && (!*field[fcount-1].str || ref_isinrange(FF,field[fcount-1].str, g_nullstr, g_nullstr,&err))
					&& (g_prefs.gen.pagealarm == AL_REQUIRED 	/* check missing page only on key */
					|| g_prefs.gen.pagealarm == AL_WARN && !mfp->alarms[A_PAGE]++))	{	/* if missing page needs flagging */
					err = entryerr(mfp->hwst, *field[fcount-1].str ? err : ERR_EMPTYPAGEFIELD);		/* bad/missing refs */
					findex = fcount-1;
				}
				else			/* need to clear errors in case err flag set and we're actually ignoring */
					err = 0;
#endif
			}
		}
		if (!err)	{	/* if ok */
			memset(&mfp->alarms,0,sizeof(mfp->alarms));
			return (FALSE);
		}
	}
	findfieldlimits(wptr,findex,&cr);
	SendMessage(mfp->hwed,EM_EXSETSEL,0,(LPARAM)&cr);
	return (TRUE);
}
/******************************************************************************/
static int gettotlen(HWND hwnd)	/* gets length of record text */

{
	char tstring[MAXREC*2];
	
//	copyfontmap(MX(hwnd,fm),WX(hwnd,owner)->head.fm);
	return mod_gettestring(MX(hwnd,hwed), tstring,MX(hwnd,fm), FALSE);
}
/****************************************************************************/
static void findfieldlimits(HWND hwnd, int index, CHARRANGE * cr)	/* finds field limits */

{
	int limit, lcount;
	MFLIST * mfp;

	mfp = getdata(hwnd);
	cr->cpMin = 0;
	cr->cpMax = -1;
	limit = SendMessage(mfp->hwed,EM_GETLINECOUNT,0,0);
	for (lcount = 0; lcount < limit; lcount++)	{	/* for all lines in map */
		if (mfp->map[lcount] == index)		/* if first line of our field */
			cr->cpMin = SendMessage(mfp->hwed,EM_LINEINDEX,lcount,0);
		if (mfp->map[lcount] == index+1)	/* if first line of next field */
			cr->cpMax = SendMessage(mfp->hwed,EM_LINEINDEX,lcount,0);
	}
}

/****************************************************************************/
static int findfield(HWND hwnd, int charpos)	/* finds field in which char lies */

{
	int line;
	MFLIST * mfp;
	CHARRANGE cr;

	mfp = getdata(hwnd);

	if (charpos <0)		{	/* if want to find for selection point */
		SendMessage(mfp->hwed,EM_EXGETSEL,(WPARAM)NULL,(LPARAM)&cr);	/* get range */
		if (cr.cpMax > cr.cpMin && mod_getcharatpos(mfp->hwed,cr.cpMax-1)== RETURN)	/* if end of range is newline */
			cr.cpMax--;		/* make sure we register correct field */
		charpos = charpos == -1 ? cr.cpMin : cr.cpMax;
	}
	line = SendMessage(mfp->hwed,EM_LINEFROMCHAR,charpos,0);
	while (line && mfp->map[line] == (TCHAR)0xFF)	/* while not on start of field */
		line--;
	return (mfp->map[line]);
}
/****************************************************************************/
static short checkfield(unsigned char * source, short *alarms)		/* checks record field */

{
	short bcount, brcount, parencnt, sqbrcnt, qcnt, dqcnt, parenbad, sqbrbad, qbad;

	bcount = brcount = parencnt = sqbrcnt = qcnt = dqcnt = parenbad = sqbrbad = qbad = 0;

	while (*source)     {       	/* for all chars in string */
		unichar uc = u8_nextU((char **)&source);
		switch (uc)      {      /* check chars */
			case CODECHR:
			case FONTCHR:
				if (!*source++)			// skip code; if end of line
					return (CCODES);	/* error return (should never happen) */
				continue;
			case KEEPCHR:       	/* next is char literal */
				if (!*source)    /* if no following char */
					return (KEEPCS);       /* error return */
				source = u8_forward1(source);	// skip protected char
				continue;   	/* round for next */
			case ESCCHR:       	/* next is escape seq */
				if (!*source)    /* if at end of line */
					return (ESCS);     /* return error */
				source = u8_forward1(source);	// skip protected char
				continue;
			case OBRACE:        /* opening brace */
				if (bcount++)
					goto end;
				continue;
			case CBRACE:      /* closing brace */
				if (--bcount)
					goto end;
				continue;
			case OBRACKET:       /* opening < */
				if (brcount++)
					goto end;
				continue;
			case CBRACKET:      /* closing > */
				if (--brcount)
					goto end;
				continue;
			case '(':       /* opening paren */
				parencnt++;
				continue;
			case ')':       /* closing paren */
				if (--parencnt < 0)     /* if closing ever precedes opening */
					parenbad++;
				continue;
			case '[':       /* opening sqbr */
				sqbrcnt++;
				continue;
			case ']':       /* closing sqbr */
				if (--sqbrcnt < 0)      /* if closing ever precedes opening */
					sqbrbad++;
				continue;
			case OQUOTE:       /* opening quote */
				qcnt++;
				continue;
			case CQUOTE:       /* closing quote */
				if (--qcnt < 0)      /* if closing ever precedes opening */
					qbad++;
				continue;
			case '"':       /* dquote */
				dqcnt++;
				continue;
		}
	}
end:
	if (bcount)
		return (BRACES);
	if (brcount)
		return (BRACKETS);
	if ((parencnt || parenbad) && !alarms[A_PAREN]++)   /* if mismatched parens */
		return (PAREN);
	if ((sqbrcnt || sqbrbad) && !alarms[A_SQBR]++)   /* if mismatched sqbr */
		return (SQBR);
	if ((qcnt || qbad) && !alarms[A_QUOTE]++)   /* if mismatched curly quotes */
		return (QUOTE);
 	if (dqcnt&1 && !alarms[A_DQUOTE]++)   /* if mismatched simple quotes */
		return (DQUOTE);
	return (FALSE);
}
/*******************************************************************************/
static void mdofont(HWND hwnd,HWND cb, int code)	/* sets font */

{
	TCHAR fname[FSSTRING];

	if (cb)	{
		if (code == CBN_SELENDCANCEL || code == CBN_SELENDOK)	{		/* if selection changed */
			if (code == CBN_SELENDOK)	{
				GetWindowText(cb,fname,FSSTRING);
				setinfont(hwnd, fromNative(fname));
			}
			SetFocus(hwnd);	// restore focuse to main window
		}
	}
	else if (code == 1)	{	/* from accelerator */
		SendMessage(g_hwfcb,CB_SHOWDROPDOWN,TRUE,0);
		SetFocus(g_hwfcb);
	}
}
/**********************************************************************************/
static void setinfont(HWND hwnd, char * fname)	/* sets selection in named font */

{
	TCHAR base[MAXREC];
	CHARFORMAT2 cf;
	int oldmask, index;
	CHARRANGE cr, ct;
	HWND ew = MX(hwnd,hwed);

	if ((index = type_findindex(fname)) >= 0)	{	/* if have font */
		findtemprange(ew,base,&cr,&ct,&cf,&oldmask);
		cf.dwMask = CFM_FACE|CFM_CHARSET;
		nstrcpy(cf.szFaceName,toNative(fname));
		cf.bCharSet = t_fontlist.lf[index].elfLogFont.lfCharSet;	/* specify char set */
		SendMessage(ew,EM_SETCHARFORMAT,SCF_SELECTION,(LPARAM)&cf);	/* set font */
		undotemprange(ew,&cr,&ct,oldmask);	/* restore old selection */
		if (cr.cpMax != cr.cpMin)	// if style not applied to empty range
			SendMessage(GetParent(ew),WM_COMMAND,MAKELONG(IDM_EDIT,EN_CHANGE),(LPARAM)ew);	/* flag change */
		else
			dobuttons(GetParent(ew), BUTTONS_SELECT);	// just set buttons
	}
}
/**********************************************************************************/
short mod_dostyle(HWND ew, CHARFORMAT2 *df, int style)	/* sets style on selected text */

{
	CHARFORMAT2 cf;
	int step, smallc, length, count,oldmask;
	TCHAR string[MAXREC], *base = string+1;
	CHARRANGE cr, ct;
	
	length = findtemprange(ew,base,&cr,&ct,&cf,&oldmask);
	*string = ct.cpMin ? mod_getcharatpos(ew,ct.cpMin-1) : '\0';	/* find character before selection (if any) */
	step = df->yHeight/5;
	smallc =4*step;

	switch (style)	{
		case IDM_STYLE_REGULAR:
			cf.dwMask = CFM_ITALIC|CFM_BOLD|CFM_UNDERLINE|CFM_SIZE|CFM_SUPERSCRIPT|CFM_SUBSCRIPT|CFM_SMALLCAPS;
			cf.dwEffects = 0;
			cf.yHeight = df->yHeight;
//			cf.yOffset = 0;
			break;
		case IDM_STYLE_BOLD:
			cf.dwEffects = (cf.dwMask&CFM_BOLD && cf.dwEffects&CFE_BOLD) ? 0 : CFE_BOLD;
			cf.dwMask = CFM_BOLD;
			break;
		case IDM_STYLE_ITALIC:
			cf.dwEffects = (cf.dwMask&CFM_ITALIC && cf.dwEffects&CFE_ITALIC) ? 0 : CFE_ITALIC;
			cf.dwMask = CFM_ITALIC;
			break;
		case IDM_STYLE_UNDERLINE:
			cf.dwEffects = (cf.dwMask&CFM_UNDERLINE && cf.dwEffects&CFE_UNDERLINE) ? 0 : CFE_UNDERLINE;
			cf.dwMask = CFM_UNDERLINE;
			break;
		case IDM_STYLE_SMALLCAPS:
			if (cf.dwMask&CFM_SMALLCAPS && cf.dwEffects&CFE_SMALLCAPS)	{	/* if continuous */
				cf.dwEffects = 0;
//				if (!cf.yOffset)	/* if not a super/sub */
					cf.yHeight = df->yHeight;
			}
			else {
				cf.dwEffects = CFE_SMALLCAPS;
				cf.yHeight = smallc;
			}
			cf.dwMask = CFM_SMALLCAPS|CFM_SIZE;
			break;
#if 0
		case IDM_STYLE_SUPER:
			if (cf.dwMask&CFM_OFFSET && cf.yOffset == step)	{	/* if continuous on */
				cf.yOffset = 0;		/* toggle off */
				if (!(cf.dwEffects&CFE_SMALLCAPS))	/* if not small caps */
					cf.yHeight = df->yHeight;
			}
			else {
				cf.yOffset = step;
				cf.yHeight = smallc;	/* turn on */
			}
			cf.dwMask = CFM_OFFSET|CFM_SIZE;
			break;
		case IDM_STYLE_SUB:
			if (cf.dwMask&CFM_OFFSET && cf.yOffset == -step)	{	/* if continuous on */
				cf.yOffset = 0;
				if (!(cf.dwEffects&CFE_SMALLCAPS))	/* if not small caps */
					cf.yHeight = df->yHeight;	/* toggle off */
			}
			else {
				cf.yOffset = -step;
				cf.yHeight = smallc;	/* turn on */
			}
			cf.dwMask = CFM_OFFSET|CFM_SIZE;
			break;
#else
		case IDM_STYLE_SUPER:
			cf.dwEffects = (cf.dwMask&CFM_SUPERSCRIPT && cf.dwEffects&CFE_SUPERSCRIPT) ? 0 : CFE_SUPERSCRIPT;
			cf.dwMask = CFM_SUPERSCRIPT;
			break;
		case IDM_STYLE_SUB:
			cf.dwEffects = (cf.dwMask&CFM_SUBSCRIPT && cf.dwEffects&CFE_SUBSCRIPT) ? 0 : CFE_SUBSCRIPT;
			cf.dwMask = CFM_SUBSCRIPT;
			break;
#endif
		case IDM_STYLE_INITIALCAPS:
			for (count = 0; count < length; count++)	{
				if (!count && !ct.cpMin || base[count-1] == SPACE || base[count-1] == RETURN)
					base[count] = (TCHAR)CharUpper((TCHAR *)base[count]);
				else
					base[count] = (TCHAR)CharLower((TCHAR *)base[count]);
			}
			break;
		case IDM_STYLE_UPPERCASE:
			CharUpper(base);
			break;
		case IDM_STYLE_LOWERCASE:
			CharLower(base);
			break;
		default:
			return (0);
	}
	if (style >= IDM_STYLE_INITIALCAPS)	{		/* if changed case */
		TCHAR ts[2];
	
		for (ts[1] = '\0', count = 0; count < length; count++)	{	/* for each character in string */
			if (u_isalpha(base[count]))	{		/* if a letter */
				SendMessage(ew,EM_SETSEL,count+ct.cpMin,count+ct.cpMin+1);	/* select single char [do this way so as to preserve attributes */
				ts[0] = base[count];
				SendMessage(ew,EM_REPLACESEL,TRUE,(LPARAM)ts);	/* replace text */
			}
		}
		ct.cpMin++;			/* just to force undotemprange to restore original selection */
	}
	else
		SendMessage(ew,EM_SETCHARFORMAT,SCF_SELECTION,(LPARAM)&cf);	/* set style */
	undotemprange(ew,&cr,&ct,oldmask);	/* restore old selection */
	if (cr.cpMax != cr.cpMin)	// if style not applied to empty range
		SendMessage(GetParent(ew),WM_COMMAND,MAKELONG(IDM_EDIT,EN_CHANGE),(LPARAM)ew);	/* flag change */
	else
		dobuttons(GetParent(ew), BUTTONS_SELECT);	// just set buttons
	return (0);
}
/**********************************************************************************/
static int findtemprange(HWND ew, TCHAR * base, CHARRANGE *cr, CHARRANGE *ct, CHARFORMAT2 *cf, int * oldmask)	/* gets selection range stuff */

{
	int olength, length;

	*oldmask = SendMessage(ew,EM_SETEVENTMASK,0,ENM_NONE);	/* turn off selchange/update events */		
	olength = SendMessage(ew,EM_GETSELTEXT,0,(LPARAM)base);	/* get selection in case need to modify */
	if (olength > 1 && base[olength-1] == RETURN)	/* if trailing newline */
		olength -= 1;
	for (length = olength; length && base[length-1] == SPACE; length--)	/* while trailing spaces */
		;
	SendMessage(ew,EM_EXGETSEL,0,(LPARAM)cr);	/* get selection range */
	*ct = *cr;
	if (olength && !length)		/* if field consisted entirely of spaces */
		length = olength;		/* use original length */
	if (!(GetKeyState(VK_LBUTTON) & 0x80))	{	// if button not down, allow adjust selection
		if (length != ct->cpMax-ct->cpMin && GetKeyState(VK_LEFT) >= 0) {	/* if need to adjust selection */
				/* key state check is for problem with extending selection with left arrow key: changing sel resets dir flag */
			ct->cpMax = ct->cpMin+length;	/* adjust range */
			SendMessage(ew,EM_EXSETSEL,0,(LPARAM)ct);	/* set temp selection */
		}
	}
	cf->cbSize = sizeof(CHARFORMAT2);
	SendMessage(ew,EM_GETCHARFORMAT,SCF_SELECTION,(LPARAM)cf);	/* recover temp selection info */
	*(base+length) = '\0';	/* terminate recovered text at actual length of temp selection */
	return (length);
}
/**********************************************************************************/
static void undotemprange(HWND ew, CHARRANGE *cr, CHARRANGE *ct, int oldmask)	/* restores old range if necessary */

{
	if (cr->cpMin != ct->cpMin || cr->cpMax != ct->cpMax)	/* if ranges differ */
		SendMessage(ew,EM_EXSETSEL,0,(LPARAM)cr);	/* restore old selection */
	SendMessage(ew,EM_SETEVENTMASK,0,oldmask);	/* restore selchange/update events */
}
/**********************************************************************************/
static void drawstatstring(HWND hwnd)		/* builds & draws status string */

{
	RECORD * recptr;
	INDEX * FF;
	HWND sb;
	TCHAR tstring[STSTRING];
	struct tm * date;
	MFLIST * mfp;

	FF = WX(hwnd,owner);
	mfp = getdata(hwnd);
	sb = mfp->hwst;

	displaycount(hwnd);
	SendMessage(sb,SB_SETTEXT,4,(LPARAM)(mfp->delstate ? TEXT("Deleted") : TEXT("")));	/* status */
	SendMessage(g_hwtoolbar,TB_CHECKBUTTON,IDM_EDIT_DELETE,mfp->delstate);	/* set flag */
	u_sprintf(tstring,mfp->tagvalue ? "Labeled %d" : g_nullstr, mfp->tagvalue);
	SendMessage(sb,SB_SETTEXT,5,(LPARAM)tstring);	/* status */
	SendMessage(g_hwtoolbar,TB_CHECKBUTTON,IDM_EDIT_TAG,mfp->tagvalue > 0);	/* set flag */
	if (recptr = rec_getrec(FF, mfp->recnum))	{
		char sstring[STSTRING];
		time_t tt = recptr->time;
		if (date = localtime(&tt))
			strftime(sstring, 100, "%b %d %Y %H:%M", date);
		else
			strcpy(sstring,"Invalid Date");
		SendMessage(sb,SB_SETTEXT,2,(LPARAM)toNative(sstring));	/* date/time */
		strncpy(sstring,recptr->user,4);
		*(sstring+4) = '\0';		/* terminate user ID */
		SendMessage(sb,SB_SETTEXT,3,(LPARAM)toNative(sstring));	/* user ID */
		SendMessage(sb,SB_SETTEXT,6,(LPARAM)(recptr->ismark ? TEXT("Marked") : TEXT("")));	/* status */
	}
	else {		/* new record */
		SendMessage(sb,SB_SETTEXT,2,(LPARAM)TEXT(""));	
		SendMessage(sb,SB_SETTEXT,3,(LPARAM)TEXT(""));
		SendMessage(sb,SB_SETTEXT,6,(LPARAM)TEXT(""));
	}
}
/**********************************************************************************/
static void displaycount(HWND hwnd)		/* displays char count info */

{
	HWND sb;
	int totlen;
	char tstring[STSTRING];

	sb = MX(hwnd,hwst);
	totlen = MX(hwnd,totlen);
	_itoa(totlen,tstring,10);
//	SendMessage(sb,SB_SIMPLE,FALSE,0);	/* kills simple status window */
	SendMessage(sb,SB_SETTEXT,0,(LPARAM)toNative(tstring));	/* length of text */
	_itoa(WX(hwnd,owner)->head.indexpars.recsize-1-totlen,tstring,10);
	SendMessage(sb,SB_SETTEXT,1,(LPARAM)toNative(tstring));	/* free count */
}
/**********************************************************************************/
static int countbreaks(HWND ew, CHARRANGE * cr)		/* counts field breaks in selection */

{
	int count = 0, findpos;
	FINDTEXT ft;

	SendMessage(ew,EM_EXGETSEL,0,(LPARAM)&ft.chrg);	/* get selection info */
	if (cr)
		*cr = ft.chrg;
	if (ft.chrg.cpMin != ft.chrg.cpMax)	{	/* if a range */
		ft.lpstrText = NEWLINESTRING;
		for (; (findpos = SendMessage(ew,EM_FINDTEXT,FR_DOWN,(LPARAM)&ft)) != -1; count++)	/* while we have hits */
			ft.chrg.cpMin = findpos+1;			/* pass over target */
		if (ft.chrg.cpMin == ft.chrg.cpMax)		/* if selection ends on newline */
			return (--count);					/* discount it */
	}
	return (count);
}
/**********************************************************************************/
static void enclosetext(HWND hwnd, char c1, char c2)	/* encloses selection in chars */

{
	if (MX(hwnd,totlen) < WX(hwnd,owner)->head.indexpars.recsize-2)	{	/* if have room */
#if 0
		TCHAR tstring[MAXREC];
		HWND ew = MX(hwnd,hwed);
		RECONTEXT rec;

		saverecontext(ew,&rec);
		SendMessage(ew,EM_GETSELTEXT,0,(LPARAM)(tstring+1));	// set sel at end
		tstring[0] = c1;
		tstring[rec.cr.cpMax-rec.cr.cpMin+1] = c2;
		tstring[rec.cr.cpMax-rec.cr.cpMin+2] = '\0';
		SendMessage(ew,EM_REPLACESEL,TRUE,(LPARAM)tstring);
		rec.cr.cpMax += 2;
		restorerecontext(ew,&rec);
#else
		MFLIST * mfp = getdata(hwnd);
		char tstring[MAXREC];
		char * tptr; /* insertion point */

//		mfp->itd->lpVtbl->Undo(mfp->itd,tomSuspend,NULL);
#if 0
		if (c1 == '{') {
			CHARRANGE cr;
			SendMessage(mfp->hwed, EM_EXGETSEL, 0, (LPARAM)&cr);
			if (mod_getcharatpos(mfp->hwed, cr.cpMax - 1) == SPACE /* && mod_getcharatpos(mfp->hwed, LEFTCHAR) != SPACE */) {
				cr.cpMax -= 1;
				SendMessage(mfp->hwed, EM_EXSETSEL, 0, (LPARAM)&cr);	// exclude terminal space
			}
		}
#endif
		tstring[0] = c1;
		mod_gettestring(mfp->hwed, tstring+1,mfp->fm, MREC_SELECT|MREC_NOTRIM);	/* recover text */
		tptr = tstring+strlen(tstring); /* insertion point */
		str_xshift(tptr,1);				/* make 1 char gap */
		*tptr = c2;
//		mfp->itd->lpVtbl->Undo(mfp->itd,tomResume,NULL);
		settestring(hwnd, tstring, mfp->fm, TRUE);	/* adds text */
#endif
	}
}
/**********************************************************************************/
static void flipfield(HWND hwnd, int mode)	/* flips field */

{
	MFLIST *mfp = getdata(hwnd);
	int pageindex = mfp->fcount-1;
	int topfield;
	CHARRANGE cr1,cr2;
	char tstring[MAXREC];
	RECONTEXT rec;
	
	topfield = mfp->startfield;
	if (topfield < pageindex)	{	/* if not in page field */
		INDEX *FF = getowner(hwnd);
		short reverse = GetKeyState(VK_SHIFT) < 0;	// true if want to switch normal mode
		char * list = topfield < pageindex-1 ? FF->head.flipwords : FF->head.refpars.crosstart;

		saverecontext(mfp->hwed,&rec);
		findfieldlimits(hwnd,topfield,&cr1);	/* find bounds of upper field */
		findfieldlimits(hwnd,topfield+1,&cr2);	/* find bounds of lower field */
		SendMessage(mfp->hwed,EM_SETSEL,cr1.cpMin,cr2.cpMax-1);	// both fieds
		copyfontmap(mfp->fm,FF->head.fm);
		mod_gettestring(mfp->hwed, tstring,mfp->fm, MREC_SELECT);	/* recover text */
		str_flip(tstring,list,g_prefs.gen.smartflip && !reverse || !g_prefs.gen.smartflip && reverse,
			mode == IDB_MOD_HALFFLIP || mode == IDB_MOD_HALFFLIPX, topfield == pageindex-1);
		settestring(hwnd, tstring, FF->head.fm, TRUE);	// replace selected fields
		rec.cr.cpMin = rec.cr.cpMax = cr1.cpMin;	// select original start position
		restorerecontext(mfp->hwed,&rec);
	}
}
/**********************************************************************************/
static void swapparens(HWND hwnd)	// swaps contents of parens

{
	MFLIST *mfp = getdata(hwnd);
	int pageindex = mfp->fcount-1;
	int curfield;
	CHARRANGE cr1;
	char tstring[MAXREC];
	RECONTEXT rec;
	
	curfield = mfp->startfield;
	if (curfield < pageindex)	{	/* if not in page field */
		INDEX *FF = getowner(hwnd);

		saverecontext(mfp->hwed,&rec);
		findfieldlimits(hwnd,curfield,&cr1);	/* find bounds of field */
		SendMessage(mfp->hwed,EM_SETSEL,cr1.cpMin,cr1.cpMax-1);	// both fieds
		copyfontmap(mfp->fm,FF->head.fm);
		mod_gettestring(mfp->hwed, tstring,mfp->fm, MREC_SELECT);	/* recover text */
		if (str_swapparen(tstring,FF->head.flipwords,TRUE))	{
			settestring(hwnd, tstring, FF->head.fm, TRUE);	// replace selected text
			rec.cr.cpMin = rec.cr.cpMax = cr1.cpMin;	// select original start position
		}
		restorerecontext(mfp->hwed,&rec);
	}
}
/**********************************************************************************/
static void incdec(HWND hwnd, int mode)	// increments/decrements page number

{
	MFLIST *mfp = getdata(hwnd);
	INDEX *FF = getowner(hwnd);
	HWND ew = mfp->hwed;
	int pageindex = mfp->fcount-1;
	CHARRANGE cr;
	char tstring[MAXREC];
	RECONTEXT rec;
	BOOL endflag;
	char *base;
	GETTEXTLENGTHEX exl;

	saverecontext(ew,&rec);
	exl.flags = GTL_DEFAULT;	// need EM_GETTEXTLENGTHEX to get accurate length
	exl.codepage = 1200;
	endflag = rec.cr.cpMax == SendMessage(ew,EM_GETTEXTLENGTHEX,(WPARAM)&exl,0);
	findfieldlimits(hwnd,pageindex,&cr);	// find bounds of page field
	SendMessage(ew,EM_SETSEL,cr.cpMin,cr.cpMax);	// set selection
	copyfontmap(mfp->fm,FF->head.fm);
	mod_gettestring(ew, tstring,mfp->fm, MREC_SELECT);	/* recover text */
	base = ref_incdec(FF, tstring, mode == IDM_MOD_INCREMENT);
	if (base)	{
		FINDTEXTEX ft;

		ft.chrg.cpMax = cr.cpMin;
		ft.chrg.cpMin = SendMessage(ew,EM_GETTEXTLENGTHEX,(WPARAM)&exl,0);
		ft.lpstrText = toNative(base);
		if (SendMessage(ew,EM_FINDTEXTEXW,0,(LPARAM)&ft) >= 0)	{	// if found text
			SendMessage(ew,EM_EXSETSEL,0,(LPARAM)&ft.chrgText);	// set selection
			SendMessage(ew,EM_REPLACESEL,TRUE,(LPARAM)toNative(base+strlen(base)+1));	// replace it
			if (endflag)	// if original selection covered changed locator
				rec.cr.cpMax = rec.cr.cpMax = -1;

		}
	}
	else
		MessageBeep(MB_OK);
	restorerecontext(ew,&rec);
}
/**********************************************************************************/
static BOOL extendref(HWND hwnd)	// adds upper element of range as lower+1

{
	MFLIST *mfp = getdata(hwnd);
	INDEX *FF = getowner(hwnd);
	HWND ew = mfp->hwed;
	int pageindex = mfp->fcount-1;
	CHARRANGE cr;
	char tstring[MAXREC];
	RECONTEXT rec;
	BOOL ok = FALSE;
	char *base;
	GETTEXTLENGTHEX exl;

	saverecontext(ew,&rec);
	exl.flags = GTL_DEFAULT;	// need EM_GETTEXTLENGTHEX to get accurate length
	exl.codepage = 1200;
	if (rec.cr.cpMax == SendMessage(ew,EM_GETTEXTLENGTHEX,(WPARAM)&exl,0))	{	// if cursor is after last char
		findfieldlimits(hwnd,pageindex,&cr);	// find bounds of page field
		SendMessage(ew,EM_SETSEL,cr.cpMin,cr.cpMax);	// set selection
		copyfontmap(mfp->fm,FF->head.fm);
		mod_gettestring(ew, tstring,mfp->fm, MREC_SELECT);	/* recover text */
		base = ref_autorange(FF, tstring);
		if (base)	{
			cr.cpMin = cr.cpMax = rec.cr.cpMax;	// set selection to end of text
			SendMessage(ew,EM_EXSETSEL,0,(LPARAM)&cr);	// set selection
//			SendMessage(ew,EM_REPLACESEL,TRUE,(LPARAM)toNative(base));	// replace it
			*(base+strlen(base)+1) = EOCS;
			settestring(hwnd, base, FF->head.fm, TRUE);	/* insert text */
			rec.cr.cpMin = rec.cr.cpMax = -1;
			ok = TRUE;
		}
	}
	restorerecontext(ew,&rec);
	return ok;
}
/**********************************************************************************/
static void mswitchdel(HWND hwnd)	/* deletes/restores */

{
	MX(hwnd,delstate) ^= 1;
	MX(hwnd, statuschanged) = TRUE;
	SendMessage(MX(hwnd,hwed),EM_SETMODIFY,TRUE,0);
	dobuttons(hwnd,BUTTONS_STATE);
	drawstatstring(hwnd);	/* force update of status line */
}
/**********************************************************************************/
static void mswitchtag(HWND hwnd,int tagid)	/* tags/untags */

{
	MFLIST *mfp = getdata(hwnd);	
	CHARFORMAT2 cf;

	if (tagid < 0)	{	// special toggle
		if (mfp->tagvalue >1)	// if not unlabeled or label 1
			return;		// don't touch any other labels
		mfp->tagvalue ^= 1;
	}
	else 
		mfp->tagvalue = mfp->tagvalue == tagid ? 0 : tagid;	// toggle or set new
	cf.cbSize = sizeof(cf);
	cf.crTextColor = g_prefs.gen.flagcolor[mfp->tagvalue];
	cf.dwMask = CFM_COLOR;
	cf.dwEffects = 0;
	SendMessage(mfp->hwed,EM_SETCHARFORMAT,SCF_ALL,(LPARAM)&cf);
	mfp->statuschanged = TRUE;
	drawstatstring(hwnd);	/* force update of status line */
}
/**********************************************************************************/
TCHAR mod_getcharatpos(HWND ew, int charpos)	/* gets character at position */

{
	TEXTRANGE tr;
	TCHAR buff[2];

	if (charpos < 0)	{	/* if want char after end of selection */
		SendMessage(ew,EM_EXGETSEL,0,(LPARAM)&tr.chrg);
		if (charpos == RIGHTCHAR)	{
			tr.chrg.cpMin = tr.chrg.cpMax;
			tr.chrg.cpMax += 1;
		}
		else	{
			tr.chrg.cpMax = tr.chrg.cpMin;
			tr.chrg.cpMin -= 1;
		}
	}
	else	{		/* just get specified char */
		tr.chrg.cpMin = charpos;
		tr.chrg.cpMax = charpos+1;
	}
	tr.lpstrText = buff;
	*buff = '\0';		/* zero it */
	SendMessage(ew,EM_GETTEXTRANGE,0,(LPARAM)&tr);
	return (*buff);
}
/******************************************************************************/
static BOOL getabbrev(HWND hwnd)      /* inserts expanded string for abbreviation */

{
	MFLIST * mfp = getdata(hwnd);
	INDEX * FF = getowner(hwnd);
	char *np;
	TCHAR *cptr;
	int count;
	int ablen = 0;
	HWND ew;
	TCHAR name[ABBNAMELEN+1], tstring[STSTRING];
	char expand[ABBREVLEN+2];
	TEXTRANGE tr;
	int holdmax;

	ew = mfp->hwed;
	SendMessage(ew,EM_EXGETSEL,0,(LPARAM)&tr.chrg);	/* get selection info */
	holdmax = tr.chrg.cpMax;		/* save end of range */
	tr.chrg.cpMax = tr.chrg.cpMin;	/* set max to ensure string not too long */
	tr.chrg.cpMin = tr.chrg.cpMin < 100 ? 0 : tr.chrg.cpMin - 100; 		/* if less than 100 from start */
	tr.lpstrText = tstring;
	SendMessage(ew,EM_GETTEXTRANGE,0,(LPARAM)&tr);
	cptr = tstring+(tr.chrg.cpMax-tr.chrg.cpMin);
	while ((u_isalnum(*--cptr) || !u_strchr(a_prefix, *cptr)) && *cptr != RETURN && cptr >= tstring)    /* while after beginning of name */
		ablen++;				/* count chars in it */
	if (ablen <= ABBNAMELEN)	{	 /* if poss abbrev not too long */
		nstrncpy(name, ++cptr, ablen);		/* copy it to buffer */
		*(name+ablen) = '\0';		/* terminate it */
		if (np = abbrev_checkentry(fromNative(name)))   {	/* if a legit abbrev */
			count = strlen(np);				/* length of expanded string */
			if (MX(hwnd,totlen)+count-ablen < FF->head.indexpars.recsize)	{	/* if wouldn't be too long */
				CHARFORMAT2 cf;
				cf.cbSize = sizeof(cf);
				SendMessage(ew,EM_GETCHARFORMAT,TRUE,(LPARAM)&cf);	// save current format info
				strcpy(expand,np);
				expand[count+1] = EOCS;	/* make xstring */
				tr.chrg.cpMin = tr.chrg.cpMax-ablen;	/* set up bottom of relacement */
				tr.chrg.cpMax = holdmax;				/* and top */
				SendMessage(ew,EM_HIDESELECTION,TRUE,0);	/* hide selection while fiddling */
				SendMessage(ew,EM_EXSETSEL,0,(LPARAM)&tr.chrg);	/* set selection range for replacement */
				settestring(hwnd, expand, FF->head.fm, TRUE);	/* inserts text */
				SendMessage(ew,EM_SETCHARFORMAT,SCF_SELECTION,(LPARAM)&cf);	// restore format
				return (TRUE);
			}
			else	/* no room */
				entryerr(mfp->hwst,ERR_RECOVERFLOW);
		}
	}
	return (FALSE);
}
/******************************************************************************/
static BOOL mhotkey(HWND hwnd, int keyid)      /* inserts expanded string hotkey */

{
	CSTR fstrings[MAXKEYS];
	HWND ew;
	char tstring[STSTRING];
	CHARRANGE cr;

	ew = MX(hwnd,hwed);
	SendMessage(ew,EM_EXGETSEL,0,(LPARAM)&cr);	/* get selection info */
	str_xparse(g_prefs.keystrings, fstrings);
	if (keyid < MAXKEYS && fstrings[keyid].ln)	{	/* if a good key && has string */
		if (MX(hwnd,totlen)+fstrings[keyid].ln-(cr.cpMax-cr.cpMin) < WX(hwnd,owner)->head.indexpars.recsize)	{	/* if would be too long */
			CHARFORMAT2 cf;
			strcpy(tstring,fstrings[keyid].str);
			tstring[fstrings[keyid].ln+1] = EOCS;	/* make xstring */
			cf.cbSize = sizeof(CHARFORMAT2);
			SendMessage(ew,EM_GETCHARFORMAT,TRUE,(LPARAM)&cf);	/* recover temp selection info */
			settestring(hwnd, tstring, g_prefs.gen.fm, TRUE);	/* inserts text (any fonts but symbol will be default) */
			if (cr.cpMax==cr.cpMin)	// if inserting text at empty selection
				SendMessage(ew,EM_SETCHARFORMAT,SCF_SELECTION,(LPARAM)&cf);	// restore previous format
			return (TRUE);
		}
		else	/* no room */
			entryerr(MX(hwnd,hwst),ERR_RECOVERFLOW);
	}
	return (FALSE);
}
/******************************************************************************/
void saverecontext(HWND ew,RECONTEXT *recp)		//	saves RichEdit context

{
	ITextDocument * itp = MX(GetParent(ew),itd);

	recp->mask = SendMessage(ew,EM_SETEVENTMASK,0,ENM_NONE);	// get old mask
	SendMessage(ew,EM_EXGETSEL,0,(LPARAM)&recp->cr);	// save selection
#if 1
	SendMessage(ew,EM_HIDESELECTION,TRUE,0);	/* hide selection while fiddling */
#else
	itp->lpVtbl->Freeze(itp,&recp->freezecount);
#endif
}
/******************************************************************************/
void restorerecontext(HWND ew,RECONTEXT *recp)		//	restores RichEdit context

{
	ITextDocument * itp = MX(GetParent(ew),itd);

	SendMessage(ew,EM_EXSETSEL,0,(LPARAM)&recp->cr);	/* restore selection */
#if 1
	SendMessage(ew,EM_HIDESELECTION,FALSE,0);	/* unhide selection after fiddling */
#else
	itp->lpVtbl->Unfreeze(itp,&recp->freezecount);
#endif
	SendMessage(ew,EM_SETEVENTMASK,0,recp->mask);	/* restore selchange/update events */
}
/******************************************************************************/
static void copyfontmap(FONTMAP * to,FONTMAP * from)

{
	int findex;

	for (findex = 0; findex < FONTLIMIT; findex++)
		to[findex] = from[findex];
}
/******************************************************************************/
int mod_gettestring(HWND ew, char * string, FONTMAP * fmp, int flags)	/* recovers coded cindex string */

{
	ITextDocument * itp = flags&MREC_NOITD ? NULL : MX(GetParent(ew), itd);	// no freeze for dialog parent
	ATTRIBUTEDSTRING *as = astr_createforsize(2*MAXREC);
	char attributes = 0, fontid = 0;
	GETTEXTEX gt;
	CHARFORMAT2 cf, dcf;
	RECONTEXT rec;
	unsigned int index, scanoffset;
	long freezecount;
//	TCHAR windowClass[100];

//	GetClassName(GetParent(ew),windowClass,sizeof(windowClass));
//	if (!nstrcmp(windowClass, TEXT("mtext")))	// will only freeze for regular mod window, not hot keys
//		itp = MX(GetParent(ew), itd);
	if (itp)
		itp->lpVtbl->Freeze(itp,&freezecount);	// freeze display
	saverecontext(ew,&rec);
	memset(&cf,0,sizeof(cf));		// set up char format struct
	cf.cbSize = sizeof(cf);
	dcf.cbSize = sizeof(cf);
	memset(&gt,0,sizeof(gt));
	gt.cb = 2*MAXREC;
	gt.flags = flags&MREC_SELECT ? GT_SELECTION : GT_DEFAULT;
	gt.codepage = 1200;	// 1200 for unicode; CP_UTF8 for utf-8
	as->length = SendMessage(ew,EM_GETTEXTEX,(WPARAM)&gt,(LPARAM)as->string);
	astr_normalize(as);
	scanoffset = flags&MREC_SELECT ? rec.cr.cpMin : 0;

	SendMessage(ew,EM_GETCHARFORMAT,SCF_SELECTION,(LPARAM)&dcf);	// get format of current selection 
	for (index = 0; index < as->length; index++)	{
		char newfontid = 0;
		CHARRANGE ct;
		char newattributes, onstylecode, offstylecode, fontcode;

		fontcode = onstylecode = offstylecode = newattributes = 0;
		ct.cpMin = index+scanoffset;	// add offset of sel start in control
		ct.cpMax = index+scanoffset+1;
		SendMessage(ew,EM_EXSETSEL,0,(LPARAM)&ct);	// set 1 char selection
		SendMessage(ew,EM_GETCHARFORMAT,SCF_SELECTION,(LPARAM)&cf);

		if (cf.dwEffects&CFE_BOLD && cf.dwMask&CFM_BOLD)
			newattributes |= FX_BOLD;
		if (cf.dwEffects&CFE_ITALIC && cf.dwMask&CFM_ITALIC)
			newattributes |= FX_ITAL;
		if (cf.dwEffects&CFE_UNDERLINE && cf.dwMask&CFM_UNDERLINE)
			newattributes |= FX_ULINE;
		if (cf.dwEffects&CFE_SMALLCAPS && cf.dwMask&CFM_SMALLCAPS)
			newattributes |= FX_SMALL;
		if (cf.dwEffects&CFE_SUPERSCRIPT && cf.dwMask&CFM_SUPERSCRIPT)
			newattributes |= FX_SUPER;
		if (cf.dwEffects&CFE_SUBSCRIPT && cf.dwMask&CFM_SUBSCRIPT)
			newattributes |= FX_SUB;
//		if (cf.dwMask&CFM_FACE && fmp)
		// encode font only if it's one that's available on the system
		if (cf.dwMask&CFM_FACE && fmp && type_findindex(fromNative(cf.szFaceName)) >= 0)
			newfontid = type_findlocal(fmp,fromNative(cf.szFaceName),0);

		if ((newattributes&FX_BOLD) != (attributes&FX_BOLD)) {
			if (newattributes&FX_BOLD)
				onstylecode |= FX_BOLD;
			else
				offstylecode |= FX_BOLD;
		}
		if ((newattributes&FX_ITAL) != (attributes&FX_ITAL)) {
			if (newattributes&FX_ITAL)
				onstylecode |= FX_ITAL;
			else
				offstylecode |= FX_ITAL;
		}
		if ((newattributes&FX_ULINE) != (attributes&FX_ULINE)) {
			if (newattributes&FX_ULINE)
				onstylecode |= FX_ULINE;
			else
				offstylecode |= FX_ULINE;
		}
		if ((newattributes&FX_SMALL) != (attributes&FX_SMALL)) { 
			if (newattributes&FX_SMALL)
				onstylecode |= FX_SMALL;
			else
				offstylecode |= FX_SMALL;
		}
		if ((newattributes&FX_SUPER) != (attributes&FX_SUPER)) {
			if (newattributes&FX_SUPER)
				onstylecode |= FX_SUPER;
			else
				offstylecode |= FX_SUPER;
		}
		if ((newattributes&FX_SUB) != (attributes&FX_SUB)) {
			if (newattributes&FX_SUB)
				onstylecode |= FX_SUB;
			else
				offstylecode |= FX_SUB;
		}
		if (fontid != newfontid && fmp)	// if want font change and recovering font info
			fontcode = newfontid|FX_FONT|FX_AUTOFONT;
		if (offstylecode)	{	/* if off code, then off styles come before font */
			as->codesets[as->codecount].offset = index;
			as->codesets[as->codecount++].code = offstylecode|FX_OFF;
			if (fontcode)	{
				as->codesets[as->codecount].offset = index;
				as->codesets[as->codecount++].code = fontcode;
			}
			if (onstylecode)	{	/* if any on style to add */
				as->codesets[as->codecount].offset = index;
				as->codesets[as->codecount++].code = onstylecode;
			}
		}
		else {			/* otherwise fonts come first */
			if (fontcode)	{
				as->codesets[as->codecount].offset = index;
				as->codesets[as->codecount++].code = fontcode;
			}
			if (onstylecode)	{
				as->codesets[as->codecount].offset = index;
				as->codesets[as->codecount++].code = onstylecode;
			}
		}
		attributes = newattributes;
		fontid = newfontid;
		if (as->string[index] == '\r')	// if char at this position is newline
			as->string[index] = 0;		// end string
	}
	restorerecontext(ew,&rec);
	if (rec.cr.cpMax == rec.cr.cpMin)	// if zero width selection
		SendMessage(ew,EM_SETCHARFORMAT,SCF_SELECTION,(LPARAM)&dcf);// restore entry-point formatting
	if (itp)
		itp->lpVtbl->Unfreeze(itp,&freezecount);
	astr_toUTF8string(string,as);
	astr_free(as);
	return str_adjustcodes(string,(flags&MREC_NOTRIM ? 0 : CC_TRIM)|(g_prefs.gen.remspaces ? CC_ONESPACE : 0));	/* clean up codes */
}
/******************************************************************************/
HWND createtoolbar(HWND hWndParent)
{
	MFLIST * mfp = getdata(hWndParent);
	TBMETRICS tbm;
	const int bitmapSize = scaleForDpi(hWndParent,16);
	const DWORD buttonStyles = BTNS_AUTOSIZE;

	tbm.cbSize = sizeof(tbm);
	tbm.dwMask = TBMF_PAD| TBMF_BUTTONSPACING;

	// Create the toolbar.
	HWND hWndToolbar = CreateWindowEx(0, TOOLBARCLASSNAME, NULL,
		WS_CHILD | TBSTYLE_TOOLTIPS| CCS_TOP | CCS_ADJUSTABLE | TBSTYLE_ALTDRAG| BTNS_AUTOSIZE, 0, 0, 0, 0,
		hWndParent, NULL, g_hinst, NULL);

	if (hWndToolbar != NULL) {
		DWORD tbs = SendMessage(hWndToolbar, TB_GETBUTTONSIZE, 0, 0);
		// Create the image list.
		mfp->imagelist = ImageList_Create(bitmapSize, bitmapSize,   // Dimensions of individual bitmaps.
			ILC_COLOR32 , MB_MTOTAL, 5);
		mfp->imagelistdisabled = ImageList_Create(bitmapSize, bitmapSize,   // Dimensions of individual bitmaps.
			ILC_COLOR32, MB_MTOTAL, 5);

		for (int hindex = IDB_PROPAGATE; hindex <= IDB_REVERT; hindex++) {	// add button images
			HBITMAP bm = LoadImage(g_hinst, MAKEINTRESOURCE(hindex), IMAGE_BITMAP, bitmapSize, bitmapSize, 0);
			ImageList_Add(mfp->imagelist, bm, NULL);
			DeleteObject(bm);
		}
		for (int hindex = IDB_PROPAGATE_OFF; hindex <= IDB_REVERT_OFF; hindex++) {	// add button images
			HBITMAP bm = LoadImage(g_hinst, MAKEINTRESOURCE(hindex), IMAGE_BITMAP, bitmapSize, bitmapSize, 0);
			ImageList_Add(mfp->imagelistdisabled, bm, NULL);
			DeleteObject(bm);
		}
		for (int bindex = 0; bindex < TBBSIZE; bindex++) {	// scale separators
			if (tbb[bindex].fsStyle == BTNS_SEP)	// if separator
				tbb[bindex].iBitmap = scaleForDpi(hWndParent, bindex == 0 ? BASESEPARATOR : SEPARATOR);	// scale it
		}
		// Set the image list.
		SendMessage(hWndToolbar, TB_SETIMAGELIST, (WPARAM)0, (LPARAM)mfp->imagelist);
		SendMessage(hWndToolbar, TB_SETDISABLEDIMAGELIST, (WPARAM)0, (LPARAM)mfp->imagelistdisabled);

		// Add buttons.
		SendMessage(hWndToolbar, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0);
		SendMessage(hWndToolbar, TB_ADDBUTTONS, (WPARAM)TBBSIZE, (LPARAM)tbb);
#if 0
		SendMessage(hWndToolbar, TB_GETMETRICS, 0, (LPARAM)&tbm);
		tbm.cxPad = scaleForDpi(hWndParent, tbm.cxPad);
		tbm.cyPad = scaleForDpi(hWndParent, tbm.cyPad);
	//	tbm.cxButtonSpacing = scaleForDpi(hWndParent, tbm.cxButtonSpacing);
		SendMessage(hWndToolbar, TB_SETMETRICS, 0, (LPARAM)&tbm);
#endif
		// Resize the toolbar, and then show it.
		SendMessage(hWndToolbar, TB_SETBUTTONSIZE, 0, MAKELPARAM(scaleForDpi(hWndParent, LOWORD(tbs)), scaleForDpi(hWndParent, HIWORD(tbs))));
		SendMessage(hWndToolbar, TB_AUTOSIZE, 0, 0);
		ShowWindow(hWndToolbar, TRUE);
	}
	return hWndToolbar;
}
