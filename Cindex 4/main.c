#include "stdafx.h"
#include <shlobj.h>
#include <htmlhelp.h>
#include <winsparkle.h>
#include "commands.h"
#include "errors.h"
#include "files.h"
#include "text.h"
#include "modify.h"
#include "indexset.h"
#include "viewset.h"
#include "formstuff.h"
#include "print.h"
#include "sortset.h"
#include "formset.h"
#include "edit.h"
#include "util.h"
#include "tagstuff.h"
#include "abbrev.h"
#include "spell.h"
#include "clip.h"
#include "keytool.h"
#include "macros.h"
#include "registry.h"
#include "dde.h"
#include "containerset.h"
#include "rcontainerset.h"
#include "httpservice.h"
#include "collate.h"
#include "index.h"

#if 0
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#endif

static const DWORD wh_userid[] = {
	IDC_USERID, HIDC_USERID,
#ifdef PUBLISH
	IDC_PASSWORD,HIDC_PASSWORD,
#endif //PUBLISH
	0,0
};
#define BASESEPARATOR 200

enum {		/* frame window button image ids */
	FB_NEW = 0,
	FB_LABEL,
	FB_DELETE,
	FB_ALLRECS,
	FB_FULLFORMAT,
	FB_DRAFTFORMAT,
	FB_INDENTED,
	FB_RUNIN,
	FB_SORTALPHA,
	FB_SORTPAGE,
	FB_SORTNONE,

	FB_FTOTAL	/* total number of buttons in FRAME set */
};

enum {
	STANDARDIMAGELIST = 0,
	CINDEXIMAGELIST = 1,
};
static TBBUTTON tbb[] = {
	{BASESEPARATOR,0,0,BTNS_SEP,{0},0,-1},
#ifndef READER
	{MAKELONG(STD_FILENEW,STANDARDIMAGELIST),IDM_FILE_NEW,TBSTATE_ENABLED,BTNS_BUTTON,{0},0,0},
#endif	//READER
	{MAKELONG(STD_FILEOPEN,STANDARDIMAGELIST),IDM_FILE_OPEN,TBSTATE_ENABLED,BTNS_BUTTON,{0},0,0},
	{MAKELONG(STD_FILESAVE,STANDARDIMAGELIST),IDM_FILE_SAVE,0,BTNS_BUTTON,{0},0,0},
	{MAKELONG(STD_PRINT,STANDARDIMAGELIST),IDM_FILE_PRINT,0,BTNS_BUTTON,{0},0,0},
	{0,0,0,BTNS_SEP,{0},0,-1},
#ifndef READER
	{MAKELONG(STD_CUT,STANDARDIMAGELIST),IDM_EDIT_CUT,0,BTNS_BUTTON,{0},0,0},
#endif	//READER
	{MAKELONG(STD_COPY,STANDARDIMAGELIST),IDM_EDIT_COPY,0,BTNS_BUTTON,{0},0,0},
#ifndef READER
	{MAKELONG(STD_PASTE,STANDARDIMAGELIST),IDM_EDIT_PASTE,0,BTNS_BUTTON,{0},0,0},
	{0,0,0,BTNS_SEP,{0},0,-1},
#endif	//READER
	{MAKELONG(STD_FIND,STANDARDIMAGELIST),IDM_EDIT_FIND,0,BTNS_BUTTON,{0},0,0},
#ifndef READER
	{MAKELONG(STD_REPLACE,STANDARDIMAGELIST),IDM_EDIT_REPLACE,0,BTNS_BUTTON,{0},0,0},
	{0,0,0,BTNS_SEP,{0},0,-1},
	{MAKELONG(FB_NEW,CINDEXIMAGELIST),IDM_EDIT_NEWRECORD,0,BTNS_BUTTON,{0},0,0},
	{0,0,TBSTATE_ENABLED,BTNS_SEP,{0},0,0},
	{MAKELONG(FB_LABEL,CINDEXIMAGELIST),IDM_EDIT_TAG,TBSTATE_ENABLED,BTNS_WHOLEDROPDOWN,{0},0,0},
	{MAKELONG(FB_DELETE,CINDEXIMAGELIST),IDM_EDIT_DELETE,TBSTATE_ENABLED,BTNS_CHECK,{0},0,0},
#endif //READER
	{0,0,0,BTNS_SEP,{0},0,-1},
	{MAKELONG(FB_ALLRECS,CINDEXIMAGELIST),IDM_VIEW_ALLRECORDS,0,BTNS_BUTTON,{0},0,0},
	{0,0,0,BTNS_SEP,{0},0,-1},
	{MAKELONG(FB_FULLFORMAT,CINDEXIMAGELIST),IDM_VIEW_FULLFORMAT,0,BTNS_CHECKGROUP,{0},0,0},
	{MAKELONG(FB_DRAFTFORMAT,CINDEXIMAGELIST),IDM_VIEW_DRAFTFORMAT,0,BTNS_CHECKGROUP,{0},0,0},
	{0,0,0,BTNS_SEP,{0},0,-1},
	{MAKELONG(FB_INDENTED,CINDEXIMAGELIST),IDB_VIEW_INDENTED,0,BTNS_CHECKGROUP,{0},0,0},
	{MAKELONG(FB_RUNIN,CINDEXIMAGELIST),IDB_VIEW_RUNIN,0,BTNS_CHECKGROUP,{0},0,0},
	{0,0,0,BTNS_SEP,{0},0,-1},
#ifndef READER
	{MAKELONG(FB_SORTALPHA,CINDEXIMAGELIST),IDB_VIEW_SORTALPHA,0,BTNS_CHECKGROUP,{0},0,0},
	{MAKELONG(FB_SORTPAGE,CINDEXIMAGELIST),IDB_VIEW_SORTPAGE,0,BTNS_CHECKGROUP,{0},0,0},
	{MAKELONG(FB_SORTNONE,CINDEXIMAGELIST),IDB_VIEW_SORTNONE,0,BTNS_CHECK,{ 0 },0,0 },
#endif //READER
};

#ifdef READER
#define ADDSIZE 8		/* buttons not in base set */
#define SB_TOTAL 15		/* total # buttons in standard set */
#define	VB_TOTAL 5		/* total number of buttons in our view set */
#else
#define ADDSIZE 16		/* buttons not in base set */
#define SB_TOTAL 15		/* total # buttons in standard set */
#define	VB_TOTAL 10		/* total number of buttons in our view set */
#endif //READER

#define TBBSIZE (sizeof(tbb)/sizeof(TBBUTTON))	/* total size of array */


static struct ttmatch ttstrings[] = {		/* tooltip strings */
	{IDM_FILE_NEW,TEXT("New (Ctrl+N)")},
	{IDM_FILE_OPEN,TEXT("Open (Ctrl+O)")},
	{IDM_FILE_SAVE,TEXT("Save (Ctrl+S)")},
	{IDM_FILE_PRINT,TEXT("Print (Ctrl+P)")},
	{IDM_EDIT_CUT,TEXT("Cut (Ctrl+X)")},
	{IDM_EDIT_COPY,TEXT("Copy (Ctrl+C)")},
	{IDM_EDIT_PASTE,TEXT("Paste (Ctrl+V)")},
	{IDM_EDIT_FIND,TEXT("Find (Ctrl+F)")},
	{IDM_EDIT_REPLACE,TEXT("Replace (Ctrl+R)")},
	{IDM_EDIT_NEWRECORD,TEXT("New Record (Ctrl+K)")},
	{IDM_EDIT_TAG,TEXT("Label/Unlabel")},
	{IDM_EDIT_DELETE,TEXT("Delete/Restore (Ctrl+D)")},
	{IDM_VIEW_ALLRECORDS,TEXT("All Records (Ctrl+M)")},
	{IDM_VIEW_FULLFORMAT,TEXT("Fully Formatted (Ctrl+H)")},
	{IDM_VIEW_DRAFTFORMAT,TEXT("Draft Formatted (Ctrl+J)")},
	{IDB_VIEW_INDENTED,TEXT("Indented Style")},
	{IDB_VIEW_RUNIN,TEXT("Run-in Style")},
	{IDB_VIEW_SORTALPHA,TEXT("Sort Alphabetically (Ctrl+Shift+A)")},
	{IDB_VIEW_SORTPAGE,TEXT("Sort by Locator (Ctrl+Shift+L)")},
	{IDB_VIEW_SORTNONE,TEXT("Unsorted (Ctrl+Shift+U)")}
};
#define TTMAX (sizeof(ttstrings)/sizeof(struct ttmatch))

struct cmstruct{
	PROCESS_INFORMATION * pip;
	BOOL active;
};

struct ttcont {
	HWND *hwnd;
	TCHAR * text;
};
static struct ttcont ttcontrols[] = {
	&g_hwfcb,TEXT("Font (Ctrl+Shift+F)"),		/* font combo */
	&g_hwscb,TEXT("Size (Ctrl+Shift+S)"),		/* font size combo */
	&g_hwscbed,TEXT("Size (Ctrl+Shift+S)")	/* font size combo (text window) */
};
#define TOOLCONTROLS (sizeof(ttcontrols)/sizeof(struct ttcont))

static int statwidths[] = {	/* widths of segments of status window */
	210,
	280,
	400,	// sort state
	500,
	-1
};
#define STAT_NPARTS (sizeof(statwidths)/sizeof(int))

static HWND hwsplash;
static PROCESS_INFORMATION m_charmap;
static UINT_PTR m_timerid;		/* splash timer id */
static char *m_cmdline;		// command line */
static int m_cmdshow;		// show status
static WNDPROC m_swproc;	/* size combo box window proc */
static LRESULT (CALLBACK *clientproc)(HWND, UINT, WPARAM, LPARAM );	/* saved client procedure */
static BOOL calledUpdateClosed;
static HIMAGELIST standardImagelist, cindexImageList;	// toolbar image list


static TCHAR *szframename = TEXT("cdx_frame$");	/* name of frame class (plus menu & accel) */

static LRESULT CALLBACK frameproc (HWND, UINT, WPARAM, LPARAM);
//static void quitrequest(void);
static void dofcommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);	/* sets menus */
static int dofnotify(HWND hwnd, int id, NMHDR * hdr);	/* does notification tasks */
static BOOL dofcreate(HWND hwnd, LPCREATESTRUCT cs);
static void dofactivate(HWND hwnd, UINT state, HWND hwndActDeact, BOOL fMinimized);	/* activates/deactivates */
static LRESULT CALLBACK sizecb(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);	/* callback for font size list */
static void dofclose(HWND hwnd);
static void dofsize(HWND hwnd, UINT state, int cx, int cy);
static void dofsyscolor(void);
static void domenuselect(WPARAM wParam, LPARAM lParam);	/* shows menu help text in status bar */
static void dohotkey(WPARAM wParam, LPARAM lParam);	/* filters hot keys */
static void dofmenu(HWND hwnd, HMENU hMenu, UINT item, BOOL fSystemMenu);	/* sets menus */
static void dofcontmenu(HWND hwnd, HWND hwndContext, UINT xpos, UINT ypos);	/* does context menus */
static BOOL initcindex (void);	/* initializes, registers classes */
static BOOL getuserid(void);	/* gets user id */
static INT_PTR CALLBACK userproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static void displaysplash(void);	/* displays splash */
static INT_PTR CALLBACK splashproc(HWND hwnd, UINT msg, WPARAM wParam,LPARAM lParam);
static void CALLBACK timeproc(HWND hwnd, UINT msg, UINT_PTR timer, DWORD time);	/* destroys splash */
static void displayframe(void);	// shows frame window
static void showcharactermap(void);		// displays character map
static BOOL testsernum(char * sernum);	/* tests serial number string */
static LRESULT CALLBACK chook(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static BOOL CALLBACK xfind(HWND hwnd,LPARAM param);
static HWND createtoolbar(HWND hWndParent);

/************************************************************************/
 static int canquitforupdate()

{
	return index_byindex(0) ? FALSE : TRUE;	//FALSE if any index open
}
/************************************************************************/
 static void quitforupdate()

{
	DWORD mainthread = GetWindowThreadProcessId(g_hwframe,NULL);
	calledUpdateClosed = TRUE;
}
/****************************************************************/
int WINAPI WinMain(HINSTANCE hinstance, HINSTANCE previnstance, PSTR cmdline, int cmdshow)

{
	MSG msg;
	HACCEL accel;
	TCHAR path[MAX_PATH];
	DWORD vsize = MAX_PATH;
	WINDOWPLACEMENT wp;
	DWORD cookie = 0;
	char * uptr;

	g_hinst = hinstance;
	m_cmdline = cmdline;
	m_cmdshow = cmdshow;

	wp.length = sizeof(wp);

//	g_version.dwOSVersionInfoSize = sizeof(g_version);
	dofsyscolor();	// configure selection highlight
//	GetVersionEx(&g_version);
#if TOPREC == RECLIMIT && !READER && !_DEBUG			/* check registration & spelling if not demo or reader */
	if (!reg_getmachinekeyvalue(NULL,TEXT("Sernum"),path,&vsize) || !testsernum(fromNative(path)))	{	/* if got value from registry */
		MessageBox(NULL,TEXT("This is an unregistered copy of Cindex."),TEXT("Cindex 4.0"),MB_OK);
		return (FALSE);
	}
#endif
//	if (g_version.dwMajorVersion < 5 || g_version.dwMajorVersion == 5 && (g_version.dwMinorVersion == 0 || *(g_version.szCSDVersion+nstrlen(g_version.szCSDVersion)-1) < '2'))	{	/* if can't run */
//		senderr(ERR_WVERSIONERR,WARN);
//		return FALSE;
//	}
	dde_setup();		/* sets up ddeml */
#if 0			/* keep this until we can find the right check for  multiple copies */
	if (dde_sendcheck())	{	/* if this copy is running anywhere */
		senderr(ERR_DUPCOPY,WARN);
		dde_close();	/* closes ddeml */
		return (FALSE);
	}
#endif
	if (!initcindex())			/* if can't initialize */
		return FALSE;
	HWND hh = HtmlHelp(NULL,NULL,HH_INITIALIZE,(DWORD_PTR)&cookie);
	if (file_loadconfig(GetAsyncKeyState(VK_SHIFT) < 0))	{	/* if can load configuration */
//		makecheckbitmaps();
		com_setdefaultbuttons(TRUE);
		makelabelbitmaps();	// set label menu bitmaps
#ifdef PUBLISH
		if (uptr = strstr(m_cmdline,"/user:"))	{	// if have user id override [__argv[1]
			*uptr++ = '\0';		// terminate command line before user id
			uptr = strchr(uptr,':')+1;
			strncpy(g_prefs.hidden.user,uptr,4);
		}
		if (uptr || (!g_admin.requireid && !*g_admin.psswd || getuserid()))	{	/* if don't want userid or got it */
#else
		if (!g_prefs.gen.setid || getuserid())	{	/* if don't want userid or got it */
#endif //PUBLISH
			accel = LoadAccelerators(hinstance,szframename);
#if 0
			if (reg_getkeyvalue(K_GENERAL,FRAME,&wp,&wp.length) && wp.length == sizeof(WINDOWPLACEMENT))	{// if have size/pos from registry 
				if (g_prefs.gen.maxcin)
					 wp.showCmd = SW_SHOWMAXIMIZED;
				SetWindowPlacement(g_hwframe,&wp);
			}
			else
				ShowWindow(g_hwframe,g_prefs.gen.maxcin ? SW_MAXIMIZE : cmdshow);
#endif
			http_connect(TRUE);		//  silent check
			reg_setkeyvalue(K_UPDATES,TEXT("CheckForUpdates"),REG_SZ, g_prefs.gen.autoupdate ? TEXT("1") : TEXT("0"),1);
			hwsplash = CreateDialog(g_hinst,MAKEINTRESOURCE(IDD_SPLASH),g_hwframe,splashproc);
			OleInitialize(NULL);
			ucnv_setDefaultName("UTF-8");	// set default code page to UTF-8
			if (file_getuserpath(path,ALIS_ABBREV))	/* if can get spec of default abbrev file */
				abbrev_open(path, FALSE);
#ifdef PUBLISH
			if (file_getmachinepath(path,ALIS_CDXDIR) ||	/* if have default file path */
#else
			if (file_getuserpath(path,ALIS_DEFAULTDIR) ||	// if have default file path 
#endif //PUBLISH
				(SHGetFolderPath(NULL,CSIDL_PERSONAL,NULL,SHGFP_TYPE_CURRENT,path) == S_OK))	// or path to docs folder
				SetCurrentDirectory(path);
			UpdateWindow(g_hwframe);
#if TOPREC == RECLIMIT
			win_sparkle_set_registry_path(K_WINSPARKLE);
#ifdef PUBLISH
			CreateMutex(NULL, FALSE, TEXT("CindexPublisherMutex"));	// so that INNO can detect if version running
			win_sparkle_set_appcast_url("https://www.example.com/path_to_appcast.xml");
#else
			CreateMutex(NULL, FALSE, TEXT("CindexMutex"));	// so that INNO can detect if version running
			win_sparkle_set_appcast_url("https://www.example.com/path_to_appcast.xml");
#endif
			win_sparkle_set_can_shutdown_callback(canquitforupdate);
			win_sparkle_set_shutdown_request_callback(quitforupdate);
			win_sparkle_init();
#endif
			while (GetMessage(&msg,NULL, 0,0))	{	/* while not exit */
				EXCEPTION_POINTERS * ep;
				__try {
					if (calledUpdateClosed)	{	// set by sparkle callback
						PostQuitMessage(0);
						continue;
					}
					if (msg.message == WM_KEYDOWN && msg.wParam == VK_CANCEL)	{	// want cancel macro
						if (mcr_cancel())	/* if were recording */
							mcr_setname(g_hwframe);	/* name it */
						continue;		/* discard message */
					}
					if (!IsWindow(g_mdlg) || !IsDialogMessage(g_mdlg,&msg))	{	/* if not for modeless */
						if (!TranslateMDISysAccel(g_hwclient,&msg) && !TranslateAccelerator(g_hwframe,accel,&msg))	{
							TranslateMessage(&msg);
							DispatchMessage(&msg);
						}
					}
					if (!index_front() && !SendMessage(g_hwstatus,SB_ISSIMPLE,0,0))
						com_setdefaultbuttons(TRUE);
				}
				__except ((ep = GetExceptionInformation()) && (GetExceptionCode() == ERR_DAMAGEDRECORD ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)){
					senderr(ERR_DAMAGEDRECORD,WARN,ep->ExceptionRecord->ExceptionInformation[0]);		/* if error */
				}
			}
#if TOPREC == RECLIMIT
			win_sparkle_cleanup();
#endif
			OleFlushClipboard();
			OleUninitialize();
		}
		file_saveconfig();		/* save configuration */
	}
	dde_close();	/* closes ddeml */
	type_releasefonts();	/* deletes all fonts we've accumulated */
	TerminateProcess(m_charmap.hProcess,0);
	HtmlHelp(NULL,NULL,HH_UNINITIALIZE,(DWORD_PTR)&cookie);

	if (GetWindowPlacement(g_hwframe,&wp))	{	// get window state
		if (wp.showCmd == SW_SHOWMINIMIZED)
			wp.showCmd = SW_SHOWNORMAL;
		reg_setkeyvalue(K_GENERAL,FRAME,REG_BINARY,&wp,wp.length);	// save
	}
	return (msg.wParam);
}
/************************************************************************/
static LRESULT CALLBACK frameproc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)

{
	switch (msg)	{
		HANDLE_MSG(hwnd,WM_CREATE,dofcreate);
//		HANDLE_MSG(hwnd,WM_COMMAND,dofcommand);
		HANDLE_MSG(hwnd,WM_NOTIFY,dofnotify);
		HANDLE_MSG(hwnd,WM_SIZE,dofsize);
		HANDLE_MSG(hwnd,WM_CLOSE,dofclose);
		HANDLE_MSG(hwnd,WM_INITMENUPOPUP,dofmenu);
		HANDLE_MSG(hwnd,WM_CONTEXTMENU,dofcontmenu);
		HANDLE_MSG(hwnd,WM_ACTIVATE,dofactivate);
		case WM_MENUSELECT:
			domenuselect(wParam, lParam);
			return(0);
		case WM_HOTKEY:
			dohotkey(wParam,lParam);
			return (0);
		case WM_HELP:
			dowindowhelp(TEXT("base\\Cindex Help.htm"));	/* null context; gets default window */
			return (0);
		case WM_SYSCOLORCHANGE:
			dofsyscolor();
			break;
		case WM_COMMAND:
			dofcommand(hwnd, LOWORD(wParam), (HWND)(lParam), (UINT)HIWORD(wParam));
			break;
		case WM_ACTIVATEAPP:
			if (wParam)	{
				HWND hwnd = getactivemdi(TRUE);
				if (hwnd)
					setmdiactive(hwnd,FALSE);		// set to allow first click
				hk_register(&g_prefs);	/* register hot keys */
			}
			else	{
				HWND hwnd = getactivemdi(TRUE);
				if (hwnd)
					setmdiactive(hwnd,TRUE);		// set to discard first click
				hk_unregister();		/* unregister hot keys */
			}
			break;
#if 0
		case WM_MOUSEWHEEL:
			{
				POINT pt;
				HWND hwc;

				pt.x = LOWORD(lParam);
				pt.y = HIWORD(lParam);
				hwc = ChildWindowFromPoint(hwnd,pt);
				return SendMessage(hwc,msg,wParam,lParam);
			}
#endif
	}
	return (DefFrameProc(hwnd,g_hwclient,msg,wParam,lParam));
}
/************************************************************************/
static void dofcommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)	/* sets menus */

{
	HWND mwind;
	COMGROUP * comptr;
		
	mwind = getactivemdi(TRUE);
	if (comptr = com_findcommand(id))	{	/* if we know the command */
		if (mwind && comptr->mkeys&WPASS)	{	/* window and want passed */
			time(&g_comstarttime);				/* read current time in secs */
			if (comptr->flst)			/* if using command function */
				(*comptr->flst)(mwind,id,hwndCtl,codeNotify);	/* do it */
			else
				FORWARD_WM_COMMAND(mwind,id,hwndCtl,codeNotify,SendMessage);	/* send it */
			return;
		}
		switch (id)	{
#ifdef PUBLISH
			case IDM_FILE_STATIONERY:
				file_openstationery();
				return;
#endif //PUBLISH
			case IDM_FILE_SAVEFORMAT:
				file_savestylesheet(NULL);
				return;
			case IDM_FILE_RECENT1:
			case IDM_FILE_RECENT2:
			case IDM_FILE_RECENT3:
			case IDM_FILE_RECENT4:
			case IDM_FILE_RECENT5:
			case IDM_FILE_RECENT6:
			case IDM_FILE_RECENT7:
			case IDM_FILE_RECENT8:
			case IDM_FILE_RECENT9:
			case IDM_FILE_RECENT10:
#ifdef PUBLISH
			case IDM_FILE_RECENT11:
			case IDM_FILE_RECENT12:
#endif //PUBLISH
				file_openindex(com_findrecent(id-IDM_FILE_RECENT1),OP_VISIBLE);
				return;
			case IDM_FILE_EXIT:
				dofclose(NULL);
				return;
			case IDM_EDIT_PREFERENCES:
				edit_preferences();
				return;
//			case IDM_VIEW_ABBREVIATIONS:
//				abbrev_view();
//				return;
			case IDM_DOCUMENT_MARGINSCOLUMNS:
				fs_margcol(NULL);
				return;
			case IDM_DOCUMENT_HEADERSFOOTER:
				fs_headfoot(NULL);
				return;
			case IDM_DOCUMENT_GROUPINGENTRIES:
				fs_entrygroup(NULL);
				return;	
			case IDM_DOCUMENT_STYLELAYOUT:
				fs_stylelayout(NULL);
				return;	
			case IDM_DOCUMENT_HEADINGS:
				fs_headings(NULL);
				return;
			case IDM_DOCUMENT_CROSSREFERENCES:
				fs_crossrefs(NULL);
				return;			
			case IDM_DOCUMENT_PAGEREFERENCES:
				fs_pagerefs(NULL);
				return;			
			case IDM_DOCUMENT_STYLEDSTRINGS:
				fs_styledstrings(NULL);
				return;			
			case IDM_DOCUMENT_RECORDSTRUCTURE:
				is_setstruct(NULL, &g_prefs.indexpars);
				return;
			case IDM_DOCUMENT_REFERENCESYNTAX:
				is_setrefsyntax(NULL);
				return;
			case IDM_DOCUMENT_FLIPWORDS:
				is_flipwords(NULL);
				return;
			case IDM_TOOLS_SORT:
				ss_sortindex(NULL);
				return;
			case IDM_TOOLS_ABBREV_ATTACH:
				file_openabbrev();
				return;
			case IDM_TOOLS_ABBREV_DETACH:
				abbrev_close(hwnd);
				return;
			case IDM_TOOLS_ABBREV_NEW:
				file_newabbrev();
				return;
			case IDM_TOOLS_ABBREV_EDIT:
				abbrev_view();
				return;
			case IDM_TOOLS_HOTKEYS:
				hk_setwindow();
				return;
			case IDM_TOOLS_MARKUPTAGS:
				ts_managetags(NULL);
				return;
			case IDM_WINDOW_CLIPBOARD:
				clip_setwindow();
				return;
			case IDM_WINDOW_CASCADE:
				SendMessage(g_hwclient,WM_MDICASCADE,0,0);
				return;
			case IDM_WINDOW_TILE:
				SendMessage(g_hwclient,WM_MDITILE,MDITILE_VERTICAL,0);
				return;
			case IDM_FONT_SYMBOLS:
				showcharactermap();
				return;
			case IDM_HELP_TOPICS:
				dowindowhelp(TEXT("base\\Cindex Help.htm"));
				return;
			case IDM_HELP_CHECKFORUPDATES:
				win_sparkle_check_update_with_ui();
				return;
			default:
				if (id >= IDM_MACRO_1 && id <= IDM_MACRO_10)	{
					mcr_record(id-IDM_MACRO_1);
					return;
				}
				else if (id >= IDM_PMACRO_1 && id <= IDM_PMACRO_10)	{
					mcr_play(id-IDM_PMACRO_1);
					return;
				}
				else if (*comptr->flst)		{	/* temp until finally fix command dispatch */
					(*comptr->flst)(mwind,id,hwndCtl,codeNotify);	/* do it */
					return;
				}
		}
	}
	DefFrameProc(hwnd,g_hwclient,WM_COMMAND,MAKEWPARAM((UINT)id,(UINT)codeNotify),(LPARAM)hwndCtl);	/* cracker doesn't work for DefFrame.. */
}
/************************************************************************/
static int dofnotify(HWND hwnd, int id, NMHDR * hdr)	/* does notification tasks */

{
	int ttindex;

	switch (hdr->code)	{
		case TTN_NEEDTEXT:
			for (ttindex = 0; ttindex < TTMAX; ttindex++)	{	/* while not found our string */
				if (ttstrings[ttindex].id == ((LPTOOLTIPTEXT)hdr)->hdr.idFrom)	{
					((LPTOOLTIPTEXT)hdr)->lpszText = ttstrings[ttindex].string;
					return TRUE;
				}
			}
			return TRUE;
		case TBN_DROPDOWN:
			{
			LPNMTOOLBAR tb = (LPNMTOOLBAR)hdr;
//			HMENU pops = LoadMenu(g_hinst,MAKEINTRESOURCE(IDR_POPUPS));
			HMENU lmenu = GetSubMenu(g_popmenu,3);
			TPMPARAMS tbm;
			RECT rc;

			SendMessage(tb->hdr.hwndFrom, TB_GETRECT,(WPARAM)tb->iItem, (LPARAM)&rc);
			MapWindowPoints(tb->hdr.hwndFrom, HWND_DESKTOP,(LPPOINT)&rc,2);
			tbm.cbSize = sizeof(TPMPARAMS);
			tbm.rcExclude = rc;
			TrackPopupMenuEx(lmenu,TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_VERTICAL,               
				rc.left, rc.bottom, hwnd, &tbm); 
//			DestroyMenu(lmenu);
			}
			return TBDDRET_DEFAULT;
		case TBN_QUERYINSERT:
			if (((LPNMTOOLBAR)hdr)->iItem < 1)	/* if separators for combo boxes */
				return FALSE;
			return TRUE;
		case TBN_QUERYDELETE:
			if (((LPNMTOOLBAR)hdr)->iItem < 1)	/* if separators for combo boxes */
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
			com_tbsaverestore(g_hwtoolbar,hwnd,TRUE, F_TBREGISTERC);	/* saves */
			return TRUE;
		case TBN_RESET:
			com_tbsaverestore(g_hwtoolbar,hwnd,FALSE, F_TBREGISTERD);	/* recover default */
			return TRUE;
		case TBN_CUSTHELP:
			dowindowhelp(TEXT("Toolbars"));
			return TRUE;
		case TBN_RESTORE:
			if (((LPNMTBRESTORE)hdr)->iItem == 0)	// if first button (big separator)
				((LPNMTBRESTORE)hdr)->tbButton.iBitmap = scaleForDpi(hwnd, BASESEPARATOR);
			return 0;
	}
	return DefFrameProc(hwnd,g_hwclient,WM_NOTIFY,(WPARAM)id,(LPARAM)hdr);	/* cracker doesn't work for DefFrame.. */
}
/************************************************************************/
static BOOL dofcreate(HWND hwnd, LPCREATESTRUCT cs)

{
	CLIENTCREATESTRUCT clstruct;
//	NONCLIENTMETRICS ncm;
	HMENU wmenu;
	MENUITEMINFO mi;
	int count /* bindex */;
//	TBADDBITMAP tbvb;
	TOOLINFO ti;
	HDC dc;
	HWND hwtt;
	MENUINFO mmi;

	mmi.cbSize = sizeof(mmi);
	mmi.fMask = MIM_MENUDATA;
	mi.cbSize = sizeof(mi);
	mi.fMask = MIIM_ID;

	dc = GetDC(hwnd);
	g_slpix = GetDeviceCaps(dc, LOGPIXELSY);	/* save logical pixels for screen */
	ReleaseDC(hwnd, dc);
	HFONT mfont = CreateFontIndirect(&getconfigdpi(hwnd)->lfMenuFont);
	g_popmenu = LoadMenu(g_hinst,MAKEINTRESOURCE(IDR_POPUPS));
	for (count = 0;count+IDPM_MODCONTEXT <= IDPM_LABEL;count++)	{	/* for all menus in bar */
		HMENU submenu = GetSubMenu(g_popmenu,count);

		mmi.dwMenuData = IDPM_MODCONTEXT+count;		// set id as user data
		SetMenuInfo(submenu,&mmi);
	}
	wmenu = GetSubMenu(g_popmenu,count);	// get last popup menu (label button dropdown)
	SetMenuInfo(wmenu,&mmi);			// set it to have same ID as preceding label popup

	wmenu = GetMenu(hwnd);
	for (count = 0;count+IDPM_FILE <= IDPM_HELP;count++)	{	/* for all menus in main bar */
		HMENU submenu = GetSubMenu(wmenu,count);

		mi.wID = IDPM_FILE+count;			/* set id (can't do through resource editor) */
		SetMenuItemInfo(wmenu,count,TRUE,&mi);
		mmi.dwMenuData = IDPM_FILE+count;		// set id as user data
		SetMenuInfo(submenu,&mmi);
	}

//	count =  GetMenuItemCount(GetSubMenu(wmenu,5));	// get tools menu
	mi.fMask = MIIM_SUBMENU|MIIM_ID;
	GetMenuItemInfo(GetSubMenu(wmenu,5),14,TRUE,&mi);	// get abbrev item from tools menu
	mi.wID = IDPM_ABBREVIATIONS;
	SetMenuItemInfo(GetSubMenu(wmenu,5),14,TRUE,&mi);	// set its id
	mmi.dwMenuData = IDPM_ABBREVIATIONS;		// id of abbrev submenu
	SetMenuInfo(mi.hSubMenu,&mmi);				// set id on submenu

	mi.fMask = MIIM_SUBMENU;		
	mi.hSubMenu = CreatePopupMenu();	// create group hierarchical menu
	SetMenuItemInfo(wmenu,IDM_VIEW_GROUP,FALSE,&mi);
	mmi.dwMenuData = IDPM_VIEWGROUP;
	SetMenuInfo(mi.hSubMenu,&mmi);

	mi.hSubMenu = CreatePopupMenu();
	SetMenuItemInfo(wmenu,IDM_VIEW_VIEWDEPTH,FALSE,&mi);
	mmi.dwMenuData = IDPM_VIEWDEPTH;
	SetMenuInfo(mi.hSubMenu,&mmi);

	mi.hSubMenu = CreatePopupMenu();
	SetMenuItemInfo(wmenu,IDM_EDIT_TAG,FALSE,&mi);
	mmi.dwMenuData = IDPM_LABEL;
	SetMenuInfo(mi.hSubMenu,&mmi);
	for (count = 0; count < FLAGLIMIT; count++)	{	/* build boolean menus */
		char text[30];
		sprintf(text,count ? "Label &%d\tCtrl+Shift+%d" : "&No Label\tCtrl+Shift+.", count,count);
		menu_additem(mi.hSubMenu,IDM_EDIT_LABEL0+count,text);		/* add menu item */
	}

	mi.hSubMenu = CreatePopupMenu();
	SetMenuItemInfo(wmenu,IDM_ALIGNMENT,FALSE,&mi);
	mmi.dwMenuData = IDPM_ALIGNMENT;
	SetMenuInfo(mi.hSubMenu,&mmi);
	menu_additem(mi.hSubMenu,IDM_ALIGNMENT_NATURAL,"Natural");		/* add menu item */
	menu_additem(mi.hSubMenu,IDM_ALIGNMENT_LEFT,"Left");		/* add menu item */
	menu_additem(mi.hSubMenu,IDM_ALIGNMENT_RIGHT,"Right");		/* add menu item */

	clstruct.hWindowMenu = GetSubMenu(wmenu,6);
	clstruct.idFirstChild = CLIENT_ID;
//	g_hwclient = CreateWindow(TEXT("MDICLIENT"),NULL,WS_CHILD|WS_VISIBLE|WS_CLIPCHILDREN,0,0,0,0,hwnd,
//		NULL,g_hinst,(LPSTR)&clstruct);
//	SetWindowLongPtr(g_hwclient, GWL_EXSTYLE, (LONG_PTR)WS_EX_COMPOSITED);

	g_hwclient = CreateWindowEx(WS_EX_COMPOSITED, TEXT("MDICLIENT"), NULL, WS_CHILD|WS_VISIBLE|WS_CLIPCHILDREN,
		0, 0, 0, 0, hwnd, NULL, g_hinst, (LPSTR)&clstruct);
	(LONG_PTR)clientproc = SetWindowLongPtr(g_hwclient, GWLP_WNDPROC,(LONG_PTR)chook);		/* set subclass handler */
//	g_hwstatus = CreateStatusWindow(WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|SBARS_SIZEGRIP,TEXT(""),hwnd,IDC_STATUS);
//	SendMessage(g_hwstatus,SB_SETPARTS,STAT_NPARTS,(LPARAM)statwidths);
	g_hwstatus = CreateWindowEx(
		0,                       // no extended styles
		STATUSCLASSNAME,         // name of status bar class
		(PCTSTR)NULL,           // no text when first created
		SBARS_SIZEGRIP |         // includes a sizing grip
		WS_CHILD | WS_VISIBLE,   // creates a visible child window
		0, 0, 0, 0,              // ignores size and position
		hwnd,					// handle to parent window
		(HMENU)IDC_STATUS,       // child window identifier
		g_hinst,                   // handle to application instance
		NULL);                   // no window creation data
	configurestatusbar(g_hwstatus, STAT_NPARTS, statwidths);
	SendMessage(g_hwstatus, WM_SETFONT, (WPARAM)CreateFontIndirect(&getconfigdpi(hwnd)->lfStatusFont), MAKELPARAM(FALSE, 0));
#if 0
	g_hwtoolbar = CreateToolbarEx(hwnd,TBSTYLE_TOOLTIPS|TBSTYLE_ALTDRAG|
		WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|CCS_TOP|CCS_ADJUSTABLE|TBSTYLE_EX_DRAWDDARROWS,
		IDC_TOOLBAR,SB_TOTAL,HINST_COMMCTRL,IDB_STD_SMALL_COLOR,tbb,TBBSIZE,0,0,0,0,sizeof(TBBUTTON));
	tbvb.hInst = g_hinst;
	tbvb.nID = IDR_VIEWTOOLS;
	bindex = SendMessage(g_hwtoolbar,TB_ADDBITMAP,VB_TOTAL,(LPARAM)&tbvb);
	for (count = TBBSIZE; count < TBTOTSIZE; count++)	{
		if (tbb[count].idCommand)
			tbb[count].iBitmap = bindex++;
	}
	SendMessage(g_hwtoolbar,TB_ADDBUTTONS,ADDSIZE,(LPARAM)&tbb[TBBSIZE]);
#endif
	g_hwtoolbar = createtoolbar(hwnd);
	if (!reg_getkeyvalue(K_TOOLBARS,F_TBREGISTERD,NULL,NULL))	/* if don't have a default setting */
		com_tbsaverestore(g_hwtoolbar,hwnd,TRUE, F_TBREGISTERD);	/* save it */
	g_hwfcb = CreateWindowEx(0,TEXT("COMBOBOX"),TEXT(""),
		WS_CHILD|WS_BORDER|WS_VISIBLE|WS_VSCROLL|CBS_HASSTRINGS|CBS_DROPDOWNLIST,
		3,2,140,280,		/* xpos,ypos, wid, height */
		g_hwtoolbar,
		(HMENU)IDM_COMBO_FONT,
		g_hinst,
		NULL);
#if 0
	SetParent(g_hwfcb,g_hwtoolbar);		/* set toobar as parent */
#endif
	layoutForDpi(g_hwfcb, 3, 2, 140, 280);
	SendMessage(g_hwfcb,WM_SETFONT,(WPARAM)mfont,MAKELPARAM(FALSE,0));
	type_findfonts(hwnd);	/* finds fonts info */
	for (count = 0; count < t_fontlist.fcount; count++)
		ComboBox_AddString(g_hwfcb,t_fontlist.lf[count].elfLogFont.lfFaceName);
	g_hwscb = CreateWindowEx(0,TEXT("COMBOBOX"),TEXT(""),
		WS_CHILD|WS_BORDER|WS_VISIBLE|CBS_HASSTRINGS|CBS_DROPDOWN,
		147,2,50,250,	
		g_hwtoolbar,
		(HMENU)IDM_COMBO_SIZE,
		g_hinst,
		NULL);
	layoutForDpi(g_hwscb, 147, 2, 50, 250);
	SendMessage(g_hwscb, WM_SETFONT, (WPARAM)mfont, MAKELPARAM(FALSE, 0));
	type_setfontsizecombo(g_hwscb,g_prefs.privpars.size);
	g_hwscbed = GetWindow(g_hwscb,GW_CHILD);	/* get edit window of combo box, for TT */
	m_swproc = (WNDPROC)SetWindowLongPtr(g_hwscbed,GWLP_WNDPROC,(LONG_PTR)sizecb);	/* subclass the drop list */
	hwtt = (HWND)SendMessage(g_hwtoolbar,TB_GETTOOLTIPS,0,0);	/* now add tool tips for combo boxes */
	memset(&ti,0,sizeof(ti));
	ti.cbSize = sizeof(TOOLINFO);
	ti.uFlags = TTF_IDISHWND|TTF_CENTERTIP|TTF_SUBCLASS;
	ti.hwnd = g_hwtoolbar;
	for (count = 0; count < TOOLCONTROLS; count++)	{
		(HWND)ti.uId = *ttcontrols[count].hwnd;
		ti.lpszText = ttcontrols[count].text;
		SendMessage(hwtt,TTM_ADDTOOL,0,(LPARAM)&ti);	/* now add tool tips for combo boxes */
	}
	com_tbsaverestore(g_hwtoolbar,NULL,FALSE, F_TBREGISTERC);	// recover current

#if _DEBUG
	{
		RECT trect;
		TEXTPARS tpars;
		HWND tw;

		SetRect(&trect,0,0,600,600);
		memset(&tpars,0,sizeof(tpars));
		tpars.hwp = &g_logw;
//		tpars.style = ES_READONLY;
		tw = CreateWindowEx(WS_EX_MDICHILD|WS_EX_TOPMOST|WS_EX_TOOLWINDOW,g_textclass,TEXT("Log"),WS_CLIPCHILDREN|WS_CLIPSIBLINGS,
			trect.left,trect.top,trect.right,trect.bottom,g_hwclient,NULL,g_hinst,&tpars);
		if (tw)	{
			SendMessage(EX(tw,hwed),EM_SETRECT,0,(LPARAM)&EX(tw,prect));	/* set formatting rectangle (need this here) */
			ShowWindow(tw,SW_SHOWNORMAL);
		}
	}
#endif
	return(TRUE);
}
/************************************************************************/
static void dofactivate(HWND hwnd, UINT state, HWND hwndActDeact, BOOL fMinimized)	/* activates/deactivates */

{
#if 0
	HWND cwind;

	if (state != WA_INACTIVE && (cwind = getactivemdi(TRUE)))		/* if have active MDI window */
		SendMessage(cwind,WMM_UPDATETOOLBARS, 0,0);		/* do its toolbar fn to get buttons, etc */
	else
		com_setdefaultbuttons(FALSE);
#else
	if (state == WA_INACTIVE)	{	// if we're inactive
		com_setdefaultbuttons(FALSE);
//		SendMessage(g_hwtoolbar,TB_ENABLEBUTTON,IDM_FILE_NEW,FALSE);	// enable new, open
//		SendMessage(g_hwtoolbar,TB_ENABLEBUTTON,IDM_FILE_OPEN,FALSE);
	}
#endif
	DefFrameProc(hwnd,g_hwclient,WM_ACTIVATE,MAKEWPARAM((state), (fMinimized)),(LPARAM)(HWND)(hwndActDeact));
}
/************************************************************************/
static LRESULT CALLBACK sizecb(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)	/* callback for font size list */

{
	HWND mwind;

    switch (msg) { 
        case WM_KEYDOWN: 
  			mwind = getactivemdi(TRUE);
          switch (wParam) { 
                case VK_RETURN: 
					SendMessage(g_hwscb,CB_SHOWDROPDOWN,FALSE,0);
					SendMessage(mwind,WM_COMMAND, MAKELONG(IDM_COMBO_SIZE,CBN_CLOSEUP),(LPARAM)g_hwscb);
                    return 0; 
                case VK_ESCAPE:
					SendMessage(g_hwscb,CB_SETCURSEL,-1,0);	/* clear selection */
 					SendMessage(mwind,WM_COMMAND,MAKELONG(IDM_COMBO_SIZE,CBN_SELENDCANCEL),(LPARAM)g_hwscb);
                  return 0; 
            } 
            break; 
 
        case WM_KEYUP: 
        case WM_CHAR: 
            switch (wParam) { 
                case VK_ESCAPE: 
                case VK_RETURN: 
                    return 0; 
            } 
    } 
    return CallWindowProc(m_swproc, hwnd, msg, wParam, lParam);
}
/************************************************************************/
static void dofmenu(HWND hwnd, HMENU hMenu, UINT item, BOOL fSystemMenu)	/* sets menus */

{
	HWND cwind;
	
	if (hMenu == menu_gethandle(IDPM_FILE))	/* file menu */
		com_setrecentfiles(hMenu);
	if (hMenu == menu_gethandle(IDPM_TOOLS))	/* tools menu */
		mcr_setmenu(TRUE);

	if (cwind = getactivemdi(TRUE))	{
		FORWARD_WM_INITMENUPOPUP(cwind,hMenu,item,fSystemMenu,SendMessage);
		return;
	}
	com_setenable(EIDLE, XONLY, ON);	/* set menus for idling */
	if (hMenu == menu_gethandle(IDPM_ABBREVIATIONS))	/* abbrevs submenu menu */
		com_set(IDM_TOOLS_ABBREV_DETACH,abbrev_hasactiveset());
}
/************************************************************************/
static void dofcontmenu(HWND hwnd, HWND hwndContext, UINT xpos, UINT ypos)	/* does context menus */

{
	HWND cwind;
	POINT lp;
	RECT wrect;

	if (cwind = getactivemdi(TRUE))	{	/* get active index window */
		lp.x = xpos;
		lp.y = ypos;
		ScreenToClient(cwind,&lp);
		GetClientRect(cwind,&wrect);
		if (PtInRect(&wrect,lp))
			FORWARD_WM_CONTEXTMENU(cwind, hwndContext, xpos, ypos, SendMessage);
	}
}
/************************************************************************/
static void dohotkey(WPARAM wParam, LPARAM lParam)	/* filters hot keys */

{
	HWND twind = GetForegroundWindow();
	HWND cwind;
	
	if (twind == g_findw || twind == g_repw)
		SendMessage(twind,WM_HOTKEY,wParam,lParam);	/* pass it on */
	else if (cwind = getactivemdi(TRUE))
		SendMessage(cwind,WM_HOTKEY,wParam,lParam);	/* pass it on */
}
/************************************************************************/
static void dofclose(HWND hwnd)

{
	g_shutdown = TRUE;		/* use to flag special action (e.g., don't reorder recent list on activations */
	if (g_findw)					/* if a find window open */
		SendMessage(g_findw,WM_COMMAND,IDCANCEL,0);	/* dispose of it */
	if (g_repw)					/* if a replace window open */
		SendMessage(g_repw,WM_COMMAND,IDCANCEL,0);	/* dispose of it */
	if (g_spellw)					/* if a spell window open */
		SendMessage(g_spellw,WM_COMMAND,IDCANCEL,0);	/* dispose of it */
	if (mc_closeall(0,0,0,0))	{	/* if all indexes ok */
		if (abbrev_checkok())		/* if abbrevs ok */
			PostQuitMessage(0);
	}
	g_shutdown = FALSE;
}
/************************************************************************/
static void dofsize(HWND hwnd, UINT state, int cx, int cy)

{
	RECT crect, srect, trect;

	GetClientRect(g_hwframe,&crect);
	GetWindowRect(g_hwstatus,&srect);		/* get status window rect */
	GetWindowRect(g_hwtoolbar,&trect);		/* get toolbar window rect */
	crect.bottom -= srect.bottom-srect.top;	/* reduce client by height of status window */
	crect.top += trect.bottom-trect.top;		/* and by toobar */
	MoveWindow(g_hwclient,crect.left,crect.top,crect.right-crect.left,crect.bottom-crect.top,TRUE);	/* size client */
	SendMessage(g_hwtoolbar,TB_AUTOSIZE,0,0);	/* size toolbar */
	SendMessage(g_hwstatus,WM_SIZE,0,0);	/* and status bar */
}
/************************************************************************/
static void dofsyscolor(void)

{
	COLORREF cr = GetSysColor(COLOR_HIGHLIGHT);
	double norm = 0;
	BYTE rv,gv,bv;

	rv = GetRValue(cr);
	if (rv > norm)
		norm = rv;
	gv = GetGValue(cr);
	if (gv > norm)
		norm = gv;
	bv = GetBValue(cr);
	if (bv > norm)
		norm = bv;
#if 0
	if (norm)	// if not 0 (black)
		norm = .5*255/norm;
	rv = rv * norm + 128;
	gv = gv * norm + 128;
	bv = bv * norm + 128;
#else
	if (norm)	// if not 0 (black)
		norm = 0.3*255/norm;
	rv = (BYTE)(rv * norm + 0.55*255);
	gv = (BYTE)(gv * norm + 0.55*255);
	bv = (BYTE)(bv * norm + 0.55*255);
#endif
	cr = RGB(rv,gv,bv);
	if (g_highlightbrush)
		DeleteObject(g_highlightbrush);
	g_highlightbrush = CreateSolidBrush(cr);
}
/************************************************************************/
static void domenuselect(WPARAM wParam, LPARAM lParam)	/* shows menu help text in status bar */

{
	UINT string[4];

	memset(string,0, sizeof(string));
	MenuHelp(WM_MENUSELECT, wParam,lParam,NULL,g_hinst,g_hwstatus,string);
}
/************************************************************************/
static BOOL initcindex (void)	/* initializes, registers classes, etc */

{
	TCHAR namestring[STSTRING], execpath[STSTRING];
	DWORD vsize, vzero;
	void * vdata;
	LPVOID valptr;
	UINT vlen;

	WNDCLASSEX fclass;
	HWND tempf, temppop;
	char tbuff[10];
	HKEY hk;
	DWORD type, size;
	RECT wpos = {CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT};

	tempf = FindWindow(szframename,NULL);	/* find if window of frame class exists */
#ifdef PUBLISH
	if (!g_admin.allowmultiple && IsWindow(tempf))	{	/* if not allowing multiple & a copy is running */
#else
	if (IsWindow(tempf))	{
#endif //PUBLISH
		temppop = GetLastActivePopup(tempf);	/* find front window if any */
		if (IsWindow(temppop))
			tempf = temppop;
		SetForegroundWindow(tempf);
		if (IsIconic(tempf))
			ShowWindow(tempf, SW_RESTORE);
		return FALSE;
	}

//	GlobalMemoryStatus(&g_mstat);
//	GetLocaleInfo(LOCALE_USER_DEFAULT,LOCALE_IMEASURE, tbuff,10);
//	g_locale = (short)natoi(tbuff);
	col_findlocales();

	g_rightarrow = LoadCursor(g_hinst,MAKEINTRESOURCE(IDC_RIGHTPOINTER));
	g_downarrow = LoadCursor(g_hinst,MAKEINTRESOURCE(IDC_DOWNARROW));
	g_waitcurs = LoadCursor(NULL,IDC_WAIT);
	g_arrow = LoadCursor(NULL,IDC_ARROW);

	fclass.cbSize = sizeof(fclass);			/* frame window class */
	fclass.style = 0;
	fclass.lpfnWndProc = frameproc;
	fclass.cbClsExtra = 0;
	fclass.cbWndExtra = 0;
	fclass.hInstance = g_hinst;
	fclass.hIcon = LoadImage(g_hinst,MAKEINTRESOURCE(IDI_CINDEX),IMAGE_ICON,GetSystemMetrics(SM_CXICON),GetSystemMetrics(SM_CYICON),LR_SHARED);
	fclass.hCursor = g_arrow;
	fclass.hbrBackground = (HBRUSH)(COLOR_APPWORKSPACE+1);
	fclass.lpszMenuName = szframename;
	fclass.lpszClassName = szframename;	/* class name */
	fclass.hIconSm = LoadImage(g_hinst,MAKEINTRESOURCE(IDI_CINDEX),IMAGE_ICON,GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),LR_SHARED);
	if (!RegisterClassEx(&fclass))
		return FALSE;

	fclass.cbSize = sizeof(fclass);		/* container window class */
	fclass.style = CS_DBLCLKS|CS_OWNDC;
	fclass.lpfnWndProc = container_proc;
	fclass.cbClsExtra = 0;
	fclass.cbWndExtra = sizeof(int);	/* first long is mdi activate flag */
	fclass.hInstance = g_hinst;
	fclass.hIcon = LoadImage(g_hinst,MAKEINTRESOURCE(IDI_INDEX),IMAGE_ICON,GetSystemMetrics(SM_CXICON),GetSystemMetrics(SM_CYICON),LR_SHARED);
	fclass.hCursor = g_arrow;
	fclass.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
	fclass.lpszMenuName = NULL;
	fclass.lpszClassName = g_containerclass;	/* class name */
	fclass.hIconSm = LoadImage(g_hinst,MAKEINTRESOURCE(IDI_INDEX),IMAGE_ICON,GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),LR_SHARED);
	if (!RegisterClassEx(&fclass))
		return FALSE;

	fclass.cbSize = sizeof(fclass);		/* container window class */
	fclass.style = CS_DBLCLKS|CS_OWNDC;
	fclass.lpfnWndProc = rcontainer_proc;
	fclass.cbClsExtra = 0;
	fclass.cbWndExtra = sizeof(int);	/* first long is mdi activate flag */
	fclass.hInstance = g_hinst;
	fclass.hIcon = LoadImage(g_hinst,MAKEINTRESOURCE(IDI_INDEX),IMAGE_ICON,GetSystemMetrics(SM_CXICON),GetSystemMetrics(SM_CYICON),LR_SHARED);
	fclass.hCursor = g_arrow;
	fclass.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
	fclass.lpszMenuName = NULL;
	fclass.lpszClassName = g_rcontainerclass;	/* class name */
	fclass.hIconSm = LoadImage(g_hinst,MAKEINTRESOURCE(IDI_INDEX),IMAGE_ICON,GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),LR_SHARED);
	if (!RegisterClassEx(&fclass))
		return FALSE;

	fclass.cbSize = sizeof(fclass);		/* index window class */
	fclass.style = CS_DBLCLKS|CS_OWNDC;
	fclass.lpfnWndProc = view_proc;
	fclass.cbClsExtra = 0;
	fclass.cbWndExtra = sizeof(int);	/* first long is mdi activate flag */
	fclass.hInstance = g_hinst;
	fclass.hIcon = LoadImage(g_hinst,MAKEINTRESOURCE(IDI_INDEX),IMAGE_ICON,GetSystemMetrics(SM_CXICON),GetSystemMetrics(SM_CYICON),LR_SHARED);
	fclass.hCursor = g_arrow;
	fclass.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
	fclass.lpszMenuName = NULL;
	fclass.lpszClassName = g_indexclass;	/* class name */
	fclass.hIconSm = LoadImage(g_hinst,MAKEINTRESOURCE(IDI_INDEX),IMAGE_ICON,GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),LR_SHARED);
	if (!RegisterClassEx(&fclass))
		return FALSE;

	fclass.cbSize = sizeof(fclass);		/* text window class */
	fclass.style = CS_DBLCLKS|CS_OWNDC;
	fclass.lpfnWndProc = txt_proc;
	fclass.cbClsExtra = 0;
	fclass.cbWndExtra = sizeof(int);	/* first long is mdi activate flag */
	fclass.hInstance = g_hinst;
	fclass.hIcon = LoadIcon(NULL,IDI_APPLICATION);
	fclass.hCursor = g_arrow;
	fclass.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
	fclass.lpszMenuName = NULL;
	fclass.lpszClassName = g_textclass;	/* class name */
	fclass.hIconSm = LoadImage(g_hinst,MAKEINTRESOURCE(IDI_CINDEX),IMAGE_ICON,GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),LR_SHARED);
	if (!RegisterClassEx(&fclass))
		return FALSE;

	fclass.cbSize = sizeof(fclass);		/* record window class */
	fclass.style = CS_DBLCLKS|CS_PARENTDC;
	fclass.lpfnWndProc = mod_proc;
	fclass.cbClsExtra = 0;
	fclass.cbWndExtra = sizeof(int);	/* first long is mdi activate flag */
	fclass.hInstance = g_hinst;
	fclass.hIcon = LoadIcon(NULL,IDI_APPLICATION);
	fclass.hCursor = LoadCursor(NULL,IDC_ARROW);
	fclass.hbrBackground = (HBRUSH)(COLOR_BTNFACE+1);
	fclass.lpszMenuName = NULL;
	fclass.lpszClassName = g_modclass;	/* class name */
	fclass.hIconSm = LoadImage(g_hinst,MAKEINTRESOURCE(IDI_CINDEX),IMAGE_ICON,GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),LR_SHARED);
#if 0
	fclass.hIconSm = LoadIcon(NULL,IDI_APPLICATION);
#endif
	if (!(g_modclassatom = RegisterClassEx(&fclass)))
		return FALSE;

	InitCommonControls();
	LoadLibrary(TEXT("Riched20.dll"));
	g_recref = RegisterClipboardFormat(CF_CINREC);
	g_rtfref = RegisterClipboardFormat(CF_RTF);
//	g_htmlref = RegisterClipboardFormat(CF_HTML);

	GetModuleFileName(NULL,execpath,STSTRING);
	if (vsize = GetFileVersionInfoSize(execpath,&vzero))	{
		if (vdata = getmem(vsize))	{
			if (GetFileVersionInfo(execpath,0,vsize,vdata))	{
				VerQueryValue(vdata, TEXT("\\StringFileInfo\\040904E4\\InternalName"),&valptr, &vlen);
				nstrcpy(namestring, valptr);
				VerQueryValue(vdata, TEXT("\\StringFileInfo\\040904E4\\FileVersion"),&valptr, &vlen);
				nstrcpy(g_cinversion,valptr);	// save version info
			}
			freemem(vdata);
		}
	}
//	g_hwframe = CreateWindow(szframename,namestring,WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN,
//		CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,NULL,NULL,g_hinst,NULL);
	g_hwframe = CreateWindowEx(0,szframename, namestring, WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, g_hinst, NULL);
	if (RegOpenKeyEx(HKEY_CURRENT_USER,TEXT("Control Panel\\Keyboard"),0,KEY_ALL_ACCESS,&hk) == ERROR_SUCCESS)	{
		size = sizeof(tbuff);
		if (RegQueryValueEx(hk,TEXT("KeyboardDelay"),NULL,&type,tbuff,&size) == ERROR_SUCCESS)
			g_keydelay = (int)(*tbuff-'0'+1)*250;		/* now have basic key delay in msec (0 = 250 msec; each step is 250) */
		RegCloseKey(hk);
	}
	SendMessage(g_hwframe,WM_SIZE,0,0);	/* force sizing of client so dde open can work if checking userID */
	return(TRUE);
}
/*******************************************************************************/
static BOOL getuserid(void)	/* gets user id */

{
	return DialogBox(g_hinst,MAKEINTRESOURCE(IDD_USERID),g_hwframe,userproc) > 0;
}
/******************************************************************************/
static INT_PTR CALLBACK userproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)

{
	HWND eh;

	switch (msg)	{

		case WM_INITDIALOG:
			eh = GetDlgItem(hwnd,IDC_USERID);
			SendMessage(eh,EM_SETLIMITTEXT,4,0);	/* limits text */
#ifdef PUBLISH
			if (!g_admin.requireid)
#endif
				setDItemText(hwnd,IDC_USERID,g_prefs.hidden.user);
			SendMessage(eh,EM_SETSEL,0,-1);	/* limits text */
			centerwindow(hwnd,0);
			return FALSE;
		case WM_COMMAND:
			switch (LOWORD(wParam))	{
				case IDOK:
					getDItemText(hwnd,IDC_USERID,g_prefs.hidden.user,5);
					if (!*g_prefs.hidden.user)	{	/* if no id */
						SetFocus(GetDlgItem(hwnd,IDC_USERID));
						return (TRUE);
					}
#ifdef PUBLISH
					GetDlgItemText(hwnd,IDC_PASSWORD,g_password,FSSTRING);
					if (g_admin.validate)	{	/* if want validated */
						int hit = FALSE;
						FILE * fptr;
						char * txptr, userline[STSTRING];

						if (fptr = nfopen(g_admin.userpath,TEXT("r")))	{
							while (fgets(userline,STSTRING,fptr))	{
								if ((txptr = strchr(userline,';')) || (txptr = strchr(userline,'\n')))	{
									*txptr = '\0';					
									if (*g_prefs.hidden.user && !strnicmp(userline,g_prefs.hidden.user,4))
										hit = TRUE;
								}
							}
							fclose(fptr);
						}
						else
							hit = TRUE;
						if (!hit || *g_password && nstrcmp(g_password,g_admin.psswd))	{ /* if bad id or bad password */
							SetFocus(GetDlgItem(hwnd,IDC_USERID));
							return (TRUE);
						}
					}
#endif //PUBLISH
				case IDCANCEL:
					EndDialog(hwnd,LOWORD(wParam) == IDOK ? TRUE : FALSE);
					return TRUE;
			}
			break;
		case WM_HELP:
			return (dodialoghelp(hwnd,TEXT("_Starting An Index\\_Identifyinguser.htm"),(HELPINFO *)lParam,wh_userid));
	}
	return FALSE;
}

/******************************************************************************/
static INT_PTR CALLBACK splashproc(HWND hwnd, UINT msg, WPARAM wParam,LPARAM lParam)

{
	RECT wrect, srect;
	int splashW, splashH;
#ifdef PUBLISH
	int iid = SPLASH_PUB;
#else
	int iid = SPLASH;
#endif
	switch (msg)	{
		case WM_INITDIALOG:
			splashW = 430 * 1.25;
			splashH = 215 * 1.25;
			SystemParametersInfo(SPI_GETWORKAREA,sizeof(RECT),&srect,0);
			GetWindowRect(hwnd,&wrect);
			SetWindowPos(hwnd,NULL,(srect.right-splashW)/2,(srect.bottom-splashH)/2, splashW, splashH, SWP_NOZORDER | SWP_FRAMECHANGED);
			HBITMAP bm = LoadImage(g_hinst, MAKEINTRESOURCE(iid), IMAGE_BITMAP, splashW, splashH, 0);
			SendDlgItemMessage(hwnd, IDC_SPLASH_IMAGE, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)bm);			
			m_timerid = SetTimer(NULL,0,10000,timeproc);
			return (FALSE);
	}
	return FALSE;
}
/******************************************************************************/
static void CALLBACK timeproc(HWND hwnd, UINT msg, UINT_PTR timer, DWORD time)	/* destroys splash */

{
	KillTimer(NULL,m_timerid);
	DestroyWindow(hwsplash);
	displayframe();

	hwsplash = NULL;
}
/******************************************************************************/
static void displayframe(void)	// shows frame window

{
	TCHAR path[MAX_PATH], *eptr;
	WINDOWPLACEMENT wp;

	wp.length = sizeof(wp);
	if (reg_getkeyvalue(K_GENERAL,FRAME,&wp,&wp.length) && wp.length == sizeof(WINDOWPLACEMENT))	{	// if have size/pos from registry 
		if (g_prefs.gen.maxcin)
			 wp.showCmd = SW_SHOWMAXIMIZED;
		SetWindowPlacement(g_hwframe,&wp);
	}
	else
		ShowWindow(g_hwframe,g_prefs.gen.maxcin ? SW_MAXIMIZE: m_cmdshow);
	if (!d_ddeopen)	{	// if didn't get opened by double-click
		TCHAR cline[MAX_PATH];
		TCHAR * sptr = cline;

		nstrcpy(cline,toNative(m_cmdline));
		if (*sptr == '\"')	/* clean it up (strip any protective quotes) */
			sptr++;
		nstrcpy(path,sptr);
		if (*path)	{	// if want to open something
			if (eptr = nstrrchr(path,'\"'))
				*eptr = '\0';
			if (!(eptr = file_getextn(path)))	/* doesn't have an extension */
				nstrcat(path,file_extensionfortype(FTYPE_INDEX));		/* add it */
			if (file_open(path,OP_VISIBLE))	// if have document to open
				return;							/* don't look for last index */
		}
		if (g_prefs.gen.openflag == 1)		{/* if want to open last index */
			if (file_getuserpath(path,ALIS_RECENT[0]))
				file_openindex(path,OP_VISIBLE);
		}
		else if (g_prefs.gen.openflag == 2)
			mc_open(NULL,0,NULL,0);
	}
}
#if 0
/*****************************************************************************/
static void showcharactermap(void)		// displays character map

{
	struct cmstruct ms;

	ms.pip = &m_charmap;
	ms.active = FALSE;

	EnumWindows(xfind,(LPARAM)&ms);
	if (!ms.active)	{	// if don't have active map
		TCHAR name[MAX_PATH] = TEXT("charmap.exe");
		STARTUPINFO si;

		CloseHandle(m_charmap.hProcess);	// in case prev window closed by user
		CloseHandle(m_charmap.hThread);
		ZeroMemory(&si, sizeof(si));
		si.cb = sizeof(si);
		ZeroMemory(&m_charmap, sizeof(PROCESS_INFORMATION));
		if (!CreateProcess(NULL,name,NULL,NULL,FALSE,0,NULL,NULL,&si,&m_charmap))	{
			DWORD err = GetLastError();
			TCHAR tbuff[4096];
			FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,NULL,err,0,tbuff,4096,NULL);
			senderr(ERR_NOCHARMAP,WARN,tbuff);
		}
	}
}
#else
/*****************************************************************************/
static void showcharactermap(void)		// displays character map

{
	struct cmstruct ms;

	ms.pip = &m_charmap;
	ms.active = FALSE;

	EnumWindows(xfind,(LPARAM)&ms);
	if (!ms.active)	{	// if don't have active map
		TCHAR name[MAX_PATH];
		STARTUPINFO si;

		CloseHandle(m_charmap.hProcess);	// in case prev window closed by user
		CloseHandle(m_charmap.hThread);
		ZeroMemory(&si, sizeof(si));
		si.cb = sizeof(si);
		ZeroMemory(&m_charmap, sizeof(PROCESS_INFORMATION));
		ZeroMemory(&name,MAX_PATH);
//		nstrcpy(name,TEXT("charmap.exe"));

		if (SHGetFolderPath(NULL,CSIDL_SYSTEM,NULL,SHGFP_TYPE_CURRENT,name) == S_OK)
			nstrcat(name,TEXT("\\charmap.exe"));

		if (!CreateProcess(name,NULL,NULL,NULL,FALSE,0,NULL,NULL,&si,&m_charmap))	{
			DWORD err = GetLastError();
			TCHAR tbuff[4096];
			FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,NULL,err,0,tbuff,4096,NULL);
			senderr(ERR_NOCHARMAP,WARN,tbuff);
		}
	}
}
#endif
/*****************************************************************************/
static BOOL CALLBACK xfind(HWND hwnd,LPARAM param)

{
	struct cmstruct *msp = (struct cmstruct *)param;
	DWORD pid = 0;

	GetWindowThreadProcessId(hwnd,&pid);
	if (pid && pid == msp->pip->dwProcessId)	{
		SetForegroundWindow(hwnd);
		msp->active = TRUE;	// show active
		return FALSE;		// stop enumeration
	}
	return TRUE;
}
/*****************************************************************************/
static BOOL testsernum(char * sernum)	/* tests serial number string */
{
	return 1;
}
/************************************************************************/
static LRESULT CALLBACK chook(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)

{
//  so that we can intercept messages to the MDI client
	LRESULT result;

	result = CallWindowProc((WNDPROC)clientproc,hwnd,msg,wParam,lParam);	/* pass to ordinary handler */
	return result;
}
/******************************************************************************/
static HWND createtoolbar(HWND hWndParent)

{
//	TBMETRICS tbm;
	const int ImageListID = 0;
	int imageSize = scaleForDpi(hWndParent, 16) > 16 ? 24 : 16;	// use 24 image lists if high res (needed for standard icons)
	const DWORD buttonStyles = BTNS_AUTOSIZE;

//	tbm.cbSize = sizeof(tbm);
//	tbm.dwMask = TBMF_PAD | TBMF_BUTTONSPACING;

	// Create the toolbar.
	HWND hWndToolbar = CreateWindowEx(0, TOOLBARCLASSNAME, NULL,
		WS_CHILD | TBSTYLE_TOOLTIPS | CCS_TOP  | CCS_ADJUSTABLE | TBSTYLE_ALTDRAG, 0, 0, 0, 0,
		hWndParent, NULL, g_hinst, NULL);

	if (hWndToolbar != NULL) {
		DWORD tbs = SendMessage(hWndToolbar, TB_GETBUTTONSIZE, 0, 0);
#if 0
		SendMessage(hWndToolbar, TB_GETMETRICS, 0, (LPARAM)&tbm);
		tbm.cxPad = scaleForDpi(hWndParent, tbm.cxPad);
		tbm.cyPad = scaleForDpi(hWndParent, tbm.cyPad);
		//	tbm.cxButtonSpacing = scaleForDpi(hWndParent, tbm.cxButtonSpacing);
		SendMessage(hWndToolbar, TB_SETMETRICS, 0, (LPARAM)&tbm);
#endif

//		int xx = getsysmetrics(hWndParent, SM_CYICON);
		SendMessage(hWndToolbar, CCM_SETVERSION, (WPARAM)5, 0);		// allow multiple image lists

		standardImagelist = ImageList_Create(imageSize, imageSize,   // Dimensions of individual bitmaps.
			ILC_COLOR16|ILC_MASK, 9, 0);	// 9 initial buttons
		cindexImageList = ImageList_Create(imageSize, imageSize,   // Dimensions of individual bitmaps.
			ILC_COLOR32, 11, 5);	// 11 initial buttons
		for (int hindex = IDB_NEW; hindex <= IDB_SORTNONE; hindex++) {	// add button images
			HBITMAP bm = LoadImage(g_hinst, MAKEINTRESOURCE(hindex), IMAGE_BITMAP, imageSize, imageSize, 0);
			ImageList_Add(cindexImageList, bm, NULL);
			DeleteObject(bm);
		}
		for (int bindex = 0; bindex < TBBSIZE; bindex++) {	// scale separators
			if (tbb[bindex].fsStyle == BTNS_SEP)	// if separator
				tbb[bindex].iBitmap = scaleForDpi(hWndParent, tbb[bindex].iBitmap);	// scale it
		}
		// Set the cindex image list.
		SendMessage(hWndToolbar, TB_SETIMAGELIST, (WPARAM)CINDEXIMAGELIST, (LPARAM)cindexImageList);

		// Set the standard image list.
		SendMessage(hWndToolbar, TB_SETIMAGELIST, (WPARAM)STANDARDIMAGELIST, (LPARAM)standardImagelist);

		SendMessage(hWndToolbar, TB_LOADIMAGES, imageSize > 16 ? (WPARAM)IDB_STD_LARGE_COLOR : (WPARAM)IDB_STD_SMALL_COLOR, (LPARAM)HINST_COMMCTRL);

		// Initialize button info.
		SendMessage(hWndToolbar, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0);
		SendMessage(hWndToolbar, TB_ADDBUTTONS, (WPARAM)TBBSIZE, (LPARAM)tbb);
		SendMessage(hWndToolbar, TB_SETBUTTONSIZE, 0, MAKELPARAM(scaleForDpi(hWndParent, LOWORD(tbs)), scaleForDpi(hWndParent, HIWORD(tbs))));
	}
	ShowWindow(hWndToolbar, TRUE);
	return hWndToolbar;
}
