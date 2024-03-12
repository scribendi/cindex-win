#include "stdafx.h"
#include "viewset.h"
#include "formset.h"
#include "formstuff.h"
#include "sort.h"
#include "collate.h"
#include "edit.h"
#include "strings.h"
#include "errors.h"
#include "util.h"
#include "index.h"

static const DWORD wh_margid[] = {
	IDC_MARGCOL_TOP, HIDC_MARGCOL_TOP,
	IDC_MARGCOL_BOTTOM, HIDC_MARGCOL_TOP,
	IDC_MARGCOL_LEFT, HIDC_MARGCOL_TOP,
	IDC_MARGCOL_RIGHT, HIDC_MARGCOL_TOP,
	IDC_MARGCOL_FACING, HIDC_MARGCOL_FACING,
	IDC_MARGCOL_UNIT, HIDC_MARGCOL_UNIT,
	IDC_MARGCOL_COLUMNS, HIDC_MARGCOL_COLUMNS,
	IDC_MARGCOL_GUTTER, HIDC_MARGCOL_GUTTER,
	IDC_MARGCOL_NEVER, HIDC_MARGCOL_NEVER,
	IDC_MARGCOL_NEWPAGE, HIDC_MARGCOL_NEWPAGE,
	IDC_MARGCOL_NEWCOLUMN, HIDC_MARGCOL_NEWPAGE,
	IDC_MARGCOL_APPEND, HIDC_MARGCOL_APPEND,
	IDC_MARGCOL_LEVEL, HIDC_MARGCOL_NEWPAGE,
	IDC_MARGCOL_STYLE, HIDC_MARGCOL_STYLE,
	0,0
};

static const DWORD wh_hfid[] = {
	IDC_HEADFOOT_TAB, HIDC_HEADFOOT_TAB,
	IDC_HEADFOOT_LEFT, HIDC_HEADFOOT_LEFT,
	IDC_HEADFOOT_CENTER, HIDC_HEADFOOT_CENTER,
	IDC_HEADFOOT_RIGHT, HIDC_HEADFOOT_RIGHT,
	IDC_HEADFOOT_FONT, HIDC_HEADFOOT_FONT,
	IDC_HEADFOOT_SIZE, HIDC_HEADFOOT_SIZE,
	IDC_HEADFOOT_STYLE, HIDC_HEADFOOT_STYLE,
	IDC_HEADFOOT_LONG, HIDC_HEADFOOT_LONG,
	IDC_HEADFOOT_SHORT, HIDC_HEADFOOT_LONG,
	IDC_HEADFOOT_TIME, HIDC_HEADFOOT_TIME,
	IDC_HEADFOOT_PAGENUM, HIDC_HEADFOOT_PAGENUM,
	IDC_HEADFOOT_NUMFORMAT, HIDC_HEADFOOT_NUMFORMAT,
	IDC_HEADFOOT_COPY, HIDC_HEADFOOT_COPY,
	0,0
};

static const DWORD wh_groupid[] = {
	IDC_GROUPENTRIES_FONT, HIDC_GROUPENTRIES_FONT,
	IDC_GROUPENTRIES_NUMSYM, HIDC_GROUPENTRIES_NUMSYM,
	IDC_GROUPENTRIES_STYLE, HIDC_GROUPENTRIES_STYLE,
	IDC_GROUPENTRIES_TITLE, HIDC_GROUPENTRIES_TITLE,
	IDC_GROUPENTRIES_ALLNUMBERS, HIDC_GROUPENTRIES_ALLNUMBERS,
	IDC_GROUPENTRIES_ALLSYMBOLS, HIDC_GROUPENTRIES_ALLSYMBOLS,
	IDC_GROUPENTRIES_NUMBERSANDSYMBOLS, HIDC_GROUPENTRIES_NUMBERSANDSYMBOLS,
	0,0
};

static const DWORD wh_stylayid[] = {
	IDC_STYLAY_INDENTED, HIDC_STYLAY_INDENTED,
	IDC_STYLAY_RUNIN, HIDC_STYLAY_RUNIN,
	IDC_STYLAY_RUNLEVEL, HIDC_STYLAY_RUNLEVEL,
	IDC_STYLAY_STYLEMOD, HIDC_STYLAY_STYLEMOD,
	IDC_STYLAY_COLLAPSE, HIDC_STYLAY_COLLAPSE,
	IDC_STYLAY_COLLAPSELEVEL, HIDC_STYLAY_COLLAPSE,
	IDC_STYLAY_INDENTTYPE, HIDC_STYLAY_INDENTTYPE,
	IDC_STYLAY_TAB, HIDC_STYLAY_TAB,
	IDC_STYLAY_INDUNIT, HIDC_STYLAY_INDUNIT,
	IDC_STYLAY_LEADINDENT, HIDC_STYLAY_LEADINDENT,
	IDC_STYLAY_RUNINDENT, HIDC_STYLAY_RUNINDENT,
	IDC_STYLAY_SPACING, HIDC_STYLAY_SPACING,
	IDC_STYLAY_AUTO, HIDC_STYLAY_AUTO,
	IDC_STYLAY_LSPACE, HIDC_STYLAY_LSPACE,
	IDC_STYLAY_LSPUNIT, HIDC_STYLAY_LSPUNIT,
	IDC_STYLAY_LSPMAIN, HIDC_STYLAY_LSPMAIN,
	IDC_STYLAY_LSPGROUP, HIDC_STYLAY_LSPGROUP,
	IDC_STYLAY_ADJQUOTE, HIDC_STYLAY_ADJQUOTE,
	IDC_STYLAY_ADJSTYLE, HIDC_STYLAY_ADJSTYLE,
	0,0
};

static const DWORD wh_headid[] = {
	IDC_HEADING_TAB, HIDC_HEADING_TAB,
	IDC_HEADING_FONT, HIDC_HEADING_FONT,
	IDC_HEADING_STYLE, HIDC_HEADING_STYLE,
	IDC_HEADING_LEADTEXT, HIDC_HEADING_LEADTEXT,
	IDC_HEADING_TRAILPUNCT, HIDC_HEADING_TRAILPUNCT,
	IDC_HEADING_SUPPRESS, HIDC_HEADING_SUPPRESS,
	0,0
};

static const DWORD wh_crossid[] = {
	IDC_CROSSREF_BSTYLE, HIDC_CROSSREF_BSTYLE,
	IDC_CROSSREF_PSTYLE, HIDC_CROSSREF_PSTYLE,
	IDC_CROSSREF_ORDER, HIDC_CROSSREF_ORDER,
	IDC_CROSSREF_SUPPRESS, HIDC_CROSSREF_SUPPRESS,
	IDC_CROSSREF_PREFIXSTYLECHECK, HIDC_CROSSREF_PREFIXSTYLECHECK,
	IDC_CROSSREF_TAB, HIDC_CROSSREF_TAB,
	IDC_CROSSREF_XPOSITION, HIDC_CROSSREF_XPOSITION,
	IDC_CROSSREF_XAPOSITION, HIDC_CROSSREF_XAPOSITION,
	IDC_CROSSREF_XLEAD, HIDC_CROSSREF_XLEAD,
	IDC_CROSSREF_XALEAD, HIDC_CROSSREF_XLEAD,
	IDC_CROSSREF_XTRAIL, HIDC_CROSSREF_XTRAIL,
	IDC_CROSSREF_XATRAIL, HIDC_CROSSREF_XTRAIL,
	0,0
};

static const DWORD wh_pageid[] = {
	IDC_PAGEREFS_CONNECT, HIDC_PAGEREFS_CONNECT,
	IDC_PAGEREFS_CONFLATE, HIDC_PAGEREFS_CONFLATE,
	IDC_PAGEREFS_NOABBREV, HIDC_PAGEREFS_NOABBREV,
	IDC_PAGEREFS_SINGLE, HIDC_PAGEREFS_SINGLE,
	IDC_PAGEREFS_MULTIPLE, HIDC_PAGEREFS_MULTIPLE,
	IDC_PAGEREFS_AFTER, HIDC_PAGEREFS_AFTER,
	IDC_PAGEREFS_JUSTIFY, HIDC_PAGEREFS_JUSTIFY,
	IDC_PAGEREFS_LEADER, HIDC_PAGEREFS_JUSTIFY,
	IDC_PAGEREFS_SUPPRESS, HIDC_PAGEREFS_SUPPRESS,
	IDC_PAGEREFS_SUPPRESSTO, HIDC_PAGEREFS_SUPPRESSTO,
	IDC_PAGEREFS_CONCAT, HIDC_PAGEREFS_CONCAT,
	IDC_PAGEREFS_SORT, HIDC_PAGEREFS_SORT,
	IDC_PAGEREFS_SUPPRESSALL, HIDC_PAGEREFS_SUPPRESSALL,
	IDC_PAGEREFS_HIDEDUPLICATES, HIDC_PAGEREFS_HIDEDUPLICATES,
	IDC_PAGEREFS_STYLE, HIDC_PAGEREFS_STYLE,
	0,0
};

static const DWORD wh_sstringid[] = {
	IDC_SS_LIST, HIDC_SS_LIST,
	IDC_SS_BOLD,HIDC_SS_BOLD,
	IDC_SS_ITALIC,HIDC_SS_BOLD,
	IDC_SS_ULINE,HIDC_SS_BOLD,
	IDC_SS_SMALL,HIDC_SS_BOLD,
	IDC_SS_NORMAL,HIDC_SS_NORMAL,
	IDC_SS_SUPER,HIDC_SS_NORMAL,
	IDC_SS_SUB,HIDC_SS_NORMAL,
	IDC_SS_ADD,HIDC_SS_ADD,
	IDC_SS_DELETE, HIDC_SS_DELETE,
	0,0
};

static const DWORD wh_tstyleid[] = {
	IDC_TEXTSTYLE_BOLD, HIDC_TEXTSTYLE_BOLD,
	IDC_TEXTSTYLE_ITALIC,HIDC_TEXTSTYLE_BOLD,
	IDC_TEXTSTYLE_ULINE,HIDC_TEXTSTYLE_BOLD,
	IDC_TEXTSTYLE_SMALL,HIDC_TEXTSTYLE_BOLD,
	IDC_TEXTSTYLE_NCAP, HIDC_TEXTSTYLE_NCAP,
	IDC_TEXTSTYLE_ALLCAP,HIDC_TEXTSTYLE_NCAP,
	IDC_TEXTSTYLE_ICAP,HIDC_TEXTSTYLE_NCAP,
	IDC_TEXTSTYLE_AUTO,HIDC_TEXTSTYLE_AUTO,
	0,0
};

static const DWORD wh_locstyleid[] = {
	IDC_LOCATORSTYLE_TAB, HIDC_LOCATORSTYLE_TAB,
	IDC_TEXTSTYLE_BOLD, HIDC_TEXTSTYLE_BOLD,
	IDC_TEXTSTYLE_ITALIC,HIDC_TEXTSTYLE_BOLD,
	IDC_TEXTSTYLE_ULINE,HIDC_TEXTSTYLE_BOLD,
	IDC_TEXTSTYLE_SMALL,HIDC_TEXTSTYLE_BOLD,
	IDC_LOCATORSTYLE_BOLD, HIDC_TEXTSTYLE_BOLD,
	IDC_LOCATORSTYLE_ITALIC,HIDC_TEXTSTYLE_BOLD,
	IDC_LOCATORSTYLE_ULINE,HIDC_TEXTSTYLE_BOLD,
	IDC_LOCATORSTYLE_SMALL,HIDC_TEXTSTYLE_BOLD,
	0,0
};

struct mcgrp {		/* margin & column composite */
	MARGINCOLUMN mc;
	INDEXPARAMS *ip;
	PAPERINFO pi;
	int eunit;
};

struct slgroup {
	FORMATPARAMS fp;
	INDEXPARAMS *ip;
	long lspace, emspace;
	short dirtylead,dirtyrun,dirtyline;
};

struct headgrp {
	ENTRYFORMAT ef;
	INDEXPARAMS * ip;
};

struct locgroup {
	LOCATORFORMAT lf;
	HFONT fh;
};

struct stylewrapper {
	CSTYLE cs;
	int extramode;
};

static TCHAR *bnames[] = {TEXT("Header"),TEXT("Footer")};
static TCHAR *fnames[] = {TEXT("Left Header"),TEXT("Left Footer"), TEXT("Right Header"), TEXT("Right Footer")};

static TCHAR *ns[] = {
	TEXT("[@] [#] [$] [%]... [1] [2] [3] [4]..."),
	TEXT("[@#$%...] [1] [2] [3] [4]..."),
	TEXT("[@] [#] [$] [%]... [1234...]"),
	TEXT("[@#$%...1234...]"),
	TEXT("[@#$%...] [1234...]")
};
#define GP_NSSIZE (sizeof(ns)/sizeof(char *))

static TCHAR *mods[] = {
	TEXT("Standard Layout"),
	TEXT("Run-back Subheadings"),
	TEXT("Indent Major Subheadings"),
	TEXT("Repeat all Headings")
};
#define SL_MSIZE (sizeof(mods)/sizeof(char *))

static TCHAR *itype[] = {
	TEXT("None"),
	TEXT("Incremental"),
	TEXT("Fixed"),
	TEXT("Special Last"),
};
#define SL_ITSIZE (sizeof(itype)/sizeof(char *))

static TCHAR *spopt[] = {
	TEXT("Single"),
	TEXT("1.5 Lines"),
	TEXT("Double"),
};
#define SL_SPSIZE (sizeof(spopt)/sizeof(char *))

static TCHAR *cname[] = {TEXT("From Main Heading"), TEXT("From Subheading")};
#define CR_NSIZE (sizeof(cname)/sizeof(char *))

static TCHAR *xpos[] = {TEXT("Heading"), TEXT("Subheading")};
#define CR_XPSIZE (sizeof(xpos)/sizeof(char *))

static TCHAR *xapos[] = {TEXT("Heading (always)"), TEXT("Heading (no page)"), TEXT("First Subheading"), TEXT("Last Subheading (always)"), TEXT("Last Subheading (conditional)")};
#define CR_XAPSIZE (sizeof(xapos)/sizeof(char *))

static TCHAR *pnum[] = {TEXT("1, 2, 3"), TEXT("i, ii, iii"), TEXT("I, II, III")};
#define CR_NUMSIZE (sizeof(pnum)/sizeof(char *))

static TCHAR *pcon[] = {TEXT("None"), TEXT("First"), TEXT("Second"), TEXT("Third"), TEXT("Fourth"), TEXT("Fifth"), TEXT("Sixth"), TEXT("Seventh"), TEXT("Eighth")};
#define PR_CSIZE (sizeof(pcon)/sizeof(char *))

static TCHAR *pabbrev[] = {TEXT("None"), TEXT("Chicago"), TEXT("Hart"), TEXT("Full")};
#define PR_ABSIZE (sizeof(pabbrev)/sizeof(char *))

static TCHAR *lsname[] = {TEXT("First"), TEXT("Second"), TEXT("Third"), TEXT("Fourth"), TEXT("Fifth"), TEXT("Sixth"), TEXT("Seventh"), TEXT("Eighth"), TEXT("Ninth"), TEXT("Tenth")};
#define LS_NSIZE (sizeof(lsname)/sizeof(char *))


static INT_PTR CALLBACK fontproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK mcproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static void setfacing(HWND hwnd, int reflect);	/* sets titles for facing pages */
static int testmargins(MARGINCOLUMN * mcp, PAPERINFO * pi);	/* tests margin settings against page settings */
static INT_PTR CALLBACK hfproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static void settabitems(HWND hwnd,HWND th);		/* sets tab items */
static void gettabitems(HWND hwnd,HWND th);		/* sets tab items */
static HEADERFOOTER * gethfp(HWND th);		/* gets pointer to currrent headfoot struct  */
static void checkleftright(HWND hwnd, PAGEFORMAT * pfp);	/* checks left/right diffs */
static short compheadfoot(HEADERFOOTER * h1, HEADERFOOTER * h2);	/* compares contents of headfoot structs */
static INT_PTR CALLBACK grpproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK stlproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static void buildstyleoptionsmenu(HWND hwnd, int stylemod);	/* builds & sets style options menu */
static void setlinespace(HWND hwnd, struct slgroup *slp);	/* sets line spacing */
static BOOL getlinespace(HWND hwnd, struct slgroup *slp);	/* gets line spacing */
static void builditabset(HWND hwnd, struct slgroup *slp);	/* builds tab set for current indent scheme */
static void setitabitems(HWND hwnd,struct slgroup *slp);		/* sets tab items */
static BOOL getitabitems(HWND hwnd,struct slgroup *slp);		/* gets tab items */
static void switchunit(HWND hwnd,struct slgroup *slp);		/* does em-space conversions */
static void getcurrenttabpointers(HWND hwnd, INDEXPARAMS *ip,FORMATPARAMS *fp, char **mp, float **lp, float ** rp);	/* sets font in combo box */
static INT_PTR CALLBACK headproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static void sethtabitems(HWND hwnd, HWND th,ENTRYFORMAT *efp);	/* sets items for heading tab */
static void gethtabitems(HWND hwnd, HWND th,ENTRYFORMAT *efp);	/* gets items for heading tab */
static INT_PTR CALLBACK crefproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static void setctabs(HWND hwnd, HWND th,CROSSREFFORMAT *cfp);	/* sets cross-ref tab items */
static void getctabs(HWND hwnd, HWND th,CROSSREFFORMAT *cfp);	/* gets cross-ref tab items */
static INT_PTR CALLBACK prefproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static void getlocatorstyle(HWND hwnd,LSTYLE *lsp);	/* gets style settings */
static INT_PTR CALLBACK lstyleproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static void setltabs(HWND hwnd, HWND th,LSTYLE *sp);	/* sets page ref style tab items */
static void getltabs(HWND hwnd, HWND th,LSTYLE *sp);	/* gets page ref style tab items */
static INT_PTR CALLBACK ssproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static BOOL recoverstring(HWND lhw, char * text);	// recovers text from
static void setsstyle(HWND hwnd, HWND th);		/* sets styles for styled string */
static void getsstyle(HWND hwnd, HWND th);	/* retrieves styles for styled string */
static void setfontcombo(HWND cbh, char * fname);	/* sets font in combo box */
static void getfontcombo(HWND cbh, char * fname);	/* gets font from combo box */
static void getstyle(HWND hwnd,CSTYLE *stp, int extramode);		/* gets style settings */
static INT_PTR CALLBACK styleproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

/*******************************************************************************/
void fs_margcol(HWND hwnd)	/* sets up margins & colums */

{
	INDEX * FF;
	struct mcgrp mcc;

	if (hwnd)	{
		FF = WX(hwnd,owner);
		mcc.mc = FF->head.formpars.pf.mc;
		mcc.ip = &FF->head.indexpars;
		mcc.eunit = FF->head.privpars.eunit;
		mcc.pi = FF->head.formpars.pf.pi;
	}
	else {
		mcc.mc = g_prefs.formpars.pf.mc;
		mcc.ip = &g_prefs.indexpars;
		mcc.eunit = g_prefs.privpars.eunit;
		mcc.pi = g_prefs.formpars.pf.pi;
	}
	
	if (DialogBoxParam(g_hinst,MAKEINTRESOURCE(IDD_MARGCOL),g_hwframe,mcproc,(LPARAM)&mcc))	{
		if (hwnd)	{
			FF->head.formpars.pf.mc = mcc.mc;
			FF->head.privpars.eunit = mcc.eunit;
			view_changedisplaysize(hwnd);/* reset display dimensions */
			index_markdirty(FF);
		}
		else	{
			g_prefs.formpars.pf.mc = mcc.mc;
			g_prefs.privpars.eunit = mcc.eunit;
		}
	}
}
/******************************************************************************/
static INT_PTR CALLBACK mcproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)

{
	struct mcgrp * mcptr;
	int count, err;
	char tstring[STSTRING];
	float tval;
	HWND lbh;

	mcptr = getdata(hwnd);
	switch (msg)	{

		case WM_INITDIALOG:
			if (mcptr = setdata(hwnd,(void *)lParam))	{	/* set data */
				setfacing(hwnd,mcptr->mc.reflect);
				setfloat(hwnd,IDC_MARGCOL_TOP,env_toexpress(mcptr->eunit,mcptr->mc.top));
				setfloat(hwnd,IDC_MARGCOL_BOTTOM,env_toexpress(mcptr->eunit,mcptr->mc.bottom));
				setfloat(hwnd,IDC_MARGCOL_LEFT,env_toexpress(mcptr->eunit,mcptr->mc.left));
				setfloat(hwnd,IDC_MARGCOL_RIGHT,env_toexpress(mcptr->eunit,mcptr->mc.right));
				setfloat(hwnd,IDC_MARGCOL_GUTTER,env_toexpress(mcptr->eunit,mcptr->mc.gutter));
				lbh = GetDlgItem(hwnd,IDC_MARGCOL_COLUMNS);
				for (count = 0; count < MAXCOLS; count++)	{
					_itoa(count+1,tstring,10);
					ComboBox_AddString(lbh,toNative(tstring));
				}
				ComboBox_SetCurSel(lbh,mcptr->mc.ncols-1);
				checkitem(hwnd,IDC_MARGCOL_FACING,mcptr->mc.reflect);
				buildunitmenu(GetDlgItem(hwnd,IDC_MARGCOL_UNIT),mcptr->eunit);
				CheckRadioButton(hwnd,IDC_MARGCOL_NEVER,IDC_MARGCOL_NEWCOLUMN,IDC_MARGCOL_NEVER+mcptr->mc.pgcont);
				buildheadingmenu(GetDlgItem(hwnd,IDC_MARGCOL_LEVEL),mcptr->ip,mcptr->mc.clevel);
				Edit_LimitText(GetDlgItem(hwnd,IDC_MARGCOL_APPEND),FSSTRING-1);
				type_settextboxfont(GetDlgItem(hwnd,IDC_MARGCOL_APPEND));
				setDItemText(hwnd,IDC_MARGCOL_APPEND,mcptr->mc.continued);
				centerwindow(hwnd,1);
			}
			return FALSE;
		case WM_COMMAND:
			if (HIWORD(wParam) == EN_UPDATE)	{
				WORD id = LOWORD(wParam);
				int length = 0;

				if (id == IDC_MARGCOL_APPEND)
					length = FSSTRING;
				checktextfield((HWND)lParam,length);
				return TRUE;
			}
			switch (LOWORD(wParam))	{
				case IDOK:
					mcptr->mc.reflect = (short)isitemchecked(hwnd,IDC_MARGCOL_FACING);
					mcptr->mc.pgcont = findgroupcheck(hwnd,IDC_MARGCOL_NEVER,IDC_MARGCOL_NEWCOLUMN)-IDC_MARGCOL_NEVER;
					mcptr->mc.ncols = ComboBox_GetCurSel(GetDlgItem(hwnd,IDC_MARGCOL_COLUMNS))+1;
					mcptr->mc.clevel = ComboBox_GetCurSel(GetDlgItem(hwnd,IDC_MARGCOL_LEVEL));
					getDItemText(hwnd,IDC_MARGCOL_APPEND,mcptr->mc.continued,FSSTRING);
					err = 0;
					if (getfloat(hwnd,IDC_MARGCOL_TOP,&tval) || tval < 0)
						mcptr->mc.top = env_tobase(mcptr->eunit,tval);
					else
						err = IDC_MARGCOL_TOP;
					if (getfloat(hwnd,IDC_MARGCOL_BOTTOM,&tval) || tval < 0)
						mcptr->mc.bottom = env_tobase(mcptr->eunit,tval);
					else
						err = IDC_MARGCOL_BOTTOM;
					if (getfloat(hwnd,IDC_MARGCOL_LEFT,&tval) || tval < 0)
						mcptr->mc.left = env_tobase(mcptr->eunit,tval);
					else
						err = IDC_MARGCOL_LEFT;
					if (getfloat(hwnd,IDC_MARGCOL_RIGHT,&tval) || tval < 0)
						mcptr->mc.right = env_tobase(mcptr->eunit,tval);
					else
						err = IDC_MARGCOL_RIGHT;
					if (getfloat(hwnd,IDC_MARGCOL_GUTTER,&tval) || tval < 0)
						mcptr->mc.gutter = env_tobase(mcptr->eunit,tval);
					else
						err = IDC_MARGCOL_GUTTER;
					if (!err)	/* if no existing error */
						err = testmargins(&mcptr->mc,&mcptr->pi);	/*  check margins against page sizes */
					if (err)	{
						selectitext(hwnd,err);
						return (TRUE);
					}
				case IDCANCEL:
					EndDialog(hwnd,LOWORD(wParam) == IDOK ? TRUE : FALSE);
					return TRUE;
				case IDC_MARGCOL_FACING:
					setfacing(hwnd, iscontrolchecked((HWND)lParam));
					return (TRUE);
				case IDC_MARGCOL_UNIT:
					mcptr->eunit = ComboBox_GetCurSel((HWND)lParam);
					setfloat(hwnd,IDC_MARGCOL_TOP,env_toexpress(mcptr->eunit,mcptr->mc.top));
					setfloat(hwnd,IDC_MARGCOL_BOTTOM,env_toexpress(mcptr->eunit,mcptr->mc.bottom));
					setfloat(hwnd,IDC_MARGCOL_LEFT,env_toexpress(mcptr->eunit,mcptr->mc.left));
					setfloat(hwnd,IDC_MARGCOL_RIGHT,env_toexpress(mcptr->eunit,mcptr->mc.right));
					setfloat(hwnd,IDC_MARGCOL_GUTTER,env_toexpress(mcptr->eunit,mcptr->mc.gutter));
					return (TRUE);
				case IDC_MARGCOL_STYLE:
					getstyle(hwnd,&mcptr->mc.cstyle, 0);
					return(TRUE);
			}
			break;
		case WM_HELP:
			return (dodialoghelp(hwnd,TEXT("_Style and Layout\\Format_marginsandcolumns.htm"),(HELPINFO *)lParam,wh_margid));
	}
	return FALSE;
}
/*******************************************************************************/
static void setfacing(HWND hwnd, int reflect)	/* sets titles for facing pages */

{
	if (reflect)	{
		setDItemText(hwnd,IDC_MARGCOL_LEFTTEXT,"&Inside:");
		setDItemText(hwnd,IDC_MARGCOL_RIGHTTEXT,"&Outside:");
	}
	else	{
		setDItemText(hwnd,IDC_MARGCOL_LEFTTEXT,"&Left:");
		setDItemText(hwnd,IDC_MARGCOL_RIGHTTEXT,"&Right:");
	}
}
/*******************************************************************************/
static int testmargins(MARGINCOLUMN * mcp, PAPERINFO * pi)	/* tests margin settings against page settings */

{
#define MINPAGEHEIGHT 72	/* 72 points for min printable height */
#define MINCOLWIDTH 72		/* 72 points for min printable column width */

	if (pi->pheightactual - mcp->top - mcp->bottom < MINPAGEHEIGHT)	{	/* if too little height for text */
		senderr(ERR_VERTMARGINS, WARN);
		return(IDC_MARGCOL_TOP);
	}
	if ((pi->pwidthactual - mcp->left - mcp->right - (mcp->ncols-1)*mcp->gutter)/mcp->ncols < MINCOLWIDTH)	{	/* if too little width for column */
		senderr(ERR_HORIZMARGINS, WARN);
		return(IDC_MARGCOL_LEFT);
	}
	return (0);
}
/*******************************************************************************/
void fs_headfoot(HWND hwnd)	/* sets up headers and footers */

{
	INDEX * FF;
	PAGEFORMAT pf;

	if (hwnd)	{
		FF = getowner(hwnd);
		pf = FF->head.formpars.pf;
	}
	else
		pf = g_prefs.formpars.pf;
	if (DialogBoxParam(g_hinst,MAKEINTRESOURCE(IDD_HEADFOOT),g_hwframe,hfproc,(LPARAM)&pf))	{
		if (hwnd)	{
			FF->head.formpars.pf = pf;
			index_markdirty(FF);
		}
		else
			g_prefs.formpars.pf = pf;
	}
}
/******************************************************************************/
static INT_PTR CALLBACK hfproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)

{
	PAGEFORMAT * pfp;
	TC_ITEM tc;
	HWND th, cbh;
	int count, ttot;
	HEADERFOOTER *hf[4], *thp;
	TCHAR tstring[FTSTRING];
	TCHAR ** sptr;
	short tsize;

	pfp = getdata(hwnd);
	switch (msg)	{

		case WM_INITDIALOG:
			if (pfp = setdata(hwnd,(void *)lParam))	{	/* set data */
				type_setfontnamecombo(hwnd,IDC_HEADFOOT_FONT,NULL);
				CheckRadioButton(hwnd,IDC_HEADFOOT_LONG,IDC_HEADFOOT_SHORT,IDC_HEADFOOT_LONG+pfp->dateformat);
				checkitem(hwnd,IDC_HEADFOOT_TIME,pfp->timeflag);
				type_setfontsizecombo(GetDlgItem(hwnd,IDC_HEADFOOT_SIZE),12);	/* default 12 */
				Edit_LimitText(GetDlgItem(hwnd,IDC_HEADFOOT_LEFT),FTSTRING-1);
				Edit_LimitText(GetDlgItem(hwnd,IDC_HEADFOOT_CENTER),FTSTRING-1);
				Edit_LimitText(GetDlgItem(hwnd,IDC_HEADFOOT_RIGHT),FTSTRING-1);
				type_settextboxfont(GetDlgItem(hwnd,IDC_HEADFOOT_LEFT));
				type_settextboxfont(GetDlgItem(hwnd,IDC_HEADFOOT_CENTER));
				type_settextboxfont(GetDlgItem(hwnd,IDC_HEADFOOT_RIGHT));
				th = GetDlgItem(hwnd,IDC_HEADFOOT_TAB);
				if (pfp->mc.reflect)	{		/* if facing pages */
					hf[0] = &pfp->lefthead;		/* generate pointers for tab control */
					hf[1] = &pfp->leftfoot;
					hf[2] = &pfp->righthead;
					hf[3] = &pfp->rightfoot;
					sptr = fnames;
					ttot = 4;
#if 0
					if (compheadfoot(hf[0],hf[2]) || compheadfoot(hf[1],hf[3]))
						enableitem(hwnd,IDC_HEADFOOT_COPY);
					else
						disableitem(hwnd,IDC_HEADFOOT_COPY);
#endif
				}
				else {							/* all pages the same */
					hf[0] = &pfp->righthead;
					hf[1] = &pfp->rightfoot;
					sptr = bnames;
					ttot = 2;
					hideitem(hwnd,IDC_HEADFOOT_COPY);
				}
				memset(&tc,0,sizeof(tc));
				tc.mask = TCIF_PARAM|TCIF_TEXT;
				for (count = 0; count < ttot; count++)	{	/* build menu */
					tc.pszText = sptr[count];			/* tab title */
					tc.lParam = (LPARAM)hf[count];		/* insert pointer to its data */
					TabCtrl_InsertItem(th,count,&tc);
				}
				TabCtrl_SetCurSel(th,ttot-2);	/* set right header */
				settabitems(hwnd,th);		/* set the text for the first index (right head) */
				checkleftright(hwnd,pfp);
				setint(hwnd,IDC_HEADFOOT_PAGENUM,pfp->firstpage);
				cbh = GetDlgItem(hwnd,IDC_HEADFOOT_NUMFORMAT);
				for (count = 0; count < CR_NUMSIZE; count++)
					ComboBox_AddString(cbh,pnum[count]);
				ComboBox_SetCurSel(cbh,pfp->numformat);
				centerwindow(hwnd,1);	
			}
			return FALSE;
		case WM_COMMAND:
			if (HIWORD(wParam) == EN_UPDATE)	{
				WORD id = LOWORD(wParam);
				int length = 0;

				if (id == IDC_HEADFOOT_LEFT || id == IDC_HEADFOOT_CENTER || id == IDC_HEADFOOT_RIGHT)
					length = FTSTRING;
				checktextfield((HWND)lParam,length);
				return TRUE;
			}
			th = GetDlgItem(hwnd,IDC_HEADFOOT_TAB);
			thp = gethfp(th);
			switch (LOWORD(wParam))	{
				case IDOK:
					gettabitems(hwnd,th);	/* recover items */
					getshort(hwnd,IDC_HEADFOOT_PAGENUM,&pfp->firstpage);
					pfp->dateformat = findgroupcheck(hwnd,IDC_HEADFOOT_LONG,IDC_HEADFOOT_SHORT)-IDC_HEADFOOT_LONG;
					pfp->timeflag = (char)isitemchecked(hwnd,IDC_HEADFOOT_TIME);
					pfp->numformat = ComboBox_GetCurSel(GetDlgItem(hwnd,IDC_HEADFOOT_NUMFORMAT));
				case IDCANCEL:
					EndDialog(hwnd,LOWORD(wParam) == IDOK ? TRUE : FALSE);
					return TRUE;
				case IDC_HEADFOOT_COPY:
					gettabitems(hwnd,th);		/* force update of one displayed */
					if (pfp->mc.reflect && TabCtrl_GetCurSel(th) < 2)	{	/* if left page is on display */
						pfp->righthead = pfp->lefthead;
						pfp->rightfoot = pfp->leftfoot;
					}
					else {		/* right is on display */
						pfp->lefthead = pfp->righthead;
						pfp->leftfoot = pfp->rightfoot;
					}
					disableitem(hwnd,IDC_HEADFOOT_COPY);
					return TRUE;
				case IDC_HEADFOOT_STYLE:
					getstyle(hwnd,&thp->hfstyle, 0);
					checkleftright(hwnd,pfp);
					return TRUE;
				case IDC_HEADFOOT_FONT:
					if (HIWORD(wParam) == LBN_SELCHANGE)	{	/* changed */
						gettabitems(hwnd,th);		/* force update */
						checkleftright(hwnd,pfp);
					}
					return TRUE;
				case IDC_HEADFOOT_LEFT:
					GetWindowText((HWND)lParam,tstring,FTSTRING);
					strcpy(thp->left, fromNative(tstring));
					checkleftright(hwnd,pfp);
					return (TRUE);
				case IDC_HEADFOOT_CENTER:
					GetWindowText((HWND)lParam,tstring,FTSTRING);
					strcpy(thp->center, fromNative(tstring));
					checkleftright(hwnd,pfp);
					return (TRUE);
				case IDC_HEADFOOT_RIGHT:
					GetWindowText((HWND)lParam,tstring,FTSTRING);
					strcpy(thp->right, fromNative(tstring));
					checkleftright(hwnd,pfp);
					return (TRUE);
				case IDC_HEADFOOT_SIZE:
					if (HIWORD(wParam) == CBN_EDITCHANGE)	{	/* changed text */
						if (!getshort(hwnd,IDC_HEADFOOT_SIZE,&tsize))	/* if bad size */
 							setint(hwnd,IDC_HEADFOOT_SIZE,thp->size);
						else
							checkleftright(hwnd,pfp);
					}
					else if (HIWORD(wParam) == CBN_SELCHANGE)	/* changed text */
						checkleftright(hwnd,pfp);
					return (TRUE);
			}
			break;
		case WM_NOTIFY:
			switch (LOWORD(wParam))	{
				case IDC_HEADFOOT_TAB:
					if (((LPNMHDR)lParam)->code == TCN_SELCHANGING)	
						gettabitems(hwnd,((LPNMHDR)lParam)->hwndFrom);
					else if (((LPNMHDR)lParam)->code == TCN_SELCHANGE)
						settabitems(hwnd,((LPNMHDR)lParam)->hwndFrom);
					return TRUE;
				default:
					;
			}
			break;
		case WM_HELP:
			return (dodialoghelp(hwnd,TEXT("_Style and Layout\\Format_headersandfooters.htm"),(HELPINFO *)lParam,wh_hfid));
	}
	return FALSE;
}
/*******************************************************************************/
static void settabitems(HWND hwnd,HWND th)		/* sets tab items */

{
	HEADERFOOTER * hfp;

	hfp = gethfp(th);
	setDItemText(hwnd,IDC_HEADFOOT_LEFT,hfp->left);
	setDItemText(hwnd,IDC_HEADFOOT_CENTER,hfp->center);
	setDItemText(hwnd,IDC_HEADFOOT_RIGHT,hfp->right);
	setfontcombo(GetDlgItem(hwnd,IDC_HEADFOOT_FONT),hfp->hffont);
	setint(hwnd,IDC_HEADFOOT_SIZE,hfp->size);
	SetFocus(GetDlgItem(hwnd,IDC_HEADFOOT_LEFT));
}
/*******************************************************************************/
static void gettabitems(HWND hwnd,HWND th)		/* gets tab items */

{
	HEADERFOOTER * hfp;

	hfp = gethfp(th);
	getDItemText(hwnd,IDC_HEADFOOT_LEFT,hfp->left,FTSTRING);
	getDItemText(hwnd,IDC_HEADFOOT_CENTER,hfp->center,FTSTRING);
	getDItemText(hwnd,IDC_HEADFOOT_RIGHT,hfp->right,FTSTRING);
	getfontcombo(GetDlgItem(hwnd,IDC_HEADFOOT_FONT),hfp->hffont);
	getshort(hwnd,IDC_HEADFOOT_SIZE,&hfp->size);
}
/*******************************************************************************/
static HEADERFOOTER * gethfp(HWND th)		/* gets pointer to current headfoot struct  */

{
	int tindex;
	TC_ITEM tc;

	tindex = TabCtrl_GetCurSel(th);
	tc.mask = TCIF_PARAM;
	TabCtrl_GetItem(th,tindex,&tc);
	return((HEADERFOOTER *)tc.lParam);
}
/*******************************************************************************/
static void checkleftright(HWND hwnd, PAGEFORMAT * pfp)	/* checks left/right diffs */

{
	if (compheadfoot(&pfp->righthead,&pfp->lefthead) ||compheadfoot(&pfp->rightfoot,&pfp->leftfoot))
		enableitem(hwnd,IDC_HEADFOOT_COPY);
	else
		disableitem(hwnd,IDC_HEADFOOT_COPY);
}
/*******************************************************************************/
static short compheadfoot(HEADERFOOTER * h1, HEADERFOOTER * h2)	/* compares contents of headfoot structs */

{
	return (strcmp(h1->left,h2->left) || strcmp(h1->center,h2->center) || strcmp(h1->right, h2->right)
		|| h1->hfstyle.style != h2->hfstyle.style || h1->hfstyle.cap != h2->hfstyle.cap
		|| strcmp(h1->hffont,h2->hffont) || h1->size != h2->size);
}
/*******************************************************************************/
void fs_entrygroup(HWND hwnd)	/* sets up entry grouping */

{
	GROUPFORMAT eg;
	INDEX * FF;

	if (hwnd)	{
		FF = WX(hwnd,owner);
		eg = FF->head.formpars.ef.eg;
	}
	else
		eg = g_prefs.formpars.ef.eg;
	if (DialogBoxParam(g_hinst,MAKEINTRESOURCE(IDD_GROUPENTRIES),g_hwframe,grpproc,(LPARAM)&eg))	{
		if (hwnd)	{
			FF->head.formpars.ef.eg = eg;
			view_redisplay(FF,0,VD_CUR|VD_RESET);	/* force reset of display pars */
			index_markdirty(FF);
		}
		else
			g_prefs.formpars.ef.eg = eg;
	}
}
/******************************************************************************/
static INT_PTR CALLBACK grpproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)

{
	GROUPFORMAT *egp;
	HWND cbh;
	int count;

	egp = getdata(hwnd);
	switch (msg)	{

		case WM_INITDIALOG:
			if (egp = setdata(hwnd,(void *)lParam))	{	/* set data */
				type_setfontnamecombo(hwnd,IDC_GROUPENTRIES_FONT,NULL);
				setfontcombo(GetDlgItem(hwnd,IDC_GROUPENTRIES_FONT),egp->gfont);
				setDItemText(hwnd,IDC_GROUPENTRIES_TITLE,egp->title);
				setDItemText(hwnd,IDC_GROUPENTRIES_ALLNUMBERS,egp->ninsert);
				setDItemText(hwnd,IDC_GROUPENTRIES_ALLSYMBOLS,egp->sinsert);
				setDItemText(hwnd,IDC_GROUPENTRIES_NUMBERSANDSYMBOLS,egp->nsinsert);
				type_settextboxfont(GetDlgItem(hwnd,IDC_GROUPENTRIES_TITLE));
				type_settextboxfont(GetDlgItem(hwnd,IDC_GROUPENTRIES_ALLNUMBERS));
				type_settextboxfont(GetDlgItem(hwnd,IDC_GROUPENTRIES_ALLSYMBOLS));
				type_settextboxfont(GetDlgItem(hwnd,IDC_GROUPENTRIES_NUMBERSANDSYMBOLS));
				Edit_LimitText(GetDlgItem(hwnd,IDC_GROUPENTRIES_TITLE),FSSTRING-1);
				cbh = GetDlgItem(hwnd,IDC_GROUPENTRIES_NUMSYM);
				for (count = 0; count < GP_NSSIZE; count++)
					ComboBox_AddString(cbh,ns[count]);
				ComboBox_SetCurSel(cbh,egp->method);
				centerwindow(hwnd,1);	
			}
			return FALSE;
		case WM_COMMAND:
			if (HIWORD(wParam) == EN_UPDATE)	{
				WORD id = LOWORD(wParam);
				int length = 0;

				if (id == IDC_GROUPENTRIES_TITLE)
					length = FSSTRING;
				checktextfield((HWND)lParam,length);
				return TRUE;
			}
			switch (LOWORD(wParam))	{
				case IDOK:
					egp->method = ComboBox_GetCurSel(GetDlgItem(hwnd,IDC_GROUPENTRIES_NUMSYM));
					getDItemText(hwnd, IDC_GROUPENTRIES_TITLE,egp->title,FSSTRING);
					getDItemText(hwnd, IDC_GROUPENTRIES_ALLNUMBERS,egp->ninsert,FSSTRING);
					getDItemText(hwnd, IDC_GROUPENTRIES_ALLSYMBOLS,egp->sinsert,FSSTRING);
					getDItemText(hwnd, IDC_GROUPENTRIES_NUMBERSANDSYMBOLS,egp->nsinsert,FSSTRING);
					getfontcombo(GetDlgItem(hwnd,IDC_GROUPENTRIES_FONT), egp->gfont);
				case IDCANCEL:
					EndDialog(hwnd,LOWORD(wParam) == IDOK ? TRUE : FALSE);
					return TRUE;
				case IDC_GROUPENTRIES_STYLE:
					getstyle(hwnd,&egp->gstyle, 0);
					return (TRUE);

			}
			break;
		case WM_HELP:
			return (dodialoghelp(hwnd,TEXT("_Style and Layout\\Format_groupingentries.htm"),(HELPINFO *)lParam,wh_groupid));
	}
	return (FALSE);
}
/*******************************************************************************/
void fs_stylelayout(HWND hwnd)	/* sets up style and layout */

{
	struct slgroup slg;
	INDEX * FF;
	
	if (hwnd)	{		/* if index is front */
		FF = WX(hwnd,owner);
		slg.fp = FF->head.formpars;
		slg.ip = &FF->head.indexpars;
		slg.lspace = LX(hwnd,lspace);
		slg.emspace = LX(hwnd,empoints);
	}
	else {
		slg.fp = g_prefs.formpars;
		slg.ip = &g_prefs.indexpars;
		slg.lspace = 16;
		slg.emspace = 14;
	}
	slg.dirtylead = slg.dirtyrun = slg.dirtyline = FALSE;
	if (DialogBoxParam(g_hinst,MAKEINTRESOURCE(IDD_STYLELAYOUT),g_hwframe,stlproc,(LPARAM)&slg))	{
		if (hwnd)	{
			FF->head.formpars = slg.fp;
			view_redisplay(FF,0,VD_CUR|VD_RESET);	/* force reset of display pars */
			index_markdirty(FF);
		}
		else
			g_prefs.formpars = slg.fp;
	}
}
/******************************************************************************/
static INT_PTR CALLBACK stlproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)

{
	struct slgroup *slp;
	HWND cbh;
	int count, tindex;

	slp = getdata(hwnd);
	switch (msg)	{
		case WM_INITDIALOG:
			if (slp = setdata(hwnd,(void *)lParam))	{	/* set data */
				CheckRadioButton(hwnd,IDC_STYLAY_INDENTED,IDC_STYLAY_RUNIN,slp->fp.ef.runlevel ? IDC_STYLAY_RUNIN :IDC_STYLAY_INDENTED);
				buildheadingmenu(GetDlgItem(hwnd,IDC_STYLAY_RUNLEVEL),slp->ip,slp->fp.ef.runlevel-1);
				ComboBox_DeleteString(GetDlgItem(hwnd,IDC_STYLAY_RUNLEVEL),slp->ip->maxfields-2);	// delete string for lowest level
				buildstyleoptionsmenu(hwnd,slp->fp.ef.style);
				checkitem(hwnd,IDC_STYLAY_COLLAPSE,slp->fp.ef.collapselevel ? TRUE : FALSE);
				buildheadingmenu(GetDlgItem(hwnd,IDC_STYLAY_COLLAPSELEVEL),slp->ip,slp->fp.ef.collapselevel-1);
				ComboBox_DeleteString(GetDlgItem(hwnd,IDC_STYLAY_COLLAPSELEVEL),slp->ip->maxfields-2);	// delete string for lowest level
				if (!slp->fp.ef.collapselevel)
					disableitem(hwnd,IDC_STYLAY_COLLAPSELEVEL);
				cbh = GetDlgItem(hwnd,IDC_STYLAY_INDENTTYPE);
				for (count = 0;count < SL_ITSIZE; count++)	/* set indentation type */
					ComboBox_AddString(cbh,itype[count]);
				ComboBox_SetCurSel(cbh,slp->fp.ef.itype);
				cbh = GetDlgItem(hwnd,IDC_STYLAY_INDUNIT);
				buildunitmenu(cbh,0);
				ComboBox_InsertString(cbh,0,TEXT("Em space"));
				builditabset(hwnd, slp);		/* builds indent tab set */
				cbh = GetDlgItem(hwnd,IDC_STYLAY_SPACING);
				for (count = 0;count < SL_SPSIZE; count++)	/* set spacing options */
					ComboBox_AddString(cbh,spopt[count]);
				ComboBox_SetCurSel(cbh,slp->fp.pf.linespace);
				buildunitmenu(GetDlgItem(hwnd,IDC_STYLAY_LSPUNIT),slp->fp.pf.lineunit);
				checkitem(hwnd,IDC_STYLAY_AUTO,slp->fp.pf.autospace);
				if (slp->fp.pf.autospace)		/* if autospacing */
					disableitem(hwnd,IDC_STYLAY_LSPACE);
				setlinespace(hwnd,slp);
				setint(hwnd,IDC_STYLAY_LSPMAIN,slp->fp.pf.entryspace);
				setint(hwnd,IDC_STYLAY_LSPGROUP,slp->fp.pf.above);			
				checkitem(hwnd,IDC_STYLAY_ADJQUOTE,slp->fp.ef.adjustpunct);
				checkitem(hwnd,IDC_STYLAY_ADJSTYLE,slp->fp.ef.adjstyles);
				centerwindow(hwnd,1);	
			}
			return FALSE;
		case WM_COMMAND:
			switch (LOWORD(wParam))	{
				case IDOK:
					if (isitemchecked(hwnd,IDC_STYLAY_INDENTED))	/* if indented */
						slp->fp.ef.runlevel = 0;
					else
						slp->fp.ef.runlevel = ComboBox_GetCurSel(GetDlgItem(hwnd,IDC_STYLAY_RUNLEVEL))+1;
					if (isitemchecked(hwnd,IDC_STYLAY_COLLAPSE))	/* if want collapsing */
						slp->fp.ef.collapselevel = ComboBox_GetCurSel(GetDlgItem(hwnd,IDC_STYLAY_COLLAPSELEVEL))+1;
					else
						slp->fp.ef.collapselevel = 0;
					cbh = GetDlgItem(hwnd,IDC_STYLAY_STYLEMOD);
					tindex = ComboBox_GetCurSel(cbh);
					slp->fp.ef.style = (char)ComboBox_GetItemData(cbh,tindex);
					if (!getitabitems(hwnd,slp))	/* if bad conversions */
						return (TRUE);
#if 0
					slp->fp.pf.autospace = (char)isitemchecked(hwnd,IDC_STYLAY_AUTO);
#endif
					slp->fp.pf.linespace = ComboBox_GetCurSel(GetDlgItem(hwnd,IDC_STYLAY_SPACING));
					if (!getlinespace(hwnd,slp))	/* if error in line spacing */
						return (TRUE);
					getshort(hwnd,IDC_STYLAY_LSPMAIN,&slp->fp.pf.entryspace);
					getshort(hwnd,IDC_STYLAY_LSPGROUP,&slp->fp.pf.above);
					slp->fp.ef.adjustpunct = (char)isitemchecked(hwnd,IDC_STYLAY_ADJQUOTE);
					slp->fp.ef.adjstyles = (char)isitemchecked(hwnd,IDC_STYLAY_ADJSTYLE);
				case IDCANCEL:
					EndDialog(hwnd,LOWORD(wParam) == IDOK ? TRUE : FALSE);
					return TRUE;
				case IDC_STYLAY_INDENTED:	/* indent type */
				case IDC_STYLAY_RUNIN:		/* indent type */
					buildstyleoptionsmenu(hwnd,0);
					return (TRUE);
				case IDC_STYLAY_COLLAPSE:
					if (iscontrolchecked((HWND)lParam))
						enableitem(hwnd,IDC_STYLAY_COLLAPSELEVEL);
					else
						disableitem(hwnd,IDC_STYLAY_COLLAPSELEVEL);
					return (TRUE);
				case IDC_STYLAY_INDENTTYPE:		/* indent type */
					if (HIWORD(wParam) == CBN_SELENDOK)
						builditabset(hwnd, slp);	/* builds tab set & redisplays */
					break;
				case IDC_STYLAY_INDUNIT:		/* indent units */
					if (HIWORD(wParam) == CBN_SELENDOK)
						switchunit(hwnd,slp);
					return(TRUE);
				case IDC_STYLAY_LSPUNIT:		/* linespace units */
					if (HIWORD(wParam) == CBN_SELENDOK)	{
						getlinespace(hwnd,slp);
						slp->fp.pf.lineunit = ComboBox_GetCurSel((HWND)lParam);
						setlinespace(hwnd,slp);
					}
					return (TRUE);
				case IDC_STYLAY_AUTO:
					if (iscontrolchecked((HWND)lParam))	{
						slp->fp.pf.autospace = TRUE;
						disableitem(hwnd,IDC_STYLAY_LSPACE);
						setlinespace(hwnd,slp);
					}
					else	{
						slp->fp.pf.autospace = FALSE;
						enableitem(hwnd,IDC_STYLAY_LSPACE);
						slp->fp.pf.lineheight = (short)slp->lspace;	/* fixed becomes same as auto */
					}
					return (TRUE);
				case IDC_STYLAY_LSPACE:
					if (HIWORD(wParam) == EN_CHANGE)
						slp->dirtyline = TRUE;
					return (TRUE);
				case IDC_STYLAY_LEADINDENT:
					if (HIWORD(wParam) == EN_CHANGE)
						slp->dirtylead = TRUE;
					return (TRUE);
				case IDC_STYLAY_RUNINDENT:
					if (HIWORD(wParam) == EN_CHANGE)
						slp->dirtyrun = TRUE;
					return (TRUE);
			}
			break;
		case WM_NOTIFY:
			switch (LOWORD(wParam))	{
				case IDC_STYLAY_TAB:
					if (((LPNMHDR)lParam)->code == TCN_SELCHANGING)	{
						if (!getitabitems(hwnd,slp))
							SetWindowLongPtr(hwnd, DWLP_MSGRESULT,TRUE);
						return (TRUE);
					}
					else if (((LPNMHDR)lParam)->code == TCN_SELCHANGE)
						setitabitems(hwnd,slp);
					break;
				default:
					;
			}
			break;
		case WM_HELP:
			return (dodialoghelp(hwnd,TEXT("_Style and Layout\\Format_styleandlayout.htm"),(HELPINFO *)lParam,wh_stylayid));
	}
	return (FALSE);
}
/*******************************************************************************/
static void buildstyleoptionsmenu(HWND hwnd, int stylemod)	/* builds & sets style options menu */

{
	HWND cbh;
	int count, runlevel;

	runlevel = isitemchecked(hwnd,IDC_STYLAY_RUNIN);
	cbh = GetDlgItem(hwnd,IDC_STYLAY_STYLEMOD);
	ComboBox_ResetContent(cbh);
	for (count = 0; count < SL_MSIZE; count++)	{
		if (!count || !runlevel && (count ==  1 || count == 3) || runlevel && count == 2)	{	/* pick admissible items */
			int item = ComboBox_AddString(cbh,mods[count]);
			ComboBox_SetItemData(cbh,item,count);	/* set index as data */
			if (count == stylemod)				/* if this is our style modifier */
				ComboBox_SetCurSel(cbh,item);	/* select current item */
		}
	}
	if (!runlevel)
		disableitem(hwnd,IDC_STYLAY_RUNLEVEL);
	else
		enableitem(hwnd,IDC_STYLAY_RUNLEVEL);
}
/*******************************************************************************/
static void setlinespace(HWND hwnd, struct slgroup *slp)	/* sets line spacing */

{
	setfloat(hwnd,IDC_STYLAY_LSPACE,env_toexpress(slp->fp.pf.lineunit,slp->fp.pf.autospace ? slp->lspace : slp->fp.pf.lineheight));
	slp->dirtyline = FALSE;
}
/*******************************************************************************/
static BOOL getlinespace(HWND hwnd, struct slgroup *slp)	/* gets line spacing */

{
	float tval;

	if (slp->dirtyline)	{	/* if we actually changed the text field */
		if (!getfloat(hwnd,IDC_STYLAY_LSPACE,&tval) || !tval || env_tobase(slp->fp.pf.lineunit,tval) < slp->lspace/2)	{
			/* if no number or too small (< half auto) */
			senditemerr(hwnd,IDC_STYLAY_LSPACE);
			return (FALSE);
		}
		slp->fp.pf.lineheight = env_tobase(slp->fp.pf.lineunit,tval);
	}
	slp->dirtyline = FALSE;
	return (TRUE);
}
/*******************************************************************************/
static void builditabset(HWND hwnd, struct slgroup *slp)	/* builds tab set for current indent scheme */

{
	TC_ITEM tc;
	HWND th;
	static TCHAR *tabhead[] = {TEXT("Incremental Indent"), TEXT("Last Indent")};
	int count;

	slp->fp.ef.itype = ComboBox_GetCurSel(GetDlgItem(hwnd,IDC_STYLAY_INDENTTYPE));
	th = GetDlgItem(hwnd,IDC_STYLAY_TAB);
	TabCtrl_DeleteAllItems(th);
	memset(&tc,0,sizeof(tc));
	tc.mask = TCIF_TEXT;
	if (slp->fp.ef.itype == FI_NONE)	{
		disableitem(hwnd,IDC_STYLAY_INDUNIT);
		disableitem(hwnd,IDC_STYLAY_LEADINDENT);
		disableitem(hwnd,IDC_STYLAY_RUNINDENT);
		disableitem(hwnd,IDC_STYLAY_TAB);
	}
	else	{
		enableitem(hwnd,IDC_STYLAY_INDUNIT);
		enableitem(hwnd,IDC_STYLAY_LEADINDENT);
		enableitem(hwnd,IDC_STYLAY_RUNINDENT);
		enableitem(hwnd,IDC_STYLAY_TAB);
	}
	if (slp->fp.ef.itype == FI_AUTO || slp->fp.ef.itype == FI_NONE)	{
		tc.pszText = tabhead[0];			/* tab title */
		TabCtrl_InsertItem(th,0,&tc);
	}
	else if (slp->fp.ef.itype == FI_FIXED)	{
		for (count = 0; count < slp->ip->maxfields-1; count++)	{	/* build menu */
			tc.pszText = toNative(slp->ip->field[count].name);			/* tab title */
			TabCtrl_InsertItem(th,count,&tc);
		}
	}
	else if (slp->fp.ef.itype == FI_SPECIAL)	 {	/* special last */
		for (count = 0; count < 2; count++)	{	/* build menu */
			tc.pszText = tabhead[count];		/* tab title */
			TabCtrl_InsertItem(th,count,&tc);
		}
	}
	TabCtrl_SetCurSel(th,0);	/* set first */
	setitabitems(hwnd,slp);	/* set items as per tab index */
}
/*******************************************************************************/
static void setitabitems(HWND hwnd,struct slgroup *slp)		/* sets tab items */

{
	float * flptr,*frptr;
	char *unit;

	getcurrenttabpointers(hwnd,slp->ip,&slp->fp,&unit,&flptr,&frptr);
	ComboBox_SetCurSel(GetDlgItem(hwnd,IDC_STYLAY_INDUNIT),*unit); /* set current item */
	setfloat(hwnd,IDC_STYLAY_LEADINDENT,*unit ? env_toexpress(*unit-1,(short)*flptr) : *flptr);
	setfloat(hwnd,IDC_STYLAY_RUNINDENT,*unit ? env_toexpress(*unit-1,(short)*frptr) : *frptr);
	slp->dirtylead = slp->dirtyrun = FALSE;
}
/*******************************************************************************/
static BOOL getitabitems(HWND hwnd,struct slgroup *slp)		/* gets tab items */

{
	float tlead,trun;
	float * flptr, *frptr;
	char * unit;

	getcurrenttabpointers(hwnd,slp->ip,&slp->fp,&unit,&flptr,&frptr);
	if (!getfloat(hwnd,IDC_STYLAY_LEADINDENT,&tlead))	{
		senditemerr(hwnd,IDC_STYLAY_LEADINDENT);
		return (FALSE);
	}
	if (!getfloat(hwnd,IDC_STYLAY_RUNINDENT,&trun))	{
		senditemerr(hwnd,IDC_STYLAY_RUNINDENT);
		return (FALSE);
	}
	if (*unit)	{	/* if were using fixed units & want conversion */
		tlead = env_tobase(*unit-1,tlead);
		trun = env_tobase(*unit-1,trun);
	}
	/* otherwise no conversion required, cause em's are stored directly */
/* following is to ensure that we avoid rounding errors in reading from text boxes */
	if (slp->dirtylead)	/* if we ever actually changed the lead */
		*flptr = tlead;	/* use new value */
	if (slp->dirtyrun)	/* if we ever changed the run */
		*frptr = trun;
	slp->dirtylead = slp->dirtyrun = FALSE;
	return (TRUE);
}
/*******************************************************************************/
static void switchunit(HWND hwnd,struct slgroup *slp)		/* does em-space conversions */

{
	int count,type;
	float * flptr, *frptr;
	char *unit, newunit;

	getcurrenttabpointers(hwnd,slp->ip,&slp->fp,&unit,&flptr,&frptr);	/* recover current settings */
	newunit = ComboBox_GetCurSel(GetDlgItem(hwnd,IDC_STYLAY_INDUNIT));		/* get new unit */
	if (*unit != newunit)	{	/* if changed unit */
		if (getitabitems(hwnd,slp))	{	/* if can recover current values */
				/* identify case where all fields have to be scaled */
			type = slp->fp.ef.itype == FI_FIXED || slp->fp.ef.itype == FI_SPECIAL && TabCtrl_GetCurSel(GetDlgItem(hwnd,IDC_STYLAY_TAB));
			if (!newunit)	{	/* changing to em space */
				if (type)	{	/* doing fixed indentation, scale all */
					for (count = 0; count < FIELDLIM-1; count++)	{
						slp->fp.ef.field[count].leadindent /= slp->emspace;
						slp->fp.ef.field[count].runindent /= slp->emspace;
					}
				}
				else	{
					slp->fp.ef.autolead /= slp->emspace;
					slp->fp.ef.autorun /= slp->emspace;
				}
			}
			else if (!*unit) {	/* changing from em space */
				if (type)	{	/* doing fixed indentation, scale all */
					for (count = 0; count < FIELDLIM-1; count++)	{
						slp->fp.ef.field[count].leadindent *= slp->emspace;
						slp->fp.ef.field[count].runindent *= slp->emspace;
					}
				}
				else {
					slp->fp.ef.autolead *= slp->emspace;
					slp->fp.ef.autorun *= slp->emspace;
				}
			}
			*unit = newunit;	/* all's well */
		}
		setitabitems(hwnd, slp);	/* redisplay */
	}
}
/*******************************************************************************/
static void getcurrenttabpointers(HWND hwnd, INDEXPARAMS *ip,FORMATPARAMS *fp, char **mp, float **lp, float ** rp)

	/* gets pointers to elements in current tab display */
{
	int tindex;

	tindex = TabCtrl_GetCurSel(GetDlgItem(hwnd,IDC_STYLAY_TAB));
	if (fp->ef.itype == FI_AUTO || fp->ef.itype == FI_SPECIAL)	{	/* if auto or special */
		if (!tindex)	{	/* if the auto indent */
			*mp = &fp->ef.autounit;
			*lp = &fp->ef.autolead;
			*rp = &fp->ef.autorun;
		}
		else	{		/* the special indent */
			*mp = &fp->ef.fixedunit;
//			*lp = &fp->ef.field[ip->maxfields-2].leadindent;
//			*rp = &fp->ef.field[ip->maxfields-2].runindent;
			*lp = &fp->ef.field[L_SPECIAL].leadindent;
			*rp = &fp->ef.field[L_SPECIAL].runindent;
		}
	}
	else {		/* get contents for a specific field */
		*mp = &fp->ef.fixedunit;
		*lp = &fp->ef.field[tindex].leadindent;
		*rp = &fp->ef.field[tindex].runindent;
	}

}
/*******************************************************************************/
void fs_headings(HWND hwnd)	/* sets up heading format */

{
	struct headgrp hg;
	INDEX * FF;

	if (hwnd)	{
		FF = getowner(hwnd);
		hg.ef = FF->head.formpars.ef;
		hg.ip = &FF->head.indexpars;
	}
	else {
		hg.ef = g_prefs.formpars.ef;
		hg.ip = &g_prefs.indexpars;
	}
	if (DialogBoxParam(g_hinst,MAKEINTRESOURCE(IDD_HEADINGS),g_hwframe,headproc,(LPARAM)&hg))	{
		if (hwnd)	{
			FF->head.formpars.ef = hg.ef;
			if (FF->head.privpars.vmode == VM_FULL)	/* if index in formatted view */
				view_redisplay(FF,0,VD_CUR|VD_RESET);	/* force reset of display pars */
			index_markdirty(FF);
		}
		else
			g_prefs.formpars.ef = hg.ef;
	}
}
/******************************************************************************/
static INT_PTR CALLBACK headproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)

{
	struct headgrp *hgp;
	TC_ITEM tc;
	HWND th;
	int count, tindex;

	hgp = getdata(hwnd);
	switch (msg)	{

		case WM_INITDIALOG:
			if (hgp = setdata(hwnd,(void *)lParam))	{	/* set data */
				th = GetDlgItem(hwnd,IDC_HEADING_TAB);
				memset(&tc,0,sizeof(tc));
				tc.mask = TCIF_TEXT;
				for (count = 0; count < hgp->ip->maxfields-1; count++)	{	/* build menu */
					if (count == hgp->ip->maxfields-2 && hgp->ip->required)	{	// if required field
						TCHAR fname[FNAMELEN+10];
						u_sprintf(fname,"%s *", hgp->ip->field[count].name);	// make special title
						tc.pszText = fname;
					}
					else
						tc.pszText = toNative(hgp->ip->field[count].name);			/* tab title */
					TabCtrl_InsertItem(th,count,&tc);
				}
				TabCtrl_SetCurSel(th,0);
				type_setfontnamecombo(hwnd,IDC_HEADING_FONT,NULL);
				th = GetDlgItem(hwnd,IDC_HEADING_TRAILPUNCT);
				Edit_LimitText(th,FMSTRING-1);
				type_settextboxfont(th);
				th = GetDlgItem(hwnd,IDC_HEADING_LEADTEXT);
				Edit_LimitText(th,FMSTRING-5);
				type_settextboxfont(th);
				SetFocus(th);
				sethtabitems(hwnd,((LPNMHDR)lParam)->hwndFrom,&hgp->ef);
				centerwindow(hwnd,1);
			}
			return FALSE;
		case WM_COMMAND:
			if (HIWORD(wParam) == EN_UPDATE)	{
				WORD id = LOWORD(wParam);
				int length = 0;

				if (id == IDC_HEADING_TRAILPUNCT)
					length = FMSTRING;
				else if (id == IDC_HEADING_LEADTEXT)
					length = FMSTRING-4;
				checktextfield((HWND)lParam,length);
				return TRUE;
			}
			switch (LOWORD(wParam))	{
				case IDOK:
					gethtabitems(hwnd,GetDlgItem(hwnd,IDC_HEADING_TAB),&hgp->ef);
				case IDCANCEL:
					EndDialog(hwnd,LOWORD(wParam) == IDOK ? TRUE : FALSE);
					return TRUE;
				case IDC_HEADING_STYLE:
					tindex = TabCtrl_GetCurSel(GetDlgItem(hwnd,IDC_HEADING_TAB));
					getstyle(hwnd,&hgp->ef.field[tindex].style, FC_TITLE);
					return (TRUE);
			}
			break;
		case WM_NOTIFY:
			switch (LOWORD(wParam))	{
				case IDC_HEADING_TAB:
					if (((LPNMHDR)lParam)->code == TCN_SELCHANGING)	
						gethtabitems(hwnd,((LPNMHDR)lParam)->hwndFrom,&hgp->ef);
					else if (((LPNMHDR)lParam)->code == TCN_SELCHANGE)
						sethtabitems(hwnd,((LPNMHDR)lParam)->hwndFrom,&hgp->ef);
					break;
				default:
					;
			}
			break;
		case WM_HELP:
			return (dodialoghelp(hwnd,TEXT("_Style and Layout\\Format_headings.htm"),(HELPINFO *)lParam,wh_headid));
	}
	return (FALSE);
}
/*******************************************************************************/
static void sethtabitems(HWND hwnd, HWND th,ENTRYFORMAT *efp)	/* sets items for heading tab */

{
	int tindex, sindex;

	for (sindex = 0; sindex < TabCtrl_GetItemCount(th); sindex++)	{	// for all fields
		if (efp->field[sindex].flags&FH_SUPPRESS)	// if one is suppressed
			break;		// break with index at first suppresed
	}
	tindex = TabCtrl_GetCurSel(th);
	if (sindex < tindex)	{	// if our feld suppressed
		checkitem(hwnd,IDC_HEADING_SUPPRESS,TRUE);
		disableitem(hwnd,IDC_HEADING_SUPPRESS);
	}
	else	{
		enableitem(hwnd,IDC_HEADING_SUPPRESS);
		checkitem(hwnd,IDC_HEADING_SUPPRESS,efp->field[tindex].flags&FH_SUPPRESS);
	}
	setDItemText(hwnd,IDC_HEADING_LEADTEXT,efp->field[tindex].leadtext);
	setDItemText(hwnd,IDC_HEADING_TRAILPUNCT,efp->field[tindex].trailtext);
	setfontcombo(GetDlgItem(hwnd,IDC_HEADING_FONT),efp->field[tindex].font);
}
/*******************************************************************************/
static void gethtabitems(HWND hwnd, HWND th,ENTRYFORMAT *efp)	/* gets items for heading tab */

{
	int tindex;

	tindex = TabCtrl_GetCurSel(th);
	getDItemText(hwnd,IDC_HEADING_LEADTEXT,efp->field[tindex].leadtext,FMSTRING-4);
	getDItemText(hwnd,IDC_HEADING_TRAILPUNCT,efp->field[tindex].trailtext,FMSTRING);
	if (isitemenabled(hwnd,IDC_HEADING_SUPPRESS) && isitemchecked(hwnd,IDC_HEADING_SUPPRESS))
		efp->field[tindex].flags |= FH_SUPPRESS;
	else
		efp->field[tindex].flags &= ~FH_SUPPRESS;
	getfontcombo(GetDlgItem(hwnd,IDC_HEADING_FONT),efp->field[tindex].font);
}
/*******************************************************************************/
void fs_crossrefs(HWND hwnd)	/* sets up cross-references */

{
	CROSSREFFORMAT cf;
	INDEX * FF;
	
	if (hwnd)	{
		FF = getowner(hwnd);
		cf = FF->head.formpars.ef.cf;
	}
	else
		cf = g_prefs.formpars.ef.cf;
	if (DialogBoxParam(g_hinst,MAKEINTRESOURCE(IDD_CROSSREFS),g_hwframe,crefproc,(LPARAM)&cf))	{
		if (hwnd)	{
			if (cf.mainposition <= CP_FIRSTSUB && FF->head.formpars.ef.cf.mainposition >= CP_LASTSUB
					|| cf.mainposition >= CP_LASTSUB && FF->head.formpars.ef.cf.mainposition <= CP_FIRSTSUB
					|| cf.subposition <= CP_FIRSTSUB && FF->head.formpars.ef.cf.subposition >= CP_LASTSUB
					|| cf.subposition >= CP_LASTSUB && FF->head.formpars.ef.cf.subposition <= CP_FIRSTSUB)	{	/* if need resort */
				if (sendwarning(WARN_CROSSSORT))	{
					FF->head.formpars.ef.cf = cf;	/* set new params */
					col_init(&FF->head.sortpars,FF);		// initialize collator
					sort_resort(FF);		/* sort whole index anyway */
					if (FF->curfile)
						sort_sortgroup(FF);
				}
				else	/* abandon */
					return;
			}
			FF->head.formpars.ef.cf = cf;
			view_redisplay(FF,0,VD_CUR|VD_RESET);	/* force reset of display pars */
			index_markdirty(FF);
		}
		else
			g_prefs.formpars.ef.cf = cf;
	}
}
/******************************************************************************/
static INT_PTR CALLBACK crefproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)

{
	CROSSREFFORMAT *cfp;
	TC_ITEM tc;
	HWND th, cbh;
	int count;

	cfp = getdata(hwnd);
	switch (msg)	{

		case WM_INITDIALOG:
			if (cfp = setdata(hwnd,(void *)lParam))	{	/* set data */
				cbh = GetDlgItem(hwnd,IDC_CROSSREF_XPOSITION);
				for (count = 0; count < CR_XPSIZE; count++)
					ComboBox_AddString(cbh,xpos[count]);
//				ComboBox_SetCurSel(cbh,cfp->mainseeposition);
				cbh = GetDlgItem(hwnd,IDC_CROSSREF_XAPOSITION);
				for (count = 0; count < CR_XAPSIZE; count++)
					ComboBox_AddString(cbh,xapos[count]);
//				ComboBox_SetCurSel(cbh,cfp->mainposition);
				checkitem(hwnd,IDC_CROSSREF_SUPPRESS,cfp->suppressall);
				checkitem(hwnd,IDC_CROSSREF_ORDER,cfp->sortcross);
				checkitem(hwnd,IDC_CROSSREF_PREFIXSTYLECHECK,cfp->suppressifbodystyle);
				th = GetDlgItem(hwnd,IDC_CROSSREF_TAB);
				memset(&tc,0,sizeof(tc));
				tc.mask = TCIF_TEXT;
				for (count = 0; count < CR_NSIZE; count++)	{		/* build tabs */
					tc.pszText = cname[count]; /* tab title */
					TabCtrl_InsertItem(th,count,&tc);
				}
				TabCtrl_SetCurSel(th,0);
				setctabs(hwnd,th,cfp);
				Edit_LimitText(GetDlgItem(hwnd,IDC_CROSSREF_XLEAD),FMSTRING-1);
				Edit_LimitText(GetDlgItem(hwnd,IDC_CROSSREF_XALEAD),FMSTRING-1);
				Edit_LimitText(GetDlgItem(hwnd,IDC_CROSSREF_XTRAIL),FMSTRING-1);
				Edit_LimitText(GetDlgItem(hwnd,IDC_CROSSREF_XATRAIL),FMSTRING-1);
				type_settextboxfont(GetDlgItem(hwnd,IDC_CROSSREF_XLEAD));
				type_settextboxfont(GetDlgItem(hwnd,IDC_CROSSREF_XALEAD));
				type_settextboxfont(GetDlgItem(hwnd,IDC_CROSSREF_XTRAIL));
				type_settextboxfont(GetDlgItem(hwnd,IDC_CROSSREF_XATRAIL));
				centerwindow(hwnd,1);
			}
			return FALSE;
		case WM_COMMAND:
			if (HIWORD(wParam) == EN_UPDATE)	{
				WORD id = LOWORD(wParam);
				int length = 0;

				if (id == IDC_CROSSREF_XLEAD || id == IDC_CROSSREF_XALEAD ||
					id == IDC_CROSSREF_XTRAIL || id == IDC_CROSSREF_XATRAIL)
					length = FMSTRING;
				checktextfield((HWND)lParam,length);
				return TRUE;
			}
			switch (LOWORD(wParam))	{
				case IDOK:
					getctabs(hwnd,GetDlgItem(hwnd,IDC_CROSSREF_TAB),cfp);
					cfp->suppressall = (char)isitemchecked(hwnd,IDC_CROSSREF_SUPPRESS);
					cfp->sortcross = (char)isitemchecked(hwnd,IDC_CROSSREF_ORDER);
					cfp->suppressifbodystyle = (char)isitemchecked(hwnd,IDC_CROSSREF_PREFIXSTYLECHECK);
				case IDCANCEL:
					EndDialog(hwnd,LOWORD(wParam) == IDOK ? TRUE : FALSE);
					return TRUE;
				case IDC_CROSSREF_PSTYLE:
					getstyle(hwnd,&cfp->leadstyle, FC_AUTO);
					return (TRUE);
				case IDC_CROSSREF_BSTYLE:
					getstyle(hwnd,&cfp->bodystyle, FC_TITLE);
					return(TRUE);
			}
			break;
		case WM_NOTIFY:
			switch (LOWORD(wParam))	{
				case IDC_CROSSREF_TAB:
					if (((LPNMHDR)lParam)->code == TCN_SELCHANGING)	
						getctabs(hwnd,((LPNMHDR)lParam)->hwndFrom,cfp);
					else if (((LPNMHDR)lParam)->code == TCN_SELCHANGE)
						setctabs(hwnd,((LPNMHDR)lParam)->hwndFrom,cfp);
					break;
				default:
					;
			}
			break;
		case WM_HELP:
			return (dodialoghelp(hwnd,TEXT("_Style and Layout\\Format_crossreferences.htm"),(HELPINFO *)lParam,wh_crossid));
	}
	return (FALSE);
}
/*******************************************************************************/
static void setctabs(HWND hwnd, HWND th,CROSSREFFORMAT *cfp)	/* sets cross-ref tab items */

{
	int tindex;

	tindex = TabCtrl_GetCurSel(th);
	setDItemText(hwnd,IDC_CROSSREF_XLEAD,cfp->level[tindex].cleadb);
	setDItemText(hwnd,IDC_CROSSREF_XTRAIL,cfp->level[tindex].cendb);
	setDItemText(hwnd,IDC_CROSSREF_XALEAD,cfp->level[tindex].cleada);
	setDItemText(hwnd,IDC_CROSSREF_XATRAIL,cfp->level[tindex].cenda);
	ComboBox_SetCurSel(GetDlgItem(hwnd,IDC_CROSSREF_XPOSITION),tindex ? cfp->subseeposition : cfp->mainseeposition);
	ComboBox_SetCurSel(GetDlgItem(hwnd,IDC_CROSSREF_XAPOSITION),tindex ? cfp->subposition : cfp->mainposition);
}
/*******************************************************************************/
static void getctabs(HWND hwnd, HWND th,CROSSREFFORMAT *cfp)	/* gets cross-ref tab items */

{
	int tindex;

	tindex = TabCtrl_GetCurSel(th);
	getDItemText(hwnd,IDC_CROSSREF_XLEAD,cfp->level[tindex].cleadb,FMSTRING);
	getDItemText(hwnd,IDC_CROSSREF_XTRAIL,cfp->level[tindex].cendb,FMSTRING);
	getDItemText(hwnd,IDC_CROSSREF_XALEAD,cfp->level[tindex].cleada,FMSTRING);
	getDItemText(hwnd,IDC_CROSSREF_XATRAIL,cfp->level[tindex].cenda,FMSTRING);
	if (tindex)	{
		cfp->subseeposition = ComboBox_GetCurSel(GetDlgItem(hwnd,IDC_CROSSREF_XPOSITION));
		cfp->subposition = ComboBox_GetCurSel(GetDlgItem(hwnd,IDC_CROSSREF_XAPOSITION));
	}
	else	{
		cfp->mainseeposition = ComboBox_GetCurSel(GetDlgItem(hwnd,IDC_CROSSREF_XPOSITION));
		cfp->mainposition = ComboBox_GetCurSel(GetDlgItem(hwnd,IDC_CROSSREF_XAPOSITION));
	}
}
/*******************************************************************************/
void fs_pagerefs(HWND hwnd)	/* sets up page references */

{
	struct locgroup lg;
	INDEX * FF;
	
	if (hwnd)	{
		FF = WX(hwnd,owner);
		lg.lf = FF->head.formpars.ef.lf;
	}
	else
		lg.lf = g_prefs.formpars.ef.lf;
	if (DialogBoxParam(g_hinst,MAKEINTRESOURCE(IDD_PAGEREFS),g_hwframe,prefproc,(LPARAM)&lg))	{
		if (hwnd)	{
			FF->head.formpars.ef.lf = lg.lf;
			view_redisplay(FF,0,VD_CUR|VD_RESET);	/* force reset of display pars */
			index_markdirty(FF);
		}
		else
			g_prefs.formpars.ef.lf = lg.lf;
	}
}
/******************************************************************************/
static INT_PTR CALLBACK prefproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)

{
	LOCATORFORMAT *lfp;
	HWND cbh, hwc;
	int count;

	lfp = getdata(hwnd);
	switch (msg)	{

		case WM_INITDIALOG:
			if (lfp = setdata(hwnd,(void *)lParam))	{	/* set data */
				cbh = GetDlgItem(hwnd,IDC_PAGEREFS_CONFLATE);
				for (count = 0; count < PR_CSIZE; count++)
					ComboBox_AddString(cbh,pcon[count]);
				ComboBox_SetCurSel(cbh,lfp->conflate);
				hwc = GetDlgItem(hwnd,IDC_PAGEREFS_CONNECT);
//				((struct locgroup *)lfp)->fh = type_makewindowfont(hwc,g_basefont);
	//			SendMessage(hwc,WM_SETFONT,(WPARAM)((struct locgroup *)lfp)->fh,MAKELPARAM(TRUE,0));	/* set font */
				type_settextboxfont(hwc);
				Edit_LimitText(hwc,FMSTRING-1);
				Edit_LimitText(GetDlgItem(hwnd,IDC_PAGEREFS_SINGLE),FMSTRING-1);
				Edit_LimitText(GetDlgItem(hwnd,IDC_PAGEREFS_MULTIPLE),FMSTRING-1);
				Edit_LimitText(GetDlgItem(hwnd,IDC_PAGEREFS_AFTER),FMSTRING-1);
				Edit_LimitText(GetDlgItem(hwnd,IDC_PAGEREFS_SUPPRESSTO),FMSTRING-1);
				Edit_LimitText(GetDlgItem(hwnd,IDC_PAGEREFS_CONCAT),FMSTRING-1);
				type_settextboxfont(GetDlgItem(hwnd,IDC_PAGEREFS_SINGLE));
				type_settextboxfont(GetDlgItem(hwnd,IDC_PAGEREFS_MULTIPLE));
				type_settextboxfont(GetDlgItem(hwnd,IDC_PAGEREFS_AFTER));
				type_settextboxfont(GetDlgItem(hwnd,IDC_PAGEREFS_SUPPRESSTO));
				type_settextboxfont(GetDlgItem(hwnd,IDC_PAGEREFS_CONCAT));
				setDItemText(hwnd,IDC_PAGEREFS_CONNECT,lfp->connect);
				setDItemText(hwnd,IDC_PAGEREFS_SINGLE,lfp->llead1);
				setDItemText(hwnd,IDC_PAGEREFS_MULTIPLE,lfp->lleadm);
				setDItemText(hwnd,IDC_PAGEREFS_AFTER,lfp->trail);
				setDItemText(hwnd,IDC_PAGEREFS_SUPPRESSTO,lfp->suppress);
				setDItemText(hwnd,IDC_PAGEREFS_CONCAT,lfp->concatenate);
				checkitem(hwnd,IDC_PAGEREFS_SORT,lfp->sortrefs);
				checkitem(hwnd,IDC_PAGEREFS_JUSTIFY,lfp->rjust);
				checkitem(hwnd,IDC_PAGEREFS_LEADER,lfp->leader);
				if (!lfp->rjust)			/* if not justifying */
					disableitem(hwnd,IDC_PAGEREFS_LEADER);
				checkitem(hwnd,IDC_PAGEREFS_SUPPRESSALL,lfp->suppressall);
				if (lfp->sortrefs)
					checkitem(hwnd, IDC_PAGEREFS_HIDEDUPLICATES, lfp->noduplicates);
				else
					disableitem(hwnd, IDC_PAGEREFS_LEADER);
				checkitem(hwnd,IDC_PAGEREFS_SUPPRESS,lfp->suppressparts);
//				CheckRadioButton(hwnd,IDC_PAGEREFS_NOABBREV,IDC_PAGEREFS_HART,IDC_PAGEREFS_NOABBREV+lfp->abbrevrule);
				cbh = GetDlgItem(hwnd,IDC_PAGEREFS_NOABBREV);
				for (count = 0; count < PR_ABSIZE; count++)
					ComboBox_AddString(cbh,pabbrev[count]);
				ComboBox_SetCurSel(cbh,lfp->abbrevrule);
				centerwindow(hwnd,1);
			}
			return FALSE;
		case WM_COMMAND:
			if (HIWORD(wParam) == EN_UPDATE)	{
				WORD id = LOWORD(wParam);
				int length = 0;

				if (id == IDC_PAGEREFS_CONNECT || id == IDC_PAGEREFS_SINGLE || id == IDC_PAGEREFS_MULTIPLE ||
					id == IDC_PAGEREFS_AFTER || id == IDC_PAGEREFS_SUPPRESSTO || id == IDC_PAGEREFS_CONCAT)
					length = FMSTRING;
				checktextfield((HWND)lParam,length);
				return TRUE;
			}
			switch (LOWORD(wParam)) {
			case IDOK:
				getDItemText(hwnd, IDC_PAGEREFS_CONNECT, lfp->connect, FMSTRING);
				getDItemText(hwnd, IDC_PAGEREFS_SINGLE, lfp->llead1, FMSTRING);
				getDItemText(hwnd, IDC_PAGEREFS_MULTIPLE, lfp->lleadm, FMSTRING);
				getDItemText(hwnd, IDC_PAGEREFS_AFTER, lfp->trail, FMSTRING);
				getDItemText(hwnd, IDC_PAGEREFS_SUPPRESSTO, lfp->suppress, FMSTRING);
				getDItemText(hwnd, IDC_PAGEREFS_CONCAT, lfp->concatenate, FMSTRING);
				lfp->conflate = ComboBox_GetCurSel(GetDlgItem(hwnd, IDC_PAGEREFS_CONFLATE));
				lfp->sortrefs = (char)isitemchecked(hwnd, IDC_PAGEREFS_SORT);
				lfp->rjust = (char)isitemchecked(hwnd, IDC_PAGEREFS_JUSTIFY);
				lfp->leader = (char)isitemchecked(hwnd, IDC_PAGEREFS_LEADER);
				lfp->suppressall = (char)isitemchecked(hwnd, IDC_PAGEREFS_SUPPRESSALL);
				lfp->noduplicates = (char)isitemchecked(hwnd, IDC_PAGEREFS_HIDEDUPLICATES);
				lfp->suppressparts = (char)isitemchecked(hwnd, IDC_PAGEREFS_SUPPRESS);
//				lfp->abbrevrule = findgroupcheck(hwnd,IDC_PAGEREFS_NOABBREV,IDC_PAGEREFS_HART)-IDC_PAGEREFS_NOABBREV;
				lfp->abbrevrule = ComboBox_GetCurSel(GetDlgItem(hwnd, IDC_PAGEREFS_NOABBREV));
			case IDCANCEL:
//				DeleteFont(((struct locgroup *)lfp)->fh);
				EndDialog(hwnd, LOWORD(wParam) == IDOK ? TRUE : FALSE);
				return TRUE;
			case IDC_PAGEREFS_STYLE:
				getlocatorstyle(hwnd, lfp->lstyle);
				return TRUE;
			case IDC_PAGEREFS_SORT:
				if ((char)isitemchecked(hwnd, IDC_PAGEREFS_SORT))	// if sorting
					enableitem(hwnd, IDC_PAGEREFS_HIDEDUPLICATES);
				else {
					checkitem(hwnd, IDC_PAGEREFS_HIDEDUPLICATES, 0);
					disableitem(hwnd, IDC_PAGEREFS_HIDEDUPLICATES);
				}
				return TRUE;
			case IDC_PAGEREFS_JUSTIFY:
				if ((char)isitemchecked(hwnd,IDC_PAGEREFS_JUSTIFY))	/* if right justifying */
					enableitem(hwnd,IDC_PAGEREFS_LEADER);
				else
					disableitem(hwnd,IDC_PAGEREFS_LEADER);
				return TRUE;
			}
			break;
		case WM_HELP:
			return (dodialoghelp(hwnd,TEXT("_Style and Layout\\Format_pagereferences.htm"),(HELPINFO *)lParam,wh_pageid));
	}
	return (FALSE);
}
/*******************************************************************************/
static void getlocatorstyle(HWND hwnd,LSTYLE *lsp)	/* gets style settings */

{
	LSTYLE ls[COMPMAX];
	
	memcpy(ls,lsp,sizeof(ls));
	if (DialogBoxParam(g_hinst,MAKEINTRESOURCE(IDD_LOCATORSTYLE),hwnd,lstyleproc,(LPARAM)&ls))
		memcpy(lsp,ls,sizeof(ls));
}
/*******************************************************************************/
static INT_PTR CALLBACK lstyleproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)

{
	LSTYLE * sp;
	HWND th;
	TC_ITEM tc;
	int count;

	sp = getdata(hwnd);
	switch (msg)	{

		case WM_INITDIALOG:
			if (sp = setdata(hwnd,(void *)lParam))	{	/* set data */
				th = GetDlgItem(hwnd,IDC_LOCATORSTYLE_TAB);
				memset(&tc,0,sizeof(tc));
				tc.mask = TCIF_TEXT;
				for (count = 0; count < LS_NSIZE; count++)	{		/* build tabs */
					tc.pszText = lsname[count]; /* tab title */
					TabCtrl_InsertItem(th,count,&tc);
				}
				TabCtrl_SetCurSel(th,0);
				setltabs(hwnd,th,sp);
				centerwindow(hwnd,0);
			}
			return FALSE;
		case WM_COMMAND:
			switch (LOWORD(wParam))	{
				case IDOK:
					getltabs(hwnd,GetDlgItem(hwnd,IDC_LOCATORSTYLE_TAB),sp);
				case IDCANCEL:
					EndDialog(hwnd,LOWORD(wParam) == IDOK ? TRUE : FALSE);
					return TRUE;
			}
			break;
		case WM_NOTIFY:
			switch (LOWORD(wParam))	{
				case IDC_LOCATORSTYLE_TAB:
					if (((LPNMHDR)lParam)->code == TCN_SELCHANGING)	
						getltabs(hwnd,((LPNMHDR)lParam)->hwndFrom,sp);
					else if (((LPNMHDR)lParam)->code == TCN_SELCHANGE)
						setltabs(hwnd,((LPNMHDR)lParam)->hwndFrom,sp);
					break;
				default:
					;
			}
			break;
		case WM_HELP:
			return (dodialoghelp(hwnd,TEXT("_Style and Layout\\_Format_pagereftypography.htm"),(HELPINFO *)lParam,wh_locstyleid));
	}
	return (FALSE);
}
/*******************************************************************************/
static void setltabs(HWND hwnd, HWND th, LSTYLE *sp)	/* sets page ref style tab items */

{
	int tindex;
	LSTYLE * lsp;

	tindex = TabCtrl_GetCurSel(th);
	lsp = &sp[tindex];
	checkitem(hwnd,IDC_TEXTSTYLE_BOLD,lsp->loc.style&FX_BOLD);
	checkitem(hwnd,IDC_TEXTSTYLE_ITALIC,lsp->loc.style&FX_ITAL);
	checkitem(hwnd,IDC_TEXTSTYLE_ULINE,lsp->loc.style&FX_ULINE);
	checkitem(hwnd,IDC_TEXTSTYLE_SMALL,lsp->loc.style&FX_SMALL);
	checkitem(hwnd,IDC_LOCATORSTYLE_BOLD,lsp->punct.style&FX_BOLD);
	checkitem(hwnd,IDC_LOCATORSTYLE_ITALIC,lsp->punct.style&FX_ITAL);
	checkitem(hwnd,IDC_LOCATORSTYLE_ULINE,lsp->punct.style&FX_ULINE);
	checkitem(hwnd,IDC_LOCATORSTYLE_SMALL,lsp->punct.style&FX_SMALL);
}
/*******************************************************************************/
static void getltabs(HWND hwnd, HWND th,LSTYLE *sp)	/* gets page ref style tab items */

{
	int tindex;
	LSTYLE * lsp;

	tindex = TabCtrl_GetCurSel(th);
	lsp = &sp[tindex];
	lsp->loc.style = 0;
	if (isitemchecked(hwnd,IDC_TEXTSTYLE_BOLD))
		lsp->loc.style |= FX_BOLD;
	if (isitemchecked(hwnd,IDC_TEXTSTYLE_ITALIC))
		lsp->loc.style |= FX_ITAL;
	if (isitemchecked(hwnd,IDC_TEXTSTYLE_ULINE))
		lsp->loc.style |= FX_ULINE;
	if (isitemchecked(hwnd,IDC_TEXTSTYLE_SMALL))
		lsp->loc.style |= FX_SMALL;
	lsp->punct.style = 0;
	if (isitemchecked(hwnd,IDC_LOCATORSTYLE_BOLD))
		lsp->punct.style |= FX_BOLD;
	if (isitemchecked(hwnd,IDC_LOCATORSTYLE_ITALIC))
		lsp->punct.style |= FX_ITAL;
	if (isitemchecked(hwnd,IDC_LOCATORSTYLE_ULINE))
		lsp->punct.style |= FX_ULINE;
	if (isitemchecked(hwnd,IDC_LOCATORSTYLE_SMALL))
		lsp->punct.style |= FX_SMALL;
}
/*******************************************************************************/
void fs_styledstrings(HWND hwnd)	/* sets up styled strings */

{
	char ss[STYLESTRINGLEN];
	INDEX * FF;

	if (hwnd)	{
		FF = WX(hwnd,owner);
		str_xcpy(ss,FF->head.stylestrings);
	}
	else
		str_xcpy(ss,g_prefs.stylestrings);
	if (DialogBoxParam(g_hinst,MAKEINTRESOURCE(IDD_STYLEDSTRINGS),g_hwframe,ssproc,(LPARAM)ss))	{
		if (hwnd)	{
			str_xcpy(FF->head.stylestrings,ss);
			if (FF->head.privpars.vmode == VM_FULL)
				view_redisplay(FF,0,VD_CUR|VD_RESET);	/* force reset of display pars */
			index_markdirty(FF);
		}
		else
			str_xcpy(g_prefs.stylestrings,ss);
	}
}
/******************************************************************************/
static INT_PTR CALLBACK ssproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)

{
	HWND listwnd = GetDlgItem(hwnd,IDC_SS_LIST);
	char * ssp;
	int limit, index;
	LVITEM lv;
	static int elimit;

	ssp = getdata(hwnd);
	switch (msg)	{
		case WM_INITDIALOG:
			if (ssp = setdata(hwnd,(void *)lParam))	{	/* set data */
				RECT lr;
				LVCOLUMN lvc; 

				type_settextboxfont(listwnd);
				GetClientRect(listwnd,&lr);
				lvc.mask = LVCF_SUBITEM | LVCF_WIDTH;
				lvc.iSubItem = 0;
				lvc.cx = lr.right;
				ListView_InsertColumn(listwnd, 0, &lvc);
				limit = str_xcount(ssp);
				for (index = 0; index < limit; index++)	{
					char * sptr = str_xatindex(ssp,index);

					memset(&lv,0,sizeof(LVITEM));
					lv.mask = LVIF_TEXT|LVIF_PARAM;
					lv.iItem = index;
					lv.iSubItem = 0;
					lv.pszText = toNative(sptr+1);
					lv.lParam = (LPARAM)((unsigned)*sptr|FX_OFF);
					ListView_InsertItem(listwnd,&lv);
				}
				ListView_SetItemState(listwnd,0,LVIS_FOCUSED|LVIS_SELECTED,LVIS_FOCUSED|LVIS_SELECTED);
				centerwindow(hwnd,1);	
			}
			return FALSE;
		case WM_COMMAND:
			if (lParam == (LPARAM)ListView_GetEditControl(listwnd))	{	// if from edit control
				if (HIWORD(wParam) == EN_UPDATE)	// check that we've not overflowed
					checktextfield((HWND)lParam,elimit);
				return TRUE;
			}
			switch (LOWORD(wParam))	{
				case IDOK:
					if (!recoverstring(listwnd,ssp))	{
						senderr(ERR_NOROOM,WARN);
						return(TRUE);
					}
				case IDCANCEL:
					EndDialog(hwnd,LOWORD(wParam) == IDOK ? TRUE : FALSE);
					return TRUE;
				case IDC_SS_BOLD:
				case IDC_SS_ITALIC:
				case IDC_SS_ULINE:
				case IDC_SS_SMALL:
				case IDC_SS_NORMAL:
				case IDC_SS_SUPER:
				case IDC_SS_SUB:
					SetFocus(listwnd);
					getsstyle(hwnd,listwnd);
					return (TRUE);
				case IDC_SS_ADD:
					SetFocus(listwnd);
					memset(&lv,0,sizeof(LVITEM));
					lv.mask = LVIF_TEXT|LVIF_PARAM;
					lv.iItem = 0;
					lv.iSubItem = 0;
					lv.pszText = TEXT("");
					lv.lParam = (LPARAM)(FX_OFF);
					ListView_InsertItem(listwnd,&lv);
					ListView_EditLabel(listwnd,0);
					return (TRUE);
				case IDC_SS_DELETE:
					SetFocus(listwnd);
					index = ListView_GetSelectionMark(listwnd);	/* get item */
					ListView_DeleteItem(listwnd, index);	/* delete it */
					return (TRUE);
			}
			break;
		case WM_NOTIFY:
			switch (((NMHDR *)lParam)->code)	{
				case LVN_ITEMCHANGED:
					{
						NMLISTVIEW * lvnp = (NMLISTVIEW *)lParam;
						BOOL enabled = ListView_GetSelectedCount(listwnd) > 0;

						EnableWindow((GetDlgItem(hwnd,IDC_SS_BOLD)),enabled);
						EnableWindow((GetDlgItem(hwnd,IDC_SS_ITALIC)),enabled);
						EnableWindow((GetDlgItem(hwnd,IDC_SS_ULINE)),enabled);
						EnableWindow((GetDlgItem(hwnd,IDC_SS_SMALL)),enabled);
						EnableWindow((GetDlgItem(hwnd,IDC_SS_NORMAL)),enabled);
						EnableWindow((GetDlgItem(hwnd,IDC_SS_SUPER)),enabled);
						EnableWindow((GetDlgItem(hwnd,IDC_SS_SUB)),enabled);
						EnableWindow((GetDlgItem(hwnd,IDC_SS_DELETE)),enabled);
						if (lvnp->uChanged&LVIF_STATE)	{	// if state changed
							if (lvnp->uNewState&LVIS_FOCUSED)	{	// if has focus
								ListView_SetSelectionMark(listwnd,lvnp->iItem);
//								NSLog("Item: %d; getfocus: %d",lvnp->iItem,lvnp->uNewState&LVIS_FOCUSED);
								setsstyle(hwnd,listwnd);
							}
						}
					}
					break;
				case LVN_BEGINLABELEDIT:
					{
//						HWND ew = ListView_GetEditControl(listwnd);
//						recoverstring(listwnd,ssp);
//						elimit = STYLESTRINGLEN-str_xlen(ssp)-1+Edit_GetTextLength(ew);
//						NSLog("%d",elimit);
						return FALSE;
					}
				case LVN_ENDLABELEDIT:
					{
						NMLVDISPINFO * lvdi = (NMLVDISPINFO *)lParam;
						ListView_SetItem(listwnd,&lvdi->item);
					}
					return TRUE;
				case NM_DBLCLK:
					{
						NMITEMACTIVATE * nm = (NMITEMACTIVATE *)lParam;
						ListView_EditLabel(listwnd,nm->iItem);
					}
					return TRUE;
			}
			break;
		case WM_HELP:
			return (dodialoghelp(hwnd,TEXT("_Style and Layout\\_Format_styledstrings.htm"),(HELPINFO *)lParam,wh_sstringid));
	}
	return (FALSE);
}
/*******************************************************************************/
static BOOL recoverstring(HWND lhw, char * text)	// recovers styled string text

{
	int limit = ListView_GetItemCount(lhw);
	int index,count;
	char * sptr;

	for (index = 0, sptr = text; index < limit; index++)	{	/* for all items */
		TCHAR tstring[STYLESTRINGLEN];
		char *tsptr;
		LVITEM lv;

		memset(&lv,0,sizeof(LVITEM));
		lv.mask = LVIF_TEXT|LVIF_PARAM|LVIF_STATE;
		lv.iItem = index;
		lv.pszText = tstring;
		lv.iSubItem = 0;
		lv.cchTextMax = STYLESTRINGLEN;
		ListView_GetItem(lhw,&lv);
		tsptr = fromNative(tstring);
		count = strlen(tsptr);
//		NSLog("%s", tsptr);
		if (count)	{	// if not empty string
			if (sptr+count+2 < text+STYLESTRINGLEN-1)	{	/* if room to add */
				*sptr++ = (unsigned char)lv.lParam;
				strcpy(sptr,tsptr);
				sptr += count + 1;
			}
			else
				return FALSE;
		}
	}
	*sptr = EOCS;
	return TRUE;
}
/*******************************************************************************/
static void setsstyle(HWND hwnd, HWND th)	/* displays styles for styled string */

{
	int tindex = ListView_GetSelectionMark(th);
	int pos = 0, style = 0;

	if (tindex >= 0)	{
		LVITEM lv;

		memset(&lv,0,sizeof(LVITEM));
		lv.mask = LVIF_PARAM;
		lv.iItem = tindex;
		lv.iSubItem = 0;
		ListView_GetItem(th,&lv);
		style = (unsigned char)lv.lParam;
	}
//	NSLog("%d",tindex);
	checkitem(hwnd,IDC_SS_BOLD,style&FX_BOLD);
	checkitem(hwnd,IDC_SS_ITALIC,style&FX_ITAL);
	checkitem(hwnd,IDC_SS_ULINE,style&FX_ULINE);
	checkitem(hwnd,IDC_SS_SMALL,style&FX_SMALL);
	if (style&FX_SUPER)
		pos = 1;
	else if (style&FX_SUB)
		pos = 2;
	CheckRadioButton(hwnd,IDC_SS_NORMAL,IDC_SS_SUB,IDC_SS_NORMAL+pos);
}
/*******************************************************************************/
static void getsstyle(HWND hwnd, HWND th)	/* retrieves styles for styled string */

{
	int tindex = ListView_GetSelectionMark(th);

	if (tindex >= 0)	{	/* if have item selected */
		LVITEM lv;
		int pos, style;

		memset(&lv,0,sizeof(LVITEM));
		lv.mask = LVIF_PARAM;
		lv.iItem = tindex;
		lv.iSubItem = 0;
		style = FX_OFF;
		if (isitemchecked(hwnd,IDC_SS_BOLD))
			style |= FX_BOLD;
		if (isitemchecked(hwnd,IDC_SS_ITALIC))
			style |= FX_ITAL;
		if (isitemchecked(hwnd,IDC_SS_ULINE))
			style |= FX_ULINE;
		if (isitemchecked(hwnd,IDC_SS_SMALL))
			style |= FX_SMALL;
		pos = findgroupcheck(hwnd,IDC_SS_NORMAL,IDC_SS_SUB)-IDC_SS_NORMAL;
		if (pos == 1)
			style |= FX_SUPER;
		else if (pos == 2)
			style |= FX_SUB;
		lv.lParam = (LPARAM)style;
		ListView_SetItem(th,&lv);
	}
}
/*******************************************************************************/
static void setfontcombo(HWND cbh, char * fname)	/* sets font in combo box */

{
	int index;

	if (!*fname || (index = ComboBox_FindStringExact(cbh, 1, toNative(fname))) == CB_ERR)		/* if default */ 
		index = 0;
	ComboBox_SetCurSel(cbh,index);
}
/*******************************************************************************/
static void getfontcombo(HWND cbh, char * fname)	/* gets font from combo box */

{
	if (!ComboBox_GetCurSel(cbh))
		*fname = '\0';
	else	{
		TCHAR tname[FSSTRING];
		GetWindowText(cbh,tname,FSSTRING);
		strcpy(fname,fromNative(tname));
	}
}
/*******************************************************************************/
static void getstyle(HWND hwnd,CSTYLE *stp, int extramode)	/* gets style settings */

{
	struct stylewrapper wrapper;
	wrapper.cs = *stp;
	wrapper.extramode = extramode;

	if (DialogBoxParam(g_hinst,MAKEINTRESOURCE(IDD_TEXTSTYLE),hwnd,styleproc,(LPARAM)&wrapper))
		*stp = wrapper.cs;
}
/*******************************************************************************/
static INT_PTR CALLBACK styleproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)

{
	struct stylewrapper *wp = getdata(hwnd);

	switch (msg)	{
		case WM_INITDIALOG:
			if (wp = setdata(hwnd,(void *)lParam))	{	/* set data */
				checkitem(hwnd,IDC_TEXTSTYLE_BOLD,wp->cs.style&FX_BOLD);
				checkitem(hwnd,IDC_TEXTSTYLE_ITALIC, wp->cs.style&FX_ITAL);
				checkitem(hwnd,IDC_TEXTSTYLE_ULINE, wp->cs.style&FX_ULINE);
				checkitem(hwnd,IDC_TEXTSTYLE_SMALL, wp->cs.style&FX_SMALL);
				if (wp->extramode)
					SetWindowText(GetDlgItem(hwnd, IDC_TEXTSTYLE_AUTO), wp->extramode == FC_AUTO ? TEXT("Auto") : TEXT("Title Case"));	/* set new title */
				else
					hideitem(hwnd,IDC_TEXTSTYLE_AUTO);
				int buttonindex = wp->cs.cap <= FC_UPPER ? wp->cs.cap : 3;
				CheckRadioButton(hwnd, IDC_TEXTSTYLE_NCAP, IDC_TEXTSTYLE_AUTO, IDC_TEXTSTYLE_NCAP + buttonindex);
				centerwindow(hwnd,0);
			}
			return FALSE;
		case WM_COMMAND:
			switch (LOWORD(wParam))	{
				case IDOK:
					wp->cs.style = 0;
					if (isitemchecked(hwnd,IDC_TEXTSTYLE_BOLD))
						wp->cs.style |= FX_BOLD;
					if (isitemchecked(hwnd,IDC_TEXTSTYLE_ITALIC))
						wp->cs.style |= FX_ITAL;
					if (isitemchecked(hwnd,IDC_TEXTSTYLE_ULINE))
						wp->cs.style |= FX_ULINE;
					if (isitemchecked(hwnd,IDC_TEXTSTYLE_SMALL))
						wp->cs.style |= FX_SMALL;
					wp->cs.cap = findgroupcheck(hwnd,IDC_TEXTSTYLE_NCAP,IDC_TEXTSTYLE_AUTO)-IDC_TEXTSTYLE_NCAP;
					if (wp->cs.cap == 3)
						wp->cs.cap = wp->extramode;
				case IDCANCEL:
					EndDialog(hwnd,LOWORD(wParam) == IDOK ? TRUE : FALSE);
					return TRUE;
			}
			break;
		case WM_HELP:
			return (dodialoghelp(hwnd,NULL,(HELPINFO *)lParam,wh_tstyleid));
	}
	return (FALSE);
}


