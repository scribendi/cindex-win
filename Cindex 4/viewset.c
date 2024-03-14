#include "stdafx.h"
#include "commands.h"
#include "files.h"
#include "index.h"
#include "records.h"
#include "strings.h"
#include "collate.h"
#include "sort.h"
#include "search.h"
#include "viewset.h"
#include "modify.h"
#include "edit.h"
#include "formstuff.h"
#include "formattedexport.h"
#include "draftstuff.h"
#include "indexset.h"
#include "print.h"
#include "util.h"
#include "errors.h"
#include "findset.h"
#include "replaceset.h"
#include "searchset.h"
#include "group.h"
#include "formset.h"
#include "import.h"
#include "spellset.h"
#include "toolset.h"
#include "sortset.h"
#include "indexset.h"
#include "tagstuff.h"
#include "registry.h"
#include "macros.h"
#include "export.h"
#include "abbrev.h"

enum {
	KEY_TIMER = 1,		/* timer ids */
	SCROLL_TIMER,
	SAVE_TIMER,
	DRAG_TIMER
};

#define KEYDELAY 250
#define FASTDELAY 180
#define SCROLLDELAY 40		/* delay between timed scroll steps */

static struct selstruct v_statsel = {
	0,			/* first selected record in range */
	0,			/* last selected record in range */
	0,			/* direction in which selection made */
	-1,			/* start char of range */
	-1,			/* length of range */
	0,			/* record line on which range starts */
	-1,			/* start pixel pos on line */
	SHRT_MAX,	/* record line on which range ends */
	-1			/* end pixel pos on line */
};

#define DESTINSET 2		/* inset of destination rect */
#define STARGETLINE 3	/* line on which to put selection */
#define SBOTTOMGAP 2	// gap between selection an bottom line in V_BOTTOM display

#define PAGEMUL 100	// scaling const to improve scroll managing fractional lines displayed on page
#define SMALLVIEWSET 300

static void dofullform(HWND hwnd);	/* shows fully formatted */
static void dodraftform(HWND hwnd);	/* shows draft formatted */
static void dotempgroup(HWND hwnd);	/* displays search records */
static void donewrecords(HWND hwnd);	/* displays new records */

static void ldofont(HWND hwnd,HWND cb, int code);	/* sets font */
static void lsetalignment(HWND hwnd, int alignment);	// sets text alignment
static void ldosize(HWND hwnd,HWND cb,UINT notify);	/* sets font size */
static void lswitchform(HWND hwnd, int indented);	/* switches between formats */
static void lswitchsort(HWND hwnd, int alpha);	/* switches between alpha & page sort */
static void ldosortonoff(HWND hwnd);	/* sets sort on or off */
static void lcommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);	/* does menu tasks */
static void ldomenu(HWND hwnd, HMENU hMenu, UINT item, BOOL fSystemMenu);	/* sets menus */
static void ldocontextmenu(HWND hwnd, HWND hwndContext, UINT xpos, UINT ypos);	/* does context menus */
static void setbasetoolbar(HWND hwnd);		/* updates all toolbar buttons */
static void ldoviewbuttons(INDEX * FF);	/* sets menus */
static void doselbuttons(HWND hwnd);	/* does selection-dependent buttons */
static void dopastebutton(INDEX * FF);	/* does paste button */
static BOOL lcreate(HWND hwnd,LPCREATESTRUCT cs);		/* initializes view window */
static void lupdate(HWND hwnd);		/* updates display window */
static void lsize(HWND hwnd, UINT state, int cx, int cy);	/* deals with resize */
static void lselall(HWND hwnd);		/* selects all */
static void lsetfocus(HWND hwnd, HWND oldfocus);	/* gets focus shift message */
static void lkillfocus(HWND hwnd, HWND newfocus);	/* gets focus shift message */
static void ldomousedown(HWND wptr, BOOL dclick, int x, int y, UINT flags);	/* handles clicks */
static void ldomouseup(HWND wptr, int x, int y, UINT flags);	/* handles mouse up */
static void ldomousemove(HWND wptr, int px, int pyy, UINT flags);	/* handles movements */
static void CALLBACK dragscroll(HWND hwnd, UINT msg, UINT_PTR timer, DWORD time);	/* checks need for drag scroll & does it */
static BOOL ldocursor(HWND hwnd, HWND hwndCursor, UINT hitcode, UINT msg);	/* sets cursor */
static void ldochar(HWND hwnd, TCHAR ch, int cRepeat);		/* handles chars */
static void ldokey(HWND wptr, UINT vk, BOOL fDown, int cRepeat, UINT flags);		/* handles keystrokes */
static void lprint(HWND wptr);		/* forms & prints text pages */
static void formimages(HWND wptr, HDC dc, BOOL silent);	/* forms text pages; prints if necess */
static short fixbreak(HWND hwnd, RECN * rnum, short *continuedlines);		/* fixes break & continuation */
static void lsetpage(HWND wptr);		/* sets up after page setup */
static void resetdisplay(INDEX * FF);	/* resets display */
static void setpars(HWND wptr);		/*  sets up display attributes */
static void settextinfo(HWND wptr, HDC dc);		/* sets up page/screen text params */
static void setvdimensions(HWND wptr);	/*  sets up view rect dimensions, etc */
static void setddimensions(HWND wptr, BOOL screen);	/*  sets up dest rect dimensions, etc */
static void getpagerect(RECT * drect, INDEX * FFg);	/* sets destination rect for display */
static short setlines(HWND wptr, short fline, short maxcount, RECN rnum, short firstoffset);	/* sets up new line table from record rnum */
static void drawlines(HWND wptr, short first, short count);	/* draws count lines from pos first */
static void setscrollrange(HWND hwnd);	/* sets range of scroll bar */
static void checkvscroll(HWND wptr);	/* checks range of scroll bar */
static void setvscroll(HWND hwnd);	/* sets vertical scroll bar posn */
static void trackvscroll(HWND hwnd, HWND hctl, UINT code, int pos);	/* displays repetitively while mousdown in scrollbar */
static void ldowheel(HWND hwnd, int xPos, int yPos, int zDelta, UINT fwKeys);
static void lineup(HWND wptr, short lcount);	/* moves display up lines (scrolls up) */
static void linedown(HWND wptr, short lcount);		/* moves display down one line (scrolls down) */
static void pageup(HWND wptr);		/* moves display down one page (scrolls down) */
static void pagedown(HWND wptr);		/* moves display up one page (scrolls up) */
static void scrolllines(HWND wptr, short start, short nlines);	/* adjusts lines array & scrolls display + - n lines */
static void checkhscroll(HWND wptr);		/* scales, sets horizontal scroll bars */
static void trackhscroll(HWND wptr, HWND hctl, UINT code, int pos);	/* displays repetitively while mousdown in scrollbar */
static void scrollh(HWND wptr, long offset);	/* scrolls window horizontally */
static void invalidateclientrect(HWND hwnd);
static void resettimer(HWND hwnd);		// sets or resets save timer
static BOOL isinmargin(HWND hwnd, int offset);	// checks whether point is in margin

static short selectlines(HWND wptr, short target, short extend, short level, struct selstruct *slptr); /* selects & highlights lines */
static HRGN formselrgn(HWND wptr, short fline, short lline, short lheight);	/* forms & returns selection rgn */
static short linesonscreen(HWND wptr, RECN rnum, short * first);		/* finds how many lines on screen */
static void CALLBACK autosave(HWND hwnd, UINT msg, UINT_PTR timer, DWORD time);	/* auto saves index */
static void lcopy(HWND hwnd);	/* copies selection to clipboard */
static void lpaste(HWND hwnd);	/* pastes records from clipboard */

/*******************************************************************************/
void view_allrecords(HWND hwnd)	/* displays all records */

{
	INDEX * FF = getowner(hwnd);
	
	grp_closecurrent(FF);
	FF->viewtype = VIEW_ALL;
	resetdisplay(FF);		/* reset whole display in current form */
}
/*******************************************************************************/
static void dotempgroup(HWND hwnd)	/* displays search records */

{
	INDEX * FF = getowner(hwnd);

	grp_closecurrent(FF);
	FF->curfile = FF->lastfile;		/* enable temp group */
	FF->viewtype = VIEW_TEMP;
	resetdisplay(FF);		/* reset whole display in current form */
}
/*******************************************************************************/
static void donewrecords(HWND hwnd)	/* displays new records */

{
	INDEX * FF = getowner(hwnd);
	
	grp_closecurrent(FF);
	FF->viewtype = VIEW_NEW;
	resetdisplay(FF);		/* reset whole display in current form */
}
/*******************************************************************************/
void view_showgroup(HWND wptr, char * gname)	/* displays group */

{
	INDEX * FF = getowner(wptr);
	
	if (!grp_install(FF,gname))	{
		FF->viewtype = VIEW_GROUP;
		resetdisplay(FF);		/* reset whole display in current form */
	}
}
/*******************************************************************************/
static void dofullform(HWND hwnd)	/* shows fully formatted */

{
	INDEX * FF = getowner(hwnd);
	
	file_disposesummary(FF);		/* dispose of any summary */
	FF->head.privpars.vmode = VM_FULL;
	view_redisplay(FF,0,VD_CUR|VD_RESET|VD_TOP);
	ldoviewbuttons(FF);	/* set buttons */
}
/*******************************************************************************/
static void dodraftform(HWND hwnd)	/* shows draft formatted */

{
	INDEX * FF = getowner(hwnd);
	
	file_disposesummary(FF);		/* dispose of any summary */
	FF->head.privpars.vmode = VM_DRAFT;
	view_redisplay(FF,0,VD_CUR|VD_RESET|VD_TOP);
	ldoviewbuttons(FF);	/* set buttons */
}
/*******************************************************************************/
long mc_summary(HWND hwnd,int comid,HWND chandle,UINT notify)	/* shows summary */

{
	INDEX * FF = getowner(hwnd);
	
	if (!FF->sumsource)	{	/* if don't already have summary */
		if (FF->sumsource = file_buildsummary(FF))		/* make summary index */
			FF->head.privpars.vmode = VM_SUMMARY;
		view_redisplay(FF,0,VD_CUR|VD_RESET|VD_TOP);
		ldoviewbuttons(FF);	/* set buttons */
	}
	return (0);
}
/*******************************************************************************/
long mc_unform(HWND hwnd,int comid,HWND chandle,UINT notify)	/* shows unformatted */

{
	INDEX * FF = getowner(hwnd);
	
	file_disposesummary(FF);		/* dispose of any summary */
	FF->head.privpars.vmode = VM_NONE;
	view_redisplay(FF,0,VD_CUR|VD_RESET|VD_TOP);
	ldoviewbuttons(FF);	/* set buttons */
	return (0);
}
/*******************************************************************************/
long mc_hidebelow(HWND hwnd,int comid,HWND chandle,UINT notify)	/* hides fields below specified */

{	
	INDEX * FF = getowner(hwnd);
	int index;
	
	index = comid-IDM_VIEW_VIEWDEPTH_FIRST;
	if (index == PAGEINDEX)		/* if wanted last item (locator) */
		index = ALLFIELDS;
	else
		index += 1;
	if (index != FF->head.privpars.hidebelow)	{
		FF->head.privpars.hidebelow = index;
		view_clearselect(FF->vwind);			 /* clears selection */
		view_redisplay(FF,0,VD_CUR|VD_RESET);
	}
	return (0);
}
/*******************************************************************************/
long mc_shownum(HWND hwnd,int comid,HWND chandle,UINT notify)	/* shows numbers on records */

{
	INDEX * FF = getowner(hwnd);
	
	com_check(IDM_VIEW_SHOWNUMBERS,FF->head.privpars.shownum ^= 1);
	view_redisplay(FF,0,VD_CUR|VD_RESET);
	return (0);
}
/*******************************************************************************/
long mc_wraplines(HWND hwnd,int comid,HWND chandle,UINT notify)	/* wraps lines */

{
	INDEX * FF = getowner(hwnd);
	
	com_check(IDM_VIEW_WRAPLINES,FF->head.privpars.wrap ^= 1);
	view_redisplay(FF,0,VD_CUR);
	return (0);
}
/*******************************************************************************/
static void ldosortonoff(HWND hwnd)	/* sets sort on or off */

{
	INDEX * FF = getowner(hwnd);
	
	com_check(IDM_VIEW_SHOWSORTED,FF->head.sortpars.ison ^= 1);
	view_clearselect(hwnd);			 /* clears selection */
	view_redisplay(FF,0,VD_TOP);
	view_setstatus(FF->vwind);		/* set title (forces display of sort status) */
	ldoviewbuttons(FF);	/* set buttons */
}
/*******************************************************************************/
static void ldofont(HWND hwnd,HWND cb, int code)	/* sets font */

{
	if (cb)	{
		if (code == CBN_SELENDOK || code == CBN_SELENDCANCEL)	{		/* if done */
			INDEX * FF = getowner(hwnd);
			TCHAR fname[FSSTRING];

			if (code == CBN_SELENDOK)	{
				int sel = ComboBox_GetCurSel(cb);
				char * fn;

				ComboBox_GetLBText(cb,sel,fname);	/* get text */
				fn = fromNative(fname);
				if (strcmp(FF->head.fm[0].name,fn))	{	/* if current alt not same as default */
					if (!strcmp(FF->head.fm[0].name,FF->head.fm[0].pname))	/* if preferred and alt same */
						strcpy(FF->head.fm[0].pname,fn);		/* change preferred */
					strcpy(FF->head.fm[0].name,fn);
					view_redisplay(FF,0,VD_CUR|VD_RESET);
				}
			}
			else	/* cancelled NB: this doesn't work!! */
				ComboBox_SelectString(cb,-1,FF->head.fm[0].pname);
			SetFocus(hwnd);	/* restore focus to window */
		}
	}
	else if (code == 1)	{	/* from accelerator */
		SendMessage(g_hwfcb,CB_SHOWDROPDOWN,TRUE,0);
		SetFocus(g_hwfcb);
	}
}
/************************************************************************/
static void lsetalignment(HWND hwnd, int alignment)	// sets text alignment

{
	WX(hwnd,owner)->head.formpars.pf.alignment = alignment;
	view_redisplay(getowner(hwnd),0,VD_CUR|VD_RESET);	/* force reset of display pars */
}
/************************************************************************/
static void lswitchform(HWND hwnd, int indented)	/* switches between formats */

{
	WX(hwnd,owner)->head.formpars.ef.runlevel = indented ? 0 : 1;
	view_redisplay(getowner(hwnd),0,VD_CUR|VD_RESET);	/* force reset of display pars */
}
/*******************************************************************************/
static void lswitchsort(HWND hwnd, int alpha)	/* switches between alpha & page sort */

{
	INDEX * FF = getowner(hwnd);

	if (!FF->mf.readonly) {
		SORTPARAMS *sg;
		int count;

		sg = FF->curfile ? &FF->curfile->sg : &FF->head.sortpars;
		for (count = 0; count < FF->head.indexpars.maxfields; count++) {	/* for all fields */
			if (alpha)
				sg->fieldorder[count] = count == FF->head.indexpars.maxfields - 1 ? PAGEINDEX : count;
			else
				sg->fieldorder[count] = !count ? PAGEINDEX : count - 1;
		}
		if (FF->curfile)
			sort_sortgroup(FF);
		else
			sort_resort(FF);
		view_redisplay(FF, 0, VD_CUR | VD_RESET);	/* force reset of display pars */
		view_setstatus(hwnd);
		ldoviewbuttons(FF);		// for benfit of API server
	}
}
/*******************************************************************************/
static void ldosize(HWND hwnd,HWND cb,UINT notify)	/* sets font size */

{
	TCHAR tbuff[FSSTRING];
	char tbuff1[FSSTRING], *eptr;
	long newsize;
	INDEX * FF;
	
	if (cb)	{	/* if from control */
		FF = getowner(hwnd);
		if (notify == CBN_KILLFOCUS)	{
			GetWindowText(cb,tbuff,FSSTRING);
			newsize = strtol(fromNative(tbuff), &eptr,10);	/* get val */
			if (newsize <= 0 || *eptr)	{
				_itoa(FF->head.privpars.size,tbuff1,10);
				SetWindowText(cb,toNative(tbuff1));
				if (newsize <0 || *eptr)	{		/* if a bad number */
					senderr(ERR_BADNUMERR,WARNNB,tbuff);
					SetFocus(cb);
				}
			}
			else if (newsize != FF->head.privpars.size)	{
				FF->head.privpars.size = (short)newsize;
				view_redisplay(FF,0,VD_CUR|VD_RESET|VD_IMMEDIATE);
			}
		}
		else if (notify == CBN_SELENDOK)	{	/* list selection */
			int sel;
			sel = ComboBox_GetCurSel(cb);
			ComboBox_GetLBText(cb,sel,tbuff);	/* for forcing update of text box */
			SetWindowText(cb,tbuff);			/* subsquent closeup -> killfocus -> action */
		}
		else if (notify == CBN_SELENDCANCEL || notify == CBN_CLOSEUP)
			SetFocus(hwnd);
	}
	else if (notify == 1)	{	/* from accelerator */
		SendMessage(g_hwscb,CB_SHOWDROPDOWN,TRUE,0);
		SetFocus(g_hwscb);
	}
}
/*********************************************************************************/
void view_savecontext(INDEX * FF, struct vcontext * vc)	/* saves view context */

{	
	vc->sort = FF->head.sortpars.ison;		/* sort status */
	vc->priv = FF->head.privpars;			/* private params */
	vc->topline = LX(FF->vwind,lines)[0];	/* first line */
}
/*********************************************************************************/
void view_restorecontext(INDEX * FF, struct vcontext * vc, int returnflag)	/* restores view context */

{	
	FF->head.sortpars.ison = vc->sort;		/* sort status */
	FF->head.privpars = vc->priv;			/* private params */
	if (returnflag)		/* if want to return to entry point */
		LX(FF->vwind,lines)[0] = vc->topline;	/* first line */
}
/*********************************************************************************/
HWND view_setwindow(INDEX * FF, short visflag)	/*  sets up view window */

{
	HWND hwnd;
	RECT crect, prect;

	GetClientRect(FF->cwind,&prect);
	adjustWindowRect(g_hwclient,&crect,WS_HSCROLL|WS_VSCROLL|WS_CLIPCHILDREN,FALSE,WS_EX_CLIENTEDGE);	/* now get window rect that has right client area */
	hwnd = CreateWindowEx(WS_EX_CLIENTEDGE/*|WS_EX_WINDOWEDGE*/,g_indexclass,TEXT(""),WS_HSCROLL|WS_VSCROLL|WS_CHILD|WS_CLIPCHILDREN|WS_VISIBLE,
		prect.left,prect.top,prect.right,prect.bottom,FF->cwind,NULL,g_hinst,FF);
	return (hwnd);
}
/************************************************************************/
LRESULT CALLBACK view_proc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)

{
	switch (msg)	{
		HANDLE_MSG(hwnd,WM_CREATE,lcreate);
		HANDLE_MSG(hwnd,WM_HSCROLL,trackhscroll);
		HANDLE_MSG(hwnd,WM_VSCROLL,trackvscroll);
		HANDLE_MSG(hwnd,WM_PAINT,lupdate);
		HANDLE_MSG(hwnd,WM_SIZE,lsize);
		HANDLE_MSG(hwnd,WM_SETFOCUS,lsetfocus);
		HANDLE_MSG(hwnd,WM_KILLFOCUS,lkillfocus);
		HANDLE_MSG(hwnd,WM_LBUTTONDOWN,ldomousedown);
		HANDLE_MSG(hwnd,WM_LBUTTONUP,ldomouseup);
		HANDLE_MSG(hwnd,WM_LBUTTONDBLCLK,ldomousedown);
		HANDLE_MSG(hwnd,WM_MOUSEMOVE,ldomousemove);
		HANDLE_MSG(hwnd,WM_SETCURSOR,ldocursor);
		HANDLE_MSG(hwnd,WM_KEYDOWN,ldokey);
		HANDLE_MSG(hwnd,WM_CHAR,ldochar);
		HANDLE_MSG(hwnd,WM_INITMENUPOPUP,ldomenu);
		HANDLE_MSG(hwnd,WM_COMMAND,lcommand);
		HANDLE_MSG(hwnd,WM_MOUSEWHEEL,ldowheel);
		HANDLE_MSG(hwnd,WM_CONTEXTMENU,ldocontextmenu);
//		case WMM_UPDATETOOLBARS:
//			lupdatetoolbar(hwnd);
//			return (0);
	}
	return (DefWindowProc(hwnd,msg,wParam,lParam));
}
/************************************************************************/
static void lcommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)	/* does menu tasks */

{
	switch (id)	{
		case IDM_FILE_SAVE:
			file_saveprivatebackup(getowner(hwnd));
			resettimer(hwnd);		// restart save timer after manual flush
			return;
		case IDM_FILE_SAVEAS:
			exp_export(hwnd);
			return;
		case IDM_FILE_SAVEACOPYAS:
			file_saveacopy(hwnd);
			return;
		case IDM_FILE_REVERT:
			file_revertprivatebackup(getowner(hwnd));
			return;
		case IDM_FILE_PAGESETUP:
			lsetpage(hwnd);
			return;
		case IDM_FILE_PRINT:
			lprint(hwnd);
			return;
		case IDM_EDIT_COPY:
			lcopy(hwnd);
			return;
		case IDM_EDIT_PASTE:
			lpaste(hwnd);
			return;
		case IDM_EDIT_SELECTALL:
			lselall(hwnd);
			return;
		case IDM_EDIT_NEWRECORD:
			mod_settext(getowner(hwnd),NOTEXT,NEWREC,NULL);			
			return;
		case IDM_EDIT_EDITRECORD:
			mod_settext(getowner(hwnd),NOTEXT,LX(hwnd,sel).first,NULL);			
			return;
		case IDM_EDIT_DUPLICATE:
			edit_duplicate(hwnd);
			return;
		case IDM_EDIT_DEMOTE:
			edit_demote(hwnd);
			return;
		case IDM_EDIT_REMOVEMARK:
			edit_removemark(hwnd);
			return;
		case IDM_EDIT_DELETE:
			edit_delrestore(hwnd);
			return;
//		case IDM_EDIT_LABEL_SWITCH:
		case IDM_EDIT_LABEL0:
		case IDM_EDIT_LABEL1:
		case IDM_EDIT_LABEL2:
		case IDM_EDIT_LABEL3:
		case IDM_EDIT_LABEL4:
		case IDM_EDIT_LABEL5:
		case IDM_EDIT_LABEL6:
		case IDM_EDIT_LABEL7:
		case IDM_EDIT_LABEL8:
			edit_switchtag(hwnd,id-IDM_EDIT_LABEL0);			
			return;
		case IDM_EDIT_NEWGROUP:
			sset_makegroup(hwnd);
			return;
		case IDM_EDIT_SAVEGROUP:
			edit_savegroup(hwnd);
			return;
		case IDM_VIEW_GOTO:
			sset_goto(hwnd);
			return;
		case IDM_VIEW_ATTRIBUTES:
			sset_filter(hwnd);
			return;
		case IDM_VIEW_ALLRECORDS:
			view_allrecords(hwnd);
			return;
		case IDM_VIEW_NEWRECORDS:
			donewrecords(hwnd);
			return;
		case IDM_VIEW_TEMPORARYGROUP:
			dotempgroup(hwnd);
			return;
		case IDM_VIEW_FULLFORMAT:
			dofullform(hwnd);
			return;
		case IDM_VIEW_DRAFTFORMAT:
			dodraftform(hwnd);
			return;
		case IDM_VIEW_SHOWSORTED:
			ldosortonoff(hwnd);
			return;
		case IDM_ALIGNMENT_NATURAL:
		case IDM_ALIGNMENT_LEFT:
		case IDM_ALIGNMENT_RIGHT:
			lsetalignment(hwnd,id-IDM_ALIGNMENT_NATURAL);
			return;
		case IDB_VIEW_INDENTED:
		case IDB_VIEW_RUNIN:
			lswitchform(hwnd, id == IDB_VIEW_INDENTED);
			return;
		case IDB_VIEW_SORTALPHA:
		case IDB_VIEW_SORTPAGE:
			lswitchsort(hwnd, id == IDB_VIEW_SORTALPHA);
			return;
		case IDB_VIEW_SORTNONE:
			ldosortonoff(hwnd);
			return;
		case IDM_COMBO_FONT:
			ldofont(hwnd,hwndCtl,codeNotify);
			return;
		case IDM_COMBO_SIZE:
			ldosize(hwnd,hwndCtl,codeNotify);
			return;
		case IDM_DOCUMENT_MARGINSCOLUMNS:
			fs_margcol(hwnd);
			return;
		case IDM_DOCUMENT_HEADERSFOOTER:
			fs_headfoot(hwnd);
			return;
		case IDM_DOCUMENT_GROUPINGENTRIES:
			fs_entrygroup(hwnd);
			return;	
		case IDM_DOCUMENT_STYLELAYOUT:
			fs_stylelayout(hwnd);
			return;	
		case IDM_DOCUMENT_HEADINGS:
			fs_headings(hwnd);
			return;	
		case IDM_DOCUMENT_CROSSREFERENCES:
			fs_crossrefs(hwnd);
			return;			
		case IDM_DOCUMENT_PAGEREFERENCES:
			fs_pagerefs(hwnd);
			return;			
		case IDM_DOCUMENT_STYLEDSTRINGS:
			fs_styledstrings(hwnd);
			return;			
		case IDM_TOOLS_CHECKINDEX:
			ts_checkindex(hwnd);
			return;
		case IDM_TOOLS_RECONCILEHEADINGS:
			ts_reconcile(hwnd);
			return;
		case IDM_TOOLS_GENERATECROSSREFERENCES:
			ts_managecrossrefs(hwnd);
			return;
		case IDM_TOOLS_ALTERREFERENCES:
			ts_adjustrefs(hwnd);
			return;
		case IDM_TOOLS_SPLITHEADINGS:
			ts_splitheadings(hwnd);
			return;
		case IDM_TOOLS_SORT:
			ss_sortindex(hwnd);
			return;
		case IDM_TOOLS_COMPRESS:
			ss_compress(hwnd);
			return;
		case IDM_TOOLS_EXPAND:
			ss_expand(hwnd);
			return;
		case IDM_TOOLS_FONTS:
			ts_fontsub(getowner(hwnd), NULL);
			return;		
		case IDM_TOOLS_GROUPS:
			edit_managegroups(hwnd);
			return;
	}
	FORWARD_WM_COMMAND(hwnd,id,hwndCtl,codeNotify, DefWindowProc);
}
/************************************************************************/
static void ldomenu(HWND hwnd, HMENU hMenu, UINT item, BOOL fSystemMenu)	/* sets menus */

{
	int menuid = menu_getmid(hMenu);
	INDEX * FF = getowner(hwnd);

	mcr_setmenu(FALSE);		// enable macro menus
	if (FF->rwind)	// if record window open
		com_setenable(ERSVIEW,XONLY,ON);		/* enable reduced set for main view */
	else {
		com_setenable(EVIEW,XONLY,ON);		/* enable set for index */
		com_setbyindexstate(hwnd, hMenu,item);	/* set general program commands by current state */

		if (menuid == IDPM_EDIT || menuid == IDPM_FILE || menuid == IDPM_LABEL)	{	/* if edit or file menu */
			IDataObject * dp;
			RECORD * recptr;

			com_set(IDM_EDIT_FINDAGAIN,fs_testok(FF));
			if (view_recordselect(hwnd))		{		/* if have selection */
				if (FF->mf.readonly) {	/* if read only */
					com_set(IDM_EDIT_COPY, ON);
					com_set(IDM_EDIT_NEWGROUP, ON);
				}
				else	{
					com_setenable(FF->head.privpars.vmode == VM_FULL ? EVFSEL : EVSEL,ONLY,ON);
					if (recptr = rec_getrec(FF,LX(hwnd,sel).first))	{
						com_check(IDM_EDIT_DELETE,recptr->isdel);
						if (menuid == IDPM_LABEL)	{
							CheckMenuRadioItem(hMenu,IDM_EDIT_LABEL0,IDM_EDIT_LABEL0+FLAGLIMIT,IDM_EDIT_LABEL0+recptr->label,MF_BYCOMMAND);
							menu_setlabelcolors(hMenu);
						}
					}
				}
			}
			else			/* clear check marks */
				com_check(IDM_EDIT_DELETE,FALSE);
			if (FF->mf.readonly || OleGetClipboard(&dp) != S_OK)
				com_set(IDM_EDIT_PASTE,OFF);
			else
				com_set(IDM_EDIT_PASTE,recole_ispastable(dp));
		}
	}
	if (menuid == IDPM_ALIGNMENT)
		CheckMenuRadioItem(hMenu,IDM_ALIGNMENT_NATURAL,IDM_ALIGNMENT_RIGHT,IDM_ALIGNMENT_NATURAL+FF->head.formpars.pf.alignment,MF_BYCOMMAND);
	if (menuid == IDPM_ABBREVIATIONS)	/* abbrevs submenu menu */
		com_set(IDM_TOOLS_ABBREV_DETACH,abbrev_hasactiveset());
}
/************************************************************************/
static void ldocontextmenu(HWND hwnd, HWND hwndContext, UINT xpos, UINT ypos)	/* does context menus */

{
	HMENU mhs;

	if ((mhs = GetSubMenu(g_popmenu,1)))	{		/* if can get submenu */
		INDEX * FF = getowner(hwnd);
		BOOL state = view_recordselect(hwnd) && !FF->rwind && !FF->mf.readonly ? MF_ENABLED : MF_GRAYED;
		BOOL nrstate = view_recordselect(hwnd) && !FF->rwind  ? MF_ENABLED : MF_GRAYED;
		MENUITEMINFO mi;
	
		mi.cbSize = sizeof(mi);
		mi.fMask = MIIM_SUBMENU;		// label submenu
		mi.hSubMenu = GetSubMenu(g_popmenu,2);
		SetMenuItemInfo(mhs,IDM_EDIT_TAG,FALSE,&mi);
		EnableMenuItem(mhs,0,MF_BYPOSITION|state);	/* need by position for Copy occurs in two menus */
		EnableMenuItem(mhs,IDM_EDIT_EDITRECORD,MF_BYCOMMAND|state);
		EnableMenuItem(mhs,IDM_EDIT_REMOVEMARK, MF_BYCOMMAND | state);
		EnableMenuItem(mhs,IDM_EDIT_DEMOTE, MF_BYCOMMAND | state);
		EnableMenuItem(mhs,IDM_EDIT_DUPLICATE,MF_BYCOMMAND|state);
		EnableMenuItem(mhs,IDM_EDIT_DELETE,MF_BYCOMMAND|state);
		EnableMenuItem(mhs,IDM_EDIT_TAG,MF_BYCOMMAND|state);
		EnableMenuItem(mhs,IDM_EDIT_NEWGROUP,MF_BYCOMMAND| nrstate);
		if (state)	{	// if enabled
			RECORD * recptr;
			if (recptr = rec_getrec(FF,LX(hwnd,sel).first))	{
				CheckMenuItem(mhs,IDM_EDIT_DELETE, recptr->isdel ? MF_BYCOMMAND|MF_CHECKED : MF_BYCOMMAND|MF_UNCHECKED);	/* set it */
//				CheckMenuItem(mhs,IDM_EDIT_TAG, recptr->label ? MF_BYCOMMAND|MF_CHECKED : MF_BYCOMMAND|MF_UNCHECKED);	/* set it */
//				CheckMenuRadioItem(mi.hSubMenu,IDM_EDIT_LABEL0,IDM_EDIT_LABEL0+FLAGLIMIT,IDM_EDIT_LABEL0+recptr->label,MF_BYCOMMAND);
			}
		}
		TrackPopupMenuEx(mhs,TPM_VERTICAL|TPM_LEFTALIGN|TPM_TOPALIGN|TPM_RIGHTBUTTON,
			xpos,ypos,hwnd,NULL);
	}
}
/*******************************************************************************/
static void setbasetoolbar(HWND hwnd)		/* updates all toolbar buttons */

{
	INDEX *FF = getowner(hwnd);

	if (!FF->rwind)	{	// if separate record entry window open
//		SendMessage(g_hwtoolbar,TB_ENABLEBUTTON,IDM_FILE_NEW,TRUE);
//		SendMessage(g_hwtoolbar,TB_ENABLEBUTTON,IDM_FILE_OPEN,TRUE);
		SendMessage(g_hwtoolbar,TB_ENABLEBUTTON,IDM_FILE_SAVE,TRUE);	/* set button */
		SendMessage(g_hwtoolbar,TB_ENABLEBUTTON,IDM_FILE_PRINT,TRUE);	/* set button */
		SendMessage(g_hwtoolbar,TB_ENABLEBUTTON,IDM_EDIT_CUT,FALSE);	/* disable buttons */
		SendMessage(g_hwtoolbar,TB_ENABLEBUTTON,IDM_EDIT_FIND,TRUE);	/* set button */
		SendMessage(g_hwtoolbar,TB_ENABLEBUTTON,IDM_EDIT_REPLACE,!FF->mf.readonly);	/* set button */
		ldoviewbuttons(FF);		/* sets view buttons */
		doselbuttons(hwnd);
		dopastebutton(FF);
		com_settextmenus(FF,ON,ON);
	}
}
/******************************************************************************/
static void ldoviewbuttons(INDEX * FF)	/* sets menus */

{
	if (!FF->rwind)	{	// if no record entry window open
		short fieldindex;

		SendMessage(g_hwtoolbar,TB_ENABLEBUTTON,IDM_VIEW_FULLFORMAT,TRUE);	/* enable these */
		SendMessage(g_hwtoolbar,TB_ENABLEBUTTON,IDM_VIEW_DRAFTFORMAT,TRUE);
		SendMessage(g_hwtoolbar,TB_ENABLEBUTTON,IDM_EDIT_NEWRECORD,!FF->mf.readonly);
		SendMessage(g_hwtoolbar,TB_ENABLEBUTTON,IDM_VIEW_ALLRECORDS,FF->viewtype != VIEW_ALL);
		SendMessage(g_hwtoolbar,TB_CHECKBUTTON,IDM_VIEW_FULLFORMAT,FF->head.privpars.vmode == VM_FULL);
		SendMessage(g_hwtoolbar,TB_CHECKBUTTON,IDM_VIEW_DRAFTFORMAT,FF->head.privpars.vmode != VM_FULL);
		SendMessage(g_hwtoolbar,TB_ENABLEBUTTON,IDB_VIEW_INDENTED,FF->head.privpars.vmode == VM_FULL);/* set button */
		SendMessage(g_hwtoolbar,TB_ENABLEBUTTON,IDB_VIEW_RUNIN,FF->head.privpars.vmode == VM_FULL);	/* set button */
		SendMessage(g_hwtoolbar,TB_CHECKBUTTON,IDB_VIEW_INDENTED,!FF->head.formpars.ef.runlevel);
		SendMessage(g_hwtoolbar,TB_CHECKBUTTON,IDB_VIEW_RUNIN,FF->head.formpars.ef.runlevel);

		SendMessage(g_hwtoolbar,TB_ENABLEBUTTON,IDB_VIEW_SORTALPHA,!FF->mf.readonly);	/* set button */
		SendMessage(g_hwtoolbar,TB_ENABLEBUTTON,IDB_VIEW_SORTPAGE,!FF->mf.readonly);	/* set button */
		SendMessage(g_hwtoolbar, TB_ENABLEBUTTON, IDB_VIEW_SORTNONE, TRUE);	/* set button */
		fieldindex = FF->curfile ? FF->curfile->sg.fieldorder[0] : FF->head.sortpars.fieldorder[0];
		SendMessage(g_hwtoolbar,TB_CHECKBUTTON,IDB_VIEW_SORTALPHA,fieldindex != PAGEINDEX && FF->head.sortpars.ison);
		SendMessage(g_hwtoolbar,TB_CHECKBUTTON,IDB_VIEW_SORTPAGE,fieldindex == PAGEINDEX && FF->head.sortpars.ison);
		SendMessage(g_hwtoolbar, TB_CHECKBUTTON, IDB_VIEW_SORTNONE, !FF->head.sortpars.ison);
	}
}
/******************************************************************************/
static void doselbuttons(HWND hwnd)	/* does selection-dependent buttons */

{
	INDEX * FF = getowner(hwnd);

	if (!FF->rwind)	{	// if no record entry window open
		LFLIST * lfp = getdata(hwnd);
		int selenable = lfp->sel.first && !FF->rwind ;
		RECORD * trptr;

		SendMessage(g_hwtoolbar,TB_ENABLEBUTTON,IDM_EDIT_COPY,selenable);	/* set button */
		SendMessage(g_hwtoolbar,TB_ENABLEBUTTON,IDM_EDIT_DELETE,selenable && !FF->mf.readonly);	/* set */
		SendMessage(g_hwtoolbar,TB_ENABLEBUTTON,IDM_EDIT_TAG,selenable && !FF->mf.readonly);	/* enable flag */
		if (selenable && (trptr = rec_getrec(FF,lfp->sel.first)))	{
			SendMessage(g_hwtoolbar,TB_CHECKBUTTON,IDM_EDIT_DELETE,trptr->isdel);	/* set */
			SendMessage(g_hwtoolbar,TB_CHECKBUTTON,IDM_EDIT_TAG,trptr->label > 0);	/* set */
		}
		SendMessage(g_hwstatus,SB_SIMPLE,FALSE,0);	/* restore multi-part status window */
	}
}
/******************************************************************************/
static void dopastebutton(INDEX * FF)	/* does paste button */

{
	if (!FF->rwind)	{	// if no record entry window open
		IDataObject * dp;

		SendMessage(g_hwtoolbar,TB_ENABLEBUTTON,IDM_EDIT_PASTE,!FF->mf.readonly  && !FF->rwind && OleGetClipboard(&dp) == S_OK && recole_ispastable(dp));
	}
}
/******************************************************************************/
static BOOL lcreate(HWND hwnd,LPCREATESTRUCT cs)		/* initializes view window */

{
	LFLIST * lfp;
	INDEX * FF;

	if (lfp = getmem(sizeof(LFLIST)))	{	/* if can get memory for our window structure */
		setdata(hwnd,lfp);		/* get memory */
		FF = WX(hwnd,owner) = (INDEX *)cs->lpCreateParams;	/* recover param passed (FF) */
		FF->vwind = hwnd;
		setddimensions(hwnd,TRUE);	/* set destination rect */
		setpars(hwnd);			/* bounds, etc. */
		resettimer(hwnd);
		lfp->trgobj = dtrg_call;
		lfp->trgobj.hwnd = hwnd;
		lfp->sptr = lfp->hold = lfp->searchstring;
		RegisterDragDrop(hwnd,(IDropTarget *)&lfp->trgobj);
		return (TRUE);
	}
	return (FALSE);
}
/*******************************************************************************/
static void resettimer(HWND hwnd)		// sets or resets save timer

{
	if (g_prefs.gen.saveinterval && !WX(hwnd, owner)->mf.readonly)	// if want index saved regularly
		LX(hwnd,savetimer) = SetTimer(hwnd,SAVE_TIMER,g_prefs.gen.saveinterval*1000,autosave);
}
/*****************************************************************************/
static void lupdate(HWND hwnd)		/* updates view window */

{
	LFLIST * lfp = (LFLIST *)getdata(hwnd);
	PAINTSTRUCT ps;
	long firstline, lastline;

	if (!lfp->printflag)	{		/* if we're not printing */
		SetTextColor(lfp->hdc,GetSysColor(COLOR_WINDOWTEXT));	/* in case global change from control panel */
		BeginPaint(hwnd, &ps);
		firstline = ps.rcPaint.top/lfp->lheight;
		lastline = (ps.rcPaint.bottom+lfp->lheight-1)/lfp->lheight;
		if (lastline == firstline)	/* always do at least 1 line */
			lastline++;
		if (lastline > lfp->nlines)	/* if enlarged region beyond current line set */
			lastline = lfp->nlines;	/* impose limit */
		drawlines(hwnd,firstline,lastline-firstline);
		EndPaint(hwnd,&ps);
	}
}
/******************************************************************************/
static void lsize(HWND hwnd, UINT state, int cx, int cy)

{
	LFLIST * lfp;

	if ((lfp = getdata(hwnd)) /* && (state == SIZE_MAXIMIZED || state == SIZE_RESTORED) */)	{	/* if not destroying or minimizing, etc */
		setvdimensions(hwnd);
		setlines(hwnd,0,SHRT_MAX,lfp->lines[0].rnum,0);
		checkhscroll(hwnd);		/* scales & activates as necess */
		checkvscroll(hwnd);		/* scales & activates as necess */
	}
}
/******************************************************************************/
static void lselall(HWND hwnd)		/* selects all */

{
	RECORD * curptr;
	INDEX * FF;
	struct selstruct sel;
	
	FF = getowner(hwnd);
	sel = v_statsel;
	if (curptr = sort_top(FF))	{
		sel.first = curptr->num;
		if (curptr = sort_bottom(FF))	{
			sel.last = curptr->num;
			selectlines(hwnd,-1,FALSE,0,&sel);
		}
	}
}
/**********************************************************************************/
short view_close(HWND hwnd)		/* closes index */

{
	INDEX * FF = getowner(hwnd);
	short iindex;
	
	if (!FF->rwind || mod_close(FF->rwind,MREC_ALLOWCHECK))	{	/* if no rec window or closed sucessfully */
		SetCursor(g_waitcurs);
		if (FF->twind)							/* if have open text window */
			SendMessage(FF->twind,WM_CLOSE,0,0);	/* shut it down */
		if (g_findw && FX(g_findw,lastindex) == FF)	{	/* if find dialog open for us */
			SendMessage(g_findw,WM_COMMAND,IDCANCEL,0);	// hide
			FX(g_findw,lastindex) = NULL;
		}
		if (g_repw && RX(g_repw,lastindex) == FF)	{	/* if replace dialog open for us */
			SendMessage(g_repw,WM_COMMAND,IDCANCEL,0);	// clean up and hide
			RX(g_repw,lastindex) = NULL;
		}
#if !READER		// if not Reader
		sset_closeifused(FF);	/* close spell window if we control it */
#endif
		if (LX(hwnd,savetimer))		/* if have save timer */
			KillTimer(hwnd,LX(hwnd,savetimer));		/* kill it */
		if ((iindex = index_findindex(FF)) >= 0)	{	/* if index exists (might not after resize) */
			file_disposesummary(FF);
			if (index_close(FF))	{/* discard index */
				senderr(ERR_INDEXCLOSERR,WARN,FF->pfspec);
				return (FALSE);		/* error discarding */
			}
		}
		RevokeDragDrop(hwnd);
		freedata(hwnd);		/* free function list */
		DestroyWindow(hwnd);	// destroy
		return (TRUE);		/* always indicate we've processed it */
	}
	return (FALSE);
}
/******************************************************************************/
static void lsetfocus(HWND hwnd, HWND oldfocus)	/* gets focus shift message */

{
	invalidateclientrect(hwnd);
	getowner(hwnd)->currentfocus = hwnd;
	setbasetoolbar(hwnd);
//	FORWARD_WM_SETFOCUS(hwnd,oldfocus,DefMDIChildProc);
}
/******************************************************************************/
static void lkillfocus(HWND hwnd, HWND newfocus)	/* gets focus shift message */

{
	invalidateclientrect(hwnd);
	if (newfocus != g_hwfcb && newfocus != g_hwscb && newfocus != g_hwscbed)	// if not toolbar drop-down
		com_setdefaultbuttons(FALSE);
//	FORWARD_WM_KILLFOCUS(hwnd,newfocus,DefMDIChildProc);
}
/******************************************************************************/
static void ldomousedown(HWND hwnd, BOOL dclick, int px, int py, UINT flags)	/* handles mouse down */

{
	INDEX * FF = getowner(hwnd);
	LFLIST * lfp = getdata(hwnd);
	RECORD * curptr;
	short line;
	char sptr[MAXREC+1], *xptr;

	SetFocus(hwnd);
	line = (py-lfp->vrect.top)/lfp->lheight;	/* find line */
	if (lfp->lines[line].rnum)	{
		struct selstruct sel = v_statsel;	// initialize selection struct

		if (FF->rwind) {
			RECN rnum = lfp->lines[line].rnum;	// preserve new target record (lfp->lines changes)
			if (mod_canenterrecord(FF->rwind,MREC_ALWAYSACCEPT)) 	/* if in a record && can enter any open one */
				mod_settext(FF,NOTEXT,rnum,hwnd);
			return;
		}
		if (dclick && lfp->sel.last == lfp->sel.first && lfp->lines[line].rnum == lfp->sel.first)	{	// if double-click on selected record
			mod_settext(FF,NOTEXT,lfp->sel.first,NULL);
			return;
		}
		if (isinmargin(hwnd,px))	{	/* if in margin */
			selectlines(hwnd,line,FALSE,lfp->lines[line].indent&FO_HMASK,&sel);	/* adjust selection */
			curptr = rec_getrec(FF,lfp->lines[line].rnum);	/* get record */
			str_xcpy(sptr, curptr->rtext);		/* get text */
			if (xptr = str_xatindex(sptr,lfp->lines[line].indent&FO_HMASK))	{	/* find field displayed */
				xptr += strlen(xptr++);
				*xptr = EOCS;
				curptr = search_lastmatch(FF,curptr,sptr,FALSE);	/* find last that matches at that level */
				curptr = lfp->getrec(FF,curptr->num);	/* find proper parent of last record */
				sel.last = curptr->num;					/* define end of it */
				sel.first = lfp->sel.first;
				selectlines(hwnd,-1,TRUE,lfp->lines[line].indent&FO_HMASK,&sel);	/* extend selection */
			}
		}
		else {		// down in main area
			if (lfp->lines[line].flags&VSELECT)	{		// if in selection
				DWORD effect;
				POINT pt;

				pt.x = px;
				pt.y = py;
				ClientToScreen(hwnd,&pt);
				if (DragDetect(hwnd,pt))	{	// if starting drag
					recole_capture(hwnd,&rec_call);
					dsrc_call.hwnd = hwnd;
					lfp->dragsource = TRUE;
					DoDragDrop((IDataObject *)&rec_call,(IDropSource *)&dsrc_call,DROPEFFECT_COPY|DROPEFFECT_MOVE,&effect);
					lfp->dragsource = FALSE;
					return;
				}
			}
			lfp->target = selectlines(hwnd,line,flags&MK_SHIFT,0,&sel);	// change/extend current selection
		}
		SetCapture(hwnd);	// set to enable drag selection
	}
}
/******************************************************************************/
static void ldomouseup(HWND hwnd, int px, int py, UINT flags)	/* handles mouse up */

{
	if (GetCapture() == hwnd)	{
		LFLIST *lfp = getdata(hwnd);
		INDEX * FF = getowner(hwnd);

		ReleaseCapture();
		if (lfp->scrolltimer)	{	/* if have a scroll timer */
			KillTimer(hwnd,lfp->scrolltimer);
			lfp->scrolltimer = 0;
		}
//		if (FF->rwind && lfp->sel.first)
//			mod_settext(FF,NOTEXT,lfp->sel.first,hwnd);
	}
}
/******************************************************************************/
static void ldomousemove(HWND hwnd, int px, int py, UINT flags)	/* handles movements */

{
	struct selstruct sel;
	short nline;

	if (GetCapture() == hwnd)	{
		if (flags&MK_LBUTTON)	{	/* if left button down */
			LFLIST *lfp = getdata(hwnd);
			sel = v_statsel;					/* initialize it */
			if (py > lfp->vrect.top && py < lfp->vrect.bottom)	{/* if inside client area */
				if (lfp->scrolltimer)	{
					KillTimer(hwnd,lfp->scrolltimer);
					lfp->scrolltimer = 0;
				}
				nline = (py-lfp->vrect.top)/lfp->lheight;
				if (nline != lfp->target) /* if have dragged within window */
					lfp->target = selectlines(hwnd,nline,TRUE,0,&sel);
				setvscroll(hwnd);
			}
			else if (!lfp->scrolltimer)
				lfp->scrolltimer = SetTimer(hwnd,SCROLL_TIMER,SCROLLDELAY,dragscroll);
		}	
		else
			ReleaseCapture();
	}
}
/******************************************************************************/
static void CALLBACK dragscroll(HWND hwnd, UINT msg, UINT_PTR timer, DWORD time)	/* checks need for drag scroll & does it */

{
	if (GetCapture() == hwnd)	{	/* if we have cursor in drag */
		int lheight = LX(hwnd,lheight);
		int maxlines = LX(hwnd,nlines);
		POINT pt;
		RECT crect;
		struct selstruct sel;
		int step;

		GetCursorPos(&pt);
		ScreenToClient(hwnd,&pt);
		sel = v_statsel;					/* initialize it */
		GetClientRect(hwnd,&crect);

		if (pt.y < crect.top)	{
			step = (crect.top-pt.y+lheight)/lheight;
			if (step >= maxlines)
				step = maxlines;
			linedown(hwnd,step);	// move back by step
			LX(hwnd,target) = selectlines(hwnd,0,TRUE,0,&sel);		/* extend selection */
		}
		else if (pt.y >= crect.bottom)	{
			step = (pt.y-crect.bottom+lheight)/lheight;
			if (step >= maxlines)
				step = maxlines;
			lineup(hwnd,step);	// move forward by step
			LX(hwnd,target) = selectlines(hwnd,LX(hwnd,nlines)-1,TRUE,0,&sel);
		}
		setvscroll(hwnd);
	}
}
/******************************************************************************/
static BOOL ldocursor(HWND hwnd, HWND hwndCursor, UINT hitcode, UINT msg)	/* sets cursor */

{
	POINT pt;

	if (hwnd == hwndCursor && hitcode == HTCLIENT)	{
		GetCursorPos(&pt);
		ScreenToClient(hwnd,&pt);
		if (isinmargin(hwnd,pt.x))	{	/* if in margin */
			SetCursor(g_rightarrow);
			return (TRUE);
		}
	}
	return FORWARD_WM_SETCURSOR(hwnd,hwndCursor,hitcode,msg, DefWindowProc);
}
/******************************************************************************/
static void ldochar(HWND hwnd, TCHAR ch, int cRepeat)		/* handles chars */

{
	INDEX * FF = getowner(hwnd);
	long mtime;
	RECN rnum;
	LFLIST * lfp;

	if ((unsigned char)ch >= SPACE)		{
		lfp = getdata(hwnd);
		mtime = GetMessageTime();
		if (mtime > lfp->lastkeytime + KEYDELAY+g_keydelay)	{	/* if beyond addition time */
			lfp->hold = lfp->sptr = lfp->searchstring;	/* reset pointer */
			SendMessage(g_hwstatus,SB_SIMPLE,FALSE,0);	/* restore multi-part status window */
		}
		lfp->badstring = FALSE;
		lfp->sptr = u8_appendU(lfp->sptr, ch);
		*lfp->sptr = '\0';		/* terminate string */
		if ((*(lfp->sptr-1) == '\\' || *(lfp->sptr-1) == ';') && *(lfp->sptr-2) != '\\' || (!col_collatablelength(FF, lfp->searchstring) && !isdigit(*(lfp->sptr-1))))	// if special lead or no primary search text
			lfp->hold = lfp->sptr;	/* don't seek until we have another character */
		if (lfp->sptr > lfp->hold)	{	/* if not holding */
			if (rnum = com_findrecord(FF,lfp->searchstring,FALSE))	{	/* if can find record */
				if (FF->rwind)	// if have record window open
					mod_settext(FF,NOTEXT,rnum,hwnd);	
				else
					view_selectrec(FF,rnum,VD_SELPOS,-1,-1);		/* select it */
			}
			else	{
				lfp->badstring = TRUE;
				*lfp->searchstring = '\0';
			}
		}
		lfp->lastkeytime = mtime;
		SendMessage(g_hwstatus,SB_SETTEXT,STATSEG_SEARCH|0,(LPARAM)toNative(lfp->searchstring));
	}
}
/******************************************************************************/
static void ldokey(HWND hwnd, UINT vk, BOOL fDown, int cRepeat, UINT flags)		/* handles keystrokes */

{
	RECORD * recptr, *trptr;
	INDEX * FF;
	LFLIST * lfp;
	
	FF = getowner(hwnd);
	lfp = getdata(hwnd);
	switch (vk)	{
	
		case VK_LEFT:
			FORWARD_WM_HSCROLL(hwnd,SB_HORZ,SB_LINELEFT,0, view_proc);
			break;
		case VK_RIGHT:
			FORWARD_WM_HSCROLL(hwnd,SB_HORZ,SB_LINERIGHT,0, view_proc);
			break;
		case VK_PRIOR:
			FORWARD_WM_VSCROLL(hwnd,SB_VERT,SB_PAGEUP,0, view_proc);
			break;
		case VK_NEXT:
			FORWARD_WM_VSCROLL(hwnd,SB_VERT,SB_PAGEDOWN,0, view_proc);
			break;
		case VK_HOME:
		case VK_END:
			FORWARD_WM_VSCROLL(hwnd,SB_VERT,vk == VK_HOME ? SB_TOP : SB_BOTTOM,0, view_proc);
			break;
		case VK_ESCAPE:
			if (!FF->rwind)	// if no record window open
				view_clearselect(hwnd);
			break;
		case VK_TAB:
			if (GetKeyState(VK_SHIFT) < 0)	/* if shift tab */
				view_selectrec(FF,lfp->lines[0].rnum,VD_TOP,-1,-1);	/* select record at top of screen */
			else if (lfp->sel.first)			/* if have record selected */
				view_redisplay(FF,lfp->sel.first,VD_SELPOS);	/* bring into view */
			break;
		case VK_RETURN:
			if (lfp->sel.first)			/* if have record selected */
				mod_settext(FF,NOTEXT,lfp->sel.first,NULL);			
			break;
		case VK_UP:
		case VK_DOWN:
			if (lfp->sel.first)		{		/* if have record selected */
				if (vk == VK_UP)	{		/* if step backwards */
					if (recptr = rec_getrec(FF,lfp->selectdir <= 0 ? lfp->sel.first : lfp->sel.last))
						if (trptr = lfp->skip(FF,recptr,-1))	/* if there is a prev */
							recptr = trptr;
				}
				else if (recptr = rec_getrec(FF,lfp->selectdir >= 0 ? lfp->sel.last : lfp->sel.first))	/* if step forward */
					if (trptr = lfp->skip(FF,recptr,1))		/* if there's a next */
						recptr = trptr;
				if (recptr) {
						// trick: dummy selection to make sure record will be on screen
					view_selectrec(FF,recptr->num,vk == VK_UP ? VD_TOP : VD_BOTTOM,-1,0);
					if (GetKeyState(VK_SHIFT) < 0)	{	// if extending selection
						struct selstruct sel = v_statsel;
						if (vk == VK_UP)	{
							if (lfp->selectdir <= 0)	{		// continue up
								sel.first = recptr->num;
								sel.last = lfp->sel.last;
								lfp->selectdir = -1;
							}
							else {	// retreat
								sel.first = lfp->sel.first;
								sel.last = recptr->num;
								lfp->selectdir = 1;
							}
						}
						else	{
							if (lfp->selectdir >= 0)	{		// continue down
								sel.last = recptr->num;
								sel.first = lfp->sel.first;
								lfp->selectdir = 1;
							}
							else {
								sel.first = recptr->num;
								sel.last = lfp->sel.last;
								lfp->selectdir = -1;
							}
						}
						if (sel.first != sel.last)	{	// if range to select
							selectlines(hwnd,-1,TRUE,0,&sel);
							return;
						}
					}
					if (FF->rwind)	// if record window open
						mod_settext(FF,NOTEXT,recptr->num,hwnd);	// set and select
					else
						view_selectrec(FF,recptr->num,VD_TOP,-1,-1);
				}
			}
			else	/* none selected, so scroll */
				FORWARD_WM_VSCROLL(hwnd,SB_VERT,vk == VK_UP ? SB_LINEUP : SB_LINEDOWN,0, view_proc);
	}
}
/******************************************************************************/
static void lprint(HWND hwnd)		/* forms & prints text pages */

{
	DWORD flags;
	INDEX * FF;

	FF = getowner(hwnd);
	flags = view_recordselect(hwnd) ? PD_ENABLEPRINTTEMPLATE : PD_ENABLEPRINTTEMPLATE|PD_NOSELECTION;
	memset(&FF->pf,0,sizeof(PRINTFORMAT));	/* clear format info struct */
	if (print_begin(flags, FF->iname,&FF->head.formpars.pf.pi,hwnd) > 0)	{	/* if OK */
		FF->pf.first = 1;
		FF->pf.last = 9999;
		if (p_dlg.Flags&PD_PAGENUMS)	{		/* specified pages */
			FF->pf.last = p_dlg.nToPage;
			FF->pf.first = p_dlg.nFromPage;
		}
		FF->pf.pagenum = FF->head.formpars.pf.firstpage;
		formimages(hwnd,p_dlg.hDC, FALSE);
		print_end(FF->pf.pageout, hwnd);
		if (!FF->pf.pageout)	/* if did no pages */
			sendinfo(INFO_LASTPAGE,FF->pf.lastpage);
	}
}
/******************************************************************************/
static void formimages(HWND hwnd, HDC pdc, BOOL silent)	/* forms text pages; prints if necess */

{
	INDEX * FF = getowner(hwnd);
	RECN rnum;
	short pcount, lmargin, colcount;
	LFLIST * contextptr;
	RECT baserect, pagerect;
	short continflag, contlines, foffset, cltot;
	HCURSOR ocurs;
	int basecount = 0;
	
	if (contextptr = getmem(sizeof(LFLIST)))	{	/* if can get memory */
		LFLIST *lfp = getdata(hwnd);

		memcpy(contextptr,lfp, sizeof(LFLIST));	/* save context */
		p_dc = pdc;		// need this for text metrics
		lfp->printflag = TRUE;		/* TRUE when dc is for printing */
		if (pdc)	{			/* if printer dc available */
			settextinfo(hwnd,pdc);
			setddimensions(hwnd,FALSE);
			lfp->hdc = pdc;	/* use it */
		}
		lfp->sel.first = lfp->sel.last = 0;		/* printer sees no selection */
		baserect = lfp->drect;
		OffsetRect(&baserect,-FF->head.formpars.pf.pi.xoffset,-FF->head.formpars.pf.pi.yoffset);	/* offsets for unprintable area */
		pagerect.left = 0;		/* left pos is start of printable area */
		pagerect.right = FF->head.formpars.pf.pi.pwidthactual-2*FF->head.formpars.pf.pi.xoffset;	/* right is end of printable area */
		lfp->nlines = (lfp->drect.bottom-lfp->drect.top)/lfp->lheight;	/* # lines per page */
		if (lfp->nlines > SCREENBUFFSIZE)	/* if bigger than screen buffer */
			lfp->nlines = SCREENBUFFSIZE;	/* set limit */
		rnum = FF->pf.firstrec;
		lfp->stoprec = FF->pf.lastrec;	/* record that will block setlines */
		lfp->foffset = 0;	/* holds offset lines into first record displayed */
		continflag = FF->head.privpars.vmode == VM_FULL ? FF->head.formpars.pf.mc.pgcont : FALSE;
		if (silent)
			ocurs = SetCursor(g_waitcurs);
		for (contlines = foffset = FF->nostats = 0, p_err = pcount = 1; rnum && pcount <= FF->pf.last && !p_abort && p_err > 0; pcount++)	{		/* for all records */
//			if (!silent)
//				SendMessage(p_hwnd,WMM_PAGENUM,0,pcount);
			lmargin = FF->head.formpars.pf.mc.reflect && (pcount+FF->pf.pagenum)&1 ? FF->head.formpars.pf.mc.right : FF->head.formpars.pf.mc.left;
			lfp->drect = baserect;		/* reset drawing rect to first column */
			OffsetRect(&lfp->drect,lmargin,FF->head.formpars.pf.mc.top);		/* positions page rect for margins */
			lfp->vrect = lfp->drect;	/* set view to match destination (for clipping) */
			colcount = 0;
//			showprogress(PRG_PRINTING,FF->head.rtot,FF->pf.entries+basecount,pcount);	  /* send message */
			if (!silent)
				showprogress(PRG_PRINTING,FF->head.rtot,-1,pcount);	  /* send message */
			if (pcount == FF->pf.first)	{	/* if first page to produce [NB need to do before setting lines] */
				basecount = FF->pf.entries;	/* save current count for progress bar */
				FF->pf.entries=FF->pf.prefs=FF->pf.crefs=FF->pf.lines=FF->pf.pageout = 0;
				FF->pf.rnum = rnum;			/* record record number */
				FF->pf.offset = foffset;	/* and line offset */
			}
			do {
				if (continflag && contlines && (!colcount || FF->head.formpars.pf.mc.pgcont == RH_COL))	{	/* if want continuation headings and something carried over */
					FF->continued = TRUE;
					cltot = setlines(hwnd,0,-1,rnum,0);	/* install contin lines for record */
				}
				else		/* no contin */
					cltot = 0;
				setlines(hwnd,cltot,SHRT_MAX,rnum,foffset);
				foffset = fixbreak(hwnd,&rnum,&contlines);
				if (pcount >= FF->pf.first && (!FF->pf.oddeven || (FF->pf.oddeven&1) == (pcount&1)))	{	/* if not skipping the page */
					// for some reason need to set this before every page if rtl
					if (GetLayout(pdc)&LAYOUT_RTL)
						setdc(pdc,TRUE,FF->head.formpars.pf.pi.pwidthactual-FF->head.formpars.pf.mc.right);			/* sets mapping, etc */
					else
						setdc(pdc,FALSE,0);			/* sets mapping, etc */
					if (!silent)	{		/* if want real output */
						if (!colcount)	{		/* if first column */
							if ((p_err = StartPage(lfp->hdc)) <= 0)
								break;
							pr_headfoot(FF,lfp->hdc,TRUE,pcount+FF->pf.pagenum-1,&pagerect);	/* put in header */
						}
						drawlines(hwnd,0,lfp->nlines);
						OffsetRect(&lfp->drect,FF->head.formpars.pf.mc.gutter+lfp->width,0);	/* shift destination rect */
						if (colcount == FF->head.formpars.pf.mc.ncols-1)	{	/* if done last col */
							pr_headfoot(FF,lfp->hdc,FALSE,pcount+FF->pf.pagenum-1,&pagerect);	/* put in footer */
							if ((p_err = EndPage(lfp->hdc)) <= 0)	/* close page */
								break;
						}
					}
					if (!colcount)	/* if first column */
						FF->pf.pageout++;	/* count a page that would be output */
				}
			} while (++colcount < FF->head.formpars.pf.mc.ncols);
		}
		FF->pf.lastrnum = rnum;
		FF->pf.lastpage = pcount-1;	/* last page formatted */
		memcpy(lfp,contextptr, sizeof(LFLIST));	/* restore context */
		freemem(contextptr);
		if (silent)	
			SetCursor(ocurs);
		else
			showprogress(0,0,0);	  /* clear message */
	}
}
/******************************************************************************/
void view_formsilentimages(HWND hwnd)		/* formats pages silently */

{
	HDC dc = print_getic();

	formimages(hwnd,dc,TRUE);
	if (dc)		/* if actually had a printer */
		DeleteDC(dc);
}
/****************************************************************************/
static short fixbreak(HWND hwnd, RECN * rnum, short *continuedlines)		/* fixes break & continuation */

{
	INDEX * FF;
	RECORD * recptr;
	short foffset, hlevel, residue, sprlevel,hidelevel,clevel;
	DLINE * lptr, *tlptr;
	LFLIST * lfp;

	lfp = getdata(hwnd);
	residue = foffset = lfp->foffset; 	/* FF->nostats stops duplicate measure by measurelines */
	FF = getowner(hwnd);
	FF->nostats = lfp->widows;
	lptr = &lfp->lines[lfp->nlines-1];	/* ptr to last line */
	*rnum = lptr->rnum;		/* save number of last record */
	if (recptr = rec_getrec(FF,*rnum))	{
		if (*continuedlines = lfp->widows)	{	/* if entry continues */
			if (lptr->lnum < lfp->nlines)	{	/* if first line of record is on page */
				rec_uniquelevel(FF,recptr,&hlevel,&sprlevel,&hidelevel,&clevel);		/* find where record is unique */
				for (tlptr = lptr-lptr->lnum; !tlptr->count && tlptr <= lptr; tlptr++)	/* count empty lines */
					;
				if (tlptr <= lptr)	{		/* if not just blank lines up to break, test for shift */
#ifdef PUBLISH
					if ((lptr->indent&FO_HMASK) < lfp->maxindent && !(lptr->flags&VFORCERUN))	/* if not at lowest-level or run-on heading */
#else
					if ((lptr->indent&FO_HMASK) < lfp->maxindent)	/* if not at lowest-level or run-on heading */
#endif //PUBLISH
						residue = 0;		/* leave nothing behind */
					else if (*continuedlines == 1)	/* if dangling continuation */
						residue = foffset-1;		/* move a complete line to next page */
					if (residue <= 2)		/* if would leave < 2 lines behind */
						residue = 0;		/* push whole record to next page */
					while (foffset > residue)		{		/* move lines */	
						(lptr--)->rnum = 0;
						foffset--;
					}
				}
				if (!hlevel && (!residue || tlptr > lptr))	/* if main head and no residue or blank one */
					*continuedlines = 0;	/* inhibit "continued" message */
			}
		}
		else if (recptr = lfp->skip(FF,recptr,1))	{	/* complete entry on this page, check next */
			rec_uniquelevel(FF,recptr,continuedlines,&sprlevel,&hidelevel,&clevel);
			*rnum = recptr->num;
			foffset = 0;
		}
		else		/* no record follows */
			*rnum = 0;
	}
	return (foffset);
}
/******************************************************************************/
static void lsetpage(HWND hwnd)		/* sets up after page setup */

{
	INDEX * FF;

	FF = getowner(hwnd);
	if (print_setpage(&FF->head.formpars.pf.mc,&FF->head.formpars.pf.pi))
		view_changedisplaysize(hwnd);
}
/******************************************************************************/
void view_changedisplaysize(HWND hwnd)		/* sets up after page/margin change */

{
	setddimensions(hwnd,TRUE);
	view_redisplay(getowner(hwnd),0,VD_CUR|VD_RESET);
	checkvscroll(hwnd);		/* scales & activates as necess */
	checkhscroll(hwnd);		/* scales & activates as necess */
}
/*******************************************************************************/
static void resetdisplay(INDEX * FF)	/* resets display */

{	
	view_setstatus(FF->vwind);		/* set title */
	view_clearselect(FF->vwind);	/* clears selection */
	view_redisplay(FF,0,VD_TOP|VD_RESET|VD_IMMEDIATE);
	ldoviewbuttons(FF);	/* set buttons */
}
/*******************************************************************************/
RECN view_selectrec(INDEX * FF, RECN rnum, int posflag, short startoffset, short length)		/* selects record */
	// if length = 0, just puts record on screen
{
	short startline, vislines, lcount;
	LSET larray[LLIMIT];		/* holds offsets of starts of lines */
	RECORD * curptr;
	HWND hwnd;
	RECN pnum, fnum;
	struct selstruct selpars;
	int top, scrollcount;
	LFLIST * lfp;
	
	hwnd = FF->vwind;
	lfp = getdata(hwnd);
	if (curptr = lfp->getrec(FF, rnum))	{		/* get the record (or parent, if formatted) */
		rnum = curptr->num;						/* ensure match in case getrec changes record */
		vislines = lfp->nlines;
		if (top = isobscured(FF->cwind,g_mdlg))	{	/* if any of it is obscured */
			vislines = (top-lfp->vrect.top)/lfp->lheight;
			if (vislines < STARGETLINE)	/* if not enough clear lines above obstruction */
				vislines = STARGETLINE;	/* pretend anyway */
		}
		else if (FF->rwind && !FF->rcwind)	// set special scroll pos
			vislines -= SBOTTOMGAP;
		selpars = v_statsel;			/* initialize selection struct */
		selpars.first = selpars.last = rnum;
		if (FF->head.privpars.vmode != VM_FULL)	{	/* if a draft mode */
			selpars.startpos = u8_countU(curptr->rtext,startoffset);			/* set up selection pars */
			selpars.length = u8_countU(curptr->rtext+startoffset,length);
		}
		lcount = lfp->measurelines(hwnd, lfp->hdc,curptr, larray,&selpars);	/* set up selection info & count lines */
		rec_getprevnext(FF,curptr,&pnum, &fnum, lfp->skip);		/* get previous & following records */
		for (startline = 0; startline < lfp->nlines; startline++)	{	/* for all lines on screen */
			if (lfp->lines[startline].rnum == rnum)		/* if our record */
				break;
		}
		if (!startline || startline == lfp->nlines && fnum && lfp->lines[0].rnum == fnum)	{	/* on or right before first line */
			scrollcount = lfp->lines[0].lnum + (lfp->lines[0].rnum == rnum ? 0: lcount);	/* just residue of this, or this +prev */
			if (scrollcount < lfp->nlines)	{	/* if need to scroll less than screenful */
				linedown(hwnd,scrollcount);	
				setvscroll(hwnd);
			}
			else		/* just redisplay at right position */
				view_redisplay(FF,rnum,posflag|VD_IMMEDIATE);	/* set new screen */
		}
		else if (startline < lfp->nlines && startline+lcount >= vislines || startline == lfp->nlines
				&& pnum && lfp->lines[lfp->nlines-1].rnum == pnum)	{	/* if on screen and some below vis limit, or next off screen */
			if (lcount >= vislines)		/* if there's more of the record than we can display */
				lcount = vislines-1;	/* limit the display */
			scrollcount = lcount-(vislines-startline)+1 + (lfp->lines[lfp->nlines-1].rnum == pnum ? lfp->widows : 0);
			if (scrollcount < lfp->nlines)	{	/* if scrolling less than screenful */
				lineup(hwnd,scrollcount);		/* push previous record */
				setvscroll(hwnd);
			}
			else		/* just redisplay at right position */
				view_redisplay(FF,rnum,posflag|VD_IMMEDIATE);	/* set new screen */
		}
		else if (startline == lfp->nlines)	/*  not on or adjacent to screen */
			view_redisplay(FF,rnum,posflag|VD_IMMEDIATE);	/* set new screen */
		if (length)	{	// if want actual highlight
			selectlines(hwnd,-1,FALSE,0,&selpars);	/* select part of our rec that's on screen */
			lfp->selectdir = 0;
		}
	}
	return (rnum);
}
/*******************************************************************************/
void view_appendrec(INDEX * FF, RECN rnum)		/* displays record on first empty screen line */

{
	short count, lcount;
	HWND hwnd;
	
	for (hwnd = FF->vwind, count = 0; count < LX(hwnd,nlines); count++)	{	/* for all lines on screen */
		if (!LX(FF->vwind,lines)[count].rnum)	{			/* if blank line */
			lcount = setlines(hwnd,count,SHRT_MAX,rnum,0);	/* set line array for the record */
			drawlines(hwnd,count, lcount);		/* draw its lines */
			break;
		}
	}
}
/*******************************************************************************/
void view_resetrec(INDEX * FF, RECN rnum)		/* redisplays record that's on screen */

{
	short vcount, first, last;
	HWND hwnd;
	RECT trect;
	DLINE lines[SCREENBUFFSIZE];	/* array of line structs */
	RECORD * recptr;
	LFLIST * lfp;
	
	hwnd = FF->vwind;
	lfp = getdata(hwnd);
	if (recptr = lfp->getrec(FF,rnum))	{	/* if can get record (or parent) */
		if (vcount = linesonscreen(hwnd,recptr->num,&first))	{	/* if on screen */
			memcpy(lines,lfp->lines,lfp->nlines*sizeof(DLINE));	/* duplicate the current lines array */
			setlines(hwnd,first,SHRT_MAX,recptr->num,lfp->lines[first].lnum);	/* reset line array below this rec */
			for (last = lfp->nlines-1; last > first; last--)
				if (memcmp(&lines[last],&lfp->lines[last], sizeof(DLINE)) || lines[last].rnum == rnum)	/* if linestructs differ, or it's current record */
					break;
			trect = lfp->vrect;
			trect.top = lfp->lheight * first;
			trect.bottom = (++last)*lfp->lheight;
			InvalidateRect(hwnd,&trect,TRUE);
		}
		else	/* record not on screen; only known cause is editing single duplicated record in full format */
			view_selectrec(FF,recptr->num,VD_MIDDLE,-1,-1);
		return;
	}
	senderr(ERR_INTERNALERR,WARN,"no record reset");
}
/*********************************************************************************/
void view_redisplay(INDEX * FF, RECN rnum, short flags)	/*  redisplays view window */

{
	HWND hwnd = FF->vwind;
	LFLIST * lfp = getdata(hwnd);
	struct selstruct selpars = v_statsel; // prevent finding selection
	RECORD * recptr;
	short lcount, targetline, lintot, lpos, foffset, i, newcount, newlast;
	LSET larray[LLIMIT];			/* holds offsets of starts of lines */
	
	if (flags&VD_RESET || numdigits(FF->head.rtot) > lfp->ndigits)	/* to cope with adding/reading records */
		setpars(hwnd);			/* reset bounds, etc. */
	if (hwnd)	{
		foffset = 0;					/* default display from start of specified record */
		if (!rnum)	{					/* if no top record specified */
			if (flags&VD_CUR && lfp->lines[0].rnum)		{	/* want redisplay from current top record */
				rnum = lfp->lines[0].rnum;
				foffset = lfp->lines[0].lnum;		/* offset of first displayed line */
			}
			else if (recptr = sort_top(FF))		/* from top of index */
				rnum = recptr->num;
		}
		if (FF->head.privpars.vmode == VM_FULL)	{	// if full format, check for visible
			if (recptr = form_getprec(FF,rnum)) {	// find parent if necessary
				if (sort_isignored(FF,recptr))		// if can't display
					recptr = sort_skip(FF,recptr,1);	// find first displayable
			}
			rnum = recptr ? recptr->num : 0;
		}
		lfp->fillEnabled = TRUE;	// allow counting consumed records
		lfp->filledRecords = 0;
		lfp->filledLines = 0;
		if (rnum && (lcount = setlines(hwnd,0, SHRT_MAX,rnum,foffset)))	{		/* if anything to display */
			lfp->fillEnabled = FALSE;	// disable counting consumed records
			lintot = lfp->nlines;
			lpos = 0;
			if (flags&(VD_MIDDLE|VD_BOTTOM|VD_SELPOS) || lcount < lintot && !(flags&VD_TOP))		{	/* if record to middle/bottom of screen, or empty space below */
				if (flags&(VD_MIDDLE|VD_BOTTOM|VD_SELPOS))	{	/* if want to show somewhere other than top */
					if (flags&VD_BOTTOM)	{		// set to ensure that last line of record is displayed
						while (lfp->lines[lpos].rnum == rnum)	/* count lines in top record */
							lpos++;
						targetline = lintot-SBOTTOMGAP-lpos;
					}
					else if (flags&VD_MIDDLE)
						targetline = lcount-(lintot >>1);	/* in this case defines line for record end */
					else
						targetline = STARGETLINE;
				}
				else if (lcount < lintot)	{	/* else if empty screen to fill */
					while (lfp->lines[lpos].rnum == rnum)	/* count lines in top record */
						lpos++;
					targetline = lintot-lcount+lpos;	/* position to fill gap */
				}
				while (lpos < targetline && (recptr = rec_getrec(FF,rnum)))	{	/* while our record ends above target */
					if (!(recptr = lfp->skip(FF, recptr, -1)))
						break;				/* can't go further */
					newcount = lfp->measurelines(hwnd, lfp->hdc,recptr, larray,&selpars);	/* measure lines */
					lpos += newcount;
					if (lpos >= targetline)		/* if too many lines in this record */
						newcount -= lpos-targetline;	/* reduce number to fill */
					rnum = recptr->num;
					newlast = lintot-1-newcount;		/* line that will be last on screen */
					if (lfp->lines[newlast].rnum != lfp->lines[lintot-1].rnum)	{	/* if it's not current last record */
						for (lfp->widows = 0; lfp->lines[newlast].rnum == lfp->lines[newlast+1].rnum; newlast++)	/* count new widows */
							lfp->widows++;
					}
					else if (lfp->lines[newlast].rnum)	/* if not blank line */
						lfp->widows += newcount;	/* add widows to current last record */
					else
						lfp->widows = 0;
					for (i = lintot-1; i-newcount >= 0; i--)	/* push down the existing line set */
						lfp->lines[i] = lfp->lines[i-newcount];
					setlines(hwnd,0, newcount, rnum, lpos < targetline ? 0 : lpos-targetline);	/* fill gap */
				}
			}
		}
		else		/* nothing to display, so clear array */
			memset(&lfp->lines,0,sizeof(lfp->lines));
		lfp->fillEnabled = FALSE;	// disable counting consumed records
		setscrollrange(FF->vwind);		/* check/set scroll range */
		InvalidateRect(hwnd,NULL,TRUE);
	}
}
/*******************************************************************************/
void view_updatestatus(HWND hwnd, BOOL marginonly) /* updates status display for selected records */

{
	short fline, lline, line;
	RECT srect;
	LFLIST * lfp;
	
	lfp = getdata(hwnd);
	for (fline = -1, line = 0; line < lfp->nlines; line++)		{	/* for all lines with records */
		if (lfp->lines[line].flags&VSELECT)	{	/* if a selected row */
			if (fline < 0)			/* if first selected row */
				fline = line;		/* mark it */
			lline = line;
		}
		else if (fline >= 0)	/* if first after selection */
			break;
	}
	if (fline >= 0)	{		/* if something selected */
		srect.top = fline*lfp->lheight;		/* set top */
		srect.left = lfp->drect.left;
		srect.bottom = (lline+1)*lfp->lheight;	/* and bottom */
		if (marginonly)	/* if want margin change only */
			srect.right = lfp->drect.left+lfp->nspace;
		else
			srect.right = lfp->drect.right;
		InvalidateRect(hwnd,&srect,TRUE);
	}
}
/*******************************************************************************/
void view_clearselect(HWND hwnd) /* clears selection */

{
	selectlines(hwnd,-1,FALSE,0, &v_statsel);
}
/*******************************************************************************/
void view_setstatus(HWND hwnd)	/* sets window title and status */

{
	INDEX * FF = getowner(hwnd);
	char tstring[STSTRING];
	int sflag;
	TCHAR sortstring[100];

	switch (FF->viewtype)	{
		case VIEW_ALL:
			sprintf(tstring, "All Records (%ld)",FF->head.rtot);
			sflag = FF->head.sortpars.ison;
			break;
		case VIEW_GROUP:
			sprintf(tstring, "Group \"%s\" (%ld of %ld)",FF->curfile->gname, FF->curfile->rectot,FF->head.rtot);
			sflag = FF->curfile->lg.sortmode;
			break;
		case VIEW_TEMP:
			sprintf(tstring, "Temporary Group (%ld of %ld)",FF->lastfile->rectot,FF->head.rtot);
			sflag = FF->lastfile->lg.sortmode;
			break;
		case VIEW_NEW:
			sprintf(tstring, "New Records (%ld of %ld)",FF->head.rtot-FF->startnum,FF->head.rtot);
			sflag = FALSE;
			break;
	}
	SendMessage(g_hwstatus,SB_SETTEXT,STATSEG_RECTOT|0,(LPARAM)toNative(tstring));
	sprintf(tstring,"%ld New", FF->head.rtot-FF->startnum);
	SendMessage(g_hwstatus,SB_SETTEXT,STATSEG_NEW|0,(LPARAM)toNative(tstring));
#if 0
	if (iswordsort(FF->head.sortpars.type))
		sortstring = TEXT("Sorted (Word)");
	else if (islettersort(FF->head.sortpars.type))
		sortstring = TEXT("Sorted (Letter)");
	else
		sortstring = TEXT("Sorted (Simple)");
#else
	if (sflag)
		wsprintf(sortstring,TEXT("Sorted (%s)"),cl_sorttypes[FF->head.sortpars.type]);
	else
		nstrcpy(sortstring,TEXT("Unsorted"));
#endif
	SendMessage(g_hwstatus,SB_SETTEXT,STATSEG_SORT|0,(LPARAM)sortstring);
	SendMessage(g_hwstatus,SB_SETTEXT,STATSEG_SEARCH|0,(LPARAM)toNative(LX(hwnd,searchstring)));
	SendMessage(g_hwstatus,SB_SIMPLE,FALSE,0);	/* restore multi-part status window */
}
/*********************************************************************************/
static void setpars(HWND hwnd)	/*  sets up display attributes */

{
	INDEX * FF = getowner(hwnd);
	LFLIST *lfp = getdata(hwnd);

	lfp->hdc = GetDC(hwnd);
	settextinfo(hwnd,lfp->hdc);
	if (FF->head.privpars.vmode != VM_FULL)	{	/* for any draft mode */
		lfp->measurelines = draft_measurelines;
		lfp->disp = draft_disp;
		lfp->skip = draft_skip;
		lfp->getrec = rec_getrec;
	}
	else	{
		FF->stylecount = str_xparse(FF->head.stylestrings,FF->slist);	/* parse styled strings */
		lfp->measurelines = form_measurelines;
		lfp->disp = form_disp;
		lfp->skip = form_skip;
		lfp->getrec = form_getprec;
	}
	sort_setfilter(FF,SF_VIEWDEFAULT);
	InvalidateRect(hwnd,NULL,TRUE);	/* ensure whole of old rect cleared, cause new one might not be same size */
	setvdimensions(hwnd);		/* get rest of parameters */
	ReleaseDC(hwnd,lfp->hdc);
}
/*********************************************************************************/
static void settextinfo(HWND hwnd, HDC dc)		/* sets up page/screen text params */

{
	TCHAR * spacebuf = TEXT("888888888");
	INDEX * FF = getowner(hwnd);
	LFLIST * lfp = getdata(hwnd);
	int layout = 0;
	char tstring[20];
	TEXTMETRIC tmetric;
	SIZE dim;
	
	if (FF->head.formpars.pf.alignment == 0)	{	// if natural alignment
		int direction = col_getLocaleInfo(&FF->head.sortpars)->direction;
		if (direction == ULOC_LAYOUT_RTL)
			layout = LAYOUT_RTL;
	}
	else if (FF->head.formpars.pf.alignment == 2)	// right
		layout = LAYOUT_RTL;
	FF->righttoleftreading = layout == LAYOUT_RTL;
	SetLayout(dc, layout);
	SetTextAlign(dc,TA_BASELINE|TA_UPDATECP);
	type_setfont(dc,FF->head.fm[0].name,FF->head.privpars.size,0);	
	GetTextMetrics(dc,&tmetric);
	lfp->lspace = tmetric.tmHeight+tmetric.tmExternalLeading;
	lfp->lheight = FF->head.formpars.pf.autospace ? lfp->lspace : FF->head.formpars.pf.lineheight;
	if (FF->head.formpars.pf.linespace == FS_ONEANDHALF)	{	/* 1.5 line spacing */
		lfp->lheight *= 3;
		lfp->lheight /= 2;
	}
	else if (FF->head.formpars.pf.linespace == FS_DOUBLE)	/* double spacing */
		lfp->lheight *= 2;
	GetCharWidth32(dc,FSPACE,FSPACE,&lfp->cwidth);	// get width of figure space
	GetCharWidth32(dc,EMSPACE,EMSPACE,&lfp->emwidth);	// get width of em space
	lfp->empoints = MulDiv(lfp->emwidth, 72,dpiforwindow(FF->vwind));	// 1/12/2022 adjust to take account of monitor not being 72 dpi
	lfp->indent = lfp->emwidth;
	lfp->linedrop = tmetric.tmAscent+tmetric.tmExternalLeading;	/* move to start posn in window */
	lfp->ndigits = sprintf(tstring,"%lu", FF->head.rtot);
	GetTextExtentPoint32(dc,spacebuf,3+(FF->head.privpars.shownum ? lfp->ndigits: 0),&dim);
	lfp->nspace = FF->head.privpars.vmode == VM_FULL ? 0 : dim.cx;
	SetBkMode(dc,TRANSPARENT);
}
/*********************************************************************************/
static void setvdimensions(HWND hwnd)	/*  sets up view rect dimensions, etc */

{
	RECT trect;
	LFLIST * lfp;
			
	GetClientRect(hwnd,&trect);
	lfp = getdata(hwnd);
	lfp->nlines = (trect.bottom-trect.top + lfp->lheight-1)/lfp->lheight;
	if (lfp->nlines > SCREENBUFFSIZE)		/* if max lines greater than buffer */
		lfp->nlines = SCREENBUFFSIZE;
	lfp->vrect = trect;
#if 1
	if (GetLayout(lfp->hdc)&LAYOUT_RTL)		{	// if ltr, drect always aligned with right edge of vrect
		SetLayout(lfp->hdc,0);		// no idea why this is what's needed to get aligment right on size change
		SetLayout(lfp->hdc,LAYOUT_RTL);
	}
#endif
}
/******************************************************************************/
static void setddimensions(HWND hwnd, BOOL screen)	/*  sets up dest rect dimensions, etc */

{
	INDEX * FF = getowner(hwnd);
	LFLIST * lfp = getdata(hwnd);
	RECT drect;
	
	getpagerect(&drect,FF);		/* gets display rect adjusted for margins */
	lfp->width = (drect.right-drect.left-(FF->head.formpars.pf.mc.ncols-1)*FF->head.formpars.pf.mc.gutter)/FF->head.formpars.pf.mc.ncols;
	if (screen)	{		/* if screen */
		scalerect(&drect,g_slpix,72);		/* scale for screen pixels */
		lfp->width  = (lfp->width*g_slpix)/72;
		OffsetRect(&drect,DESTINSET,0);		/* add offset */
	}
	lfp->drect = drect;		/* set destination rect */
}
/******************************************************************************/
static void getpagerect(RECT * drect, INDEX * FF)	/* sets destination rect for display */

{
	drect->left = drect->top = 0;
	drect->right = FF->head.formpars.pf.pi.pwidthactual-(FF->head.formpars.pf.mc.right+FF->head.formpars.pf.mc.left);
	drect->bottom = FF->head.formpars.pf.pi.pheightactual-(FF->head.formpars.pf.mc.top+FF->head.formpars.pf.mc.bottom);
}
/******************************************************************************/
static short setlines(HWND hwnd, short fline, short maxcount, RECN rnum, short firstoffset)	/* sets up new line table from record rnum */

{
	register short i, lcount, loadcount;
	short totlines, screenlines, lindex, numline, sflag;
	RECORD * rptr;
	LSET lineset[LLIMIT], *j;	/* line offsets for breaks in single record */
	DLINE * lptr;
	INDEX * FF;
	struct selstruct selpars;
	LFLIST * lfp;

	FF = getowner(hwnd);
	lfp = getdata(hwnd);
	selpars = v_statsel;		/* prevent finding selection */
	lptr = lfp->lines;
	screenlines = lfp->nlines;
	for (loadcount = totlines = 0, i = fline, rptr = rec_getrec(FF,rnum); rptr && rptr->num != lfp->stoprec && i < screenlines && totlines != maxcount; rptr = lfp->skip(FF,rptr,1))	{	/* for as many lines as can be displayed */
		lcount = lfp->measurelines(hwnd, lfp->hdc, rptr, lineset,&selpars);		/* set up lines */
		numline = 0;
		if (FF->head.privpars.vmode == VM_DRAFT || FF->head.privpars.vmode == VM_SUMMARY)	{	/* if draft or summary */
			for (lindex = 0; lindex < lcount-1; lindex++) 	{
				if ((lineset[lindex+1].indent&FO_HMASK) > (lineset[lindex].indent&FO_HMASK))	/* if next line is new subhead or this line is a blank */
					numline = lindex+1;
			}
		}
		if (lfp->sel.first && sort_relpos(FF,rptr->num,lfp->sel.first) >= 0 && 
			 sort_relpos(FF,rptr->num,lfp->sel.last) <= 0)	/* if selected */
			sflag = VSELECT;
		else
			sflag = FALSE;
		for (loadcount = 0, j = lineset; loadcount < lcount && i < screenlines && totlines != maxcount; loadcount++)		{	/* while can add lines */
			lptr[i].rnum = rptr->num;		/* install record num */
			lptr[i].flags = loadcount == numline ? VNUMLINE : FALSE;	/* tag line */
			if (j->indent&FO_FORCERUN)		/* if forced runover in entry */
				lptr[i].flags |= VFORCERUN;
#ifdef PUBLISH
			if (j->indent&FO_LABELED)		/* if labeled line */
				lptr[i].flags |= VLABELED;
#endif //PUBLISH
			if (sflag)	{					/* if in selection range */
				lptr[i].flags |= sflag;		/* presume line selected */
				lptr[i].selstart = lptr[i].selend = -1;		/* and whole line */
				if (rptr->num == lfp->sel.first)	{
					if (loadcount < lfp->sel.sline)		/* if before first line */
						lptr[i].flags &= ~sflag;			/* clear select */
					if (loadcount == lfp->sel.sline)	/* if on first line */
						lptr[i].selstart = lfp->sel.shighlight;	/* set it */
				}
				if (rptr->num == lfp->sel.last)	{		/* last record */
					if (loadcount > lfp->sel.eline)		/* if after last line */
						lptr[i].flags &= ~sflag;			/* clear select */
					if (loadcount == lfp->sel.eline)	/* if on last line */
						lptr[i].selend = lfp->sel.ehighlight;	/* set it */
				}
			}
			lptr[i].lnum = loadcount;	/* line index into record */
			lptr[i].indent = j->indent;
			lptr[i].freespace = j->free;		/* set width of right-justified locator */
			lptr[i].base = (j++)->offset;		/* install offset */
			lptr[i].count = j->offset -(j-1)->offset;		/* and char count */
			if (firstoffset > 0)	{	/* if want to get rid of part of first record */
				firstoffset--;			/* will overwrite lines until get to right place */
				continue;
			}
			if (!totlines && lfp->printflag && !FF->typesetter && lptr[i].indent == FO_BLANK)	/* if would print blank lines at top of column */
				continue;
			i++;
			totlines++;
		}
		if (maxcount < 0)	/* if just wanted this record */
			maxcount = totlines;	/* force break */
	}
	if (i == screenlines)	{		/* if loaded last line */
		lfp->foffset = loadcount;	/* number of lines loaded in last record */
		lfp->widows = lcount-loadcount;		/* number remaining to be displayed */
		lfp->maxindent = lineset[lcount-1].indent&FO_HMASK;	/* lowest level formatted heading */
	}
	else if (totlines != maxcount) {	/* didn't write all the lines requested */
		while (i < screenlines)		/* while lines to clear */
			lptr[i++].rnum = 0;		/* set null */
		lfp->widows = 0;		/* no widows */
	}
	return (totlines);	/* number of lines loaded */
}
/*******************************************************************************/
static void drawlines(HWND hwnd, short first, short count)	/* draws count lines from pos first */

{
	short tot;
	LFLIST * lfp;

	lfp = getdata(hwnd);
	for (tot = 0; tot < count && lfp->lines[first+tot].rnum;)
		tot += lfp->disp(hwnd,lfp->hdc,first+tot,count-tot);	/* draw it */
}
#if 0
/*******************************************************************************/
static void setscrollrange(HWND hwnd)	/* sets range of scroll bar */

{
	LFLIST * lfp = getdata(hwnd);
	INDEX * FF;
	SCROLLINFO si;

	FF = getowner(hwnd);
	switch (FF->viewtype)	{
	
		case VIEW_ALL:
			si.nMax = FF->head.rtot;	/* records spanned by vertical bar */
			break;
		case VIEW_GROUP:
			si.nMax = FF->curfile->rectot;	/* records spanned by vertical bar */
			break;
		case VIEW_TEMP:
			si.nMax = FF->lastfile->rectot;	/* records spanned by vertical bar */
			break;
		case VIEW_NEW:
			si.nMax = FF->head.rtot-FF->startnum;	/* records spanned by vertical bar */
			break;
	}
	si.cbSize = sizeof(SCROLLINFO);
	si.fMask = SIF_RANGE|SIF_DISABLENOSCROLL;
	si.nMin = 1;
	SetScrollInfo(hwnd,SB_VERT,&si,TRUE);	/* sets new range */
//	lfp->vrange = si.nMax < 2*lfp->nlines-1 ? 2*lfp->nlines-1 : si.nMax;
	lfp->vrange = si.nMax < 2*lfp->nlines-1 ? lfp->nlines+si.nMax-1 : si.nMax;
	lfp->wheelresidue = 0;
}
/*********************************************************************************/
static void checkvscroll(HWND wptr)	/* checks range of scroll bar */

{
	LFLIST *lfp = getdata(wptr);
	RECN lastrec;
	unsigned long rcount;
	short lcount;
	SCROLLINFO si;
	
	if (TRUE)	{	/* if window is active */
		for (lastrec = rcount = lcount = 0; lfp->lines[lcount].rnum && lcount < lfp->nlines; lcount++)		{/* for all lines on screen */
			if (lastrec != lfp->lines[lcount].rnum)		{	/* if a new record */
				rcount++;							/* count a record */
				lastrec = lfp->lines[lcount].rnum;
			}
		}
		if (lfp->vrange > rcount || lfp->widows || rec_number(sort_top(getowner(wptr))) != lfp->lines[0].rnum)	{	/* if display can't all fit */
			setvscroll(wptr);		/* set thumb */
			si.nMax = lfp->vrange;
		}
		else	/* all fits in height */
			si.nMax = 1;		/* min and max are same, therefore disable */
//		si.nPage = lcount > 1 ? (rcount*lfp->nlines)/(lcount-1) : 0;	/* 'page' is number of records that fit on screen */
//		si.nPage = lcount ? (rcount*lfp->nlines)/lcount : 0;	/* 'page' is number of records that fit on screen */
		si.nPage = lcount < lfp->nlines ? lfp->nlines : rcount;	/* 'page' is number of records that fit on screen */
		si.cbSize = sizeof(SCROLLINFO);
		si.fMask = SIF_RANGE|SIF_PAGE|SIF_DISABLENOSCROLL;
		si.nMin = 1;
		SetScrollInfo(wptr,SB_VERT,&si,TRUE);	/* sets new range */
	}
}
/*********************************************************************************/
static void setvscroll(HWND hwnd)	/* sets vertical scroll bar posn */

{
	SCROLLINFO si;
	
	si.cbSize = sizeof(SCROLLINFO);
	si.nPos = sort_findpos(getowner(hwnd), LX(hwnd,lines)[0].rnum);	/* find position of rec & top of screen */
	si.fMask = SIF_POS;
	SetScrollInfo(hwnd,SB_VERT,&si,TRUE);	/* sets new range */
}
/*******************************************************************************/
static void trackvscroll(HWND hwnd, HWND hctl, UINT code, int pos)	/* displays repetitively while mousdown in scrollbar */

{
	SCROLLINFO si;
	RECORD * curptr;
	INDEX * FF;

	FF = getowner(hwnd);
	switch (code)	{			/* which bit hit */
		case SB_THUMBTRACK:
			si.cbSize = sizeof(SCROLLINFO);
			si.fMask = SIF_POS|SIF_RANGE|SIF_TRACKPOS|SIF_PAGE;
			GetScrollInfo(hwnd,SB_VERT,&si);
			if (si.nTrackPos != LX(hwnd,lastthumbpos))	{	// if not same pasition as last time
				if (si.nTrackPos == si.nMin)		/* if to top */
					curptr = sort_top(FF);
				else if (si.nTrackPos+si.nPage-1 == si.nMax)	/* or bottom */
					curptr = sort_bottom(FF);
				else
					curptr = sort_jump(FF,si.nTrackPos);	/* jump to right ordered position */
				if (curptr)
					curptr = LX(hwnd,getrec)(FF,curptr->num);	/* fix for formatted view */
				if (curptr)	{		// if have record && not already in right place
					setlines(hwnd,0,SHRT_MAX,curptr->num,0);
					invalidateclientrect(hwnd);
					LX(hwnd,lastthumbpos) = si.nTrackPos;
					si.fMask = SIF_POS;
					si.nPos = si.nTrackPos;
					SetScrollInfo(hwnd,SB_VERT,&si,TRUE);	/* sets new range */
				}
			}
			return;
		case SB_LINEUP:
			linedown(hwnd,1);		/* move back one line */
			break;
		case SB_LINEDOWN:
			lineup(hwnd,1);		/* move on one line */
			break;
		case SB_PAGEUP:
			pageup(hwnd);
			break;
		case SB_PAGEDOWN:
			pagedown(hwnd);
			break;
		case SB_TOP:
		case SB_BOTTOM:
			curptr = code == SB_TOP ? sort_top(FF) : sort_bottom(FF);
			if (curptr)		{	/* if got top or bottom */
				setlines(hwnd,0,SHRT_MAX,curptr->num,0);
				invalidateclientrect(hwnd);
			}
			break;
		default:
			return;
	}
	setvscroll(hwnd);			/* set thumb position */
}
#else
/*******************************************************************************/
static void setscrollrange(HWND hwnd)	/* sets range of scroll bar */

{
	LFLIST * lfp = getdata(hwnd);
	INDEX * FF = getowner(hwnd);
	SCROLLINFO si;
	RECORD * curptr;
	RECN visible = 0;

//	NSLog("Start");
	for (curptr = sort_top(FF); curptr; curptr = sort_skip(FF,curptr,1))
		visible++;
	lfp->visibleRecords = visible;
//	NSLog("End");
	si.cbSize = sizeof(SCROLLINFO);
	si.nPage = PAGEMUL*(lfp->vrect.bottom-lfp->vrect.top)/lfp->lheight;
	si.nMax = si.nPage;	// scrollable height is min 1 page
	if (lfp->filledLines)
		si.nMax += PAGEMUL*visible*lfp->filledLines/lfp->filledRecords;	// line is proxy for height unit
	si.fMask = SIF_PAGE|SIF_RANGE|SIF_DISABLENOSCROLL;
	si.nMin = 1;
	SetScrollInfo(hwnd,SB_VERT,&si,TRUE);	/* sets new range */
	lfp->wheelresidue = 0;
	setvscroll(hwnd);
//	NSLog("Visible %d; filledLines: %d, filledRecord:%d",visible,lfp->filledLines,lfp->filledRecords);
//	NSLog("SetScrollRange [%d, %d]",si.nMax,si.nPage );
}
/*********************************************************************************/
static void checkvscroll(HWND wptr)	/* sets size of page */

{
	LFLIST *lfp = getdata(wptr);
	SCROLLINFO si;
	
//	si.nPage = lfp->nlines;
	si.nPage = PAGEMUL*(lfp->vrect.bottom-lfp->vrect.top)/lfp->lheight;
	si.cbSize = sizeof(SCROLLINFO);
	si.fMask = SIF_PAGE|SIF_DISABLENOSCROLL;
	SetScrollInfo(wptr,SB_VERT,&si,TRUE);	/* sets new range */
	setvscroll(wptr);		/* set thumb */
//	NSLog("CheckScroll %d",lfp->nlines );
}
/*********************************************************************************/
static void setvscroll(HWND hwnd)	// sets vertical bar position per record at top of screen

{
	LFLIST *lfp = getdata(hwnd);
	INDEX * FF = getowner(hwnd);
	RECORD * recptr = sort_top(FF);
	float position;
	SCROLLINFO si;
	
	if (lfp->visibleRecords < SMALLVIEWSET)	// if small viewable set
		position = (float)sort_viewindexforrecord(FF, lfp->lines[0].rnum)/lfp->visibleRecords;	// get exact position
	else
		position = sort_findpos(FF, lfp->lines[0].rnum);
	si.cbSize = sizeof(SCROLLINFO);
	si.fMask = SIF_RANGE;
	GetScrollInfo(hwnd,SB_VERT,&si);
	if (recptr && recptr->num == lfp->lines[0].rnum)	// check number in case leading deleted records
		si.nPos = 0;
	else 
		si.nPos = position*si.nMax;
//	NSLog("position: %f", position);
	si.fMask = SIF_POS;
	SetScrollInfo(hwnd,SB_VERT,&si,TRUE);	/* sets new range */
}
/*******************************************************************************/
static void trackvscroll(HWND hwnd, HWND hctl, UINT code, int pos)	/* displays repetitively while mousdown in scrollbar */

{
	SCROLLINFO si;
	RECORD * curptr;
	LFLIST *lfp = getdata(hwnd);
	INDEX * FF = getowner(hwnd);

	switch (code)	{			/* which bit hit */
		case SB_THUMBTRACK:
			si.cbSize = sizeof(SCROLLINFO);
			si.fMask = SIF_POS|SIF_RANGE|SIF_TRACKPOS|SIF_PAGE;
			GetScrollInfo(hwnd,SB_VERT,&si);
			if (si.nTrackPos != lfp->lastthumbpos)	{	// if not same position as last time
//				NSLog("Max: %d, Pos:%d, Trackpos: %d [%d : %d]",si.nMax,si.nPos, si.nTrackPos,si.nPos+si.nPage, si.nTrackPos+si.nPage);
				if (si.nTrackPos == si.nMin)		/* if to top */
					curptr = sort_top(FF);
				else if (si.nTrackPos+si.nPage == si.nMax)	/* or bottom (track position always discounts 1 page) */
					curptr = sort_bottom(FF);
				else {
					if (lfp->visibleRecords < SMALLVIEWSET)	// if small viewable set
						curptr = sort_recordforviewindex(FF,(float)lfp->visibleRecords*(si.nTrackPos+si.nPage)/si.nMax);	// jump to exact position
					else
						curptr = sort_jump(FF,(float)(si.nTrackPos+si.nPage)/si.nMax);	/* jump to right ordered position */
				}
				if (curptr)
					curptr = lfp->getrec(FF,curptr->num);	/* fix for formatted view */
				if (curptr)	{		// if have record && not already in right place
					setlines(hwnd,0,SHRT_MAX,curptr->num,0);
					invalidateclientrect(hwnd);
					lfp->lastthumbpos = si.nTrackPos;
					si.fMask = SIF_POS;
					si.nPos = si.nTrackPos;
					SetScrollInfo(hwnd,SB_VERT,&si,TRUE);	/* sets new range */
				}
			}
			return;
		case SB_LINEUP:
			linedown(hwnd,1);		/* move back one line */
			break;
		case SB_LINEDOWN:
			lineup(hwnd,1);		/* move on one line */
			break;
		case SB_PAGEUP:
			pageup(hwnd);
			break;
		case SB_PAGEDOWN:
			pagedown(hwnd);
			break;
		case SB_TOP:
		case SB_BOTTOM:
			curptr = code == SB_TOP ? sort_top(FF) : sort_bottom(FF);
			if (curptr)		{	/* if got top or bottom */
				setlines(hwnd,0,SHRT_MAX,curptr->num,0);
				invalidateclientrect(hwnd);
			}
			break;
		default:
			return;
	}
	setvscroll(hwnd);			/* set thumb position */
}
#endif
/*******************************************************************************/
static void ldowheel(HWND hwnd, int xPos, int yPos, int zDelta, UINT fwKeys)

{
	LFLIST * lfp = getdata(hwnd);
	int scrolllines;
	UINT slines;

	SystemParametersInfo(SPI_GETWHEELSCROLLLINES,sizeof(UINT),&slines,0);
	scrolllines = (zDelta * (int)slines + lfp->wheelresidue)/WHEEL_DELTA;
	lfp->wheelresidue = (zDelta * (int)slines + lfp->wheelresidue)%WHEEL_DELTA;
//	NSLog("%d, %d, %d",scrolllines, zDelta/WHEEL_DELTA,lfp->wheelresidue);

	if (scrolllines)	{
		if (scrolllines < 0)
			scrolllines = -scrolllines;
		if (slines == WHEEL_PAGESCROLL || scrolllines >= lfp->nlines)	{	// if need a page move
			if (zDelta > 0)
				pageup(hwnd);
			else
				pagedown(hwnd);
		}
		else if (zDelta > 0)	// lines down
			linedown(hwnd,scrolllines);
		else
			lineup(hwnd,scrolllines);	// lines up
		setvscroll(hwnd);			/* set thumb position */
	}
}
/*******************************************************************************/
static void lineup(HWND wptr, short lcount)	/* moves display up lines (scrolls up) */

{
	RECORD * curptr;
	INDEX * FF;
	short pos;
	LFLIST * lfp;
	
	FF = getowner(wptr);
	lfp = getdata(wptr);
	if (curptr = rec_getrec(FF,lfp->lines[lfp->nlines-1].rnum))	{		/* get record */
		if (lfp->widows)		/* if some of last record undisplayed */
			pos = lfp->lines[lfp->nlines-1].lnum+1;
		else	{		/* no more to show */
			curptr = lfp->skip(FF,curptr,1);		/* get next record */
			pos = 0;
		}
		if (curptr)	{
			scrolllines(wptr,0,lcount);		/* clear the space  */
			setlines(wptr,lfp->nlines-lcount,lcount,curptr->num,pos);
			return;
		}
	}
	if (lfp->lines[lcount].rnum)	/* if wouldn't scroll off top off screen */
		scrolllines(wptr,0,lcount);	/* scroll up empty space */
}
/*******************************************************************************/
static void linedown(HWND wptr, short lcount)		/* moves display lcount lines (scrolls down) */

{
	LSET larray[LLIMIT];			/* holds offsets of starts of lines */
	RECORD * curptr;
	INDEX * FF;
	short pos;
	struct selstruct selpars;
	LFLIST * lfp;

	FF = getowner(wptr);
	lfp = getdata(wptr);
	if (curptr = rec_getrec(FF,lfp->lines[0].rnum))		{
		selpars.startpos = selpars.length = -1;		/* prevent finding selection */
		pos = lfp->lines[0].lnum;
		while (pos < lcount && (curptr = lfp->skip(FF,curptr,-1)))
			pos += lfp->measurelines(wptr, lfp->hdc, curptr, larray, &selpars);	/* if lines to set */
		if (curptr)	{
			scrolllines(wptr,0,-lcount);		/* clear the space; adjust the array */
			setlines(wptr,0,lcount,curptr->num,pos-lcount);
		}
	}
}
/*******************************************************************************/
static void scrolllines(HWND wptr, short start, short nlines)	/* adjusts lines array & scrolls display + - n lines */

{
	short i, newlast;
	HRGN trgn;
	RECT trect, updaterect;
	LFLIST * lfp;
	
	if (nlines)		{
		lfp = getdata(wptr);
		if (nlines > 0)		{	/* scroll up ( i.e., make space at foot of window) */
			for (i = start; i+nlines < lfp->nlines; i++)
				lfp->lines[i] = lfp->lines[i+nlines];
			memset(&lfp->lines[lfp->nlines-nlines], 0, nlines *sizeof(DLINE));	/* clear elements */
		}
		else 		{			/* scroll down (make space at head of window) */
			/* NB: nlines is -ve for scroll direction */
			newlast = lfp->nlines+nlines-1;		/* line that will be last on screen */
			if (lfp->lines[newlast].rnum != lfp->lines[lfp->nlines-1].rnum)	{	/* if not current last record */
				for (lfp->widows = 0; lfp->lines[newlast].rnum == lfp->lines[newlast+1].rnum; newlast++)	/* count new widows */
					lfp->widows++;
			}
			else if (lfp->lines[newlast].rnum)		/* will add widows to current last record */
				lfp->widows -= nlines;
			else
				lfp->widows = 0;
			for (i = lfp->nlines-1; i+nlines >= start; i--)
				 lfp->lines[i] = lfp->lines[i+nlines];
			memset(&lfp->lines[start], 0, -nlines *sizeof(DLINE));	/* clear elements */
		}
		trgn = CreateRectRgn(0,0,0,0);			/* get empty rgn */
		trect = lfp->vrect;
		trect.top += lfp->lheight*start;
		ScrollDC(lfp->hdc,0,lfp->lheight*-nlines,&trect,&trect,trgn,&updaterect);
		InvalidateRgn(wptr,trgn,TRUE);
		DeleteRgn(trgn);
	}
}
/*******************************************************************************/
static void pageup(HWND wptr)		/* moves display down one page (scrolls down) */

{
	LSET larray[LLIMIT];				/* holds offsets of starts of lines */
	INDEX * FF;
	RECORD * recptr, * nextptr;
	short pos;
	struct selstruct selpars;
	LFLIST * lfp;
	
	FF = getowner(wptr);
	lfp = getdata(wptr);
	selpars.startpos = selpars.length = -1;		/* prevent finding selection */
	if (recptr = rec_getrec(FF,lfp->lines[0].rnum))	{			/* if any record */
		pos = lfp->lines[0].lnum;
		while (pos < lfp->nlines && (nextptr = lfp->skip(FF,recptr,-1)))	{	/* while need more lines */ 
			recptr = nextptr;
			pos += lfp->measurelines(wptr, lfp->hdc,recptr, larray,&selpars);
		}
		if (pos)	{		/* if any lines to display  */
			setlines(wptr,0, SHRT_MAX, recptr->num,pos-lfp->nlines+1);		/* set up new line table */
			invalidateclientrect(wptr);
		}
	}
}
/*******************************************************************************/
static void pagedown(HWND wptr)		/* moves display up one page (scrolls up) */

{
	RECN rnum;
	short epos;

	epos = LX(wptr,nlines)-1;
	while (!(rnum = LX(wptr,lines)[epos].rnum))		/* if empty last line */
		epos--;
	if (epos)	{	/* if anything below first line */
		if (setlines(wptr, 0, SHRT_MAX, rnum, LX(wptr,lines)[epos].lnum))		/* set up new line table */
			invalidateclientrect(wptr);
	}
}
/*********************************************************************************/
static void checkhscroll(HWND wptr)		/* scales, sets horizontal scroll bars */

{
	LFLIST * lfp = getdata(wptr);
	long pixshift;
	SCROLLINFO si;

	si.cbSize = sizeof(SCROLLINFO);
	si.fMask = SIF_RANGE|SIF_POS|SIF_PAGE|SIF_DISABLENOSCROLL;
	si.nMin = 0;
	si.nMax = lfp->width+2*DESTINSET;	// width of standard page
	si.nPage = lfp->vrect.right-lfp->vrect.left;	// actual width of display
	if (si.nMax > si.nPage)	{	// if showing less than page width
		if (GetLayout(lfp->hdc)&LAYOUT_RTL)	{
			if ((pixshift = (lfp->vrect.right - (lfp->drect.right+DESTINSET))) > 0) /* if displaying less on far edge than could */
				scrollh(wptr,-pixshift);
			si.nPos = lfp->drect.right-lfp->vrect.right;
		}
		else {
			if ((pixshift = (lfp->vrect.right - (lfp->drect.right+DESTINSET))) > 0) /* if displaying less on far edge than could */
				scrollh(wptr,pixshift);
			si.nPos = (lfp->vrect.left-lfp->drect.left);
		}
	}
	else	{	/* all fits in width */
		si.nMax = si.nMin;		/* disable */
	}
	SetScrollInfo(wptr,SB_HORZ,&si,TRUE);	/* sets new range */
}
/******************************************************************************/
static void trackhscroll(HWND wptr, HWND hctl, UINT code, int pos)	/* displays repetitively while mousdown in scrollbar */

{
	SCROLLINFO si;
	long width, csize, step;
	
	si.cbSize = sizeof(SCROLLINFO);
	si.fMask = SIF_POS|SIF_RANGE|SIF_TRACKPOS;
	GetScrollInfo(wptr,SB_HORZ,&si);

	csize = LX(wptr,cwidth);
	width = LX(wptr,vrect).right-LX(wptr,vrect).left;
	
	switch (code)		{		/* do the right things */
		case SB_THUMBTRACK:
		case SB_THUMBPOSITION:
			if (step = si.nPos-si.nTrackPos)		/* deal here with thumb */
				scrollh(wptr,step);
			si.nPos = si.nTrackPos;
			break;
		case SB_LINELEFT:		/* show one point at left */
			if (si.nPos > si.nMin)	{
				scrollh(wptr,csize);
				si.nPos -= csize;
			}
			break;
		case SB_LINERIGHT:		/* show point at right */
			if (si.nPos < si.nMax-width)	{
				step = si.nPos+csize > width ? si.nPos-width : csize;	/* find # pixels to scroll */
				scrollh(wptr,-step);
				si.nPos += step;
			}
			break;
		case SB_PAGELEFT:			/* show new page at left */
			if (si.nPos > si.nMin)	{
//				step = si.nPos-si.nMin > width ? width : si.nPos-si.nMin;	/* find # pixels to scroll */
				step = si.nPos-si.nMin;	/* find # pixels to scroll */
				scrollh(wptr,step);
				si.nPos -= step;
			}
			break;
		case SB_PAGERIGHT:		/* show new page at right */
			if (si.nPos < si.nMax-width)	{
//				step = si.nMax-si.nPos > width ? si.nMax-width-si.nPos : si.nMax-si.nPos;	/* find # pixels to scroll */
				step = si.nMax-width-si.nPos;	/* find # pixels to scroll */
				scrollh(wptr,-step);
				si.nPos += step;
			}
			break;
	}
	si.fMask = SIF_POS|SIF_DISABLENOSCROLL;
	SetScrollInfo(wptr,SB_HORZ,&si,TRUE);	/* sets new position */
}
#if 0
/*******************************************************************************/
static void scrollh(HWND wptr, long offset)	/* scrolls window horizontally */

{
	LFLIST * lfp = getdata(wptr);
	
	if (offset)	{
		int sign = GetLayout(lfp->hdc)&LAYOUT_RTL ? -1 : 1;
		HRGN trgn;
		RECT trect, updaterect;

		trgn = CreateRectRgn(0,0,0,0);			/* get empty rgn */
		trect = lfp->vrect;
#if 0
		ScrollDC(lfp->hdc,offset,0,&trect,&trect,trgn,&updaterect);
		OffsetRect(&lfp->drect,offset,0);
#else
		ScrollDC(lfp->hdc,offset*sign,0,NULL,NULL,trgn,&updaterect);
		OffsetRect(&lfp->drect,offset,0);
#endif
		InvalidateRgn(wptr,trgn,TRUE);
		DeleteRgn(trgn);
	}
}
#else
/*******************************************************************************/
static void scrollh(HWND wptr, long offset)	/* scrolls window horizontally */

{
	LFLIST * lfp = getdata(wptr);
	int sign = GetLayout(lfp->hdc)&LAYOUT_RTL ? -1 : 1;
	
	if (offset && sign == 1)	{
		HRGN trgn;
		RECT trect, updaterect;

		trgn = CreateRectRgn(0,0,0,0);			/* get empty rgn */
		trect = lfp->vrect;
		ScrollDC(lfp->hdc,offset,0,&trect,&trect,trgn,&updaterect);
		OffsetRect(&lfp->drect,offset,0);
		InvalidateRgn(wptr,trgn,TRUE);
		DeleteRgn(trgn);
	}
	else {
//		NSLog("XX: %d",offset);
		invalidateclientrect(wptr);
		OffsetRect(&lfp->drect,-offset,0);
	}
}
#endif
/*******************************************************************************/
static void invalidateclientrect(HWND hwnd)

{
	InvalidateRect(hwnd,NULL,TRUE);
}
/*******************************************************************************/
static short selectlines(HWND wptr, short target, short extend, short level, struct selstruct *slptr) /* selects & highlights lines */

/* usage: if target line > 0, highlights all of record on that line
	if target < 0; highlights record (or range of records) as per selstruct
	if extend is TRUE, extends the current selection */

{
	short fline, lline, ftline, ltline, line, llimit, lheight, lcount, tline;
	HRGN oldr, newr;
	RECN targetnum;
	int xf;
	LFLIST * lfp;
	
	lfp = getdata(wptr);
	if (target < 0 || lfp->lines[target].rnum)	{	/* if selecting by record, or line has content */
		lheight = lfp->lheight;
		for (llimit = lfp->nlines; !lfp->lines[llimit-1].rnum && llimit > 1; llimit--)	/* find last filled line */
			;
		for (fline = lline = -1, line = 0; line < llimit; line++)		{	/* for all lines with records */
			if (lfp->lines[line].flags&VSELECT)	{	/* if a selected row */
				lfp->lines[line].flags &= ~VSELECT;		/* clear flag */
				if (fline < 0)			/* if first selected row */
					fline = line;		/* mark it */
				lline = line;
			}
			else if (fline >= 0)	/* if first after selection */
				break;
		}
		if (target < 0)	{		/* if selection defined by record numbers */
			if (lcount = linesonscreen(wptr,slptr->last,&ltline))	/* if last record has any lines on screen */
				ltline += lcount-1;				/* set last line */
			linesonscreen(wptr, slptr->first, &ftline);	/* find first line */
			if (ftline < 0 && ltline < 0)	{		/* nothing on screen */
				if (!slptr->first || sort_relpos(getowner(wptr), slptr->last, lfp->lines[0].rnum) < 0 ||	/* if start & end not in window */
						sort_relpos(getowner(wptr), slptr->first, lfp->lines[llimit-1].rnum) > 0)	{		/* if selection doesn't cross window */
					ftline = 1;		/* set lines to force empty rect */
					ltline = 0;
				}
				else	{			/* select all that's on screen */
					ftline = 0;
					ltline = llimit-1;
				}
			}
			else {						/* something is on screen */
				if (ltline < 0)			/* if last is beyond end */
					ltline = llimit-1;
				if (ftline < 0)			/* if first is before start */
					ftline = 0;
			}
			lfp->sel.first = slptr->first;		/* save start REC of selection */
			lfp->sel.last = slptr->last;		/* end REC of selection */
		}
		else	{		/* specified record from line */
			if ((ftline = target-lfp->lines[target].lnum) < 0) /* find first target line */
				ftline = 0;
			for (ltline = target; lfp->lines[ltline+1].rnum == lfp->lines[target].rnum && ltline < llimit-1;)
				ltline++;
			targetnum = lfp->lines[ftline].rnum;
			
			/* lines occupied by target record now defined */
	
			if (extend && lfp->sel.first)	{		/* if extending existing selection */
				if (fline < 0)	{ 			/* if something selected but not on screen */
					if (sort_relpos(getowner(wptr), lfp->sel.last, lfp->lines[0].rnum) > 0)	{	/* if target before hidden selection */
						lfp->sel.dir = -1;		/* moved backwards */
						ltline = llimit-1;				/* highlight to last line */
						lfp->sel.first = targetnum;	/* target becomes start of selection */
					}
					else	{		/* target is after hidden selection */
						lfp->sel.dir = 1;		/* moved forwards */
						ftline = 0;	/* highlight from first line */
						lfp->sel.last = targetnum;	/* target becomes end of selection */
					}
				}
				else {	/* extending selection that's at least partly on screen */
					if (ftline < fline)		{			/* if target before current selection */
						lfp->sel.dir = -1;		/* moved backwards */
						ltline = lline;
						lfp->sel.first = targetnum;	/* target becomes start of selection */
					}
					else if (ltline > lline)	{		/* if target is after end of selection */
						lfp->sel.dir = 1;			/* moved forwards */
						ftline = fline;
						lfp->sel.last = targetnum;	/* target becomes end of selection */
					}
					else {				/* moved within current selection range */
						if (lfp->sel.dir > 0)	{			/* if last move was downwards */
							ftline = fline;		/* moving backwards, so keep old top */
							lfp->sel.last = targetnum;	/* target becomes end of selection */
						}
						else	{
							ltline = lline;		/* use old bottom */
							lfp->sel.first = targetnum;	/* target becomes start of selection */
						}
					}
				}
			}
			else	{
				lfp->sel.first = targetnum;			/* save start REC of selection */
				lfp->sel.last = targetnum;			/* end REC of selection */
				lfp->sel.dir = 0;					/* new selection */
			}
		}
		oldr = formselrgn(wptr,fline,lline,lheight);		/* find rgn for current selection */
		while ((lfp->lines[ftline].indent&FO_HMASK) < level || lfp->lines[ftline].lnum < slptr->sline)	/* while dropping levels above target */
			ftline++;		/* advance first line */
		while (lfp->lines[ltline].lnum > slptr->eline)	/* while too much on end */
			ltline--;		/* retreat */
		for (tline = ftline; tline <= ltline; tline++)	{		/* for range of new selection */
			lfp->lines[tline].flags |= VSELECT;		/* set flags */
			lfp->lines[tline].selstart = -1;		/* start of range */
			lfp->lines[tline].selend = -1;		/* end of range */
		}
		lfp->lines[ftline].selstart = slptr->shighlight;	/* set highlight limit on first line */
		lfp->lines[ltline].selend = slptr->ehighlight;		/* and last line */
		newr = formselrgn(wptr,ftline,ltline,lheight);		/* form new sel region */
		xf = CombineRgn(oldr,newr,oldr,RGN_XOR);		/* result is all but overlap */
		InvalidateRgn(wptr,oldr,TRUE);
		DeleteRgn(oldr);
		DeleteRgn(newr);
		lfp->sel.sline = slptr->sline;		/* save start line */
		lfp->sel.eline = slptr->eline;		/* save end line */
		lfp->sel.shighlight = slptr->shighlight;	/* set highlight limit on first */
		lfp->sel.ehighlight = slptr->ehighlight;	/* and last */
		if (SendMessage(g_hwclient,WM_MDIGETACTIVE,0,0) == (LPARAM)getowner(wptr)->cwind)	/* if we're the active window */
			doselbuttons(wptr);
	}
	return (target);
}
/****************************************************************************/
static HRGN formselrgn(HWND wptr, short fline, short lline, short lheight)	/* forms & returns selection rgn */

{
	HRGN xrgn, orgn;
	RECT srect, trect;
	LFLIST * lfp = getdata(wptr);

	srect.top = fline*lheight;		/* set top */
	if (getowner(wptr)->righttoleftreading)	{
		srect.left = lfp->vrect.left;
		srect.right = lfp->drect.right+lfp->nspace;
	}
	else {
		srect.left = lfp->drect.left + lfp->nspace;
		srect.right = lfp->vrect.right;
	}
	srect.bottom = (lline+1)*lheight;	/* and bottom */
	srect.right = lfp->vrect.right;
	orgn = CreateRectRgn(srect.left,srect.top, srect.right,srect.bottom);
	if (fline >= 0)		{	/* if anything needing attention */
		if (lfp->lines[fline].selstart >= 0)	{	/* if selection is offset at left */
			trect = srect;
			trect.bottom = trect.top+lheight;	/* adapt the base highlight rect */
			trect.right = trect.left+lfp->lines[fline].selstart + 
				(lfp->lines[fline].indent&FO_HMASK)*lfp->indent + (lfp->lines[fline].indent&FO_RFLAG ? lfp->indent : 0);	/* rect for lead indent */
			xrgn = CreateRectRgn(trect.left,trect.top, trect.right,trect.bottom);
			CombineRgn(orgn,orgn,xrgn,RGN_DIFF);	/* subtract first indent */
			DeleteRgn(xrgn);
		}
		if (lfp->lines[lline].selend >= 0)	{	/* if selection indented at right */
			trect = srect;		/* adapt the base highlight rect */
			trect.top = trect.bottom-lheight;
			trect.left += lfp->lines[lline].selend + 
				(lfp->lines[lline].indent&FO_HMASK)*lfp->indent + (lfp->lines[lline].indent&FO_RFLAG ? lfp->indent : 0);
			xrgn = CreateRectRgn(trect.left,trect.top, trect.right,trect.bottom);
			CombineRgn(orgn,orgn,xrgn,RGN_DIFF);	/* now have region trimmed */
			DeleteRgn(xrgn);
		}
	}
	return (orgn);
}
/*******************************************************************************/
static short linesonscreen(HWND wptr, RECN rnum, short * first)		/* finds how many lines on screen */

{
	short count, lcount;
	
	for (*first = -1, lcount = count = 0; rnum && count < LX(wptr,nlines); count++)	{	/* for all lines on screen */
		if (LX(wptr,lines)[count].rnum == rnum)	{	/* if our record */
			if (*first < 0)
				*first = count;
			lcount++;
		}
	}
    return (lcount);
}
/******************************************************************************/
static BOOL isinmargin(HWND hwnd, int offset)	// checks whether point is in margin

{
	LFLIST * lfp = getdata(hwnd);
	int mslop = WX(hwnd,owner)->head.privpars.vmode == VM_FULL ? lfp->indent : lfp->nspace;

	if (GetLayout(lfp->hdc)&LAYOUT_RTL)
		offset -= lfp->vrect.right-mslop;
	return offset > 0 && offset < lfp->drect.left-lfp->vrect.left+mslop;
}
/*********************************************************************************/
RECN view_getsellimit(HWND wptr)	/* finds record immed beyond selection */

{
	RECORD * recptr;
	
	if (recptr = rec_getrec(getowner(wptr),LX(wptr, sel).last))	{
		if (recptr = LX(wptr,skip)(getowner(wptr),recptr,1))
			return (recptr->num);
	}
	return (ULONG_MAX);
}
/******************************************************************************/
static void CALLBACK autosave(HWND hwnd, UINT msg, UINT_PTR timer, DWORD time)	/* auto saves index */

{
	TCHAR tstring[_MAX_FNAME+_MAX_EXT];

	u_sprintf(tstring,"Saving %S...",getowner(hwnd)->iname);
//	nstrcat(tstring,getowner(hwnd)->iname);
	SendMessage(g_hwstatus,SB_SIMPLE,TRUE,0);	/* set simple status window */
	SendMessage(g_hwstatus,SB_SETTEXT,255|SBT_NOBORDERS,(LPARAM)tstring);	/* display on status line */
	file_saveprivatebackup(getowner(hwnd));
	SendMessage(g_hwstatus,SB_SIMPLE,FALSE,0);	/* restore multi-part status window */
}
/******************************************************************************/
static void lcopy(HWND hwnd)	/* copies selection to clipboard */

{
	SCODE sc;

	recole_capture(hwnd,&rec_call);
	sc = OleSetClipboard((IDataObject *)&rec_call);	/* source for copy */
	dopastebutton(getowner(hwnd));		/* sets paste button */
	/* call release here ? */
}
/******************************************************************************/
static void lpaste(HWND hwnd)	/* pastes records from clipboard */

{
	IDataObject * dp;

	OleGetClipboard(&dp);	/* paste item only enabled if formats OK */
	recole_paste(hwnd,dp);
	view_setstatus(hwnd);
}
