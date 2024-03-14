#include "stdafx.h"
#include <stdarg.h>
#include "errors.h"
#include "util.h"
#include "apiservice.h"

short err_eflag;	/* global error flag -- TRUE after any error */

struct elevel{
	char beep;
	char box;
};

static struct elevel exx[4][4] ={
	{{1,1},{1,1},{2,1},{3,1}},
	{{1,0},{1,1},{1,1},{1,1}},
	{{0,1},{1,1},{1,1},{1,1}},
	{{0,1},{0,1},{0,1},{0,1}}
};
static TCHAR cinname[] = TEXT("CINDEX");

__declspec( thread ) short xt_lasterr, xt_samecount;
__declspec( thread ) long xt_lasttime;
__declspec( thread ) TCHAR xt_laststring[256];

/*******************************************************************************/
short sendinfooption(TCHAR * title, const int warnno, ...)		/*  O.K. */

{
	int action;

	if (!api_active)	{
		TCHAR text[256], tbuff[256];
		va_list aptr;

		LoadString(g_hinst, warnno,text,255);
		va_start(aptr, warnno);		 /* initialize arg pointer */
		u_vsprintf_u(tbuff, text, aptr); /* get string */
		va_end(aptr);
		action = MessageBox(NULL,tbuff,title,MB_TASKMODAL|MB_ICONINFORMATION|MB_YESNO| MB_TOPMOST);	/* display alert */
	}
	return (action == IDYES ? TRUE : FALSE);
}
/*******************************************************************************/
void sendinfo(const int warnno, ...)		/*  O.K. */

{
	if (!api_active)	{
		TCHAR text[256], tbuff[256];
		va_list aptr;

		LoadString(g_hinst, warnno,text,255);
		va_start(aptr, warnno);		 /* initialize arg pointer */
		u_vsprintf_u(tbuff, text, aptr); /* get string */
		va_end(aptr);
		MessageBox(NULL,tbuff,cinname,MB_TASKMODAL|MB_ICONINFORMATION|MB_OK| MB_TOPMOST);	/* display alert */
	}
}
/*******************************************************************************/
short sendwarning(const int warnno, ...)		/* cancel, O.K. */

{
	TCHAR text[STSTRING], tbuff[STSTRING];
	va_list aptr;
	int action;

	if (api_active)
		return FALSE;
	LoadString(g_hinst, warnno,text,STSTRING);
	va_start(aptr, warnno);		 /* initialize arg pointer */
	u_vsprintf_u(tbuff, text, aptr); /* get string */
	va_end(aptr);
	action = MessageBox(NULL,tbuff,cinname,MB_TASKMODAL|MB_ICONWARNING|MB_YESNO| MB_TOPMOST);	/* display alert */
	return (action == IDYES ? TRUE : FALSE);
}
/*******************************************************************************/
short savewarning(const int warnno, ...)		/* discard, cancel, o.k. */

{
	TCHAR text[STSTRING], tbuff[STSTRING];
	va_list aptr;
	int action;

	if (!api_active)	{
		LoadString(g_hinst, warnno,text,STSTRING);
		va_start(aptr, warnno);		 /* initialize arg pointer */
		u_vsprintf_u(tbuff, text, aptr); /* get string */
		va_end(aptr);
		action = MessageBox(NULL,tbuff,cinname,MB_TASKMODAL|MB_ICONWARNING|MB_YESNOCANCEL| MB_TOPMOST);	/* display alert */
		if (action == IDYES)
			return (1);
		if (action == IDNO)
			return (-1);
	}
	return (FALSE);
}
/*******************************************************************************/
short senderr(const int errnum, const int level, ...)

{
	TCHAR text[STSTRING], tbuff[MAXREC];
	short bcount;
	va_list aptr;

	if (api_active)
		return -1;
	LoadString(g_hinst, errnum,text,STSTRING);
	va_start(aptr, level);		 /* initialize arg pointer */
	u_vsprintf_u(tbuff, text, aptr); /* get string */
	va_end(aptr);
	if (xt_lasterr != errnum || time(NULL) > xt_lasttime+10 || nstrcmp(tbuff,xt_laststring)) 	{	/* if changed error or more than 10 sec or diff string */
		xt_lasterr = errnum;
		xt_samecount = 0;		/* reset alert stage */
		xt_lasttime = time(NULL);
	}
	nstrcpy(xt_laststring,tbuff);
	xt_samecount &= 3;			/* limit alert level */
	for (bcount = 0; bcount < exx[level][xt_samecount].beep; bcount++)
		MessageBeep(MB_ICONHAND);
	if (exx[level][xt_samecount].box)
		MessageBox(NULL,tbuff,cinname,MB_TASKMODAL|MB_ICONERROR|MB_OK| MB_TOPMOST);	/* display alert */
	xt_samecount++;	/* counts number of identical errors */
	err_eflag = -1;
	return (err_eflag);
}
/*******************************************************************************/
short senderronstatusline(const int errnum, const int level, ...)

{
	TCHAR text[STSTRING], tbuff[MAXREC];
	short bcount;
	va_list aptr;

	LoadString(g_hinst, errnum,text,STSTRING);
	va_start(aptr, level);		 /* initialize arg pointer */
	u_vsprintf_u(tbuff, text, aptr); /* get string */
	va_end(aptr);
	if (xt_lasterr != errnum || time(NULL) > xt_lasttime+10 || nstrcmp(tbuff,xt_laststring)) 	{	/* if changed error or more than 10 sec or diff string */
		xt_lasterr = errnum;
		xt_samecount = 0;		/* reset alert stage */
		xt_lasttime = time(NULL);
	}
	nstrcpy(xt_laststring,tbuff);
	xt_samecount &= 3;			/* limit alert level */
	for (bcount = 0; bcount < exx[level][xt_samecount].beep; bcount++)
		MessageBeep(MB_ICONHAND);
	if (exx[level][xt_samecount].box)	{
		SendMessage(g_hwstatus,SB_SIMPLE,TRUE,0);	/* set simple status window */
		SendMessage(g_hwstatus,SB_SETTEXT,255|SBT_NOBORDERS,(LPARAM)tbuff);	/* display on status line */
	}
	xt_samecount++;	/* counts number of identical errors */
	err_eflag = -1;
	return (err_eflag);
}
/*******************************************************************************/
void senditemerr(HWND hwnd,int item)	/* flags item error and sets focus */

{
	TCHAR tstring[STSTRING];
	HWND hwed;

	hwed = GetDlgItem(hwnd,item);
	GetWindowText(hwed,tstring,STSTRING);
	senderr(ERR_BADITEMTEXT,WARNNB,tstring);
	SetFocus(hwed);
	SendMessage(hwed,EM_SETSEL,0,-1);
}