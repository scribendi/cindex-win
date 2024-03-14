#include "stdafx.h"
#include <afxres.h>
#include <htmlhelp.h>
#include "files.h"
#include "errors.h"
#include "strings.h"
#include "util.h"
#include "registry.h"
#include "macros.h"
#include "sort.h"
#include "text.h"
#include "utime.h"

UINT WMM_DRAGMESSAGE;		/* registered drag list message */

#define shiftstring(source,count) memmove((source)+(short)(count),source,strlen(source)+1)

char codecharset[] = {CODECHR,FONTCHR,0};
//static int dpiforwindow(HWND hwnd);

/***************************************************************************/
NONCLIENTMETRICS * getconfigdpi(HWND hwnd)

{
	static NONCLIENTMETRICS ncm;

	ncm.cbSize = sizeof(ncm);
	FARPROC api = (FARPROC)GetProcAddress(GetModuleHandle(TEXT("User32.dll")), "SystemParametersInfoForDpi");
	if (api)
		api(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0, dpiforwindow(hwnd));
	else
		SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);
	return &ncm;
}
/***************************************************************************/
int getsysmetrics(HWND hwnd, int index)

{
	FARPROC api = (FARPROC)GetProcAddress(GetModuleHandle(TEXT("User32.dll")), "GetSystemMetricsForDpi");
	return api ? api(index, dpiforwindow(hwnd)) : GetSystemMetrics(index);
}
/***************************************************************************/
int dpiforwindow(HWND hwnd)

{
	FARPROC api = (FARPROC)GetProcAddress( GetModuleHandle(TEXT("User32.dll")), "GetDpiForWindow");
	return api ? api(hwnd) : g_slpix;
}
/***************************************************************************/
void adjustWindowRect(HWND hwnd, LPRECT prect, DWORD style, BOOL menu, DWORD exstyle)

{
	FARPROC api = (FARPROC)GetProcAddress(GetModuleHandle(TEXT("User32.dll")), "AdjustWindowRectExForDpi");
	if (api)
		api(prect, style, menu, exstyle, dpiforwindow(hwnd));	// get rect needed for whole view window
	else
		AdjustWindowRectEx(prect, style, menu, exstyle);	// get rect needed for whole view window
}
/***************************************************************************/
int scaleForDpi(HWND hwnd, int input)

{
	return MulDiv(input, dpiforwindow(hwnd), 96);
}
/***************************************************************************/
void scaleRectForDpi(HWND hwnd, RECT * rptr)	// scales standard to display on high res

{
	int dpi = dpiforwindow(hwnd);
	rptr->left = (rptr->left*dpi) / 96;
	rptr->top = (rptr->top*dpi) / 96;
	rptr->bottom = (rptr->bottom*dpi) / 96;
	rptr->right = (rptr->right*dpi) / 96;
}
/***************************************************************************/
void scaleRectFromDpi(HWND hwnd, RECT * rptr)	// scales high res to display on standard

{
	int dpi = dpiforwindow(hwnd);
	rptr->left = (rptr->left*96) / dpi;
	rptr->top = (rptr->top*96) / dpi;
	rptr->bottom = (rptr->bottom*96) / dpi;
	rptr->right = (rptr->right*96) / dpi;
}
/***************************************************************************/
void layoutForDpi(HWND hwnd, int x, int y, int w, int h)

{
	int iDpi = dpiforwindow(hwnd);
	int dpiScaledX = MulDiv(x, iDpi, 96);
	int dpiScaledY = MulDiv(y, iDpi, 96);
	int dpiScaledWidth = MulDiv(w, iDpi, 96);
	int dpiScaledHeight = MulDiv(h, iDpi, 96);
	SetWindowPos(hwnd, hwnd, dpiScaledX, dpiScaledY, dpiScaledWidth, dpiScaledHeight, SWP_NOZORDER | SWP_NOACTIVATE);
}
/***************************************************************************/
void configurestatusbar(HWND hwnd, int count, int * widths)	// sets up scaled segment sizes

{
	int iDpi = dpiforwindow(hwnd);
	int awidths[15];

	for (int lcount = 0; lcount < count; lcount++) {
		awidths[lcount]= MulDiv(widths[lcount], iDpi, 96);
	}
	SendMessage(hwnd, SB_SETPARTS, count, (LPARAM)awidths);
}
/***************************************************************************/
void fixsmallcaps(TCHAR * buffer)	// cleans up small caps codes

{
	TCHAR * sptr = buffer;

	while (*sptr)	{
		if (*sptr++ == CODECHR)	{
			if (*sptr++&FX_SMALL && !(*(sptr-1)&FX_OFF))	{	// we're interested
				TCHAR * eptr, * mark;

				mark = sptr;		// save first code position
				for (eptr = sptr; *eptr; eptr++)	{	// until reach next small caps
					if (*eptr == CODECHR && *(eptr+1)&FX_SMALL)
						break;
				}
				do {
					TCHAR *base = sptr;	// mark start of text to examine
					while (sptr < eptr && !u_islower(*sptr))	// find length of run that's not lower case
						sptr++;
					if (sptr > base) {		// some non l.c. run exists; need to insert off code
						if (mark)	{		// if still have original on code
							*(mark-1) &= ~FX_SMALL;		// clear it
							if (!*(mark-1))
								*(mark-1) = FX_OFF;
						}
						else {
							memmove(base+2, base, (nstrlen(base)+1)*2);	// make gap & insert code
							*base++ = CODECHR;
							*base++ = FX_SMALL|FX_OFF;
							sptr += 2;
							eptr += 2;
						}
					}
					else {	// must have some lower case
						if (!mark)	{		// if don't have original on code
							memmove(sptr+2, sptr, (nstrlen(sptr)+1)*2);	// make gap & insert code
							*sptr++ = CODECHR;
							*sptr++ = FX_SMALL;
							eptr += 2;
						}
						while (u_islower(*sptr) && sptr < eptr)	// move over the run
							sptr++;
					}		
					mark = NULL;
				} while (sptr < eptr);
			}
		}
	}
}
/***************************************************************************/
void appendString(HWND hw, char *string)	// appends string to edit text

{
	TCHAR * sptr;
	static UChar u16str[8192];
	UErrorCode error = U_ZERO_ERROR;
	int32_t olength;

	u_strFromUTF8(u16str,sizeof(u16str),&olength,string,-1,&error);
	sptr = u16str;
	SendMessage(hw,EM_REPLACESEL,FALSE,(LPARAM)sptr);
}
/***************************************************************************/
char * fromNative(TCHAR * string)	// converts to utf-8 from native

{
	static char u8str[4096];
	UErrorCode error = U_ZERO_ERROR;
	int32_t olength;

	u_strToUTF8(u8str,sizeof(u8str),&olength,string,-1,&error);
	return u8str;
}
/***************************************************************************/
TCHAR * toNative(char * string)	// converts to native from utf-8

{
	static UChar u16str[4096];
	UErrorCode error = U_ZERO_ERROR;
	int32_t olength;

	u_strFromUTF8(u16str,sizeof(u16str),&olength,string,-1,&error);
	return u16str;
}
/***************************************************************************/
UINT getDItemText(HWND hwnd, int item, char * dest, int length)

{
	TCHAR istring[1024];
	UErrorCode error = U_ZERO_ERROR;
	UINT result = GetDlgItemText(hwnd,item,istring,sizeof(istring));

	memset(dest,0,length);	// clear destination
	if (result)
		u_strToUTF8(dest,length-1,&result,istring,-1,&error);
	return result;
}
/***************************************************************************/
BOOL setDItemText(HWND hwnd, int item, char * source)

{
	TCHAR ostring[1024];
	UErrorCode error = U_ZERO_ERROR;
	int32_t olength;

	u_strFromUTF8(ostring,sizeof(ostring),&olength,source,-1,&error);
	return SetDlgItemText(hwnd, item,ostring);	/* set text */
}
/***************************************************************************/
BOOL iscodechar(short cc)	// tests if character is code char

{
	if (cc == CODECHR || cc == FONTCHR)
		return TRUE;
	return  FALSE;
}
/***************************************************************************/
int isfontcodeptr(char *cc)	// tests if code represents font

{
	if (*cc++ == FONTCHR && *cc&FX_FONT)
		return TRUE;
	return FALSE;
}
#if 0
/*******************************************************************************/
unichar symbolcharfromroman(unsigned char schar)	// return unichar value of symbol font character

{
	return symbolsource[schar] ? symbolsource[schar] : schar;
}
#endif
/***************************************************************************/
char * u8_back1(char * ptr)	// moves back one code point

{
	int sindex = 0;
	U8_BACK_1_UNSAFE(ptr,sindex);
	return ptr+sindex;
}
/***************************************************************************/
char * u8_forward1(char * ptr)	// moves forward one code point

{
	int sindex = 0;
	U8_FWD_1_UNSAFE(ptr,sindex);
	return ptr+sindex;
}
/***************************************************************************/
unichar u8_toU(char * ptr)	// returns single char from utf-8

{
	unichar cc;
	U8_GET_UNSAFE(ptr,0,cc);
	return cc;
}
/***************************************************************************/
unichar u8_prevU(char ** pptr)	// returns single char from utf-8

{
	unichar cc;
	int sindex = 0;
	
	U8_PREV_UNSAFE(*pptr,sindex,cc);
	*pptr += sindex;
	return cc;
}
/***************************************************************************/
unichar u8_nextU(char ** pptr)	// returns single char from utf-8

{
	unichar cc;
	int sindex = 0;
	
	U8_NEXT_UNSAFE(*pptr,sindex,cc);
	*pptr += sindex;
	return cc;
}
/***************************************************************************/
char * u8_appendU(char * ptr, unichar uc)	// appends utf8 string for unichar

{
	int sindex = 0;
	
	U8_APPEND_UNSAFE(ptr,sindex,uc);
	return ptr+sindex;
}
/***************************************************************************/
char * u8_insertU(char * ptr, unichar uc, int gapchars)	// inserts utf8 string for unichar

{
	int length = U8_LENGTH(uc);
	int sindex = 0;

	if (length > gapchars)
		shiftstring(ptr+1,length-gapchars);
	U8_APPEND_UNSAFE(ptr,sindex,uc);
	return ptr+length;
}
/***************************************************************************/
int u8_countU(char * ptr, int length)	// counts uchars for length

{
	int ucount, ccount;
	
	// which method is faster?
#if 0
	UErrorCode error = U_ZERO_ERROR;
	u_strFromUTF8(NULL,0,&ucount,ptr,length,&error);		// preflighting count
#else
	for (ucount = ccount = 0; ccount < length; ucount++)
		U8_FWD_1_UNSAFE(ptr,ccount);
#endif
	return ucount;
}
/****************************************************************************************/
BOOL u8_isvalidUTF8(char * ptr, int32_t length)	// checks validity of utf8

{
	UErrorCode error = U_ZERO_ERROR;
	unichar * dest = malloc(length * sizeof(unichar));
	int32_t destlength = 0;
	u_strFromUTF8(dest, length, &destlength, ptr, length, &error);
	free(dest);
	return U_SUCCESS(error);
}
/***************************************************************************************/
void u8_normalize(char * ptr, int length)		// normalizes utf-8 string to composed characters

{
	static const UNormalizer2 * n2;
	unichar * sourcestring = malloc(length * sizeof(unichar));
	UErrorCode error = U_ZERO_ERROR;
	int32_t sourcelength, cleanlength;

	if (!n2) {
		n2 = unorm2_getInstance(NULL, "nfc", UNORM2_COMPOSE, &error);
		error = U_ZERO_ERROR;
	}
	u_strFromUTF8(sourcestring, length, &sourcelength, ptr, length, &error);
	if (error == U_ZERO_ERROR) {
		cleanlength = unorm2_spanQuickCheckYes(n2, sourcestring, sourcelength, &error);
		if (cleanlength < sourcelength && error == U_ZERO_ERROR) {		// if need normalization
			unichar * normstring = malloc(2 * sourcelength * sizeof(unichar));
			int32_t normlength, utf8length;

			normlength = unorm2_normalize(n2, sourcestring, sourcelength, normstring, 2 * sourcelength * sizeof(unichar), &error);
			if (error == U_ZERO_ERROR)
				u_strToUTF8(ptr, length, &utf8length, normstring, normlength, &error);
			free(normstring);
		}
	}
	free(sourcestring);
}
/******************************************************************************/
void setint(HWND dptr, int item, long ival)	/* sets text int for dialog item */

{
	SetDlgItemInt(dptr, item,ival,TRUE);	/* set text */
}
/******************************************************************************/
void setfloat(HWND dptr, int item, float fval)	/* sets text int for dialog item */

{
	dprintf(dptr,item,fval ? "%.3g" : "0",fval);
}
/******************************************************************************/
BOOL checkdate(HWND hwnd, int item)		// gets date from dlg item

{
	char buffer[100];

	if (GetForegroundWindow() == hwnd && getDItemText(hwnd, item, buffer, 100) > 0 && time_dateValue(buffer) <= 0) {	// if bad date
		MessageBeep(MB_ICONWARNING);
		HWND hwed = GetDlgItem(hwnd, item);
		SetFocus(hwed);
		SendMessage(hwed, EM_SETSEL, 0, -1);
		return FALSE;
	}
	return TRUE;
}
/******************************************************************************/
BOOL getshort(HWND dptr, int item,short * val)		/* gets short from dialog item */

{
	BOOL error;

	*val = (short)GetDlgItemInt(dptr, item,&error,TRUE);
	return (error);
}
/******************************************************************************/
BOOL getlong(HWND dptr, int item,long * val)		/* gets long from dialog item */

{
	BOOL error;

	*val = GetDlgItemInt(dptr, item,&error,TRUE);
	return (error);
}
/*******************************************************************************/
short getfloat(HWND hwnd, short item, float * ddptr)	/* tests, loads double */

{
	char tstring[STSTRING];
	char * eptr;
	double tdouble;
	
	getDItemText(hwnd, item,tstring,STSTRING);		/* recover it */
	tdouble = strtod(tstring, &eptr);	/* set val */
	if (*eptr)						/* if a bad number */
		return (FALSE);
	if (ddptr)
		*ddptr = (float)tdouble;
	return (TRUE);
}
/******************************************************************************/
long findgroupcheck(HWND hwnd, int startitem, int enditem)		/* finds checked item in group */

{
	while (startitem <= enditem)	{
		if (isitemchecked(hwnd,startitem))	/* if checked */
			return(startitem);
		startitem++;
	}
	return (enditem); 
}
/*******************************************************************************/
void selectitext(HWND hwnd, int item)	/* selects text and sets focus */

{
	HWND twnd;

	twnd = GetDlgItem(hwnd,item);
	SendMessage(twnd,EM_SETSEL,0,-1);
	SetFocus(twnd);
}
/*******************************************************************************/
void dprintf(HWND dptr, int item, char *fstring, ...)	 /* does printf to dialog item */

{
	va_list aptr;
	TCHAR tbuff[256];

	va_start (aptr, fstring);			/* initialize arg pointer */
	u_vsprintf(tbuff,fstring, aptr);	/* get string */
	va_end(aptr);
	SetDlgItemText(dptr, item,tbuff);	/* set text */
}
/*******************************************************************************/
short buildheadingmenu(HWND lbh, INDEXPARAMS * ig, int item)	 /* build heading menu */

{
	int count;

	for (count = 0; count < ig->maxfields-1; count++)	/* make field list */
		ComboBox_AddString(lbh,toNative(ig->field[count].name));
	ComboBox_SetCurSel(lbh,item >= 0 && item < count ? item : 0);
	return (count);
}
/*******************************************************************************/
short buildunitmenu(HWND lbh,int item)	 /* build unit menu */

{
	int count;
	static TCHAR *unit[4] = {TEXT("Inch"),TEXT("Mm"),TEXT("Point"),TEXT("Pica")};

	for (count = 0; count < sizeof(unit)/sizeof(char *); count++)	/* make unit list */
		ComboBox_AddString(lbh,unit[count]);
	ComboBox_SetCurSel(lbh,item);
	return (count);
}
/*******************************************************************************/
void * setdata(HWND hwnd,void * vptr)		/* sets data associated with window */
{
	SetWindowLongPtr(hwnd, GWLP_USERDATA,(LONG_PTR)vptr);
	return ((void *)(vptr));
}
/*******************************************************************************/
void freedata(HWND hwnd)		/* frees data associated with window */
{
	void * ptr = (void *)GetWindowLongPtr(hwnd, GWLP_USERDATA);

	freemem(ptr);
	SetWindowLongPtr(hwnd, GWLP_USERDATA,0);
	return;
}
/*******************************************************************************/
short numdigits(long number)		/* returns number of digits in the number */

{
	int count;
	long tnum;
	
	for (count = 0,tnum = 1; tnum < number; tnum *= 10, count++)
		;
	return ((short)count);
}
/*******************************************************************************/
void fixleadspace(TCHAR * buff)		// replaces leading spaces with fixed spaces

{
	while (*buff == SPACE)
		*buff++ = FSPACE;
}
/*******************************************************************************/
int toroman(char * string, int num, int upperflag)		/* makes roman numeral from int */

{
	char * sptr;
	int index;
	struct rgroup {
		int arab1;
		char roman1;
		int arab5;
		char roman5;
	};
	static struct rgroup base[4] = {
		{1,'i',5,'v'},
		{10,'x', 50,'l'},
		{100,'c', 500,'d'},
		{1000,'m', 0,'-'}
	};

	for (sptr = string, index = 3; index >= 0; index--)	{		// for 1000's downwards
		if (index < 3)	{	/* if < 1000 */
			if (num >= base[index].arab5+4*base[index].arab1)	{		// parts >= 5x + 4x 
				*sptr++ = base[index].roman1;		// 1x
				*sptr++ = base[index+1].roman1;		// 10x
				num -= base[index].arab5+4*base[index].arab1;
			}
			else if (num >= base[index].arab5)	{	// parts >= 5x 
				*sptr++ = base[index].roman5;		// 5x
				num -= base[index].arab5;
			}
			else if (num >= 4*base[index].arab1)	{	// parts >= 4x
				*sptr++ = base[index].roman1;		// 1x
				*sptr++ = base[index].roman5;		// 5x
				num -= 4*base[index].arab1;
			}
		}
		while (num >= base[index].arab1)	{	// parts >= 1x
			*sptr++ = base[index].roman1;		// 1x
			num -= base[index].arab1;
		}
	}
	*sptr = '\0';
	if (upperflag)
		strupr(string);
	return (sptr-string);
}
/*******************************************************************************/
void menu_gettext(HMENU mh, int mid, TCHAR * text)		/* gets text of item ID */

{
	MENUITEMINFO mi;

	memset(&mi,0,sizeof(mi));
	mi.cbSize = sizeof(mi);
	mi.fMask = MIIM_TYPE;
	mi.dwTypeData = text;
	mi.cch = LF_FACESIZE;	/* space available for string */
	GetMenuItemInfo(mh,mid,FALSE,&mi);
}
/*******************************************************************************/
void menu_addsorteditem(HMENU mh, MENUITEMINFO *nmi)	// adds item in alpha position

{
	int index;
	MENUITEMINFO mi;
	TCHAR text[100];

	memset(&mi,0,sizeof(mi));
	mi.cbSize = sizeof(mi);
	mi.fMask = MIIM_TYPE;
	mi.dwTypeData = text;
	mi.cch = 100;	/* space available for string */
	for (index = 0; index < GetMenuItemCount(mh); index++)	{
		if (GetMenuItemInfo(mh,index,TRUE,&mi))	{
			if (nstricmp(nmi->dwTypeData, text) < 0)
				break;
		}
	}
	InsertMenuItem(mh,index,TRUE,nmi);
}
#if 0
/*******************************************************************************/
int menu_getid(HMENU mh, TCHAR * text)	/* returns ID of item matching text */

{
	MENUITEMINFO mi;
	TCHAR buff[STSTRING];
	int index;

	for (index = GetMenuItemCount(mh)-1; index >= 0; index--)	{
		mi.cbSize = sizeof(mi);
		mi.fMask = MIIM_TYPE|MIIM_ID;
		mi.dwTypeData = buff;
		mi.cch = STSTRING;	/* space available for string */
		if (GetMenuItemInfo(mh,index,TRUE,&mi))	{	/* if a reasonable item (not separator, etc) */
			if (!strcmp(buff,text))
				return (mi.wID);
		}
	}
	return (0);
}
/*******************************************************************************/
int menu_getindex(HMENU mh, char * text)	/* returns position index of item matching text */

{
	MENUITEMINFO mi;
	char buff[STSTRING];
	int index;

	for (index = GetMenuItemCount(mh)-1; index >= 0; index--)	{
		mi.cbSize = sizeof(mi);
		mi.fMask = MIIM_TYPE|MIIM_ID;
		mi.dwTypeData = buff;
		mi.cch = STSTRING;	/* space available for string */
		if (GetMenuItemInfo(mh,index,TRUE,&mi))	{	/* if a reasonable item (not separator, etc) */
			if (!strcmp(buff,text))
				return (index);
		}
	}
	return (-1);
}
/*******************************************************************************/
#endif
HMENU menu_gethandle(int mid)	/* returns handle of submenu */

{
	MENUITEMINFO mi;
	HMENU mh;

	memset(&mi,0,sizeof(mi));
	mi.cbSize = sizeof(mi);
	mi.fMask = MIIM_SUBMENU;

	mh = GetMenu(g_hwframe);
	if (GetMenuItemInfo(mh,mid,FALSE,&mi))	// if in main menu
		return (mi.hSubMenu);
	return NULL;
}
/*******************************************************************************/
void menu_setlabelcolors(HMENU hmenu)	// sets bcolored bitmaps for labels

{
	int index;
	MENUITEMINFO mi;

	memset(&mi,0,sizeof(mi));
	mi.cbSize = sizeof(mi);
	mi.fMask = MIIM_BITMAP;

	for (index = 1; index < FLAGLIMIT; index++)	{
		mi.hbmpItem = g_labelmaps[index];
		SetMenuItemInfo(hmenu,index,TRUE,&mi);
	}
}
/*******************************************************************************/
int menu_getmid(HMENU hmenu)	// returns id of menu

{
	MENUINFO mmi;

	mmi.cbSize = sizeof(mmi);
	mmi.fMask = MIM_MENUDATA;
	GetMenuInfo(hmenu,&mmi);
	return (int)mmi.dwMenuData;
}
/*******************************************************************************/
void menu_delall(HMENU mh)	/* deletes all items */

{
	int icount, err;

	icount = GetMenuItemCount(mh);
	while (icount--)
		err = DeleteMenu(mh,icount,MF_BYPOSITION);
}
/*******************************************************************************/
void menu_addsubmenu(HMENU mh, HMENU sm, char * title)	// appends submenu

{
	MENUITEMINFO mi;
	int count;

	memset(&mi,0,sizeof(mi));
	mi.cbSize = sizeof(mi);
	mi.fMask = MIIM_SUBMENU|MIIM_TYPE;
	mi.hSubMenu = sm;
	mi.dwTypeData = toNative(title);			/* text */

	count = GetMenuItemCount(mh);	/* get item count */
	InsertMenuItem(mh,count,TRUE,&mi);	/* appends */
}
/*******************************************************************************/
void menu_additem(HMENU mh, int mid, char * text)	/* appends item */

{
	MENUITEMINFO mi;
	int count;

	memset(&mi,0,sizeof(mi));
	mi.cbSize = sizeof(mi);
	if (!strcmp(text,"-"))	{	// if want separator
		mi.fMask = MIIM_ID|MIIM_TYPE;
		mi.fType = MFT_SEPARATOR;
	}
	else	{
		mi.fMask = MIIM_ID|MIIM_TYPE;
		mi.wID = mid;					/* id */
		mi.dwTypeData = toNative(text);			/* text */
	}
	count = GetMenuItemCount(mh);	/* get item count */
	InsertMenuItem(mh,count,TRUE,&mi);	/* appends */
}
/*******************************************************************************/
void combo_addfiles(HWND cb, TCHAR * dir)		// adds files from dir

{
	WIN32_FIND_DATA data;
	HANDLE handle;
	TCHAR specpath[MAX_PATH];

	nstrcpy(specpath,dir);
	handle = FindFirstFile(specpath,&data);
	if (handle != INVALID_HANDLE_VALUE)	{
		do {
			TCHAR name[_MAX_FNAME];
			int index;

			nstrcpy(name,data.cFileName);
			PathRemoveExtension(name);
			SendMessage(cb,CB_INSERTSTRING,-1,(LPARAM)name);	// add file name
			index = SendMessage(cb,CB_GETCOUNT,0,0)-1;
			SendMessage(cb,CB_SETITEMDATA,index,(LPARAM)dir);	// set ptr to directory
		} while (FindNextFile(handle,&data));
		FindClose(handle);
	}
}
/*******************************************************************************/
void tbreturninfo(LPNMTOOLBAR tbnptr, TBBUTTON * tbb, int max, struct ttmatch *stbase)	/* fills out button info */

{
	int count;
	TCHAR * eptr;

	tbnptr->tbButton = tbb[tbnptr->iItem];	/* load button info */
	if (tbnptr->cchText)	{		/* if have a buffer */
		for (count = 0; count < max; count++)	{	/* for all strings */
			if (stbase[count].id == tbnptr->tbButton.idCommand)	{	/* if a match */
				nstrcpy(tbnptr->pszText,stbase[count].string);		/* copy text */
				if (eptr = nstrchr(tbnptr->pszText,'('))	{	/* if have parens */
					while (*(eptr-1) == SPACE)
						eptr--;
					*eptr = '\0';
				}
				return;
			}
		}
	}
}
/******************************************************************************/
void setdc(HDC dc, BOOLEAN rtl, int viewoffset)		/* sets proper device context */

{
	SetMapMode(dc,MM_ISOTROPIC);
	SetWindowExtEx(dc,72,72,NULL);
	SetViewportExtEx(dc, GetDeviceCaps(dc, LOGPIXELSX),GetDeviceCaps(dc, LOGPIXELSY), NULL);
	if (rtl)	{	// if right-to-left reading
		SetLayout(dc,LAYOUT_RTL);
		SetTextAlign(dc,TA_BASELINE|TA_UPDATECP|TA_RIGHT);
	}
	else {
		SetLayout(dc,0);
		SetTextAlign(dc,TA_BASELINE|TA_UPDATECP);
	}
	SetViewportOrgEx(dc,viewoffset,0,NULL);
	SetWindowOrgEx(dc,viewoffset,0,NULL);
}
/******************************************************************************/
void scalerect(RECT * rptr, int mul, int div)		/* scales a rectangle */

{
	rptr->left = (rptr->left*mul)/div;
	rptr->top = (rptr->top*mul)/div;
	rptr->bottom = (rptr->bottom*mul)/div;
	rptr->right = (rptr->right*mul)/div;
}
/*******************************************************************************/
void centerwindow(HWND dptr, int mode)		/* center dptr on bgnd */

{
/* usage:	mode == 0, on frame window
			<0 bottom of view window 
			>0 centered on view window */

	RECT drect, prect, wrect;
	POINT pt;
	HWND twind, bgnd;

	if (mode)	{/* trying for view window of top index */
		bgnd = g_hwclient;	/* default is center on client */
		twind = FORWARD_WM_MDIGETACTIVE(g_hwclient, SendMessage);
		if (IsWindow(twind) && WX(twind, owner))	{
			if (getmmstate(twind,NULL) != SW_SHOWMINIMIZED && getdata(twind) && WX(twind,owner)->vwind)	/* if not minimized, etc */
				bgnd = WX(twind,owner)->vwind;
		}
	}
	else	{
		bgnd = GetParent(dptr);
		if (!bgnd)
			bgnd = g_hwframe;
	}
	SystemParametersInfo(SPI_GETWORKAREA,sizeof(RECT),&wrect,0);
	GetWindowRect(dptr,&drect);	// our window rect
	GetWindowRect(bgnd,&prect);	// bgnd window rect
	if (!IsWindowVisible(bgnd))	// if bgnd isn't visible
		prect = wrect;		// set desktop as rect
	pt.x = prect.left+((prect.right-prect.left)-(drect.right-drect.left))/2;
	if (mode < 0)		/* if want positioned at bottom of background */
		pt.y = prect.bottom-(drect.bottom-drect.top)-30;
	else	/* centered */
		pt.y = prect.top+((prect.bottom-prect.top)-(drect.bottom-drect.top))/2;
	if (pt.x < wrect.left)
		pt.x = wrect.left;
	if (pt.y < wrect.top)
		pt.y = wrect.top;
	else if (pt.y+(drect.bottom-drect.top) >wrect.bottom)
		pt.y = wrect.bottom-(drect.bottom-drect.top);
	SetWindowPos(dptr,NULL,pt.x,pt.y,0,0,SWP_NOSIZE|SWP_NOZORDER);
}
/*******************************************************************************/
int isobscured (HWND hwnd, HWND hwtop)	/* finds top of obscured rect in window */

{
	RECT wrect,orect,drect;
	HWND prev, next;
	POINT pt;
	long ref;

	GetClientRect(hwnd,&wrect);
	ClientToScreen(hwnd,(POINT *)&wrect.left);
	ClientToScreen(hwnd,(POINT *)&wrect.right);

	for (ref = pt.y = wrect.bottom, prev = hwnd; next = GetWindow(prev, GW_HWNDPREV);prev = next)	{
		GetWindowRect(next,&orect);
		if (IsWindowVisible(next) && IntersectRect(&drect,&wrect,&orect)){	/* if intersection */
			if (drect.top < pt.y)	/* if top of new intersection is above old */
				pt.y = drect.top;
		}
	}
	if (hwtop)	{	/* if have modeless dialog on display */
		GetWindowRect(hwtop,&orect);
		if (IntersectRect(&drect,&wrect,&orect))	{
			if (EqualRect(&drect,&wrect))	/* if our window is wholly obscured */
				pt.y = orect.top;			/* top is top of obscuring window */
			else if (drect.top < pt.y)	/* if top of new intersection is above old */
				pt.y = drect.top;
		}
	}
	if (pt.y != ref)	{	/* if obscured */
		ScreenToClient(hwnd,&pt);
		return(pt.y);
	}
	return 0;
}
/************************************************************************/
int getmmstate(HWND hwnd, RECT *wrptr)	/* gets max/min state */

{
	WINDOWPLACEMENT wp;
	
	wp.length = sizeof(wp);
	GetWindowPlacement(hwnd, &wp);
	if (wrptr) {	// if want rect
		scaleRectFromDpi(hwnd, &wp.rcNormalPosition);	// scale potential high dpi rect to normal
		*wrptr = wp.rcNormalPosition;	// get normal size, pos (unmaximized), in parent coords
	}
	return (wp.showCmd);
}
/*******************************************************************************/
void fitrecttoclient(RECT * rptr)	//	adjusts rect to fit in MDI client window

{
	RECT vcrect;
	int cheight;

	GetClientRect(g_hwclient,&vcrect);
	cheight = vcrect.bottom-vcrect.top;
	if (!PtInRect(&vcrect,*(POINT *)&rptr->left))	{	/* if left/top not within client */
		if (rptr->bottom-rptr->top <= cheight)	/* if could fit within height of client */
			OffsetRect(rptr,vcrect.left-rptr->left,vcrect.bottom-rptr->bottom);
		else	/* make sure we show top */
			OffsetRect(rptr,vcrect.left-rptr->left,vcrect.top-rptr->top);
	}
	if (rptr->bottom-rptr->top > cheight)	// if height exceeds client height
		rptr->bottom = rptr->top+cheight;	// limit height
}
/*******************************************************************************/
HWND getactivemdi(int topflag)		/* gets active mdi window */

{
	HWND cwind;

	if (!topflag || (GetTopWindow(g_hwframe) == g_hwclient))	{	/* if don't care about top window or it's a client */
		cwind = FORWARD_WM_MDIGETACTIVE(g_hwclient, SendMessage);
		if (cwind && getmmstate(cwind,NULL) != SW_SHOWMINIMIZED)	/* if not minimized */
			return (cwind);
	}
	return (NULL);
}
/******************************************************************************/
int getcliplength(HWND hwnd, int *breaks)		/* gets clipboard text length & para breaks */

{
	HANDLE ch;
	TCHAR * bptr, *cptr;

	if (OpenClipboard(hwnd)) {
		*breaks = 0;
		if (ch = GetClipboardData(CF_UNICODETEXT))	{	/* if there's stuff to paste */
			for (bptr = cptr = GlobalLock(ch); *cptr; cptr++)	/* for whole string */
				if (*cptr == NEWLINE)
					(*breaks)++;
			GlobalUnlock(ch);
		}
		CloseClipboard();
		return (cptr-bptr);
	}
	return (0);
}
/*******************************************************************************/
void enumclipformats(HWND hwnd)

{
	UINT format = 0;
	TCHAR buffer[200]; 

	OpenClipboard(hwnd) ;
	while (format = EnumClipboardFormats(format)) {
		GetClipboardFormatName(format, buffer,200);
		NSLog(fromNative(buffer));
	} 
	CloseClipboard() ;
}
/*******************************************************************************/
void showprogress(int id, long total, long done, ...)	/* displays progress */

{
	static TCHAR text[STSTRING];
	static HCURSOR ocurs;
	static int mode;
	TCHAR tbuff[MAXREC];
	RECT srect;

	if (!g_hwprog && total)	{	/* if no window and want to display something */
		;	// marquee mode

		LoadString(g_hinst,id,text,STSTRING);
		SendMessage(g_hwstatus,SB_GETRECT,1,(LPARAM)&srect);
//		SendMessage(g_hwstatus,SB_SIMPLE,TRUE,0);	/* restore simple status window */
//		SendMessage(g_hwstatus,SB_SETTEXT,255|SBT_NOBORDERS,(LPARAM)text);	/* display on status line */
//		UpdateWindow(g_hwstatus);
		mode = done < 0 ? PBS_MARQUEE : 0;
		if (!mode)	// if not  marquee
			SendMessage(g_hwstatus,SB_SETTEXT,0,(LPARAM)text);	// display fixed text on status line
		g_hwprog = CreateWindowEx(WS_EX_WINDOWEDGE,PROGRESS_CLASS,TEXT(""),WS_CHILD|WS_VISIBLE|mode,srect.left,srect.top,280,
			srect.bottom-srect.top,g_hwstatus,(HMENU)IDC_PROGBAR,g_hinst,NULL);
		ocurs = SetCursor(g_waitcurs);

	}
	if (g_hwprog)	{		/* if have window */
		if (!total)		{		/* if all done */
			SendMessage(g_hwprog,PBM_SETPOS,100,0);	/* show complete */
			DestroyWindow(g_hwprog);
			g_hwprog = NULL;
//			SendMessage(g_hwstatus,SB_SIMPLE,FALSE,0);	/* restore status window */
			SendMessage(g_hwstatus,SB_SETTEXT,0,(LPARAM)TEXT(""));	// clear message text
			SetCursor(ocurs);
		}
		else	{
			if (done <= total)	{
				if (mode)	{ 	// if indefinite progress indicator
					va_list aptr;

					va_start(aptr, done);		 /* initialize arg pointer */
					u_vsprintf_u(tbuff, text, aptr); /* get string */
					va_end(aptr);
					SendMessage(g_hwstatus,SB_SETTEXT,0,(LPARAM)tbuff);	// display text on status line
				}
				SendMessage(g_hwprog,PBM_SETPOS,(done*100)/total,0);
			}
		}
	}
}
#if 1
/*******************************************************************************/
BOOL iscancelled(HWND hwnd)		/* looks at message queue for cancellation */

{
	MSG msg;
	RECT wrect;
	HWND cwnd;

	while (!mcr_isplaying() && (PeekMessage(&msg,hwnd,WM_KEYFIRST,WM_KEYLAST,PM_REMOVE) || PeekMessage(&msg,hwnd,WM_MOUSEFIRST,WM_MOUSELAST,PM_REMOVE)))	{	/* all messages for window (any window if null) */
		if (msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE)	/* if escape */
			return (TRUE);				/* cancel */
		if (msg.message == WM_LBUTTONDOWN && hwnd == g_mdlg)	{	/* if have a dialog */
			if (cwnd = GetDlgItem(hwnd,IDCANCEL))	{	/* if have cancel button */
				GetWindowRect(cwnd,&wrect);
				return (PtInRect(&wrect,msg.pt));
			}
		}
	}
	return (FALSE);
}
#else
/*******************************************************************************/
BOOL iscancelled(HWND hwnd)		/* looks at message queue for cancellation */

{
	MSG msg;
	RECT wrect;
	HWND cwnd;

	while (PeekMessage(&msg,hwnd,WM_KEYFIRST,WM_KEYLAST,PM_REMOVE) || PeekMessage(&msg,hwnd,WM_COMMAND,WM_COMMAND,PM_REMOVE))	{	/* all messages for window (any window if null) */
		if (msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE)	/* if escape */
			return (TRUE);				/* cancel */
		else if (msg.message == WM_COMMAND)	{
			if (LOWORD(msg.wParam) == IDCANCEL)
				return (TRUE);
		}
	}
	return (FALSE);
}
#endif
/******************************************************************************/
int builddraglist(HWND hwdl, char * string, short * list)	/* builds drag list */

{
	TCHAR tbuff[STSTRING];
	int count, tcount, lcount, icount,ts[2];
			
	ts[0] = 12;
	ListBox_ResetContent(hwdl);
	ListBox_SetTabStops(hwdl,1,ts);
	tcount = str_xcount(string);
	for (icount = 0; icount < tcount && list[icount] >= 0; icount++)	{ /* for enabled strings */
		u_sprintf(tbuff,"+\t%s", str_xatindex(string,list[icount]));
		ListBox_AddString(hwdl,tbuff);
		ListBox_SetItemData(hwdl,icount,list[icount]);
	}
	for (count = 0; count < tcount; count++)	{	/* for all strings */
		for (lcount = 0; list[lcount] >= 0; lcount++)	{	/* for all items in enabled list */
			if (count == list[lcount])		/* if current is one */
				break;
		}
		if (list[lcount] < 0)	{		/* if we're not already in list */
			u_sprintf(tbuff,"\t%s",str_xatindex(string,count));
			ListBox_AddString(hwdl,tbuff);
			ListBox_SetItemData(hwdl,icount++,count);
		}
	}
	WMM_DRAGMESSAGE = RegisterWindowMessage(DRAGLISTMSGSTRING);
	return (tcount);	/* number of items in list */
}
/******************************************************************************/
void recoverdraglist(HWND hwdl, short * list)	/* recovers list order of drag items */

{
	TCHAR tstring[STSTRING];
	short count, tcount, acount;
	
	for (acount = count = 0, tcount = ListBox_GetCount(hwdl); count < tcount; count++)	{
		ListBox_GetText(hwdl,count,tstring);		/* recover text to check enabling */
		if (*tstring != '\t')		/* if enabled (covers both checkable and uncheckable lists) */
			list[acount++] = (short)ListBox_GetItemData(hwdl,count);	/* assign id of token */
	}
	list[acount] = -1;	/* terminate the list of active items */
}
/**********************************************************************************/	
void switchdragitem (HWND hwdl)	/* handles double-click on drag item */

{
	int index, data;
	TCHAR tstring[STSTRING];

	*tstring = '+';	/* set special char as optional lead */
	index = ListBox_GetCurSel(hwdl);
	ListBox_GetText(hwdl,index,tstring+1);
	data = ListBox_GetItemData(hwdl,index);
	ListBox_DeleteString(hwdl,index);
	if (*(tstring+1) == '+')	/* if checked */
		ListBox_InsertString(hwdl,index, tstring+2);
	else
		ListBox_InsertString(hwdl,index,tstring);
	ListBox_SetItemData(hwdl,index,data);
	ListBox_SetCurSel(hwdl,index);
}
/**********************************************************************************/	
int handledrag (HWND hwnd, LPARAM lParam, int * sindex)	/* handles drag */

{
	HWND hwdl;
	int dindex, data;
	char tstring[STSTRING];

	hwdl = ((DRAGLISTINFO *)lParam)->hWnd;
	switch (((DRAGLISTINFO *)lParam)->uNotification)	{	/* what's happening */
		case DL_BEGINDRAG:
			*sindex = LBItemFromPt(hwdl,((DRAGLISTINFO *)lParam)->ptCursor,TRUE);
			return (TRUE);
		case DL_CANCELDRAG:
			DrawInsert(GetParent(hwdl),hwdl,-1);	/* kill drag icon */
			return (TRUE);
		case DL_DRAGGING:
			dindex = LBItemFromPt(hwdl,((DRAGLISTINFO *)lParam)->ptCursor,TRUE);
			DrawInsert(GetParent(hwdl),hwdl,dindex);
			if (dindex >= 0)
				return (DL_MOVECURSOR);
			return (DL_STOPCURSOR);
		case DL_DROPPED:
			DrawInsert(GetParent(hwdl),hwdl,-1);	/* kill drag icon */
			dindex = LBItemFromPt(hwdl,((DRAGLISTINFO *)lParam)->ptCursor,TRUE);
			if (dindex >= 0)	{	/* if in good zone */
				ListBox_GetText(hwdl,*sindex,tstring);
				data = ListBox_GetItemData(hwdl,*sindex);
				ListBox_DeleteString(hwdl,*sindex);
				ListBox_InsertString(hwdl,dindex,tstring);
				ListBox_SetItemData(hwdl,dindex,data);
				ListBox_SetCurSel(hwdl,dindex);
			}
		default:
			return (TRUE);
	}
}
/*******************************************************************************/
BOOL dodialoghelp(HWND hwnd, TCHAR * file, HELPINFO * hp, const int * idarray)	/* does dialog help */

{
	TCHAR helppath[MAX_PATH]; 

	if (hp->iContextType == HELPINFO_WINDOW)	{	/* for control */
		if (hp->iCtrlId != IDC_STATIC)	{	/* if not static */
			file_makecinpath(helppath,TEXT("Cindex.chm"));
			if (GetAsyncKeyState(VK_LBUTTON) < 0)	// if button down
				HtmlHelp(hp->hItemHandle,helppath,HH_TP_HELP_WM_HELP,(DWORD_PTR)idarray);
			else
				dowindowhelp(file ? file : TEXT("base\\Cindex Help.htm"));
		}
		return TRUE;
	}
	return FALSE;
}
/*******************************************************************************/
void dowindowhelp(TCHAR * file)	/* does window level help */

{
	TCHAR helppath[MAX_PATH];

	file_makecinpath(helppath,TEXT("Cindex.chm"));
	nstrcat(helppath,TEXT("::/WinCin Help\\web-content\\"));
	nstrcat(helppath,file);
	HWND hw = HtmlHelp(g_hwframe,helppath,HH_DISPLAY_TOPIC,(DWORD_PTR)NULL);
#if 0
	{
		TCHAR helppath[MAX_PATH];
		HH_WINTYPE wtp = { 0 };
		PHH_WINTYPE wtpp;
		wtp.cbStruct = sizeof(wtp);
		wtpp = &wtp;

		file_makecinpath(helppath, TEXT("Cindex.chm"));
		nstrcat(helppath,TEXT(">HTML Help Viewer"));
		HWND xx = HtmlHelp(NULL, helppath, HH_GET_WIN_TYPE, (DWORD_PTR)&wtp);
		int yy = 10;
	}
#endif
}
/**********************************************************************************/
int geteditclass(HWND hwnd)	/* returns -1,0, 1 */

{
	TCHAR tstring[STSTRING];

	GetClassName(hwnd,tstring,STSTRING);
	if (!nstricmp(tstring,TEXT("RichEdit20A")))
		return 1;				/* rich edit */
	else if (!nstrcmp(tstring,TEXT("Edit")))
		return 0;				/* edit */
	else						/* uneditable */
		return -1;
}
/**********************************************************************************/
void adjustsortfieldorder(short * fieldorder, short oldtot, short newtot)	/* expands/contracts field order table, or warns */

{
	int count;

	if (oldtot != newtot)	{	/* if changed number of fields */
		for (count = 0; count < newtot && fieldorder[count] >= 0;)	{	/* until checked all fields we'll want to use */
			if (fieldorder[count] > newtot-2 && fieldorder[count] != PAGEINDEX)	/* if using a field we'll discard */
				memmove(&fieldorder[count],&fieldorder[count+1],(FIELDLIM-count)*sizeof(short));	/* copy over it */
			else
				count++;
		}
		/* now old array is adjusted for lost fields */
		if (sort_isinfieldorder(fieldorder,oldtot < newtot ? oldtot : newtot))	/* if currently in field order */
			sort_buildfieldorder(fieldorder,oldtot,newtot);
		else if (newtot > oldtot)	/* if added new fields that we don't know how to use */
			sendinfo(INFO_SORTORDER);
	}
}
/**********************************************************************************/
time_t timegm(struct tm * rt)	// makes gm time from local time

{
#if 1
	rt->tm_isdst = _daylight ? 1 : 0;
	return _mkgmtime(rt);
#else
	static long tdiff;

	if (!tdiff)
		_get_timezone(&tdiff);
	return mktime(rt)-tdiff;
#endif
}
/*******************************************************************************/
void fixcombomenu(HWND hwnd, int item)	/* builds item menu for combo box */

{
	HWND cb;
	int icount, ipos;
	TCHAR tstring[STSTRING];

	cb = GetDlgItem(hwnd,item);
	GetWindowText(cb,tstring,STSTRING);
	ipos = ComboBox_FindStringExact(cb,-1,tstring);
	if (ipos)	{	/* if not first item */
		icount = ComboBox_GetCount(cb);
		if (ipos != CB_ERR)		/* if already in list */
			ComboBox_DeleteString(cb,ipos);
		else if (icount == CBSTRING_MAX)	/* or list would be too long */
			ComboBox_DeleteString(cb,icount-1);
		ComboBox_InsertString(cb,0,tstring);
		ComboBox_SetCurSel(cb,0);
	}
}
/***************************************************************************/
void checktextfield(HWND field, int limit)	// limits length of text in control

{
	if (limit)	{
		TCHAR text[1024];
		int initlength = GetWindowText(field,text,1024);
		int length = initlength;	// available buffer length allows for terminating null

//		NSLog("Checking < %d", length);
		while (strlen(fromNative(text)) >= limit)	{
			*text = '\0';
			GetWindowText(field,text,length--);
		}
		if (initlength > length)	{	// if needed to reduce length
			SetWindowText(field,text);
			SendMessage(field,EM_SETSEL,length,length);	/* show complete */
//			MessageBeep(MB_ICONASTERISK);
//			MessageBeep(MB_ICONERROR);
//			MessageBeep(MB_ICONEXCLAMATION);
//			MessageBeep(MB_ICONWARNING);
			MessageBeep(MB_OK);
//			NSLog("Reduced from %d to %d", initlength, length);
		}
	}
}
/*******************************************************************************/
void makelabelbitmaps(void)		// makes label bitmaps

{
	HDC hdc = GetDC(g_hwframe);
	HDC dc = CreateCompatibleDC(hdc);
	int bwidth = GetSystemMetrics(SM_CXMENUCHECK);
	int bheight = GetSystemMetrics(SM_CYMENUCHECK);
	int index;

	for (index = 1; index < FLAGLIMIT; index++)	{	// for all colors
		HBRUSH sb, ob;

		if (g_labelmaps[index])	// if have previous bitmap
			DeleteObject(g_labelmaps[index]);
		g_labelmaps[index] = CreateCompatibleBitmap(hdc,bwidth,bheight);
		SelectObject(dc,g_labelmaps[index]);
		sb = CreateSolidBrush(g_prefs.gen.flagcolor[index]);
		ob = SelectObject(dc,sb);
		ExtFloodFill(dc,0,0,g_prefs.gen.flagcolor[index],FLOODFILLBORDER);
		SelectObject(dc,ob);	// select former brush so that we can delete current
		DeleteObject(sb);
	}
	DeleteDC(dc);
	ReleaseDC(g_hwframe,hdc);
}
/*******************************************************************************/
void NSLog(char * message, ...)

{
#if _DEBUG
	if (g_logw)	{
		SYSTEMTIME now;
		va_list aptr;
		TCHAR tbuff[4096];

		GetSystemTime(&now);
		wsprintf(tbuff,TEXT("%.2d:%.2d:%.3d "),now.wMinute, now.wSecond,now.wMilliseconds);
		txt_append(g_logw,tbuff,NULL);
		if (message)	{
			va_start(aptr, message);
			u_vsprintf(tbuff, message, aptr);
			va_end(aptr);
		}
		else {
			DWORD err = GetLastError();
			FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,NULL,err,0,tbuff,4096,NULL);
		}
		txt_append(g_logw,tbuff,NULL);
		txt_append(g_logw,TEXT("\r\n"),NULL);
	}
#endif
}
