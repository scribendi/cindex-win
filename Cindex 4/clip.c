#include "stdafx.h"
#include "commands.h"
#include "util.h"
#include "text.h"
#include "clip.h"

static LRESULT CALLBACK clipproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static void ccommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);	/* does command tasks */
static int cnotify(HWND hwnd, int id, NMHDR * hdr);	/* does notification tasks */
static void cdomenu(HWND hwnd, HMENU hMenu, UINT item, BOOL fSystemMenu);	/* sets menus */
static void cchange(HWND hwnd, HWND hwndRemove, HWND hwndNext);	/* handles change in viewer chain */
static void cdraw(HWND hwnd);	/* handles content change */
static long cclose(HWND hwnd);		/* closes clipboard window */
/*******************************************************************************/
void clip_setwindow(void)			/* sets up clipboard window */

{
	RECT trect;
	TEXTPARS tpars;

	if (!g_clipw)	{
		SetRect(&trect,0,0,400,200);
		memset(&tpars,0,sizeof(tpars));
		tpars.hook = clipproc;
		tpars.hwp = &g_clipw;
		tpars.style = 0;
		if (txt_setwindow(TEXT("Clipboard"),&trect,&tpars))	{
			EX(g_clipw,handle1) = SetClipboardViewer(g_clipw);	/* set us as viewer */
			SendMessage(EX(g_clipw,hwed),EM_SETEVENTMASK,0,ENM_KEYEVENTS|ENM_MOUSEEVENTS);	/* set event mask */		
		}
	}
	SendMessage(g_hwclient,WM_MDIACTIVATE,(WPARAM)g_clipw,0);	/* make sure it's active */
}
/************************************************************************/
static LRESULT CALLBACK clipproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)

{
	switch (msg)	{
		HANDLE_MSG(hwnd,WM_COMMAND,ccommand);
		HANDLE_MSG(hwnd,WM_NOTIFY,cnotify);
		HANDLE_MSG(hwnd,WM_INITMENUPOPUP,cdomenu);
		HANDLE_MSG(hwnd,WM_CHANGECBCHAIN,cchange);	/* clipboard change */
		HANDLE_MSG(hwnd,WM_DRAWCLIPBOARD,cdraw);	/* need redraw */
		case (WM_CLOSE):
			return (cclose(hwnd));
	}
	return (txt_proc(hwnd,msg,wParam,lParam));	/* default handling */
}
/************************************************************************/
static void ccommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)	/* does command tasks */

{
#if 0
	if (!hwndCtl)	{	/* if menu */
		switch (id)	{
				return;
		}
		return;
	}
#endif
	FORWARD_WM_COMMAND(hwnd,id,hwndCtl,codeNotify,txt_proc);
}
/************************************************************************/
static int cnotify(HWND hwnd, int id, NMHDR * hdr)	/* does notification tasks */

{
	if (hdr->code == EN_MSGFILTER)	{	
		switch (((MSGFILTER *)hdr)->msg)	{
			case WM_CHAR:	/* discard all key and mouse events */
			case WM_KEYDOWN:
				return TRUE;
			case WM_LBUTTONDOWN:
			case WM_LBUTTONDBLCLK:
				return TRUE;
			default:
				break;
		}
	}
	return FORWARD_WM_NOTIFY(hwnd,id,hdr,txt_proc);	/* default notify handler */
}
/************************************************************************/
static void cdomenu(HWND hwnd, HMENU hMenu, UINT item, BOOL fSystemMenu)	/* sets menus */

{
	com_setenable(ETEXT,XONLY,ON);		/* enable set for text */
}
/************************************************************************/
static void cchange(HWND hwnd, HWND hwndRemove, HWND hwndNext)	/* handles change in viewer chain */

{
	if (hwndRemove != hwndNext)		/* if not removing next window in chain */
		SendMessage(EX(hwnd,handle1),WM_CHANGECBCHAIN,(WPARAM)hwndRemove, (LPARAM)hwndNext);
	else
		EX(hwnd,handle1) = hwndNext;
}
/************************************************************************/
static void cdraw(HWND hwnd)	/* handles content change */

{
	HWND ew;
	CHARRANGE cr;

	SendMessage(EX(hwnd,handle1),WM_DRAWCLIPBOARD,0,0);
	ew = EX(hwnd,hwed);
	cr.cpMin = 0;
	cr.cpMax = -1;
	SendMessage(ew,EM_EXSETSEL,0,(LPARAM)&cr);
	SendMessage(ew,WM_CLEAR,0,0);
	SendMessage(ew,WM_PASTE,0,0);
}
/******************************************************************************/
static long cclose(HWND hwnd)		/* closes clipboard window */

{
	ChangeClipboardChain(hwnd,EX(hwnd,handle1));
	FORWARD_WM_CLOSE(hwnd,txt_proc);
	return (TRUE);
}