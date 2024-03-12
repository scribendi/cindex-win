#include "stdafx.h"
#include <string.h>
#include <htmlhelp.h>
#include "errors.h"
#include "commands.h"
#include "sort.h"
#include "collate.h"
#include "group.h"
#include "util.h"
#include "strings.h"
#include "viewset.h"
#include "registry.h"

static const DWORD wh_sortid[] = {
	IDC_SORT_LANGUAGE, HIDC_SORT_LANGUAGE,
	IDC_SORT_TEXTSIMPLE, HIDC_SORT_TEXTSIMPLE,
	IDC_SORT_TEXTCHARPRI, HIDC_SORT_TEXTCHARPRI,
	IDC_SORT_TEXTIGNH, HIDC_SORT_TEXTIGNH,
	IDC_SORT_TEXTIGNP, HIDC_SORT_TEXTIGNP,
	IDC_SORT_TEXTEVALN, HIDC_SORT_TEXTEVALN,
	IDC_SORT_TEXTPARENPHRASE, HIDC_SORT_TEXTPARENPHRASE,
	IDC_SORT_TEXTPAREN, HIDC_SORT_TEXTPAREN,
	IDC_SORT_SCRIPTFIRST,HIDC_SORT_SCRIPTFIRST,
	IDC_SORT_SUBSTITUTE, HIDC_SORT_SUBSTITUTE,
	IDC_SORT_TEXTPREFIX, HIDC_SORT_TEXTPREFIX,
	IDC_SORT_FIELDLIST, HIDC_SORT_FIELDLIST,
	IDC_SORT_FIELDSIGNLOWEST, HIDC_SORT_FIELDSIGNLOWEST,
	IDC_SORT_REFPRI, HIDC_SORT_REFPRI,
	IDC_SORT_REFSASCEND, HIDC_SORT_REFSASCEND,
	IDC_SORT_REFSALL, HIDC_SORT_REFSALL,
	IDC_SORT_REFSEGORDER, HIDC_SORT_REFSEGORDER,
	IDC_SORT_REFSTYLEPRI,HIDC_SORT_REFSTYLEPRI,
	IDC_SORT_LEFT_RIGHT,HIDC_SORT_LEFT_RIGHT,
	0,0
};

static const DWORD wh_sortsubid[] = {
	IDC_SORT_SUB_LIST,HIDC_SORT_SUB_LIST,
	IDC_SORT_SUB_ADD, HIDC_SORT_SUB_ADD,
	IDC_SORT_SUB_REMOVE, HIDC_SORT_SUB_REMOVE,
	IDC_SORT_SUB_SOURCE, HIDC_SORT_SUB_SOURCE,
	IDC_SORT_SUB_REP, HIDC_SORT_SUB_REP,
	0,0
};

static TCHAR * s_sorthelptext = TEXT("_Sorting\\Sorting_alphabetizing.htm");
static TCHAR * s_sorthelpfields = TEXT("_Sorting\\Sorting_fields.htm");
static TCHAR * s_sorthelplocators = TEXT("_Sorting\\Sorting_locators.htm");

static const DWORD wh_compid[] = {
	IDC_COMPRESS_CONSOLIDATE, HIDC_COMPRESS_CONSOLIDATE,
	IDC_COMPRESS_REMDEL, HIDC_COMPRESS_REMDEL,
	IDC_COMPRESS_REMDUP, HIDC_COMPRESS_REMDUP,
	IDC_COMPRESS_REMEMPTY, HIDC_COMPRESS_REMEMPTY,
	IDC_COMPRESS_REMGEN, HIDC_COMPRESS_REMGEN,
	IDC_COMPRESS_IGNORELABELS,HIDC_COMPRESS_IGNORELABELS,
	0,0
};

struct comp {		/* composite struct for dialog box */
	INDEXPARAMS *ip;
	SORTPARAMS *sp;
	int dragitem;
};

struct sub {	// composite for substitute dialog
	char * substring;
	int controlID;
	int listitem;
	BOOL lock;
};

static short getsortpars(INDEXPARAMS *ig, SORTPARAMS *sg, HWND hwnd);		/* sets up sort pars */
static INT_PTR CALLBACK stextproc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static void checkscript (HWND hwnd);	// configures script first
static void rulebuttons (HWND hwnd, short rule, short * charpri);	/* enables and disables by sort rule */
static INT_PTR CALLBACK sfieldproc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK srefproc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK compproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static void sortsub(HWND hwnd, char * sstring);	// sets up substitutions
static INT_PTR CALLBACK subproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static BOOLEAN recoverstrings(HWND lwind, char * substrings, TCHAR * source, int column);
BOOL istextok(HWND hwnd, struct sub * sbp);

/*******************************************************************************/
void ss_sortindex(HWND hwnd)	/* re-sorts index */

{
	INDEX * FF;
	INDEXPARAMS *ig;
	SORTPARAMS *sg;
	
	if (hwnd)	{		/* if have index */
		FF = getowner(hwnd);
		sg = FF->curfile ? &FF->curfile->sg : &FF->head.sortpars;
		ig = &FF->head.indexpars;
	}
	else	{			/* lese setting prefs */
		sg = &g_prefs.sortpars;
		ig = &g_prefs.indexpars;
	}
	if (getsortpars(ig,sg, hwnd) && hwnd)	{	/* if not cancelled  & for index */
		col_init(sg,FF);	// rebuild tables
		if (FF->curfile)
			sort_sortgroup(FF);
		else
			sort_resort(FF);
		view_redisplay(FF,0,VD_TOP|VD_RESET);
		view_setstatus(hwnd);
	}
}
/**********************************************************************************/	
static short getsortpars(INDEXPARAMS *ig, SORTPARAMS *sg, HWND hwnd)		/* sets up sort pars */

{
#define NPAGES 3

	PROPSHEETHEADER psh;
	PROPSHEETPAGE psp[NPAGES];
	int count;
	struct comp tc;
	SORTPARAMS tsg;

	tsg = *sg;		/* make copy of sort pars */
	tc.ip = ig;		/* pointer ot index group */
	tc.sp = &tsg;	/* pointer to sort group */

	memset(psp,0,sizeof(psp));
	for (count = 0; count < NPAGES; count++)	{
		psp[count].dwSize = sizeof(PROPSHEETPAGE);
		psp[count].dwFlags = PSP_HASHELP;
		psp[count].hInstance = g_hinst;
		psp[count].pszTemplate = MAKEINTRESOURCE(count+IDD_SORT_TEXT);
		psp[count].lParam = (LONG_PTR)&tc;
	}
	psp[0].pfnDlgProc = stextproc;
	psp[1].pfnDlgProc = sfieldproc;
	psp[2].pfnDlgProc = srefproc;

	memset(&psh,0,sizeof(psh));
	psh.dwSize = sizeof(psh);
	psh.dwFlags = PSH_PROPSHEETPAGE|PSH_HASHELP|PSH_NOAPPLYNOW;
	psh.hwndParent = hwnd ? hwnd : g_hwframe;
	psh.hInstance = g_hinst;
	psh.pszCaption = TEXT("Sort");
	psh.nPages = NPAGES;
	psh.ppsp = psp;

	if (PropertySheet(&psh))	{		/* ok */
		*sg = tsg;		/* load new sort params */
		return (TRUE);
	}
	return (FALSE);
}
/**********************************************************************************/	
static INT_PTR CALLBACK stextproc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)

{
	typedef struct {
		TCHAR * name;
		int type;
	} RULE;

	struct comp * tcp;
	static RULE rules[] = {
		TEXT("Simple"),RAWSORT,
		TEXT("Letter-by-Letter"),LETTERSORT,
		TEXT("Letter-by-Letter (CMS)"),LETTERSORT_CMS,
		TEXT("Letter-by-Letter (ISO)"),LETTERSORT_ISO,
		TEXT("Letter-by-Letter (SBL)"),LETTERSORT_SBL,
		TEXT("Word-by-Word"),WORDSORT,
		TEXT("Word-by-Word (CMS)"),WORDSORT_CMS,
		TEXT("Word-by-Word (ISO)"),WORDSORT_ISO,
		TEXT("Word-by-Word (SBL)"),WORDSORT_SBL
	};
#define RULECOUNT ((sizeof rules)/sizeof(RULE))
	static short defcharpri[] = {0,1,2,-1};
	HWND hwdl;
	int index;

	tcp = getdata(hwnd);
	switch (msg)	{
		case WM_INITDIALOG:
			if (tcp = setdata(hwnd,(void *)((LPPROPSHEETPAGE)lParam)->lParam))	{	/* set data */
				centerwindow(GetParent(hwnd),1);		/* need to center whole prop sheet at this stage (when init first page) */
				hwdl = GetDlgItem(hwnd,IDC_SORT_TEXTCHARPRI);
				MakeDragList(hwdl);
				Edit_LimitText(GetDlgItem(hwnd,IDC_SORT_TEXTPREFIX),STSTRING-1);
				setDItemText(hwnd,IDC_SORT_TEXTPREFIX,tcp->sp->ignore);
				hwdl = GetDlgItem(hwnd,IDC_SORT_LANGUAGE);
				for (index = 0; index < cl_languagecount; index++)	{
					ComboBox_AddString(hwdl,toNative(cl_languages[index].name));
					if (!strcmp(tcp->sp->localeID,cl_languages[index].localeID))	{	// if our locale (try full first)
						ComboBox_SetCurSel(hwdl,index);	// select sort
						checkscript(hwnd);
					}
				}
				rulebuttons(hwnd,tcp->sp->type,tcp->sp->charpri);
				hwdl = GetDlgItem(hwnd,IDC_SORT_TEXTSIMPLE);
				for (index = 0; index < RULECOUNT; index++)	{
					ComboBox_AddString(hwdl,rules[index].name);
					if (rules[index].type == tcp->sp->type)	// if this is our sort
						ComboBox_SetCurSel(hwdl,index);	// select it
				}
				rulebuttons(hwnd,tcp->sp->type,tcp->sp->charpri);
				checkitem(hwnd,IDC_SORT_TEXTIGNH,tcp->sp->ignoreslash);
				checkitem(hwnd,IDC_SORT_TEXTIGNP,tcp->sp->ignorepunct);
				checkitem(hwnd,IDC_SORT_TEXTEVALN,tcp->sp->evalnums);
				checkitem(hwnd,IDC_SORT_TEXTPARENPHRASE,tcp->sp->ignoreparenphrase);
				checkitem(hwnd,IDC_SORT_TEXTPAREN,tcp->sp->ignoreparen);
				checkitem(hwnd,IDC_SORT_SCRIPTFIRST,tcp->sp->nativescriptfirst);
			}
			return (TRUE);
		case WM_NOTIFY:
			switch (((NMHDR *)lParam)->code)	{
				case PSN_KILLACTIVE:
					recoverdraglist(GetDlgItem(hwnd,IDC_SORT_TEXTCHARPRI), tcp->sp->charpri);
					tcp->sp->ignoreslash = (char)isitemchecked(hwnd,IDC_SORT_TEXTIGNH);
					tcp->sp->ignorepunct = (char)isitemchecked(hwnd,IDC_SORT_TEXTIGNP);
					tcp->sp->evalnums = (char)isitemchecked(hwnd,IDC_SORT_TEXTEVALN);
					tcp->sp->ignoreparenphrase = (char)isitemchecked(hwnd,IDC_SORT_TEXTPARENPHRASE);
					tcp->sp->ignoreparen = (char)isitemchecked(hwnd,IDC_SORT_TEXTPAREN);
					tcp->sp->nativescriptfirst = (char)isitemchecked(hwnd,IDC_SORT_SCRIPTFIRST);
					index = ComboBox_GetCurSel(GetDlgItem(hwnd,IDC_SORT_LANGUAGE));
					strcpy(tcp->sp->language,cl_languages[index].id);
					strcpy(tcp->sp->localeID,cl_languages[index].localeID);
					getDItemText(hwnd,IDC_SORT_TEXTPREFIX,tcp->sp->ignore,STSTRING);
					SetWindowLongPtr(hwnd, DWLP_MSGRESULT,FALSE);
					return (TRUE);		/* check our results; SetwindowLong TRUE to hold page; FALSE to free */
				case PSN_HELP:
					dowindowhelp(s_sorthelptext);
					return (TRUE);
			}
			break;
		case WM_COMMAND:
			if (HIWORD(wParam) == EN_UPDATE)	{
				WORD id = LOWORD(wParam);
				int length = 0;

				if (id == IDC_SORT_TEXTPREFIX)
					length = STSTRING;
				checktextfield((HWND)lParam,length);
				return TRUE;
			}
			switch (LOWORD(wParam))	{
				case IDC_SORT_TEXTSIMPLE:
					if (HIWORD(wParam) == CBN_SELCHANGE)	{
						tcp->sp->type = rules[ComboBox_GetCurSel((HWND)lParam)].type;
						rulebuttons(hwnd,tcp->sp->type, defcharpri);
					}
					break;
				case IDC_SORT_LANGUAGE:
					if (HIWORD(wParam) == CBN_SELCHANGE)
						checkscript(hwnd);
					break;
				case IDC_SORT_SUBSTITUTE:
					sortsub(hwnd,tcp->sp->substitutes);
					break;

			}
			break;
		case WM_HELP:
			return (dodialoghelp(hwnd,TEXT("_Sorting\\Sorting_entries.htm"),(HELPINFO *)lParam,wh_sortid));
		default:
			if (msg == WMM_DRAGMESSAGE)	{	/* if from drag list */
				SetWindowLongPtr(hwnd, DWLP_MSGRESULT,handledrag(hwnd,lParam, &tcp->dragitem));
				return (TRUE);
			}
	}
	return (FALSE);
}
/**********************************************************************************/	
static void checkscript (HWND hwnd)	// configures script precedence

{
	char * script = cl_languages[ComboBox_GetCurSel(GetDlgItem(hwnd,IDC_SORT_LANGUAGE))].script;
	BOOLEAN islatin = !strcmp("Latn", script);

	if (islatin)	{
		disableitem(hwnd,IDC_SORT_SCRIPTFIRST);
		checkitem(hwnd,IDC_SORT_SCRIPTFIRST,TRUE);
	}
	else {
		enableitem(hwnd,IDC_SORT_SCRIPTFIRST);
		checkitem(hwnd,IDC_SORT_SCRIPTFIRST,FALSE);
	}
}
/**********************************************************************************/	
static void rulebuttons (HWND hwnd, short rule, short * charpri)	/* enables and disables by sort rule */

{
	static char chprilist[] = "Symbols\0Numbers\0Letters\0\177";
	HWND hwdl = GetDlgItem(hwnd,IDC_SORT_TEXTCHARPRI);

	switch (rule) {
		case RAWSORT:
			checkitem(hwnd,IDC_SORT_TEXTIGNH,FALSE);
			checkitem(hwnd,IDC_SORT_TEXTIGNP,FALSE);
			checkitem(hwnd,IDC_SORT_TEXTEVALN,FALSE);
			checkitem(hwnd,IDC_SORT_TEXTPARENPHRASE,FALSE);
			checkitem(hwnd,IDC_SORT_TEXTPAREN,FALSE);
			disableitem(hwnd,IDC_SORT_TEXTIGNH);
			disableitem(hwnd,IDC_SORT_TEXTIGNP);
			disableitem(hwnd,IDC_SORT_TEXTPARENPHRASE);
			disableitem(hwnd,IDC_SORT_TEXTPAREN);
			disableitem(hwnd,IDC_SORT_TEXTEVALN);
			disableitem(hwnd,IDC_SORT_TEXTPREFIX);
			disableitem(hwnd, IDC_SORT_SUBSTITUTE);
			enableitem(hwnd,IDC_SORT_TEXTCHARPRI);
			break;
		case LETTERSORT:
			checkitem(hwnd,IDC_SORT_TEXTIGNH,TRUE); // ignore slashes
			checkitem(hwnd,IDC_SORT_TEXTIGNP,FALSE);
			checkitem(hwnd,IDC_SORT_TEXTEVALN,FALSE);
			checkitem(hwnd,IDC_SORT_TEXTPARENPHRASE,FALSE);
			checkitem(hwnd,IDC_SORT_TEXTPAREN,FALSE);
			disableitem(hwnd,IDC_SORT_TEXTIGNH);
			enableitem(hwnd,IDC_SORT_TEXTIGNP);	
			enableitem(hwnd,IDC_SORT_TEXTEVALN);
			enableitem(hwnd,IDC_SORT_TEXTPARENPHRASE);
			enableitem(hwnd,IDC_SORT_TEXTPAREN);
			enableitem(hwnd,IDC_SORT_TEXTPREFIX);
			enableitem(hwnd, IDC_SORT_SUBSTITUTE);
			enableitem(hwnd,IDC_SORT_TEXTCHARPRI);
			break;
		case LETTERSORT_CMS:
			checkitem(hwnd,IDC_SORT_TEXTIGNH,TRUE); // ignore slashes
			checkitem(hwnd,IDC_SORT_TEXTIGNP,BST_INDETERMINATE);
			checkitem(hwnd,IDC_SORT_TEXTEVALN,TRUE);	// eval nums
			checkitem(hwnd,IDC_SORT_TEXTPARENPHRASE,TRUE);
			checkitem(hwnd,IDC_SORT_TEXTPAREN,FALSE);
			disableitem(hwnd,IDC_SORT_TEXTIGNH);
			disableitem(hwnd,IDC_SORT_TEXTIGNP);	
			disableitem(hwnd,IDC_SORT_TEXTEVALN);
			disableitem(hwnd,IDC_SORT_TEXTPARENPHRASE);
			enableitem(hwnd,IDC_SORT_TEXTPAREN);
			enableitem(hwnd,IDC_SORT_TEXTPREFIX);
			enableitem(hwnd, IDC_SORT_SUBSTITUTE);
			disableitem(hwnd,IDC_SORT_TEXTCHARPRI);
			break;
		case LETTERSORT_ISO:
			checkitem(hwnd,IDC_SORT_TEXTIGNH,TRUE); // ignore slashes
			checkitem(hwnd,IDC_SORT_TEXTIGNP,TRUE);	// ignores punct
			checkitem(hwnd,IDC_SORT_TEXTEVALN,TRUE);	// eval nums
			checkitem(hwnd,IDC_SORT_TEXTPARENPHRASE,TRUE);
			checkitem(hwnd,IDC_SORT_TEXTPAREN,FALSE);
			disableitem(hwnd,IDC_SORT_TEXTIGNH);
			disableitem(hwnd,IDC_SORT_TEXTIGNP);	
			disableitem(hwnd,IDC_SORT_TEXTEVALN);
			disableitem(hwnd,IDC_SORT_TEXTPARENPHRASE);
			enableitem(hwnd,IDC_SORT_TEXTPAREN);
			enableitem(hwnd,IDC_SORT_TEXTPREFIX);
			enableitem(hwnd, IDC_SORT_SUBSTITUTE);
			disableitem(hwnd,IDC_SORT_TEXTCHARPRI);
			break;
		case LETTERSORT_SBL:
			checkitem(hwnd,IDC_SORT_TEXTIGNH,TRUE); // ignore slashes
			checkitem(hwnd,IDC_SORT_TEXTIGNP,BST_INDETERMINATE);
			checkitem(hwnd,IDC_SORT_TEXTEVALN,TRUE);	// eval nums
			checkitem(hwnd,IDC_SORT_TEXTPARENPHRASE,FALSE);
			checkitem(hwnd,IDC_SORT_TEXTPAREN,FALSE);
			disableitem(hwnd,IDC_SORT_TEXTIGNH);
			disableitem(hwnd,IDC_SORT_TEXTIGNP);	
			disableitem(hwnd,IDC_SORT_TEXTEVALN);
			enableitem(hwnd,IDC_SORT_TEXTPARENPHRASE);
			enableitem(hwnd,IDC_SORT_TEXTPAREN);
			enableitem(hwnd,IDC_SORT_TEXTPREFIX);
			enableitem(hwnd, IDC_SORT_SUBSTITUTE);
			disableitem(hwnd,IDC_SORT_TEXTCHARPRI);
			break;
		case WORDSORT:
			checkitem(hwnd,IDC_SORT_TEXTIGNH,FALSE);
			checkitem(hwnd,IDC_SORT_TEXTIGNP,FALSE);
			checkitem(hwnd,IDC_SORT_TEXTEVALN,FALSE);
			checkitem(hwnd,IDC_SORT_TEXTPARENPHRASE,FALSE);
			checkitem(hwnd,IDC_SORT_TEXTPAREN,FALSE);
			enableitem(hwnd,IDC_SORT_TEXTIGNH);
			enableitem(hwnd,IDC_SORT_TEXTIGNP);	
			enableitem(hwnd,IDC_SORT_TEXTEVALN);
			enableitem(hwnd,IDC_SORT_TEXTPARENPHRASE);
			enableitem(hwnd,IDC_SORT_TEXTPAREN);
			enableitem(hwnd,IDC_SORT_TEXTPREFIX);
			enableitem(hwnd, IDC_SORT_SUBSTITUTE);
			enableitem(hwnd,IDC_SORT_TEXTCHARPRI);
			break;
		case WORDSORT_CMS:
			checkitem(hwnd,IDC_SORT_TEXTIGNH,TRUE);	// ignore slash
			checkitem(hwnd,IDC_SORT_TEXTIGNP,BST_INDETERMINATE);
			checkitem(hwnd,IDC_SORT_TEXTEVALN,TRUE);	// eval nums
			checkitem(hwnd,IDC_SORT_TEXTPARENPHRASE,TRUE);
			checkitem(hwnd,IDC_SORT_TEXTPAREN,FALSE);
			disableitem(hwnd,IDC_SORT_TEXTIGNH);
			disableitem(hwnd,IDC_SORT_TEXTIGNP);	
			disableitem(hwnd,IDC_SORT_TEXTEVALN);
			disableitem(hwnd,IDC_SORT_TEXTPARENPHRASE);
			enableitem(hwnd,IDC_SORT_TEXTPAREN);
			enableitem(hwnd,IDC_SORT_TEXTPREFIX);
			enableitem(hwnd, IDC_SORT_SUBSTITUTE);
			disableitem(hwnd,IDC_SORT_TEXTCHARPRI);
			break;
		case WORDSORT_ISO:
			checkitem(hwnd,IDC_SORT_TEXTIGNH,FALSE);
			checkitem(hwnd,IDC_SORT_TEXTIGNP,TRUE);		// ignore punct
			checkitem(hwnd,IDC_SORT_TEXTEVALN,TRUE);	// eval nums
			checkitem(hwnd,IDC_SORT_TEXTPARENPHRASE,TRUE);
			checkitem(hwnd,IDC_SORT_TEXTPAREN,FALSE);
			disableitem(hwnd,IDC_SORT_TEXTIGNH);
			disableitem(hwnd,IDC_SORT_TEXTIGNP);	
			disableitem(hwnd,IDC_SORT_TEXTEVALN);
			disableitem(hwnd,IDC_SORT_TEXTPARENPHRASE);
			enableitem(hwnd,IDC_SORT_TEXTPAREN);
			enableitem(hwnd,IDC_SORT_TEXTPREFIX);
			enableitem(hwnd, IDC_SORT_SUBSTITUTE);
			disableitem(hwnd,IDC_SORT_TEXTCHARPRI);
			break;
		case WORDSORT_SBL:
			checkitem(hwnd,IDC_SORT_TEXTIGNH,TRUE);	// ignore slash
			checkitem(hwnd,IDC_SORT_TEXTIGNP,BST_INDETERMINATE);
			checkitem(hwnd,IDC_SORT_TEXTEVALN,TRUE);	// eval nums
			checkitem(hwnd,IDC_SORT_TEXTPARENPHRASE,FALSE);
			checkitem(hwnd,IDC_SORT_TEXTPAREN,FALSE);
			disableitem(hwnd,IDC_SORT_TEXTIGNH);
			disableitem(hwnd,IDC_SORT_TEXTIGNP);	
			disableitem(hwnd,IDC_SORT_TEXTEVALN);
			enableitem(hwnd,IDC_SORT_TEXTPARENPHRASE);
			enableitem(hwnd,IDC_SORT_TEXTPAREN);
			enableitem(hwnd,IDC_SORT_TEXTPREFIX);
			disableitem(hwnd,IDC_SORT_TEXTCHARPRI);
			break;
	}
	builddraglist(hwdl,chprilist,charpri);
}
/**********************************************************************************/	
static INT_PTR CALLBACK sfieldproc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)

{
	struct comp * tcp;
	char fostring[STSTRING], * sptr;
	int count;
	HWND hwdl;

	tcp = getdata(hwnd);
	switch (msg)	{

		case WM_INITDIALOG:
			if (tcp = setdata(hwnd,(void *)((LPPROPSHEETPAGE)lParam)->lParam))	{	/* set data */
				for (sptr = fostring, count = 0; count < tcp->ip->maxfields; count++)	{	/* for all fields */
					strcpy(sptr,tcp->ip->field[count == tcp->ip->maxfields-1 ? PAGEINDEX : count].name);		/* add field to xstring */
					sptr += strlen(sptr++);
					if (tcp->sp->fieldorder[count] == PAGEINDEX)		/* set right temp index for page field */
						tcp->sp->fieldorder[count] = tcp->ip->maxfields-1;
				}
				*sptr = EOCS;
				hwdl = GetDlgItem(hwnd,IDC_SORT_FIELDLIST);
				MakeDragList(hwdl);
				builddraglist(hwdl,fostring,tcp->sp->fieldorder);
				checkitem(hwnd,IDC_SORT_FIELDSIGNLOWEST,tcp->sp->skiplast);
			}
			return (TRUE);
		case WM_NOTIFY:
			switch (((NMHDR *)lParam)->code)	{
				case PSN_KILLACTIVE:
					recoverdraglist(GetDlgItem(hwnd,IDC_SORT_FIELDLIST),tcp->sp->fieldorder);
					for (count = 0; count < tcp->ip->maxfields; count++)	{	/* for all fields */
						if (tcp->sp->fieldorder[count] == tcp->ip->maxfields-1)
							tcp->sp->fieldorder[count] = PAGEINDEX;
					}
					tcp->sp->skiplast = (char)isitemchecked(hwnd,IDC_SORT_FIELDSIGNLOWEST);
					SetWindowLongPtr(hwnd, DWLP_MSGRESULT,FALSE);
					return (TRUE);		/* check our results; SetwindowLong TRUE to hold page; FALSE to free */
				case PSN_HELP:
					dowindowhelp(s_sorthelpfields);
					return (TRUE);
			}
			break;
		case WM_COMMAND:
			switch (LOWORD(wParam))	{
				case IDC_SORT_FIELDLIST:
					if (HIWORD(wParam) == LBN_DBLCLK)	/* if double-click on list */
						switchdragitem((HWND)lParam);
					break;
			}
			break;
		case WM_HELP:
			return (dodialoghelp(hwnd,TEXT("_Sorting\\Sorting_entries.htm"),(HELPINFO *)lParam,wh_sortid));
		default:
			if (msg == WMM_DRAGMESSAGE)	{	/* if from drag list */
				SetWindowLongPtr(hwnd, DWLP_MSGRESULT,handledrag(hwnd,lParam, &tcp->dragitem));
				return (TRUE);
			}
	}
	return (FALSE);
}
/**********************************************************************************/	
static INT_PTR CALLBACK srefproc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)

{
	struct comp * tcp;
	HWND hwdl;
	static char refprilist[] = "Roman Numerals\0Arabic Numerals\0Letters\0Months\0\177";
	static char complist[] = "First\0Second\0Third\0Fourth\0Fifth\0Sixth\0Seventh\0Eighth\0Ninth\0Tenth\0\177";
	static char stylelist[] = "Plain\0Bold\0Italic\0Underline\0\177";

	tcp = getdata(hwnd);
	switch (msg)	{

		case WM_INITDIALOG:
			if (tcp = setdata(hwnd,(void *)((LPPROPSHEETPAGE)lParam)->lParam))	{	/* set data */
				hwdl = GetDlgItem(hwnd,IDC_SORT_REFPRI);
				MakeDragList(hwdl);
				builddraglist(hwdl,refprilist,tcp->sp->refpri);
				hwdl = GetDlgItem(hwnd,IDC_SORT_REFSEGORDER);
				MakeDragList(hwdl);
				builddraglist(hwdl,complist,tcp->sp->partorder);
				hwdl = GetDlgItem(hwnd,IDC_SORT_REFSTYLEPRI);
				MakeDragList(hwdl);
				builddraglist(hwdl,stylelist,tcp->sp->styleorder);
				checkitem(hwnd,IDC_SORT_REFSALL,tcp->sp->ordered);
				checkitem(hwnd,IDC_SORT_REFSASCEND,tcp->sp->ascendingorder);
				checkitem(hwnd, IDC_SORT_LEFT_RIGHT, tcp->sp->forceleftrightorder);
			}
			return (TRUE);
		case WM_NOTIFY:
			switch (((NMHDR *)lParam)->code)	{
				case PSN_KILLACTIVE:
					recoverdraglist(GetDlgItem(hwnd,IDC_SORT_REFPRI),tcp->sp->refpri);
					recoverdraglist(GetDlgItem(hwnd,IDC_SORT_REFSEGORDER),tcp->sp->partorder);
					recoverdraglist(GetDlgItem(hwnd,IDC_SORT_REFSTYLEPRI),tcp->sp->styleorder);
					tcp->sp->ordered = (char)isitemchecked(hwnd,IDC_SORT_REFSALL);
					tcp->sp->ascendingorder = (char)isitemchecked(hwnd,IDC_SORT_REFSASCEND);
					tcp->sp->forceleftrightorder = (char)isitemchecked(hwnd, IDC_SORT_LEFT_RIGHT);
					SetWindowLongPtr(hwnd, DWLP_MSGRESULT,FALSE);
					return (TRUE);		/* check our results; SetwindowLong TRUE to hold page; FALSE to free */
				case PSN_HELP:
					dowindowhelp(s_sorthelplocators);
					return (TRUE);
			}
			break;
		case WM_COMMAND:
			switch (LOWORD(wParam))	{
				case IDC_SORT_REFPRI:
				case IDC_SORT_REFSEGORDER:
					if (HIWORD(wParam) == LBN_DBLCLK)	/* if double-click on list */
						switchdragitem((HWND)lParam);
					break;
			}
			break;
		case WM_HELP:
			return (dodialoghelp(hwnd,TEXT("_Sorting\\Sorting_entries.htm"),(HELPINFO *)lParam,wh_sortid));
		default:
			if (msg == WMM_DRAGMESSAGE)	{	/* if from drag list */
				SetWindowLongPtr(hwnd, DWLP_MSGRESULT,(LONG_PTR)handledrag(hwnd,lParam, &tcp->dragitem));
				return (TRUE);
			}
	}
	return (FALSE);
}
/*******************************************************************************/
static void sortsub(HWND hwnd, char * sstring)	// sets up substitutions

{
	struct sub sb;

	memset(&sb,0,sizeof(struct sub));
	sb.substring = sstring;

	if (DialogBoxParam(g_hinst, MAKEINTRESOURCE(IDD_SORT_SUBSTITUTES), hwnd, subproc, (LPARAM)&sb)) {
		;
	}
}
/******************************************************************************/
static INT_PTR CALLBACK subproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)

{
	struct sub *sbp = getdata(hwnd);
	HWND lwind = GetDlgItem(hwnd, IDC_SORT_SUB_LIST);
	HWND tsource = GetDlgItem(hwnd, IDC_SORT_SUB_SOURCE);
	HWND trep = GetDlgItem(hwnd, IDC_SORT_SUB_REP);
	NMITEMACTIVATE * ia;
	NMLISTVIEW *lv;
	int activeitem;
	TCHAR buffer[1000];
	LVITEM lvI;

	switch (msg) {

	case WM_INITDIALOG:
		sbp = setdata(hwnd, (void *)lParam);
		LVCOLUMN lvc;
		int iCol;

		SetWindowLongPtr(lwind,GWL_EXSTYLE, LVS_REPORT|LVS_SINGLESEL| LVS_SHOWSELALWAYS);
//		SetWindowSubclass(tsource, EditControlProc, 0, 0);
//		SetWindowSubclass(trep, EditControlProc, 0, 0);
//		ListView_SetExtendedListViewStyleEx(lwind, LVS_EX_ONECLICKACTIVATE | LVS_EX_FULLROWSELECT, LVS_EX_ONECLICKACTIVATE | LVS_EX_FULLROWSELECT);
		ListView_SetExtendedListViewStyleEx(lwind, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);
		lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
		for (iCol = 0; iCol < 2; iCol++) {
			lvc.iSubItem = iCol;
			lvc.pszText = iCol == 0 ? TEXT("Source") : TEXT("Sort As");
			lvc.cx = 190;               // Width of column in pixels.
			lvc.fmt = LVCFMT_LEFT; 

			ListView_InsertColumn(lwind, iCol, &lvc);
		}
		memset(&lvI, 0, sizeof(lvI));
		lvI.mask = LVIF_TEXT;
		int iindex = 0;
		if (str_xcount(sbp->substring) >= 2) {
			for (char *sptr = sbp->substring; *sptr != EOCS; sptr += strlen(sptr) + 1) {
				lvI.iItem = iindex;
				lvI.pszText = toNative(sptr);
				lvI.iSubItem = 0;
				ListView_InsertItem(lwind, &lvI);
				sptr += strlen(sptr) + 1;
				ListView_SetItemText(lwind, iindex, 1, toNative(sptr));
				iindex++;

			}
		}
		else {	// creat empty first item
			lvI.pszText = TEXT("");
			ListView_InsertItem(lwind, &lvI);
			ListView_SetItemText(lwind, 0, 1, TEXT(""));
		}
		ListView_SetItemState(lwind, 0,LVIS_SELECTED, LVIS_SELECTED);
		Edit_SetCueBannerText(tsource,TEXT("...source text..."));
		Edit_SetCueBannerText(trep, TEXT("...sort text..."));
		centerwindow(hwnd, 1);
		return FALSE;
	case WM_NOTIFY:
		switch (((NMHDR *)lParam)->code) {
		case NM_SETFOCUS:
			if (!istextok(hwnd, sbp)) {		// if check is bad
				ListView_SetItemState(lwind, sbp->listitem, LVIS_SELECTED, LVIS_SELECTED);
				SetFocus(GetDlgItem(hwnd, sbp->controlID));
			}
			return TRUE;
		case NM_DBLCLK:
			ia = (NMITEMACTIVATE *)lParam;
			if (ia->iItem >= 0)		// if on valid item
				SetFocus(tsource);	// open for editing
			else		// if click not on valid item
				ListView_SetItemState(lwind, ListView_GetSelectionMark(lwind), LVIS_SELECTED, LVIS_SELECTED);
			return TRUE;
		case NM_CLICK:
			ia = (NMITEMACTIVATE *)lParam;
			if (ia->iItem < 0)		// if click not on valid item
				ListView_SetItemState(lwind, ListView_GetSelectionMark(lwind), LVIS_SELECTED, LVIS_SELECTED);
			return TRUE;
		case LVN_ITEMCHANGED:
			lv = (NMLISTVIEW *)lParam;
			if (lv->iItem >= 0 && !sbp->lock) {
				int item = lv->iItem;
				// all this handling to ensure no multiple selection, and single slected item is configured properly
				if (!(lv->uOldState&LVIS_SELECTED) && (lv->uNewState&LVIS_SELECTED)) {	// if new selection
					sbp->lock = TRUE;
					ListView_SetItemState(lwind, -1, 0, LVIS_SELECTED);		// clear all selections
					ListView_SetItemState(lwind, item, LVIS_SELECTED| LVIS_FOCUSED, LVIS_SELECTED| LVIS_FOCUSED);	// select this
					ListView_GetItemText(lwind, item, 0, buffer, sizeof(buffer) - 1);
					SetWindowText(tsource, buffer);
					ListView_GetItemText(lwind, item, 1, buffer, sizeof(buffer) - 1);
					SetWindowText(trep, buffer);
					ListView_SetSelectionMark(lwind, item);
					sbp->lock = FALSE;
				}
			}
			return TRUE;
		}
		break;
	case WM_COMMAND:
		if (HIWORD(wParam) == EN_KILLFOCUS) {
			WORD id = LOWORD(wParam);
			HWND tw = GetDlgItem(hwnd, id);
			GetWindowText(tw, buffer, sizeof(buffer) - 1);
			if (ListView_GetSelectionMark(lwind) < ListView_GetItemCount(lwind) - 1 || Edit_GetTextLength(tsource) || Edit_GetTextLength(trep)) {	// except for empty last row
				if (!recoverstrings(lwind, sbp->substring, buffer, id - IDC_SORT_SUB_SOURCE))
					sbp->controlID = id;
				else {
					ListView_SetItemText(lwind, ListView_GetSelectionMark(lwind), id - IDC_SORT_SUB_SOURCE, buffer);
					sbp->controlID = 0;
				}
			}
			return TRUE;
		}
		else if (HIWORD(wParam) == EN_SETFOCUS) {		// edit control receives focus; check state from last kill
			if (sbp->controlID) {	// if there's been an error 
				HWND tw = GetDlgItem(hwnd, sbp->controlID);
				sbp->listitem = ListView_GetSelectionMark(lwind);
				MessageBeep(MB_OK);
				SetFocus(tw);
				Edit_SetSel(tw, 0, -1);
			}
			return TRUE;
		}
		switch (LOWORD(wParam)) {
			case IDC_SORT_SUB_ADD:
				if (istextok(hwnd, sbp)) {
					if (ListView_GetSelectionMark(lwind) == ListView_GetItemCount(lwind) - 1 && !Edit_GetTextLength(tsource) && !Edit_GetTextLength(trep)) // if empty last row, don't add another
						activeitem = ListView_GetItemCount(lwind) - 1;
					else {	// need to add row
						memset(&lvI, 0, sizeof(lvI));
						lvI.mask = LVIF_TEXT;
						lvI.iItem = 10000;
						lvI.pszText = TEXT("");
						activeitem = ListView_InsertItem(lwind, &lvI);
						ListView_SetItemText(lwind, activeitem, 1, TEXT(""));
					}
					ListView_SetItemState(lwind, activeitem, LVIS_SELECTED, LVIS_SELECTED);
					SetFocus(tsource);
				}
				break;
			case IDC_SORT_SUB_REMOVE:
				if (istextok(hwnd, sbp)) {
					activeitem = ListView_GetSelectionMark(lwind);
					if (ListView_GetItemCount(lwind) > 1) 	// if not only item
						ListView_DeleteItem(lwind, activeitem);
					else {		// clear litem
						ListView_SetItemText(lwind, 0, 0, TEXT(""));
						ListView_SetItemText(lwind, 0, 1, TEXT(""));
						SetWindowText(tsource, TEXT(""));
						SetWindowText(trep, TEXT(""));
					}
					if (activeitem == ListView_GetItemCount(lwind)) 	// if deleted last item
						activeitem--;		// drop selection back one
					ListView_SetItemState(lwind, activeitem, LVIS_SELECTED, LVIS_SELECTED);
				}
				break;
			case IDOK:
				if (istextok(hwnd, sbp))
					recoverstrings(lwind, sbp->substring, NULL, 0);
				else
					return TRUE;
			case IDCANCEL:
				if ((HIWORD(wParam) == 1 || HIWORD(wParam) == 0)) {
					EndDialog(hwnd, LOWORD(wParam) == IDOK ? TRUE : FALSE);
					return TRUE;
				}
		}
		break;
	case WM_HELP:
		return (dodialoghelp(hwnd, TEXT("_Sorting\\Sorting_substitutions.htm"), (HELPINFO *)lParam, wh_sortsubid));
	default:
		;
	}
	return FALSE;
}
/*******************************************************************************/
BOOL istextok(HWND hwnd, struct sub * sbp) {

	HWND lwind = SetFocus(GetDlgItem(hwnd, IDC_SORT_SUB_LIST));	// set focus to list (will update any text error)
	if (sbp->controlID) {	// if there's been a text error
		HWND tw = GetDlgItem(hwnd, sbp->controlID);
		sbp->listitem = ListView_GetSelectionMark(lwind);
		MessageBeep(MB_OK);
		SetFocus(tw);
		Edit_SetSel(tw, 0, -1);
		return FALSE;
	}
	return TRUE;
}
/*******************************************************************************/
static BOOLEAN recoverstrings(HWND lwind, char * substrings, TCHAR * source, int column)
{
	LVITEM item = { 0 };
	TCHAR sbuffer[1000], rbuffer[1000];
	char * sptr = substrings;
	int active = -1;

	item.cchTextMax = sizeof(sbuffer);
	item.mask = LVFIF_TEXT;

	if (source) {	// if need to check edit
		active = ListView_GetSelectionMark(lwind);
		if (!column && (!*source || !u_isalnum(*source)))	// will fail on first pass
			return FALSE;
	}
	for (int iindex = 0; iindex < ListView_GetItemCount(lwind); iindex++) {
		item.iItem = iindex;
		item.iSubItem = 0;
		item.pszText = sbuffer;
		ListView_GetItem(lwind,&item);
		item.iSubItem = 1;
		item.pszText = rbuffer;
		ListView_GetItem(lwind, &item);
		if (*sbuffer || *rbuffer || iindex < ListView_GetItemCount(lwind) - 1) { // if not empty last row
			if (source) {	// if  checking
				if (column && (!*sbuffer || !nstrncmp(source, sbuffer, nstrlen(sbuffer)) && active != iindex)
					|| !column && active != iindex && (!nstrcmp(source, sbuffer) || !nstrncmp(rbuffer, source, nstrlen(source))))
					return FALSE;
			}
			else if (sbuffer[0]) {		// if not empty source
				int totlen = strlen(fromNative(sbuffer)) + strlen(fromNative(rbuffer)) + 2;
				if (sptr - substrings + totlen < STSTRING) {
					strcpy(sptr, fromNative(sbuffer));
					sptr += strlen(sptr) + 1;
					strcpy(sptr, fromNative(rbuffer));
					sptr += strlen(sptr) + 1;
				}
			}
		}
	}
	if (!source)
		*sptr = EOCS;
	return TRUE;
}
/*******************************************************************************/
void ss_compress(HWND hwnd)	/* compresses index */

{
	int flags = SQDELDEL|SQDELEMPTY| SQDELDUP, tsort;
	INDEX * FF;
	DWORD size = sizeof(flags);

	FF = WX(hwnd,owner);
//	reg_getkeyvalue(K_GENERAL, COMPRESSSETTINGS, &flags, &size);	// replace defaults if prior save
	if (FF->head.sortpars.fieldorder[0] == PAGEINDEX)		/* used on entry to control setup */
		flags |= SQPAGESORTFLAG;
	if (DialogBoxParam(g_hinst,MAKEINTRESOURCE(IDD_COMPRESS),g_hwframe,compproc,(LPARAM)&flags))	{
//		reg_setkeyvalue(K_GENERAL, COMPRESSSETTINGS, REG_BINARY, &flags, sizeof(flags));	// save
		if (sendwarning(WARN_SQUEEZE))	{
			tsort = FF->head.sortpars.ison;	/* get sort state */
			if (flags&(SQDELDUP|SQCOMBINE))
				FF->head.sortpars.ison = TRUE;	/* make sure its on if removing dup or consolidating */
			sort_squeeze(FF, flags);
			FF->head.sortpars.ison = (char)tsort;
			view_allrecords(FF->vwind);
			grp_buildmenu(FF);		/* rebuild group menu (groups will be invalidated) */
		}
	}
}
/******************************************************************************/
static INT_PTR CALLBACK compproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)

{
	int *flags = getdata(hwnd);

	switch (msg)	{
		case WM_INITDIALOG:
			setdata(hwnd, (void *)lParam);	// set data; 
			flags = getdata(hwnd);
			if (*flags&SQPAGESORTFLAG)	{	// if page sort
				disableitem(hwnd,IDC_COMPRESS_CONSOLIDATE);
				disableitem(hwnd,IDC_COMPRESS_REMDUP);
			}
			else {
				checkitem(hwnd, IDC_COMPRESS_CONSOLIDATE, *flags&SQCOMBINE);
				checkitem(hwnd, IDC_COMPRESS_REMDUP, *flags&SQDELDUP);
			}
			checkitem(hwnd,IDC_COMPRESS_REMDEL,*flags&SQDELDEL);
			checkitem(hwnd,IDC_COMPRESS_REMEMPTY,*flags&SQDELEMPTY);
			if (isitemchecked(hwnd, IDC_COMPRESS_CONSOLIDATE))
				checkitem(hwnd, IDC_COMPRESS_REMEMPTY, *flags&SQIGNORELABEL);
			centerwindow(hwnd,1);
			return FALSE;
		case WM_COMMAND:
			switch (LOWORD(wParam))	{
				case IDOK:
					*flags = 0;
					if (isitemchecked(hwnd,IDC_COMPRESS_REMDEL))
						*flags |= SQDELDEL;		/* want to remove deleted */
					if (isitemchecked(hwnd,IDC_COMPRESS_REMEMPTY))
						*flags |= SQDELEMPTY;	/* also want to remove empty */
					if (isitemchecked(hwnd,IDC_COMPRESS_REMDUP))
						*flags |= SQDELDUP;		/* want to remove duplicates */
					if (isitemchecked(hwnd,IDC_COMPRESS_REMGEN))
						*flags |= SQDELGEN;		/* want to remove generated */
					if (isitemchecked(hwnd,IDC_COMPRESS_CONSOLIDATE))	{
						*flags |= SQCOMBINE;	/* want to combine & remove */
						if (isitemchecked(hwnd,IDC_COMPRESS_IGNORELABELS))
							*flags |= SQIGNORELABEL;	// want to ignore label diffs
					}
				case IDCANCEL:
					EndDialog(hwnd,LOWORD(wParam) == IDOK ? TRUE : FALSE);
					return TRUE;
			}
			break;
		case WM_HELP:
			return (dodialoghelp(hwnd,TEXT("base\\Tools_compress.htm"),(HELPINFO *)lParam,wh_compid));
		default:
			if (isitemchecked(hwnd,IDC_COMPRESS_CONSOLIDATE))
				enableitem(hwnd,IDC_COMPRESS_IGNORELABELS);
			else {
				checkitem(hwnd,IDC_COMPRESS_IGNORELABELS,FALSE);
				disableitem(hwnd,IDC_COMPRESS_IGNORELABELS);
			}
	}
	return FALSE;
}
/*******************************************************************************/
void ss_expand(HWND hwnd)	/* expands records for 1 ref in each */

{
	INDEX * FF;
	
	if (sendwarning(WARN_EXPAND))	{
		FF = WX(hwnd,owner);
		sort_squeeze(FF, SQSINGLE);
		view_allrecords(FF->vwind);
		grp_buildmenu(FF);		/* rebuild group menu (groups will be invalidated) */
	}
}
