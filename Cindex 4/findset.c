#include "stdafx.h"
#include "commands.h"
#include "index.h"
#include "records.h"
#include "group.h"
#include "sort.h"
#include "group.h"
#include "errors.h"
#include "findset.h"
#include "viewset.h"
#include "modify.h"
#include "strings.h"
#include "util.h"
#include "search.h"


static const DWORD wh_findid[] = {
	IDC_FIND_ALL, HIDC_FIND_ALL,
	IDC_FIND_SELECTED, HIDC_FIND_SELECTED,
	IDC_FIND_RANGE, HIDC_FIND_RANGE,
	IDC_FIND_RANGESTART, HIDC_FIND_RANGE,
	IDC_FIND_ALLDATES, HIDC_FIND_ALLDATES,
	IDC_FIND_RANGEEND, HIDC_FIND_RANGE,
	IDC_FIND_DATERANGE, HIDC_FIND_DATERANGE,
	IDC_FIND_DATESTART, HIDC_FIND_DATERANGE,
	IDC_FIND_DATEEND, HIDC_FIND_DATERANGE,
	IDC_FIND_USERID,HIDC_FIND_USERID,
	IDC_FIND_ONLYAMONG,HIDC_FIND_ONLYAMONG,
	IDC_FIND_NOTAMONG,HIDC_FIND_NOTAMONG,
	IDC_FIND_NEWREC, HIDC_FIND_NEWREC,
	IDC_FIND_MODREC, HIDC_FIND_MODREC,
	IDC_FIND_DELREC, HIDC_FIND_DELREC,
	IDC_FIND_MARKREC, HIDC_FIND_MARKREC,
	IDC_FIND_GENREC, HIDC_FIND_GENREC,
	IDC_FIND_TAGREC, HIDC_FIND_TAGREC,
	IDC_FIND_TAGRECLEVEL,HIDC_FIND_TAGRECLEVEL,
	IDC_FIND_FINDSTART, HIDC_FIND_FINDSTART,
	IDC_FIND_FINDSTOP, HIDC_FIND_FINDSTOP,
	IDC_FIND_FINDALL, HIDC_FIND_FINDALL,
	IDC_FIND_UP, HIDC_FIND_UP,
	IDC_FIND_DOWN, HIDC_FIND_DOWN,
	IDC_FIND_NOT1, HIDC_FIND_NOT1,
	IDC_FIND_TEXT1, HIDC_FIND_TEXT1,
	IDC_FIND_STYLE1, HIDC_FIND_STYLE1,
	IDC_FIND_STYLESHOW1, HIDC_FIND_STYLESHOW1,
	IDC_FIND_ANDOR1, HIDC_FIND_ANDOR1,
	IDC_FIND_FIELDS1, HIDC_FIND_FIELDS1,
	IDC_FIND_EVALREFS1, HIDC_FIND_EVALREFS1,
	IDC_FIND_WORD1, HIDC_FIND_WORD1,
	IDC_FIND_CASE1, HIDC_FIND_CASE1,
	IDC_FIND_PATTERN1, HIDC_FIND_PATTERN1,
	IDC_FIND_NOT2, HIDC_FIND_NOT1,
	IDC_FIND_TEXT2, HIDC_FIND_TEXT1,
	IDC_FIND_STYLE2, HIDC_FIND_STYLE1,
	IDC_FIND_STYLESHOW2, HIDC_FIND_STYLESHOW1,
	IDC_FIND_ANDOR2, HIDC_FIND_ANDOR1,
	IDC_FIND_FIELDS2, HIDC_FIND_FIELDS1,
	IDC_FIND_EVALREFS2, HIDC_FIND_EVALREFS1,
	IDC_FIND_WORD2, HIDC_FIND_WORD1,
	IDC_FIND_CASE2, HIDC_FIND_CASE1,
	IDC_FIND_PATTERN2, HIDC_FIND_PATTERN1,
	IDC_FIND_NOT3, HIDC_FIND_NOT1,
	IDC_FIND_TEXT3, HIDC_FIND_TEXT1,
	IDC_FIND_STYLE3, HIDC_FIND_STYLE1,
	IDC_FIND_STYLESHOW3, HIDC_FIND_STYLESHOW1,
	IDC_FIND_ANDOR3, HIDC_FIND_ANDOR1,
	IDC_FIND_FIELDS3, HIDC_FIND_FIELDS1,
	IDC_FIND_EVALREFS3, HIDC_FIND_EVALREFS1,
	IDC_FIND_WORD3, HIDC_FIND_WORD1,
	IDC_FIND_CASE3, HIDC_FIND_CASE1,
	IDC_FIND_PATTERN3, HIDC_FIND_PATTERN1,
	IDC_FIND_NOT4, HIDC_FIND_NOT1,
	IDC_FIND_TEXT4, HIDC_FIND_TEXT1,
	IDC_FIND_STYLE4, HIDC_FIND_STYLE1,
	IDC_FIND_STYLESHOW4, HIDC_FIND_STYLESHOW1,
	IDC_FIND_ANDOR4, HIDC_FIND_ANDOR1,
	IDC_FIND_FIELDS4, HIDC_FIND_FIELDS1,
	IDC_FIND_EVALREFS4, HIDC_FIND_EVALREFS1,
	IDC_FIND_WORD4, HIDC_FIND_WORD1,
	IDC_FIND_CASE4, HIDC_FIND_CASE1,
	IDC_FIND_PATTERN4, HIDC_FIND_PATTERN1,
	0,0
};

static const DWORD wh_attribid[] = {
	HIDC_FINDSTYLE_BOLDIG, HIDC_FINDSTYLE_BOLDIG,
	IDC_FINDSTYLE_ITALIG, HIDC_FINDSTYLE_ITALIG,
	IDC_FINDSTYLE_ULINEIG, HIDC_FINDSTYLE_ULINEIG,
	IDC_FINDSTYLE_SMALLIG, HIDC_FINDSTYLE_SMALLIG,
	IDC_FINDSTYLE_SUPIG, HIDC_FINDSTYLE_SUPIG,
	IDC_FINDSTYLE_SUPIG, HIDC_FINDSTYLE_SUPIG,

	IDC_FINDSTYLE_BOLDAP,HIDC_FINDSTYLE_BOLDAP,
	IDC_FINDSTYLE_ITALAP,HIDC_FINDSTYLE_ITALAP,
	IDC_FINDSTYLE_ULINEAP,HIDC_FINDSTYLE_ULINEAP,
	IDC_FINDSTYLE_SMALLAP,HIDC_FINDSTYLE_SMALLAP,
	IDC_FINDSTYLE_SUPAP,HIDC_FINDSTYLE_SUPAP,
	IDC_FINDSTYLE_SUBAP,HIDC_FINDSTYLE_SUBAP,

	IDC_FINDSTYLE_BOLDREM,HIDC_FINDSTYLE_BOLDREM,
	IDC_FINDSTYLE_ITALREM,HIDC_FINDSTYLE_ITALREM,
	IDC_FINDSTYLE_ULINEREM,HIDC_FINDSTYLE_ULINEREM,
	IDC_FINDSTYLE_SMALLREM,HIDC_FINDSTYLE_SMALLREM,
	IDC_FINDSTYLE_SUPREM,HIDC_FINDSTYLE_SUPREM,
	IDC_FINDSTYLE_SUBREM,HIDC_FINDSTYLE_SUBREM,

	IDC_FINDSTYLE_FONTIG,HIDC_FINDSTYLE_FONTIG,
	IDC_FINDSTYLE_FONTSET,HIDC_FINDSTYLE_FONTSET,
	IDC_FINDSTYLE_FONTREM,HIDC_FINDSTYLE_FONTREM,

	IDC_FINDSTYLE_FONT, HIDC_FINDSTYLE_FONT,
	0,0
};

static int sidarray[] = {
	IDC_FINDSTYLE_BOLDIG,
	IDC_FINDSTYLE_ITALIG,
	IDC_FINDSTYLE_ULINEIG,
	IDC_FINDSTYLE_SMALLIG,
	IDC_FINDSTYLE_SUPIG,
	IDC_FINDSTYLE_SUBIG
};
#define STYLEIDS (sizeof(sidarray)/sizeof(int))

typedef struct {
	char * name;
	char * abbrev;
	char * display;
} UPATTERN;

typedef struct {
	char * name;
	UPATTERN *pat;
} UPGROUP;

static UPATTERN pa[] = {
	{"", "[:l:]+","Any word"},
	{"", "(\\b[:l:]+) \\1","Repeated word"},
	{"", "[:nd:]+","Any number"},
	{"", "[:nd:]+-[:nd:]+","Any page range"},
	{"", "([:p:]+)\\1","Repeated punctuation"},
	{ "", "(?<=')[^']+(?=')|(?<=\")[^\"]+(?=\")|(?<=‘)[^‘]+(?=’)|(?<=“)[^“]+(?=”)","Text in Quotes" },		// uses pos lookbehind and lookahead
	{ "", "(?<=\\()[^(]+(?=\\))","Text in Parentheses" },		// uses pos lookbehind and lookahead
//	{"", "(?:[:l:]+[’' ])?[:l:][:l:]+(?:[- ][:lu:][:l:]*)?, (?:[:lu:][:l:]+|[:lu:]\\.)(?:[- ](?:[:lu:][:l:]+|[:lu:]\\.))*","Surname, Forenames(s)"},
	{"", "((?:[:l:]+[’' ])?[:lu:][:l:]+(?:[- ][:l:][:l:]*)?), ((?:[:lu:][:l:]+|(?:[:lu:]\\.)*)(?:[- ](?:[:lu:][:l:]+|(?:[:lu:]\\.)))*)","Surname, Forenames(s)"},
//	{"", "(?:[:lu:][:l:]+|[:lu:]\\.)(?:[- ](?:[:lu:][:l:]+|[:lu:]\\.))* (?:[:l:]+[’' ])?[:lu:][:l:]+(?:[- ][:l:][:l:]*)?","Forename(s) Surname"},
	{"", "((?:[:lu:][:l:]+|(?:[:lu:]\\.)*)(?:[- ](?:[:lu:][:l:]+|(?:[:lu:]\\.)))*) ((?:[:l:]+[’' ])?[:lu:][:l:]+(?:[- ][:l:][:l:]*)?)","Forename(s) Surname"},
	{"", "^$","Empty field"},
	{"", "",NULL},
};
static UPATTERN pc[] = {
	{"[:ascii:]", "[:ascii:]","ASCII character"},
	{"[:letter:]", "[:l:]","Any letter"},
	{"[]", "[a-zA-Z]","Unaccented letter"},
	{"[]", "[[:latin:]-[a-zA-Z]]","Accented letter"},
	{"[:lowercase letter:]", "[:ll:]","Lowercase letter"},
	{"[:uppercase letter:]", "[:lu:]","Uppercase letter"},
	{"[:mark:]", "[:m:]","Diacritical mark"},
	{"[:separator:]", "[:z:]","Any space"},
	{"[:symbol:]", "[:s:]","Any symbol"},
	{"[:math symbol:]", "[:sm:]","Math Symbol"},
	{"[:currency symbol:]", "[:sc:]","Currency Symbol"},
	{"[:other symbol:]", "[:so:]","Other Symbol"},
	{"[:number:]", "[:n:]","Number"},
	{"[:decimal digit number:]", "[:nd:]","Decimal number"},
	{"[:punctuation:]", "[:p:]","Punctuation"},
	{"[:dash punctuation:]", "[:pd:]","Dash punctuation"},
	{"[:open punctuation:]", "[:ps:]","Opening punctuation"},
	{"[:close punctuation:]", "[:pe:]","Closing punctuation"},
	{"[:initial punctuation:]", "[:pi:]","Opening quote"},
	{"[:final punctuation:]", "[:pf:]","Closing quote"},
	{"[:private use:]", "[:co:]","Private use"},
	{"", "",NULL},
};
static UPATTERN ps[] = {
	{"[:arabic:]", "[:arabic:]","Arabic"},
	{"[:cyrillic:]", "[:cyrillic:]","Cyrillic"},
	{"[:devanagari:]", "[:devanagari:]","Devanagari"},
	{"[:greek:]", "[:greek:]","Greek"},
	{"[:han:]", "[:han:]","Han"},
	{"[:hangul:]", "[:hangul:]","Hangul"},
	{"[:hebrew:]", "[:hebrew:]","Hebrew"},
	{"[:hiragana:]", "[:hiragana:]","Hiragana"},
	{"[:katakana:]", "[:katakana:]","Katakana"},
	{"[:latin:]", "[:latin:]","Latin"},
	{"", "",NULL},
};

static UPGROUP pg[] = {
	"General patterns",pa,
	"Character Properties",pc,
	"Scripts", ps,
};
#define UPGCOUNT (sizeof(pg)/sizeof(UPGROUP))

struct stylestruct {
	INDEX * FF;
	LIST * ll;
};

static TCHAR *boolarray[] = {
	TEXT("Only"),
	TEXT("And"),
	TEXT("Or")
};
#define BOOLARRAYSIZE (sizeof(boolarray)/sizeof(char *))

enum {		/* boolean menu items */
	FF_ONLY,
	FF_AND,
	FF_OR
};

#define GROUPBASE IDC_FIND_NOT1
#define GROUPSIZE (IDC_FIND_NOT2-IDC_FIND_NOT1)

HWND f_lastwind;

static LRESULT CALLBACK findproc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static int fcommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);	/* does menu tasks */
static BOOL finit(HWND hwnd, HWND hwndFocus, LPARAM lParam);	/* initializes dialog */
static LRESULT CALLBACK hook(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static void factivate(HWND hwnd, UINT state, HWND hwndActDeact, BOOL fMinimized);	/* activates/deactivates */
static short ficlose(HWND hwnd);		/* closes text window */
static void initfind(HWND hwnd,short resume, BOOL protect);	/* clears things for new search */
static void setextent(HWND hwnd);	/* sets window depth */
static short testsearchok(HWND hwnd);	/* enables ok if fields completed */
static void setallbuttons(HWND hwnd);	/* sets all buttons */
static int fcheck(INDEX * FF, HWND hwnd, LISTGROUP *lg);		/* checks search parameters */
static void stylestring(char * tstring, unsigned char style, unsigned char font);	/* displays style settings */
static char * vistarget(INDEX * FF, RECORD * recptr, char *sptr, LISTGROUP *lg, short *lenptr, short subflag);	/* returns ptr if target vis */
static INT_PTR CALLBACK styleproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);


/*********************************************************************************/
short fs_testok(INDEX * FF)	/* checks if can do a blind find */

{
	return(g_findw && FF == FX(g_findw,lastindex) && testsearchok(g_findw));
}
/*********************************************************************************/
HWND find_setwindow(HWND hwnd)	/*  sets up find window */

{
	if (g_findw || (g_findw = CreateDialogParam(g_hinst,MAKEINTRESOURCE(IDD_FIND),g_hwframe,findproc,(LPARAM)hwnd)))
		fs_setpos(g_findw);		/* set its position if necessary */
	return (g_findw);
}
/************************************************************************/
void fs_setpos(HWND hwnd)	/* sets the position of the find/rep dialog box */

{
	RECT wrect, frect;
	char istring[STSTRING];
	LISTGROUP * lg;

	if (f_lastwind && f_lastwind != hwnd)	{			/* if have used another window */
		SendMessage(f_lastwind,WM_COMMAND,IDCANCEL,0);	/* dispose of it */
		if (getDItemText(f_lastwind,IDC_FIND_TEXT1,istring,sizeof(istring)))	{	/* get find string */
			lg = &FX(hwnd,lg);
			setDItemText(hwnd,IDC_FIND_TEXT1,istring);	/* set in our text window */
			strcpy(lg->lsarray[0].string,istring);		/* it's provisionally the string we use */
			fs_setwordenable(hwnd,IDC_FIND_WORD1,&lg->lsarray[0]);	/* if can't do word match */
		}
		GetWindowRect(hwnd,&wrect);
		GetWindowRect(f_lastwind,&frect);
		MoveWindow(hwnd,frect.left,frect.top,wrect.right-wrect.left,wrect.bottom-wrect.top,FALSE);
	}
	ShowWindow(hwnd,SW_SHOWNORMAL);
	SetFocus(GetDlgItem(hwnd,IDC_FIND_TEXT1));	/* set focus to first text item */
	SetActiveWindow(hwnd);
}
/************************************************************************/
static LRESULT CALLBACK findproc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)

{
	switch (msg)	{
		HANDLE_MSG(hwnd,WM_INITDIALOG,finit);
#if 0
		HANDLE_MSG(hwnd,WM_CLOSE,ficlose);
#endif
		HANDLE_MSG(hwnd,WM_ACTIVATE,factivate);
		case WM_COMMAND:
			return (HANDLE_WM_COMMAND(hwnd, wParam, lParam, fcommand));
		case WM_HOTKEY:
			fs_hotkey(hwnd,wParam);
			return (TRUE); 
		case WM_HELP:
			return (dodialoghelp(hwnd,TEXT("_Find & Replace\\Usingfind.htm"),(HELPINFO *)lParam,wh_findid));
	}
	return (FALSE);
}
/************************************************************************/
static int fcommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)	/* does menu tasks */

{
	INDEX * FF;
	int group, base,offset,titem, changed, count;
	LISTGROUP * lg;
	GROUPHANDLE gh;
	FFLIST * ffp;
	BOOL protect = FALSE;

	if ((ffp = getdata(hwnd)) && (FF = ffp->lastindex) && (hwndCtl || id == IDCANCEL) && !ffp->resetting)	{	/* if data, index, & child message */
		lg = &ffp->lg;
		changed = FALSE;
		switch (id)	{
			case IDC_FIND_FINDSTART:
#if 0
				if (fs_findnext(FF,hwnd,lg, &moffset,&mlength, FALSE))	{	/* if found the next target */
					setallbuttons(hwnd);		/* ok */
					if (FF->rwind)	/* if have record window open */
						if (trptr = rec_getrec(FF,ffp->target))	/* if can edit record */
							mod_settext(FF,trptr->rtext,trptr->num, NULL);
				}
#else
				fs_findagain(FF,hwnd);
#endif
#if 0
				else
					changed = TRUE;				/* nothing doing */
#endif
				break;
			case IDC_FIND_FINDSTOP:
				fs_reset(hwnd,lg,TRUE);
				setextent(hwnd);
				initfind(hwnd,FALSE,FALSE);	/* set up for new find */
				ffp->scope = COMR_ALL;
				ffp->dateflag = FALSE;	/* all dates */
				break;
			case IDCANCEL:
				if (ffp->target)	/* if have target */
					view_selectrec(FF,ffp->target,VD_SELPOS,-1,-1);	/* select it */
				ficlose(hwnd);
				return FALSE;
			case IDC_FIND_FINDALL:
				if (ffp->target || fcheck(FF,hwnd,lg))	{		/* if all checks ok */
					if (gh = grp_startgroup(FF))	{	/* initialize a group */
						SetCapture(hwnd);
						gh->lg = *lg;				/* load current search pars */
						SetWindowText(GetDlgItem(hwnd,IDC_FIND_FINDSTOP),TEXT("Stop"));	/* set new title */
						for (count = 0; count < lg->size; count++)	/* for all possible arrays */
							lg->lsarray[count].auxptr = NULL;		/* empty pointer (mem is freed by grp) */
						if (grp_buildfromsearch(FF,&gh))	{	/* make temporary group */
							grp_installtemp(FF,gh);
							SendMessage(FF->vwind,WM_COMMAND,IDM_VIEW_TEMPORARYGROUP,0);	/* display group */
							view_selectrec(FF,gh->recbase[0],VD_SELPOS,-1,-1);
							ficlose(hwnd);	/* hide us */
							protect = TRUE;	/* stops setting focus on hidden window */
						}
						else{
							grp_dispose(gh);
							senderr(ERR_RECNOTFOUNDERR, WARN);
						}
					}
					ReleaseCapture();
					changed = TRUE;		/* always re-initialize after find all */
				}
				break;
			case IDC_FIND_ALL:
			case IDC_FIND_SELECTED:
			case IDC_FIND_RANGE:
				if (id == IDC_FIND_RANGE)
					selectitext(hwnd,IDC_FIND_RANGESTART);
				if (ffp->scope != id-IDC_FIND_ALL)	{
					changed = TRUE;
					ffp->scope = id-IDC_FIND_ALL;
				}
				break;
			case IDC_FIND_RANGESTART:
			case IDC_FIND_RANGEEND:
				if (codeNotify == EN_CHANGE)	{
					CheckRadioButton(hwnd,IDC_FIND_ALL,IDC_FIND_RANGE, IDC_FIND_RANGE);
					changed = TRUE;
					protect = TRUE;		/* stops init selection of text */
					ffp->scope = COMR_RANGE;
				}
				break;
			case IDC_FIND_ALLDATES:
			case IDC_FIND_DATERANGE:
				if (id == IDC_FIND_DATERANGE)
					selectitext(hwnd,IDC_FIND_DATESTART);
				if (ffp->dateflag != id-IDC_FIND_ALLDATES)	{
					changed = TRUE;
					ffp->dateflag = id-IDC_FIND_ALLDATES;
				}
				break;
			case IDC_FIND_DATESTART:
			case IDC_FIND_DATEEND:
				if (codeNotify == EN_CHANGE)	{
					CheckRadioButton(hwnd,IDC_FIND_ALLDATES,IDC_FIND_DATERANGE,IDC_FIND_DATERANGE);
					changed = TRUE;
					protect = TRUE;		/* stops init selection of text */
					ffp->dateflag = TRUE;
				}
#if 1
				else if (codeNotify == EN_KILLFOCUS) {
					checkdate(hwnd, id);
				}
#endif
				break;
			case IDC_FIND_ONLYAMONG:
			case IDC_FIND_NOTAMONG:
				lg->excludeflag = id-IDC_FIND_ONLYAMONG;
				changed = TRUE;
				break;
			case IDC_FIND_NEWREC:
				checkcontrol(hwndCtl,lg->newflag ^= 1);
				changed = TRUE;
				break;
			case IDC_FIND_MODREC:
				checkcontrol(hwndCtl,lg->modflag ^= 1);
				changed = TRUE;
				break;
			case IDC_FIND_GENREC:
				checkcontrol(hwndCtl,lg->genflag ^= 1);
				changed = TRUE;
				break;
			case IDC_FIND_TAGREC:
				checkcontrol(hwndCtl,lg->tagflag ^= 1);
				changed = TRUE;
				break;
			case IDC_FIND_TAGRECLEVEL:
				if (codeNotify == CBN_SELENDOK)	{
					lg->tagvalue = ComboBox_GetCurSel(GetDlgItem(hwnd,IDC_FIND_TAGRECLEVEL));
					checkcontrol(GetDlgItem(hwnd,IDC_FIND_TAGREC),lg->tagflag = 1);
					changed = TRUE;
				}
				break;
			case IDC_FIND_DELREC:
				checkcontrol(hwndCtl,lg->delflag ^= 1);
				changed = TRUE;
				break;
			case IDC_FIND_MARKREC:
				checkcontrol(hwndCtl,lg->markflag ^= 1);
				changed = TRUE;
				break;
			case IDC_FIND_UP:
			case IDC_FIND_DOWN:
				lg->revflag = IDC_FIND_DOWN-id;
				changed = TRUE;
				break;
			case IDC_FIND_USERID:
#if 0
				if (codeNotify == EN_CHANGE)	{
					changed = TRUE;
					protect = TRUE;		/* stops init selection of text */
				}
#else
				if (codeNotify == EN_UPDATE)	{
					checktextfield(hwndCtl,5);
					changed = TRUE;
					protect = TRUE;		/* stops init selection of text */
				}
#endif
				break;
			default:
				if (id >= GROUPBASE)	{
					group = (id-GROUPBASE)/GROUPSIZE;
					offset = (id-GROUPBASE)%GROUPSIZE;
					base = id-offset;
					switch (offset)		{	/* deal with item */
						case IDC_FIND_ANDOR1-IDC_FIND_NOT1:
							if (codeNotify == CBN_SELENDOK)	{
								changed = TRUE;
								titem = ComboBox_GetCurSel(hwndCtl);
								lg->lsarray[group].andflag = titem == FF_AND ? TRUE : FALSE;
								if (titem == FF_ONLY && lg->size > group+1 ||		/* if showing too many */
										titem > FF_ONLY && lg->size == group+1)	{	/* or not enough */
									lg->size = titem == FF_ONLY ? group +1 : group+2;
									setextent(hwnd);
									if (lg->size == group+2)	{	/* if have added a new field */
										hwndCtl = GetDlgItem(hwnd,GROUPBASE+((lg->size-1)*GROUPSIZE)+IDC_FIND_ANDOR1-IDC_FIND_NOT1);	/* get handle of last bool pop */
										ComboBox_SetCurSel(hwndCtl,FF_ONLY);	/* set to only */
									}
									ffp->restart = FALSE;	/* forces selection/focus update */
								}
							}
							break;
						case IDC_FIND_NOT1-IDC_FIND_NOT1:
							checkcontrol(hwndCtl,lg->lsarray[group].notflag ^= 1);
							changed = TRUE;
							break;
						case IDC_FIND_TEXT1-IDC_FIND_NOT1:
							if (codeNotify == CBN_EDITCHANGE || codeNotify == CBN_SELENDOK)	{
								TCHAR ls[LISTSTRING];
								if (codeNotify == CBN_EDITCHANGE)
									GetWindowText(hwndCtl,ls,LISTSTRING);
								else
									ComboBox_GetLBText(hwndCtl,ComboBox_GetCurSel(hwndCtl),ls);
								strcpy(lg->lsarray[group].string,fromNative(ls));
								changed = TRUE;
								protect = TRUE;		/* stops init selection of text */
								fs_setwordenable(hwnd,base+IDC_FIND_WORD1-IDC_FIND_NOT1,&lg->lsarray[group]);	/* if can't do word match */
							}
							break;
						case IDC_FIND_STYLE1-IDC_FIND_NOT1:
							if (fs_getstyle(hwnd,FF,&lg->lsarray[group]))	{
								fs_showstyle(hwnd,base+IDC_FIND_STYLESHOW1-IDC_FIND_NOT1,lg->lsarray[group].style,lg->lsarray[group].font, lg->lsarray[group].forbiddenstyle, lg->lsarray[group].forbiddenfont);
								changed = TRUE;
							}
							break;
						case IDC_FIND_FIELDS1-IDC_FIND_NOT1:
							if (codeNotify == CBN_SELENDOK)	{
								changed = TRUE;
								lg->lsarray[group].field = fs_getfieldindex(hwndCtl);
								if (lg->lsarray[group].field == PAGEINDEX)	{	/* if page */
									if (!lg->lsarray[group].patflag)	{	/* if not seeking pattern */
										enableitem(hwnd,base+IDC_FIND_EVALREFS1-IDC_FIND_NOT1);
										checkitem(hwnd,base+IDC_FIND_EVALREFS1-IDC_FIND_NOT1,lg->lsarray[group].evalrefflag = 1);
									}
								}
								else	{
									disableitem(hwnd,base+IDC_FIND_EVALREFS1-IDC_FIND_NOT1);
									checkitem(hwnd,base+IDC_FIND_EVALREFS1-IDC_FIND_NOT1,lg->lsarray[group].evalrefflag = 0);
								}
							}
							break;
						case IDC_FIND_EVALREFS1-IDC_FIND_NOT1:
							checkcontrol(hwndCtl,lg->lsarray[group].evalrefflag ^= 1);
							changed = TRUE;
							break;
						case IDC_FIND_WORD1-IDC_FIND_NOT1:
							checkcontrol(hwndCtl,lg->lsarray[group].wordflag ^= 1);
							changed = TRUE;
							break;
						case IDC_FIND_CASE1-IDC_FIND_NOT1:
							checkcontrol(hwndCtl,lg->lsarray[group].caseflag ^= 1);
							changed = TRUE;
							break;
						case IDC_FIND_PATTERN1-IDC_FIND_NOT1:
							fs_configureforpattern(hwnd, base, lg->lsarray[group].patflag ^1);
							changed = TRUE;
							break;
					}
				}
				else		/* no command we recognize */
					return(FALSE);
		}
		testsearchok(hwnd);
		if (changed && !ffp->restart)	/* if need to reset (and not already reset) */
			initfind(hwnd,FALSE,protect);
		return (TRUE);
	}
	return (FALSE);	
}
/******************************************************************************/
void fs_configureforpattern(HWND hwnd, int base, boolean state)	// configures group for pattern enabled/disabled

{
	FFLIST * ffp = getdata(hwnd);
	LIST * ll = &ffp->lg.lsarray[(base - GROUPBASE) / GROUPSIZE];	// get right list for group
	HWND refitem = GetDlgItem(hwnd,base + IDC_FIND_EVALREFS1 - IDC_FIND_NOT1);	// eval ref item

	checkitem(hwnd, base + IDC_FIND_PATTERN1 - IDC_FIND_NOT1, state);
	ll->patflag = state;
	if (state) {	/* if want pattern */
		checkitem(hwnd, base + IDC_FIND_CASE1 - IDC_FIND_NOT1, TRUE);
		ll->caseflag = TRUE;
		if (refitem) {
			checkitem(hwnd, base + IDC_FIND_EVALREFS1 - IDC_FIND_NOT1, FALSE);
			disableitem(hwnd, base + IDC_FIND_EVALREFS1 - IDC_FIND_NOT1);
		}
		ll->evalrefflag = FALSE;
	}
	else {
		enableitem(hwnd, base + IDC_FIND_CASE1 - IDC_FIND_NOT1);
//		ll->caseflag = FALSE;
		if (refitem) {
			if (ll->field == PAGEINDEX)
				enableitem(hwnd, base + IDC_FIND_EVALREFS1 - IDC_FIND_NOT1);
		}
	}
	fs_setwordenable(hwnd, base + IDC_FIND_WORD1 - IDC_FIND_NOT1, ll);
}
/******************************************************************************/
void fs_findagain(INDEX * FF, HWND hwnd)	/* finds next matching record */

{
	if (!FF->rwind || mod_canenterrecord(FF->rwind,MREC_ALWAYSACCEPT))	{	// if no record window, or entry OK
		FFLIST * ffp = getdata(hwnd);
		short moffset, mlength;
		RECORD * trptr;

		if (fs_findnext(FF,hwnd,&ffp->lg, &moffset,&mlength, FALSE))	{	/* if found the next target */
			setallbuttons(hwnd);		/* ok */
			if (FF->rwind)	/* if have record window open */
				if (trptr = rec_getrec(FF,ffp->target))	/* if can edit record */
					mod_settext(FF,trptr->rtext,trptr->num, hwnd);
		}
	}
}
/******************************************************************************/
static BOOL finit(HWND hwnd, HWND hwndFocus, LPARAM lParam)	/* initializes dialog */

{
	FFLIST * fflist;
	int group, count;
	RECT rect1, rect2, wrect;

	if (fflist = getmem(sizeof(FFLIST)))	{	/* if can get memory for our window structure */
		setdata(hwnd,fflist);		/* install our private data */
		fs_setlabelcombo(hwnd);
		for (group = 0; group < MAXLISTS; group++)	{
			int base = GROUPBASE+group*GROUPSIZE;
			HWND cbh = GetDlgItem(hwnd, base + IDC_FIND_TEXT1 - IDC_FIND_NOT1);

			ComboBox_LimitText(cbh, LISTSTRING-1);
			(LONG_PTR)fflist->edproc = fs_installcombohook(hwnd,base+IDC_FIND_TEXT1-IDC_FIND_NOT1);
			type_settextboxfont(cbh);
			for (count = 0; count < BOOLARRAYSIZE; count++)	/* build boolean menus */
				ComboBox_AddString(GetDlgItem(hwnd,base+IDC_FIND_ANDOR1-IDC_FIND_NOT1),boolarray[count]);
		}
		hideitem(hwnd,IDC_FIND_ANDOR4);	/* this never visible */
		fflist->init = initfind;	/* function for setting up new find */

		GetWindowRect(GetDlgItem(hwnd,IDC_FIND_TEXT1),&rect1);	/* get rect of first text field */
		GetWindowRect(GetDlgItem(hwnd,IDC_FIND_TEXT2),&rect2);	/* and second */
		Edit_LimitText(GetDlgItem(hwnd,IDC_FIND_USERID),4);
		fflist->heightstep = rect2.top-rect1.top;
		GetWindowRect(hwnd,&wrect);
		fflist->baseheight = wrect.bottom-(MAXLISTS-1)*fflist->heightstep-wrect.top;
//		MoveWindow(hwnd,wrect.left,wrect.top,wrect.right-wrect.left,fflist->baseheight,FALSE);
		centerwindow(hwnd,-1);
		return (FALSE);
	}
	DestroyWindow(hwnd);
	return (FALSE);
}
/************************************************************************/
LONG_PTR fs_installcombohook(HWND hwnd,int item)		// install combo hook

{
	POINT pt = {3,3};
	HWND cb = GetDlgItem(hwnd,item);
	HWND eb = ChildWindowFromPoint(cb, pt);
	return SetWindowLongPtr(eb, GWLP_WNDPROC,(LONG_PTR)hook);		/* set subclass handler */
}
/************************************************************************/
static LRESULT CALLBACK hook(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)

{
	if (msg == WM_CONTEXTMENU)	{	// if contextual menu
		HMENU mhs = GetSubMenu(g_popmenu,4);

		if (mhs)	{		// if have submenu
			int item, group;

			menu_delall(mhs);
			for (group = 0; group < UPGCOUNT; group++)	{	// for all groups
				HMENU sm = CreateMenu();
				int count;

				for (count = 0; pg[group].pat[count].display; count++)
					menu_additem(sm,group*100+count+1,pg[group].pat[count].display);		/* add menu item */
				menu_addsubmenu(mhs,sm,pg[group].name);
			}

			item = TrackPopupMenuEx(mhs,TPM_VERTICAL|TPM_LEFTALIGN|TPM_TOPALIGN|TPM_RIGHTBUTTON|TPM_NONOTIFY|TPM_RETURNCMD,
				GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam),hwnd,NULL);
			if (item)	{
				int group = item/100;
				int index = item%100-1;
				SendMessage(hwnd,EM_REPLACESEL,TRUE,(LPARAM)toNative(pg[group].pat[index].abbrev));
				HWND textitem = GetParent(hwnd);	// wnd is handle to combo, not text box
				fs_configureforpattern(GetParent(textitem), GetDlgCtrlID(textitem)-(IDC_FIND_TEXT1 - IDC_FIND_NOT1), TRUE);
			}
		}
		return 0;
	}
	return CallWindowProc((WNDPROC)FX(GetParent(GetParent(hwnd)),edproc),hwnd,msg,wParam,lParam);	/* pass to ordinary handler */
}
/************************************************************************/
static void factivate(HWND hwnd, UINT state, HWND hwndActDeact, BOOL fMinimized)	/* activates/deactivates */

{
	INDEX * FF;
	int group, base;
	FFLIST *ffp;

	ffp = getdata(hwnd);
	if (state != WA_INACTIVE)	{	/* being activated */
		g_mdlg = hwnd;
		if (FF = index_front())	{			/* if have any index window */
			if (FF != ffp->lastindex)	{	/* if changed index in which searching */
				for (group = 0; group < MAXLISTS; group++)	{
					base = group*GROUPSIZE+GROUPBASE;
					ffp->lg.lsarray[group].andflag = FALSE;
					ffp->lg.lsarray[group].field = ALLFIELDS;
					fs_setfieldmenu(FF, GetDlgItem(hwnd,base+IDC_FIND_FIELDS1-IDC_FIND_NOT1));	/* get current menu */
					ComboBox_SetCurSel(GetDlgItem(hwnd,base+IDC_FIND_ANDOR1-IDC_FIND_NOT1),0);
					SendMessage(GetDlgItem(hwnd,base+IDC_FIND_EVALREFS1-IDC_FIND_NOT1),BM_SETCHECK,0,0);
					disableitem(hwnd,base+IDC_FIND_EVALREFS1-IDC_FIND_NOT1);
				}
				ffp->lg.size = 1;		/* default size of group */
				setextent(hwnd);
				ffp->lastindex = FF;
				initfind(hwnd,FALSE,FALSE);	/* set up for new find */
				ffp->scope = COMR_ALL;
				ffp->dateflag = FALSE;	/* all dates */
				ffp->lg.excludeflag = FALSE;	// only among
				CheckRadioButton(hwnd,IDC_FIND_ONLYAMONG,IDC_FIND_NOTAMONG, IDC_FIND_ONLYAMONG);
				CheckRadioButton(hwnd,IDC_FIND_UP,IDC_FIND_DOWN, IDC_FIND_DOWN);
				CheckRadioButton(hwnd,IDC_FIND_ALL,IDC_FIND_RANGE, IDC_FIND_ALL);
				CheckRadioButton(hwnd,IDC_FIND_ALLDATES,IDC_FIND_DATERANGE,IDC_FIND_ALLDATES);
				ComboBox_SetCurSel(GetDlgItem(hwnd,IDC_FIND_TAGRECLEVEL),0);
			}
			else if (ffp->target)		{	/* if we have a target */
				if (FF->acount != ffp->vacount)	{	/* if have had view window active */
					initfind(hwnd,TRUE,FALSE);			/* make resume settings */
#if 1
					view_clearselect(FF->vwind);	/* clear selection in case come in with wrong starter highlighted */
#endif
				}
				else		/* just reset condition buttons */
					setallbuttons(hwnd);
			}
			else		/* looking again in current index */
				initfind(hwnd,FALSE,FALSE);	/* set up for new find */
			if (FF->startnum == FF->head.rtot)	{	/* if no new records */
				disableitem(hwnd,IDC_FIND_NEWREC);
				checkitem(hwnd,IDC_FIND_NEWREC,0);	/* clear check box */
				ffp->lg.newflag = 0;	/* clear flag */
			}
			else
				enableitem(hwnd,IDC_FIND_NEWREC);
			if (FF->head.privpars.vmode == VM_FULL){	/* if formatted view */
				disableitem(hwnd,IDC_FIND_DELREC);
				checkitem(hwnd,IDC_FIND_DELREC,0);	/* clear check box */
				ffp->lg.delflag = 0;	/* clear flag */
			}
			else
				enableitem(hwnd,IDC_FIND_DELREC);
			if (view_recordselect(FF->vwind))
				enableitem(hwnd,IDC_FIND_SELECTED);
			else	{
				disableitem(hwnd,IDC_FIND_SELECTED);
				if (ffp->scope == COMR_SELECT)	{	/* if had wanted selection */
					ffp->scope = COMR_ALL;		/* set for all */
					CheckRadioButton(hwnd,IDC_FIND_ALL,IDC_FIND_RANGE, IDC_FIND_ALL);
				}
			}
			testsearchok(hwnd);
		}
		else
			ffp->lastindex = NULL;
	}
	else {
		g_mdlg = NULL;
		f_lastwind = hwnd;		/* this was last-used of find/replace */
		if (getdata(hwnd) && ffp->lastindex)	/* being deactivated */
			ffp->vacount = ffp->lastindex->acount;	/* save v window activation count */
	}
}
/******************************************************************************/
static short ficlose(HWND hwnd)		/* closes find window */

{
	HWND nhw;

	nhw = GetWindow(hwnd,GW_HWNDPREV);	/* see if window on top */
	if (hwnd == GetWindow(nhw,GW_OWNER))	/* if we have one, it's style */
		SendMessage(nhw,WM_CLOSE,0,0);	/* close it */
	ShowWindow(hwnd,SW_HIDE);	/* hide it */
	return (0);
}
/******************************************************************************/
static void initfind(HWND hwnd,short resume, BOOL protect)	/* clears things for new search */

{
	HWND cb;

	if (resume)
		SetWindowText(GetDlgItem(hwnd,IDC_FIND_FINDSTART),TEXT("Resume"));	/* set new title */
	else {
		SetWindowText(GetDlgItem(hwnd,IDC_FIND_FINDSTART),TEXT("&Find"));	/* set new title */
		FX(hwnd,target) = 0;		/* no record in which to replace */
		FX(hwnd,restart) = TRUE;
		SetWindowText(GetDlgItem(hwnd,IDC_FIND_FINDSTOP),TEXT("Re&set"));	/* set new title */
		cb = GetDlgItem(hwnd,GROUPBASE+(FX(hwnd,lg).size-1)*GROUPSIZE+IDC_FIND_TEXT1-IDC_FIND_NOT1);
		if (!protect)
#if 0
			ComboBox_SetEditSel(cb, 0, -1);
#endif
			SetFocus(cb);
	}
	testsearchok(hwnd);
}
/******************************************************************************/
static void setextent(HWND hwnd)	/* sets window depth; enables disables exposed/hidden items */

{
	RECT wrect;
	int change, oldsize, base;
	FFLIST *ffp;

	ffp = getdata(hwnd);
	GetWindowRect(hwnd,&wrect);
	oldsize = ((wrect.bottom-wrect.top)-ffp->baseheight)/ffp->heightstep+1;	/* # of groups currently displayed */
	change = (ffp->lg.size-oldsize)*GROUPSIZE;		/* number of items to add/subtract */

	MoveWindow(hwnd,wrect.left,wrect.top,wrect.right-wrect.left,ffp->baseheight+(ffp->lg.size-1)*ffp->heightstep,TRUE);	/* resize */
	if (change > 0)	{	/* if exposing items */
		for (base = GROUPBASE+GROUPSIZE*oldsize;change > 0;change--)	/* for all newly exposed items */
			enableitem(hwnd,base++);
	}
	else if (change < 0)	{
		for (base = GROUPBASE+GROUPSIZE*oldsize;change < 0;change++)	/* for all newly hidden items */
			disableitem(hwnd,--base);
	}
}
#if 0
/******************************************************************************/
static short testsearchok(HWND hwnd)	/* enables ok if fields completed */

{
	FFLIST * fp = getdata(hwnd);
	LISTGROUP *lg = &fp->lg;				/* ptr to list struct */
	int group, tlen,r1len,r2len,d1len,d2len,ulen;
	char istring[STSTRING];

	for (group = 0; group < lg->size; group++)	{	/* find if any pattern set */
		if (lg->lsarray[group].patflag)
			break;
	}
	tlen = strlen(lg->lsarray[0].string);
	r1len = getDItemText(hwnd,IDC_FIND_RANGESTART,istring,sizeof(istring));
	r2len = getDItemText(hwnd,IDC_FIND_RANGEEND,istring,sizeof(istring));
	d1len = getDItemText(hwnd,IDC_FIND_DATESTART,istring,sizeof(istring));
	d2len = getDItemText(hwnd,IDC_FIND_DATEEND,istring,sizeof(istring));
	ulen = getDItemText(hwnd,IDC_FIND_USERID,istring,sizeof(istring));
	if (tlen || group == lg->size 
		&& (lg->newflag || lg->delflag || lg->markflag || lg->modflag || lg->genflag || lg->tagflag || lg->lsarray[0].field > 0
		|| lg->lsarray[0].style || lg->lsarray[0].font || lg->lsarray[0].forbiddenstyle || lg->lsarray[0].forbiddenfont || FX(hwnd,dateflag) && (d1len || d2len))
		|| FX(hwnd,scope) == COMR_RANGE && (r1len || r2len)
		|| ulen)	{	/* if have text or permissible keys */
		setitemenable(hwnd,IDC_FIND_FINDSTART,g_mdlg != NULL);	// guaranteed disabled if window inactivated
		setitemenable(hwnd,IDC_FIND_FINDALL,g_mdlg && fp->lastindex && !fp->lastindex->rwind);	// enable only if no record window
		return (TRUE);
	}
	else {
		disableitem(hwnd,IDC_FIND_FINDSTART);
		disableitem(hwnd,IDC_FIND_FINDALL);
		return (FALSE);
	}
}
#else
/******************************************************************************/
static short testsearchok(HWND hwnd)	/* enables ok if fields completed */

{
	FFLIST * fp = getdata(hwnd);
	LISTGROUP *lg = &fp->lg;				/* ptr to list struct */
	int group, r1len, r2len, d1len, d2len, ulen;
	char istring[STSTRING];

	r1len = getDItemText(hwnd, IDC_FIND_RANGESTART, istring, sizeof(istring));
	r2len = getDItemText(hwnd, IDC_FIND_RANGEEND, istring, sizeof(istring));
	d1len = getDItemText(hwnd, IDC_FIND_DATESTART, istring, sizeof(istring));
	d2len = getDItemText(hwnd, IDC_FIND_DATEEND, istring, sizeof(istring));
	ulen = getDItemText(hwnd, IDC_FIND_USERID, istring, sizeof(istring));

	for (group = 0; group < lg->size; group++) {	// check params for each group
		LIST * ll = &lg->lsarray[group];
		BOOL ok = (strlen(ll->string) || !ll->patflag
			&& (lg->newflag || lg->delflag || lg->markflag || lg->modflag || lg->genflag || lg->tagflag || ll->field > 0
				|| ll->style || ll->font || ll->forbiddenstyle || ll->forbiddenfont || FX(hwnd, dateflag) && (d1len || d2len))
				|| FX(hwnd, scope) == COMR_RANGE && (r1len || r2len)
				|| ulen);	// ok
		if (!ok) {
			disableitem(hwnd, IDC_FIND_FINDSTART);
			disableitem(hwnd, IDC_FIND_FINDALL);
			return (FALSE);
		}
	}
	setitemenable(hwnd, IDC_FIND_FINDSTART, g_mdlg != NULL);	// guaranteed disabled if window inactivated
	setitemenable(hwnd, IDC_FIND_FINDALL, g_mdlg && fp->lastindex && !fp->lastindex->rwind);	// enable only if no record window
	return TRUE;
}
#endif
/******************************************************************************/
static int fcheck(INDEX * FF, HWND hwnd, LISTGROUP *lg)		/* checks search parameters */

{
	short group, err;
	char str1[STSTRING], str2[STSTRING];

	lg->sortmode = FF->head.sortpars.ison;
	lg->lflags = FX(hwnd, scope);		/* save scope in case need revision */
	getDItemText(hwnd, IDC_FIND_RANGESTART, lg->range0, STSTRING);
	getDItemText(hwnd, IDC_FIND_RANGEEND, lg->range1, STSTRING);
	if (err = com_getrecrange(FF, FX(hwnd, scope), lg->range0, lg->range1, &lg->firstr, &lg->lastr)) {	/* bad range */
		selectitext(hwnd, err < 0 ? IDC_FIND_RANGESTART : IDC_FIND_RANGEEND);
		return (FALSE);
	}
	getDItemText(hwnd, IDC_FIND_DATESTART, str1, STSTRING);
	getDItemText(hwnd, IDC_FIND_DATEEND, str2, STSTRING);
	if (err = com_getdates(FX(hwnd, dateflag), str1, str2, &lg->firstdate, &lg->lastdate)) {
		selectitext(hwnd, IDC_FIND_DATESTART + err - 1);
		return (FALSE);
	}
	getDItemText(hwnd, IDC_FIND_USERID, lg->userid, 5);	/* set up user id */
	if (!search_setupfind(FF, lg, &group)) {	/* if bad check expressions, etc */
		selectitext(hwnd, GROUPBASE + group * GROUPSIZE + IDC_FIND_TEXT1 - IDC_FIND_NOT1);
		return (FALSE);
	}
	for (group = 0; group < lg->size; group++)	/* for each group */
		fixcombomenu(hwnd, GROUPBASE + group * GROUPSIZE + IDC_FIND_TEXT1 - IDC_FIND_NOT1);	/* adjust combo menu */
	return (TRUE);
}
/******************************************************************************/
static void setallbuttons(HWND hwnd)	/* sets all buttons */

{	
	if (FX(hwnd,target))	{
		SetWindowText(GetDlgItem(hwnd,IDC_FIND_FINDSTART),TEXT("&Find Again"));	/* set new title */
		SetWindowText(GetDlgItem(hwnd,IDC_FIND_FINDSTOP),TEXT("Re&set"));	/* set new title */
	}
}
/******************************************************************************/
void fs_hotkey(HWND hwnd, int keyid)      /* use hotkey text for search */

{
	CSTR fstrings[MAXKEYS];
	TCHAR tstring[STSTRING];
	HWND hwf;

	str_xparse(g_prefs.keystrings, fstrings);
	if (hwf = GetFocus())	{
		GetClassName(hwf,tstring,STSTRING);
		if (!nstrcmp(tstring,TEXT("Edit")))	{ /* if an edit control */
			if (keyid < MAXKEYS && fstrings[keyid].ln)	{	/* if a good key && has string */
				char sstring[STSTRING];
				str_textcpy(sstring,fstrings[keyid].str);
				SetWindowText(hwf,toNative(sstring));
				SendMessage(hwf,EM_SETSEL,0,-1);
				SendMessage(GetParent(hwf),WM_COMMAND,MAKEWPARAM((UINT)(GetDlgCtrlID(hwf)),(UINT)EN_CHANGE), (LPARAM)hwf);
				// need above line because changing text in combo box edit control doesn't get message to Find/Rep window
			}
		}
	}
}
/******************************************************************************/
void fs_setwordenable(HWND hwnd, int item, LIST * ls)	/* sets word checkbox by text contents */

{
	char * sptr;

	sptr = ls->string;
	while (*sptr && u_isalnum(u8_toU(sptr)))
		sptr = u8_forward1(sptr);
	if (!*sptr && !ls->patflag)	/* if no punctuation && not pattern */
		enableitem(hwnd,item);
	else	{
		disableitem(hwnd,item);
		ls->wordflag = FALSE;
		checkitem(hwnd,item, FALSE);
	}
}
/******************************************************************************/
void fs_setfieldmenu(INDEX * FF, HWND hwnd)	/* sets field menu */
	
{
	int count;
	
	ComboBox_ResetContent(hwnd);		/* clear items */
	ComboBox_AddString(hwnd,TEXT("All"));
	ComboBox_AddString(hwnd,TEXT("All Text"));
	ComboBox_AddString(hwnd,TEXT("Last Text"));
	ComboBox_AddString(hwnd,toNative(FF->head.indexpars.field[PAGEINDEX].name));
	for (count = 0; count < FF->head.indexpars.maxfields-1; count++)
		ComboBox_AddString(hwnd,toNative(FF->head.indexpars.field[count].name));
	ComboBox_SetCurSel(hwnd,0);
}
/******************************************************************************/
int fs_getfieldindex(HWND ctl)	/* gets field index from list box */
	
{
	int index;

	index = ComboBox_GetCurSel(ctl);
	if (index == FP_ALL)
		return (ALLFIELDS);
	if (index == FP_ALLTEXT)
		return (ALLBUTPAGE);
	if (index == FP_LASTTEXT)
		return (LASTFIELD);
	if (index == FP_PAGE)
		return (PAGEINDEX);
	return (index-FP_MAIN);
}
/******************************************************************************/
int fs_findnext(INDEX * FF, HWND hwnd, LISTGROUP *lg, short * offset, short * lptr, short rflag)
	/* finds next match to target */
	
{
	FFLIST * ffp = getdata(hwnd);
	char *sptr;
	RECORD * recptr;
	HCURSOR ocurs;
		
	lg->sortmode = FF->head.sortpars.ison;
	if (ffp->target || fcheck(FF,hwnd,lg))	{		// if have target or all checks ok
		SetWindowText(GetDlgItem(hwnd,IDC_FIND_FINDSTOP),TEXT("Stop"));	/* set new title */
		view_clearselect(FF->vwind);	/* clear any highlight */
		ocurs = SetCursor(g_waitcurs);
		SetCapture(hwnd);
		do {
			recptr = search_findfirst(FF,lg,ffp->restart,&sptr,lptr);		/* while target in invis part of rec */
			if (recptr)	{	// if a hit
				sptr = vistarget(FF,recptr,sptr,lg, lptr, rflag);	// find if target visible
				if (sptr)	// target visible, so done
					break;
				if (lg->revflag)	{	// if reverse search, keep going to find visible
					ffp->restart = FALSE;
					FF->lastfound = recptr->num;
				}
			}
		} while (recptr);
		ReleaseCapture();
		SetCursor(ocurs);
		if (recptr && sptr)	{
			view_selectrec(FF,recptr->num,VD_SELPOS,sptr-recptr->rtext,*lptr ? *lptr : -1);
			*offset = sptr-recptr->rtext;
			ffp->restart = FALSE;		/* can proceed with search */
			ffp->target = recptr->num;
			return (TRUE);
		}
		else if (ffp->restart)		/* if we've had a completely failed search */
			senderr(ERR_RECNOTFOUNDERR, WARN);
		else	/* found something */
			sendinfo(INFO_NOMOREREC);		/* done */
		ffp->init(hwnd,FALSE,FALSE);	/* reinitialize for find or rep window */
	}
	return (0);
}
/******************************************************************************/
static char * vistarget(INDEX * FF, RECORD * recptr, char *sptr, LISTGROUP *lg, short *lenptr, short subflag)	/* returns ptr if target vis */

{
	short hlevel, sprlevel, hidelevel,clevel;
	char * tptr, *uptr;
	
	if (*lenptr && (FF->head.privpars.vmode || subflag))	{	/* if finite match && formatted or substituting */
		uptr = rec_uniquelevel(FF,recptr,&hlevel,&sprlevel,&hidelevel,&clevel);		/* find unique level */
		while (sptr < uptr) 	{	/* while target before unique level */
			short mlen;
			if (tptr = search_findbycontent(FF, recptr, sptr+*lenptr, lg, &mlen))	{	/* if a target after current */
				sptr = tptr;	/* set ptr */
				*lenptr = mlen;	/* and length */
			}
			else {			/* no targets at level below unique */
				if (hlevel < PAGEINDEX || sptr+strlen(sptr) < uptr || subflag)	/* if unique level isn't lowest (unsuppressed), or substituting */
					return (NULL);		/* skip */
				break;
			}
		}
	}
	return (sptr);
}
/*******************************************************************************/
int fs_getstyle(HWND hwnd, INDEX * FF, LIST * ll)	// gets find attributes

{
	struct stylestruct ss;

	ss.FF = FF;
	ss.ll = ll;
	if (DialogBoxParam(g_hinst, MAKEINTRESOURCE(IDD_FIND_STYLE1), hwnd, styleproc, (LPARAM)&ss))
		return (TRUE);
	return (FALSE);
}
/*******************************************************************************/
static INT_PTR CALLBACK styleproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)

{
	struct stylestruct * ssp = getdata(hwnd);
	TCHAR fname[FSSTRING];
	INDEX * FF;
	HWND cbh;
	int action;
	LIST * ll;
	
	if (ssp)
		ll = ssp->ll;

	switch (msg) {

	case WM_INITDIALOG:
		if (ssp = setdata(hwnd, (void *)lParam)) {	/* set data */
			FF = ssp->FF;
			ll = ssp->ll;
			cbh = GetDlgItem(hwnd, IDC_FINDSTYLE_FONT);
			char fontid;

			for (int count = 0; count < STYLEIDS; count++) {	/* for all styles */
				action = 0;

				if (ll->forbiddenstyle&(1 << count))	// if forbidden
					action = 2;
				else if (ll->style&(1 << count))	// if required
					action = 1;
				CheckRadioButton(hwnd, sidarray[count], sidarray[count] + 2, sidarray[count] + action);
			}
			if (ll->font) {
				action = 1;
				fontid = ll->font&FX_FONTMASK;
			}
			else if (ll->forbiddenfont) {
				action = 2;
				fontid = ll->forbiddenfont&FX_FONTMASK;
			}
			else {
				action = 0;
				EnableWindow(cbh, FALSE);
				fontid = -1;
			}
			for (int count = 1; *FF->head.fm[count].name; count++)
				ComboBox_AddString(cbh, toNative(FF->head.fm[count].pname));
			ComboBox_InsertString(cbh, 0, TEXT("<Default Font>"));	/* add default item */
			ComboBox_SetCurSel(cbh, fontid);
			CheckRadioButton(hwnd, IDC_FINDSTYLE_FONTIG, IDC_FINDSTYLE_FONTREM, IDC_FINDSTYLE_FONTIG + action);
			centerwindow(hwnd, 0);
		}
		return FALSE;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
			case IDOK:
				ll->style = ll->forbiddenstyle = ll->font = ll->forbiddenfont = 0;
				for (int count = 0; count < STYLEIDS; count++) {	/* for all styles */
					action = findgroupcheck(hwnd, sidarray[count], sidarray[count] + 2) - sidarray[count];
					if (action == 2)		// forbidden
						ll->forbiddenstyle |= (1 << count);	/* set style bit */
					else if (action == 1)	// required
						ll->style |= (1 << count);	/* set style bit */
				}
				action = findgroupcheck(hwnd, IDC_FINDSTYLE_FONTIG, IDC_FINDSTYLE_FONTREM) - IDC_FINDSTYLE_FONTIG;
				if (action) {		// if doing something with a font
					fname[FSSTRING];
					int wantedfont;
					cbh = GetDlgItem(hwnd, IDC_FINDSTYLE_FONT);
					if (ComboBox_GetCurSel(cbh) > 0) {	/* if not default font */
						GetWindowText(cbh, fname, FSSTRING);
						wantedfont = type_maplocal(ssp->FF->head.fm, fromNative(fname), 1);	/* get local id */
					}
					else
						wantedfont = 0;
					if (action == 1)
						ll->font = wantedfont | FX_FONT;
					else
						ll->forbiddenfont = wantedfont | FX_FONT;
				}
			case IDCANCEL:
				EndDialog(hwnd, LOWORD(wParam) == IDOK ? TRUE : FALSE);
				return TRUE;
			case IDC_FINDSTYLE_FONTIG:
			case IDC_FINDSTYLE_FONTSET:
			case IDC_FINDSTYLE_FONTREM:
				CheckRadioButton(hwnd, IDC_FINDSTYLE_FONTIG, IDC_FINDSTYLE_FONTREM, LOWORD(wParam));
				cbh = GetDlgItem(hwnd, IDC_FINDSTYLE_FONT);
				EnableWindow(cbh, !isitemchecked(hwnd, IDC_FINDSTYLE_FONTIG));
				ComboBox_SetCurSel(cbh, IsWindowEnabled(cbh) ? 0 : -1);	// enable/disable combo per mode
				return (TRUE);
			case IDC_FINDSTYLE_SUPAP:
				if (isitemchecked(hwnd, IDC_FINDSTYLE_SUBAP))
					CheckRadioButton(hwnd, IDC_FINDSTYLE_SUBIG, IDC_FINDSTYLE_SUBREM, IDC_FINDSTYLE_SUBIG);
				return TRUE;
			case IDC_FINDSTYLE_SUBAP:
				if (isitemchecked(hwnd, IDC_FINDSTYLE_SUPAP))
					CheckRadioButton(hwnd, IDC_FINDSTYLE_SUPIG, IDC_FINDSTYLE_SUPREM, IDC_FINDSTYLE_SUPIG);
				return TRUE;
		}
		break;
	case WM_HELP:
		return (dodialoghelp(hwnd, TEXT("_Find & Replace\\Find_stylesandfonts.htm"), (HELPINFO *)lParam, wh_attribid));
	}
	return (FALSE);
}
/*******************************************************************************/
void fs_showstyle(HWND hwnd, int item, unsigned char style, unsigned char font, unsigned char forbiddenstyle, unsigned char forbiddenfont)	/* displays style settings */

{
	char tstring[30];

	stylestring(tstring, style, font);
	if (forbiddenstyle || forbiddenfont) {
//		sprintf(tstring, " %c%c%c",0xe2,0x80,0x93);	// en dash
		strcat(tstring, " -");	// en dash
		stylestring(tstring+strlen(tstring),forbiddenstyle, forbiddenfont);
	}
	setDItemText(hwnd,item,tstring);
}
/*******************************************************************************/
static void stylestring(char * tstring, unsigned char style, unsigned char font)	/* displays style settings */

{
	static char pmark[] = { 0xc2,0xa7,0 };	// paragraph mark (for font) in utf-8 

	*tstring = '\0';
	if (style&FX_BOLD)
		strcat(tstring, "B");
	if (style&FX_ITAL)
		strcat(tstring, "I");
	if (style&FX_ULINE)
		strcat(tstring, "U");
	if (style&FX_SMALL)
		strcat(tstring, "S");
	if (style&FX_SUPER)
		strcat(tstring, "Sp");
	if (style&FX_SUB)
		strcat(tstring, "Sb");
	if (font)
		strcat(tstring, pmark);
}
/*******************************************************************************/
void fs_reset(HWND hwnd, LISTGROUP * lg, int findflag)	/* resets search sturct & common window items to start condition */

{
	int group, base;
	INDEX * FF = FX(hwnd,lastindex);
	
	FX(hwnd,resetting) = TRUE;
	search_clearauxbuff(lg);
	memset(lg,0,sizeof(LISTGROUP));
	lg->size = 1;		/* default size of group */
	for (group = 0; group < MAXLISTS; group++)	{
		base = group*GROUPSIZE+GROUPBASE;
		lg->lsarray[group].field = ALLFIELDS;
		SetWindowText(GetDlgItem(hwnd,base+IDC_FIND_TEXT1-IDC_FIND_NOT1),TEXT(""));
		fs_setfieldmenu(FF, GetDlgItem(hwnd,base+IDC_FIND_FIELDS1-IDC_FIND_NOT1));	/* get current menu */
		if (findflag)	{		/* if Find, not Replace */
			checkitem(hwnd,base+IDC_FIND_NOT1-IDC_FIND_NOT1,0);	/* clear 'not' */
			ComboBox_SetCurSel(GetDlgItem(hwnd,base+IDC_FIND_ANDOR1-IDC_FIND_NOT1),0);
			SendMessage(GetDlgItem(hwnd,base+IDC_FIND_EVALREFS1-IDC_FIND_NOT1),BM_SETCHECK,0,0);
			disableitem(hwnd,base+IDC_FIND_EVALREFS1-IDC_FIND_NOT1);
		}
		checkitem(hwnd,base+IDC_FIND_WORD1-IDC_FIND_NOT1,0);	/* no word search */
		if (!findflag)	/* Replace */
			lg->lsarray[group].caseflag = TRUE;
		checkitem(hwnd,base+IDC_FIND_CASE1-IDC_FIND_NOT1,lg->lsarray[group].caseflag);	/* case search */
		checkitem(hwnd,base+IDC_FIND_PATTERN1-IDC_FIND_NOT1,0);	/* no pattern search */
	}
	CheckRadioButton(hwnd,IDC_FIND_ONLYAMONG,IDC_FIND_NOTAMONG, IDC_FIND_ONLYAMONG);
	CheckRadioButton(hwnd,IDC_FIND_UP,IDC_FIND_DOWN, IDC_FIND_DOWN);
	CheckRadioButton(hwnd,IDC_FIND_ALL,IDC_FIND_RANGE, IDC_FIND_ALL);
	CheckRadioButton(hwnd,IDC_FIND_ALLDATES,IDC_FIND_DATERANGE,IDC_FIND_ALLDATES);
	checkitem(hwnd,IDC_FIND_NEWREC,0);	/* clear check box */
	checkitem(hwnd,IDC_FIND_MODREC,0);	/* clear check box */
	checkitem(hwnd,IDC_FIND_GENREC,0);	/* clear check box */
	checkitem(hwnd,IDC_FIND_DELREC,0);	/* clear check box */
	checkitem(hwnd,IDC_FIND_MARKREC,0);	/* clear check box */
	checkitem(hwnd,IDC_FIND_TAGREC,0);	/* clear check box */
	ComboBox_SetCurSel(GetDlgItem(hwnd,IDC_FIND_TAGRECLEVEL),0);
	if (findflag)	/* if FIND */
		setDItemText(hwnd,IDC_FIND_USERID,g_nullstr);	/* clear text */
	setDItemText(hwnd,IDC_FIND_RANGESTART,g_nullstr);	/* clear text */
	setDItemText(hwnd,IDC_FIND_RANGEEND,g_nullstr);	/* clear text */
	setDItemText(hwnd,IDC_FIND_DATESTART,g_nullstr);	/* clear text */
	setDItemText(hwnd,IDC_FIND_DATEEND,g_nullstr);	/* clear text */
	setDItemText(hwnd,IDC_FIND_STYLESHOW1,g_nullstr);	/* clear text */
	if (!findflag)	{
		setDItemText(hwnd,IDC_REPLACE_TEXT,g_nullstr);	/* clear text */
		setDItemText(hwnd,IDC_REPLACE_STYLESHOW,g_nullstr);	/* clear text */
	}
	FX(hwnd,resetting) = FALSE;
}
/******************************************************************************/
void fs_setlabelcombo(HWND hwnd)		// set label combo box

{
	HWND cb = GetDlgItem(hwnd,IDC_FIND_TAGRECLEVEL);
	int count;

	for (count = 0; count < FLAGLIMIT; count++)	{	/* build boolean menus */
		TCHAR text[10];
		u_sprintf(text,count ? "%d" : "Any", count);
		ComboBox_AddString(cb,text);
	}
	ComboBox_SetCurSel(cb,0);
}

