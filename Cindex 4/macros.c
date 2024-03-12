#include "stdafx.h"
#include "commands.h"
#include "errors.h"
#include "macros.h"
#include "registry.h"
#include "util.h"

#define MMAXKEYS 600
#define MAXMACROS 10

typedef struct {
	MSG msg;
	BYTE kstate[256];
} EVMSG;

struct mstruct {	
	EVMSG evt[MMAXKEYS];
	int addindex;
	int keyindex;
	TCHAR name[FTSTRING];
};

static const DWORD wh_mnid[] = {
	IDC_MACRO_SETNAME, HIDC_MACRO_SETNAME
};

static TCHAR j_message[20];
static UINT_PTR j_timerid;		/* blink timer id */

static HHOOK jhook;
static BOOL recording, playing;
static int curindex;

static struct mstruct m_set[MAXMACROS];
static TCHAR *m_accel[] = {TEXT(" &1"),TEXT(" &2"),TEXT(" &3"),TEXT(" &4"),TEXT(" &5"),TEXT(" &6"),TEXT(" &7"),TEXT(" &8"),TEXT(" &9"),TEXT("1&0")};

static LRESULT CALLBACK jrecord(int code, WPARAM wParam, LPARAM lParam);	/* records jouranl */
LRESULT CALLBACK jplay(int code, WPARAM wParam, LPARAM lParam);		/* plays journal */
static void clearkeyboard(void);		// clears keyboard state
static void CALLBACK blink(HWND hwnd, UINT msg, UINT_PTR timer, DWORD time);	/* blinks flag in status window */
static BOOL mload(struct mstruct *, int index);		/* loads macro */
static BOOL msave(struct mstruct *, int index);		/* saves macro */

static BOOL getmacroname(HWND hwnd, char * name);	/* gets name for macro */
static INT_PTR CALLBACK macproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static void setmenuname(struct mstruct *macro, int index);		/* sets macro name in menus */

/************************************************************************/
void mcr_load(void)		/* loads macros */

{
	int index;

	for (index = 0; index < MAXMACROS; index++)
		mload(&m_set[index],index);
}
/************************************************************************/
void mcr_save(void)		/* saves macros */

{
	int index;

	for (index = 0; index < MAXMACROS; index++)
		msave(&m_set[index],index);
}
/************************************************************************/
void mcr_record(int index)		/* starts/stops recording */

{
	if (jhook = SetWindowsHookEx(WH_GETMESSAGE,jrecord,NULL,GetCurrentThreadId()))	{
		recording = TRUE;	/* toggle */
		m_set[index].addindex = 0;
		curindex = index;
		u_sprintf(j_message,"Recording %d", index+1);
		com_check(IDM_TOOLS_RECORDKEY, ON);
		j_timerid = SetTimer(NULL,0,750,blink);
	}
	else
		NSLog(NULL);	// show system error
}
/************************************************************************/
BOOL mcr_isplaying(void)	/* returns TRUE if playing */

{
	return (playing);
}
/************************************************************************/
void mcr_play(int index)			/*  runs macro queue */

{
	playing = TRUE;
	m_set[index].keyindex = 0;
	curindex = index;
	BlockInput(TRUE);
	clearkeyboard();
	if (jhook = SetWindowsHookEx(WH_FOREGROUNDIDLE,jplay,g_hinst,GetCurrentThreadId()))	{
		u_sprintf(j_message,"Playing %d", index+1);
		j_timerid = SetTimer(NULL,0,750,blink);
	}
	BlockInput(FALSE);
}
/************************************************************************/
BOOL mcr_cancel(void)			/* cancels journaling */

{
	BOOL wasrecording = recording;

	UnhookWindowsHookEx(jhook);
	playing = recording = FALSE;
	com_check(IDM_TOOLS_RECORDKEY, OFF);
	KillTimer(NULL,j_timerid);
	*j_message = '\0';
	SendMessage(g_hwstatus,SB_SETTEXT,STATSEG_MACRO|0,(LPARAM)TEXT(""));
	return wasrecording;
}
/************************************************************************/
void mcr_setmenu(int disable)			/* sets macro menu items */

{
	int index;

	disable |= recording || playing;
	for (index = 0; index < MAXMACROS; index++)	{
		com_set(IDM_PMACRO_1+index,disable || !m_set[index].addindex ? OFF : ON);		/* disable playback */
		com_set(IDM_MACRO_1+index,disable ? OFF : ON);			/* disable recording */	
	}
}
/************************************************************************/
static LRESULT CALLBACK jrecord(int code, WPARAM wParam, LPARAM lParam)		/* records journal */

{
	switch (code)	{
		case HC_ACTION:
			if (m_set[curindex].addindex < MMAXKEYS)	{
				if (wParam == PM_REMOVE)		{	// if message to be removed
					MSG * msg = (MSG *)lParam;
					if (msg->message == WM_KEYDOWN || msg->message == WM_KEYUP
//						|| msg->message == WM_COMMAND
						|| msg->message == WM_SYSKEYDOWN || msg->message == WM_SYSKEYUP
//						|| msg->message >= WM_LBUTTONDOWN && msg->message <= WM_MOUSELAST
						)	{
						if (msg->message != WM_KEYDOWN || msg->wParam != VK_CANCEL)  {	// if not cancelling
							GetKeyboardState(m_set[curindex].evt[m_set[curindex].addindex].kstate);
//							ClientToScreen(msg->hwnd,&msg->pt);
							m_set[curindex].evt[m_set[curindex].addindex].msg = *msg;
							m_set[curindex].addindex++;
//							NSLog("Adding %d",m_set[curindex].addindex);
						}
					}
				}
			}
			else
				mcr_cancel();
			return 0;
		default:
			return CallNextHookEx(jhook,code,wParam,lParam);
	}
	return (0);		/* apparently ignored */
}
/************************************************************************/
LRESULT CALLBACK jplay(int code, WPARAM wParam, LPARAM lParam)		/* plays journal */

{

	switch (code)	{
		case HC_ACTION:
			if (m_set[curindex].keyindex < m_set[curindex].addindex)	{	/* if events in queue */
				MSG *msg = &m_set[curindex].evt[m_set[curindex].keyindex].msg;
				HWND window;

				SetKeyboardState(m_set[curindex].evt[m_set[curindex].keyindex].kstate);
				if (msg->message >= WM_KEYFIRST && msg->message <= WM_KEYLAST)	// if keyboard
					window = GetFocus();
				else
					window = WindowFromPoint(msg->pt);
//					window = msg->hwnd;
				PostMessage(window, msg->message, msg->wParam,msg->lParam);
//				NSLog("Removing %d",m_set[curindex].keyindex);
				m_set[curindex].keyindex++;
			}
			else	{
				clearkeyboard();
				mcr_cancel();
			}
			return (0);
		default:
			if (code < 0)	/* pass to next */
				return (CallNextHookEx(jhook,code,wParam,lParam));
	}
	return (0);		/* apparently ignored */
}
/************************************************************************/
static void clearkeyboard(void)		// clears keyboard state

{
	unsigned char karray[256];

	memset(karray,0,sizeof(karray));
	SetKeyboardState(karray);		/* clear virtual key codes */
}
/************************************************************************/
static void CALLBACK blink(HWND hwnd, UINT msg, UINT_PTR timer, DWORD time)	/* blinks flag in status window */

{
	static BOOL on;

	on ^= 1;
	SendMessage(g_hwstatus,SB_SETTEXT,STATSEG_MACRO|0,on ? (LPARAM)j_message : (LPARAM)TEXT(""));
}
/*******************************************************************************/
static BOOL mload(struct mstruct *macro, int index)		/* loads macro */

{
	DWORD size;

	size = MMAXKEYS*sizeof(EVMSG);	// set max possible size
	if (reg_getkeyvalue(K_MACROS,MACROS[index],macro->evt,&size) && size)	{	/* if got value from registry */
		macro->addindex = size/sizeof(EVMSG);
		size = FTSTRING*sizeof(TCHAR);	// set max possible size
		if (reg_getkeyvalue(K_MACROS,MSTRINGS[index],macro->name,&size) && size)	{/* if got value from registry */
#if 0
			com_setname(IDM_PMACRO_1+index, macro->name);
			com_setname(IDM_MACRO_1+index, macro->name);
#else
			setmenuname(macro,index);
#endif
			return TRUE;
		}
	}
	return FALSE;
}
/*******************************************************************************/
static BOOL msave(struct mstruct *macro, int index)		/* saves macro */

{
	DWORD size;

	size = macro->addindex*sizeof(EVMSG);
	if (reg_setkeyvalue(K_MACROS,MACROS[index],REG_BINARY,macro->evt,size))	{	/* if can save value */
		if (reg_setkeyvalue(K_MACROS,MSTRINGS[index],REG_SZ,macro->name,nstrlen(macro->name)))	/* if can save value */
			return TRUE;
	}
	return FALSE;
}
/*******************************************************************************/
void mcr_setname(HWND hwnd)		/* assigns name to current macro */

{
	TCHAR name[FTSTRING] = TEXT("macro");

	if (DialogBoxParam(g_hinst,MAKEINTRESOURCE(IDD_MACRO_NAME),hwnd,macproc,(LPARAM)name))	{
		nstrcpy(m_set[curindex].name, name);
		setmenuname(&m_set[curindex], curindex);
	}
}
/******************************************************************************/
static INT_PTR CALLBACK macproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)

{
	TCHAR * nptr;
	HWND eh;

	nptr = getdata(hwnd);
	switch (msg)	{

		case WM_INITDIALOG:
			nptr = setdata(hwnd,(void *)lParam);/* set data */
			SetDlgItemText(hwnd,IDC_MACRO_SETNAME,nptr);
			selectitext(hwnd,IDC_MACRO_SETNAME);
			eh = GetDlgItem(hwnd,IDC_MACRO_SETNAME);
			SendMessage(eh,EM_SETLIMITTEXT,FTSTRING-16,0);	/* limits text */
			SetFocus(eh);	/* set focus to text */
			centerwindow(hwnd,0);
			return FALSE;
		case WM_COMMAND:
			switch (LOWORD(wParam))	{
				case IDOK:
					GetDlgItemText(hwnd,IDC_MACRO_SETNAME,nptr,FSSTRING-4);
				case IDCANCEL:
					EndDialog(hwnd,LOWORD(wParam) == IDOK ? TRUE : FALSE);
					return TRUE;
			}
			break;
		case WM_HELP:
			return (dodialoghelp(hwnd,TEXT("base\\Macros.htm"),(HELPINFO *)lParam,wh_mnid));
	}
	return FALSE;
}
/*******************************************************************************/
static void setmenuname(struct mstruct *macro, int index)		/* sets macro name in menus */

{
	TCHAR name[FTSTRING], *sptr;

	if (*macro->name)	{
		u_sprintf(name,"%S %S\tCtrl+Alt+%c",m_accel[index],macro->name,(index+1)%10+'0');
		com_setname(IDM_PMACRO_1+index,name);
		sptr = nstrchr(name,'\t');
		*sptr = '\0';
		com_setname(IDM_MACRO_1+index,name);
	}
}
