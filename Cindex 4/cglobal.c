#include "stdafx.h"
#include "viewset.h"

#ifdef PUBLISH
ADMIN g_admin = {
	0,			/* id */
	TEXT(""),			/* password */
	FALSE,		/* no password */
	FALSE,		/* no validate */
	TRUE,		/* permit write access */
	TRUE,		/* readonly access permitted to open files */
	{
		{0,1,1, 1,1,1},	/* index: no switch, can change folder, can set default */
		{0,1,1, 1,1,1},	/* style: no switch, can change folder, can set default */
		{0,1,1, 1,1,1},	/* abbrev: no switch, can change folder, can set default */
		{1,0,0, 0,0,1},	/* tag: always switch, can change folder, can set default */
		{1,0,0, 0,0,1}	/* udic: always switch, can change folder, can set default */
	},
	TEXT(""),			/* path to user file */
	FALSE		/* don't allow multiple instances */
};

TCHAR g_password[FSSTRING];	/* user-entered password */
TCHAR *g_prefname = TEXT("cindex4_publisher.ini");
#else
TCHAR *g_prefname = TEXT("cindex4.ini");
#endif // PUBLISH

//OSVERSIONINFO g_version;
TCHAR g_cinversion[20];
TCHAR g_indexclass[] = TEXT("index");	/* name of index window class */
TCHAR g_textclass[] = TEXT("ctext");	/* name of text window class */
TCHAR g_modclass[] = TEXT("mtext");	/* name of modify window class */
TCHAR g_containerclass[] = TEXT("container");	// name of container window class
TCHAR g_rcontainerclass[] = TEXT("rcontainer");	// name of record container window class

DWORD g_modclassatom;		// class id for modify window

TCHAR * g_basefont = TEXT("Arial");	/* always the base font */

int g_keydelay;					/* user's preferred key delay before repeat */

TCHAR *CF_CINREC = TEXT("Cindex Record");
TCHAR *CF_HTML = TEXT("HTML Format");
UINT g_recref, g_rtfref, g_htmlref;	/* ids of registered clipboard formats */

HINSTANCE g_hinst;
HMENU g_popmenu;	// popup menu set
int g_slpix;		/* screen logical pixels */

HBRUSH g_highlightbrush;	// brush for text highlight;
BOOL g_shutdown;	/* TRUE when shutting down */

HWND g_hwframe, g_hwclient, g_hwstatus, g_hwtoolbar, g_mdlg, g_hwprog;
HWND g_findw, g_repw, g_abbw, g_clipw, g_spellw, g_keyw, g_toolw;		/* global windows */
HWND g_hwfcb, g_hwscb, g_hwscbed;	/* toolbar combo boxes */

HWND g_logw;

HCURSOR g_arrow, g_rightarrow, g_waitcurs, g_downarrow;

UINT g_revertstate;		// max/min state for reverting active index
RECT g_revertplacement;	// size pos for reverted window

time_t g_comstarttime;

INDEX * g_indexbase;	/* base of linked list */

char g_nullstr[] = "";		/* a null string */
char g_nullrec[] = {0,0,'\177'};	/* null record */
HBITMAP g_labelmaps[8];		// label bitmap handles

struct prefs g_prefs = {		/* preferences info */
	80003,	/* key */
	{		/* hidden */
		1,	// verify min count
		7,	// page ref max count
		0,	/* show windows for inactive indexes */
		"",	/* user ID */
		0	/* show abbrevs by name */
	},
	{		/* gen prefs */
		0,		/* open dialog on startup */
		0,		/* use fractional character spacing */
		FALSE,	/* maximize CINDEX */
		FALSE,	/* maximize indexes */
		TRUE,	/* show label in formatted view */
		TRUE,	// smart flip
		FALSE,	// auto range
		FALSE,	/* require user id */
		TRUE,	/* propagate */
		600,	/* flush interval (sec) */
		1,		/* empty page warning */
		1,		/* missing crossref warning */
		1,		/* template mismatch alarm */
		FALSE,	/* carry refs */
		TRUE,	/* switch to draft mode for adding/editing */
		FALSE,	// don't open new window for editing
		FALSE,	/* no track new records */
		FALSE,	/* no return to entry point */
		1,		/* ask about saving changes to records */
		{		/* flag colors */
			0,			// black
			0x000000FF,	// red
			0x004080FF,	// orange
			0x00008000,	// green
			0x00c08000,	//
			0x00FF0000,	//
			0x00FF0080,	//
			0x00400080,	//
		},
		{		/* custom color array */
			{0}
		},
		{		/* fontmap array */
			{"Arial","Arial",0}
		},
		MAXRECENT,		/* max number of recent files */
		FALSE,	/* autoextend */
		FALSE,	/* ignorecase in autoextend */
		TRUE,	/* remove duplicate spaces */
		FALSE,	/* no track source entry */
		0,		/* use styles to define format indents */
		0,		// use main window text size
		TRUE,	// embed sort information
		TRUE,	// auto update
		0,		// utf-8 encoding of plain text
		PASTEMODE_STYLEONLY		// paste styles but not fonts
	},
	{			/* language preferences for spell-checker */
		TRUE,		/* always suggest */
		FALSE,		/* don't ignore words in caps */
		FALSE,		/* don't ignore alnums */
	},
	{		/* index structure pars */
		100,	/* record size */
		2,		/* minfields */
		5,		/* max fields */
		{
			{"Main", 0, 0, ""},
			{"Sub 1", 0, 0, ""},
			{"Sub 2", 0, 0, ""},
			{"Sub 3", 0, 0, ""},
			{"Sub 4", 0, 0, ""},
			{"Sub 5", 0, 0, ""},
			{"Sub 6", 0, 0, ""},
			{"Sub 7", 0, 0, ""},
			{"Sub 8", 0, 0, ""},
			{"Sub 9", 0, 0, ""},
			{"Sub 10", 0, 0, ""},
			{"Sub 11", 0, 0, ""},
			{"Sub 12", 0, 0, ""},
			{"Sub 13", 0, 0, ""},
			{"Sub 14", 0, 0, ""},
			{"Page", 0, 0, ""}
		},
	},
	{		/* sort pars */
		0,		/* raw sort */
		SORTVERSION,	// sort version
		{"en"},		// language
		{0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,-1},	/* field order */
		{0,1,2,-1},		/* character priority */
		FALSE,	/* ignore punct in letter sort */
		FALSE,	/* ignore slash */
		FALSE,	/* eval num */
		FALSE,	// ignore paren phrase
		FALSE,	// ignore paren ending
		{"a against and as at before between by during for from in into of on the to under versus vs. with within"},
		{"\177"},	// substitutes (empty compound string)
		FALSE,	/* skip last heading field */
		TRUE,	/* ordered refs */
		TRUE,	/* ascending order refs */
		TRUE,	/* sort on */
		TRUE,	// language script first -- default is en (SORTVERSION 6 and higher)
		{1,2,-1,-1,-1},		/* reference priority (arabic & letters only) */
		{0},		/* priority table for reference types */
		{0,1,2,3,4,5,6,7,8,9,-1},	/* component comparison order */
		{0,1,2,3,-1},		// style priority
		{0},		// ref style precedence order
		0,		// symcode (unused from SORTVERSION 6)
		0,		// numcode (unused from SORTVERSION 6)
		{"en"}	// localID (SORTVERSION 6 and higher)
	},
	{		/* ref pars */
		{"See also under"},		/* cross refs begin */
		{"individual specific"},	/* excluded refs */
		{""},						/* max value of page ref */
		';',		/* cross ref separator */
		',',		/* page ref separator */
		'-',		/* page ref connector */
		TRUE,		// recognize crossrefs in locator field only
		0			/* max span of range */
	},
	{		/* private prefs */
		VM_DRAFT,	/* display mode */
		TRUE,		/* line wrap */
		FALSE,		/* show numbers */
		FALSE,		/* hide deleted records */
		ALLFIELDS,	/* hide fields below this level */
		12,			/* default font size */
		U_INCH,		/* unit in which dimensions expressed */
		FALSE,
//		{200,50,300,372},		/* default record window size/posn */
	},
	{		/* format preferences */
		sizeof(FORMATPARAMS),
		FORMVERSION,
		{		/* page format */
			{		/* margins & columns */
				72,		/* top */
				72,		/* bottom */
				72,		/* left */
				72,		/* right */
				1,		/* # columns */
				18,		/* gutter */
				FALSE,	/* reflection flag */
				1,		/* repeat broken heading after page break */
#ifdef PUBLISH
				"(Continued)",		/* continued text */
#else
				" (Continued)",		/* continued text */
#endif //PUBLISH
				{FX_ITAL,0},
				0,		/* heading level for continuation repeat */
			},
			{"","%","",{FX_BOLD},"",0},	/* left header */
			{"#","","",{FX_BOLD},"",0},	/* left footer */
			{"","%","",{FX_BOLD},"",0},	/* right header */
			{"#","","",{FX_BOLD},"",0},	/* right footer */
			0,				/* line spacing (single) */
			1,				/* number of first page  */
			16,				/* line height (points) */
			0,				/* extra entry space */
			1,				/* extra group space */
			U_INCH,			/* line spacing unit (inch) */
			TRUE,			/* autospacing */
			DF_LONG,		/* date format */
			FALSE,			/* don't show time */
			{
				DMORIENT_PORTRAIT,	/* portrait orientation */
				DMPAPER_LETTER,		/* letter paper */
				0,			/* no override length */
				0,			/* no override width */
				612,		/* actual width (points) */
				792			/* actual height (points) */
			}
		},
		{		/* overall heading layout */
			0,			/* run-on level */
			0,			/* collapse level */
			0,			/* style modifier */
			1,			/* indentation type */
			TRUE,		/* adjust punctuation */
			TRUE,		/* adjust style codes around punct */
			0,			/* use em spacing when fixed */
			0,			/* use em spacing when auto */
			(float)1.,		/* ems for auto lead */
			(float)2.5,		/* ems for auto runover */
			{		/* grouping of entries */
				3,		/* method */
				"",		/* use default font */
				{0},	/* style */
				0,		/* size */
				"%",		/* format string */
				"",		// all numbers
				"",		// all symbols
				""	// numbers and symbols
			},
			{		/* cross-ref format */
				{
					{			/* refs from main head */
						". ",		/* see also lead */
						"",			/* end */
						". ",		/* see lead */
						"",			/* end */
					},
					{			/* refs from subhead */
						" (",		/* see also lead */
						")",			/* end */
						" (",		/* see lead */
						")",			/* end */
					}
				},
				{2},	/* lead style (italic) */
				{0,0},	/* body style */
				0,		/* subhead 'see also' position (follows heading) */
				0,		/* main head 'see also' position (follows heading) */
				TRUE,	/* sort cross-refs */
				FALSE, 	/* suppress all cross refs*/
				0,		/* subhead 'see' position */
				0,		/* main head 'see' position */
			},
			{		/* locator format */
				TRUE,		/* sort refs */
				FALSE,		/* right justify */
				FALSE,		/* suppress all references*/
				FALSE,		/* supress repeated parts of multiple parts */
				", ",		/* lead to single */
				", ",		/* lead to multiple */
				"",			/* trailing text */
				{'\xe2','\x80','\x93'},		/* range connector (default en dash) */
				0,			/* conflation threshold */
				0,			/* abbreviation rule */
				"",			/* suppression sequence */
				", ",		/* concatenation characters */
				{0},   		/* style sequence */
				0,			/* leader type */
				TRUE,		// hide duplicates
				0, 			/* !! spare */
			},
			{		/* field layout/typography */
				{
					"",		/* use default font */
					0,		/* font size */
					{0},	/* style */
					(float)0.,		/* lead indent for explicit indent */
					(float)1.5,	/* runover text for explicit indent */
					"",		/* trailing text */
				},
				{
					"",		/* use default font */
					0,		/* font size */
					{0},	/* style */
					(float)1.,		/* lead indent for explicit indent */
					(float)2.5,	/* runover text for explicit indent */
					"",		/* trailing text */
				},
				{
					"",		/* use default font */
					0,		/* font size */
					{0},	/* style */
					(float)2.,		/* lead indent for explicit indent */
					(float)3.5,	/* runover text for explicit indent */
					"",		/* trailing text */
				},
				{
					"",		/* use default font */
					0,		/* font size */
					{0},	/* style */
					(float)3.,		/* lead indent for explicit indent */
					(float)4.5,	/* runover text for explicit indent */
					"",		/* trailing text */
				},
				{
					"",		/* use default font */
					0,		/* font size */
					{0},	/* style */
					(float)4.,		/* lead indent for explicit indent */
					(float)5.5,	/* runover text for explicit indent */
					"",		/* trailing text */
				},
				{
					"",		/* use default font */
					0,		/* font size */
					{0},	/* style */
					(float)5.,		/* lead indent for explicit indent */
					(float)6.5,	/* runover text for explicit indent */
					"",		/* trailing text */
				},
				{
					"",		/* use default font */
					0,		/* font size */
					{0},	/* style */
					(float)6.,		/* lead indent for explicit indent */
					(float)7.5,	/* runover text for explicit indent */
					"",		/* trailing text */
				},
				{
					"",		/* use default font */
					0,		/* font size */
					{0},	/* style */
					(float)7.,		/* lead indent for explicit indent */
					(float)8.5,	/* runover text for explicit indent */
					"",		/* trailing text */
				},
				{
					"",		/* use default font */
					0,		/* font size */
					{0},	/* style */
					(float)8.,		/* lead indent for explicit indent */
					(float)9.5,	/* runover text for explicit indent */
					"",		/* trailing text */
				},
				{
					"",		/* use default font */
					0,		/* font size */
					{0},	/* style */
					(float)9.,		/* lead indent for explicit indent */
					(float)10.5,	/* runover text for explicit indent */
					"",		/* trailing text */
				},
				{
					"",		/* use default font */
					0,		/* font size */
					{0},	/* style */
					(float)10.,		/* lead indent for explicit indent */
					(float)11.5,	/* runover text for explicit indent */
					"",		/* trailing text */
				},
				{
					"",		/* use default font */
					0,		/* font size */
					{0},	/* style */
					(float)11.,		/* lead indent for explicit indent */
					(float)12.5,	/* runover text for explicit indent */
					"",		/* trailing text */
				},
				{
					"",		/* use default font */
					0,		/* font size */
					{0},	/* style */
					(float)12.,		/* lead indent for explicit indent */
					(float)13.5,	/* runover text for explicit indent */
					"",		/* trailing text */
				},
				{
					"",		/* use default font */
					0,		/* font size */
					{0},	/* style */
					(float)13.,		/* lead indent for explicit indent */
					(float)14.5,	/* runover text for explicit indent */
					"",		/* trailing text */
				},
				{
					"",		/* use default font */
					0,		/* font size */
					{0},	/* style */
					(float)14.,		/* lead indent for explicit indent */
					(float)15.5,	/* runover text for explicit indent */
					"",		/* trailing text */
				}
			}
		}
	},
	{		/* style strings */
		"\2vs.\0\2versus\0\177"
	},
	{"a after against ~and as at before between by during for from in into of on over the to under ~versus ~vs. with within"},	// flip words
	{VK_F1|(HOTKEYF_SHIFT<<8),VK_F2|(HOTKEYF_SHIFT<<8),VK_F3|(HOTKEYF_SHIFT<<8),VK_F4|(HOTKEYF_SHIFT<<8),VK_F5|(HOTKEYF_SHIFT<<8),
	VK_F6|(HOTKEYF_SHIFT<<8),VK_F7|(HOTKEYF_SHIFT<<8),VK_F8|(HOTKEYF_SHIFT<<8),VK_F9|(HOTKEYF_SHIFT<<8),VK_F10|(HOTKEYF_SHIFT<<8)},		/* hot keys */
	{
		"See \0See also \0\0\0\0\0\0\0\0\0\177"
	}
};
