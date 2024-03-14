#include "stdafx.h"
#include <htmlhelp.h>
#include "commands.h"
#include "errors.h"
#include "files.h"
#include "viewset.h"
#include "index.h"
#include "formstuff.h"
#include "util.h"
#include "strings.h"
#include "tagstuff.h"
#include "registry.h"
#include "regex.h"

//#define OLDAHEADBASE (T_STRUCTBASE+4)
//#define OLDCHARBASE (T_NUMTAGS-4)
#define OLDAHEADBASE 4	// Cindex 3 T_STRUCTBASE+4
#define OLDCHARBASE 95	// Cindex 3 T_NUMTAGS-4

#define gettagsetsize(TS) (sizeof(TAGSET)+str_xlen(TS->xstr)+1)
	
static TCHAR f_codefiletypes[] = TEXT("*.cstg");
static TCHAR f_XMLfiletypes[] = TEXT("*.cxtg");

static const DWORD wh_tagsid[] = {
	IDC_TAGS_MAINDELETE, HIDC_TAGS_MAINDELETE,
	IDC_TAGS_MAINDUP, HIDC_TAGS_MAINDUP,
	IDC_TAGS_MAINEDIT, HIDC_TAGS_MAINEDIT,
	IDC_TAGS_MAINMENU, HIDC_TAGS_MAINMENU,
	IDC_TAGS_MAINNEW, HIDC_TAGS_MAINNEW,
	IDC_TAG_SETNAME, HIDC_TAG_SETNAME,
	IDC_GENERIC_DONE, HIDC_GENERIC_DONE,
	0,0
};

static const DWORD wh_tagid[] = {
	IDC_TAG_STRUCTBEG, HIDC_TAG_STRUCTBEG,
	IDC_TAG_STRUCTEND, HIDC_TAG_STRUCTEND,
	IDC_TAG_STRUCTGROUP, HIDC_TAG_STRUCTGROUP,
	IDC_TAG_STRUCTGROUPEND, HIDC_TAG_STRUCTGROUPEND,
	IDC_TAG_STRUCTAHEAD, HIDC_TAG_STRUCTAHEAD,
	IDC_TAG_STRUCTAHEADEND, HIDC_TAG_STRUCTAHEADEND,
	IDC_TAG_L1S, HIDC_TAG_L1S,
	IDC_TAG_L2S, HIDC_TAG_L1S,
	IDC_TAG_L3S, HIDC_TAG_L1S,
	IDC_TAG_L4S, HIDC_TAG_L1S,
	IDC_TAG_L5S, HIDC_TAG_L1S,
	IDC_TAG_L6S, HIDC_TAG_L1S,
	IDC_TAG_L7S, HIDC_TAG_L1S,
	IDC_TAG_L8S, HIDC_TAG_L1S,
	IDC_TAG_L9S, HIDC_TAG_L1S,
	IDC_TAG_L10S, HIDC_TAG_L1S,
	IDC_TAG_L11S, HIDC_TAG_L1S,
	IDC_TAG_L12S, HIDC_TAG_L1S,
	IDC_TAG_L13S, HIDC_TAG_L1S,
	IDC_TAG_L14S, HIDC_TAG_L1S,
	IDC_TAG_L15S, HIDC_TAG_L1S,
	IDC_TAG_L1E, HIDC_TAG_L1E,
	IDC_TAG_L2E, HIDC_TAG_L1E,
	IDC_TAG_L3E, HIDC_TAG_L1E,
	IDC_TAG_L4E, HIDC_TAG_L1E,
	IDC_TAG_L5E, HIDC_TAG_L1E,
	IDC_TAG_L6E, HIDC_TAG_L1E,
	IDC_TAG_L7E, HIDC_TAG_L1E,
	IDC_TAG_L8E, HIDC_TAG_L1E,
	IDC_TAG_L9E, HIDC_TAG_L1E,
	IDC_TAG_L10E, HIDC_TAG_L1E,
	IDC_TAG_L11E, HIDC_TAG_L1E,
	IDC_TAG_L12E, HIDC_TAG_L1E,
	IDC_TAG_L13E, HIDC_TAG_L1E,
	IDC_TAG_L14E, HIDC_TAG_L1E,
	IDC_TAG_L15E, HIDC_TAG_L1E,
	IDC_TAG_BODY, HIDC_TAG_BODY,
	IDC_TAG_PAGEREF, HIDC_TAG_PAGEREF,
	IDC_TAG_PAGEREFEND, HIDC_TAG_PAGEREFEND,
	IDC_TAG_CROSSREF, HIDC_TAG_CROSSREF,
	IDC_TAG_CROSSREFEND, HIDC_TAG_CROSSREFEND,
	IDC_TAGS_LEVELATTRIBUTE, HIDC_TAGS_LEVELATTRIBUTE,
	IDC_TAGS_NESTHEADINGS, HIDC_TAGS_NESTHEADINGS,
	IDC_TAGS_INDIVIDUALREFS, HIDC_TAGS_INDIVIDUALREFS,
	IDC_TAGS_SUPPRESSREFS, HIDC_TAGS_SUPPRESSREFS,

	IDC_TAG_BOLDON, HIDC_TAG_BOLDON,
	IDC_TAG_BOLDOFF, HIDC_TAG_BOLDOFF,
	IDC_TAG_ITALON, HIDC_TAG_BOLDON,
	IDC_TAG_ITALOFF, HIDC_TAG_BOLDOFF,
	IDC_TAG_ULINEON, HIDC_TAG_BOLDON,
	IDC_TAG_ULINEOFF, HIDC_TAG_BOLDOFF,
	IDC_TAG_SMALLON, HIDC_TAG_BOLDON,
	IDC_TAG_SMALLOFF, HIDC_TAG_BOLDOFF,
	IDC_TAG_SUPERON, HIDC_TAG_BOLDON,
	IDC_TAG_SUPEROFF, HIDC_TAG_BOLDOFF,
	IDC_TAG_SUBON, HIDC_TAG_BOLDON,
	IDC_TAG_SUBOFF, HIDC_TAG_BOLDOFF,
	IDC_TAG_BION, HIDC_TAG_BION,
	IDC_TAG_BIOFF, HIDC_TAG_BIOFF,

	IDC_FC1, HIDC_FC1,
	IDC_FCE1, HIDC_FCE1,
	IDC_FC3, HIDC_FC3,
	IDC_FCE3, HIDC_FCE3,
	IDC_FC3, HIDC_FC3,
	IDC_FCE3, HIDC_FCE3,
	IDC_FC4, HIDC_FC3,
	IDC_FCE4, HIDC_FCE3,
	IDC_FC5, HIDC_FC3,
	IDC_FCE5, HIDC_FCE3,
	IDC_FC6, HIDC_FC3,
	IDC_FCE6, HIDC_FCE3,
	IDC_FC7, HIDC_FC3,
	IDC_FCE7, HIDC_FCE3,
	IDC_FC8, HIDC_FC3,
	IDC_FCE8, HIDC_FCE3,
	IDC_FC9, HIDC_FC3,
	IDC_FCE9, HIDC_FCE3,
	IDC_FC10, HIDC_FC3,
	IDC_FCE10, HIDC_FCE3,
	IDC_FC11, HIDC_FC3,
	IDC_FCE11, HIDC_FCE3,
	IDC_FC12, HIDC_FC3,
	IDC_FCE12, HIDC_FCE3,
	IDC_FA_NONE, HIDC_FA_NONE,
	IDC_FA_ID, HIDC_FA_NONE,
	IDC_FA_NAME, HIDC_FA_NONE,
		
	IDC_ASCIIP1, HIDC_ASCIIP1,
	IDC_ASCIIC1, HIDC_ASCIIC1,
	IDC_ASCIIP2, HIDC_ASCIIP1,
	IDC_ASCIIC2, HIDC_ASCIIC1,
	IDC_ASCIIP3, HIDC_ASCIIP1,
	IDC_ASCIIC3, HIDC_ASCIIC1,
	IDC_ASCIIP4, HIDC_ASCIIP1,
	IDC_ASCIIC4, HIDC_ASCIIC1,
	IDC_ASCIIP5, HIDC_ASCIIP1,
	IDC_ASCIIC5, HIDC_ASCIIC1,
	IDC_ASCIIP6, HIDC_ASCIIP1,
	IDC_ASCIIC6, HIDC_ASCIIC1,
	IDC_ASCIIP7, HIDC_ASCIIP1,
	IDC_ASCIIC7, HIDC_ASCIIC1,
	IDC_ASCIIP8, HIDC_ASCIIP1,
	IDC_ASCIIC8, HIDC_ASCIIC1,
	IDC_TAG_UNIPREFIX, HIDC_TAG_UNIPREFIX,
	IDC_TAG_UNISUFFIX, HIDC_TAG_UNISUFFIX,
	IDC_TAG_UNIDECIMAL, HIDC_TAG_UNIDECIMAL,
	IDC_TAG_UNIHEXADECIMAL, HIDC_TAG_UNIDECIMAL,
	IDC_TAG_ENDLINE, HIDC_TAG_ENDLINE,
	IDC_TAG_ENDPARA,HIDC_TAG_ENDPARA,
	IDC_TAG_TAB, HIDC_TAG_TAB,

	IDC_TAGS_EXTENSION, HIDC_TAGS_EXTENSION,
	0,0
};

static TCHAR *t_taghelp = TEXT("base\\Markup.htm");		/* help lookup */
static 	URegularExpression * element;

struct tsname {		/* tag naming struct */
	HWND cbh;		/* handle to combo-box that contains file names */
	TCHAR * nptr;	/* name string */
	int type;
};

struct tts {		/* tag editing structure */
	TAGSET *th;
	FONTMAP * fmp;
};

static void converttags(TAGSET *ts, TCHAR * tagpath);	/* converts V1.0 tag set */
static void setcheckboxes(HWND hwnd, TAGSET * th); // sets boxes, appropriately enabled
static INT_PTR CALLBACK tmproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static void buildtagmenu(HWND hwnd, TCHAR * selname, int type);	/* builds tag menu */
static void setbuttons(HWND hwnd, int type);	/* sets buttons for current set */
static BOOL getcurrentset(HWND hwnd, TCHAR * selname, int type);	/* gets name of current set */
static BOOL edittags(HWND hwnd, TAGSET * tp, TCHAR * name, int tagtype);		/* edits tags */
static INT_PTR CALLBACK txmlstructproc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK txmlotherproc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK tstructproc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK tstyleproc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK tfontproc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK tspecialproc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static BOOL gettagname(HWND hwnd, TCHAR * name, int type);	/* gets name for tag set */
static INT_PTR CALLBACK tsnproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static BOOL savetagset(TAGSET * th, TCHAR *tagname, int type);	/* saves tagset in file */
static void deletetagset(TCHAR *tagname, int type);	/* deletes tagset file */
static void loadtags(HWND hwnd, int baseitem, char * xstr, int baseindex, int itemcount, long readflags);	/* loads items */
static int recovertags(HWND hwnd, int baseitem, char * xstr, int baseindex, int itemcount);	/* loads items */
static void checkcontents(HWND field);	// limits length of text in NSControl
static TCHAR * fulltagpath(TCHAR * tagname, int type);	// makes full path for tagset

#define maketagpath(path,tagname) file_getdefpath(path,FOLDER_CTG,tagname)
/******************************************************************************/
TCHAR * ts_getactivetagsetname(int type)	// gets active tagset name

{
	TCHAR * ns = ts_getactivetagsetpath(type);
	if (ns)	{
		PathRemoveExtension(ns);
		ns = PathFindFileName(ns);
	}
	return ns;
}
/******************************************************************************/
TCHAR * ts_getactivetagsetpath(int type)	// gets active tagset for type

{
	static TCHAR path[MAX_PATH];
	int length = MAX_PATH;
	TCHAR * ftype, *rtype;
	
	if (type == XMLTAGS)	{
		ftype = f_XMLfiletypes;
		rtype = TAG_XML;
	}
	else {
		ftype = f_codefiletypes;
		rtype = TAG_SGML;
	}
	if (!reg_getkeyvalue(K_GENERAL,rtype,path,&length) || GetFileAttributes(path) == INVALID_FILE_ATTRIBUTES)	{	// if no registry entry or no file
		WIN32_FIND_DATA data;
		HANDLE handle;

		file_makecinpath(path,ftype);
		handle = FindFirstFile(path,&data);
		if (handle != INVALID_HANDLE_VALUE)	{
			file_makecinpath(path,data.cFileName);
			FindClose(handle);
		}
		else
			return NULL;
	}
	return path;
}
/*******************************************************************************/
TAGSET * ts_openset(TCHAR * tagpath)		/* returns pointer to tag set on path */

{
	TAGSET *th = NULL;
	MFILE mf;

	if (mfile_open(&mf,tagpath,GENERIC_READ,FILE_SHARE_READ,OPEN_EXISTING,0,0))	{
		TAGSET * tth = (TAGSET *)mf.base;
		if (tth->tssize == sizeof(TAGSET) && tth->version >= 1 && tth->version <= TS_VERSION)	{		/* if compatible (can't open version 0)*/
			if (th = getmem(TAGMAXSIZE))	{		/* if can get memory */
				memcpy(th,tth,mf.size);	// copy tags
				converttags(th,tagpath);	// convert old tags as necessary
			}
		}
		mfile_close(&mf);
	}
	if (!th)
		senderr(ERR_TAGOPEN,WARN,tagpath);	/* maybe later give more specific error message */
	return (th);
}
/*******************************************************************************/
TAGSET * ts_open(TCHAR * tagname)		/* (for api) returns pointer to named tag set; and sets it as default */

{
	int type = nstrstr(nstrlwr(tagname),TEXT(".cxtg")) ? XMLTAGS : SGMLTAGS;
	TCHAR * tpath = fulltagpath(tagname,type);
	TAGSET * th = ts_openset(tpath);
	if (th) 	// if valid set
		reg_setkeyvalue(K_GENERAL,type == XMLTAGS ? TAG_XML : TAG_SGML,REG_SZ,tpath,nstrlen(tpath));
	return th;
}
/*******************************************************************************/
void ts_managetags(HWND hwnd)	/* manages tag set */

{
	DialogBox(g_hinst,MAKEINTRESOURCE(IDD_TAGS_MAIN),g_hwframe,tmproc);
}
/******************************************************************************/
static INT_PTR CALLBACK tmproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)

{
	HWND tab = GetDlgItem(hwnd,IDC_TAGS_MAINTAB);
	int tabindex = TabCtrl_GetCurSel(tab);
	TCHAR tagsetname[_MAX_FNAME+_MAX_EXT],cursetname[_MAX_FNAME+_MAX_EXT];
	char * sptr;
	TAGSET *th;
	TC_ITEM tc;

	switch (msg)	{
		case WM_INITDIALOG:
			// element name must start with letter or underscore; can't start with 'xml'; only other permissible chars are . and -
			element = regex_build("^[^A-Za-z_]|[^A-Za-z0-9_.-]", 0);	// build xml checker
			TabCtrl_DeleteAllItems(tab);
			memset(&tc,0,sizeof(tc));
			tc.mask = TCIF_TEXT;
			tc.pszText = TEXT("XML Tags");
			TabCtrl_InsertItem(tab,0,&tc);
			tc.pszText = TEXT("SGML Tags");
			TabCtrl_InsertItem(tab,1,&tc);
			buildtagmenu(hwnd,TEXT(""),0);
			centerwindow(hwnd,1);	
			return FALSE;
		case WM_COMMAND:
			switch (LOWORD(wParam))	{
				case IDC_GENERIC_DONE:
				case IDOK:
				case IDCANCEL:
					uregex_close(element);
					EndDialog(hwnd,TRUE);
					return TRUE;
				case IDC_TAGS_MAINMENU:
					if (HIWORD(wParam) == CBN_SELCHANGE)	{	/* if selection changed */
						TCHAR * tpath;
						getcurrentset(hwnd,cursetname,tabindex);
						tpath = fulltagpath(cursetname,tabindex);
						reg_setkeyvalue(K_GENERAL,tabindex == XMLTAGS ? TAG_XML : TAG_SGML,REG_SZ,tpath,nstrlen(tpath));
						setbuttons(hwnd, tabindex);
					}
					return TRUE;
				case IDC_TAGS_MAINNEW:
					if (gettagname(hwnd,tagsetname,tabindex))	{
						if (th = getmem(TAGMAXSIZE))	{	/* if can get a handle */
							sptr = str_xatindex(th->xstr,T_NUMTAGS);
							*sptr = EOCS;			/* terminate x string */
							if (edittags(hwnd,th,tagsetname,tabindex))	/* edits and saves changes if ok */
								buildtagmenu(hwnd,tagsetname,tabindex);
							freemem(th);
						}
					}
					return (TRUE);
				case IDC_TAGS_MAINEDIT:
					getcurrentset(hwnd,cursetname,tabindex);
					if (th = ts_openset(fulltagpath(cursetname,tabindex)))	{
						edittags(hwnd,th,cursetname,tabindex);	/* edits and saves changes if ok */
						freemem(th);
					}
					return (TRUE);
				case IDC_TAGS_MAINDUP:
					if (gettagname(hwnd, tagsetname,tabindex))	{		/* if want new name */
						getcurrentset(hwnd,cursetname,tabindex);	/* get name of current set */
						if (th = ts_openset(fulltagpath(cursetname,tabindex)))	{	/* load the set */
							th->readonly = FALSE;	/* allow readable */
							if (edittags(hwnd,th,tagsetname,tabindex))	/* edits and saves changes if ok */
								buildtagmenu(hwnd,tagsetname,tabindex);	/* rebuild menu */
							freemem(th);
						}
					}
					return (TRUE);
				case IDC_TAGS_MAINDELETE:
					getcurrentset(hwnd,cursetname,tabindex);	/* get name of current set */
					if (sendwarning(WARN_DELTAGSET, cursetname))	{	/* if really want to delete tag set */
						getcurrentset(hwnd,cursetname,tabindex);	/* get name of curent set */
						deletetagset(cursetname,tabindex);
						buildtagmenu(hwnd,TEXT(""),tabindex);	/* rebuild menu */
					}
					return (TRUE);
			}
			break;
		case WM_NOTIFY:
			switch (LOWORD(wParam))	{
				case IDC_TAGS_MAINTAB:
					if (((LPNMHDR)lParam)->code == TCN_SELCHANGE)	{
						tabindex = TabCtrl_GetCurSel(tab);
						buildtagmenu(hwnd,TEXT(""),tabindex);	/* rebuild menu */
						return TRUE;
					}
			}
			break;
		case WM_HELP:
			return (dodialoghelp(hwnd,t_taghelp,(HELPINFO *)lParam,wh_tagsid));
	}
	return (FALSE);
}
/*******************************************************************************/
static void buildtagmenu(HWND hwnd, TCHAR * setname, int type)	/* builds tag set menu */

{
	TCHAR * ftype, *rtype;
	TCHAR path[MAX_PATH];
	int length = MAX_PATH;
	HWND cbh;
	int index;

	if (type == XMLTAGS)	{
		rtype = TAG_XML;
		ftype = f_XMLfiletypes;
	}
	else {
		rtype = TAG_SGML;
		ftype = f_codefiletypes;
	}
	cbh = GetDlgItem(hwnd,IDC_TAGS_MAINMENU);
	ComboBox_ResetContent(cbh);				/* clear existing list */
	file_makecinpath(path,ftype);

	// NB: should check that tagest is valid before adding it to menu
	combo_addfiles(cbh,path);	// files from cindex folder
	maketagpath(path,ftype);
	combo_addfiles(cbh,path);	// add files from user folder
	if (*setname)	{		// if setting name
		TCHAR * tpath = fulltagpath(setname,type);	// make it default
		reg_setkeyvalue(K_GENERAL,rtype,REG_SZ,tpath,nstrlen(tpath));
	}
	else if (reg_getkeyvalue(K_GENERAL,rtype,path,&length))	{	// if have default
		PathRemoveExtension(path);
		setname = PathFindFileName(path);
	}
	if ((index = ComboBox_FindStringExact(cbh,-1,setname)) >= 0)
		ComboBox_SetCurSel(cbh,index);
	else
		ComboBox_SetCurSel(cbh,0);
	setbuttons(hwnd,type);
}
/*******************************************************************************/
static void setbuttons(HWND hwnd, int type)	/* sets buttons for current set */

{
	BOOL readonly;
	TCHAR selname[_MAX_FNAME+_MAX_EXT];

	readonly = getcurrentset(hwnd,selname,type);
	if (readonly || !*selname)	{	/* if protected or dud set */
		disableitem(hwnd,IDC_TAGS_MAINDELETE);
		if (*selname)	{	/* if have a set */
			enableitem(hwnd,IDC_TAGS_MAINDUP);
			enableitem(hwnd,IDC_TAGS_MAINEDIT);
			SetWindowText(GetDlgItem(hwnd,IDC_TAGS_MAINEDIT),TEXT("&View"));
		}
		else	{	/* bad set */
			disableitem(hwnd,IDC_TAGS_MAINDUP);
			disableitem(hwnd,IDC_TAGS_MAINEDIT);
		}
	}
	else {
		SetWindowText(GetDlgItem(hwnd,IDC_TAGS_MAINEDIT),TEXT("&Edit"));
		enableitem(hwnd,IDC_TAGS_MAINEDIT);
		enableitem(hwnd,IDC_TAGS_MAINDELETE);
		enableitem(hwnd,IDC_TAGS_MAINDUP);
	}
}
/*******************************************************************************/
static BOOL getcurrentset(HWND hwnd, TCHAR * selname, int type)	/* gets name of current set */

{
	HWND cbh;
	int index;
	TAGSET * th;
	BOOL readonly;

	readonly = FALSE;
	cbh = GetDlgItem(hwnd,IDC_TAGS_MAINMENU);
	if ((index = ComboBox_GetCurSel(cbh)) >= 0)	{	/* if have any item selected in list */
		ComboBox_GetLBText(cbh,index,selname);
		if (th = ts_openset(fulltagpath(selname,type)))	{		/* open to check readonly status */
			readonly = th->readonly;
			freemem(th);
		}
	}
	else
		*selname = '\0';
	return (readonly);
}
/**********************************************************************************/	
static BOOL edittags(HWND hwnd, TAGSET * tp, TCHAR * name, int tagtype)		/* edits tags */

{
#define NPAGES 4
#define NXPAGES 2

	TCHAR tagsetname[_MAX_FNAME+_MAX_EXT];
	PROPSHEETHEADER psh;
	PROPSHEETPAGE psp[NPAGES];
	int count;
	struct tts tsp;		/* wrapper structure */
	INDEX * FF;

	u_sprintf(tagsetname,"Markup Tags: %S",name);
	FF = index_front();
	tsp.th = tp;		/* pointer to tag set */
	tsp.fmp = FF ? FF->head.fm : g_prefs.gen.fm;
	memset(psp,0,sizeof(psp));
	if (tagtype == XMLTAGS)	{
		for (count = 0; count < NXPAGES; count++)	{
			psp[count].dwSize = sizeof(PROPSHEETPAGE);
			psp[count].dwFlags = PSP_HASHELP;
			psp[count].hInstance = g_hinst;
			psp[count].pszTemplate = MAKEINTRESOURCE(count+IDD_TAGS_XMLSTRUCT);
			psp[count].lParam = (LONG_PTR)&tsp;
		}
		psp[0].pfnDlgProc = txmlstructproc;
		psp[1].pfnDlgProc = txmlotherproc;
	}
	else {
		for (count = 0; count < NPAGES; count++)	{
			psp[count].dwSize = sizeof(PROPSHEETPAGE);
			psp[count].dwFlags = PSP_HASHELP;
			psp[count].hInstance = g_hinst;
			psp[count].pszTemplate = MAKEINTRESOURCE(count+IDD_TAGS_STRUCT);
			psp[count].lParam = (LONG_PTR)&tsp;
		}
		psp[0].pfnDlgProc = tstructproc;
		psp[1].pfnDlgProc = tstyleproc;
		psp[2].pfnDlgProc = tfontproc;
		psp[3].pfnDlgProc = tspecialproc;
	}
	memset(&psh,0,sizeof(psh));
	psh.dwSize = sizeof(psh);
	psh.dwFlags = PSH_PROPSHEETPAGE|PSH_HASHELP|PSH_NOAPPLYNOW;
	psh.hwndParent = hwnd;
	psh.hInstance = g_hinst;
	psh.pszCaption = tagsetname;
	psh.nPages = NPAGES;
	psh.ppsp = psp;

	if (PropertySheet(&psh) && !tp->readonly)		/* ok and not read only */
		return savetagset(tp,name, tagtype);
	return FALSE;
}
/**********************************************************************************/
static void setcheckboxes(HWND hwnd, TAGSET * th) // sets boxes, appropriately enabled

{
	checkitem(hwnd,IDC_TAGS_NESTHEADINGS,th->nested);	/* ref suppression */
	checkitem(hwnd,IDC_TAGS_SUPPRESSREFS,th->suppress);	/* ref suppression */
	checkitem(hwnd,IDC_TAGS_INDIVIDUALREFS,th->individualrefs);	/* ref suppression */
	if (th->readonly)	{
		disableitem(hwnd,IDC_TAGS_NESTHEADINGS);
		disableitem(hwnd,IDC_TAGS_SUPPRESSREFS);
		disableitem(hwnd,IDC_TAGS_INDIVIDUALREFS);
	}
}
/**********************************************************************************/	
static INT_PTR CALLBACK txmlstructproc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)

{
	struct tts * tsp = getdata(hwnd);

	switch (msg)	{
		case WM_INITDIALOG:
			if (tsp = setdata(hwnd,(void *)((LPPROPSHEETPAGE)lParam)->lParam))	{	/* set data */
				EnableWindow(hwnd, !tsp->th->readonly);
				loadtags(hwnd,IDC_TAG_STRUCTBEG+STR_BEGIN,tsp->th->xstr,T_STRUCTBASE+STR_BEGIN,1, tsp->th->readonly);
				loadtags(hwnd,IDC_TAG_STRUCTBEG+STR_GROUP,tsp->th->xstr,T_STRUCTBASE+STR_GROUP,1, tsp->th->readonly);
				loadtags(hwnd,IDC_TAG_STRUCTBEG+STR_AHEAD,tsp->th->xstr,T_STRUCTBASE+STR_AHEAD,1, tsp->th->readonly);
				loadtags(hwnd,IDC_TAG_STRUCTBEG+STR_MAIN,tsp->th->xstr,T_STRUCTBASE+STR_MAIN,15, tsp->th->readonly);
				loadtags(hwnd,IDC_TAG_STRUCTBEG+STR_PAGE,tsp->th->xstr,T_STRUCTBASE+STR_PAGE,1, tsp->th->readonly);
				loadtags(hwnd,IDC_TAG_STRUCTBEG+STR_CROSS,tsp->th->xstr,T_STRUCTBASE+STR_CROSS,1, tsp->th->readonly);
				loadtags(hwnd, IDC_ASCIIP1+OT_STARTTEXT, tsp->th->xstr, T_OTHERBASE+OT_STARTTEXT, 1, tsp->th->readonly);
				setcheckboxes(hwnd,tsp->th);	// sets boxes, appropriately enabled
				checkitem(hwnd,IDC_TAGS_LEVELATTRIBUTE,tsp->th->levelmode);	/* ref suppression */
//				if (tsp->th->readonly)
//					disableitem(hwnd,IDC_TAGS_LEVELATTRIBUTE);
				centerwindow(GetParent(hwnd),0);		/* need to center whole prop sheet at this stage (when init first page) */
			}
			return (TRUE);
		case WM_COMMAND:
			if (HIWORD(wParam) == EN_UPDATE) {
				WORD id = LOWORD(wParam);	// item id

				checkcontents((HWND)lParam);
				return TRUE;
			}
			return FALSE;
		case WM_NOTIFY:
			switch (((NMHDR *)lParam)->code)	{
				case PSN_KILLACTIVE:
					if (!recovertags(hwnd,IDC_TAG_STRUCTBEG+STR_BEGIN,tsp->th->xstr,T_STRUCTBASE+STR_BEGIN,1))
						SetWindowLongPtr(hwnd, DWLP_MSGRESULT,TRUE);	/* forbid switch */
					if (!recovertags(hwnd,IDC_TAG_STRUCTBEG+STR_GROUP,tsp->th->xstr,T_STRUCTBASE+STR_GROUP,1))
						SetWindowLongPtr(hwnd, DWLP_MSGRESULT,TRUE);	/* forbid switch */
					if (!recovertags(hwnd,IDC_TAG_STRUCTBEG+STR_AHEAD,tsp->th->xstr,T_STRUCTBASE+STR_AHEAD,1))
						SetWindowLongPtr(hwnd, DWLP_MSGRESULT,TRUE);	/* forbid switch */
					if (!recovertags(hwnd,IDC_TAG_STRUCTBEG+STR_MAIN,tsp->th->xstr,T_STRUCTBASE+STR_MAIN,15))
						SetWindowLongPtr(hwnd, DWLP_MSGRESULT,TRUE);	/* forbid switch */
					if (!recovertags(hwnd,IDC_TAG_STRUCTBEG+STR_PAGE,tsp->th->xstr,T_STRUCTBASE+STR_PAGE,1))
						SetWindowLongPtr(hwnd, DWLP_MSGRESULT,TRUE);	/* forbid switch */
					if (!recovertags(hwnd,IDC_TAG_STRUCTBEG+STR_CROSS,tsp->th->xstr,T_STRUCTBASE+STR_CROSS,1))
						SetWindowLongPtr(hwnd, DWLP_MSGRESULT,TRUE);	/* forbid switch */
					recovertags(hwnd, IDC_ASCIIP1+OT_STARTTEXT, tsp->th->xstr, T_OTHERBASE+OT_STARTTEXT, 1);
					tsp->th->levelmode = (char)isitemchecked(hwnd,IDC_TAGS_LEVELATTRIBUTE);
					tsp->th->nested = (char)isitemchecked(hwnd,IDC_TAGS_NESTHEADINGS);
					tsp->th->suppress = (char)isitemchecked(hwnd,IDC_TAGS_SUPPRESSREFS);
					tsp->th->individualrefs = (char)isitemchecked(hwnd,IDC_TAGS_INDIVIDUALREFS);
					return (TRUE);		/* check our results; SetwindowLong TRUE to hold page; FALSE to free */
				case PSN_HELP:
					dowindowhelp(t_taghelp);
					return (TRUE);
			}
			break;
		case WM_HELP:
			return (dodialoghelp(hwnd,t_taghelp,(HELPINFO *)lParam,wh_tagid));
	}
	return (FALSE);
}
/**********************************************************************************/	
static INT_PTR CALLBACK txmlotherproc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)

{
	struct tts * tsp = getdata(hwnd);
	int index;

	switch (msg)	{
		case WM_INITDIALOG:
			if (tsp = setdata(hwnd,(void *)((LPPROPSHEETPAGE)lParam)->lParam))	{	/* set data */
				EnableWindow(hwnd, !tsp->th->readonly);
				for (index = 0; index < T_STYLECOUNT; index += 2)
					loadtags(hwnd,IDC_TAG_BOLDON+index,tsp->th->xstr,T_STYLEBASE+index,1, tsp->th->readonly);
				for (index = 0; index < T_FONTCOUNT; index += 2)
					loadtags(hwnd,IDC_FC1+index,tsp->th->xstr,T_FONTBASE+index,1, tsp->th->readonly);
				CheckRadioButton(hwnd,IDC_FA_NONE,IDC_FA_NAME,IDC_FA_NONE+tsp->th->fontmode);
//				if (tsp->th->readonly)	{
//					disableitem(hwnd,IDC_FA_NONE);
//					disableitem(hwnd,IDC_FA_ID);
//					disableitem(hwnd,IDC_FA_NAME);
//				}
			}
			return (TRUE);
		case WM_COMMAND:
			if (HIWORD(wParam) == EN_UPDATE) {
				WORD id = LOWORD(wParam);	// item id

				checkcontents((HWND)lParam);
				return TRUE;
			}
			return FALSE;
		case WM_NOTIFY:
			switch (((NMHDR *)lParam)->code)	{
				case PSN_KILLACTIVE:
					for (index = 0; index < T_STYLECOUNT; index += 2)	{
						if (!recovertags(hwnd,IDC_TAG_BOLDON+index,tsp->th->xstr,T_STYLEBASE+index,1))
							SetWindowLongPtr(hwnd, DWLP_MSGRESULT,TRUE);	/* forbid switch */
					}
					for (index = 0; index < T_FONTCOUNT; index += 2)	{
						if (!recovertags(hwnd,IDC_FC1+index,tsp->th->xstr,T_FONTBASE+index,1))
							SetWindowLongPtr(hwnd, DWLP_MSGRESULT,TRUE);	/* forbid switch */
					}
					tsp->th->fontmode = findgroupcheck(hwnd,IDC_FA_NONE,IDC_FA_NAME)-IDC_FA_NONE;
					return (TRUE);		/* check our results; SetwindowLong TRUE to hold page; FALSE to free */
				case PSN_HELP:
					dowindowhelp(t_taghelp);
					return (TRUE);
			}
			break;
		case WM_HELP:
			return (dodialoghelp(hwnd,t_taghelp,(HELPINFO *)lParam,wh_tagid));
	}
	return (FALSE);
}
/**********************************************************************************/	
static INT_PTR CALLBACK tstructproc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)

{
	struct tts * tsp = getdata(hwnd);

	switch (msg)	{

		case WM_INITDIALOG:
			if (tsp = setdata(hwnd,(void *)((LPPROPSHEETPAGE)lParam)->lParam))	{	/* set data */
				EnableWindow(hwnd, !tsp->th->readonly);
				loadtags(hwnd,IDC_TAG_STRUCTBEG,tsp->th->xstr,T_STRUCTBASE,T_STRUCTCOUNT, tsp->th->readonly);
				setcheckboxes(hwnd,tsp->th);	// sets boxes, appropriately enabled
				centerwindow(GetParent(hwnd),0);		/* need to center whole prop sheet at this stage (when init first page) */
			}
			return (TRUE);
		case WM_NOTIFY:
			switch (((NMHDR *)lParam)->code)	{
				case PSN_KILLACTIVE:
					if (!recovertags(hwnd,IDC_TAG_STRUCTBEG,tsp->th->xstr,T_STRUCTBASE,T_STRUCTCOUNT))
						SetWindowLongPtr(hwnd, DWLP_MSGRESULT,TRUE);	/* forbid switch */
						
					tsp->th->nested = (char)isitemchecked(hwnd,IDC_TAGS_NESTHEADINGS);
					tsp->th->suppress = (char)isitemchecked(hwnd,IDC_TAGS_SUPPRESSREFS);
					tsp->th->individualrefs = (char)isitemchecked(hwnd,IDC_TAGS_INDIVIDUALREFS);
					return (TRUE);		/* check our results; SetwindowLong TRUE to hold page; FALSE to free */
				case PSN_HELP:
					dowindowhelp(t_taghelp);
					return (TRUE);
			}
			break;
		case WM_HELP:
			return (dodialoghelp(hwnd,t_taghelp,(HELPINFO *)lParam,wh_tagid));
	}
	return (FALSE);
}
/**********************************************************************************/	
static INT_PTR CALLBACK tstyleproc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)

{
	struct tts * tsp = getdata(hwnd);

	switch (msg)	{

		case WM_INITDIALOG:
			if (tsp = setdata(hwnd, (void *)((LPPROPSHEETPAGE)lParam)->lParam)) {/* set data */
				EnableWindow(hwnd, !tsp->th->readonly);
				loadtags(hwnd, IDC_TAG_BOLDON, tsp->th->xstr, T_STYLEBASE, T_STYLECOUNT, tsp->th->readonly);
			}
			return (TRUE);
		case WM_NOTIFY:
			switch (((NMHDR *)lParam)->code)	{
				case PSN_KILLACTIVE:
					if (!recovertags(hwnd,IDC_TAG_BOLDON,tsp->th->xstr,T_STYLEBASE,T_STYLECOUNT))
						SetWindowLongPtr(hwnd, DWLP_MSGRESULT,TRUE);	/* forbid switch */
					return (TRUE);		/* check our results; SetwindowLong TRUE to hold page; FALSE to free */
				case PSN_HELP:
					dowindowhelp(t_taghelp);
					return (TRUE);
			}
			break;
		case WM_HELP:
			return (dodialoghelp(hwnd,t_taghelp,(HELPINFO *)lParam,wh_tagid));
	}
	return (FALSE);
}
/**********************************************************************************/	
static INT_PTR CALLBACK tfontproc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)

{
	struct tts *tsp = getdata(hwnd);
	int index;

	switch (msg)	{

		case WM_INITDIALOG:
			if (tsp = setdata(hwnd,(void *)((LPPROPSHEETPAGE)lParam)->lParam))	{	/* set data */
				EnableWindow(hwnd, !tsp->th->readonly);
				loadtags(hwnd,IDC_FC1,tsp->th->xstr,T_FONTBASE,T_FONTCOUNT, tsp->th->readonly);
				for (index = 0; index < T_NUMFONTS && *tsp->fmp[index].pname; index++)	{	/* for all fonts in map */
					if (index >= 1)	/* if above perpetual ids */
						setDItemText(hwnd,IDC_FP1+index, tsp->fmp[index].pname);	/* set preferred name as prompt */
				}
			}
			return (TRUE);
		case WM_NOTIFY:
			switch (((NMHDR *)lParam)->code)	{
				case PSN_KILLACTIVE:
					if (!recovertags(hwnd,IDC_FC1,tsp->th->xstr,T_FONTBASE,T_FONTCOUNT))
						SetWindowLongPtr(hwnd, DWLP_MSGRESULT,TRUE);	/* forbid switch */
					return (TRUE);
				case PSN_HELP:
					dowindowhelp(t_taghelp);
					return (TRUE);
			}
			break;
		case WM_HELP:
			return (dodialoghelp(hwnd,t_taghelp,(HELPINFO *)lParam,wh_tagid));
	}
	return (FALSE);
}
/**********************************************************************************/	
static INT_PTR CALLBACK tspecialproc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)

{
	struct tts * tsp = getdata(hwnd);
	int count;

	switch (msg)	{

		case WM_INITDIALOG:
			for (count = 0; count < MAXTSTRINGS; count++)		/* for all the ascii characters */
				Edit_LimitText(GetDlgItem(hwnd,IDC_ASCIIP1+count*2),1);	/* limit entry to 1 char */
			Edit_LimitText(GetDlgItem(hwnd,IDC_TAG_ENDPARA),FSSTRING-1);	/* limit entry to 31 chars (then add newlinestring) */
			if (tsp = setdata(hwnd,(void *)((LPPROPSHEETPAGE)lParam)->lParam))	{	/* set data */
				HWND ew;

				EnableWindow(hwnd, !tsp->th->readonly);
				loadtags(hwnd,IDC_ASCIIP1,tsp->th->xstr,T_OTHERBASE,T_OTHERCOUNT, tsp->th->readonly);
				checkitem(hwnd, IDC_TAG_UTF8, !tsp->th->useUTF8);
				CheckRadioButton(hwnd,IDC_TAG_UNIDECIMAL,IDC_TAG_UNIHEXADECIMAL,IDC_TAG_UNIDECIMAL+tsp->th->hex);
				ew = GetDlgItem(hwnd,IDC_TAGS_EXTENSION);
				Edit_LimitText(ew,3);				/* limit entry to 3 chars */
				SetWindowText(ew,toNative(tsp->th->extn));
//				if (tsp->th->readonly)	{
//					Edit_SetReadOnly(ew,TRUE);
//					disableitem(hwnd,IDC_TAGS_SUPPRESSREFS);
//				}
			}
			return (TRUE);
		case WM_NOTIFY:
			switch (((NMHDR *)lParam)->code)	{
				case PSN_KILLACTIVE:
					recovertags(hwnd,IDC_ASCIIP1,tsp->th->xstr,T_OTHERBASE,T_OTHERCOUNT);
					tsp->th->hex = findgroupcheck(hwnd,IDC_TAG_UNIDECIMAL,IDC_TAG_UNIHEXADECIMAL)-IDC_TAG_UNIDECIMAL;
					tsp->th->useUTF8 = !(char)isitemchecked(hwnd, IDC_TAG_UTF8);
					getDItemText(hwnd,IDC_TAGS_EXTENSION,tsp->th->extn,4);
					return (TRUE);		/* check our results; SetwindowLong TRUE to hold page; FALSE to free */
				case PSN_HELP:
					dowindowhelp(t_taghelp);
					return (TRUE);
			}
			break;
		case WM_COMMAND:
			if (HIWORD(wParam) == EN_UPDATE)	{
				WORD id = LOWORD(wParam);
				int length = 0;

				if (id == IDC_TAGS_EXTENSION)
					length = 4;
				if (id == IDC_ASCIIP1 || id == IDC_ASCIIP2 || id == IDC_ASCIIP3 || id == IDC_ASCIIP4
					 || id == IDC_ASCIIP5 || id == IDC_ASCIIP6 || id == IDC_ASCIIP7 || id == IDC_ASCIIP8)
					length = 2;
				checktextfield((HWND)lParam,length);
				return TRUE;
			}
			else if (LOWORD(wParam) == IDC_TAG_UTF8){
				if (isitemchecked(hwnd, IDC_TAG_UTF8)) {		// iscontrolchecked((HWND)lParam))
					enableitem(hwnd, IDC_TAG_UNIPREFIX);
					enableitem(hwnd, IDC_TAG_UNISUFFIX);
					enableitem(hwnd, IDC_TAG_UNIHEXADECIMAL);
				}
				else {
					disableitem(hwnd, IDC_TAG_UNIPREFIX);
					disableitem(hwnd, IDC_TAG_UNISUFFIX);
					disableitem(hwnd, IDC_TAG_UNIHEXADECIMAL);
				}
			}
			break;
		case WM_HELP:
			return (dodialoghelp(hwnd,t_taghelp,(HELPINFO *)lParam,wh_tagid));
	}
	return (FALSE);
}
/*******************************************************************************/
static BOOL gettagname(HWND hwnd, TCHAR * name, int type)	/* gets name for tag set */

{
	struct tsname tsn;

	tsn.cbh = GetDlgItem(hwnd,IDC_TAGS_MAINMENU);
	tsn.nptr = name;
	tsn.type = type;

	return (DialogBoxParam(g_hinst,MAKEINTRESOURCE(IDD_TAG_SETNAME),hwnd,tsnproc,(LPARAM)&tsn) > 0);
}
/******************************************************************************/
static INT_PTR CALLBACK tsnproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)

{
	struct tsname *tnp;
	HWND eh;

	tnp = getdata(hwnd);
	switch (msg)	{

		case WM_INITDIALOG:
			setdata(hwnd,(void *)lParam);/* set data */
			eh = GetDlgItem(hwnd,IDC_TAG_SETNAME);
			SendMessage(eh,EM_SETLIMITTEXT,_MAX_FNAME+_MAX_EXT-1,0);	/* limits text */
			SetFocus(eh);	/* set focus to text */
			centerwindow(hwnd,0);
			return FALSE;
		case WM_COMMAND:
			switch (LOWORD(wParam))	{
				case IDOK:
					GetDlgItemText(hwnd,IDC_TAG_SETNAME,tnp->nptr,_MAX_FNAME+_MAX_EXT);
					if (*tnp->nptr == '$')	{	// if starts with illegal $
						senderr(ERR_BADTAGSETNAME,WARN);
							return (TRUE);
					}
					if (ComboBox_FindStringExact(tnp->cbh,-1,tnp->nptr) != CB_ERR)	{	/* if need check and exists */
						if (!sendwarning(WARN_DUPTAGSET,tnp->nptr))
							return (TRUE);
					}
				case IDCANCEL:
					EndDialog(hwnd,LOWORD(wParam) == IDOK ? TRUE : FALSE);
					return TRUE;
			}
			break;
		case WM_HELP:
			return (dodialoghelp(hwnd,t_taghelp,(HELPINFO *)lParam,wh_tagsid));
	}
	return FALSE;
}
/******************************************************************************/
static TCHAR * fulltagpath(TCHAR * tagname, int type)	// makes full path for tagset

{
	static TCHAR path[MAX_PATH];

	if (*tagname == '$')
		file_makecinpath(path,tagname);
	else
		maketagpath(path,tagname);
	PathAddExtension(path,type == XMLTAGS ? f_XMLfiletypes+1 : f_codefiletypes+1);	// add extension if not already present
	return path;
}
/******************************************************************************/
static void converttags(TAGSET *ts, TCHAR * tagpath)	/* converts V1.0 tag set */

{
	char * base;

	if (ts->version < 1)	{	// if need to convert version 1 to version 2
//		long writesize, ok1, ok2;
//		HANDLE fid;
		
		base = str_xatindex(ts->xstr, OLDCHARBASE);	// set point to old character tag base
		*base++ = 0;	// append new char code tags
		*base++ = 0;
		*base = EOCS;
		base = str_xatindex(ts->xstr, OLDAHEADBASE);	// create space for new group tags
		str_xshift(base,2);
		*base++ = 0;
		*base = 0;
		ts->version = TS_VERSION;	/* set current version */
#if 0
		ts->total = gettagsetsize(ts);
		if ((fid = CreateFile(tagpath,GENERIC_READ|GENERIC_WRITE,0,NULL,OPEN_ALWAYS,FILE_FLAG_RANDOM_ACCESS,0)) != INVALID_HANDLE_VALUE)	{
			ok1 = WriteFile(fid,ts,ts->total,&writesize,NULL);
			ok2 = CloseHandle(fid);
			return (ok1 && ok2);
#endif
	}
	if (!str_xatindex(ts->xstr, T_OTHERBASE + OT_STARTTEXT)) {	// if don't have extra body string (Cindex 4)
		base = str_xlast(ts->xstr);	// set point to current last string
		base += strlen(base) + 1;		// now points to EOCS
		*base++ = 0;	// append new heading text tags
		*base++ = 0;
		*base = EOCS;
	}
//		for (index = 0, sptr = ts->xstr; *sptr != EOCS; sptr += strlen(sptr++), index++)
//			NSLog("%d:-%s",index, sptr);
}
/******************************************************************************/
static BOOL savetagset(TAGSET * th, TCHAR *tagname, int type)	/* saves tagset in file */

{
	HANDLE fid;
	TCHAR path[MAX_PATH];
	long writesize;

	maketagpath(path,tagname);
	PathAddExtension(path, type == XMLTAGS ? f_XMLfiletypes+1 : f_codefiletypes+1);	// add extension if not already present
	th->version = TS_VERSION;
	th->tssize = sizeof(TAGSET);
	th->total = gettagsetsize(th);
	if ((fid = CreateFile(path,GENERIC_READ|GENERIC_WRITE,0,NULL,CREATE_ALWAYS,0,0)) != INVALID_HANDLE_VALUE)	{
		if (WriteFile(fid,th,th->total,&writesize,NULL))	{
			if (CloseHandle(fid))
				return TRUE;
		}
	}
	senderr(ERR_TAGSAVE,WARN,tagname);
	return (FALSE);
}
/******************************************************************************/
static void deletetagset(TCHAR *tagname, int type)	/* deletes tagset file */

{
	TCHAR path[MAX_PATH];

	maketagpath(path,tagname);
	PathAddExtension(path, type == XMLTAGS ? f_XMLfiletypes+1 : f_codefiletypes+1);	// add extension if not already present
	DeleteFile(path);
}
/**********************************************************************************/	
static void loadtags(HWND hwnd, int baseitem, char * xstr, int baseindex, int itemcount, long rflags)	/* loads items */

{
	char * sptr;
	int count;
	HWND ew;

	for (sptr = str_xatindex(xstr,baseindex), count = 0; count < itemcount; count++, sptr += strlen(sptr++))	{
//		NSLog("%d: %s",baseindex+count, sptr);
		ew = GetDlgItem(hwnd,baseitem+count);
		SetWindowText(ew,toNative(sptr));
		if (rflags)
			Edit_SetReadOnly(ew,TRUE);
	}
}
/**********************************************************************************/	
static int recovertags(HWND hwnd, int baseitem, char * xstr, int baseindex, int itemcount)	/* loads items */

{
	char *sptr, tstring[STSTRING];
	int count, nlen, olen, extralen;

	for (sptr = str_xatindex(xstr,baseindex), extralen = count = 0; count < itemcount; count++, sptr += strlen(sptr++))	/* check lengths */
		extralen += getDItemText(hwnd,baseitem+count,tstring,STSTRING)-strlen(sptr);	/* add net change for each string */
	if (extralen + str_xlen(xstr) < TAGMAXSIZE-1)	{	/* if there's room */
		for (sptr = str_xatindex(xstr,baseindex), count = 0; count < itemcount; count++, sptr += strlen(sptr++))	{
			nlen = getDItemText(hwnd,baseitem+count,tstring,STSTRING);
			olen = strlen(sptr);
			str_xshift(sptr+olen,nlen-olen);	/* make right gap */
			strcpy(sptr,tstring);		/* insert new string */
		}
		return (TRUE);
	}
	senderr(ERR_TAGOVERFLOW,WARN);
	return (FALSE);
}
/***************************************************************************/
static void checkcontents(HWND field)	// strips forbidden characters

{
	TCHAR text[1024];
	int initlength = GetWindowText(field, text, 1024);
	char * curtext = fromNative(text);
	char newtext[400];
	int startpos, endpos;

	SendMessage(field, EM_GETSEL, (WPARAM)&startpos,(LPARAM)&endpos);	/* show complete */
	strcpy(newtext, curtext);
	regex_replace(element, newtext, "");	// strip all illegal chars
	if (strcmp(curtext, newtext)) {
		int index = 0;
		while (newtext[index] && curtext[index] == newtext[index])
			index++;
		SetWindowText(field, toNative(newtext));
		SendMessage(field, EM_SETSEL, index, index);	/* show complete */
	}
}
