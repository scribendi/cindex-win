#include "stdafx.h"
#include <htmlhelp.h>
#include "commands.h"
#include "errors.h"
#include "records.h"
#include "search.h"
#include "sort.h"
#include "collate.h"
#include "strings.h"
#include "viewset.h"
#include "edit.h"
#include "util.h"
#include "group.h"
#include "registry.h"
#include "macros.h"
#include "files.h"
#include "utime.h"

static COMGROUP c_com[] = {
	IDM_FILE_NEW,IDM_FILE_NEW,IDM_FILE_NEW,mc_new,EIDLE|EVIEW|ERVIEW|ERSVIEW|EFIND|EREP|ESPELL|ETEXT|EABBREV|ECLIP,
	IDM_FILE_OPEN,IDM_FILE_OPEN,IDM_FILE_OPEN,mc_open,EIDLE|EVIEW|ERVIEW|ERSVIEW|EFIND|EREP|ESPELL|ETEXT|EABBREV|ECLIP,
	IDM_FILE_CLOSE,IDM_FILE_CLOSE,IDM_FILE_CLOSE,NULL,EVIEW|ERVIEW|ERSVIEW|EFIND|EREP|ESPELL|ETEXT|EABBREV|ECLIP|WPASS,
	IDM_FILE_SAVE,IDM_FILE_SAVE,IDM_FILE_SAVE,NULL,EVIEW|ETEXT|EABBREV|WPASS,
	IDM_FILE_SAVEAS,IDM_FILE_SAVEAS,IDM_FILE_SAVEAS,NULL,EVIEW|ETEXT|EABBREV|WPASS,
	IDM_FILE_SAVEACOPYAS,IDM_FILE_SAVEACOPYAS,IDM_FILE_SAVEACOPYAS, NULL,EVIEW|WPASS,
	IDM_FILE_REVERT,IDM_FILE_REVERT,IDM_FILE_REVERT, NULL,EVIEW|WPASS,
#ifdef PUBLISH
	IDM_FILE_STATIONERY,IDM_FILE_STATIONERY,IDM_FILE_STATIONERY,NULL,EIDLE|EVIEW,
#endif //PUBLISH
	IDM_FILE_SAVEFORMAT,IDM_FILE_SAVEFORMAT,IDM_FILE_SAVEFORMAT,NULL,EIDLE|EVIEW|WPASS,
	IDM_FILE_IMPORT,IDM_FILE_IMPORT,IDM_FILE_IMPORT,mc_import,EVIEW|WPASS,
	IDM_EDIT_NEWGROUP,IDM_EDIT_NEWGROUP,IDM_EDIT_NEWGROUP,NULL,EVSEL|EVFSEL|WPASS,
	IDM_EDIT_SAVEGROUP,IDM_EDIT_SAVEGROUP,IDM_EDIT_SAVEGROUP,NULL,EVIEW|WPASS,
	IDM_FILE_PAGESETUP, IDM_FILE_PAGESETUP, IDM_FILE_PAGESETUP,NULL,EVIEW|ETEXT|EABBREV|WPASS,
	IDM_FILE_PRINT,IDM_FILE_PRINT, IDM_FILE_PRINT,NULL,EVIEW|ETEXT|EABBREV|WPASS,
#ifdef PUBLISH
	IDM_FILE_RECENT1,IDM_FILE_RECENT1,IDM_FILE_RECENT12,NULL,EIDLE|EVIEW|ERVIEW|ERSVIEW|EFIND|EREP|ESPELL|ETEXT|EABBREV|ECLIP,
#else
	IDM_FILE_RECENT1,IDM_FILE_RECENT1,IDM_FILE_RECENT10,NULL,EIDLE|EVIEW|ERVIEW|ERSVIEW|EFIND|EREP|ESPELL|ETEXT|EABBREV|ECLIP,
#endif //PUBLISH
	IDM_FILE_EXIT,IDM_FILE_EXIT,IDM_FILE_EXIT,NULL,EIDLE|EVIEW|ERVIEW|ERSVIEW|EFIND|EREP|ESPELL|ETEXT|EABBREV|ECLIP,
	
	IDM_EDIT_UNDO,IDM_EDIT_UNDO,IDM_EDIT_UNDO,NULL,WPASS,
	IDM_EDIT_REDO,IDM_EDIT_REDO,IDM_EDIT_REDO,NULL,WPASS,
	IDM_EDIT_CUT,IDM_EDIT_CUT,IDM_EDIT_CUT,NULL,ESCONT|EFIND|EREP|ESPELL|WPASS,
	IDM_EDIT_COPY,IDM_EDIT_COPY,IDM_EDIT_COPY,NULL,ESCONT|EFIND|EREP|ESPELL|EVSEL|EVFSEL|WPASS,
	IDM_EDIT_PASTE,IDM_EDIT_PASTE,IDM_EDIT_PASTE,NULL,EFIND|EREP|ESPELL|WPASS,
	IDM_EDIT_CLEAR,IDM_EDIT_CLEAR,IDM_EDIT_CLEAR, NULL,ESCONT|EFIND|EREP|ESPELL|WPASS,
	IDM_EDIT_SELECTALL,IDM_EDIT_SELECTALL,IDM_EDIT_SELECTALL,NULL,EVIEW|ERVIEW|ERSVIEW|EABBREV|WPASS,
	IDM_EDIT_NEWRECORD,IDM_EDIT_NEWRECORD,IDM_EDIT_NEWRECORD,NULL,EVIEW|ERVIEW|ERSVIEW|WPASS,
	IDM_EDIT_EDITRECORD,IDM_EDIT_EDITRECORD,IDM_EDIT_EDITRECORD,NULL,EVSEL|EVFSEL|WPASS,
	IDM_EDIT_DUPLICATE,IDM_EDIT_DUPLICATE,IDM_EDIT_DUPLICATE,NULL,EVSEL|EVFSEL|WPASS,
	IDM_EDIT_DEMOTE,IDM_EDIT_DEMOTE,IDM_EDIT_DEMOTE,NULL,EVSEL|EVFSEL|WPASS,
	IDM_EDIT_DELETE,IDM_EDIT_DELETE,IDM_EDIT_DELETE,NULL,EVSEL|EVFSEL|ERVIEW|WPASS,
	IDM_EDIT_TAG,IDM_EDIT_TAG,IDM_EDIT_TAG,NULL,EVSEL|EVFSEL|ERVIEW|WPASS,
	IDM_EDIT_LABEL0,IDM_EDIT_LABEL_SWITCH,IDM_EDIT_LABEL0+FLAGLIMIT,NULL,EVSEL|EVFSEL|ERVIEW|ERSVIEW|WPASS,
	IDM_EDIT_FIND,IDM_EDIT_FIND,IDM_EDIT_FIND,NULL,EVIEW|ERVIEW|ERSVIEW|WPASS,
	IDM_EDIT_FINDAGAIN,IDM_EDIT_FINDAGAIN,IDM_EDIT_FINDAGAIN,NULL,EVIEW|WPASS,
	IDM_EDIT_REPLACE,IDM_EDIT_REPLACE,IDM_EDIT_REPLACE,NULL,EVIEW|WPASS,
	IDM_TOOLS_CHECKSPELLING,IDM_TOOLS_CHECKSPELLING,IDM_TOOLS_CHECKSPELLING,NULL,EVIEW|WPASS,
	IDM_EDIT_NEWABBREV,IDM_EDIT_NEWABBREV,IDM_EDIT_NEWABBREV,NULL,EABBREV|WPASS,
	IDM_EDIT_PREFERENCES,IDM_EDIT_PREFERENCES,IDM_EDIT_PREFERENCES,NULL,ENEVER,

	IDM_VIEW_GOTO,IDM_VIEW_GOTO,IDM_VIEW_GOTO,NULL,EVIEW|WPASS,
	IDM_VIEW_ALLRECORDS,IDM_VIEW_ALLRECORDS, IDM_VIEW_ALLRECORDS,NULL,EVIEW|WPASS,
	IDM_VIEW_GROUP,IDM_VIEW_GROUP_FIRST,IDM_VIEW_GROUP_LAST,mc_setgroup,EVIEW|WPASS,
	IDM_VIEW_TEMPORARYGROUP, IDM_VIEW_TEMPORARYGROUP, IDM_VIEW_TEMPORARYGROUP,NULL,EVIEW|WPASS,
	IDM_VIEW_NEWRECORDS, IDM_VIEW_NEWRECORDS, IDM_VIEW_NEWRECORDS,NULL,EVIEW|WPASS,
	IDM_VIEW_ATTRIBUTES,IDM_VIEW_ATTRIBUTES,IDM_VIEW_ATTRIBUTES,NULL,EVIEW|WPASS,
	IDM_VIEW_SHOWNUMBERS,IDM_VIEW_SHOWNUMBERS,IDM_VIEW_SHOWNUMBERS,mc_shownum,EVIEW|WPASS,
	IDM_VIEW_FULLFORMAT,IDM_VIEW_FULLFORMAT,IDM_VIEW_FULLFORMAT,NULL,EVIEW|WPASS,
	IDM_VIEW_DRAFTFORMAT,IDM_VIEW_DRAFTFORMAT,IDM_VIEW_DRAFTFORMAT,NULL,EVIEW|WPASS,
	IDM_VIEW_SUMMARY,IDM_VIEW_SUMMARY,IDM_VIEW_SUMMARY,mc_summary,EVIEW|WPASS,
	IDM_VIEW_UNFORMATTED,IDM_VIEW_UNFORMATTED,IDM_VIEW_UNFORMATTED,mc_unform,EVIEW|WPASS,
	IDM_VIEW_WRAPLINES,IDM_VIEW_WRAPLINES,IDM_VIEW_WRAPLINES,mc_wraplines,EVIEW|WPASS,
	IDM_VIEW_VIEWDEPTH,IDM_VIEW_VIEWDEPTH_FIRST,IDM_VIEW_VIEWDEPTH_FIRST+15,mc_hidebelow,EVIEW|WPASS,
	IDM_VIEW_SHOWSORTED,IDM_VIEW_SHOWSORTED,IDM_VIEW_SHOWSORTED,NULL,EVIEW|WPASS,
	IDM_COMBO_FONT,IDM_COMBO_FONT,IDM_COMBO_FONT, NULL,EVIEW|ERVIEW|ERSVIEW|EABBREV|WPASS,
	IDM_COMBO_SIZE,IDM_COMBO_SIZE,IDM_COMBO_SIZE, NULL,EVIEW|ERVIEW|ERSVIEW|EABBREV|WPASS,

	IDM_FONT_DEFAULT,IDM_FONT_DEFAULT,IDM_FONT_DEFAULT,NULL,ERVIEW|WPASS,
	IDM_STYLE_REGULAR,IDM_STYLE_REGULAR,IDM_STYLE_REGULAR,NULL,ERVIEW|EABBREV|WPASS,
	IDM_STYLE_BOLD,IDM_STYLE_BOLD,IDM_STYLE_BOLD,NULL,ERVIEW|EABBREV|WPASS,
	IDM_STYLE_ITALIC,IDM_STYLE_ITALIC,IDM_STYLE_ITALIC,NULL,ERVIEW|EABBREV|WPASS,
	IDM_STYLE_UNDERLINE,IDM_STYLE_UNDERLINE,IDM_STYLE_UNDERLINE,NULL,ERVIEW|EABBREV|WPASS,
	IDM_STYLE_SMALLCAPS,IDM_STYLE_SMALLCAPS,IDM_STYLE_SMALLCAPS,NULL,ERVIEW|EABBREV|WPASS,
	IDM_STYLE_SUPER,IDM_STYLE_SUPER,IDM_STYLE_SUPER,NULL,ERVIEW|EABBREV|WPASS,
	IDM_STYLE_SUB,IDM_STYLE_SUB,IDM_STYLE_SUB,NULL,ERVIEW|EABBREV|WPASS,
	IDM_STYLE_INITIALCAPS,IDM_STYLE_INITIALCAPS,IDM_STYLE_INITIALCAPS,NULL,ERVIEW|EABBREV|WPASS,
	IDM_STYLE_UPPERCASE,IDM_STYLE_UPPERCASE,IDM_STYLE_UPPERCASE,NULL,ERVIEW|EABBREV|WPASS,
	IDM_STYLE_LOWERCASE,IDM_STYLE_LOWERCASE,IDM_STYLE_LOWERCASE,NULL,ERVIEW|EABBREV|WPASS,
	IDM_FONT_SYMBOLS,IDM_FONT_SYMBOLS,IDM_FONT_SYMBOLS,NULL,ENEVER,
	IDM_ALIGNMENT,IDM_ALIGNMENT,IDM_ALIGNMENT,NULL,EVIEW|ERSVIEW|WPASS,
	IDM_ALIGNMENT_NATURAL,IDM_ALIGNMENT_NATURAL,IDM_ALIGNMENT_RIGHT,NULL,EVIEW|ERSVIEW|WPASS,

	IDM_DOCUMENT_MARGINSCOLUMNS,IDM_DOCUMENT_MARGINSCOLUMNS,IDM_DOCUMENT_MARGINSCOLUMNS,NULL,EIDLE|EVIEW|WPASS,
	IDM_DOCUMENT_HEADERSFOOTER,IDM_DOCUMENT_HEADERSFOOTER,IDM_DOCUMENT_HEADERSFOOTER,NULL,EIDLE|EVIEW|WPASS,
	IDM_DOCUMENT_GROUPINGENTRIES,IDM_DOCUMENT_GROUPINGENTRIES,IDM_DOCUMENT_GROUPINGENTRIES,NULL,EIDLE|EVIEW|WPASS,
	IDM_DOCUMENT_STYLELAYOUT,IDM_DOCUMENT_STYLELAYOUT,IDM_DOCUMENT_STYLELAYOUT,NULL,EIDLE|EVIEW|WPASS,
	IDM_DOCUMENT_HEADINGS,IDM_DOCUMENT_HEADINGS,IDM_DOCUMENT_HEADINGS,NULL,EIDLE|EVIEW|WPASS,
	IDM_DOCUMENT_CROSSREFERENCES,IDM_DOCUMENT_CROSSREFERENCES,IDM_DOCUMENT_CROSSREFERENCES,NULL,EIDLE|EVIEW|WPASS,
	IDM_DOCUMENT_PAGEREFERENCES,IDM_DOCUMENT_PAGEREFERENCES,IDM_DOCUMENT_PAGEREFERENCES,NULL,EIDLE|EVIEW|WPASS,
	IDM_DOCUMENT_STYLEDSTRINGS,IDM_DOCUMENT_STYLEDSTRINGS,IDM_DOCUMENT_STYLEDSTRINGS,NULL,EIDLE|EVIEW|WPASS,
	IDM_DOCUMENT_RECORDSTRUCTURE,IDM_DOCUMENT_RECORDSTRUCTURE,IDM_DOCUMENT_RECORDSTRUCTURE,NULL,EIDLE|EVIEW|WPASS,
	IDM_DOCUMENT_REFERENCESYNTAX,IDM_DOCUMENT_REFERENCESYNTAX,IDM_DOCUMENT_REFERENCESYNTAX,NULL,EIDLE|EVIEW|ERVIEW|ERSVIEW|WPASS,
	IDM_DOCUMENT_FLIPWORDS,IDM_DOCUMENT_FLIPWORDS,IDM_DOCUMENT_FLIPWORDS,NULL,EIDLE|EVIEW|ERVIEW|ERSVIEW|WPASS,

	IDM_TOOLS_VERIFYCROSSREFERENCES,IDM_TOOLS_VERIFYCROSSREFERENCES,IDM_TOOLS_VERIFYCROSSREFERENCES,NULL,EVIEW|WPASS,
	IDM_TOOLS_CHECKINDEX,IDM_TOOLS_CHECKINDEX,IDM_TOOLS_CHECKINDEX,NULL,EVIEW | WPASS,
	IDM_TOOLS_RECONCILEHEADINGS,IDM_TOOLS_RECONCILEHEADINGS,IDM_TOOLS_RECONCILEHEADINGS,NULL,EVIEW|WPASS,
	IDM_TOOLS_ALTERREFERENCES,IDM_TOOLS_ALTERREFERENCES,IDM_TOOLS_ALTERREFERENCES,NULL,EVIEW|WPASS,
	IDM_TOOLS_GENERATECROSSREFERENCES,IDM_TOOLS_GENERATECROSSREFERENCES,IDM_TOOLS_GENERATECROSSREFERENCES,NULL,EVIEW|WPASS,
	IDM_TOOLS_SORT,IDM_TOOLS_SORT,IDM_TOOLS_SORT,NULL,EIDLE|EVIEW|WPASS,
	IDM_TOOLS_COMPRESS,IDM_TOOLS_COMPRESS,IDM_TOOLS_COMPRESS,NULL,EVIEW|WPASS,
	IDM_TOOLS_EXPAND,IDM_TOOLS_EXPAND,IDM_TOOLS_EXPAND,NULL,EVIEW|WPASS,
	IDM_TOOLS_SPLITHEADINGS,IDM_TOOLS_SPLITHEADINGS,IDM_TOOLS_SPLITHEADINGS,NULL,EVIEW | WPASS,
	IDM_TOOLS_COUNTRECORDS,IDM_TOOLS_COUNTRECORDS,IDM_TOOLS_COUNTRECORDS,NULL,EVIEW|ERVIEW|ERSVIEW|WPASS,
	IDM_TOOLS_INDEXSTATISTICS,IDM_TOOLS_INDEXSTATISTICS,IDM_TOOLS_INDEXSTATISTICS,NULL,EVIEW|ERVIEW|ERSVIEW|WPASS,
	IDM_TOOLS_FONTS,IDM_TOOLS_FONTS,IDM_TOOLS_FONTS,NULL,EVIEW|WPASS,
//	IDM_VIEW_ABBREVIATIONS,IDM_VIEW_ABBREVIATIONS,IDM_VIEW_ABBREVIATIONS,NULL,EIDLE|EVIEW|ERVIEW|ERSVIEW|EFIND|EREP|ETEXT|ECLIP,
	IDM_TOOLS_ABBREV_ATTACH,IDM_TOOLS_ABBREV_ATTACH,IDM_TOOLS_ABBREV_ATTACH,NULL,EIDLE|EVIEW|ERVIEW|ERSVIEW|EFIND|EREP|ETEXT|ECLIP,
	IDM_TOOLS_ABBREV_DETACH,IDM_TOOLS_ABBREV_DETACH,IDM_TOOLS_ABBREV_DETACH,NULL,EIDLE|EVIEW|ERVIEW|ERSVIEW|EFIND|EREP|ETEXT|ECLIP,
	IDM_TOOLS_ABBREV_NEW,IDM_TOOLS_ABBREV_NEW,IDM_TOOLS_ABBREV_NEW,NULL,EIDLE|EVIEW|ERVIEW|ERSVIEW|EFIND|EREP|ETEXT|ECLIP,
	IDM_TOOLS_ABBREV_EDIT,IDM_TOOLS_ABBREV_EDIT,IDM_TOOLS_ABBREV_EDIT,NULL,EIDLE|EVIEW|ERVIEW|ERSVIEW|EFIND|EREP|ETEXT|ECLIP,
	IDM_TOOLS_HOTKEYS,IDM_TOOLS_HOTKEYS,IDM_TOOLS_HOTKEYS,NULL,ENEVER,
	IDM_TOOLS_MARKUPTAGS,IDM_TOOLS_MARKUPTAGS,IDM_TOOLS_MARKUPTAGS,NULL,EIDLE|EVIEW|ERVIEW|ERSVIEW,
//	IDM_TOOLS_RECORDKEY,IDM_TOOLS_RECORDKEY,IDM_TOOLS_RECORDKEY,NULL,EVIEW|ERVIEW|ERSVIEW,
//	IDM_TOOLS_PLAYKEY,IDM_TOOLS_PLAYKEY,IDM_TOOLS_PLAYKEY,NULL,EVIEW|ERVIEW|ERSVIEW,
	IDM_MACRO_1,IDM_MACRO_1,IDM_MACRO_10,NULL,EVIEW|ERVIEW|ERSVIEW,
	IDM_PMACRO_1,IDM_PMACRO_1,IDM_PMACRO_10,NULL,EVIEW|ERVIEW|ERSVIEW,
	IDM_TOOLS_GROUPS,IDM_TOOLS_GROUPS,IDM_TOOLS_GROUPS,NULL,EVIEW|WPASS,

	IDM_WINDOW_CLIPBOARD,IDM_WINDOW_CLIPBOARD,IDM_WINDOW_CLIPBOARD,NULL,ENEVER,
	IDM_WINDOW_CASCADE,IDM_WINDOW_CASCADE,IDM_WINDOW_CASCADE, NULL,EVIEW|ECLIP|EABBREV|ETEXT,
	IDM_WINDOW_TILE,IDM_WINDOW_TILE,IDM_WINDOW_TILE,NULL,EVIEW|ECLIP|EABBREV|ETEXT,
	IDM_WINDOW_CLOSEALL,IDM_WINDOW_CLOSEALL,IDM_WINDOW_CLOSEALL, mc_closeall,EVIEW|ECLIP|EABBREV|ETEXT,

	IDM_HELP_TOPICS,IDM_HELP_TOPICS,IDM_HELP_TOPICS, NULL,ENEVER,
	IDM_HELP_ABOUT,IDM_HELP_ABOUT,IDM_HELP_ABOUT, mc_about,ENEVER,
	IDM_HELP_CHECKFORUPDATES,IDM_HELP_CHECKFORUPDATES,IDM_HELP_CHECKFORUPDATES,NULL,ENEVER,

	/* buttons */

	IDB_VIEW_INDENTED,IDB_VIEW_INDENTED,IDB_VIEW_INDENTED,NULL,EVIEW|WPASS,
	IDB_VIEW_RUNIN,IDB_VIEW_RUNIN,IDB_VIEW_RUNIN,NULL,EVIEW|WPASS,
	IDB_VIEW_SORTALPHA,IDB_VIEW_SORTALPHA,IDB_VIEW_SORTALPHA,NULL,EVIEW|WPASS,
	IDB_VIEW_SORTPAGE,IDB_VIEW_SORTPAGE,IDB_VIEW_SORTPAGE,NULL,EVIEW|WPASS,
	IDB_VIEW_SORTNONE, IDB_VIEW_SORTNONE, IDB_VIEW_SORTNONE, NULL, EVIEW | WPASS,
	IDB_MOD_PROPAGATE,IDB_MOD_PROPAGATE,IDB_MOD_PROPAGATE,NULL,ERVIEW|ERSVIEW|WPASS,
	IDB_MOD_BRACES,IDB_MOD_BRACES,IDB_MOD_BRACES,NULL,ERVIEW|ERSVIEW|WPASS,
	IDB_MOD_BRACKETS,IDB_MOD_BRACKETS,IDB_MOD_BRACKETS,NULL,ERVIEW|ERSVIEW|WPASS,
	IDB_MOD_FLIP,IDB_MOD_FLIP,IDB_MOD_FLIP,NULL,ERVIEW|ERSVIEW|WPASS,
	IDB_MOD_FLIPX,IDB_MOD_FLIPX,IDB_MOD_FLIPX,NULL,ERVIEW|ERSVIEW|WPASS,
	IDB_MOD_HALFFLIP,IDB_MOD_HALFFLIP,IDB_MOD_HALFFLIP,NULL,ERVIEW|ERSVIEW|WPASS,
	IDB_MOD_HALFFLIPX,IDB_MOD_HALFFLIPX,IDB_MOD_HALFFLIPX,NULL,ERVIEW|ERSVIEW|WPASS,
	IDB_MOD_SWAPPAREN,IDB_MOD_SWAPPAREN,IDB_MOD_SWAPPAREN,NULL,ERVIEW|ERSVIEW|WPASS,
	IDB_MOD_REVERT,IDB_MOD_REVERT,IDB_MOD_REVERT,NULL,ERVIEW|ERSVIEW|WPASS,
	IDB_MOD_DUPNEW,IDB_MOD_DUPNEW,IDB_MOD_DUPNEW,NULL,ERVIEW|ERSVIEW|WPASS,
	
	IDM_MOD_INCREMENT,IDM_MOD_INCREMENT,IDM_MOD_INCREMENT,NULL,ERVIEW|ERSVIEW|WPASS,
	IDM_MOD_DECREMENT,IDM_MOD_DECREMENT,IDM_MOD_DECREMENT,NULL,ERVIEW|ERSVIEW|WPASS,

	0,0,0,NULL,0L,
};

struct recentlist c_recent[MAXRECENT] = {	/* recent fields */
	0,{0},
	1,{0},
	2,{0},
	3,{0}
};

#ifdef PUBLISH
static TCHAR * c_techhelp = TEXT("Support\\tech_customersupport.htm");
#else
static TCHAR * c_techhelp = TEXT("Support\\tech_customersupport.htm");
#endif //PUBLISH

static short c_vform[] = {3,1,0,2};	/* menu order of the view formats */
static BOOL CALLBACK closechild(HWND hwnd,LPARAM arg);
static INT_PTR CALLBACK aboutproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);	/* handles About.. box */
static INT_PTR CALLBACK creditsproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);	/* handles About.. box */
static time_t parsedate(char * dstr);	/* converts date string to time */

/*******************************************************************************/
COMGROUP * com_findcommand(int comid)	/* returns structure ptr for command */

{
	COMGROUP *comptr;
	
	for (comptr = c_com; comptr->comid; comptr++)	{		/* for all commands in list */
		if (comid == comptr->comid || comid >= comptr->lowlimit && comid <= comptr->highlimit)		/* if found command */
			return(comptr);	/* return command struct */
	}
	return (NULL);
}
/*******************************************************************************/
void com_check(int comid, int onoff)	/* checks/unchecks menu items */

{
	HMENU mh;
	 
	mh = GetMenu(g_hwframe);
	CheckMenuItem(mh,comid, onoff ? MF_BYCOMMAND|MF_CHECKED : MF_BYCOMMAND|MF_UNCHECKED);	/* set it */
}
/*******************************************************************************/
void com_set(int comid, int onoff)	/* disables/enables menu items */

{
	HMENU mh;
	 
	mh = GetMenu(g_hwframe);
	EnableMenuItem(mh,comid,onoff ? MF_BYCOMMAND|MF_ENABLED : MF_BYCOMMAND|MF_GRAYED);
}
/*******************************************************************************/
void com_setname(int comid, TCHAR *name)	/* sets name of menu item */

{
	HMENU mh;
	MENUITEMINFO mi;
	
	mi.cbSize = sizeof(mi);
	mi.fMask = MIIM_TYPE;
	mi.fType = MFT_STRING;
	mi.dwTypeData = name;

	mh = GetMenu(g_hwframe);
	SetMenuItemInfo(mh,comid,FALSE,&mi);
//	EnableMenuItem(mh,comid,onoff ? MF_BYCOMMAND|MF_ENABLED : MF_BYCOMMAND|MF_GRAYED);
}
/*******************************************************************************/
void com_setenable(int selector, int mode, int onoff)	/* disables menus items by selection keys */

{
	register struct comgroup *comptr;
		
	HMENU mh;
	 
	mh = GetMenu(g_hwframe);
	for (comptr = c_com; comptr->comid; comptr++)		{		/* for all commands */
		if (comptr->mkeys != ENEVER)	{	/* if this item not protected */
			if (mode&(ONLY|XONLY) && comptr->mkeys&selector || mode&ALLBUT && !(comptr->mkeys&selector))
				EnableMenuItem(mh,comptr->comid,onoff ? MF_BYCOMMAND|MF_ENABLED : MF_BYCOMMAND|MF_GRAYED);
			if (mode&XONLY && !(comptr->mkeys&selector))	/* if wanting to affect complementary set */
				EnableMenuItem(mh,comptr->comid,!onoff ? MF_BYCOMMAND|MF_ENABLED : MF_BYCOMMAND|MF_GRAYED);
		}
	}
}
/******************************************************************************/
short com_getdates(short scope, char * first, char * last, time_c * low, time_c * high)		/* finds & parses date range */

{	
	int err = FALSE;

	*low = 0;
	*high = LONG_MAX;

	if (scope)	{	/* if not all dates */
		if (*first)	{
			*low = parsedate(first);
			if (!*low)
				err = 1;
		}
		if (*last)	{
			*high = parsedate(last);
			if (!*high)
				err = 2;
		}
		if (err)
			senderr(ERR_BADDATE,WARN);
		else if (*low >= *high)	{	/* bad range */
			senderr(ERR_BADDATERANGE,WARN);
			err = 2;
		}
	}
	return (err);
}
/******************************************************************************/
short com_getrecrange(INDEX * FF, short scope, char *lptr, char * hptr, RECN * first, RECN * last)		/* finds start and stop recs */

{
	RECORD * recptr, *curptr;
	
	*first = 0;			/* default first we'll never start at */
	*last = ULONG_MAX;			/* default one we'll never stop at */
	
	if (scope == COMR_RANGE)   {	  /* if setting range */
		if (*lptr)	{			/* if specifying start */
			if (!(*first = com_findrecord(FF,lptr,FALSE)))	/* bad low limit */
				return (-1);
		}
		if (*hptr)	{		/* want specifying last */
			if (recptr = rec_getrec(FF,com_findrecord(FF,hptr, TRUE))) 	{	/* if can get last matching record */
				if (curptr = LX(FF->vwind,skip)(FF,recptr,1))	{	/* if can get get beyond */
					*last = curptr->num; 	/* set it */
					if (*first && sort_relpos(FF,*first,*last) >= 0)	/* if last not after first */
						return (senderr(ERR_RECRANGERR,WARN));
				}
			}
			else
				return (1);		/* bad record spec (message already sent) */
		}
	}
	else if (scope == COMR_SELECT)	{		/* selection */
		*first = LX(FF->vwind,sel).first;
		*last = view_getsellimit(FF->vwind);
	}
	if (!*first && (recptr = sort_top(FF)))	/* if no first spec, and rec exists */
		*first = recptr->num;   	/* set it */
	return (FALSE);		/* ok */
}
#if 0
/******************************************************************************/
RECN com_findrecord(INDEX * FF, char *arg, short lastmatchflag)		/* finds record by lead or number */

{
	char astring[256], *tptr, *eptr;
	RECORD * recptr;
	RECN num;
	
	if (*arg)	{		/* if specifying a record */
		recptr = NULL;		/* set up to fail */
		strcpy(astring, *arg == ESCCHR ? arg+1 : arg);	/* make copy of search string */
		str_extend(astring);		/* make xstring */
		for (tptr = astring; tptr = strchr(tptr,';'); tptr++) {	/* for all semicolons */
			if (*(tptr-1) != '\\')		/* if not protected */	
				*tptr = '\0';
		}		/* NB: this loop will screw up semicolons that are part of a page ref for page sort lookup */
		if (isdigit(*arg))	{				/* if number */
			num = strtoul(astring,&eptr,10);
			if (!(recptr = search_findbynumber(FF,num)))
				senderronstatusline(num > FF->head.rtot ? ERR_RECNOTEXISTERR : ERR_RECNOTINVIEWERR, WARN,num);
		}
		else {
			if (!(recptr = search_findbylead(FF,astring)))	/* else by leads */
				senderronstatusline(ERR_RECMATCHERR, WARN, astring);
			else if (lastmatchflag)		/* if want to find last matching record */
				recptr = search_lastmatch(FF,recptr, astring,MATCH_TRUNCATED|MATCH_IGNORECODES|MATCH_IGNORECASE);
		}
		return (recptr ? recptr->num : 0);
	}
	return (LX(FF->vwind,lines)[0].rnum);
}
#else
/******************************************************************************/
RECN com_findrecord(INDEX * FF, char *arg, short lastmatchflag)		/* finds record by lead or number */

{
	char astring[256], *tptr, *eptr;
	RECORD * recptr;
	RECN num;
	
	if (*arg)	{		/* if specifying a record */
		recptr = NULL;		/* set up to fail */
		strcpy(astring, *arg == ESCCHR ? arg+1 : arg);	/* make copy of search string */
		str_extend(astring);		/* make xstring */
		for (tptr = astring; tptr = strchr(tptr,';'); tptr++) {	/* for all semicolons */
			if (*(tptr-1) != '\\')		/* if not protected */	
				*tptr = '\0';
		}		/* NB: this loop will screw up semicolons that are part of a page ref for page sort lookup */
		if (isdigit(*arg))	{				/* if number */
			num = strtoul(astring,&eptr,10);
			if (!(recptr = search_findbynumber(FF,num)))
				senderronstatusline(ERR_RECNOTEXISTERR, WARN,num);
		}
		else {
			if (!(recptr = search_findbylead(FF,astring)))	/* else by leads */
				senderronstatusline(ERR_RECMATCHERR, WARN, astring);
			else if (lastmatchflag)		/* if want to find last matching record */
				recptr = search_lastmatch(FF,recptr, astring,MATCH_LOOKUP|MATCH_IGNOREACCENTS|MATCH_IGNORECODES);
		}
		if (recptr)	{	// if have record
			if (sort_isignored(FF, recptr))
				senderronstatusline(ERR_RECNOTINVIEWERR, WARN,recptr->num);
			return recptr->num;
		}
	}
//	return (LX(FF->vwind,lines)[0].rnum);
	return 0;
}
#endif
/******************************************************************************/
void com_setbyindexstate(HWND wptr,HMENU hMenu, UINT item)		/* sets menu items by index context */

{
	INDEX * FF = getowner(wptr);
	HMENU mh = GetMenu(g_hwframe);
	int menuid = menu_getmid(hMenu);	
	
	if (hMenu == menu_gethandle(IDM_VIEW_GROUP))	/* if want to know about groups */
		grp_buildmenu(FF);
	mcr_setmenu(FALSE);		/* sets macro menus */
	if (menuid == IDPM_FILE)
		com_set(IDM_FILE_REVERT,*FF->backupspec && (FF->head.dirty || FF->wasdirty));
	if (!FF->groupcount)		{		/* if have no groups */
		com_set(IDM_VIEW_GROUP, OFF);	/* disable item */
		com_set(IDM_TOOLS_GROUPS,OFF);
	}
	CheckMenuRadioItem(mh,IDM_VIEW_ALLRECORDS,IDM_VIEW_TEMPORARYGROUP,IDM_VIEW_ALLRECORDS+FF->viewtype,MF_BYCOMMAND);
	if (!FF->lastfile)	{				/* if no temp group file */
		com_set(IDM_VIEW_TEMPORARYGROUP, OFF);	/* disable display of it */
		com_set(IDM_EDIT_SAVEGROUP,OFF);		/* disable make group */
	}
	else if (FF->viewtype != VIEW_TEMP)	/* if not viewing temp group */
		com_set(IDM_EDIT_SAVEGROUP,OFF);		/* can't save group */
	if (FF->curfile)	{			/* if viewing a group */
		com_check(IDM_VIEW_SHOWSORTED,TRUE);	/* imply sort on */
		com_set(IDM_VIEW_SHOWSORTED, OFF);		/* disable sort switch */
		com_set(IDM_TOOLS_COMPRESS, OFF);		/* disable squeeze */
		com_set(IDM_TOOLS_EXPAND, OFF);		/* disable expand */
		com_set(IDM_TOOLS_RECONCILEHEADINGS, OFF);			/* disable join */
	}
	else if (FF->viewtype == VIEW_NEW)	{	/* if showing new records */
		com_set(IDM_VIEW_SHOWSORTED, OFF);		/* disable sort switch */
		com_check(IDM_VIEW_SHOWSORTED,FALSE);	/* imply sort off */
		com_set(IDM_TOOLS_COMPRESS, OFF);		/* disable squeeze */
		com_set(IDM_TOOLS_EXPAND, OFF);		/* disable expand */
		com_set(IDM_TOOLS_RECONCILEHEADINGS, OFF);			/* disable join */
	}
	else	{		/* if showing all records */
		com_check(IDM_VIEW_SHOWSORTED,FF->head.sortpars.ison);	/* set sort menu */
	}
	if (menuid == IDPM_VIEW || menuid == IDPM_VIEWDEPTH)	{		/* if view menu */
		CheckMenuRadioItem(mh,IDM_VIEW_FULLFORMAT,IDM_VIEW_UNFORMATTED,IDM_VIEW_FULLFORMAT+c_vform[FF->head.privpars.vmode],MF_BYCOMMAND);
		com_check(IDM_VIEW_WRAPLINES,FALSE);	// assume no viewing unformatted
		if (FF->head.privpars.vmode == VM_FULL)	{
			com_set(IDM_VIEW_SHOWNUMBERS,OFF);
			com_set(IDM_VIEW_VIEWDEPTH,OFF);
			com_set(IDM_VIEW_WRAPLINES,OFF);
			com_check(IDM_VIEW_ATTRIBUTES,FF->head.privpars.filter.on);
		}
		else {
			int count;

			com_set(IDM_VIEW_ATTRIBUTES,OFF);
			com_check(IDM_VIEW_ATTRIBUTES,OFF);
			com_check(IDM_VIEW_SHOWNUMBERS,FF->head.privpars.shownum);
			if (menuid == IDPM_VIEWDEPTH)	{
				mh = menu_gethandle(IDM_VIEW_VIEWDEPTH);	/* set field display menu */
				menu_delall(mh);
				for (count = 0; count < FF->head.indexpars.maxfields; count++)	{	/* for all fields */
					if (count == FF->head.indexpars.maxfields-1)	/* if locator */
						count = PAGEINDEX;
					menu_additem(mh,IDM_VIEW_VIEWDEPTH_FIRST+count,FF->head.indexpars.field[count].name);		/* add menu item */
				}
				CheckMenuRadioItem(mh,0,FF->head.indexpars.maxfields-1,
					FF->head.privpars.hidebelow == ALLFIELDS ? FF->head.indexpars.maxfields-1 : FF->head.privpars.hidebelow-1, MF_BYPOSITION);
			}
			if (FF->head.privpars.vmode == VM_DRAFT)
				com_set(IDM_VIEW_WRAPLINES,OFF);
			else if (FF->head.privpars.vmode == VM_SUMMARY)	{
				com_set(IDM_VIEW_VIEWDEPTH,OFF);
				com_set(IDM_VIEW_WRAPLINES,OFF);
			}
			else			/* unformatted */
				com_check(IDM_VIEW_WRAPLINES,FF->head.privpars.wrap);
		}
		if (FF->startnum == FF->head.rtot)		/* if no new records */
			com_set(IDM_VIEW_NEWRECORDS, OFF);		/* disable new records */
	}
	if (FF->head.sortpars.fieldorder[0] == PAGEINDEX)	 {	/* if page sort  */
		com_set(IDM_TOOLS_RECONCILEHEADINGS,OFF);
		com_set(IDM_TOOLS_GENERATECROSSREFERENCES,OFF);
		com_set(IDM_TOOLS_VERIFYCROSSREFERENCES,OFF);
		com_set(IDM_TOOLS_CHECKINDEX, OFF);
	}
	if (FF->mf.readonly)	{			/* disable following for read-only index */
		com_set(IDM_FILE_REVERT,OFF);
		com_set(IDM_FILE_IMPORT,OFF);
		com_set(IDM_EDIT_UNDO,OFF);
		com_set(IDM_EDIT_REDO,OFF);
		com_set(IDM_EDIT_CUT,OFF);
		com_set(IDM_EDIT_CLEAR,OFF);
		com_set(IDM_EDIT_DEMOTE, OFF);
		com_set(IDM_EDIT_NEWRECORD,OFF);
		com_set(IDM_EDIT_REPLACE,OFF);
//		com_set(IDM_DOCUMENT_STYLEDSTRINGS, OFF);
		com_set(IDM_DOCUMENT_RECORDSTRUCTURE, OFF);
//		com_set(IDM_DOCUMENT_REFERENCESYNTAX, OFF);
//		com_set(IDM_DOCUMENT_FLIPWORDS, OFF);
		com_set(IDM_TOOLS_ALTERREFERENCES,OFF);
		com_set(IDM_TOOLS_CHECKSPELLING,OFF);
		com_set(IDM_TOOLS_RECONCILEHEADINGS,OFF);
		com_set(IDM_TOOLS_SPLITHEADINGS, OFF);
		com_set(IDM_TOOLS_SORT,OFF);
		com_set(IDM_TOOLS_COMPRESS,OFF);
		com_set(IDM_TOOLS_EXPAND,OFF);
		com_set(IDM_TOOLS_GENERATECROSSREFERENCES,OFF);
		com_set(IDM_TOOLS_GROUPS,OFF);
	};
#if 0
#if TOPREC < RECLIMIT || READER		/* if a demo copy */
		com_set(IDM_TOOLS_CHECKSPELLING,OFF);
#endif
#endif
}
/*******************************************************************************/
long mc_setgroup(HWND hwnd,int comid,HWND chandle,UINT notify)	/* finds group to use */

{
	TCHAR tbuff[GROUPLEN];
	HMENU mh;
	
	mh = menu_gethandle(IDM_VIEW_GROUP);
	menu_gettext(mh,comid,tbuff);
	SendMessage(hwnd,WMM_SETGROUP,0,(LPARAM)tbuff);	/* attach group name to message */
	return (0);
}
/************************************************************************/
long mc_closeall(HWND hwnd,int comid,HWND chandle,UINT notify)

{
	BOOL okflag = TRUE;

	EnumChildWindows(g_hwclient,closechild,(LPARAM)&okflag);
	return (okflag);
}
/************************************************************************/
static BOOL CALLBACK closechild(HWND hwnd,LPARAM okptr)

{
	if (GetParent(hwnd) == g_hwclient)	{	/* deal only with direct MDI windows */
		*((BOOL *)okptr) = SendMessage(hwnd,WM_CLOSE,0,0);	/* if can close down */
		return (*((BOOL *)okptr));		/* FALSE stops enumeration */
	}
	return (TRUE);
}
/******************************************************************************/
long mc_about(HWND hwnd,int comid,HWND chandle,UINT notify)	/* displays About box */

{
	DialogBox(g_hinst,MAKEINTRESOURCE(IDD_ABOUTCINDEX),g_hwframe,aboutproc);
	return (0);
}
/******************************************************************************/
static INT_PTR CALLBACK aboutproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)	/* handles About.. box */

{
	TCHAR path[MAX_PATH], *endpath, tstring[STSTRING];
	DWORD vsize, vzero;
	void * vdata;
	LPVOID valptr, val1ptr, val2ptr, val3ptr;
	UINT vlen, v1len,v2len,v3len;

	switch (msg)	{

		case WM_INITDIALOG:
			nstrcpy(path,GetCommandLine()+1);	/* get our path (trash leading ") */
			endpath = nstrchr(path,'\"');		/* and trailing " */
			*endpath = '\0';
			if (vsize = GetFileVersionInfoSize(path,&vzero))	{
				if (vdata = getmem(vsize))	{
					if (GetFileVersionInfo(path,0,vsize,vdata))	{
						VerQueryValue(vdata, TEXT("\\StringFileInfo\\040904E4\\FileDescription"),&valptr, &vlen);
						VerQueryValue(vdata, TEXT("\\StringFileInfo\\040904E4\\ProductVersion"),&val1ptr, &v1len);
						VerQueryValue(vdata, TEXT("\\StringFileInfo\\040904E4\\LegalCopyright"),&val2ptr, &v2len);
						VerQueryValue(vdata, TEXT("\\StringFileInfo\\040904E4\\FileVersion"),&val3ptr, &v3len);
						dprintf(hwnd,IDC_ABOUTCINDEX_VER,"%S\n%S  (%S)\n%S",valptr,val1ptr,val3ptr,val2ptr);
 
					
					}
					freemem(vdata);
				}
			}
#if TOPREC == RECLIMIT			/* if full version */
			vsize = STSTRING;
			if (reg_getmachinekeyvalue(NULL,TEXT("Owner"),tstring,&vsize) && *tstring)	/* if got value from registry */
				SetDlgItemText(hwnd,IDC_ABOUTCINDEX_USER,tstring);
			vsize = STSTRING;
			if (reg_getmachinekeyvalue(NULL,TEXT("Company"),tstring,&vsize) && *tstring)	/* if got value from registry */
				SetDlgItemText(hwnd,IDC_ABOUTCINDEX_ORG,tstring);
			vsize = STSTRING;
			if (reg_getmachinekeyvalue(NULL,TEXT("Sernum"),tstring,&vsize) && *tstring)	/* if got value from registry */
				SetDlgItemText(hwnd,IDC_ABOUTCINDEX_SER,tstring);
#endif
			centerwindow(hwnd,0);
			return TRUE;
		case WM_COMMAND:
			switch (LOWORD(wParam))	{
				case IDOK:
				case IDCANCEL:
					EndDialog(hwnd,0);
					return TRUE;
				case IDC_ABOUTCINDEX_SUPPORT:
					dowindowhelp(c_techhelp);
					return (TRUE);
				case IDC_ABOUTCINDEX_CREDITS:
					DialogBox(g_hinst,MAKEINTRESOURCE(IDD_CREDITS),hwnd,creditsproc);
					return (TRUE);
			}
			break;
		case WM_CLOSE:
			EndDialog(hwnd,0);
			return TRUE;
	}
	return FALSE;
}
/************************************************************************/
DWORD CALLBACK EditStreamCallback(DWORD_PTR dwCookie, LPBYTE lpBuff,LONG cb, PLONG pcb)
{
    HANDLE hFile = (HANDLE)dwCookie;
    if (ReadFile(hFile, lpBuff, cb, (DWORD *)pcb, NULL)) 
        return 0;
    return -1;
}
/******************************************************************************/
static INT_PTR CALLBACK creditsproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)	/* handles About.. box */

{
	TCHAR path[MAX_PATH];
	HANDLE hFile;

	switch (msg)	{
		case WM_INITDIALOG:
			{
				file_makecinpath(path,TEXT("notices.rtf"));
				hFile = CreateFile(path, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING,FILE_FLAG_SEQUENTIAL_SCAN, NULL);
				if (hFile != INVALID_HANDLE_VALUE) {
					EDITSTREAM es = { 0 };
					es.pfnCallback = EditStreamCallback;
					es.dwCookie = (DWORD_PTR)hFile;
					if (SendMessage(GetDlgItem(hwnd,IDC_CREDITTEXT), EM_STREAMIN, SF_RTF, (LPARAM)&es) && es.dwError == 0) {
						;
					}
					CloseHandle(hFile);
				}
			}
			centerwindow(hwnd,0);
			return TRUE;
		case WM_COMMAND:
			{
				switch (LOWORD(wParam))
					case IDOK:
					case IDCANCEL:
						EndDialog(hwnd, 0);
						return TRUE;
			}
	}
	return FALSE;
}
/************************************************************************/
void com_setdefaultbuttons(int simple)	/* sets default buttons */

{
	SendMessage(g_hwtoolbar,TB_ENABLEBUTTON,IDM_FILE_NEW,TRUE);
	SendMessage(g_hwtoolbar,TB_ENABLEBUTTON,IDM_FILE_OPEN,TRUE);
	SendMessage(g_hwtoolbar,TB_ENABLEBUTTON,IDM_FILE_SAVE,FALSE);		/* disable button */
	SendMessage(g_hwtoolbar,TB_ENABLEBUTTON,IDM_FILE_PRINT,FALSE);
	SendMessage(g_hwtoolbar,TB_ENABLEBUTTON,IDM_EDIT_COPY,FALSE);
	SendMessage(g_hwtoolbar,TB_ENABLEBUTTON,IDM_EDIT_CUT,FALSE);
	SendMessage(g_hwtoolbar,TB_ENABLEBUTTON,IDM_EDIT_PASTE,FALSE);
	SendMessage(g_hwtoolbar,TB_ENABLEBUTTON,IDM_EDIT_FIND,FALSE);
	SendMessage(g_hwtoolbar,TB_ENABLEBUTTON,IDM_EDIT_REPLACE,FALSE);
	SendMessage(g_hwtoolbar,TB_ENABLEBUTTON,IDM_EDIT_NEWRECORD,FALSE);
	SendMessage(g_hwtoolbar,TB_ENABLEBUTTON,IDM_EDIT_DELETE,FALSE);
	SendMessage(g_hwtoolbar,TB_ENABLEBUTTON,IDM_EDIT_TAG,FALSE);
	SendMessage(g_hwtoolbar,TB_ENABLEBUTTON,IDM_VIEW_ALLRECORDS,FALSE);
	SendMessage(g_hwtoolbar,TB_ENABLEBUTTON,IDM_VIEW_FULLFORMAT,FALSE);
	SendMessage(g_hwtoolbar,TB_ENABLEBUTTON,IDM_VIEW_DRAFTFORMAT,FALSE);
	SendMessage(g_hwtoolbar,TB_ENABLEBUTTON,IDB_VIEW_INDENTED,FALSE);
	SendMessage(g_hwtoolbar,TB_ENABLEBUTTON,IDB_VIEW_RUNIN,FALSE);
	SendMessage(g_hwtoolbar,TB_ENABLEBUTTON,IDB_VIEW_SORTALPHA,FALSE);
	SendMessage(g_hwtoolbar,TB_ENABLEBUTTON,IDB_VIEW_SORTPAGE,FALSE);
	SendMessage(g_hwtoolbar, TB_ENABLEBUTTON,IDB_VIEW_SORTNONE,FALSE);
	com_settextmenus(NULL,OFF,OFF);
	if (simple)	{
		SendMessage(g_hwstatus,SB_SIMPLE,TRUE,0);	/* restore simple status window */
		SendMessage(g_hwstatus,SB_SETTEXT,255|SBT_NOBORDERS,(LPARAM)TEXT("For help press F1 at any time"));	/* display on status line */
	}
}

/******************************************************************************/
void com_tbsaverestore(HWND hwtb,HWND parent, BOOL sflag, TCHAR * vname)	/* saves or retrieves tb */

{
	TBSAVEPARAMS tbs;

	if (tbs.hkr = reg_findkey(HKEY_CURRENT_USER,TEXT("")))	{
		tbs.pszSubKey = K_TOOLBARS;
		tbs.pszValueName = vname;
		SendMessage(hwtb,TB_SAVERESTORE,sflag,(LPARAM)&tbs);	/* TRUE if saving */
		if (!sflag)	{	/* if restoring */
#if 0
			if (parent != g_hwframe)
				SendMessage(parent,WMM_UPDATETOOLBARS,0,0);
			else
				com_setdefaultbuttons(TRUE);
#endif
		}
	}
}
/************************************************************************/
void com_settextmenus(INDEX * FF,int font,int size)	/* sets default menus */

{
	char tstring[20];

	if (FF)	{
		EnableWindow(g_hwfcb,font);	/* font name window */
		EnableWindow(g_hwscb,size);	/* font size window */
		ComboBox_SelectString(g_hwfcb,-1,toNative(FF->head.fm[0].name));		/* indicate default font */
		_itoa(FF->head.privpars.size,tstring,10);
		SetWindowText(g_hwscb,toNative(tstring));		/* indicate size */
	}
	else {
		EnableWindow(g_hwfcb,FALSE);	/* font name window */
		EnableWindow(g_hwscb,FALSE);	/* font size window */
#if 0
		ComboBox_SelectString(g_hwfcb,-1, toNative(g_prefs.gen.fm[0].name));	/* select default font */
		_itoa(g_prefs.privpars.size,tstring,10);
		SetWindowText(g_hwscb,toNative(tstring));		/* indicate size */
#else
		ComboBox_SetCurSel(g_hwfcb,-1);
		ComboBox_SetCurSel(g_hwscb,-1);
#endif
	}
}
/*********************************************************************************/
time_t parsedate(char * dstr)	/* converts date string to time */

{
	UDate date = time_dateValue(dstr);
//	NSLog(time_stringFromDate(date));

	return date/1000;
}
/*********************************************************************************/
void com_pushrecent(TCHAR * newpath)		/* manages recent file list */

{
	int index, olimit, itop, oldest;

	if (!g_shutdown)	{	/* if not shutting down */
		for (oldest = itop = index = 0; index < MAXRECENT; index++)	{
			if (!*c_recent[index].path)	{	/* if empty slot */
				nstrcpy(c_recent[index].path,newpath);
				olimit = index;
				break;
			}
			if (!nstricmp(newpath,c_recent[index].path))	{/* if a match */
				olimit = c_recent[index].order;		/* we'll reorder all those with values less than this */
				break;
			}
			if (c_recent[index].order > oldest)	{/* if this is older than the oldest we know */
				oldest = c_recent[index].order;
				itop = index;					/* get index of oldest */
			}
		}
		if (index == MAXRECENT)	{	/* if we're not yet in the list */
			index = itop;
			nstrcpy(c_recent[index].path,newpath);	/* place in oldest */
			olimit = MAXREC-1;
		}
		c_recent[index].order = -1;	/* we'll become first in the list */
		for (index = 0; index < MAXRECENT; index++)	{	/* now re-order */
			if (c_recent[index].order < olimit)		/* if one to be pushed further down the order */
				c_recent[index].order++;
		}
	}
}
#if 1
/******************************************************************************/
void com_setrecentfiles(HMENU mh)		/* sets recent file items in file menu */

{
	MENUITEMINFO mi;
	int icount, index, added;
	char tbuff[STSTRING],  keybuff[10], * tptr, *eptr, *xptr;
	TCHAR curdir[MAX_PATH];

#define M_RECENTFILE 716

	/* delete existing menu items */
	icount = GetMenuItemCount(mh);
	memset(&mi,0,sizeof(MENUITEMINFO));
	mi.cbSize = sizeof(MENUITEMINFO);
	mi.fMask = MIIM_DATA;
	index = icount-2;	/* point to item before Exit */
	do {			/* for all our recent file items */
		GetMenuItemInfo(mh,index,TRUE,&mi);
		if (mi.dwItemData == M_RECENTFILE)		/* if one of our file items */
			DeleteMenu(mh,index,MF_BYPOSITION);	/* delete it */
	} while (mi.dwItemData == M_RECENTFILE && index--);/* while we have file items */

	memset(&mi,0,sizeof(MENUITEMINFO));
	mi.cbSize = sizeof(MENUITEMINFO);
	mi.fMask = MIIM_ID|MIIM_TYPE|MIIM_DATA;
	mi.fType = MFT_STRING;
	mi.dwItemData = M_RECENTFILE;
	GetCurrentDirectory(MAX_PATH,curdir);
	for (added = index = 0; index < g_prefs.gen.recentlimit; index++)	{
		TCHAR * cp = com_findrecent(index);

		if (cp && *cp)	{
			char pptr[MAX_PATH];

			strcpy(pptr, fromNative(cp));
			if (pptr && (tptr = strrchr(pptr,'\\')))	{	/* parse filename */
				sprintf(keybuff, index < 9 ? "&%d " : "%d ",index+1);	/* Ctrl shortcut  for < 10 */
				if (_strnicmp(fromNative(curdir),pptr,tptr-pptr))	{/* if not in current directory */
					for (eptr = pptr; (xptr = strchr(eptr,'\\')) && xptr < pptr+26; eptr = xptr+1)
						;
					if (eptr == tptr)	/* no need to truncate */
						sprintf(tbuff,"%s%s",keybuff,pptr);
					else				/* truncate path */
						sprintf(tbuff,"%s%.*s\\... \\%s",keybuff,(int)(eptr-pptr-1),pptr,++tptr);
	//					PathCompactPathEx(tbuff,pptr,26,0);
				}
				else
					sprintf(tbuff,"%s%s",keybuff,++tptr);
				mi.dwTypeData = toNative(tbuff);
				mi.wID = index+IDM_FILE_RECENT1;				
				InsertMenuItem(mh,IDM_FILE_EXIT,FALSE,&mi);
				added = TRUE;
			}
		}
	}
	if (added)	{	/* if added any */
		mi.fMask = MIIM_TYPE|MIIM_DATA;
		mi.fType = MFT_SEPARATOR;
		InsertMenuItem(mh,IDM_FILE_EXIT,FALSE,&mi);
	}
}
#else
/******************************************************************************/
void com_setrecentfiles(HMENU mh)		/* sets recent file items in file menu */

{
	MENUITEMINFO mi;
	int icount, index, added;
	char * tptr, tbuff[STSTRING], *pptr, *eptr, *xptr;
	char curdir[MAX_PATH];

#define M_RECENTFILE 716

	/* delete existing menu items */
	icount = GetMenuItemCount(mh);
	memset(&mi,0,sizeof(MENUITEMINFO));
	mi.cbSize = sizeof(MENUITEMINFO);
	mi.fMask = MIIM_DATA;
	index = icount-2;	/* point to item before Exit */
	do {			/* for all our recent file items */
		GetMenuItemInfo(mh,index,TRUE,&mi);
		if (mi.dwItemData == M_RECENTFILE)		/* if one of our file items */
			DeleteMenu(mh,index,MF_BYPOSITION);	/* delete it */
	} while (mi.dwItemData == M_RECENTFILE && index--);/* while we have file items */

	memset(&mi,0,sizeof(MENUITEMINFO));
	mi.cbSize = sizeof(MENUITEMINFO);
	mi.fMask = MIIM_ID|MIIM_TYPE|MIIM_DATA;
	mi.fType = MFT_STRING;
	mi.dwItemData = M_RECENTFILE;
	GetCurrentDirectory(MAX_PATH,curdir);
	for (added = index = 0; index < MAXRECENT; index++)	{
		pptr = com_findrecent(index);
		if (pptr && (tptr = strrchr(pptr,'\\')))	{	/* parse filename */
			if (strnicmp(curdir,pptr,tptr-pptr))	{/* if not in current directory */
				for (eptr = pptr; (xptr = strchr(eptr,'\\')) && xptr < pptr+26; eptr = xptr+1)
					;
				if (eptr == tptr)	/* no need to truncate */
					sprintf(tbuff,"&%d %s",index+1,pptr);
				else				/* truncate path */
					sprintf(tbuff,"&%d %.*s\\... \\%s",index+1,eptr-pptr-1,pptr,++tptr);
			}
			else
				sprintf(tbuff,"&%d %s",index+1,++tptr);
			mi.dwTypeData = tbuff;
			mi.wID = index+IDM_FILE_RECENT1;				
			InsertMenuItem(mh,IDM_FILE_EXIT,FALSE,&mi);
			added = TRUE;
		}
	}
	if (added)	{	/* if added any */
		mi.fMask = MIIM_TYPE|MIIM_DATA;
		mi.fType = MFT_SEPARATOR;
		InsertMenuItem(mh,IDM_FILE_EXIT,FALSE,&mi);
	}
}
#endif
/******************************************************************************/
TCHAR * com_findrecent(int recentorder)		/* returns path for file in order */

{
	int index;

	for (index = 0; index < MAXRECENT; index++)	{
		if (c_recent[index].order == recentorder)/* if our target */
			return (c_recent[index].path);
	}
	return (NULL);
}

