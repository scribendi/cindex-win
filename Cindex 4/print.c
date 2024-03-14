#include "stdafx.h"
#include <string.h>
#include "strings.h"
#include "commands.h"
#include "errors.h"
#include "sort.h"
#include "print.h"
#include "viewset.h"
#include "util.h"
#include "edit.h"
#include "formstuff.h"
#include "apiservice.h"

static const DWORD wh_printid[] = {
	IDC_PRINT_ODDEVEN, HIDC_PRINT_ODDEVEN,
	IDC_PRINT_RECFROM, HIDC_PRINT_RECFROM,
	IDC_PRINT_RECTO, HIDC_PRINT_RECFROM,
	1059, HIDC_PRINT_RECFROM,
#ifdef PUBLISH
	IDC_PRINT_BULLET, HIDC_PRINT_BULLET,
#endif //PUBLISH
	0,0
};

struct ppsize {
	WORD code;
	short wid;
	short len;
};

static struct ppsize p_psize[] = {	/* paper codes and real sizes in pixels */
	{DMPAPER_LETTER,612,792},
	{DMPAPER_LETTERSMALL,612,792},
	{DMPAPER_NOTE,612,792},
	{DMPAPER_TABLOID,792,1224},
	{DMPAPER_LEDGER,1224,792},
	{DMPAPER_LEGAL,612,1008},
	{DMPAPER_STATEMENT,396,612},
	{DMPAPER_EXECUTIVE,522,756},
	{DMPAPER_FANFOLD_US,1071,792},
	{DMPAPER_A3,842,1190},
	{DMPAPER_A4,595,842},
	{DMPAPER_A4SMALL,595,842},
	{DMPAPER_A5,419,595},
	{DMPAPER_10X14,720,1008},
	{DMPAPER_11X17,792,1224},
	{DMPAPER_9X11,648,792}
};
#define PAPERLIM (sizeof(p_psize)/sizeof(struct ppsize))

short p_firstpage, p_lastpage;
RECN p_firstrec, p_lastrec;
PRINTDLG p_dlg;
BOOL p_abort;	/* print abort flag */
HDC p_dc;		// printing dc
//HWND p_hwnd;	/* cancel print dialog handle */
int p_err;		/* print error flag */

static TCHAR p_path[MAX_PATH];

enum {		/* additional items in print dialog */
	PD_ALLREC = 1,
	PD_SELREC,
	PD_RANGEREC,
	PD_RANGESTART,
	PD_RANGEEND
};

#define thoutopoints(A) ((short)((A*72)/1000))
#define pointstothou(A)	((short)((A*1000)/72))
#define hmmtopoints(A) ((short)((A*720)/25400))
#define pointstohmm(A)	((short)((A*25400)/(720)))

static INT_PTR CALLBACK pshook(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);	/* page setup hook */
static INT_PTR CALLBACK prhook(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);	/* print dlg hook */
//static INT_PTR CALLBACK pabort(HDC dc, int code);	/* called during printing loop */
//static INT_PTR CALLBACK printproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);	/* print cancel dialog proc */
static void setpaperinfo(HGLOBAL hdm, PAPERINFO * pip);		/* sets paper info in device context */
static void getpaperinfo(HGLOBAL hdm, PAPERINFO * pip);		/* gets paper info from device context */
static short buildhfstring(INDEX * FF, char * dest, char *source, short page, short captype, short limit);		/* forms title string */
static short getdatetime(char *dstring, char dateform, char timeflag);		/* forms long or short date string */
/******************************************************************************/
HDC print_getic(void)	/* gets ic for current printer	*/

{
	PRINTDLG pd;
	HDC pdc;

	memset(&pd,0,sizeof(pd));
	pdc = NULL;
	pd.lStructSize = sizeof(pd);
	pd.Flags = PD_RETURNDC|PD_RETURNDEFAULT;
	if (PrintDlg(&pd))	{
		setdc(pd.hDC,FALSE,0);
		pdc = pd.hDC;
	}
		/* else: fail silently, and page sizes default to preferences */
	if (pd.hDevMode)
		GlobalFree(pd.hDevMode);
	if (pd.hDevNames)
		GlobalFree(pd.hDevNames);
	return pdc;
}
/******************************************************************************/
BOOL print_setpage(MARGINCOLUMN *mcp, PAPERINFO * pip)	/* sets & gets page info */

{
	BOOL ok;
	PAGESETUPDLG psd;
	TCHAR tstring[STSTRING];

	memset(&psd,0,sizeof(psd));
	psd.lStructSize = sizeof(psd);
	psd.Flags = PSD_RETURNDEFAULT;
	ok = 0;
	GetLocaleInfo(LOCALE_USER_DEFAULT,LOCALE_IMEASURE,tstring,STSTRING);
	if (PageSetupDlg(&psd))		{	/* if OK on defaults */
		setpaperinfo(psd.hDevMode,pip);
		psd.hwndOwner = g_hwframe;
		psd.Flags = PSD_DEFAULTMINMARGINS|PSD_SHOWHELP|PSD_ENABLEPAGESETUPHOOK;
		psd.hInstance = g_hinst;
		psd.lpfnPageSetupHook = pshook;	/* hook fn */
		if (mcp)	{	/* if want to set margin params */
			psd.Flags |= PSD_MARGINS;
			if (*tstring == '0')	{/* if si measure */
				psd.rtMargin.left = pointstohmm(mcp->left);
				psd.rtMargin.top = pointstohmm(mcp->top);
				psd.rtMargin.bottom = pointstohmm(mcp->bottom);
				psd.rtMargin.right = pointstohmm(mcp->right);
			}
			else	{
				psd.rtMargin.left = pointstothou(mcp->left);
				psd.rtMargin.top = pointstothou(mcp->top);
				psd.rtMargin.bottom = pointstothou(mcp->bottom);
				psd.rtMargin.right = pointstothou(mcp->right);
			}
		}
		if (ok = PageSetupDlg(&psd))	{	/* if OK */
			getpaperinfo(psd.hDevMode,pip);	/* get new paper info */
			if (mcp)	{
				if (*tstring == '0')	{/* if si measure */
					mcp->left = hmmtopoints(psd.rtMargin.left);
					mcp->top = hmmtopoints(psd.rtMargin.top);
					mcp->bottom = hmmtopoints(psd.rtMargin.bottom);
					mcp->right = hmmtopoints(psd.rtMargin.right);
				}
				else {
					mcp->left = thoutopoints(psd.rtMargin.left);
					mcp->top = thoutopoints(psd.rtMargin.top);
					mcp->bottom = thoutopoints(psd.rtMargin.bottom);
					mcp->right = thoutopoints(psd.rtMargin.right);
				}
			}
		}
	}
	if (psd.hDevMode)
		GlobalFree(psd.hDevMode);
	if (psd.hDevNames)
		GlobalFree(psd.hDevNames);
	return (ok);
}
/******************************************************************************/
static INT_PTR CALLBACK pshook(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)	/* page setup hook */

{
	switch (msg)	{
		case WM_INITDIALOG:
			centerwindow(hwnd,1);
			return TRUE;
	}
	return FALSE;
}
/******************************************************************************/
int print_begin(DWORD flags, TCHAR * title, PAPERINFO * pip, HWND hwnd)	/* sets up printing for window */

{
	DOCINFO di;
	BOOL ok;
	DEVNAMES * dvp;
	
	memset(&p_dlg,0,sizeof(p_dlg));
	p_dlg.lStructSize = sizeof(p_dlg);
	p_dlg.Flags = PD_RETURNDEFAULT;
	if (api_active)	{	// will call only once if api active
		INDEX * FF = getowner(hwnd);
		RECORD * recptr = sort_top(FF);
		p_dlg.Flags |= flags|PD_ALLPAGES|PD_RETURNDC|PD_USEDEVMODECOPIESANDCOLLATE|PD_SHOWHELP|PD_ENABLEPRINTHOOK;
		FF->pf.firstrec = recptr ? recptr->num : 0;
		FF->pf.lastrec = ULONG_MAX;
		FF->pf.labelmark = TRUE;
	}
	if (ok = PrintDlg(&p_dlg))		{	/* if OK on defaults */
		setpaperinfo(p_dlg.hDevMode,pip);	/* adjust any paper settings */
		p_dlg.Flags = flags|PD_ALLPAGES|PD_RETURNDC|PD_USEDEVMODECOPIESANDCOLLATE|PD_SHOWHELP|PD_ENABLEPRINTHOOK;
		p_dlg.hwndOwner = g_hwframe;
		if (flags&PD_ENABLEPRINTTEMPLATE)	{	/* if printing from view window with special template */
			p_dlg.hInstance = g_hinst;
			p_dlg.lpPrintTemplateName = MAKEINTRESOURCE(IDD_PRINT);
			p_dlg.lCustData = (LPARAM)WX(hwnd,owner);
		}
		p_dlg.nFromPage = 1;
		p_dlg.nToPage = 1;
		p_dlg.nMinPage = 1;
		p_dlg.nMaxPage = SHRT_MAX;
		p_dlg.lpfnPrintHook = prhook;
		if (api_active || (ok = PrintDlg(&p_dlg)))	{	/* if not cancelled */
			UpdateWindow(hwnd);		/* force update so that no screen action can happen while we're printing */		
			EnableWindow(hwnd,FALSE);		/* disable any mouse/key while printing */
			getpaperinfo(p_dlg.hDevMode,pip);	/* get new paper info */
//			SetAbortProc(p_dlg.hDC,pabort);	/* set abort proc */
//			CreateDialog(g_hinst,MAKEINTRESOURCE(IDD_PRINTPROGRESS),g_hwframe,printproc);
			memset(&di,0,sizeof(di));
			di.cbSize = sizeof(DOCINFO);
			di.lpszDocName = title;
			if (api_printfile)
				di.lpszOutput = api_printfile;
			else if (p_dlg.Flags&PD_PRINTTOFILE)	{
				if (dvp = GlobalLock(p_dlg.hDevNames))	{
					nstrcpy(p_path,(TCHAR *)dvp+dvp->wOutputOffset);	/* set path name in static because locals disappear */
					di.lpszOutput = p_path;
					GlobalUnlock(p_dlg.hDevNames);
				}
			}
			pip->xoffset = (pip->pwidthactual-(720*GetDeviceCaps(p_dlg.hDC,pip->porien == DMORIENT_PORTRAIT ? HORZSIZE : VERTSIZE))/254)>>1;
			pip->yoffset = (pip->pheightactual-(720*GetDeviceCaps(p_dlg.hDC,pip->porien == DMORIENT_PORTRAIT ? VERTSIZE : HORZSIZE))/254)>>1;
		}
	}
	else
		senderr(ERR_PRINTFAIL,WARN);
	if (p_dlg.hDevMode)
		GlobalFree(p_dlg.hDevMode);
	if (p_dlg.hDevNames)
		GlobalFree(p_dlg.hDevNames);

	if (ok)	{		/* if ok this far */
		if ((p_err = StartDoc(p_dlg.hDC,&di)) > 0)	/* if can start doc */
			ok = TRUE;
		else	{
			print_end(0,hwnd);		/* cleanup */
			ok = FALSE;
		}
	}
	return (ok);
}
/******************************************************************************/
static INT_PTR CALLBACK prhook(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)	/* print dlg hook */

{
	char string1[STSTRING], string2[STSTRING];
	PRINTDLG * pdp;
	INDEX * FF;
	int type, err;
	HWND cbh;

	pdp = getdata(hwnd);
	switch (msg)	{
		case WM_INITDIALOG:
			pdp = setdata(hwnd,(void *)lParam);	/* set data */
			centerwindow(hwnd,0);	/* center on frame window */
			SetWindowText(GetDlgItem(hwnd,rad1),TEXT("&All"));	/* Win 95 box adds page range to this */
			cbh = GetDlgItem(hwnd,IDC_PRINT_ODDEVEN);
			ComboBox_AddString(cbh,TEXT("All Pages in Range"));
			ComboBox_AddString(cbh,TEXT("Odd Pages"));
			ComboBox_AddString(cbh,TEXT("Even Pages"));
			ComboBox_SetCurSel(cbh,0);		/* set all pages */
			SetFocus(GetDlgItem(hwnd,edt1));
			return FALSE;
		case WM_COMMAND:
			switch (LOWORD(wParam))	{
				case IDOK:
					FF = (INDEX *)pdp->lCustData;
					if (FF)	{
						if (isitemchecked(hwnd,rad4))	/* if want record range */
							type = COMR_RANGE;
						else if (isitemchecked(hwnd,rad2))
							type = COMR_SELECT;
						else
							type = COMR_ALL;		/* all records (not necess all pages) */
						getDItemText(hwnd,IDC_PRINT_RECFROM,string1,STSTRING);
						getDItemText(hwnd,IDC_PRINT_RECTO,string2,STSTRING);
						FF->pf.oddeven = ComboBox_GetCurSel(GetDlgItem(hwnd,IDC_PRINT_ODDEVEN));	/* get odd/even page control */
						if (err = com_getrecrange(FF,type, string1,string2 ,&FF->pf.firstrec, &FF->pf.lastrec))	{
							selectitext(hwnd,err < 0 ? IDC_PRINT_RECFROM : IDC_PRINT_RECTO);
							return (TRUE);
						}
#ifdef PUBLISH
						FF->pf.labelmark = (short)isitemchecked(hwnd,IDC_PRINT_BULLET);
#endif //PUBLISH
					}
					break;
				case edt1:
				case edt2:
					if (HIWORD(wParam) == EN_CHANGE)		/* check pages button */
						CheckRadioButton(hwnd,rad1,rad4, rad3);
					return (TRUE);
				case rad4:
					selectitext(hwnd,IDC_PRINT_RECFROM);
					return TRUE;;
				case IDC_PRINT_RECFROM:
				case IDC_PRINT_RECTO:
					if (HIWORD(wParam) == EN_CHANGE)		/* check range button */
						CheckRadioButton(hwnd,rad1,rad4, rad4);
					return (TRUE);
			}
			break;
		case WM_HELP:
			return (dodialoghelp(hwnd,NULL,(HELPINFO *)lParam,wh_printid));
		default:
			;
	}
	return FALSE;
}
/******************************************************************************/
void print_end(int pages, HWND hwnd)	/* cleans up after printing	*/

{
	if (!p_abort && p_err > 0)	{	/* if not aborted and not error */
		if (!pages)					/* if nothing to print */
			AbortDoc(p_dlg.hDC);	/* abort */
		else
			EndDoc(p_dlg.hDC);	/* close doc */
	}
	EnableWindow(hwnd,TRUE);	/* re-enable window */
//	DestroyWindow(p_hwnd);
	DeleteDC(p_dlg.hDC);
	memset(&p_dlg,0,sizeof(p_dlg));		/* to make sure no print DC is available */
}
#if 0
/******************************************************************************/
static INT_PTR CALLBACK pabort(HDC dc, int code)	/* called during printing loop */

{
	MSG msg;

	while (!p_abort && PeekMessage(&msg,NULL,0,0,PM_REMOVE))	{	/* get messages  */
		if (!p_hwnd || !IsDialogMessage(p_hwnd,&msg))	{		/* if not for us */
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	return (!p_abort);
}

/******************************************************************************/
static INT_PTR CALLBACK printproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)	/* print cancel dialog proc */

{
	switch (msg)	{

		case WM_INITDIALOG:
			p_abort = FALSE;	/* clear abort */
			p_hwnd = hwnd;		/* install our handle */
			centerwindow(hwnd,0);
			return TRUE;
		case WM_DESTROY:
			p_hwnd = NULL;
			return TRUE;
		case WM_COMMAND:
			p_abort = TRUE;
			return TRUE;
		case WMM_PAGENUM:
			setint(hwnd,IDC_PRINTPROGRESS_PAGE,lParam);
			return TRUE;
	}
	return FALSE;
}
#endif
/******************************************************************************/
static void setpaperinfo(HGLOBAL hdm, PAPERINFO * pip)		/* sets paper info in device context */

{
	DEVMODE * dmp;

	if (hdm)	{
		if (dmp = GlobalLock(hdm))	{
			dmp->dmPaperSize = pip->psize;
			dmp->dmOrientation = pip->porien;
			dmp->dmFields |= DM_PAPERSIZE|DM_ORIENTATION;
			if (pip->pheight)	{	/* if length override */
				dmp->dmPaperLength = pip->pheight;
				dmp->dmFields |= DM_PAPERLENGTH;
			}
			if (pip->pheight)	{	/* if length override */
				dmp->dmPaperWidth = pip->pwidth;
				dmp->dmFields |= DM_PAPERWIDTH;
			}
			GlobalUnlock(hdm);
		}
	}
}
/******************************************************************************/
static void getpaperinfo(HGLOBAL hdm, PAPERINFO * pip)		/* gets paper info from device context */

{
	DEVMODE * dmp;
	int count;

	if (hdm)	{
		if (dmp = GlobalLock(hdm))	{
			pip->psize = dmp->dmPaperSize;
			pip->porien = dmp->dmOrientation;
			pip->pheight = dmp->dmFields & DM_PAPERLENGTH ? dmp->dmPaperLength : 0;
			pip->pwidth = dmp->dmFields & DM_PAPERWIDTH ? dmp->dmPaperWidth : 0;
			GlobalUnlock(hdm);
		}
		/* need to check for paper overrides */
		for (count = 0; count < PAPERLIM; count++)	{	/* for all sizes we know */
			if (p_psize[count].code == pip->psize)	{	/* if we recognize this one */
				pip->pheightactual = pip->porien == DMORIENT_PORTRAIT ? p_psize[count].len : p_psize[count].wid;
				pip->pwidthactual = pip->porien == DMORIENT_PORTRAIT ? p_psize[count].wid : p_psize[count].len;
			}
		}
	}
}
/******************************************************************************/
void pr_defaulthead(HDC dc, TCHAR * title, short pnum, MARGINCOLUMN *margins, RECT *prect)	/* prints default header */

{
	RECT trect = *prect;
	int writepos, ccount;
	char string[STSTRING];
	TEXTMETRIC tmetric;

	SaveDC(dc);
	setdc(dc,FALSE,0);		/* set mapping modes and text alignment */
	type_setfont(dc,fromNative(g_basefont),10,FX_BOLD);
	GetTextMetrics(dc,&tmetric);
	writepos = trect.top+margins->top >>1;	/* set head pos */
	if (writepos < 0)	/* if not in writable area */
		writepos = 0;
	writepos += tmetric.tmAscent+tmetric.tmExternalLeading;
	OffsetRect(&trect,margins->left,0);
	MoveToEx(dc,trect.left,writepos,NULL);
	ccount = getdatetime(string,DATE_SHORTDATE,TRUE);
	TextOut(dc,0,0,toNative(string),ccount);

	SetTextAlign(dc,TA_BASELINE|TA_NOUPDATECP|TA_CENTER);
	ccount = nstrlen(title);
	TextOut(dc,trect.left+(trect.right-trect.left)/2,writepos,title,ccount);

	SetTextAlign(dc,TA_BASELINE|TA_NOUPDATECP|TA_RIGHT);
	ccount = sprintf(string, "%d", pnum);
	TextOut(dc,trect.right,writepos,toNative(string),ccount);
	RestoreDC(dc,-1);
}
/******************************************************************************/
void pr_headfoot(INDEX * FF, HDC dc, short headflag, long page, RECT *textrect)	/* prints header/footer */

{
	char string[STSTRING];
	TEXTMETRIC tmetric;
	int writepos;
	short capflag;
	HEADERFOOTER *hfptr;
	long tsize, ccount;
	int rightpage;
	SIZE ssize;
	short fontsize;
//	int dcstate;
	
	SaveDC(dc);
	setdc(dc,FALSE,0);		/* set mapping modes and text alignment */
	rightpage = page&1 || !FF->head.formpars.pf.mc.reflect;	/* right page header if odd or no reflection */
	if (headflag)
		hfptr = rightpage ? &FF->head.formpars.pf.righthead : &FF->head.formpars.pf.lefthead;
	else
		hfptr = rightpage ? &FF->head.formpars.pf.rightfoot : &FF->head.formpars.pf.leftfoot;
	fontsize = hfptr->size ? hfptr->size : FF->head.privpars.size;
	tsize = fontsize;
	if (hfptr->hfstyle.style&FX_SMALL)	{
		tsize *= SMALLCAPSCALE;
		capflag = FC_UPPER;		/* force upper case conversion */
	}
	else
		capflag = hfptr->hfstyle.cap;
	type_setfont(dc,*hfptr->hffont ? hfptr->hffont : FF->head.fm[0].name,fontsize,hfptr->hfstyle.style&(~FX_SMALL));
	GetTextMetrics(dc,&tmetric);
	if (headflag)	{
		writepos = FF->head.formpars.pf.mc.top/2-FF->head.formpars.pf.pi.yoffset;	/* set head pos */
		if (writepos < 0)	/* if not in writable area */
			writepos = 0;
		writepos += tmetric.tmAscent+tmetric.tmExternalLeading;
	}
	else	/* footer */
		writepos = FF->head.formpars.pf.pi.pheightactual-FF->head.formpars.pf.mc.bottom/2-FF->head.formpars.pf.pi.yoffset;
	ccount = buildhfstring(FF,string,hfptr->left,page,capflag,sizeof(string));
	MoveToEx(dc,textrect->left,writepos,NULL);
	TextOut(dc,0,0,toNative(string),ccount);
	ccount = buildhfstring(FF,string,hfptr->center,page,capflag,sizeof(string));
	GetTextExtentPoint32(dc,toNative(string),ccount,&ssize);
	MoveToEx(dc,textrect->left+(textrect->right-textrect->left-ssize.cx)/2, writepos,NULL);	/* center it */
	TextOut(dc,0,0,toNative(string),ccount);
	ccount = buildhfstring(FF,string,hfptr->right,page,capflag,sizeof(string));
	GetTextExtentPoint32(dc,toNative(string),ccount,&ssize);
	MoveToEx(dc,textrect->right-ssize.cx,writepos,NULL);	/* right justify */
	TextOut(dc,0,0,toNative(string),ccount);
	RestoreDC(dc,-1);
}
/**********************************************************************/
static short buildhfstring(INDEX * FF, char * dest, char *source, short page, short captype, short limit)		/* forms title string */

{
	char * base, cc, *tpos;

	enum {
		PNUM_ARAB = 0,
		PNUM_ROMANLOWER,
		PNUM_ROMANUPPER
	};
	
	base = dest;
	while (*source && dest-base < limit)	{ 
		switch (cc = *source++)	{
			case ESCCHR:		/* escaped special char */
				if (*source)
					*dest++ = *source++;
				continue;
			case '#':		/* page # */
				if (!FF->head.formpars.pf.numformat)	/* arabic numerals */
					dest += sprintf(dest,"%d", page);
				else
					dest += toroman(dest, page, FF->head.formpars.pf.numformat == PNUM_ROMANUPPER);
				break;
			case '@':		/* date */
				dest += getdatetime(dest,FF->head.formpars.pf.dateformat,FF->head.formpars.pf.timeflag);
				break;
			case '%':		/* index file name */
				strcpy(dest,fromNative(FF->iname));
				dest += strlen(dest);
				break;
			default:
				*dest++ = cc;
		}
	}
	if (dest-base >= limit)
		dest = base+limit;
	*dest = '\0';
	if (captype == FC_INITIAL)	{
		unichar cc;

		tpos = str_skipcodes(base);
		while (*tpos == '\"' || *tpos == '\'')
			tpos++;
		cc = u8_toU(tpos);
		if (u_islower(cc))
			u8_appendU(tpos,u_toupper(cc));	// conversion relies on fact that uc and lc are same size
	}
	else if (captype == FC_UPPER)
		str_upr(base);
	return u8_countU(base,dest-base);
}
/**********************************************************************/
static short getdatetime(char *dstring, char dateform, char timeflag)		/* forms long or short date string */

{
	char * start = dstring;
	TCHAR ds[STSTRING],ts[STSTRING];
	char datetype;
	SYSTEMTIME st;
	
	GetLocalTime(&st);
	if (dateform == DF_LONG)
		datetype = DATE_LONGDATE;
	else
		datetype = DATE_SHORTDATE;
	GetDateFormat(LOCALE_SYSTEM_DEFAULT,datetype,&st,NULL,ds,STSTRING-1);
	strcpy(dstring, fromNative(ds));
	if (timeflag)	{	/* if also want time */
		char * start = dstring + strlen(dstring);
		*start++ = SPACE;
		*start++ = SPACE;
		GetTimeFormat(LOCALE_SYSTEM_DEFAULT,TIME_NOSECONDS,&st,NULL,ts,STSTRING-1);
		strcpy(start, fromNative(ts));
	}
	return (strlen(dstring));
}