#include "stdafx.h"
#include "containerset.h"
#include "rcontainerset.h"
#include "commands.h"
#include "files.h"
#include "viewset.h"
#include "modify.h"
#include "util.h"
#include "findset.h"
#include "replaceset.h"
#include "spellset.h"
#include "indexset.h"
#include "toolset.h"
#include "tagstuff.h"
#include "index.h"
#include "abbrev.h"

#define CX(WW,II) (((CFLIST *)GetWindowLongPtr(WW,GWLP_USERDATA))->II)	/* for addressing items */

static void ccommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);	/* does menu tasks */
static BOOL ccreate(HWND hwnd,LPCREATESTRUCT cs);		/* initializes view window */
static LRESULT CALLBACK hook(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static void csize(HWND hwnd, UINT state, int cx, int cy);
static short cclose(HWND hwnd);		/* closes index */
static void csetfocus(HWND hwnd, HWND oldfocus);	/* gets focus shift message */
static int cnotify(HWND hwnd, int id, NMHDR * hdr);	/* does notification tasks */
static void cpnotify(HWND hwnd, UINT msg, HWND hwndChild, UINT idChild); 
static void csyscommand(HWND hwnd, UINT cmd, int x, int y);	// window max min info
static BOOL cposchanging(HWND hwnd, LPWINDOWPOS wpos);	// window pos changing
static BOOL cncactivate(HWND hwnd, BOOL fActive, HWND hwndActDeact, BOOL fMinimized);	/* non-client window act */
static void cactivate(HWND hwnd, BOOL fActive, HWND hwndActivate, HWND hwndDeactivate);		/* activates window */
static int cactivateonmouse(HWND hwnd, HWND topwind, UINT hitcode, UINT msg);	/* does right kind of activation on window click */
//static int cactivateapp(HWND hwnd, BOOL fActivate, DWORD dwThreadId);	/* does right kind of activation on window click */
static HWND activechild(HWND hwnd);
static WNDPROC activechildproc(HWND hwnd);

/*********************************************************************************/
HWND container_setwindow(INDEX * FF, short visflag)	/*  sets up container window */

{
	HWND hwnd;
	RECT crect, prect;
	UINT mmstate;

	if (g_revertstate)		{	// if reverting
		prect = g_revertplacement;		// settings from reverting index override others
		mmstate = g_revertstate;
		g_revertstate = 0;		// make sure revert disabled
		scaleRectForDpi(g_hwclient, &prect);
	}
	else if (FF->head.vrect.right)	{	// if have established rect
		prect = FF->head.vrect;
		mmstate = FF->head.mmstate;
		scaleRectForDpi(g_hwclient, &prect);
		fitrecttoclient(&prect);
	}
	else {	// use standard rect
		mmstate = 0;
		GetClientRect(g_hwclient,&crect);
		prect.left = prect.top = 0;
		prect.right = FF->head.formpars.pf.pi.pwidthactual-(FF->head.formpars.pf.mc.right+FF->head.formpars.pf.mc.left);
		prect.bottom = FF->head.formpars.pf.pi.pheightactual-(FF->head.formpars.pf.mc.top+FF->head.formpars.pf.mc.bottom);
		prect.right += 2*4+GetSystemMetrics(SM_CXVSCROLL); // !! set this properly
		scalerect(&prect,g_slpix,72);		/* scale for screen pixels */
		prect.bottom = crect.bottom-crect.top;	/* now has desired client area */
		adjustWindowRect(g_hwclient,&prect,WS_HSCROLL|WS_VSCROLL|WS_CLIPCHILDREN,FALSE,WS_EX_CLIENTEDGE);	// get rect needed for whole view window
		adjustWindowRect(g_hwclient,&prect,WS_CLIPCHILDREN,FALSE,WS_EX_MDICHILD);	/* now get window rect that has right client area */
		// prect comes out of this properly scaled for dpi
	}
	hwnd = CreateWindowEx(WS_EX_MDICHILD,g_containerclass,index_displayname(FF),WS_CLIPCHILDREN|WS_CLIPSIBLINGS,
			prect.left,prect.top,prect.right-prect.left,prect.bottom-prect.top,g_hwclient,NULL,g_hinst,FF);
	if (hwnd)	{
		if (g_prefs.gen.maxdoc || mmstate == SW_SHOWMAXIMIZED)	// if want maximized
			SendMessage(g_hwclient,WM_MDIMAXIMIZE,(WPARAM)hwnd,0);
	}
	return (hwnd);
}
/************************************************************************/
LRESULT CALLBACK container_proc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)

{
	switch (msg)	{
		HANDLE_MSG(hwnd,WM_CREATE,ccreate);
		HANDLE_MSG(hwnd,WM_SIZE,csize);
//		HANDLE_MSG(hwnd,WM_NCACTIVATE,cncactivate);
		HANDLE_MSG(hwnd,WM_MDIACTIVATE,cactivate);
		HANDLE_MSG(hwnd,WM_MOUSEACTIVATE,cactivateonmouse);
//		HANDLE_MSG(hwnd,WM_ACTIVATEAPP,cactivateapp);
		HANDLE_MSG(hwnd,WM_NOTIFY,cnotify);
		HANDLE_MSG(hwnd,WM_COMMAND,ccommand);
		HANDLE_MSG(hwnd,WM_SETFOCUS,csetfocus);
		HANDLE_MSG(hwnd,WM_WINDOWPOSCHANGING,cposchanging);
		HANDLE_MSG(hwnd,WM_SYSCOMMAND,csyscommand);		
		case WM_CLOSE:
			return cclose(hwnd);
		case WMM_SETGROUP:
			view_showgroup(hwnd,fromNative((TCHAR *)lParam));
			return (0);
		case WM_INITMENUPOPUP:
//		case WMM_UPDATETOOLBARS:
		case WM_KEYDOWN:
		case WM_CHAR:
		case WM_HOTKEY:
			return SendMessage(activechild(hwnd),msg,wParam,lParam);
	}
	return DefMDIChildProc(hwnd,msg,wParam,lParam);
}
/*********************************************************************************/
void container_installrecordwindow(HWND hwnd)	/*  sets up record window */

{
	INDEX * FF = getowner(hwnd);

	if (g_prefs.gen.newwindow)	 // if  want new window
		rcontainer_installrecordwindow(FF);
	else {			// embedded
		CFLIST * cfp = getdata(hwnd);
		REBARBANDINFO rbBand;

		rbBand.cbSize = sizeof(REBARBANDINFO);
		rbBand.fMask =  RBBIM_CHILD | RBBIM_IDEALSIZE|RBBIM_STYLE;
		rbBand.fStyle = RBBS_CHILDEDGE | RBBS_FIXEDBMP;
		rbBand.hwndChild  = FF->rwind;
		rbBand.cxIdeal = scaleForDpi(hwnd,FF->head.privpars.rwrect.bottom-FF->head.privpars.rwrect.top);
		SendMessage(cfp->rbwind, RB_INSERTBAND, (WPARAM)-1, (LPARAM)&rbBand);
		SendMessage(cfp->rbwind, RB_MAXIMIZEBAND, (WPARAM)1, TRUE);	// set to ideal height
	}
}
/*********************************************************************************/
void container_removerecordwindow(HWND hwnd)	/*  removes record window */

{
	INDEX * FF = getowner(hwnd);

	if (FF->rcwind) {	// if floating window
//		rcontainer_removerecordwindow(FF);
		freedata(FF->rcwind);		/* releases window data */
		SendMessage(g_hwclient, WM_MDIDESTROY, (WPARAM)FF->rcwind, 0);	/* destroy container */
		FF->rcwind = NULL;
	}
	else {
		CFLIST * cfp = getdata(hwnd);
		SendMessage(cfp->rbwind, RB_DELETEBAND, (WPARAM)1, 0);
//		SendMessage(FF->vwind,WMM_UPDATETOOLBARS,0,0);	//reset toolbar for vwind
		view_setstatus(FF->vwind);		// need this'cause embedded rec window doesn't automatically update view status
	}
}
/************************************************************************/
static void ccommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)	/* does menu tasks */

{
	switch (id)	{
		case IDM_FILE_CLOSE:
			cclose(hwnd);
			return;
		case IDM_FILE_SAVEFORMAT:
			file_savestylesheet(hwnd);
			return;
		case IDM_EDIT_FIND:
			find_setwindow(hwnd);
			return;
		case IDM_EDIT_FINDAGAIN:
			fs_findagain(getowner(hwnd), g_findw);
			return;
		case IDM_EDIT_REPLACE:
			rep_setwindow(hwnd);
			return;
#if !READER		// if not Reader
		case IDM_TOOLS_CHECKSPELLING:
			spell_setwindow(hwnd);
			return;
#endif
		case IDM_DOCUMENT_RECORDSTRUCTURE:
			is_setstruct(hwnd, NULL);
			return;
		case IDM_DOCUMENT_REFERENCESYNTAX:
			is_setrefsyntax(hwnd);
			return;
		case IDM_DOCUMENT_FLIPWORDS:
			is_flipwords(hwnd);
			return;
		case IDM_TOOLS_VERIFYCROSSREFERENCES:
			ts_verify(hwnd);
			return;
		case IDM_TOOLS_COUNTRECORDS:
			ts_count(hwnd);
			return;
		case IDM_TOOLS_INDEXSTATISTICS:
			ts_statistics(hwnd);
			return;
		case IDM_TOOLS_MARKUPTAGS:
			ts_managetags(hwnd);
			return;
	}
	FORWARD_WM_COMMAND(activechild(hwnd),id,hwndCtl,codeNotify,activechildproc(hwnd));	
}
/******************************************************************************/
static BOOL ccreate(HWND hwnd,LPCREATESTRUCT cs)		/* initializes view window */

{
	CFLIST * cfp;

	if (cfp = getmem(sizeof(CFLIST)))	{	/* if can get memory for our window structure */
		INDEX * FF;
		HWND rbwind;

		setdata(hwnd,cfp);		/* set memory */
		FF = WX(hwnd, owner) = getMDIparam(cs);	/* recover param passed (FF) */
		FF->cwind = hwnd;
		rbwind = CreateWindowEx(WS_EX_TOOLWINDOW,REBARCLASSNAME, NULL,
				WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|WS_CLIPCHILDREN|
				/* RBS_BANDBORDERS| */RBS_FIXEDORDER|RBS_DBLCLKTOGGLE|CCS_NODIVIDER|CCS_VERT,
				0,0,0,0,hwnd,NULL,g_hinst,NULL);
		if (rbwind){
			REBARINFO rbi;
			REBARBANDINFO rbBand;
			RECT rc;

			cfp->rbwind = rbwind;
			(LONG_PTR)cfp->rbproc = SetWindowLongPtr(cfp->rbwind,GWLP_WNDPROC,(LONG_PTR)hook);		/* set subclass handler */
			rbi.cbSize = sizeof(REBARINFO);
			rbi.fMask = 0;
			rbi.himl = (HIMAGELIST)NULL;
			if (SendMessage(rbwind, RB_SETBARINFO, 0, (LPARAM)&rbi)){
				rbBand.cbSize = sizeof(REBARBANDINFO);
				rbBand.fMask = RBBIM_STYLE | RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_SIZE;

				// create view window
				FF->vwind = view_setwindow(FF,TRUE);
				GetWindowRect(FF->vwind, &rc);
				rbBand.fStyle = RBBS_CHILDEDGE | RBBS_FIXEDBMP|RBBS_NOGRIPPER;
				rbBand.hwndChild  = FF->vwind;
				rbBand.cxMinChild = 100;	// min height
				rbBand.cyMinChild = rc.right-rc.left;	// min width
				rbBand.cx = rc.bottom-rc.top;
				SendMessage(rbwind, RB_INSERTBAND, (WPARAM)-1, (LPARAM)&rbBand);
				return TRUE;
			}
		}
	}
	return (FALSE);
}
/************************************************************************/
static LRESULT CALLBACK hook(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)

{
	// need this hook to prevent rebar overriding cursor for rich edit control
	LRESULT result  = CallWindowProc((WNDPROC)CX(GetParent(hwnd),rbproc),hwnd,msg,wParam,lParam);	/* pass to ordinary handler */

	if (msg == WM_SETCURSOR)	// if setting cursor
		return FALSE;
	return result;
}
//**********************************************************************************/
static short cclose(HWND hwnd)		/* closes index */

{
	INDEX * FF = getowner(hwnd);

	FF->head.mmstate = getmmstate(hwnd,&FF->head.vrect);// get window state, size, pos (in parent coords)
	if (view_close(FF->vwind))	{	// if all ok
		CFLIST * cfp = getdata(hwnd);

		SetWindowLongPtr(cfp->rbwind, GWLP_WNDPROC,(LONG_PTR)cfp->rbproc);	// restore rebar proc
		freedata(hwnd);		/* releases window data */
		SendMessage(g_hwclient,WM_MDIDESTROY,(WPARAM)hwnd,0);	/* destroy container */			
		return TRUE;
	}
	return FALSE;
}
/******************************************************************************/
static void csetfocus(HWND hwnd, HWND oldfocus)	/* gets focus shift message */

{
	SetFocus(activechild(hwnd));
	FORWARD_WM_SETFOCUS(hwnd,oldfocus,DefMDIChildProc);
}
/************************************************************************/
static int cnotify(HWND hwnd, int id, NMHDR * hdr)	/* does notification tasks */

{
	switch (hdr->code)	{
		case RBN_ENDDRAG:
			if (((LPNMREBAR)hdr)->uBand == 1)	{
				CFLIST * cfp = getdata(hwnd);
				REBARBANDINFO rbBand;

				rbBand.cbSize = sizeof(REBARBANDINFO);
				rbBand.fMask = RBBIM_SIZE;
				SendMessage(cfp->rbwind,RB_GETBANDINFO,1,(LPARAM)&rbBand);
				rbBand.fMask = RBBIM_IDEALSIZE;
				rbBand.cxIdeal = rbBand.cx-14;	// ideal size is current size (- band border offset?)
				SendMessage(cfp->rbwind,RB_SETBANDINFO,1,(LPARAM)&rbBand);
			}
			return TRUE;
	}
	return FORWARD_WM_NOTIFY(hwnd,id,hdr,DefMDIChildProc);
}
/*********************************************************************************/
void csyscommand(HWND hwnd, UINT cmd, int x, int y)	// window max min info

{
	INDEX *FF = getowner(hwnd);

	if (cmd == SC_NEXTWINDOW || cmd == SC_PREVWINDOW)	{
		if (FF->rwind && !FF->rcwind)	{	// if internal (new style) entry window is open
			if (GetFocus() == FF->vwind)	// if view window is active
				SetFocus(FF->rwind);
			else
				SetFocus(FF->vwind);
			return;
		}
	}
	if (cmd == SC_MINIMIZE || cmd == SC_RESTORE || cmd == SC_MAXIMIZE)	{	// if minimizing or restoring
		if (FF->rcwind)	// if have separate record window
			return;
	}
	FORWARD_WM_SYSCOMMAND(hwnd,cmd,x,y,DefMDIChildProc);
}
/*********************************************************************************/
static BOOL cposchanging(HWND hwnd, LPWINDOWPOS wpos)	// window pos changing

{
	if (activechild(hwnd))	{	// if has content
		INDEX * FF = getowner(hwnd);

//		if (FF->holdsizechange) {
//			wpos->flags |= SWP_NOSIZE;
//		}
		if (FF->rcwind && FF->rcwind != wpos->hwndInsertAfter)	{		// if has separate record window
			SetWindowPos(FF->rcwind, HWND_TOP,0,0,0,0,SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOSIZE);	// make sure record window is top
			wpos->hwndInsertAfter = FF->rcwind;	// place us behind
			return 0;
		}
	}
	return FORWARD_WM_WINDOWPOSCHANGING(hwnd,wpos,DefMDIChildProc);
}
#if 0
/******************************************************************************/
static BOOL cncactivate(HWND hwnd, BOOL fActive, HWND hwndActDeact, BOOL fMinimized)	/* non-client window act */

{
	if (fActive)	{	/* if becoming active */
		INDEX * FF = getowner(hwnd);

		FF->acount++;		/* count an activation */
		if (IsChild(FF->rcwind,GetFocus()))	// if focus window is our stand-alone record window
			setmdiactive(hwnd,FALSE);		// clear discard first click
	}
	else
		setmdiactive(hwnd,TRUE);		// discard next click
	return FORWARD_WM_NCACTIVATE(hwnd,fActive,hwndActDeact, fMinimized,DefMDIChildProc);
}
#endif
/******************************************************************************/
static void cactivate(HWND hwnd, BOOL fActive, HWND hwndActivate, HWND hwndDeactivate)		/* activates window */

{
	if (hwnd == hwndActivate)	{	/* if being activated */
		INDEX * FF = getowner(hwnd);

		if (FF->rcwind == hwndDeactivate)	// if from record window
			setmdiactive(hwnd,FALSE);		// clear discard first click
		else
			FF->acount++;		// count an activation
		com_pushrecent(FF->pfspec);		/* update recent file list */
		view_setstatus(FF->vwind);		/* update status bar */
//		SendMessage(activechild(hwnd),WMM_UPDATETOOLBARS,0,0);
	}
	else
		setmdiactive(hwnd,TRUE);		// discard next click
	FORWARD_WM_MDIACTIVATE(hwnd,fActive,hwndActivate, hwndDeactivate,DefMDIChildProc);
}
/*******************************************************************************/
static int cactivateonmouse(HWND hwnd, HWND topwind, UINT hitcode, UINT msg)	/* does right kind of activation on window click */

{
	if (hitcode == HTCLIENT && getmdiactive(hwnd))	{/* if first new hit in client area */
		setmdiactive(hwnd,FALSE);		// clear discard first click
		return MA_ACTIVATEANDEAT;
	}
	setmdiactive(hwnd,FALSE);		// clear discard first click
	return MA_ACTIVATE;
}
#if 0
/*******************************************************************************/
//Cls_OnActivateApp(HWND hwnd, BOOL fActivate, DWORD dwThreadId)
static int cactivateapp(HWND hwnd, BOOL fActivate, DWORD dwThreadId)	/* does right kind of activation on window click */

{
	if (!fActivate)	{	// if being deactivated
		setmdiactive(hwnd,TRUE);		// discard next click
		return (0);
	}
}
#endif
/******************************************************************************/
static void csize(HWND hwnd, UINT state, int cx, int cy)

{
	CFLIST * cfp;

	if (cfp = getdata(hwnd))	{
		INDEX * FF = getowner(hwnd);
		RECT crect;
		REBARBANDINFO rbBand;

		GetClientRect(hwnd,&crect);
		MoveWindow(cfp->rbwind,crect.left,crect.top,crect.right-crect.left,crect.bottom-crect.top,TRUE);	/* size rebar */
		rbBand.cbSize = sizeof(REBARBANDINFO);
		rbBand.fMask = RBBIM_CHILDSIZE|RBBIM_SIZE;
		rbBand.cxMinChild = 100;	// min height
		rbBand.cyMinChild = cx;	// min width
		rbBand.cx = cy;		// set height
		SendMessage(cfp->rbwind,RB_SETBANDINFO,0,(LPARAM)&rbBand);
		if (FF->rwind && !FF->rcwind)
			SendMessage(cfp->rbwind, RB_MAXIMIZEBAND, (WPARAM)1, TRUE);	// set to ideal size
	}
	FORWARD_WM_SIZE(hwnd,state,cx,cy,DefMDIChildProc);
}
/************************************************************************/
static HWND activechild(HWND hwnd)

{

	if (getdata(hwnd))	{	// if have any client window, and this has data
		// some messages received from within call to WM_MDIDESTROY; all fails if there's no client window
//		NSLog("data: %ld", getdata(hwnd));
		INDEX * FF = getowner(hwnd);

		if (!FF->rcwind)	{	// if no separate entry window
			if (FF->currentfocus)	// if some child had focus
				return FF->currentfocus;	// restore it
			return FF->rwind ? FF->rwind : FF->vwind;	// default to record window
		}
		return FF->vwind;
	}
	return NULL;
}
/************************************************************************/
static WNDPROC activechildproc(HWND hwnd)

{
	return (WNDPROC)GetWindowLongPtr(activechild(hwnd), GWLP_WNDPROC);
}
