#include "stdafx.h"
#include <string.h>
#include "commands.h"
#include "toolset.h"
#include "index.h"
#include "errors.h"
#include "search.h"
#include "modify.h"
#include "records.h"
#include "sort.h"
#include "tools.h"
#include "viewset.h"
#include "formstuff.h"
#include "text.h"
#include "util.h"
#include "regex.h"
#include "refs.h"
#include "files.h"
#include "print.h"
#include "import.h"
#include "strings.h"
#include "export.h"
#include "registry.h"

static const DWORD wh_recid[] = {
	IDC_RECONCILE_LEVEL, HIDC_RECONCILE_LEVEL,
	IDC_RECONCILE_SEPARATOR, HIDC_RECONCILE_SEPARATOR,
	IDC_RECONCILE_NOSPLIT, HIDC_RECONCILE_NOSPLIT,
	IDC_RECONCILE_CONVERT, HIDC_RECONCILE_CONVERT,
	IDC_RECONCILE_PROTECT, HIDC_RECONCILE_PROTECT,
	IDC_RECONCILE_REMOVE,HIDC_RECONCILE_REMOVE,
	0,0
};

static const DWORD wh_verid[] = {
	IDC_VERIFY_CROSSREF,HIDC_VERIFY_CROSSREF,
	IDC_VERIFY_EXACT, HIDC_VERIFY_EXACT,
	IDC_VERIFY_MIN, HIDC_VERIFY_MIN,
	IDC_VERIFY_LOCATOR,HIDC_VERIFY_LOCATOR,
	IDC_VERIFY_LOCATORLIMIT,HIDC_VERIFY_LOCATORLIMIT,
	0,0
};

static const DWORD wh_manageid[] = {
	IDC_MANAGECROSS_GENERATE, HIDC_MANAGECROSS_GENERATE,
	IDC_MANAGECROSS_CONVERT, HIDC_MANAGECROSS_CONVERT,
	IDC_MANAGECROSS_CCOUNT,HIDC_MANAGECROSS_CCOUNT,
	0,0
};

static const DWORD wh_agenid[] = {
	IDC_AUTOGEN_SEE, HIDC_AUTOGEN_SEE,
	0,0
};

static const DWORD wh_adjid[] = {
	IDC_ALTER_ADJONLY, HIDC_ALTER_ADJONLY,
	IDC_ALTER_ADJUSTMENT, HIDC_ALTER_ADJUSTMENT,
	IDC_ALTER_ALLREFS, HIDC_ALTER_ALLREFS,
	IDC_ALTER_HOLDHIGHER, HIDC_ALTER_HOLDHIGHER,
	IDC_ALTER_PATTERN, HIDC_ALTER_PATTERN,
	IDC_ALTER_RANGENED, HIDC_ALTER_RANGEREF,
	IDC_ALTER_RANGEREF, HIDC_ALTER_RANGEREF,
	IDC_ALTER_RANGESTART, HIDC_ALTER_RANGEREF,
	IDC_ALTER_REMOVE, HIDC_ALTER_REMOVE,
	0,0
};

static const DWORD wh_countid[] = {
	IDC_COUNT_PAGEEND, HIDC_COUNT_PAGEEND,
	IDC_COUNT_PAGESTART, HIDC_COUNT_PAGEEND,
	IDC_COUNT_RESULT, HIDC_COUNT_RESULT,
	IDC_COUNT_START, HIDC_COUNT_START,
	IDC_GENERIC_DONE, HIDC_GENERIC_DONE,

	IDC_FIND_ALL, HIDC_FIND_ALL,
	IDC_FIND_SELECTED, HIDC_FIND_SELECTED,
	IDC_FIND_RANGE, HIDC_FIND_RANGE,
	IDC_FIND_RANGESTART, HIDC_FIND_RANGE,
	IDC_FIND_RANGEEND, HIDC_FIND_RANGE,
	IDC_FIND_NEWREC, HIDC_FIND_NEWREC,
	IDC_FIND_MODREC, HIDC_FIND_MODREC,
	IDC_FIND_DELREC, HIDC_FIND_DELREC,
	IDC_FIND_MARKREC, HIDC_FIND_MARKREC,
	IDC_FIND_GENREC, HIDC_FIND_GENREC,
	IDC_FIND_TAGREC, HIDC_FIND_TAGREC,
	0,0
};

static const DWORD wh_statid[] = {
	IDC_STATISTICS_RESULT, HIDC_STATISTICS_RESULT,
	IDC_STATISTICS_GO, HIDC_STATISTICS_GO,
	IDC_GENERIC_DONE, HIDC_GENERIC_DONE,

	IDC_FORMWRITE_ALL, HIDC_FORMWRITE_ALL,
	IDC_FORMWRITE_PAGE, HIDC_FORMWRITE_PAGE,
	IDC_FORMWRITE_PSTART, HIDC_FORMWRITE_PAGE,
	IDC_FORMWRITE_PEND, HIDC_FORMWRITE_PAGE,
	IDC_FORMWRITE_SELECT, HIDC_FORMWRITE_SELECT,
	IDC_FORMWRITE_RECORDS, HIDC_FORMWRITE_RECORDS,
	IDC_FORMWRITE_RSTART, HIDC_FORMWRITE_RECORDS,
	IDC_FORMWRITE_REND, HIDC_FORMWRITE_RECORDS,
	0,0
};

static const DWORD wh_fontid[] = {
	IDC_FONTSUB_CHECK, HIDC_FONTSUB_CHECK,
	IDC_FONTSUB_LIST, HIDC_FONTSUB_LIST,
	IDC_FONTSUB_PREFERRED, HIDC_FONTSUB_PREFERRED,
	IDC_FONTSUB_SUBS, HIDC_FONTSUB_SUBS,
	0,0
};

static const DWORD wh_checkid[] = {
	IDC_CHECK_B_MISUSED, HIDC_CHECK_B_MISUSED,
	IDC_CHECK_B_PUNCTSPACE, HIDC_CHECK_B_PUNCTSPACE,
	IDC_CHECK_B_MISSINGSPACE, HIDC_CHECK_B_MISSINGSPACE,
	IDC_CHECK_B_UNBALANCEDPAREN, HIDC_CHECK_B_UNBALANCEDPAREN,
	IDC_CHECK_B_UNBALANCEDQUOTE, HIDC_CHECK_B_UNBALANCEDQUOTE,
	IDC_CHECK_B_MIXEDCASE, HIDC_CHECK_B_MIXEDCASE,

	IDC_CHECK_H_INCONSISTENTCAPS,HIDC_CHECK_H_INCONSISTENTCAPS,
	IDC_CHECK_H_INCONSISTENTSTYLE,HIDC_CHECK_H_INCONSISTENTSTYLE,
	IDC_CHECK_H_INCONSISTENTPUNCT,HIDC_CHECK_H_INCONSISTENTPUNCT,
	IDC_CHECK_H_INCONSISTENTLEADPREP,HIDC_CHECK_H_INCONSISTENTLEADPREP,
	IDC_CHECK_H_INCONSISTENTPLURALS,HIDC_CHECK_H_INCONSISTENTPLURALS,
	IDC_CHECK_H_INCONSISTENTPREP,HIDC_CHECK_H_INCONSISTENTPREP,
	IDC_CHECK_H_INCONSISTENTPAREN,HIDC_CHECK_H_INCONSISTENTPAREN,
	IDC_CHECK_H_ORPHANEDSUBHEAD,HIDC_CHECK_H_ORPHANEDSUBHEAD,

	IDC_CHECK_R_EMPTYPAGE,HIDC_CHECK_R_EMPTYPAGE,
	IDC_CHECK_R_VERIFY,HIDC_CHECK_R_VERIFY,
	IDC_CHECK_R_EXACTMATCH,HIDC_CHECK_R_EXACTMATCH,
	IDC_CHECK_R_MINMATCHES,HIDC_CHECK_R_MINMATCHES,
	IDC_CHECK_R_TOOMANYPAGE,HIDC_CHECK_R_TOOMANYPAGE,
	IDC_CHECK_R_PAGEREFLIMIT,HIDC_CHECK_R_PAGEREFLIMIT,
	IDC_CHECK_R_OVERLAPPINGPAGE,HIDC_CHECK_R_OVERLAPPINGPAGE,
	IDC_CHECK_R_HEADINGLEVEL,HIDC_CHECK_R_HEADINGLEVEL,
	0,0
};
static const DWORD wh_splitid[] = {
	IDC_SPLIT_PATTERN,HIDC_SPLIT_PATTERN,
	IDC_SPLIT_USERPATTERN,HIDC_SPLIT_USERPATTERN,
	IDC_SPLIT_MARKRECORDS,HIDC_SPLIT_MARKRECORDS,
	IDC_SPLIT_REMOVESTYLES,HIDC_SPLIT_REMOVESTYLES,
	IDC_SPLIT_PREVIEW,HIDC_SPLIT_PREVIEW,
	0,0
};

static TCHAR * c_checkhelpbasic = TEXT("base\\Check_basic.htm");
static TCHAR * c_checkhelpheadings = TEXT("base\\Check_headings.htm");
static TCHAR * c_checkhelprefs = TEXT("base\\Check_refs.htm");

typedef struct {
	INDEX *FF;
	void * params;
} CARRIER;

struct substruct {	
	INDEX *FF;
	FONTMAP *fm;
	short farray[FONTLIMIT];
};

static HEADERITEM _reconcileheader[] = {
	{TEXT("Record"),60,0},
	{TEXT("Heading... orphaned subheading"),1000,0},
	{NULL,0,0}
};
static HEADERITEM _splitheader[] = {
	{ TEXT("Record"),60,0 },
	{ TEXT("Split Headings Preview"),1000,0 },
	{ NULL,0,0 }
};

static HEADERITEM _checkheader[] = {
	{ TEXT("Record"),60,0 },
	{ TEXT("Index Check"),1000,0 },
	{ NULL,0,0 }
};
static HEADERITEM _verifyheader[] = {
	{TEXT("Record"),60,0},
	{TEXT("Error"),90,0},
	{TEXT("Type"),52,0},
	{TEXT("Reference"),1000,0},
	{NULL,0,0}
};
static INT_PTR CALLBACK recproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK autohook(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK verproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static HWND setwindow(INDEX * FF, char * titlestring, int errtot,HEADERITEM * hitems, TCHAR * chelp);	/* sets up verify window */
static LRESULT CALLBACK textdisplayproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static int vnotify(HWND hwnd, int id, NMHDR * hdr);	/* does notification tasks */
static void vdisplayrecord(HWND hwnd, int fline, int frontflag);	/* selects record identified on line line */
static INT_PTR CALLBACK adjproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK cbasicproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK cheadingproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK crefproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK splitproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK countproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK statsproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static void displaybasestats (HWND hwed, INDEX * FF);	// displays base info
static INT_PTR CALLBACK fsubproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static void setrowstring(HWND hwl,int row, TCHAR * pstring, TCHAR * sstring);	/* sets row string */
static BOOL getstringptrs(HWND hwl,int row, TCHAR * cstring, TCHAR ** pstring, TCHAR ** sstring);	/* gets string ptrs */
static INT_PTR CALLBACK manageproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

/*******************************************************************************/
void ts_reconcile(HWND hwnd)	/* reconciles headings */

{
	if (DialogBoxParam(g_hinst,MAKEINTRESOURCE(IDD_RECONCILE),hwnd,recproc,(LPARAM)WX(hwnd,owner)))
		view_redisplay(WX(hwnd,owner),0,VD_TOP);
}
/******************************************************************************/
static INT_PTR CALLBACK recproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)

{
	HWND ewnd;
	int count;
	INDEX * FF;
	JOINPARAMS tjn;
	char tstring[STSTRING];
//	int reconcilemode;

	FF = getdata(hwnd);
	switch (msg)	{

		case WM_INITDIALOG:
			if (FF = setdata(hwnd,(void *)lParam))	{	/* set data */
				centerwindow(hwnd,1);
				buildheadingmenu(GetDlgItem(hwnd,IDC_RECONCILE_LEVEL),&FF->head.indexpars,0);
				if (!sort_isinfieldorder(FF->head.sortpars.fieldorder,FF->head.indexpars.maxfields))	{	/* if isn't straight field order */
					disableitem(hwnd,IDC_RECONCILE_NOSPLIT);	/* disable split setting */
					checkitem(hwnd,IDC_RECONCILE_NOSPLIT,TRUE);	/* can't split */
				}
				checkitem(hwnd,IDC_RECONCILE_PROTECT,TRUE);	/* default protect names */
				CheckRadioButton(hwnd,IDC_RECONCILE_CONVERT,IDC_RECONCILE_REMOVE,IDC_RECONCILE_CONVERT);
				ewnd = GetDlgItem(hwnd,IDC_RECONCILE_SEPARATOR);
				Edit_LimitText(ewnd,1);		/* limit to 1 char */
				SetWindowText(ewnd,TEXT(","));
				SetFocus(ewnd);	/* set focus to text */
			}
			return FALSE;
		case WM_COMMAND:
			if (HIWORD(wParam) == EN_UPDATE)	{
				WORD id = LOWORD(wParam);
				int length = 0;

				if (id == IDC_RECONCILE_SEPARATOR)
					length = 2;
				checktextfield((HWND)lParam,length);
				return TRUE;
			}
			switch (LOWORD(wParam))	{
				case IDOK:
					memset(&tjn,0,sizeof(tjn));
					tjn.firstfield = ComboBox_GetCurSel(GetDlgItem(hwnd,IDC_RECONCILE_LEVEL));	/* get field index */
					if (!getDItemText(hwnd,IDC_RECONCILE_SEPARATOR,tstring,2))	{	/* if don't have character */
						senditemerr(hwnd,IDC_RECONCILE_SEPARATOR);
						break;
					}
					tjn.jchar = *tstring;	/* set separator */
					tjn.nosplit = (short)isitemchecked(hwnd,IDC_RECONCILE_NOSPLIT);
					tjn.protectnames = (short)isitemchecked(hwnd,IDC_RECONCILE_PROTECT);
					tjn.orphanaction = findgroupcheck(hwnd,IDC_RECONCILE_CONVERT,IDC_RECONCILE_REMOVE)-IDC_RECONCILE_CONVERT;
#if 0
					if (!reconcilemode)	{// if want only orphan display
						tjn.orphanaction = OR_PRESERVE;	// preserve orphans
						tjn.nosplit = TRUE;		// don't split
					}
					tjn.orphans = malloc(ORPHANARRAYBLOCK*sizeof(int));
#endif
					tjn.orphancount = 0;
					SetCursor(g_waitcurs);		/* set to watch */
					if (count = tool_join(FF, &tjn))
						senderr(ERR_RECMARKERR,WARN, count);   /* give message */
#if 0
					if (!reconcilemode) {
						if (tjn.orphancount)	{
							TCHAR string[MAXREC];

							if (setwindow(FF,"Orphaned Subheadings",0,_reconcileheader,TEXT("base\\Tools_reconciling.htm")))	{
//								int digits = numdigits(FF->head.rtot);
								int index;
//								int curpos, selstart,selend,line;

								for (index = 0; index < tjn.orphancount; index++) {
									RECORD * curptr = rec_getrec(FF,tjn.orphans[index]);	// get record
									if (curptr) {
										int fcount = str_xcount(curptr->rtext);

//										u_sprintf(string, "%*ld\t%s...", digits, curptr->num, curptr->rtext);
										u_sprintf(string,"\t%u\t%s...",curptr->num,curptr->rtext);
										fixleadspace(string);	// provides fixed leading space
										txt_append(FF->twind,string,NULL);
										txt_appendctext(FF->twind,toNative(str_xatindex(curptr->rtext,fcount-2)));
										txt_append(FF->twind,TEXT("\r"),NULL);
									}
								}
							}
						}
						else
							; // no orphaned subheadings
					}
					free(tjn.orphans);
#endif
				case IDCANCEL:
					EndDialog(hwnd,LOWORD(wParam) == IDOK ? TRUE : FALSE);
					return TRUE;
				default:
					enableitem(hwnd,IDC_RECONCILE_SEPARATOR);
					enableitem(hwnd,IDC_RECONCILE_NOSPLIT);
					enableitem(hwnd,IDC_RECONCILE_CONVERT);
					enableitem(hwnd,IDC_RECONCILE_REMOVE);
					if (isitemchecked(hwnd,IDC_RECONCILE_NOSPLIT))	{
						checkitem(hwnd,IDC_RECONCILE_PROTECT,TRUE);
						disableitem(hwnd,IDC_RECONCILE_PROTECT);
					}
					else
						enableitem(hwnd,IDC_RECONCILE_PROTECT);
			}
			break;
		case WM_HELP:
			return (dodialoghelp(hwnd,TEXT("base\\Tools_reconciling.htm"),(HELPINFO *)lParam,wh_recid));
	}
	return FALSE;
}
/*******************************************************************************/
void ts_managecrossrefs(HWND hwnd)	// manages cross-refs

{
	DialogBoxParam(g_hinst,MAKEINTRESOURCE(IDD_MANAGECROSSREFS),hwnd,manageproc,(LPARAM)WX(hwnd,owner));
}
/******************************************************************************/
static INT_PTR CALLBACK manageproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)

{
	INDEX * FF = getdata(hwnd);
	short ccount;
	RECN newrecs;

	switch (msg)	{

		case WM_INITDIALOG:
			if (FF = setdata(hwnd,(void *)lParam))	{	/* set data */
				CheckRadioButton(hwnd,IDC_MANAGECROSS_GENERATE,IDC_MANAGECROSS_CONVERT,IDC_MANAGECROSS_CONVERT);
				setint(hwnd,IDC_MANAGECROSS_CCOUNT,2);
				SetFocus(GetDlgItem(hwnd,IDC_MANAGECROSS_CCOUNT));	/* set focus to text */
				SendMessage(GetDlgItem(hwnd,IDC_MANAGECROSS_CCOUNT),EM_SETSEL,0,-1);
				centerwindow(hwnd,1);
			}
			return FALSE;
		case WM_COMMAND:
			switch (LOWORD(wParam))	{
				case IDOK:
					if (isitemchecked(hwnd,IDC_MANAGECROSS_GENERATE))
						ts_autogen(FF->vwind);
					else {
						if (!getshort(hwnd,IDC_MANAGECROSS_CCOUNT, &ccount) || ccount < 2)	{	/* if don't have good number */
							senditemerr(hwnd,IDC_MANAGECROSS_CCOUNT);
							break;
						}
						if (newrecs = search_convertcross(FF,ccount))	{	// if added records
							view_redisplay(FF,0,VD_TOP);
							sendinfo(INFO_RECCONVERT,newrecs);
						}
						else
							sendinfo(INFO_NORECCONVERT);
					}
					view_setstatus(FF->vwind);	// force update of status display (new recs)
				case IDCANCEL:
					EndDialog(hwnd,LOWORD(wParam) == IDOK ? TRUE : FALSE);
					return TRUE;
			}
			break;
		case WM_HELP:
			return (dodialoghelp(hwnd,TEXT("base\\Crossref_manage.htm"),(HELPINFO *)lParam,wh_manageid));
	}
	return FALSE;
}
/*******************************************************************************/
void ts_autogen(HWND hwnd)	/* generates auto cross-references */

{
	INDEX * XF;
	TCHAR path[_OFN_NAMELEN], title[_MAX_FNAME+_MAX_EXT], dir[MAX_PATH];
	RECN rcount;
	AUTOGENERATE ag;

	memset(&ag,0,sizeof(ag));
	ofn.hwndOwner = g_hwframe;
	file_getdefpath(dir,FOLDER_CDX,TEXT(""));
	*path = '\0';
	*title = '\0';
	ofn.lpstrFilter = newindexfilter;
	ofn.lpstrFile = path;		/* holds path on return */
	ofn.lpstrInitialDir = dir;
	ofn.lpstrFileTitle = title;	/* holds file title on return */
	ofn.lpstrTitle = TEXT("Generate Cross-References");
	ofn.Flags = OFN_OVERWRITEPROMPT|OFN_HIDEREADONLY|OFN_PATHMUSTEXIST
		|OFN_EXPLORER|OFN_ENABLETEMPLATE|OFN_ENABLEHOOK|OFN_ENABLESIZING;
	ofn.lpstrDefExt = file_extensionfortype(FTYPE_INDEX)+1;
	ofn.lpfnHook = autohook;
	ofn.hInstance = g_hinst;
	ofn.lCustData = (LONG_PTR)&ag.seeonly;	/* "See" only flag */
	ofn.lpTemplateName = MAKEINTRESOURCE(IDD_AUTOGEN);
	
	if (GetOpenFileName(&ofn))	{	/* if want to use */
		file_openindex(path,OP_READONLY);
		if (XF = index_byfspec(path))	{
			rcount = search_autogen(WX(hwnd,owner),XF, &ag);/* generate refs; if error */
			if (!XF->vwind)		/* if cref index wasn't already open */
				index_close(XF);
			if (rcount)	{		/* if generated any records */
				if (ag.skipcount)	
					sendinfo(INFO_RECGENSKIPINFO,rcount,ag.skipcount,ag.maxneed);
				else
					sendinfo(INFO_RECGENNUMINFO,rcount);
				view_allrecords(hwnd);		/* redisplay */
			}
			else
				sendinfo(INFO_NONGENNUMINFO);
		}
	}
}
/**********************************************************************************/	
static INT_PTR CALLBACK autohook(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)

{
	HWND parent;
	short * sfptr;

	sfptr = getdata(hwnd);
	switch (msg)	{

		case WM_INITDIALOG:
			setdata(hwnd, (void *)((OPENFILENAME*)lParam)->lCustData);
			parent = GetParent(hwnd);
			centerwindow(parent,0);
			ShowWindow(GetDlgItem(parent,stc2),SW_HIDE);	/* hide 'save as type' */
			ShowWindow(GetDlgItem(parent,cmb1),SW_HIDE);	/* hide 'type combo' */
			return (TRUE);
		case WM_COMMAND:
			switch (LOWORD(wParam))	{
				case IDC_AUTOGEN_SEE:
					*sfptr = (short)iscontrolchecked((HWND)lParam);	/* get see flag */
					return TRUE;
			}
			return FALSE;
		case WM_HELP:
			return (dodialoghelp(hwnd,NULL,(HELPINFO *)lParam,wh_agenid));
		default:
			;
	}
	return (FALSE);
}
/*******************************************************************************/
void ts_verify(HWND hwnd)	/* verifies cross-refs */

{
	if (DialogBoxParam(g_hinst,MAKEINTRESOURCE(IDD_VERIFY),hwnd,verproc,(LPARAM)WX(hwnd,owner)))
		view_redisplay(WX(hwnd,owner),0,VD_TOP);
}
/******************************************************************************/
static INT_PTR CALLBACK verproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)

{
	INDEX * FF = getdata(hwnd);
	int errtot = 0;

	RECN digits;
	RECORD * recptr;
	int count, crosscount, rcount;
	VERIFYGROUP tvg;
	char trec[MAXREC];
	TCHAR tbuff[MAXREC];

	static char * v_err[] = 	{	/* verification error tags */
		"",
		"Too Few",
		"Circular",
		"Missing",
		"Case/Accent"
	};
	BOOL docross,dopage;
	short pagelimit;
	unsigned char delstat;

	switch (msg)	{
		case WM_INITDIALOG:
			setdata(hwnd,(void *)lParam);	/* set data */
			centerwindow(hwnd,1);
			setint(hwnd,IDC_VERIFY_MIN,g_prefs.hidden.crossminmatch ? g_prefs.hidden.crossminmatch : 1);
			setint(hwnd,IDC_VERIFY_LOCATORLIMIT,g_prefs.hidden.pagemaxcount ? g_prefs.hidden.pagemaxcount : 7);
			checkitem(hwnd,IDC_VERIFY_CROSSREF,TRUE);
			checkitem(hwnd,IDC_VERIFY_LOCATOR,TRUE);
			SetFocus(GetDlgItem(hwnd,IDC_VERIFY_MIN));	/* set focus to text */
			return FALSE;
		case WM_COMMAND:
			switch (LOWORD(wParam))	{
				case IDOK:
					digits = numdigits(FF->head.rtot);
					docross = isitemchecked(hwnd,IDC_VERIFY_CROSSREF);
					dopage = isitemchecked(hwnd,IDC_VERIFY_LOCATOR);
					
					if (dopage && (!getshort(hwnd,IDC_VERIFY_LOCATORLIMIT, &pagelimit) || pagelimit < 1))
						break;
					memset(&tvg,0,sizeof(tvg));
					if (!getshort(hwnd,IDC_VERIFY_MIN, &tvg.lowlim) || tvg.lowlim < 1)	{	/* if don't have character */
						senditemerr(hwnd,IDC_VERIFY_MIN);
						break;
					}
					tvg.fullflag = (short)isitemchecked(hwnd,IDC_VERIFY_EXACT);
					tvg.t1 = trec;	/* set ptr to temp string */
					tvg.locatoronly = FF->head.refpars.clocatoronly;	// set as necess for locator field only
					delstat = FF->head.privpars.hidedelete;
					FF->head.privpars.hidedelete = TRUE;
//					sort_setfilter(FF,SF_HIDEDELETEONLY);
					if (docross) {
						for (rcount = 0, recptr = sort_top(FF); recptr && !iscancelled(NULL); recptr = sort_skip(FF,recptr,1)) {	   /* for all records */
							showprogress(PRG_VERIFYING,FF->head.rtot,rcount++);
							if (!recptr->isdel && (crosscount = search_verify(FF,recptr->rtext,&tvg)))	{	/* if have cross-ref */
								for (count = 0; count < crosscount; count++)	{
									if (tvg.cr[count].error || tvg.eflags&V_TYPEERR)	{
										if (setwindow(FF, "Reference Check",errtot,_verifyheader,TEXT("base\\Checkrefs.htm")))	{	/* if have window */
											char testring[MAXREC];
											u_sprintf(tbuff, "%*ld\t%s\t%s\t",digits,recptr->num,v_err[tvg.cr[count].error], tvg.eflags&V_TYPEERR ? "X/XA" : "");
											fixleadspace(tbuff);	// provides fixed leading space
											txt_append(FF->twind,tbuff,NULL);
//											u_sprintf(tbuff,"%.*s [from %s]\r", tvg.cr[count].length,recptr->rtext+tvg.cr[count].offset, recptr->rtext);
											sprintf(testring,"%.*s [from %s]\r", tvg.cr[count].length,recptr->rtext+tvg.cr[count].offset, recptr->rtext);
											txt_appendctext(FF->twind,toNative(testring));
											errtot++;
										}
										else
											;	/* some breakout on no window */
									}
								}
							}
						}
					}
					if (dopage) {
						char vmode = FF->head.privpars.vmode; 
						short runlevel = FF->head.formpars.ef.runlevel;
							
						FF->head.privpars.vmode = VM_FULL;	// force into full form, full indented
						FF->head.formpars.ef.runlevel = 0;
						for (recptr = sort_top(FF); recptr; recptr = form_skip(FF, recptr,1)) {	   /* for all records */
							ENTRYINFO es;
							
							FF->singlerefcount = 0;		// for net count of locators
							form_buildentry(FF, recptr, &es);
							if (FF->singlerefcount > pagelimit) {
								if (setwindow(FF, "Reference Check",errtot,_verifyheader,TEXT("base\\Crossref_verify.htm")))	{	/* if have window */
									u_sprintf(tbuff,"%*ld\tLocator\t[%d]\t",digits,recptr->num, FF->singlerefcount);
									fixleadspace(tbuff);	// provides fixed leading space
									txt_append(FF->twind,tbuff,NULL);
									u_sprintf(tbuff,"%S\r",toNative(recptr->rtext));
									txt_appendctext(FF->twind,tbuff);
									errtot++;
								}
								else
									;
							}
						}
						FF->head.privpars.vmode = vmode;
						FF->head.formpars.ef.runlevel = runlevel;
					}
					FF->head.privpars.hidedelete = delstat;
//					sort_setfilter(FF,SF_VIEWDEFAULT);
					showprogress(0,0,0);	  /* kill message */
					if (errtot)	{		/* set up to select first error */
						int curpos, selstart,selend,line;

						SendMessage(EX(FF->twind,hwed),EM_SETEVENTMASK,0,ENM_SELCHANGE|ENM_KEYEVENTS|ENM_MOUSEEVENTS);	/* set event mask */		
						curpos = Edit_LineIndex(EX(FF->twind, hwed),0);
						txt_findpara(EX(FF->twind, hwed),curpos,&selstart,&selend,&line,TRUE);
						vdisplayrecord(FF->twind,line,FALSE);
					}
					else if (docross || dopage)		// if actually did a check
						sendinfo(INFO_VERIFYOK);
					getshort(hwnd,IDC_VERIFY_MIN,&g_prefs.hidden.crossminmatch);
					getshort(hwnd,IDC_VERIFY_LOCATORLIMIT,&g_prefs.hidden.pagemaxcount);
				case IDCANCEL:
					EndDialog(hwnd,LOWORD(wParam) == IDOK ? TRUE : FALSE);
					return TRUE;
			}
			break;
		case WM_HELP:
			return (dodialoghelp(hwnd,TEXT("base\\Checkrefs.htm"),(HELPINFO *)lParam,wh_verid));
	}
	return FALSE;
}
/*******************************************************************************/
static HWND setwindow(INDEX * FF, char * titlestring, int errtot,HEADERITEM * hitems, TCHAR * chelp)	/* sets up verify window */

{
	TCHAR title[STSTRING];
	RECT trect;
	PARAFORMAT2 pf;			/* paragraph format */
	TEXTPARS tpars;

	if (!errtot)	{	/* if first error */
		int digitwidth = 8;
		int digits = numdigits(FF->head.rtot);
		if (digits < 4)
			digits = 4;
		int width = digits * digitwidth;	// size of lead for record number
		u_sprintf(title,"%S: %s",FF->iname, titlestring);
		SetRect(&trect,0,0,600,300);
		memset(&tpars,0,sizeof(tpars));
		tpars.hook = textdisplayproc;
		tpars.hwp = &FF->twind;
		tpars.style = ES_READONLY|ES_NOHIDESEL;
		tpars.owner = FF;
		tpars.hitems = hitems;
		hitems[0].width = width+digitwidth*2;
		tpars.helpcontext = chelp;
		txt_setwindow(title, &trect, &tpars);	// makes new even if already exists
//		HDC dc = GetDC(FF->twind);
//		GetCharWidth32(dc, '9', '9', &digitwidth);	// get width of figure space
		memset(&pf,0, sizeof(pf));	/* set up for para formatting */
		pf.cbSize = sizeof(pf);
		pf.dwMask = PFM_STARTINDENT|PFM_OFFSET | PFM_TABSTOPS;
		pf.dxStartIndent = 0;
		pf.dxOffset = (width + digitwidth)*TWIPS_PER_POINT-20;
		pf.cTabCount = 2;
		pf.rgxTabs[0] = (width*TWIPS_PER_POINT)|(2<<24);	// first is right tab
		pf.rgxTabs[1] = (width+digitwidth)*TWIPS_PER_POINT;
//		pf.rgxTabs[2] = 160*TWIPS_PER_POINT;
//		pf.rgxTabs[3] = 250*TWIPS_PER_POINT;
		txt_setparaformat(FF->twind,&pf);
		SendMessage(EX(FF->twind, hwed), EM_SETEVENTMASK, 0, ENM_SELCHANGE | ENM_KEYEVENTS | ENM_MOUSEEVENTS);	/* set event mask */
	}
	return (FF->twind);
}
/************************************************************************/
static LRESULT CALLBACK textdisplayproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)

{
	switch (msg)	{
		HANDLE_MSG(hwnd,WM_NOTIFY,vnotify);
	}
	return (txt_proc(hwnd,msg,wParam,lParam));	/* default handling */
}
/************************************************************************/
static int vnotify(HWND hwnd, int id, NMHDR * hdr)	/* does notification tasks */

{
	int line, selstart, selend, curpos, curline;
	POINT lp;		/*!!NB documentation bug: EM_CHARFROMPOS requires point */

	switch (hdr->code)	{
		case EN_MSGFILTER:
			switch (((MSGFILTER *)hdr)->msg)	{
				case WM_CHAR:		/* never handle characters */
					return (TRUE);
				case WM_KEYDOWN:
					switch (((MSGFILTER *)hdr)->wParam)	{
						case VK_UP:
						case VK_DOWN:
							curline = txt_selectpara(hdr->hwndFrom,((MSGFILTER *)hdr)->wParam == VK_UP ? -1 : 1);
							vdisplayrecord(hwnd,curline,FALSE);
							return(TRUE);
						case VK_RETURN:
							curline = Edit_LineFromChar(hdr->hwndFrom,-1);
							vdisplayrecord(hwnd,curline,TRUE);
							return (TRUE);
					}
					return (FALSE);		/* deal with original message */
				case WM_LBUTTONDOWN:
				case WM_LBUTTONDBLCLK:
					lp.x = LOWORD(((MSGFILTER *)hdr)->lParam);
					lp.y = HIWORD(((MSGFILTER *)hdr)->lParam);
					curpos = SendMessage(hdr->hwndFrom,EM_CHARFROMPOS,0,(LPARAM)&lp);	/* get char pos */
					txt_findpara(hdr->hwndFrom,curpos,&selstart,&selend,&line,TRUE);	/* select it */
					vdisplayrecord(hwnd,line,((MSGFILTER *)hdr)->msg == WM_LBUTTONDBLCLK);
					return (TRUE);
			}
	}
	return FORWARD_WM_NOTIFY(hwnd,id,hdr,txt_proc);	/* default notify handler */
}
/************************************************************************/
static void vdisplayrecord(HWND hwnd, int line, int frontflag)	/* selects record identified on line */

{
	INDEX * FF = getowner(hwnd);
	TCHAR tstring[STSTRING], *skipptr = tstring;
	RECN rnum;
	RECORD * trptr;

	if (getmmstate(FF->vwind,NULL) == SW_SHOWMINIMIZED)	/* if minimized */
		SendMessage(g_hwclient,WM_MDIRESTORE,(WPARAM)FF->vwind,0);	/* always force restore */
	Edit_GetLine(EX(hwnd, hwed),line,tstring,STSTRING-1);
	while (*skipptr == FSPACE)	// skip lead fixed space
		skipptr++;
	rnum = natoi(skipptr);
	if (!FF->rwind || mod_canenterrecord(FF->rwind,MREC_ALWAYSACCEPT))	{	// if no record window, or entry OK
		view_selectrec(FF,rnum,VD_SELPOS,-1,-1);
		if (frontflag || FF->rwind) {	// if want open record or window already open
			if (trptr = rec_getrec(FF,rnum))	/* if can edit record */
				mod_settext(FF,trptr->rtext,trptr->num, frontflag ? NULL : hwnd);
		}
	}
}
/*******************************************************************************/
void ts_adjustrefs(HWND hwnd)	/* adjusts references */

{
	struct adjstruct tadj;
	long ecount;
		
	memset(&tadj, 0, sizeof(struct adjstruct));
	if (DialogBoxParam(g_hinst,MAKEINTRESOURCE(IDD_ALTER),g_hwframe,adjproc,(LPARAM)&tadj))	{
		if (ecount = ref_adjust(WX(hwnd,owner), &tadj))
			senderr(ERR_RECMARKERR,WARN, ecount);          /* give message */
		if (tadj.regex)
			uregex_close(tadj.regex);
		view_redisplay(WX(hwnd,owner),0,VD_CUR);
	}
}
/******************************************************************************/
static INT_PTR CALLBACK adjproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)

{
	struct adjstruct *ajp;
	char tstring[STSTRING];
	int allrefs;

	ajp = getdata(hwnd);
	switch (msg)	{
		case WM_INITDIALOG:
			setdata(hwnd,(void *)lParam);	/* set data */
			centerwindow(hwnd,1);
			CheckRadioButton(hwnd,IDC_ALTER_ALLREFS,IDC_ALTER_RANGEREF,IDC_ALTER_ALLREFS);
			CheckRadioButton(hwnd,IDC_ALTER_ADJONLY,IDC_ALTER_REMOVE,IDC_ALTER_ADJONLY);
			disableitem(hwnd,IDC_ALTER_HOLDHIGHER);
			SetFocus(GetDlgItem(hwnd,IDC_ALTER_ADJUSTMENT));
			return FALSE;
		case WM_COMMAND:
			switch (LOWORD(wParam))	{
				case IDOK:
					if (getDItemText(hwnd,IDC_ALTER_PATTERN,tstring,STSTRING))	{	// if want regex
						if (!(ajp->regex = regex_build(tstring,0)))	{	/* if bad expression */
							senderr(ERR_BADEXPERR,WARN,tstring);
							selectitext(hwnd,IDC_ALTER_PATTERN);
							break;
						}
						ajp->patflag = TRUE;
					}
					getlong(hwnd,IDC_ALTER_RANGESTART,&ajp->low);
					getlong(hwnd,IDC_ALTER_RANGENED,&ajp->high);
					allrefs = isitemchecked(hwnd,IDC_ALTER_ALLREFS);
					ajp->hold = isitemchecked(hwnd,IDC_ALTER_HOLDHIGHER);
					ajp->cut = findgroupcheck(hwnd,IDC_ALTER_ADJONLY,IDC_ALTER_REMOVE)-IDC_ALTER_ADJONLY;
					if (ajp->cut && !ajp->low && !ajp->high)	{	/* if haven't specified at least one end of range to cut */
						senderr(ERR_NOLOCATORRANGE,WARN);
						selectitext(hwnd,IDC_ALTER_RANGESTART);
						break;
					}
					if (!ajp->low)
						ajp->low = 1;
					if (!ajp->high)
						ajp->high = LONG_MAX;
					if (ajp->low > ajp->high)	{
						senderr(ERR_REFORDERERR,WARN);
						selectitext(hwnd,IDC_ALTER_RANGESTART);
						break;
					}
					if (!getlong(hwnd,IDC_ALTER_ADJUSTMENT,&ajp->shift) && !ajp->cut)	{ /* if no adjustment and not cutting */
						getDItemText(hwnd,IDC_ALTER_ADJUSTMENT,tstring,STSTRING);
						senderr(ERR_BADNUMERR,WARN,tstring);
						selectitext(hwnd,IDC_ALTER_ADJUSTMENT);
						break;
					}
					if (ajp->shift+ajp->low < 1)	{	/* if would potentially remove refs */
						if (!sendwarning(WARN_NEGADJUST, ajp->low, -ajp->shift))
							break;
					}
				case IDCANCEL:
					EndDialog(hwnd,LOWORD(wParam) == IDOK ? TRUE : FALSE);
					return TRUE;
				case IDC_ALTER_RANGEREF:
					selectitext(hwnd,IDC_ALTER_RANGESTART);
					return TRUE;
				case IDC_ALTER_RANGESTART:
				case IDC_ALTER_RANGENED:
					if (HIWORD(wParam) == EN_CHANGE)
						CheckRadioButton(hwnd,IDC_ALTER_ALLREFS,IDC_ALTER_RANGEREF,IDC_ALTER_RANGEREF);
					return TRUE;
				case IDC_ALTER_ADJONLY:
					checkitem(hwnd,IDC_ALTER_HOLDHIGHER,FALSE);
					disableitem(hwnd,IDC_ALTER_HOLDHIGHER);
					enableitem(hwnd,IDC_ALTER_ALLREFS);
					return TRUE;
				case IDC_ALTER_REMOVE:
					CheckRadioButton(hwnd,IDC_ALTER_ALLREFS,IDC_ALTER_RANGEREF,IDC_ALTER_RANGEREF);
					enableitem(hwnd,IDC_ALTER_HOLDHIGHER);
					disableitem(hwnd,IDC_ALTER_ALLREFS);
					return TRUE;
			}
			break;
		case WM_HELP:
			return (dodialoghelp(hwnd,TEXT("base\\Pageref_alter.htm"),(HELPINFO *)lParam,wh_adjid));
	}
	return FALSE;
}
/**********************************************************************************/
void ts_checkindex(HWND hwnd)	// checks index

{
	static char * verifyerrors[] = {	// verification error messages
		"",
		"too few targets",
		"circular or open",
		"missing target",
		"case/style/accent mismatch",
	};

	static char * errorstrings[] = {
		"Multiple spaces",		// currently unused
		"Misplaced or questionable punctuation",
		"Space missing before ( or [",
		"Unbalanced parens or brackets",
		"Unbalanced quotation marks",
		"Mixed case word(s)",
		"Misused special character",
		"Misused brackets",
		"Extraneous code",

		"Inconsistent letter case",
		"Inconsistent style/typeface",
		"Inconsistent punctuation",
		"Inconsistent leading conjunction/preposition",
		"Inconsistent plural ending",
		"Inconsistent trailing conjunction/preposition",
		"Inconsistent parenthetical ending",
		"Orphaned subheading",

		"Missing locator",
		"Too many page references",
		"Overlapping page references",
		"Locator not at lowest heading",

		"Invalid cross reference"
	};

	static CHECKPARAMS defaults = {
		0,	// version
		{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1, },	// keys
		5,	// page ref limit
		{ 0 },	// join params
		{NULL,1 },	// verify params
	};

#define NPAGES 3

	PROPSHEETHEADER psh;
	PROPSHEETPAGE psp[NPAGES];
	int count;

	INDEX * FF = WX(hwnd, owner);
	CHECKPARAMS chp = defaults;	// set defaults
	CARRIER cp;
	DWORD size = sizeof(CHECKPARAMS);

	reg_getkeyvalue(K_GENERAL, CHECKSETTINGS, &chp, &size);		// replace defaults if prior save
	cp.FF = FF;
	cp.params = &chp;
	memset(psp, 0, sizeof(psp));
	for (count = 0; count < NPAGES; count++) {
		psp[count].dwSize = sizeof(PROPSHEETPAGE);
		psp[count].dwFlags = PSP_HASHELP;
		psp[count].hInstance = g_hinst;
		psp[count].pszTemplate = MAKEINTRESOURCE(count + IDD_CHECK_BASIC);
		psp[count].lParam = (LONG_PTR)&cp;
	}
	psp[0].pfnDlgProc = cbasicproc;
	psp[1].pfnDlgProc = cheadingproc;
	psp[2].pfnDlgProc = crefproc;

	memset(&psh, 0, sizeof(psh));
	psh.dwSize = sizeof(psh);
	psh.dwFlags = PSH_PROPSHEETPAGE | PSH_HASHELP | PSH_NOAPPLYNOW;
	psh.hwndParent = hwnd ? hwnd : g_hwframe;
	psh.hInstance = g_hinst;
	psh.pszCaption = TEXT("Check Index");
	psh.nPages = NPAGES;
	psh.ppsp = psp;

	if (PropertySheet(&psh)) {		/* ok */
		reg_setkeyvalue(K_GENERAL, CHECKSETTINGS, REG_BINARY, &chp, sizeof(chp));	// save
		chp.errors = calloc(FF->head.rtot + 1, sizeof(CHECKERROR *));
		chp.vg.locatoronly = FF->head.refpars.clocatoronly;
		chp.jng.nosplit = TRUE;
		chp.jng.orphanaction = OR_PRESERVE;
		chp.jng.errors = chp.errors;
		tool_check(FF, &chp);
		HWND tw = NULL;

		for (RECORD * curptr = sort_top(FF); curptr; curptr = sort_skip(FF, curptr, 1)) {
			CHECKERROR * ebase = chp.errors[curptr->num];
			if (ebase) {	// if any errors recorded
				char rbase[MAXREC];
				int errorCount = 0;
				char * pos = rbase;
				for (int findex = 0; findex < FF->head.indexpars.maxfields; findex++) {
					if (ebase->fields[findex]) {	// if errors
						for (int bitpos = 0; bitpos < 31; bitpos++) {	// for all potential errors
							if (ebase->fields[findex] & (1 << bitpos) && chp.reportKeys[bitpos]) {	// if error and want to see it
								if (ebase->fields[findex] & (1 << bitpos) && chp.reportKeys[bitpos]) {	// if error and want to see it
									char * fname = FF->head.indexpars.field[findex < FF->head.indexpars.maxfields - 1 ? findex : PAGEINDEX].name;
									char message[MAXREC];
									if ((1 << bitpos) == CE_CROSSERR) {	// if a crossref error, create description
										for (VERIFY * ceptr = ebase->crossrefs; ceptr->error; ceptr++) {	// for all errors
											char * typeerror = "";		// default no type error
											BOOL wrongtype = ceptr->error&V_TYPEERR;
											ceptr->error &= ~V_TYPEERR;	// clear type flag
											if (wrongtype)
												typeerror = ceptr->error ? "wrong type and " : "wrong type";
//											sprintf(message, "Cross reference from “%s” to “%.*s”: %s%s", curptr->rtext, ceptr->length, curptr->rtext + ceptr->offset, typeerror, verifyerrors[ceptr->error]);
											sprintf(message, u8"Cross reference from \u201C%s\u201D to \u201C%.*s\u201D: %s%s", curptr->rtext, ceptr->length, curptr->rtext + ceptr->offset, typeerror, verifyerrors[ceptr->error]);
										}
									}
									else if ((1 << bitpos) == CE_TOOMANYPAGE)	// if re count error
									//	sprintf(message, "Too many references (%d) to “%s”", ebase->refcount, curptr->rtext);
										sprintf(message, u8"Too many references (%d) to \u201C%s\u201D", ebase->refcount, curptr->rtext);
									else	// other error; just show string
										strcpy(message, errorstrings[bitpos]);
									if (!errorCount)
										pos += sprintf(rbase, "\t%u\t%s: %s", curptr->num, fname, message);	// lead for record, with error report
									else 	// start a new error field
										pos += sprintf(pos, "%s\t%s: %s", LINEBREAK, fname, message);	// lead for field, with error report
									errorCount++;
								}
							}
						}
					}
				}
				if (ebase->crossrefs)
					free(ebase->crossrefs);
				free(ebase);
				if (errorCount) {
					*pos++ = '\r';
					*pos++ = '\0';
					if (!tw)	// if don't yet have text window
						tw = setwindow(FF, "Index Check Results", 0, _checkheader, TEXT("base\\Tools_checkindex.htm"));
//					TCHAR * tptr = toNative(rbase);
					txt_appendctext(FF->twind, toNative(rbase));
				}
			}
		}
		free(chp.errors);
//		return (TRUE);
	}
//	return (FALSE);
}
/**********************************************************************************/
static INT_PTR CALLBACK cbasicproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)

{
	BOOL * rKeyPtr;
	CARRIER * cp = getdata(hwnd);
	CHECKPARAMS * chp;

	switch (msg) {
	case WM_INITDIALOG:
		if (cp = setdata(hwnd, (void *)((LPPROPSHEETPAGE)lParam)->lParam)) {	/* set data */
			centerwindow(GetParent(hwnd), 1);		/* need to center whole prop sheet at this stage (when init first page) */
			chp = cp->params;
			rKeyPtr = &chp->reportKeys[1];	// skip multi-space
			checkitem(hwnd, IDC_CHECK_B_PUNCTSPACE,*rKeyPtr++);
			checkitem(hwnd, IDC_CHECK_B_MISSINGSPACE, *rKeyPtr++);
			checkitem(hwnd, IDC_CHECK_B_UNBALANCEDPAREN, *rKeyPtr++);
			checkitem(hwnd, IDC_CHECK_B_UNBALANCEDQUOTE, *rKeyPtr++);
			checkitem(hwnd, IDC_CHECK_B_MIXEDCASE, *rKeyPtr++);
			checkitem(hwnd, IDC_CHECK_B_MISUSED, TRUE);	// always true
		}
		return (TRUE);
	case WM_NOTIFY:
		switch (((NMHDR *)lParam)->code) {
			case PSN_KILLACTIVE:
				chp = cp->params;
				rKeyPtr = chp->reportKeys;
				*rKeyPtr++ = FALSE;	// multi space
				*rKeyPtr++ = isitemchecked(hwnd, IDC_CHECK_B_PUNCTSPACE);
				*rKeyPtr++ = isitemchecked(hwnd, IDC_CHECK_B_MISSINGSPACE);
				*rKeyPtr++ = isitemchecked(hwnd, IDC_CHECK_B_UNBALANCEDPAREN);
				*rKeyPtr++ = isitemchecked(hwnd, IDC_CHECK_B_UNBALANCEDQUOTE);
				*rKeyPtr++ = isitemchecked(hwnd, IDC_CHECK_B_MIXEDCASE);
				*rKeyPtr++ = YES;		// misused escape
				*rKeyPtr++ = YES;		// misued brackets
				*rKeyPtr++ = YES;		// bad code
				SetWindowLongPtr(hwnd, DWLP_MSGRESULT, FALSE);
				return (TRUE);		/* check our results; SetwindowLong TRUE to hold page; FALSE to free */
			case PSN_HELP:
				dowindowhelp(c_checkhelpbasic);
				return (TRUE);
		}
		break;
	case WM_COMMAND:
		if (HIWORD(wParam) == EN_UPDATE) {
			return TRUE;
		}
		switch (LOWORD(wParam)) {
			case IDC_SORT_TEXTSIMPLE:
				break;
		}
		break;
	case WM_HELP:
		return (dodialoghelp(hwnd, TEXT("base\\Check_basic.htm"), (HELPINFO *)lParam, wh_checkid));
	default:
		;
	}
	return (FALSE);
}
/**********************************************************************************/
static INT_PTR CALLBACK cheadingproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)

{
	BOOL * rKeyPtr;
	CARRIER * cp = getdata(hwnd);
	CHECKPARAMS * chp;

	switch (msg) {
	case WM_INITDIALOG:
		if (cp = setdata(hwnd, (void *)((LPPROPSHEETPAGE)lParam)->lParam)) {	/* set data */
			buildheadingmenu(GetDlgItem(hwnd, IDC_CHECK_H_ORPHANDEPTH), &cp->FF->head.indexpars, 0);
			chp = cp->params;
			rKeyPtr = &chp->reportKeys[9];	// skip  
			checkitem(hwnd, IDC_CHECK_H_INCONSISTENTCAPS, *rKeyPtr++);
			checkitem(hwnd, IDC_CHECK_H_INCONSISTENTSTYLE, *rKeyPtr++);
			checkitem(hwnd, IDC_CHECK_H_INCONSISTENTPUNCT, *rKeyPtr++);
			checkitem(hwnd, IDC_CHECK_H_INCONSISTENTLEADPREP, *rKeyPtr++);
			checkitem(hwnd, IDC_CHECK_H_INCONSISTENTPLURALS, *rKeyPtr++);
			checkitem(hwnd, IDC_CHECK_H_INCONSISTENTPREP, *rKeyPtr++);
			checkitem(hwnd, IDC_CHECK_H_INCONSISTENTPAREN, *rKeyPtr++);
			checkitem(hwnd, IDC_CHECK_H_ORPHANEDSUBHEAD, *rKeyPtr++);
			ComboBox_SetCurSel(GetDlgItem(hwnd, IDC_CHECK_H_ORPHANDEPTH), chp->jng.firstfield);
		}
		return (TRUE);
	case WM_NOTIFY:
		switch (((NMHDR *)lParam)->code) {
		case PSN_KILLACTIVE:
			chp = cp->params;
			rKeyPtr = &chp->reportKeys[9];
			*rKeyPtr++ = isitemchecked(hwnd, IDC_CHECK_H_INCONSISTENTCAPS);
			*rKeyPtr++ = isitemchecked(hwnd, IDC_CHECK_H_INCONSISTENTSTYLE);
			*rKeyPtr++ = isitemchecked(hwnd, IDC_CHECK_H_INCONSISTENTPUNCT);
			*rKeyPtr++ = isitemchecked(hwnd, IDC_CHECK_H_INCONSISTENTLEADPREP);
			*rKeyPtr++ = isitemchecked(hwnd, IDC_CHECK_H_INCONSISTENTPLURALS);
			*rKeyPtr++ = isitemchecked(hwnd, IDC_CHECK_H_INCONSISTENTPREP);
			*rKeyPtr++ = isitemchecked(hwnd, IDC_CHECK_H_INCONSISTENTPAREN);
			*rKeyPtr++ = isitemchecked(hwnd, IDC_CHECK_H_ORPHANEDSUBHEAD);
			chp->jng.firstfield = ComboBox_GetCurSel(GetDlgItem(hwnd, IDC_CHECK_H_ORPHANDEPTH));	/* get field index */
			SetWindowLongPtr(hwnd, DWLP_MSGRESULT, FALSE);
			return (TRUE);		/* check our results; SetwindowLong TRUE to hold page; FALSE to free */
		case PSN_HELP:
			dowindowhelp(c_checkhelpheadings);
			return (TRUE);
		}
		break;
	case WM_COMMAND:
		if (HIWORD(wParam) == EN_UPDATE) {
			return TRUE;
		}
		switch (LOWORD(wParam)) {
			case IDC_SORT_TEXTSIMPLE:
				break;
		}
		break;
	case WM_HELP:
		return (dodialoghelp(hwnd, TEXT("base\\Check_headings.htm"), (HELPINFO *)lParam, wh_checkid));
	default:
		;
	}
	return (FALSE);
}
/**********************************************************************************/
static INT_PTR CALLBACK crefproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)

{
	BOOL * rKeyPtr;
	CARRIER * cp = getdata(hwnd);
	CHECKPARAMS * chp;

	switch (msg) {
	case WM_INITDIALOG:
		if (cp = setdata(hwnd, (void *)((LPPROPSHEETPAGE)lParam)->lParam)) {	/* set data */
			chp = cp->params;
			rKeyPtr = &chp->reportKeys[17];	// skip
			checkitem(hwnd, IDC_CHECK_R_EMPTYPAGE, *rKeyPtr++);
			checkitem(hwnd, IDC_CHECK_R_TOOMANYPAGE, *rKeyPtr++);
			checkitem(hwnd, IDC_CHECK_R_OVERLAPPINGPAGE, *rKeyPtr++);
			checkitem(hwnd, IDC_CHECK_R_HEADINGLEVEL, *rKeyPtr++);
			checkitem(hwnd, IDC_CHECK_R_VERIFY, *rKeyPtr++);
			checkitem(hwnd, IDC_CHECK_R_EXACTMATCH, chp->vg.fullflag);
			setint(hwnd, IDC_CHECK_R_MINMATCHES, chp->vg.lowlim);
			setint(hwnd, IDC_CHECK_R_PAGEREFLIMIT, chp->pagereflimit);
		}
		return (TRUE);
	case WM_NOTIFY:
		switch (((NMHDR *)lParam)->code) {
		case PSN_KILLACTIVE:
			chp = cp->params;
			rKeyPtr = &chp->reportKeys[17];
			*rKeyPtr++ = isitemchecked(hwnd, IDC_CHECK_R_EMPTYPAGE);
			*rKeyPtr++ = isitemchecked(hwnd, IDC_CHECK_R_TOOMANYPAGE);
			*rKeyPtr++ = isitemchecked(hwnd, IDC_CHECK_R_OVERLAPPINGPAGE);
			*rKeyPtr++ = isitemchecked(hwnd, IDC_CHECK_R_HEADINGLEVEL);
			*rKeyPtr++ = isitemchecked(hwnd, IDC_CHECK_R_VERIFY);
			chp->vg.fullflag = isitemchecked(hwnd, IDC_CHECK_R_EXACTMATCH);
			getshort(hwnd, IDC_CHECK_R_MINMATCHES, &chp->vg.lowlim);
			getlong(hwnd, IDC_CHECK_R_PAGEREFLIMIT, &chp->pagereflimit);
			SetWindowLongPtr(hwnd, DWLP_MSGRESULT, FALSE);
			return (TRUE);		/* check our results; SetwindowLong TRUE to hold page; FALSE to free */
		case PSN_HELP:
			dowindowhelp(c_checkhelprefs);
			return (TRUE);
		}
		break;
	case WM_COMMAND:
		if (HIWORD(wParam) == EN_UPDATE) {
			return TRUE;
		}
		switch (LOWORD(wParam)) {
			case IDC_SORT_TEXTSIMPLE:
				break;
		}
		break;
	case WM_HELP:
		return (dodialoghelp(hwnd, TEXT("base\\Check_refs.htm"), (HELPINFO *)lParam, wh_checkid));
	default:
		;
	}
	return (FALSE);
}
/*******************************************************************************/
void ts_splitheadings(HWND hwnd)	// splits headings

{
	INDEX * FF = WX(hwnd, owner);
	SPLITPARAMS params;
	DWORD size = sizeof(SPLITPARAMS);

	memset(&params, 0, sizeof(params));
	reg_getkeyvalue(K_GENERAL, SPLITSETTINGS, &params, &size);		// setup if prior save
	params.preflight = FALSE;		// prior param save in registry leaves it set
	if (DialogBoxParam(g_hinst, MAKEINTRESOURCE(IDD_SPLIT), g_hwframe, splitproc, (LPARAM)&params)) {
		reg_setkeyvalue(K_GENERAL, SPLITSETTINGS, REG_BINARY, &params, sizeof(params));	// save
		if (params.preflight) {
			params.reportlist = calloc(FF->head.rtot + 1, sizeof(char *));		// memory for pointer array
			tool_explode(FF, &params);
			if (params.reportlist[0]) {	// if have anything to report
				if (setwindow(FF, "Split Preview", 0, _splitheader, TEXT("base\\Tools_splitheadings.htm"))) {
					for (RECN rindex = 0; params.reportlist[rindex]; rindex++) {
						TCHAR * tptr = toNative(params.reportlist[rindex]);
						txt_appendctext(FF->twind, tptr);
						free(params.reportlist[rindex]);
					}
					free(params.reportlist);
				}
			}
		}
		else if (sendwarning(WARN_SPLIT)) {
			tool_explode(FF, &params);
			sendinfo(INFO_SPLITRECORDS, params.gencount, params.modcount, params.markcount);
			view_redisplay(FF, 0, VD_CUR);
		}
	}
}
/******************************************************************************/
static INT_PTR CALLBACK splitproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)

{
	SPLITPARAMS * params = getdata(hwnd);
	HWND cbb = GetDlgItem(hwnd, IDC_SPLIT_PATTERN);

	switch (msg) {
	case WM_INITDIALOG:
		if (params = setdata(hwnd, (void *)lParam)) {	/* set data */
			ComboBox_AddString(cbb, TEXT("Terms separated by punctuation"));
			ComboBox_AddString(cbb, TEXT("Surname, Forename(s)"));
			ComboBox_AddString(cbb, TEXT("Forename(s) Surname"));
			ComboBox_AddString(cbb, TEXT("User-defined pattern:"));
			ComboBox_SetCurSel(cbb, params->patternindex >= 0 ? params->patternindex : ComboBox_GetCount(cbb)-1);	// set last if user-defined
			checkitem(hwnd, IDC_SPLIT_REMOVESTYLES, params->removestyles);
			checkitem(hwnd, IDC_SPLIT_MARKRECORDS, params->markmissing);
			setDItemText(hwnd, IDC_SPLIT_USERPATTERN, params->userpattern);
			centerwindow(hwnd, 1);
			SetFocus(cbb);
		}
		return FALSE;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_SPLIT_PREVIEW:
			params->preflight = YES;	// fall through
		case IDOK:
			params->removestyles = isitemchecked(hwnd, IDC_SPLIT_REMOVESTYLES);
			params->markmissing = isitemchecked(hwnd, IDC_SPLIT_MARKRECORDS);
			params->patternindex = ComboBox_GetCurSel(cbb);
			if (params->patternindex == ComboBox_GetCount(cbb) - 1)	// if last item (user defined pattern)
				params->patternindex = -1;		// fix
			getDItemText(hwnd, IDC_SPLIT_USERPATTERN, params->userpattern, SPLITPATTERNLEN-1);
			if (params->patternindex == -1 && !regex_validexpression(params->userpattern, 0)) {
				selectitext(hwnd, IDC_SPLIT_USERPATTERN);
				return TRUE;
			}
		case IDCANCEL:
			EndDialog(hwnd, LOWORD(wParam) == IDCANCEL ? FALSE : TRUE);
			return TRUE;
		case IDC_SPLIT_PATTERN:
			if (ComboBox_GetCurSel(cbb) == 3)
				enableitem(hwnd, IDC_SPLIT_USERPATTERN);
			else
				disableitem(hwnd, IDC_SPLIT_USERPATTERN);
			return TRUE;
		}
		break;
	case WM_HELP:
		return (dodialoghelp(hwnd, TEXT("base\\Tools_splitheadings.htm"), (HELPINFO *)lParam, wh_splitid));
	}
	return FALSE;
}
/*******************************************************************************/
void ts_count(HWND hwnd)	/* counts records */

{
	DialogBoxParam(g_hinst,MAKEINTRESOURCE(IDD_COUNT),g_hwframe,countproc,(LPARAM)WX(hwnd,owner));
}
/******************************************************************************/
static INT_PTR CALLBACK countproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)

{
	INDEX * FF;
	char string1[STSTRING],string2[STSTRING], *e1ptr, *e2ptr;
	TCHAR us[STSTRING];
	COUNTPARAMS cc;
	int i, colcount, tabstop, totalrec;
	long ref1, ref2;
	short scope, err;
	HWND hwed;

	FF = getdata(hwnd);
	switch (msg)	{

		case WM_INITDIALOG:
			if (FF = setdata(hwnd,(void *)lParam))	{	/* set data */
				Edit_LimitText(GetDlgItem(hwnd,IDC_COUNT_PAGESTART),FSSTRING);		/* limit length */
				Edit_LimitText(GetDlgItem(hwnd,IDC_COUNT_PAGEEND),FSSTRING);		/* limit length */
				CheckRadioButton(hwnd,IDC_FIND_ALL,IDC_FIND_RANGE,IDC_FIND_ALL);
				if (!view_recordselect(FF->vwind))		/* if no selection */	
					disableitem(hwnd,IDC_FIND_SELECTED);	/* disable item */
				if (FF->head.privpars.vmode == VM_FULL)	/* if formatted view */
					disableitem(hwnd,IDC_FIND_DELREC);
				tabstop = 33;
				Edit_SetTabStops(GetDlgItem(hwnd,IDC_COUNT_RESULT),1,&tabstop);
				centerwindow(hwnd,1);
			}
			return FALSE;
		case WM_COMMAND:
			if (HIWORD(wParam) == EN_UPDATE)	{
				WORD id = LOWORD(wParam);
				int length = 0;

				if (id == IDC_COUNT_PAGESTART || id == IDC_COUNT_PAGEEND)
					length = FSSTRING;
				checktextfield((HWND)lParam,length);
				return TRUE;
			}
			switch (LOWORD(wParam))	{
				case IDC_COUNT_START:
					memset(&cc, 0, sizeof(COUNTPARAMS));		/* clear structure */
					getDItemText(hwnd,IDC_FIND_RANGESTART,string1,STSTRING);
					getDItemText(hwnd,IDC_FIND_RANGEEND,string2,STSTRING);
					getDItemText(hwnd,IDC_COUNT_PAGESTART,cc.firstref,FSSTRING);
					getDItemText(hwnd,IDC_COUNT_PAGEEND,cc.lastref,FSSTRING);
					if (*cc.lastref && !*cc.firstref || ref_match(FF,cc.firstref, cc.lastref, FF->head.sortpars.partorder, PMEXACT|PMSENSE) > 0)	{
						senderr(*cc.firstref ? ERR_INVALPAGERANGE : ERR_INVALLOCATORRANGE,WARN);
						selectitext(hwnd,*cc.firstref ? IDC_COUNT_PAGEEND : IDC_COUNT_PAGESTART);
						return (TRUE);
					}
					scope = findgroupcheck(hwnd,IDC_FIND_ALL,IDC_FIND_RANGE)-IDC_FIND_ALL;
					cc.smode = FF->head.sortpars.ison;
					if (scope == COMR_ALL && !FF->curfile)	{		/* make it fast if doing all records & no group */
						cc.firstrec = rec_number(sort_top(FF));
						cc.lastrec = ULONG_MAX;
					}
					else	{
						if (err = com_getrecrange(FF,scope, string1,string2, &cc.firstrec, &cc.lastrec))	{
							selectitext(hwnd,err < 0 ? IDC_FIND_RANGESTART : IDC_FIND_RANGEEND);
							return (TRUE);
						}
					}
					cc.delflag = (char)isitemchecked(hwnd,IDC_FIND_DELREC) ? CO_ONLYDEL : COMR_ALL;
					cc.markflag = (char)isitemchecked(hwnd,IDC_FIND_MARKREC);
					cc.modflag = (char)isitemchecked(hwnd,IDC_FIND_MODREC);
					cc.genflag = (char)isitemchecked(hwnd,IDC_FIND_GENREC);
					cc.tagflag = (char)isitemchecked(hwnd,IDC_FIND_TAGREC);
					totalrec = search_count(FF,&cc,SF_VIEWDEFAULT);	/* get count */
					hwed = GetDlgItem(hwnd,IDC_COUNT_RESULT);
					Edit_SetText(hwed,TEXT(""));		/* clear out the control (from any previous passes) */
#if 0
					sprintf(string1,"%ld Records", totalrec);
//					SendMessage(hwed,EM_REPLACESEL,FALSE,(LPARAM)string1);
					appendString(hwed,string1);
					ref1 = strtol(cc.firstref,&e1ptr,10);
					ref2 = strtol(cc.lastref,&e2ptr,10);
					if (!*e1ptr && !*e2ptr && ref1 && ref2)	{	/* if decent arabic range */
						sprintf(string1," (%01.1f per page)", (float)totalrec/(ref2+1-ref1));
//						SendMessage(hwed,EM_REPLACESEL,FALSE,(LPARAM)string1);
						appendString(hwed,string1);
					}
					if (totalrec)	{	/* if any info to display */
//						sprintf(string1,"\t\t(%ld modified, %ld deleted, %ld marked, %ld generated, %ld labeled)\r\n",
//							cc.modified, cc.deleted, cc.marked, cc.generated, cc.labeled);
						sprintf(string1,"\r\nModified: %ld;  Deleted: %ld;  Marked: %ld;  Generated: %ld;  Labeled: [1:%ld, 2:%ld, 3:%ld, 4:%ld, 5:%ld, 6:%ld, 7:%ld]\r\n",
							cc.modified, cc.deleted, cc.marked, cc.generated, cc.labeled[1],cc.labeled[2],cc.labeled[3],cc.labeled[4],cc.labeled[5],cc.labeled[6],cc.labeled[7]);
//						SendMessage(hwed,EM_REPLACESEL,FALSE,(LPARAM)string1);
						appendString(hwed,string1);
						sprintf(string1,"Deepest (#%ld) contains %u levels\tLongest (#%ld) contains %u characters\r\n\r\n",
							 cc.deeprec, cc.deepest-1, cc.longrec, cc.longest);		 /* print number */
//						SendMessage(hwed,EM_REPLACESEL,FALSE,(LPARAM)string1);
						appendString(hwed,string1);
						for (i = colcount = 0; cc.leads[i].total && i < COUNTTABSIZE; i++) {		/* for whole table */
//							if (cc.leads[i])	{	/* if an entry */
								sprintf(string1,"%c %ld\t", cc.leads[i].lead, cc.leads[i].total);
//								SendMessage(hwed,EM_REPLACESEL,FALSE,(LPARAM)string1);
								appendString(hwed,string1);
								colcount++;
								if (!(colcount %= 10))	/* if at end of line */
									SendMessage(hwed,EM_REPLACESEL,FALSE,(LPARAM)TEXT("\r\n"));
//							}
						}
					}
#else
					u_sprintf(us,"%ld Records", totalrec);
					SendMessage(hwed,EM_REPLACESEL,FALSE,(LPARAM)us);
					ref1 = strtol(cc.firstref,&e1ptr,10);
					ref2 = strtol(cc.lastref,&e2ptr,10);
					if (!*e1ptr && !*e2ptr && ref1 && ref2)	{	/* if decent arabic range */
						u_sprintf(us," (%01.1f per page)", (float)totalrec/(ref2+1-ref1));
						SendMessage(hwed,EM_REPLACESEL,FALSE,(LPARAM)us);
					}
					if (totalrec)	{	/* if any info to display */
						u_sprintf(us,"\r\nModified: %ld;  Deleted: %ld;  Marked: %ld;  Generated: %ld;  Labeled: [1:%ld, 2:%ld, 3:%ld, 4:%ld, 5:%ld, 6:%ld, 7:%ld]\r\n",
							cc.modified, cc.deleted, cc.marked, cc.generated, cc.labeled[1],cc.labeled[2],cc.labeled[3],cc.labeled[4],cc.labeled[5],cc.labeled[6],cc.labeled[7]);
						SendMessage(hwed,EM_REPLACESEL,FALSE,(LPARAM)us);
						u_sprintf(us,"Deepest (#%ld) contains %u levels\tLongest (#%ld) contains %u characters\r\n\r\n",
							 cc.deeprec, cc.deepest-1, cc.longrec, cc.longest);		 /* print number */
						SendMessage(hwed,EM_REPLACESEL,FALSE,(LPARAM)us);
						for (i = colcount = 0; cc.leads[i].total && i < COUNTTABSIZE; i++) {		/* for whole table */
							u_sprintf(us,"%C %ld\t", cc.leads[i].lead, cc.leads[i].total);
							SendMessage(hwed,EM_REPLACESEL,FALSE,(LPARAM)us);
							colcount++;
							if (!(colcount %= 10))	/* if at end of line */
								SendMessage(hwed,EM_REPLACESEL,FALSE,(LPARAM)TEXT("\r\n"));
						}
					}
#endif
					return (FALSE);
				case IDC_GENERIC_DONE:
				case IDCANCEL:
					EndDialog(hwnd,FALSE);
					return TRUE;
				case IDC_FIND_RANGESTART:
				case IDC_FIND_RANGEEND:
					if (HIWORD(wParam) == EN_CHANGE)
						CheckRadioButton(hwnd,IDC_FIND_ALL,IDC_FIND_RANGE, IDC_FIND_RANGE);
					break;
			}
			break;
		case WM_HELP:
			return (dodialoghelp(hwnd,TEXT("base\\Index Informationcount.htm"),(HELPINFO *)lParam,wh_countid));
	}
	return FALSE;
}
/*******************************************************************************/
void ts_statistics(HWND hwnd)	/* gets statistics */

{
	DialogBoxParam(g_hinst,MAKEINTRESOURCE(IDD_STATISTICS),g_hwframe,statsproc,(LPARAM)WX(hwnd,owner));
}
/******************************************************************************/
static INT_PTR CALLBACK statsproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)

{

	INDEX * FF;
	char string1[STSTRING],string2[STSTRING];
	TCHAR us[STSTRING];
	int tnum;
	short scope, err;
	HWND hwed;

	FF = getdata(hwnd);
	switch (msg)	{

		case WM_INITDIALOG:
			if (FF = setdata(hwnd,(void *)lParam))	{	/* set data */
				centerwindow(hwnd,1);
				CheckRadioButton(hwnd,IDC_FORMWRITE_ALL,IDC_FORMWRITE_RECORDS,IDC_FORMWRITE_ALL);
				if (!view_recordselect(FF->vwind))		/* if no selection */	
					disableitem(hwnd,IDC_FIND_SELECTED);	/* disable item */
				displaybasestats(GetDlgItem(hwnd,IDC_STATISTICS_RESULT),FF);
			}
			return FALSE;
		case WM_COMMAND:
			switch (LOWORD(wParam))	{
				case IDC_STATISTICS_GO:
					memset(&FF->pf,0,sizeof(PRINTFORMAT));
					FF->singlerefcount = FALSE;		// page range to count as 2 refs
					FF->pf.first = 1;
					FF->pf.last = INT_MAX;
					scope = findgroupcheck(hwnd,IDC_FORMWRITE_ALL,IDC_FORMWRITE_RECORDS)-IDC_FORMWRITE_ALL;
					if (scope == RR_PAGES) {		/* if want page range */
						if (getlong(hwnd,IDC_FORMWRITE_PSTART,&tnum))	/* if not empty */
							FF->pf.first = tnum;
						if (getlong(hwnd,IDC_FORMWRITE_PEND,&tnum))		/* if not empty */
							FF->pf.last = tnum;
						if (FF->pf.first > FF->pf.last)	{
							senderr(ERR_INVALPAGERANGE,WARN);
							selectitext(hwnd,IDC_FORMWRITE_PEND);
							return(TRUE);
						}
					}
					if (scope != COMR_ALL)	/* if not all */
						scope--;			/* to discount page range stuff */
					getDItemText(hwnd,IDC_FORMWRITE_RSTART,string1,STSTRING);
					getDItemText(hwnd,IDC_FORMWRITE_REND,string2,STSTRING);
					if (err = com_getrecrange(FF,scope, string1,string2,&FF->pf.firstrec, &FF->pf.lastrec))	{	/* bad range */
						selectitext(hwnd,err < 0 ? IDC_FORMWRITE_RSTART : IDC_FORMWRITE_REND);
						return (TRUE);
					}
					view_formsilentimages(FF->vwind);
					hwed = GetDlgItem(hwnd,IDC_STATISTICS_RESULT);
					displaybasestats(hwed,FF);
					u_sprintf(us, "%d Pages, %ld Lines.", FF->pf.pageout,FF->pf.lines);
					SendMessage(hwed,EM_REPLACESEL,FALSE,(LPARAM)us);
					u_sprintf(us, "\r\n%ld Entries, %ld Unique main headings.", FF->pf.entries, FF->pf.uniquemain);
					SendMessage(hwed,EM_REPLACESEL,FALSE,(LPARAM)us);
					if (FF->head.privpars.vmode == VM_FULL)	{	/* if doing formatted */
						u_sprintf(us,"\r\n%ld Page references, %ld Cross-references.",FF->pf.prefs,FF->pf.crefs);
						SendMessage(hwed,EM_REPLACESEL,FALSE,(LPARAM)us);
					}
					return (TRUE);
				case IDC_GENERIC_DONE:
				case IDCANCEL:
					EndDialog(hwnd,FALSE);
					return TRUE;
				case IDC_FORMWRITE_RSTART:
				case IDC_FORMWRITE_REND:
					if (HIWORD(wParam) == EN_CHANGE)
						CheckRadioButton(hwnd,IDC_FORMWRITE_ALL,IDC_FORMWRITE_RECORDS, IDC_FORMWRITE_RECORDS);
					break;
				case IDC_FORMWRITE_PSTART:
				case IDC_FORMWRITE_PEND:
					if (HIWORD(wParam) == EN_CHANGE)
						CheckRadioButton(hwnd,IDC_FORMWRITE_ALL,IDC_FORMWRITE_RECORDS, IDC_FORMWRITE_PAGE);
					break;
			}
			break;
		case WM_HELP:
			return (dodialoghelp(hwnd,TEXT("base\\Index Informationstatistics.htm"),(HELPINFO *)lParam,wh_statid));
	}
	return FALSE;
}
/*******************************************************************************/
static void displaybasestats (HWND hwed, INDEX * FF)	// displays base info

{
	SYSTEMTIME st;
	FILETIME lt;
	char string1[STSTRING];
	TCHAR s1[STSTRING], s2[STSTRING], s3[2*STSTRING];
	time_t tottime;

	Edit_SetText(hwed,TEXT(""));		/* clear out the control (from any previous passes) */
	time_t tt = FF->head.createtime;
	strftime(string1,STSTRING,"Created: %A %B %d, %Y %I:%M:%S %p.\r\n",localtime(&tt));
	SendMessage(hwed,EM_REPLACESEL,FALSE,(LPARAM)toNative(string1));
	FileTimeToLocalFileTime(&FF->modtime,&lt);
	FileTimeToSystemTime(&lt,&st);
	GetDateFormat(LOCALE_SYSTEM_DEFAULT,DATE_LONGDATE,&st,NULL,s1,STSTRING);
	GetTimeFormat(LOCALE_SYSTEM_DEFAULT,LOCALE_NOUSEROVERRIDE,&st,NULL,s2,STSTRING);
	u_sprintf(s3,"Modified: %S %S.\r\n", s1, s2);
	SendMessage(hwed,EM_REPLACESEL,FALSE,(LPARAM)s3);
	tottime = FF->head.elapsed + time(NULL)-FF->lastflush;	 /* make total time */
	u_sprintf(s3, "Open for %ldhrs %ldmin.\r\n",tottime/3600, (tottime%3600)/60);
	SendMessage(hwed,EM_REPLACESEL,FALSE,(LPARAM)s3);
}
/*******************************************************************************/
BOOL ts_fontsub(INDEX * FF, FONTMAP * fm)	/* organizes font mapping */

{
	struct substruct sb;
	
	sb.FF = FF;
	sb.fm = FF ? FF->head.fm : fm;

	if (DialogBoxParam(g_hinst,MAKEINTRESOURCE(IDD_FONTSUB),g_hwframe,fsubproc,(LPARAM)&sb))	{
		if (FF && FF->vwind)	{	/* if have window */
			view_redisplay(FF,0,VD_CUR|VD_RESET);
			ComboBox_SelectString(g_hwfcb,-1,FF->head.fm[0].name);
		}
		return (TRUE);
	}
	return (FALSE);
}
/******************************************************************************/
static INT_PTR CALLBACK fsubproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)

{
	struct substruct *sbp;
	HWND hwl, hwp, hws;
	TCHAR cstring[STSTRING], dstring[STSTRING], *pptr, *sptr;
	int row, ts[2], pindex, sindex;
//	short farray[FONTLIMIT];
//	HCURSOR ocurs;

	sbp = getdata(hwnd);
	switch (msg)	{

		case WM_INITDIALOG:
			sbp = setdata(hwnd,(void *)lParam);/* set data */
			hwp = GetDlgItem(hwnd,IDC_FONTSUB_PREFERRED);
			hws = GetDlgItem(hwnd,IDC_FONTSUB_SUBS);
			ComboBox_LimitText(hwp,LF_FACESIZE);
			type_setfontnamecombo(hwnd,IDC_FONTSUB_PREFERRED,sbp->fm[0].pname);
			SetWindowText(hwp,toNative(sbp->fm[0].pname));
			type_setfontnamecombo(hwnd,IDC_FONTSUB_SUBS,sbp->fm[0].name);
			hwl = GetDlgItem(hwnd,IDC_FONTSUB_LIST);
			ts[0] = 40;
			ts[1] = 132;
			ListBox_SetTabStops(hwl,2,ts);
			for (row = 0; *sbp->fm[row].pname; row++)	{	/* for all fonts we're using */
				nstrcpy(cstring,toNative(sbp->fm[row].pname));	// copy preferred because of double call to toNative()
				setrowstring(hwl,row,cstring,toNative(sbp->fm[row].name));	/* build list */
			}
			ListBox_SetCurSel(hwl,0);
			BOOLEAN allfontsused = sbp->FF ? type_scanfonts(sbp->FF, sbp->farray) : TRUE;
			if (!sbp->FF || sbp->FF->mf.readonly || row == VOLATILEFONTS || allfontsused)	{	/* if not working on index map or no optional fonts */
				disableitem(hwnd,IDC_FONTSUB_CHECK);
//				hideitem(hwnd,IDC_FONTSUB_CHECK);
			}
			centerwindow(hwnd,1);
			return FALSE;
		case WM_COMMAND:
			hwl = GetDlgItem(hwnd,IDC_FONTSUB_LIST);	/* get list box handle */
			hwp = GetDlgItem(hwnd,IDC_FONTSUB_PREFERRED);
			hws = GetDlgItem(hwnd,IDC_FONTSUB_SUBS);
			row = ListBox_GetCurSel(hwl);
			switch (LOWORD(wParam))	{
				case IDOK:
				for (row = 0; row < FONTLIMIT; row++)	{		/* for all fonts we're using */
					memset(sbp->fm[row].pname,0,FONTNAMELEN);	/* clear preferred name */
					memset(sbp->fm[row].name,0,FONTNAMELEN);	/* clear sub name */
					if (getstringptrs(hwl,row,cstring,&pptr,&sptr))	{
						strcpy(sbp->fm[row].pname,fromNative(pptr));
						if (type_available(sbp->fm[row].pname))		/* if our preferred is one we have */
							strcpy(sbp->fm[row].name,sbp->fm[row].pname);	/* make it our sub as well */
						else if (type_available(fromNative(sptr)))		/* else if have a sub font */
							strcpy(sbp->fm[row].name,fromNative(sptr));		// set it
						else {
							senderr(ERR_FONTBADSUB,WARN,pptr);
							ListBox_SetCurSel(hwl,row);
							SendMessage(hwnd,WM_COMMAND,MAKELONG(IDC_FONTSUB_LIST,LBN_SELCHANGE),(LPARAM)hwl);	/* force display update */
							return (TRUE);
						}
					}
				}
				case IDCANCEL:
					EndDialog(hwnd,LOWORD(wParam) == IDOK ? TRUE : FALSE);
					return TRUE;
				case IDC_FONTSUB_LIST:
					if (HIWORD(wParam) == LBN_SELCHANGE)	{
						getstringptrs(hwl,row,cstring,&pptr,&sptr);
						if (ComboBox_SelectString(hws,-1,sptr) == CB_ERR)	/* if no substitute avail */
							ComboBox_SetCurSel(hws,-1);
//						if (ComboBox_SelectString(hwp,-1,pptr) == CB_ERR)	/* if not an item from list */
						if (ComboBox_SetCurSel(hwp,SendMessage(hwp,CB_FINDSTRINGEXACT,-1,(LPARAM)pptr)) == CB_ERR)	/* if not an item from list */
							SetWindowText(hwp,pptr);	// set entered text
					}
					return TRUE;
				case IDC_FONTSUB_PREFERRED:
				case IDC_FONTSUB_SUBS:
					*cstring = *dstring = '\0';
					if (HIWORD(wParam) == CBN_SELCHANGE || HIWORD(wParam) == CBN_KILLFOCUS)	{
						if (row != LB_ERR)	{
							pindex = ComboBox_GetCurSel(hwp);
							if (HIWORD(wParam) == CBN_SELCHANGE && LOWORD(wParam) == IDC_FONTSUB_PREFERRED)	{
								ComboBox_GetLBText(hwp,pindex,cstring);		/* force edit box to agree with selection */
								SetWindowText(hwp,cstring);
							}
							else
								GetWindowText(hwp,cstring,STSTRING);
							if ((sindex = ComboBox_GetCurSel(hws)) != CB_ERR)
								ComboBox_GetLBText(hwp,sindex,dstring);
							if (*cstring && *dstring)	{	/* if both ok */
								ListBox_DeleteString(hwl,row);
								setrowstring(hwl,row,cstring,dstring);
								ListBox_SetCurSel(hwl,row);
							}
						}
					}
					return (TRUE);
				case IDC_FONTSUB_CHECK:
//					ocurs = SetCursor(g_waitcurs);
//					if (!type_scanfonts(sbp->FF,farray))	{	/* if not all used */
//						SetCursor(ocurs);
//						if (sendwarning(WARN_FONTGAPS))		{
//							ocurs = SetCursor(g_waitcurs);
							type_adjustfonts(sbp->FF,sbp->farray);	/* consolidate */
							ListBox_ResetContent(hwl);
							for (row = 0; *sbp->fm[row].pname; row++)	{	/* for all fonts we're using */
								nstrcpy(cstring,toNative(sbp->fm[row].pname));	// copy preferred because of double call to toNative()
								setrowstring(hwl,row,cstring,toNative(sbp->fm[row].name));	/* build list */
							}
							ListBox_SetCurSel(hwl,0);
							disableitem(hwnd,IDCANCEL);
							disableitem(hwnd, IDC_FONTSUB_CHECK);
//						}
//					}
//					else
//						sendinfo(INFO_ALLFONTSUSED);
//					SetCursor(ocurs);
					return (TRUE);
			}
			break;
		case WM_HELP:
			return (dodialoghelp(hwnd,TEXT("base\\Fonts.htm"),(HELPINFO *)lParam,wh_fontid));
	}
	return FALSE;
}
/******************************************************************************/
static void setrowstring(HWND hwl,int row, TCHAR * pstring, TCHAR * sstring)	/* sets row string */
 
{
	TCHAR cstring[STSTRING], dstring[STSTRING];

	if (!row)
		nstrcpy(cstring, TEXT("Default"));
	else
		u_sprintf(cstring,"Font %d",row);
	u_sprintf(dstring,"%S\t%S\t%S",cstring, pstring, sstring);
	SendMessage(hwl,LB_INSERTSTRING,row,(LPARAM)dstring);		/* Cindex ID name */
}
/******************************************************************************/
static BOOL getstringptrs(HWND hwl,int row, TCHAR * cstring, TCHAR ** pstring, TCHAR ** sstring)	/* gets string ptrs */

{
	TCHAR *tptr;

	if (ListBox_GetText(hwl,row,cstring) != LB_ERR)	{
		tptr = nstrrchr(cstring,'\t');
		*sstring = tptr+1;
		*tptr = '\0';
		*pstring = nstrrchr(cstring,'\t')+1;
		return (TRUE);
	}
	return (FALSE);
}
