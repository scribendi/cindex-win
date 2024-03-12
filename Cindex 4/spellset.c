#include "stdafx.h"
#include "strings.h"
#include "records.h"
#include "index.h"
#include "sort.h"
#include "errors.h"
#include "commands.h"
#include "files.h"
#include "viewset.h"
#include "util.h"
#include "spell.h"
#include "spellset.h"
#include "search.h"
#include "findset.h"
#include "modify.h"
#include "registry.h"
#include "locales.h"
#include "registry.h"

static const DWORD wh_spellid[] = {
	IDC_SPELL_ADD, HIDC_SPELL_ADD,
	IDC_SPELL_BADWORD, HIDC_SPELL_BADWORD,
	IDC_SPELL_CHANGE, HIDC_SPELL_CHANGE,
	IDC_SPELL_CHECKPAGE, HIDC_SPELL_CHECKPAGE,
	IDC_SPELL_DICTIONARY, HIDC_SPELL_DICTIONARY,
	IDC_SPELL_EDITDICLIST, HIDC_SPELL_EDITDICLIST,
	IDC_SPELL_FIELD, HIDC_SPELL_FIELD,
	IDC_SPELL_GOODLIST, HIDC_SPELL_GOODLIST,
	IDC_SPELL_IGNOREALL, HIDC_SPELL_IGNOREALL,
	IDC_SPELL_LANGUAGE, HIDC_SPELL_LANGUAGE,
	IDC_SPELL_MODREC, HIDC_SPELL_MODREC,
	IDC_SPELL_NEWDIC, HIDC_SPELL_NEWDIC,
	IDC_SPELL_NEWREC, HIDC_SPELL_NEWREC,
	IDC_SPELL_OPTIONLANGUAGE, HIDC_SPELL_OPTIONLANGUAGE,
	IDC_SPELL_OPTIONS, HIDC_SPELL_OPTIONS,
	IDC_SPELL_OPTIONSCAPS, HIDC_SPELL_OPTIONSCAPS,
	IDC_SPELL_OPTIONSCLEAR, HIDC_SPELL_OPTIONSCLEAR,
	IDC_SPELL_OPTIONSEDITPD, HIDC_SPELL_OPTIONSEDITPD,
	IDC_SPELL_OPTIONSPD, HIDC_SPELL_OPTIONSPD,
	IDC_SPELL_OPTIONSPDNEW, HIDC_SPELL_OPTIONSPDNEW,
	IDC_SPELL_OPTIONSSTR, HIDC_SPELL_OPTIONSSTR,
	IDC_SPELL_OPTIONSSUGG, HIDC_SPELL_OPTIONSSUGG,
	IDC_SPELL_START, HIDC_SPELL_START,
	IDC_SPELL_STOP, HIDC_SPELL_STOP,
	IDC_SPELL_SUGGEST, HIDC_SPELL_SUGGEST,

	IDC_FIND_ALL, HIDC_FIND_ALL,
	IDC_FIND_SELECTED, HIDC_FIND_SELECTED,
	IDC_FIND_RANGE, HIDC_FIND_RANGE,
	IDC_FIND_RANGESTART, HIDC_FIND_RANGE,
	IDC_FIND_RANGEEND, HIDC_FIND_RANGE,

	0,0
};

struct estruct {	/* struct for editing personal dictionary */
	TCHAR * path;
	int selcount;		/* count of total items */
};

typedef struct {		/* contains data/functions for window */
	WFLIST wstuff;			/* basic set of window functions */
	INDEX * lastindex;		/* ptr to last touched */
	LRESULT (CALLBACK *edproc)(HWND, UINT, WPARAM, LPARAM );	/* saved riched procedure */
	SPELL sps;	/* spelling control struct */
	long vacount;			/* view window activation count */
	short restart;			/* TRUE if changed some search parameter */
	short resume;			/* TRUE if resuming */
	short scope;			/* scope of search */
	RECN target;			/* target for replacement */
	short offset;			/* offset of match or search position */
	short mlength;			/* length of matching text */
	short hasalts;			/* TRUE if tried to generate alts */
	short clearignore;		/* TRUE if ignore list to be cleared (used in options) */
	struct numstruct *nptr;	/* numstruct array for resorting */
	HWND cbmain;			// main dictionary combo
	HWND cbuser;			// user dictionary combo
	BOOL reopen;			// reopen main dictionary
} SFLIST;

#define SX(WW,II) (((SFLIST *)GetWindowLongPtr(WW,GWLP_USERDATA))->II)	/* for addressing items */

static TCHAR sp_uext[] = TEXT("*.udc");
static TCHAR sp_udcext[] = TEXT(".udc");

static LRESULT CALLBACK spellproc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static void scommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);	/* does menu tasks */
static BOOL sinit(HWND hwnd, HWND hwndFocus, LPARAM lParam);	/* initializes dialog */
static LRESULT CALLBACK hook(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static void sactivate(HWND hwnd, UINT state, HWND hwndActDeact, BOOL fMinimized);	/* activates/deactivates */
static short sclose(HWND hwnd);		/* closes text window */
static void initspell(HWND hwnd, short resume);	/* clears things for new search */
static void setallbuttons(HWND hwnd);	/* sets all buttons */
static void setchangebuttons(HWND hwnd);	/* sets change buttons */
char * dochange(HWND dptr, INDEX * FF, RECORD * recptr, char * sptr, short *lptr, char * reptext);	 /* replaces */
static void clearalts(HWND hwnd);	/*  clears list */
BOOL spellcontinue(INDEX * FF, HWND hwnd, RECORD * recptr, char * sptr, SFLIST * sfp);
static void showbadword(HWND hwnd, char * sptr, short mlength);	/* sets up list */
static short showalts(HWND hwnd, char * tword);	/* sets up list of alts for bad word */
static short cleanupspell(HWND hwnd);	/* cleans up after finishing replacements */
static short scheck(INDEX * FF, HWND hwnd, SPELL * sp);		/* checks search parameters */
static char * vistarget(INDEX * FF, RECORD * recptr, char *sptr);	/* returns ptr if target vis */
static int buildlanguagemenu(HWND cbmain, TCHAR * name);	/* builds language dic menu */
static int buildusermenu(HWND cbuser, TCHAR * name);	/* builds user dic menu */
static HUNSPELL * openmaindic(SFLIST * sfp, char *locale);	// opens main dic
static void openuserdic(SFLIST *sfp, TCHAR * dicname);	/* opens user dic selected in combo box */
INT_PTR newdic(HWND hwnd);	/* makes permanent group from temp */
static INT_PTR CALLBACK dnameproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
INT_PTR editdic(HWND hwnd,TCHAR * path);	/* edits dictionary */
static INT_PTR CALLBACK deditproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
int options(HWND hwnd);	/* handles spelling options */
static INT_PTR CALLBACK optionproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static TCHAR * getdefaultdic(TCHAR * dickey);	// gets default dic name from registry
static void setdefaultdic(TCHAR * dickey, TCHAR * name);	// sets default dic name in registry

#define makedicpath(path,dicname) 	file_getdefpath(path,FOLDER_UDC,dicname)

/*********************************************************************************/
HWND spell_setwindow(HWND hwnd)	/*  sets up spell window */

{
	if (!g_spellw)
		g_spellw = CreateDialogParam(g_hinst,MAKEINTRESOURCE(IDD_SPELL),g_hwframe,spellproc,(LPARAM)hwnd);
	else
		SetActiveWindow(g_spellw);
	return (g_spellw);
}
/************************************************************************/
void sset_closeifused(INDEX * FF)	/* closes spell window if controlled by index */

{
	if (g_spellw && SX(g_spellw,lastindex) == FF)	/* if we control spell window */
			SendMessage(g_spellw,WM_COMMAND,IDCANCEL,0);	/* dispose of it */
}
/************************************************************************/
static LRESULT CALLBACK spellproc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)

{
	switch (msg)	{
		HANDLE_MSG(hwnd,WM_INITDIALOG,sinit);
#if 0
		HANDLE_MSG(hwnd,WM_CLOSE,sclose);
#endif
		HANDLE_MSG(hwnd,WM_ACTIVATE,sactivate);
		HANDLE_MSG(hwnd, WM_COMMAND,scommand);
		case WM_HELP:
			return (dodialoghelp(hwnd,TEXT("_Spelling\\Usingspelling.htm"),(HELPINFO *)lParam,wh_spellid));
	}
	return (FALSE);
}
/************************************************************************/
static void scommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)	/* does menu tasks */

{
	INDEX * FF;
	SPELL * sp;
	RECORD * recptr;
	char *sptr;
	int changed;
	INT_PTR titem;
	char badword[STSTRING], goodword[STSTRING];
	SFLIST * sfp;

	if ((sfp = getdata(hwnd)) && (FF = sfp->lastindex) && (hwndCtl || id == IDCANCEL))	{	/* if data, index, & child message */
	
		/* need following in case records moved between find and change */
		/* might leave dialog then come back */

		if (sfp->target && (recptr = rec_getrec(FF,sfp->target)))		/* if already have a target */
			sptr = recptr->rtext+sfp->offset;
		else	{
			sptr = NULL;
			recptr = NULL;
		}
		changed = FALSE;
		sp = &sfp->sps;		/* get ptr to spell struct */
		
		getDItemText(hwnd,IDC_SPELL_BADWORD,badword,STSTRING);
//		GetWindowText(GetDlgItem(hwnd,IDC_SPELL_GOODLIST),goodword,STSTRING);
		getDItemText(hwnd,IDC_SPELL_GOODLIST,goodword,STSTRING);

		switch (id)	{
			case IDC_SPELL_CHANGE:		/* replace this occurrence */
				if (sfp->nptr || (sfp->nptr = sort_setuplist(FF)))	{	/* if have/can set up structures */
					if (!hspell_spellword(goodword))	// if don't have new word in dic
						hspell_addword(goodword);
					dochange(hwnd,FF,recptr, sptr, &sfp->mlength, sp->doubleflag ? NULL : goodword); /* replaces */
				}
				if (!spellcontinue(FF,hwnd,recptr,sptr,sfp))	/* if no more in this record */
					changed = TRUE;		/* force new search */
				break;
			case IDC_SPELL_IGNOREALL:		/* IDC_SPELL_CHANGE passes through */
				if (!hspell_addignoredword(badword))
					break;
			case IDC_SPELL_START:			/* start/ignore button */
				if (!spellcontinue(FF,hwnd,recptr,sptr,sfp))	/* if no more in this record */
					changed = TRUE;		/* force new search */
				break;
			case IDCANCEL:
				sclose(hwnd);
				return;
			case IDC_SPELL_STOP:	/* when used explicitly, forces cleanup */
				cleanupspell(hwnd);			/* force resort of changed records */
				initspell(hwnd,FALSE);		/* force new search */
				{
					checkitem(hwnd,IDC_SPELL_NEWREC,0);
					checkitem(hwnd,IDC_SPELL_MODREC,0);
					checkitem(hwnd,IDC_SPELL_CHECKPAGE,0);
					enableitem(hwnd,IDC_SPELL_CHECKPAGE);	/* enable page field checking */
					setDItemText(hwnd,IDC_FIND_RANGESTART,g_nullstr);	/* clear text */
					setDItemText(hwnd,IDC_FIND_RANGEEND,g_nullstr);	/* clear text */
					CheckRadioButton(hwnd,IDC_FIND_ALL,IDC_FIND_RANGE, IDC_FIND_ALL);
					ComboBox_SetCurSel(GetDlgItem(hwnd,IDC_SPELL_FIELD),0);
					sp->modflag = sp->newflag = sp->checkpage = 0;
					sp->field = ALLFIELDS;
					sfp->scope = COMR_ALL;
				}
				break;
			case IDC_SPELL_NEWREC:
				checkcontrol(hwndCtl, sp->newflag ^= 1);
				changed = TRUE;
				break;
			case IDC_SPELL_MODREC:
				checkcontrol(hwndCtl, sp->modflag ^= 1);
				changed = TRUE;
				break;
			case IDC_SPELL_CHECKPAGE:
				checkcontrol(hwndCtl, sp->checkpage ^= 1);
				changed = TRUE;
				break;
			case IDC_FIND_ALL:
			case IDC_FIND_SELECTED:
			case IDC_FIND_RANGE:
				if (id == IDC_FIND_RANGE)
					selectitext(hwnd,IDC_FIND_RANGESTART);
				if (sfp->scope != id-IDC_FIND_ALL)	{
					changed = TRUE;
					sfp->scope = id-IDC_FIND_ALL;
				}
				break;
			case IDC_FIND_RANGESTART:
			case IDC_FIND_RANGEEND:
				if (codeNotify == EN_CHANGE)	{
					CheckRadioButton(hwnd,IDC_FIND_ALL,IDC_FIND_RANGE, IDC_FIND_RANGE);
					changed = TRUE;
					sfp->scope = COMR_RANGE;
				}
				break;
			case IDC_SPELL_FIELD:
				if (codeNotify == CBN_SELENDOK)	{
					changed = TRUE;
					titem = fs_getfieldindex(hwndCtl);
					sp->field= titem;
					if (sp->field != PAGEINDEX && sp->field != ALLFIELDS)	{
						disableitem(hwnd,IDC_SPELL_CHECKPAGE);	/* disable page field checking */
						checkitem(hwnd,IDC_SPELL_CHECKPAGE,0);
					}
					else
						enableitem(hwnd,IDC_SPELL_CHECKPAGE);	/* enable page field checking */
				}
				break;
			case IDC_SPELL_LANGUAGE:
				if (codeNotify == CBN_SELENDOK)	{
					char * root = (char *)SendMessage(hwndCtl, CB_GETITEMDATA, ComboBox_GetCurSel(hwndCtl),0); 

					reg_deleteuserkey(K_SPELLPATHS);	// clear all existing aux dics
					openmaindic(sfp,root);	// open new
				}
				break;
			case IDC_SPELL_DICTIONARY:
				if (codeNotify == CBN_SELENDOK)	{
					TCHAR tstring[STSTRING];

					GetWindowText(hwndCtl,tstring,STSTRING);		// get whatever is now current main dic
					openuserdic(sfp,tstring);			/* opens new user dic if poss */
				}
				break;
			case IDC_SPELL_SUGGEST:		/* suggest */
				showalts(hwnd,badword);
				disableitem(hwnd,IDC_SPELL_SUGGEST);
				break;
			case IDC_SPELL_OPTIONS:
				options(hwnd);		/* get dictionary info */
				break;
			case IDC_SPELL_ADD:
				if ((titem = ComboBox_GetCurSel(GetDlgItem(hwnd,IDC_SPELL_DICTIONARY))) < 1)		{	/* if no dic in use */
					if (!sendwarning(WARN_NODIC))	/* if don't want new */
						break;
					titem = newdic(hwnd);	/* make dict if poss */
				}
				if (titem >= 0)	{
					if (hspell_addpdword(badword))	{
						if (!spellcontinue(FF,hwnd,recptr,sptr,sfp))	/* if no more in this record */
							changed = TRUE;		/* force new search */
					}
				}
				break;
			case IDC_SPELL_GOODLIST:
				if (codeNotify == CBN_SELENDOK || codeNotify == CBN_EDITCHANGE)
					setchangebuttons(hwnd);
				break;
			case IDC_SPELL_BADWORD:
//				SetWindowText(GetDlgItem(hwnd,IDC_SPELL_GOODLIST),badword);
				setDItemText(hwnd,IDC_SPELL_GOODLIST,badword);	// make bad word the good word [why?]
				setchangebuttons(hwnd);
				break;
		}
		if (changed && !sfp->restart)	{	/* if need to reset (and not already reset) */
			cleanupspell(hwnd);			/* force resort of changed records */
			initspell(hwnd,FALSE);		/* force new search */
		}
	}
}
/******************************************************************************/
static BOOL sinit(HWND hwnd, HWND hwndFocus, LPARAM lParam)	/* initializes dialog */

{
	SFLIST * sfp;

	if (sfp = getmem(sizeof(SFLIST)))	{	/* if can get memory for our window structure */
		setdata(hwnd,sfp);		/* install our private data */
		centerwindow(hwnd,-1);
		sfp->sps.lp = g_prefs.langpars;	/* install language prefs */
		sfp->sps.field = ALLFIELDS;		/* set index to allfields */
		sfp->cbmain = GetDlgItem(hwnd,IDC_SPELL_LANGUAGE);
		sfp->cbuser = GetDlgItem(hwnd,IDC_SPELL_DICTIONARY);
		hspell_init();
		buildlanguagemenu(sfp->cbmain,getdefaultdic(LANGUAGEDIC));
		buildusermenu(sfp->cbuser,getdefaultdic(USERDIC));		/* select correct dic */
		openmaindic(sfp,fromNative(getdefaultdic(LANGUAGEDIC)));
		if (sfp->sps.speller)	{	// if successfully opened
			(LONG_PTR)sfp->edproc = SetWindowLongPtr(GetDlgItem(hwnd,IDC_SPELL_BADWORD), GWLP_WNDPROC,(LONG_PTR)hook);	/* set subclass handler */
			/* nb: set hook here because then it doesn't have to be unset if there's a failure loading dictionaries */
			return (TRUE);
		}
		else
			senderr(ERR_SPELLNODBERR,WARN);
		freedata(hwnd);		/* window now has nothing */
	}
	DestroyWindow(hwnd);
	return (FALSE);
}
/************************************************************************/
static LRESULT CALLBACK hook(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)

{
	HWND parent;
	TCHAR tstring[STSTRING], tstring1[STSTRING];

	parent = GetParent(hwnd);
	if (msg == WM_SETCURSOR)	{		/* so that we can scroll prompt control */
		GetWindowText(hwnd,tstring,STSTRING);
		if (*tstring)	{		/* if a bad word displayed */
			GetWindowText(GetDlgItem(parent,IDC_SPELL_GOODLIST),tstring1,STSTRING);
			if (nstrcmp(tstring,tstring1))	{	/* if words differ */
				SetCursor(g_downarrow);
				return TRUE;
			}
		}
	}
	return CallWindowProc((WNDPROC)SX(parent,edproc),hwnd,msg,wParam,lParam);	/* pass to ordinary handler */
}
/************************************************************************/
static void sactivate(HWND hwnd, UINT state, HWND hwndActDeact, BOOL fMinimized)	/* activates/deactivates */

{
	INDEX * FF;
	SFLIST * sfp;

	sfp = getdata(hwnd);
	if (state != WA_INACTIVE)	{	/* being activated */
		g_mdlg = hwnd;
		if (FF = index_front())	{		/* if have any index window, and no record open */
			if (!FF->rwind)	{	// if no record window open
				enableitem(hwnd,IDC_SPELL_START);
				if (FF != sfp->lastindex)	{	/* if changed index in which searching */
					fs_setfieldmenu(FF, GetDlgItem(hwnd,IDC_SPELL_FIELD));	/* get current menu */
					sfp->lastindex = FF;
					initspell(hwnd,FALSE);			/* set up for new find */
					sfp->scope = COMR_ALL;
					CheckRadioButton(hwnd,IDC_FIND_ALL,IDC_FIND_RANGE, IDC_FIND_ALL);
				}
				else	{
					if (sfp->target)		{	/* if we have a target */
						if (FF->acount != sfp->vacount)	{	/* if have had view window active */
							initspell(hwnd,TRUE);			/* make resume settings */
#if 0
							view_selectrec(FF,sfp->target,VD_SELPOS,-1,-1);	/* show where starting */
#endif
						}
						else		/* just reset condition buttons */
							setallbuttons(hwnd);
//						enableitem(hwnd,IDC_SPELL_STOP);	/* enable stop */
					}
					else
						initspell(hwnd,FALSE);			/* set up for new find */
				}
				if (FF->startnum == FF->head.rtot)	{	/* if no new records */
					disableitem(hwnd,IDC_SPELL_NEWREC);
					checkitem(hwnd,IDC_SPELL_NEWREC,FALSE);
					sfp->sps.newflag = 0;
				}
				else
					enableitem(hwnd,IDC_SPELL_NEWREC);
				if (view_recordselect(FF->vwind))
					enableitem(hwnd,IDC_FIND_SELECTED);
				else	{
					disableitem(hwnd,IDC_FIND_SELECTED);
					if (sfp->scope == COMR_SELECT)	{	/* if had wanted selction */
						sfp->scope = COMR_ALL;		/* set for all */
						CheckRadioButton(hwnd,IDC_FIND_ALL,IDC_FIND_RANGE, IDC_FIND_ALL);
					}
				}
			}
		}
		else
			sfp->lastindex = NULL;
	}
	else {	/* being deactivated */
		g_mdlg = NULL;
		disableitem(hwnd,IDC_SPELL_START);	// don't spell
//		disableitem(hwnd,IDC_SPELL_STOP);
		disableitem(hwnd,IDC_SPELL_CHANGE);
		disableitem(hwnd,IDC_SPELL_ADD);
		disableitem(hwnd,IDC_SPELL_IGNOREALL);
		disableitem(hwnd,IDC_SPELL_SUGGEST);
		if (getdata(hwnd) && sfp->lastindex)	{
			cleanupspell(hwnd);				/* resort records */
			sfp->vacount = sfp->lastindex->acount;	/* save v window activation count */
		}
	}
}
/******************************************************************************/
static short sclose(HWND hwnd)		/* closes text window */

{
	cleanupspell(hwnd);
	g_prefs.langpars = SX(hwnd,sps).lp;	/* save language prefs */
	hspell_close();
	view_clearselect(SX(hwnd,lastindex)->vwind);
	SetWindowLongPtr(GetDlgItem(hwnd,IDC_SPELL_BADWORD), GWLP_WNDPROC,(LONG_PTR)SX(hwnd,edproc));
	freedata(hwnd);		/* window now has nothing */
	g_spellw = NULL;
	DestroyWindow(hwnd);	/* destroy it */
	return (0);
}
/******************************************************************************/
static void initspell(HWND hwnd, short resume)	/* clears things for new search */

{
	if (resume)	{
		SetWindowText(GetDlgItem(hwnd,IDC_SPELL_START),TEXT("Resume"));	/* set new title */
		SX(hwnd,resume) = TRUE;
	}
	else	{
		SX(hwnd,restart) = TRUE;
		SX(hwnd,target) = 0;			/* no record as target */
		SetWindowText(GetDlgItem(hwnd,IDC_SPELL_START),TEXT("Start"));	/* set new title */
//		disableitem(hwnd,IDC_SPELL_STOP);
	}
	SX(hwnd,offset) = 0;			/* no next search position */
	SX(hwnd,mlength) = 0;			/* no length */
	disableitem(hwnd,IDC_SPELL_CHANGE);
	disableitem(hwnd,IDC_SPELL_ADD);
	disableitem(hwnd,IDC_SPELL_IGNOREALL);
	disableitem(hwnd,IDC_SPELL_SUGGEST);
	clearalts(hwnd);
}
/******************************************************************************/
static void setallbuttons(HWND hwnd)	/* sets all buttons */

{
	SFLIST * sfp = getdata(hwnd);

	if (sfp->target)	{
		if (sfp->sps.doubleflag)	{			/* if double word */
			SetWindowText(GetDlgItem(hwnd,IDC_SPELL_BADPROMPT),TEXT("Duplicate"));	/* set new title */
			disableitem(hwnd,IDC_SPELL_ADD);
			disableitem(hwnd,IDC_SPELL_IGNOREALL);
			disableitem(hwnd,IDC_SPELL_SUGGEST);
		}
		else {
			enableitem(hwnd,IDC_SPELL_ADD);
			enableitem(hwnd,IDC_SPELL_IGNOREALL);
			if (sfp->hasalts)
				disableitem(hwnd,IDC_SPELL_SUGGEST);
			else
				enableitem(hwnd,IDC_SPELL_SUGGEST);
		}
		setchangebuttons(hwnd);
	}
}
/******************************************************************************/
static void setchangebuttons(HWND hwnd)	/* sets change buttons */

{
	TCHAR twordb[STSTRING], twordg[STSTRING];
	
	GetDlgItemText(hwnd,IDC_SPELL_BADWORD,twordb, STSTRING);
	GetWindowText(GetDlgItem(hwnd,IDC_SPELL_GOODLIST),twordg,STSTRING);
	if (!*twordg)			/* if nothing to change to */
		SetWindowText(GetDlgItem(hwnd,IDC_SPELL_CHANGE),TEXT("&Delete"));	/* set title */
	else
		SetWindowText(GetDlgItem(hwnd,IDC_SPELL_CHANGE),TEXT("&Change"));	/* set title */
	if (nstrcmp(twordb,twordg))	/* if good & bad strings differ */
		enableitem(hwnd,IDC_SPELL_CHANGE);
	else
		disableitem(hwnd,IDC_SPELL_CHANGE);
}
/******************************************************************************/
char * dochange(HWND hwnd, INDEX * FF, RECORD * recptr, char * sptr, short *lptr, char * reptext)	 /* replaces */

{
	char dupcopy[MAXREC];
	RECN pcount;

	str_xcpy(dupcopy, recptr->rtext);		/* save copy */
	if (sptr = sp_reptext(FF,recptr, sptr, *lptr, reptext)) {
		str_adjustcodes(recptr->rtext,CC_TRIM|(g_prefs.gen.remspaces ? CC_ONESPACE : 0));		/* adjust codes around spaces */
		rec_strip(FF,recptr->rtext);		/* remove empty fields */
		sort_addtolist(SX(hwnd,nptr),recptr->num);	/* add to sort list */
		pcount = rec_propagate(FF,recptr,dupcopy, SX(hwnd,nptr));	/* propagate */
		view_clearselect(FF->vwind);
		view_resetrec(FF,recptr->num);	/* redisplay */
		*lptr = 0;			/* no matched length */
	}
	return (sptr);
}
/******************************************************************************/
static void clearalts(HWND hwnd)	/*  clears list */

{
	setDItemText(hwnd,IDC_SPELL_BADWORD,g_nullstr);
	ComboBox_ResetContent(GetDlgItem(hwnd,IDC_SPELL_GOODLIST));
	SX(hwnd,hasalts) = FALSE;				/* no alts generated */
	if (SX(hwnd,sps).doubleflag)	{		/* if had double-word err */
		SetWindowText(GetDlgItem(hwnd,IDC_SPELL_BADPROMPT),TEXT("Unknown:"));	/* set new title */
		SX(hwnd,sps).doubleflag = FALSE;
	}
}
/******************************************************************************/
BOOL spellcontinue(INDEX * FF, HWND hwnd, RECORD * recptr, char * sptr, SFLIST * sfp)
			/* finds  next bad word  in record */

{
	clearalts(hwnd);	/* clears list */
	if (sfp->resume)	/* if restarting */
		sfp->resume = FALSE;
	if (recptr && (sptr = sp_checktext(FF,recptr, sptr+sfp->mlength, &sfp->sps, &sfp->mlength)))	{	/* if any more in this record */
		view_selectrec(FF, recptr->num, VD_SELPOS,sptr-recptr->rtext,sfp->mlength);
		sfp->offset = sptr-recptr->rtext;	/* new offset  */
		showbadword(hwnd,sptr,sfp->mlength);
		return TRUE;
	}
	if (scheck(FF,hwnd,&sfp->sps))	{			/* if all checks ok */
		char *sptr;
		RECORD * recptr;
		HCURSOR ocurs = SetCursor(g_waitcurs);

		SetCapture(hwnd);
		do {
			recptr = sp_findfirst(FF,&sfp->sps,sfp->restart,&sptr,&sfp->mlength);		/* while target in invis part of rec */
		} while (recptr && !(sptr = vistarget(FF,recptr,sptr)));
		sfp->restart = FALSE;		/* can proceed with search */
		ReleaseCapture();
		SetCursor(ocurs);
		if (recptr)	{
			view_selectrec(FF,recptr->num,VD_SELPOS,sptr-recptr->rtext,sfp->mlength);
			sfp->offset = sptr-recptr->rtext;
			sfp->target = recptr->num;
			showbadword(hwnd,sptr,sfp->mlength);
			return TRUE;
		}
		else
			sendinfo(INFO_NOMOREREC);	/* done */
	}
	return (FALSE);
}
/******************************************************************************/
static void showbadword(HWND hwnd, char * sptr, short mlength)	/* sets up list */

{
	char tword[STSTRING];
	SPELL * sp;
	short wcount;

	strncpy(tword,sptr,mlength);
	*(tword+mlength) = '\0';
	setDItemText(hwnd,IDC_SPELL_BADWORD,tword);
	SetWindowText(GetDlgItem(hwnd,IDC_SPELL_START),TEXT("Ignore"));	/* set new title */
	sp = &SX(hwnd,sps);
	if (!sp->doubleflag)	{		/* if not double-word err (i.e., error from IPR) */
		wcount = showalts(hwnd,tword);
		if (!wcount)	{	/* if no alts */
			SetWindowText(GetDlgItem(hwnd,IDC_SPELL_GOODLIST),toNative(tword));/* show bad word stripped of punct */
			ComboBox_SetEditSel(GetDlgItem(hwnd,IDC_SPELL_GOODLIST),0,-1);
		}
	}
	SetFocus(GetDlgItem(hwnd,IDC_SPELL_GOODLIST));
	setallbuttons(hwnd);		/* set buttons */
}
/******************************************************************************/
static short showalts(HWND hwnd, char * tword)	/* sets up list of alts for bad word */

{

	SPELL * sps = &SX(hwnd,sps);
	HWND cbptr = GetDlgItem(hwnd,IDC_SPELL_GOODLIST);
	int altcount, listindex;
	
	altcount = hspell_suggest(tword);
	
	for (listindex = 0; listindex < altcount; listindex++) {
		char * cword = hspell_convertword(sps->speller->suggestions.list[listindex],TONATIVE);	// convert to native charset
		int index = ComboBox_AddString(cbptr,toNative(cword));
		if (!index)				/* if first suggestion */
			ComboBox_SetCurSel(cbptr,0);	/* set it in display buffer */
}
	SX(hwnd,hasalts) = TRUE;			/* alts attempted */
	return (altcount);
}
/******************************************************************************/
static short cleanupspell(HWND hwnd)	/* cleans up after finishing replacements */

{
	INDEX * FF;
	HCURSOR ocurs;

	if (SX(hwnd,nptr))	{		/* if have some replacements */
		ocurs = SetCursor(g_waitcurs);
		FF = SX(hwnd,lastindex);
		view_clearselect(FF->vwind);
		sort_resortlist(FF,SX(hwnd,nptr));		/* make new nodes */
		freemem(SX(hwnd,nptr));
		SX(hwnd,nptr) = NULL;
		view_redisplay(FF,0,VD_CUR);
		SetCursor(ocurs);
		return (TRUE);		/* did something */
	}
	return (FALSE);			/* did nothing */
}
/******************************************************************************/
static short scheck(INDEX * FF, HWND hwnd, SPELL * sp)		/* checks search parameters */

{
	char str1[STSTRING], str2[STSTRING];
	int err;
	
	getDItemText(hwnd,IDC_FIND_RANGESTART,str1,STSTRING);
	getDItemText(hwnd,IDC_FIND_RANGEEND,str2,STSTRING);
	if (err = com_getrecrange(FF,SX(hwnd,scope), str1,str2,&sp->firstr, &sp->lastr))	{	/* bad range */
		selectitext(hwnd,err < 0 ? IDC_FIND_RANGESTART : IDC_FIND_RANGEEND);
		return (FALSE);
	}
	return (TRUE);
}
/******************************************************************************/
static char * vistarget(INDEX * FF, RECORD * recptr, char *sptr)	/* returns ptr if target vis */

{
	short hlevel,sprlevel,hidelevel, clevel;
	char *uptr;
	
	uptr = rec_uniquelevel(FF,recptr,&hlevel,&sprlevel,&hidelevel,&clevel);		/* find unique level */
	if (sptr < uptr) 	/* if target before unique level */			
		return (NULL);	/* not visible */
	return (sptr);
}
/*******************************************************************************/
static int buildlanguagemenu(HWND cbmain, TCHAR * name)	/* builds language dic menu */

{
	unsigned int index;

	ComboBox_ResetContent(cbmain);
	for (index = 0; index < hs_dictionaryCount; index++)	{
		if (!(hs_dictionaries[index].flags&DIC_ISAUX))	{	// if isn't auxiliary
			int iindex = ComboBox_AddString(cbmain,toNative(hs_dictionaries[index].displayname));
			SendMessage(cbmain, CB_SETITEMDATA, iindex, (LPARAM)hs_dictionaries[index].root); 
		}
	}
	if ((index = ComboBox_FindStringExact(cbmain,-1,name)) == CB_ERR)	// if can't find dic in list
		index = 0;		// set first language
	ComboBox_SetCurSel(cbmain,index);
	return (index);
}
/*******************************************************************************/
static int buildusermenu(HWND cbuser, TCHAR * name)	/* builds user dic menu */

{
	TCHAR pdpath[MAX_PATH];
	int index, itemcount;

	ComboBox_ResetContent(cbuser);
	makedicpath(pdpath,sp_uext);
	ComboBox_Dir(cbuser,DDL_READWRITE,pdpath);	/* make user dic list */
	itemcount = ComboBox_GetCount(cbuser);
	for (index = 0; index < itemcount; index++)	{	// strip extension
		TCHAR itemname[STSTRING];

		ComboBox_GetLBText(cbuser,index,itemname);		// get whatever is now current main dic
		ComboBox_DeleteString(cbuser,index);
		PathRemoveExtension(itemname);
		ComboBox_InsertString(cbuser,index,itemname);
	}
	ComboBox_InsertString(cbuser,0,TEXT("<None Selected>"));
	if ((index = ComboBox_FindStringExact(cbuser,0,name)) == CB_ERR)	/* if can't find dic in list */
		index = 0;		/* set none selected */
	ComboBox_SetCurSel(cbuser,index);
	return (index);
}
/*******************************************************************************/
static HUNSPELL * openmaindic(SFLIST * sfp, char *locale)	// opens main dic

{
	int curindex = ComboBox_FindString(sfp->cbmain,0,toNative(displayNameForLocale(locale)));

	if (curindex == CB_ERR)	// if don't have dic for this locale
		curindex = 0;		// get first
	ComboBox_SetCurSel(sfp->cbmain,curindex);	// select specified in case changed
	locale = (char *)SendMessage(sfp->cbmain, CB_GETITEMDATA, curindex,0); 
	sfp->sps.speller = hspell_open(locale);
	if (sfp->sps.speller)	{
		TCHAR * dname;
		int index;
		
		for (index = 0; dname = reg_enumeratekey(HKEY_CURRENT_USER,K_SPELLPATHS,index); index++)		// for all aux dics
			hspell_openauxdic(hspell_dictionarysetforlocale(fromNative(dname)));
		openuserdic(sfp,getdefaultdic(USERDIC));
	}
	setdefaultdic(LANGUAGEDIC,toNative(locale));
	return sfp->sps.speller;
}
/*******************************************************************************/
static void openuserdic(SFLIST *sfp, TCHAR * dicname)	/* opens user dic selected in combo box */

{
	int curindex = ComboBox_FindString(sfp->cbuser,0,dicname);
	TCHAR pdpath[MAX_PATH];

	if (curindex == CB_ERR)	// if don't have dic
		curindex = 0;		// set no dictionary
	ComboBox_SetCurSel(sfp->cbuser,curindex);	// select it
	hspell_closepd();	// close any open dic
	if (curindex)	{			/* if have user dictionary selected */
		makedicpath(pdpath,dicname);		/* make a path */
		PathAddExtension(pdpath,sp_udcext);
		if (!hspell_openpd(fromNative(pdpath)))
			ComboBox_SetCurSel(sfp->cbuser,0);	/* show none selected */
	}
	setdefaultdic(USERDIC,dicname);
}
/*******************************************************************************/
INT_PTR newdic(HWND hwnd)	/* creates new dictionary */

{
	return (DialogBoxParam(g_hinst,MAKEINTRESOURCE(IDD_SPELL_NEWDIC),hwnd,dnameproc,(LPARAM)hwnd));
}
/******************************************************************************/
static INT_PTR CALLBACK dnameproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)

{
	HWND cbwnd, pwnd;
	TCHAR namestr[STSTRING], fpath[MAX_PATH];

	pwnd = getdata(hwnd);
	switch (msg)	{

		case WM_INITDIALOG:
			setdata(hwnd,(void *)lParam);	/* set data */
			centerwindow(hwnd,0);
			SetFocus(GetDlgItem(hwnd,IDC_SPELL_NEWDIC));	/* set focus to text */
			return FALSE;
		case WM_COMMAND:
			switch (LOWORD(wParam))	{
				case IDOK:
					cbwnd = GetDlgItem(g_spellw,IDC_SPELL_DICTIONARY);
					if (GetDlgItemText(hwnd,IDC_SPELL_NEWDIC,namestr,STSTRING))	{
						if (ComboBox_FindStringExact(cbwnd,0,namestr) == CB_ERR)	{	/* if our dic doesn't exist */
							makedicpath(fpath,namestr);
							PathAddExtension(fpath,sp_udcext);
							hspell_createpd(fromNative(fpath));
							if (!hspell_openpd(fromNative(fpath)))	// just to check that all is well ?
								break;						
							buildusermenu(cbwnd,namestr);		/* rebuild menu, select right dic */
							if (pwnd != g_spellw)		/* if called from options box */
								buildusermenu(GetDlgItem(pwnd,IDC_SPELL_OPTIONSPD),namestr);	/* rebuild main menu, select right dic */
							openuserdic(getdata(g_spellw),namestr);	/* opens current user dic if poss */
						}
						else	{
							senderr(ERR_SPELLDICEXISTERR,WARN,namestr);
							selectitext(hwnd,IDC_SPELL_NEWDIC);
							break;
						}
					}
					else	{
						SetFocus(GetDlgItem(hwnd,IDC_SPELL_NEWDIC));	/* set focus to text */
						break;
					}
				case IDCANCEL:
					EndDialog(hwnd,LOWORD(wParam) == IDOK ? ComboBox_FindStringExact(cbwnd,0,namestr) : -1);
					return TRUE;
			}
			break;
		case WM_HELP:
			return (dodialoghelp(hwnd,TEXT("_Spelling\\Spellingoptions.htm"),(HELPINFO *)lParam,wh_spellid));
	}
	return FALSE;
}
/*******************************************************************************/
INT_PTR editdic(HWND hwnd,TCHAR * path)	/* edits dictionary */

{
	struct estruct es;

	es.path = path;
	return (DialogBoxParam(g_hinst,MAKEINTRESOURCE(IDD_SPELL_EDITDIC),hwnd,deditproc,(LPARAM)&es));
}
/******************************************************************************/
static INT_PTR CALLBACK deditproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)

{
	HWND lwnd = GetDlgItem(hwnd,IDC_SPELL_EDITDICLIST);	/* list window */
	struct estruct * esp = getdata(hwnd);
	SPELL * sp = &SX(g_spellw,sps);
	int imax;

	switch (msg)	{
		case WM_INITDIALOG:
			if (esp = setdata(hwnd,(void *)lParam))	{	/* set data */
				WORDLIST *listptr = hspell_wlfromfile(fromNative(esp->path),NULL);
				UChar * ustring = hspell_wlToUtext(listptr,&imax);

				Edit_ReplaceSel(lwnd,ustring);
				hspell_wlfree(&listptr);
				free(ustring);
				centerwindow(hwnd,0);
				SetDlgItemInt(hwnd,IDC_SPELL_EDITDICCOUNT, imax,TRUE);
				disableitem(hwnd,IDC_SPELL_EDITDELETE);
				disableitem(hwnd,IDC_SPELL_EDITDESELALL);
				SetFocus(GetDlgItem(hwnd,IDC_SPELL_EDITDICLIST));	/* set focus to list */
			}
			return FALSE;
		case WM_COMMAND:
			switch (LOWORD(wParam))	{
				case IDOK:
				{
					WORDLIST * wlp = NULL;
					int length;
					TCHAR * ustring;
		
					if (!nstrcmp(PathFindFileName(esp->path),getdefaultdic(USERDIC)))	// if active dictionary
						hspell_closepd();	// close it
					length = GetWindowTextLength(lwnd)*sizeof(short)+2;
					ustring = malloc(length);
					GetWindowText(lwnd,ustring,length);	// get text
					wlp = hspell_wlFromUtext(ustring);
					free(ustring);
					hspell_wlsavetofile(wlp,fromNative(esp->path),"w");	//replace existing dic file
					hspell_wlfree(&wlp);		// free list
					if (!nstrcmp(PathFindFileName(esp->path),getdefaultdic(USERDIC)))	// if active dictionary
						openuserdic(getdata(g_spellw),getdefaultdic(USERDIC));	// reopen it
				}
				case IDCANCEL:
					EndDialog(hwnd,TRUE);
					return TRUE;
				case IDC_SPELL_EDITDICLIST:
					if (HIWORD(wParam) == EN_UPDATE)	{
						int itemcount = Edit_GetLineCount(lwnd);
						SetDlgItemInt(hwnd,IDC_SPELL_EDITDICCOUNT, itemcount,TRUE);
					}
					return FALSE;
				case IDM_EDIT_SELECTALL:
					SendMessage(lwnd, EM_SETSEL, 0, -1); 
					return TRUE;
			}
			break;
		case WM_HELP:
			return (dodialoghelp(hwnd,TEXT("_Spelling\\Spellingoptions.htm"),(HELPINFO *)lParam,wh_spellid));
	}
	return FALSE;
}
/*******************************************************************************/
int options(HWND hwnd)	/* handles spelling options */

{
	struct langprefs lp;

	lp = SX(hwnd,sps).lp;		/* make temp copy of langage prefs */
	SX(hwnd,clearignore) = 0;	/* clear ignore list flag */

	if (DialogBoxParam(g_hinst,MAKEINTRESOURCE(IDD_SPELL_OPTIONS),hwnd,optionproc,(LPARAM)&lp))	{
		SX(hwnd,sps).lp = lp;
		return (TRUE);
	}
	return (FALSE);
}
/******************************************************************************/
static INT_PTR CALLBACK optionproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)

{
	HWND listwnd = GetDlgItem(hwnd,IDC_SPELL_OPTIONLANGUAGE);
	SPELL * sp;
	struct langprefs *lp;
	TCHAR tstring[STSTRING], dicpath[MAX_PATH];
	int index;

	sp = &SX(g_spellw,sps);
	lp = getdata(hwnd);

	switch (msg)	{
		case WM_INITDIALOG:
			if (lp = setdata(hwnd,(void *)lParam))	{	/* set data */
				RECT lr;
				LVCOLUMN lvc; 

				ListView_SetExtendedListViewStyle(listwnd,LVS_EX_CHECKBOXES);
				GetClientRect(listwnd,&lr);
				lvc.mask = LVCF_TEXT | LVCF_SUBITEM | LVCF_WIDTH;
				lvc.iSubItem = 0;
				lvc.pszText = TEXT("Use Additional Dictionaries");
				lvc.cx = lr.right;
				ListView_InsertColumn(listwnd, 0, &lvc);
				for (index = 0; index < sp->speller->dicset->setcount; index++)	{
					DWORD size = MAX_PATH;
					char displayname[MAX_PATH];
					LVITEM lv;
					BOOL isactive;

					memset(&lv,0,sizeof(LVITEM));
					lv.mask = LVIF_TEXT|LVIF_PARAM;
					lv.iSubItem = 0;
					lv.pszText = toNative(sp->speller->dicset->sets[index]->displayname);
					lv.lParam = (LPARAM)sp->speller->dicset->sets[index]->root;
					ListView_InsertItem(listwnd,&lv);
					isactive = reg_getkeyvalue(K_SPELLPATHS,toNative((char *)lv.lParam),displayname,&size);	// look for displayname key
					ListView_SetCheckState(listwnd,index,isactive);	// set state by key found
				}
				SX(g_spellw,reopen) = FALSE;	// clar dic reset
				centerwindow(hwnd,0);
				checkitem(hwnd,IDC_SPELL_OPTIONSSUGG,lp->suggestflag);
				checkitem(hwnd,IDC_SPELL_OPTIONSCAPS,lp->ignallcaps);
				checkitem(hwnd,IDC_SPELL_OPTIONSSTR,lp->ignalnums);
				if (!buildusermenu(GetDlgItem(hwnd,IDC_SPELL_OPTIONSPD),getdefaultdic(USERDIC)))	{	/* build & set current user dic */
					disableitem(hwnd,IDC_SPELL_OPTIONSEDITPD);
					disableitem(hwnd,IDC_SPELL_OPTIONSPDDELETE);
				}
			}
			return FALSE;
		case WM_COMMAND:
			switch (LOWORD(wParam))	{
				case IDOK:
					if (SX(g_spellw,clearignore))		/* if want to clear ignore list */
						hspell_removeignoredwords();
					if (SX(g_spellw,reopen))	{	// some change in aux dics
						reg_deleteuserkey(K_SPELLPATHS);	// clear all existing aux dics
						reg_makeuserkey(K_SPELLPATHS);		// make new key
						for (index = 0; index < sp->speller->dicset->setcount; index++)	{
							BOOL state = ListView_GetCheckState(listwnd,index);
							if (state)	{	// want to use
								LVITEM lv;

								memset(&lv,0,sizeof(LVITEM));
								lv.mask = LVIF_PARAM;
								lv.iItem = index;
								lv.iSubItem = 0;
								ListView_GetItem(listwnd,&lv);
								reg_setkeyvalue(K_SPELLPATHS,toNative((char *)lv.lParam),REG_SZ,TEXT(""),0); // save root name
							}
						}
						openmaindic(getdata(g_spellw),fromNative(getdefaultdic(LANGUAGEDIC)));
					}
				case IDCANCEL:
					EndDialog(hwnd,LOWORD(wParam) == IDOK ? TRUE : FALSE);
					break;
				case IDC_SPELL_OPTIONSSUGG:		/* always suggest */
					lp->suggestflag = (short)iscontrolchecked(lParam);
					break;
				case IDC_SPELL_OPTIONSCAPS:		/* ignore caps */
					lp->ignallcaps = (short)iscontrolchecked(lParam);
					break;
				case IDC_SPELL_OPTIONSSTR:		/* ignore alnums */
					lp->ignalnums = (short)iscontrolchecked(lParam);
					break;
				case IDC_SPELL_OPTIONSPD:		/* choose pd */
					if (HIWORD(wParam) == CBN_SELCHANGE)	{
						if (ComboBox_GetCurSel((HWND)lParam))	{
							enableitem(hwnd,IDC_SPELL_OPTIONSEDITPD);
							enableitem(hwnd,IDC_SPELL_OPTIONSPDDELETE);
						}
						else	{
							disableitem(hwnd,IDC_SPELL_OPTIONSEDITPD);
							disableitem(hwnd,IDC_SPELL_OPTIONSPDDELETE);
						}
					}
					break;
				case IDC_SPELL_OPTIONSEDITPD:	/* edit pd */
					hspell_savepd();	// make sure personal dic saved image is up to date
					GetWindowText(GetDlgItem(hwnd,IDC_SPELL_OPTIONSPD),tstring,STSTRING);
					makedicpath(dicpath,tstring);
					PathAddExtension(dicpath,sp_udcext);
					editdic(hwnd,dicpath);
					/* !! should disable cancel button after this */
					break;
				case IDC_SPELL_OPTIONSPDNEW:	/* new pd */
					if (newdic(hwnd) >= 0)	/* if made new dic (it's selected) */
						enableitem(hwnd,IDC_SPELL_OPTIONSEDITPD);	/* enable edit button */
					break;
				case IDC_SPELL_OPTIONSPDDELETE:	/* delete pd */
					if (sendwarning(WARN_DELETEDIC))	{
						GetWindowText(GetDlgItem(hwnd,IDC_SPELL_OPTIONSPD),tstring,STSTRING);
						if (!nstrcmp(getdefaultdic(USERDIC),tstring))	{	/* if currently active one */
							hspell_closepd();	// close any open dic
							buildusermenu(GetDlgItem(g_spellw,IDC_SPELL_DICTIONARY),TEXT(""));		/* rebuild main menu, select no dic */
						}
						makedicpath(dicpath,tstring);
						PathAddExtension(dicpath,sp_udcext);
						DeleteFile(dicpath);		// delete dictionary
						if (!buildusermenu(GetDlgItem(hwnd,IDC_SPELL_OPTIONSPD),TEXT("")))	{	/* rebuild select right dic */
							disableitem(hwnd,IDC_SPELL_OPTIONSEDITPD);
							disableitem(hwnd,IDC_SPELL_OPTIONSPDDELETE);
						}
					}
					break;
				case IDC_SPELL_OPTIONSCLEAR:	/* clear ignore list */
					if (SX(g_spellw,clearignore) ^= 1)		/* switch state */
						SetWindowText((HWND)lParam,TEXT("&Restore Ignore List"));
					else
						SetWindowText((HWND)lParam,TEXT("&Clear Ignore List"));
					break;
			}
			return FALSE;
		case WM_NOTIFY:
			switch (((NMHDR *)lParam)->code)	{
				case LVN_ITEMCHANGED:
					{
						NMLISTVIEW * lvnp = (NMLISTVIEW *)lParam;
						if (lvnp->uChanged&LVIF_STATE)	// if state changed
							SX(g_spellw,reopen) = TRUE;	// force dictionary reset
						break;
					}
			}
	}
	return FALSE;
}
/******************************************************************************/
static TCHAR * getdefaultdic(TCHAR * dickey)	// gets default dic name from registry

{
	static TCHAR name[STSTRING] = TEXT("");
	int length = STSTRING;

	reg_getkeyvalue(K_GENERAL,dickey,name,&length);
	return name;
}
/******************************************************************************/
static void setdefaultdic(TCHAR * dickey, TCHAR * name)	// sets default dic name in registry

{
	reg_setkeyvalue(K_GENERAL,dickey,REG_SZ,name,nstrlen(name));	// save
}

