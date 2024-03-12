#include "stdafx.h"
#include "commands.h"
#include "util.h"
#include "keytool.h"
#include "typestuff.h"
#include "strings.h"
#include "modify.h"
#include "errors.h"

static const DWORD wh_keyid[] = {
	IDC_HOTKEY1, HIDC_HOTKEY1,
	IDC_HOTKEY2, HIDC_HOTKEY1,
	IDC_HOTKEY3, HIDC_HOTKEY1,
	IDC_HOTKEY4, HIDC_HOTKEY1,
	IDC_HOTKEY5, HIDC_HOTKEY1,
	IDC_HOTKEY6, HIDC_HOTKEY1,
	IDC_HOTKEY7, HIDC_HOTKEY1,
	IDC_HOTKEY8, HIDC_HOTKEY1,
	IDC_HOTKEY9, HIDC_HOTKEY1,
	IDC_HOTKEY10, HIDC_HOTKEY1,
	IDC_HOTKEYTEXT1, HIDC_HOTKEYTEXT1,
	IDC_HOTKEYTEXT2, HIDC_HOTKEYTEXT1,
	IDC_HOTKEYTEXT3, HIDC_HOTKEYTEXT1,
	IDC_HOTKEYTEXT4, HIDC_HOTKEYTEXT1,
	IDC_HOTKEYTEXT5, HIDC_HOTKEYTEXT1,
	IDC_HOTKEYTEXT6, HIDC_HOTKEYTEXT1,
	IDC_HOTKEYTEXT7, HIDC_HOTKEYTEXT1,
	IDC_HOTKEYTEXT8, HIDC_HOTKEYTEXT1,
	IDC_HOTKEYTEXT9, HIDC_HOTKEYTEXT1,
	IDC_HOTKEYTEXT10, HIDC_HOTKEYTEXT1,
	0,0
};

static LRESULT CALLBACK hkproc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static BOOL hkinit(HWND hwnd, HWND hwndFocus, LPARAM lParam);	/* initializes dialog */
static void hkcommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);	/* does commands */
static void hkactivate(HWND hwnd, UINT state, HWND hwndActDeact, BOOL fMinimized);	/* activates/deactivates */
static short hkclose(HWND hwnd);		/* closes char tool window */
static void loadtext(HWND ew, char * rtext);	/*  loads cindex styled text */
/*******************************************************************************/
void hk_setwindow(void)			/* sets up hotkey window */

{
	if (g_keyw || (g_keyw = CreateDialogParam(g_hinst,MAKEINTRESOURCE(IDD_HOTKEYS),g_hwframe,hkproc,(LPARAM)&g_prefs)))
		g_mdlg = g_keyw;
}
/*******************************************************************************/
void hk_register(struct prefs * pp)			/* registers hot keys */

{
	int count;
	UINT modifiers;

	for (count = 0; count < MAXKEYS; count++)		{/* for all keys */
		modifiers = 0;
		if (HIBYTE(pp->hotkeys[count])&HOTKEYF_ALT)
			modifiers |= MOD_ALT;
		if (HIBYTE(pp->hotkeys[count])&HOTKEYF_CONTROL)
			modifiers |= MOD_CONTROL;
		if (HIBYTE(pp->hotkeys[count])&HOTKEYF_SHIFT)
			modifiers |= MOD_SHIFT;
		RegisterHotKey(g_hwframe,count,modifiers, LOBYTE(pp->hotkeys[count]));	/* reregister */
	}
}
/*******************************************************************************/
void hk_unregister(void)	/* unregisters hot keys */

{
	int count;

	for (count = 0; count < MAXKEYS; count++)	/* for all keys */
		UnregisterHotKey(g_hwframe,count);	/* unregister key */
}
/************************************************************************/
static LRESULT CALLBACK hkproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)

{
	switch (msg)	{
		HANDLE_MSG(hwnd,WM_INITDIALOG,hkinit);
		HANDLE_MSG(hwnd,WM_COMMAND,hkcommand);
		HANDLE_MSG(hwnd,WM_CLOSE,hkclose);
		HANDLE_MSG(hwnd,WM_ACTIVATE,hkactivate);
		case WM_HELP:
			return (dodialoghelp(hwnd,TEXT("_Adding and Editing\\Add_hotkeys.htm"),(HELPINFO *)lParam,wh_keyid));
	}
	return (FALSE);
}
/************************************************************************/
static void hkcommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)	/* does commands */

{
	struct prefs *pp;
	char tstring[STSTRING], dstring[STSTRING],*sptr;
	int length, count, tcount;
	short newkey;

	pp = getdata(hwnd);	/* get data */
	switch (id)	{
		case IDOK:
			for (count = 0,sptr = dstring; count < MAXKEYS; count++, sptr += strlen(sptr++))	{	/* for all keys */
				newkey = (short)SendMessage(GetDlgItem(hwnd,IDC_HOTKEY1+count),HKM_GETHOTKEY,0,0);
				for (tcount = 0; tcount < count; tcount++)	{		/* check for duplicate keys */
					if (pp->hotkeys[tcount] == newkey)	{	/* if duplicate */
						senderr(ERR_DUPKEY,WARN);
						SetFocus(GetDlgItem(hwnd,IDC_HOTKEY1+count));
						return;
					}
				}
				pp->hotkeys[count] = newkey;
				length = mod_gettestring(GetDlgItem(hwnd,IDC_HOTKEYTEXT1+count),tstring,NULL,MREC_NOTRIM|MREC_SINGLE|MREC_NOITD);
				if (sptr-dstring+length+1 >= STSTRING){	// if string too long
					senderr(ERR_NOROOM,WARN);
					return;
				}
				strcpy(sptr,tstring);
			}
			*sptr = EOCS;
			str_xcpy(pp->keystrings,dstring);
		case IDCANCEL:
			hk_register(pp);
			hkclose(hwnd);
			return;
	}
}
/******************************************************************************/
static BOOL hkinit(HWND hwnd, HWND hwndFocus, LPARAM lParam)	/* initializes dialog */

{
	struct prefs *pp;
	int count, scount;
	CSTR fstrings[MAXKEYS];
	HWND ew, hk;
	CHARFORMAT2 cf;
	int err;

	pp = setdata(hwnd,(void *)lParam);	/* set data */
	scount = str_xparse(pp->keystrings, fstrings);
	memset(&cf,0,sizeof(CHARFORMAT2));
	cf.cbSize = sizeof(CHARFORMAT2);
	cf.dwMask = CFM_FACE;
	cf.bPitchAndFamily = DEFAULT_PITCH;
	nstrcpy(cf.szFaceName,toNative(pp->gen.fm[0].name));	/* set face */

	hk_unregister();
	for (count = 0; count < MAXKEYS; count++)	{	/* for all keys */
		hk = GetDlgItem(hwnd,IDC_HOTKEY1+count);
		err = SendMessage(GetDlgItem(hwnd,IDC_HOTKEYTEXT1+count),EM_SETCHARFORMAT,0,(LPARAM)&cf);	/* set character formatting (face) */
		SendMessage(hk,HKM_SETRULES,HKCOMB_NONE,MAKELPARAM(HOTKEYF_ALT|HOTKEYF_CONTROL,0));
		SendMessage(hk,HKM_SETHOTKEY,pp->hotkeys[count],0);
		ew = GetDlgItem(hwnd,IDC_HOTKEYTEXT1+count);
		Edit_LimitText(ew,100);
		loadtext(ew,fstrings[count].str);
	}
	centerwindow(hwnd,0);	/* center */
	return(FALSE);
}
/************************************************************************/
static void hkactivate(HWND hwnd, UINT state, HWND hwndActDeact, BOOL fMinimized)	/* activates/deactivates */

{
	if (state != WA_INACTIVE)	{	/* being activated */
		g_mdlg = hwnd;
	}
	else 	/* being deactivated */
		g_mdlg = NULL;
}
/******************************************************************************/
static short hkclose(HWND hwnd)		/* closes hotkey window */

{
	g_keyw = NULL;
	DestroyWindow(hwnd);	/* destroy it */
	return (0);
}
/*********************************************************************************/
static void loadtext(HWND ew, char * rtext)	/*  loads cindex styled text */

{
	CHARFORMAT2 cf;
	short codes;
	long baseheight, smallc, step;
	TCHAR tstring[MAXREC], *tptr;
	char *rptr;

	memset(&cf,0,sizeof(CHARFORMAT2));
	cf.cbSize = sizeof(CHARFORMAT2);
	SendMessage(ew,EM_GETCHARFORMAT,0,(LPARAM)&cf);
	cf.dwMask = CFM_BOLD|CFM_SIZE|CFM_ITALIC|CFM_SUPERSCRIPT|CFM_SUBSCRIPT|CFM_UNDERLINE|CFM_SMALLCAPS|CFM_CHARSET|CFM_FACE;
	cf.dwEffects = 0;
	baseheight = cf.yHeight;	/* use for scaling */
	step = baseheight/5;
	smallc = 4*step;
	SendMessage(ew,EM_SETCHARFORMAT,SCF_ALL,(LPARAM)&cf);	/* set default format */
	for (rptr = rtext, tptr = tstring; *rptr; rptr)	{	/* prepare segments */
		unichar uc = u8_nextU(&rptr);		
		if (iscodechar(uc))	{
			*tptr = '\0';
			SendMessage(ew,EM_REPLACESEL,TRUE,(LPARAM)tstring);
			codes = *rptr++;	/* get code */
			if (uc == FONTCHR)	{	/* if a font spec */
				int index;
//				if ((codes&FX_FONTMASK) >= FONTLIMIT)
//					senderr(ERR_INTERNALERR, WARN, "local font id too large");
				nstrcpy(cf.szFaceName,toNative(g_prefs.gen.fm[codes&FX_FONTMASK].name));
				if ((index = type_findindex(g_prefs.gen.fm[codes&FX_FONTMASK].name)) >= 0)	/* if have font */
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
//						cf.yHeight = cf.yOffset ? smallc : baseheight;
						cf.yHeight = baseheight;
						cf.dwEffects &= ~CFE_SMALLCAPS;
					}
#if 0
					if (codes&FX_SUPER || codes&FX_SUB)		{
						cf.yOffset = 0;
						cf.yHeight = cf.dwEffects&CFE_SMALLCAPS ?  smallc : baseheight;
					}
#else
					if (codes&FX_SUPER)
						cf.dwEffects &= ~CFE_SUPERSCRIPT;
					if (codes&FX_SUB)
						cf.dwEffects &= ~CFE_SUBSCRIPT;
#endif
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
#if 0
					if (codes&FX_SUPER)	{
						cf.yHeight = smallc;
						cf.yOffset = step;
					}
					if (codes&FX_SUB)	{	
						cf.yHeight = smallc;
						cf.yOffset = -step;
					}
#else
					if (codes&FX_SUPER)	{
						cf.dwEffects |= CFE_SUPERSCRIPT;
					}
					if (codes&FX_SUB)	{	
						cf.dwEffects |= CFE_SUBSCRIPT;
					}
#endif
				}
			}
			SendMessage(ew,EM_SETCHARFORMAT,SCF_SELECTION,(LPARAM)&cf);	/* set changed style */
			tptr = tstring;	/* reset buffer */
			continue;
		}
		else
			*tptr++ = uc;
	}
	*tptr = '\0';
	SendMessage(ew,EM_REPLACESEL,FALSE,(LPARAM)tstring);	/* add residual text */
}

