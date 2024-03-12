#include "stdafx.h"
#include "errors.h"
#include "strings.h"
#include "commands.h"
#include "group.h"
#include "viewset.h"
#include "modify.h"
#include "formstuff.h"
#include "searchset.h"
#include "sort.h"
#include "util.h"

static const DWORD wh_gotoid[] = {
	IDC_GOTO_TEXT, HIDC_GOTO_TEXT,
	IDC_GOTO_RECORD, HIDC_GOTO_RECORD,
	IDC_GOTO_PAGE,HIDC_GOTO_PAGE,
	0,0
};
static const DWORD wh_filterid[] = {
	IDC_FILTER_ON,HIDC_FILTER_ON,
	IDC_FILTER_LABEL0,HIDC_FILTER_LABEL0,
	IDC_FILTER_LABEL1, HIDC_FILTER_LABEL1,
	IDC_FILTER_LABEL2, HIDC_FILTER_LABEL1,
	IDC_FILTER_LABEL3, HIDC_FILTER_LABEL1,
	IDC_FILTER_LABEL4, HIDC_FILTER_LABEL1,
	IDC_FILTER_LABEL5, HIDC_FILTER_LABEL1,
	IDC_FILTER_LABEL6, HIDC_FILTER_LABEL1,
	IDC_FILTER_LABEL7, HIDC_FILTER_LABEL1,
	0,0
};

#define GBUFFLEN 480

static char s_goto[GBUFFLEN] = {EOCS};

static INT_PTR CALLBACK gotoproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);	/* handles goto box */
static INT_PTR CALLBACK filterproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);	// handles filter settings
/*******************************************************************************/
void sset_goto(HWND hwnd)	/* gets spec and goes to record */

{
	DialogBoxParam(g_hinst,MAKEINTRESOURCE(IDD_GOTO),g_hwframe,gotoproc,(LPARAM)WX(hwnd,owner));
}
/******************************************************************************/
static INT_PTR CALLBACK gotoproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)

{
	INDEX * FF = getdata(hwnd);
	char *sptr;
	RECN rnum;
	RECORD *recptr;
	long pnum;
	CSTR sarray[CBSTRING_MAX];
	HWND cbg;
	int count, index;

	cbg = GetDlgItem(hwnd,IDC_GOTO_TEXT);
	switch (msg)	{
		case WM_INITDIALOG:
			setdata(hwnd,(void *)lParam);	/* set data */
			CheckRadioButton(hwnd,IDC_GOTO_RECORD,IDC_GOTO_PAGE, IDC_GOTO_RECORD);
			centerwindow(hwnd,1);
			count = str_xparse(s_goto,sarray);
			type_settextboxfont(cbg);
			for (index = 0; index < count; index++)
				ComboBox_AddString(cbg,toNative(sarray[index].str));
			ComboBox_SetCurSel(cbg,0);
			SetFocus(cbg);	/* set focus to text */
			return FALSE;
		case WM_COMMAND:
			switch (LOWORD(wParam))	{
				case IDOK:
					fixcombomenu(hwnd,IDC_GOTO_TEXT);	/* build/sort all the items */
					count = ComboBox_GetCount(cbg);
					for (index = 0,sptr = s_goto; index < count; index++, sptr += strlen(sptr++))	{
						TCHAR tstring[1000];
						char *tp;

						ComboBox_GetLBText(cbg,index,tstring);
						tp = fromNative(tstring);
						if (sptr + strlen(tp) - s_goto < GBUFFLEN-1)	/* if room */
							strcpy(sptr,tp);
					}
					*sptr = EOCS;
					if (isitemchecked(hwnd,IDC_GOTO_RECORD))	{
						if (!(rnum = com_findrecord(FF,s_goto,FALSE)))	{	/* if no record */
							selectitext(hwnd,IDC_GOTO_TEXT);		/* send error */
							return TRUE;
						}
					}
					else if (getlong(hwnd,IDC_GOTO_TEXT,&pnum) && pnum >= 1)	{	/* if have number */
						memset(&FF->pf,0,sizeof(PRINTFORMAT));		/* clear format info struct */
						if (recptr = sort_top(FF))	{
							FF->pf.pagenum = FF->head.formpars.pf.firstpage;
							FF->pf.first = pnum;
							FF->pf.last = pnum;
							FF->pf.firstrec = recptr->num;
							FF->pf.lastrec = ULONG_MAX;
							view_formsilentimages(FF->vwind);
							if (FF->pf.lastpage != pnum)	{	/* if didn't find */
								setint(hwnd,IDC_GOTO_TEXT,FF->pf.lastpage);
								senditemerr(hwnd,IDC_GOTO_TEXT);
								return (TRUE);
							}
							rnum = FF->pf.rnum;
						}
						else
							return (TRUE);
					}
					if (!FF->rwind || mod_canenterrecord(FF->rwind,MREC_ALWAYSACCEPT))	// if no record window, or entry OK
						view_selectrec(FF,rnum,VD_SELPOS,-1,-1);
				case IDCANCEL:
					EndDialog(hwnd,LOWORD(wParam) == IDOK ? TRUE : FALSE);
					return TRUE;
			}
			break;
		case WM_HELP:
			return (dodialoghelp(hwnd,TEXT("_Views\\Navigating.htm"),(HELPINFO *)lParam,wh_gotoid));
	}
	return FALSE;
}
/*******************************************************************************/
void sset_filter(HWND hwnd)		// handles filter settings

{
	DialogBoxParam(g_hinst,MAKEINTRESOURCE(IDD_FILTERRECORDS),g_hwframe,filterproc,(LPARAM)WX(hwnd,owner));
}
/******************************************************************************/
static INT_PTR CALLBACK filterproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)

{
	INDEX * FF = getdata(hwnd);
	int index;

	switch (msg)	{
		case WM_INITDIALOG:
			setdata(hwnd,(void *)lParam);	/* set data */
			FF = getdata(hwnd);
			checkitem(hwnd,IDC_FILTER_ON,FF->head.privpars.filter.on);
			for (index = 0; index < FLAGLIMIT; index++)		// for all labels
				checkitem(hwnd,IDC_FILTER_LABEL0+index,FF->head.privpars.filter.label[index]);
			centerwindow(hwnd,1);
			return FALSE;
		case WM_COMMAND:
			switch (LOWORD(wParam))	{
				case IDOK:
					FF->head.privpars.filter.on = isitemchecked(hwnd,IDC_FILTER_ON);
					for (index = 0; index < FLAGLIMIT; index++)	// for all labels
						FF->head.privpars.filter.label[index] = isitemchecked(hwnd,IDC_FILTER_LABEL0+index);
					view_redisplay(FF,0,VD_TOP|VD_RESET);
				case IDCANCEL:
					EndDialog(hwnd,LOWORD(wParam) == IDOK ? TRUE : FALSE);
					return TRUE;
			}
			break;
		case WM_HELP:
			return (dodialoghelp(hwnd,TEXT("_Views\\Viewsubsets.htm"),(HELPINFO *)lParam,wh_filterid));
	}
	return FALSE;
}
/*******************************************************************************/
void sset_makegroup(HWND wptr)	/* forms group from selection */

{
	INDEX * FF;
	GROUPHANDLE gh;
	RECN endsel;
	
	FF = getowner(wptr);
	gh = grp_startgroup(FF);
	endsel = view_getsellimit(wptr);
	if (grp_buildfromrange(FF,&gh,LX(wptr,sel).first,endsel,GF_SELECT))	{
		grp_installtemp(FF,gh);
		SendMessage(FF->vwind,WM_COMMAND,IDM_VIEW_TEMPORARYGROUP,0);	/* display group */
		view_selectrec(FF,gh->recbase[0],VD_SELPOS,-1,-1);
	}
	else
		grp_dispose(gh);
}
