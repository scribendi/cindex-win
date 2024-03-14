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

#define CX(WW,II) (((CFLIST *)GetWindowLongPtr(WW,GWLP_USERDATA))->II)	/* for addressing items */

static void ccommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);	/* does menu tasks */
static BOOL ccreate(HWND hwnd,LPCREATESTRUCT cs);		/* initializes view window */
static void csize(HWND hwnd, UINT state, int cx, int cy);
static short cclose(HWND hwnd);		/* closes index */
static void csetfocus(HWND hwnd, HWND oldfocus);	/* gets focus shift message */
static int cnotify(HWND hwnd, int id, NMHDR * hdr);	/* does notification tasks */
//static void cpnotify(HWND hwnd, UINT msg, HWND hwndChild, UINT idChild); 
static BOOL cncactivate(HWND hwnd, BOOL fActive, HWND hwndActDeact, BOOL fMinimized);	/* non-client window act */
static void cactivate(HWND hwnd, BOOL fActive, HWND hwndActivate, HWND hwndDeactivate);		/* activates window */
static int cactivateonmouse(HWND hwnd, HWND topwind, UINT hitcode, UINT msg);	/* does right kind of activation on window click */
static HWND activeclient(HWND hwnd);
static WNDPROC activeclientproc(HWND hwnd);

/*********************************************************************************/
HWND rcontainer_installrecordwindow(INDEX * FF)	/*  sets up record container window */

{
	HWND hwnd;
	TCHAR dname[MAX_PATH];

	nstrcpy(dname,FF->iname);
	PathRemoveExtension(dname);
	hwnd = CreateWindowEx(WS_EX_MDICHILD,g_rcontainerclass,dname,WS_CLIPCHILDREN,
			0,0,0,0,g_hwclient,NULL,g_hinst,FF);
	return (hwnd);
}
#if 0
/*********************************************************************************/
void rcontainer_removerecordwindow(INDEX * FF)	// removes floating record window and destroys

{
//	GetWindowRect(FF->rcwind,&FF->head.privpars.rwrect);	// save window rect
//	FF->head.privpars.rwrect.bottom += scaleForDpi(FF->rcwind,4);	// adjustment for band width 
	freedata(FF->rcwind);		/* releases window data */
	SendMessage(g_hwclient,WM_MDIDESTROY,(WPARAM)FF->rcwind,0);	/* destroy container */			
	FF->rcwind = NULL;
}
#endif
/************************************************************************/
LRESULT CALLBACK rcontainer_proc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)

{
	switch (msg)	{
		HANDLE_MSG(hwnd,WM_CREATE,ccreate);
		HANDLE_MSG(hwnd,WM_SIZE,csize);
		HANDLE_MSG(hwnd,WM_NCACTIVATE,cncactivate);
		HANDLE_MSG(hwnd,WM_MDIACTIVATE,cactivate);
		HANDLE_MSG(hwnd,WM_MOUSEACTIVATE,cactivateonmouse);
//		HANDLE_MSG(hwnd,WM_PARENTNOTIFY,cpnotify);
//		HANDLE_MSG(hwnd,WM_NOTIFY,cnotify);
		HANDLE_MSG(hwnd,WM_COMMAND,ccommand);
		HANDLE_MSG(hwnd,WM_SETFOCUS,csetfocus);
		case WM_CLOSE:
			return cclose(hwnd);
		case WMM_SETGROUP:
			view_showgroup(hwnd,(char *)lParam);
			return (0);
		case WM_INITMENUPOPUP:
//		case WMM_UPDATETOOLBARS:
		case WM_KEYDOWN:
		case WM_CHAR:
		case WM_HOTKEY:
			SendMessage(activeclient(hwnd),msg,wParam,lParam);
	}
	return (DefMDIChildProc(hwnd,msg,wParam,lParam));
}
/************************************************************************/
static void ccommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)	/* does menu tasks */

{
	switch (id)	{
		case IDM_FILE_CLOSE:
			cclose(hwnd);
			return;
		case IDM_EDIT_FIND:
			find_setwindow(hwnd);
			return;
		case IDM_EDIT_FINDAGAIN:
			fs_findagain(getowner(hwnd), g_findw);
			return;
//		case IDM_DOCUMENT_RECORDSTRUCTURE:
//			is_setstruct(hwnd, NULL);
//			return;
		case IDM_DOCUMENT_REFERENCESYNTAX:
			is_setrefsyntax(hwnd);
			return;
		case IDM_DOCUMENT_FLIPWORDS:
			is_flipwords(hwnd);
			return;
//		case IDM_TOOLS_VERIFYCROSSREFERENCES:
//			ts_verify(hwnd);
//			return;
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
	FORWARD_WM_COMMAND(activeclient(hwnd),id,hwndCtl,codeNotify,activeclientproc(hwnd));	
}
/******************************************************************************/
static BOOL ccreate(HWND hwnd,LPCREATESTRUCT cs)		/* initializes record window */

{
	CFLIST * cfp;

	if (cfp = getmem(sizeof(CFLIST)))	{	/* if can get memory for our window structure */
		INDEX * FF;
		RECT mrect;
		DWORD style;

		setdata(hwnd,cfp);		/* set memory */
		style = GetWindowLongPtr(hwnd,GWL_STYLE);
		SetWindowLongPtr(hwnd,GWL_STYLE, (LONG_PTR)(style&~(WS_MINIMIZEBOX|WS_MAXIMIZEBOX)));	/* change style */
		FF = WX(hwnd, owner) = getMDIparam(cs);	/* recover param passed (FF) */
		FF->rcwind = hwnd;
		mrect = FF->head.privpars.rwrect;	// get saved rect
		scaleRectForDpi(hwnd,&mrect);
		adjustWindowRect(hwnd,&mrect,style,FALSE,WS_EX_MDICHILD);	/* now get window rect that has right client area */
		fitrecttoclient(&mrect);
		MoveWindow(hwnd,mrect.left,mrect.top,mrect.right-mrect.left,mrect.bottom-mrect.top,FALSE);	// set position
//		if (top = isobscured(hwnd,g_findw))	/* if it's completely obscured */
//			MoveWindow(hwnd,mrect.left,mrect.top+top,mrect.right-mrect.left,mrect.bottom-mrect.top,FALSE);	// adjust position
		SetParent(FF->rwind,hwnd);
		return TRUE;
	}
	return (FALSE);
}
/**********************************************************************************/
static short cclose(HWND hwnd)		/* closes record window */

{
	INDEX * FF = getowner(hwnd);
	if (mod_close(FF->rwind,MREC_ALLOWCHECK))	// if all ok
		return TRUE;
	return FALSE;
}

/******************************************************************************/
static void csetfocus(HWND hwnd, HWND oldfocus)	/* gets focus shift message */

{
	SetFocus(activeclient(hwnd));
	FORWARD_WM_SETFOCUS(hwnd,oldfocus,DefMDIChildProc);
}
/******************************************************************************/
static BOOL cncactivate(HWND hwnd, BOOL fActive, HWND hwndActDeact, BOOL fMinimized)	/* non-client window act */

{
	if (fActive)	/* if becoming active */
		SetWindowPos(getowner(hwnd)->cwind,hwnd,0,0,0,0,SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOSIZE);
	return FORWARD_WM_NCACTIVATE(hwnd,fActive,hwndActDeact, fMinimized,DefMDIChildProc);
}
/******************************************************************************/
static void cactivate(HWND hwnd, BOOL fActive, HWND hwndActivate, HWND hwndDeactivate)		/* activates window */

{
	if (hwnd == hwndActivate)	{	/* if being activated */
//		SendMessage(activeclient(hwnd),WMM_UPDATETOOLBARS,0,0);
		if (getowner(hwnd)->cwind == hwndDeactivate)	// if from view window
			setmdiactive(hwnd,FALSE);		// clear discard first click
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
		return (MA_ACTIVATEANDEAT);
	}
	setmdiactive(hwnd,FALSE);		// clear discard first click
	return (MA_ACTIVATE);
}
/******************************************************************************/
static void csize(HWND hwnd, UINT state, int cx, int cy)

{
	CFLIST * cfp;

	if (cfp = getdata(hwnd))	{
		INDEX * FF = getowner(hwnd);
		RECT crect;

		GetClientRect(hwnd,&crect);
		MoveWindow(FF->rwind,crect.left,crect.top,crect.right-crect.left,crect.bottom-crect.top,TRUE);	// record window
	}
	FORWARD_WM_SIZE(hwnd,state,cx,cy,DefMDIChildProc);
}
/************************************************************************/
static HWND activeclient(HWND hwnd)

{
	if (getdata(hwnd))	{	// this is a way round handling messages receive after we've closed
		// some messages receive from within call to WM_MDIDESTROY
		return getowner(hwnd)->rwind;
	}
	return NULL;
}
/************************************************************************/
static WNDPROC activeclientproc(HWND hwnd)

{
	return (WNDPROC)GetWindowLongPtr(activeclient(hwnd), GWLP_WNDPROC);
}
