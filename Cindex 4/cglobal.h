#pragma once

enum {
	PASTEMODE_ALL,
	PASTEMODE_STYLEONLY,
	PASTEMODE_PLAIN
};

#ifdef PUBLISH
struct fpermiss	{		/* admin folder permissions */
	char opendef;
	char changefolder;
	char setdef;
	char opendefok;		/* flags control what can be set */
	char changefolderok;
	char setdefok;
	long spare[4];
};

typedef struct {
	int id;					/* unused id value */
	TCHAR psswd[FSSTRING];	/* password */
	char requireid;			/* user must enter ID */
	char validate;			/* id must be validated */
	char permitwrite;		/* permits write access to files */
	char readaccess;		/* TRUE permits read-only access to open files */
	struct fpermiss access[8];	/* file access keys */
	TCHAR userpath[MAX_PATH];	/* path to user file */
	char allowmultiple;		/* TRUE to allow multiple instances */
	char sp1;
	char sp2;
	char sp3;
	long spare[3];
} ADMIN;
#endif // PUBLISH

struct genpref {		/* general preferences */
	char openflag;		/* open last index on startup */
	char labelsetsdate;	// labeling record stamps date
	char maxcin;		/* maximize cindex on startup */
	char maxdoc;		/* maximize indexes on opening */
	char showlabel;		/* show labeled records in formatted view */
	char smartflip;		/* smart flip */
	char autorange;		/* autorange */
	char setid;			/* want user id */
	char propagate;		/* propagate edit changes */
	short saveinterval;	/* preferred save interval */
	char pagealarm;		/* alarm level for page refs */
	char crossalarm;	/* alarm level for cross refs */
	char templatealarm;	/* alarm level for template mismatch */
	char carryrefs;		/* carry refs from one record to the next */
	char switchview;	/* TRUE if to switch to draft mode for editing */
	char newwindow;		/* TRUE if use new window for editing */
	char track;			/* TRUE if want to track position of new entries */
	char vreturn;		/* TRUE if want to return display to original record */
	char saverule;		/* saving behavior on record window */
	COLORREF flagcolor[FLAGLIMIT];	/* flag text colors */
	COLORREF custcolors[16];	/* our set of custom colors */
	FONTMAP fm[FONTLIMIT];	/* mapping of local IDs to font names (0 is default) */
	short recentlimit;	/* limit on # of recent files */
	char autoextend;	/* autoextends fields while typing */
	char autoignorecase;	/* ignores case in autoextend matching */
	char remspaces;		/* remove duplicate spaces */
	char tracksource;	/* tracks source entry */
	char indentdef;		/* type of indent for formatted export */
	short recordtextsize;
	char embedsort;		// TRUE if want to embed sort info
	char autoupdate;	//TRUE if auto updating
	char nativetextencoding;	// TRUE if native encoding
	char pastemode;		// mode for pasting styles/fonts
	char spare[7];		/* !!spare */
};

struct hiddenpref {		/* hidden preferences (always become defaults) */
	unsigned short crossminmatch;	// verify min count
	unsigned short pagemaxcount;	// page max count
	short hideinactive;	/* show/hide windows for inactive indexes */
	char user[6];		/* user initials */
	char absort;		/* abbreviation sort type */
	char spare1;
	long spare[7];		/* !!spare */
};

struct langprefs {	/* aggregate of language preferences */
	short suggestflag;	/* auto-suggest */
	short ignallcaps;		/* check words in caps */
	short ignalnums;		/* check alnums */
	long spare[8];			/* !!spare */
};

struct prefs {			/* preferences struct */
	long key;			/* key number identifies file generation */
	struct hiddenpref hidden;	/* hidden preferences */
	struct genpref gen;	/* general preferences */
	struct langprefs langpars;	/* spell checker language settings */
	INDEXPARAMS indexpars;	/* index structure prefs */
	SORTPARAMS sortpars;	/* sort preferences */
	REFPARAMS refpars;	/* ref preferences */
	PRIVATEPARAMS privpars;	/* private preferences */
	FORMATPARAMS formpars;	/* format preferences */
	char stylestrings[STYLESTRINGLEN];		/* strings for auto style */
	char flipwords[STSTRING];		/* strings for auto style */
	short hotkeys[MAXKEYS];		/* hot key codes */
	char keystrings[STSTRING];	/* keys strings */
};

enum {		/* frame window child window ids */
	CLIENT_ID = 100,
	IDC_STATUS,
	IDC_TOOLBAR,
	IDC_PROGBAR,
	IDC_FONTCOMBO,
	IDC_SIZECOMBO
};

#ifdef PUBLISH
extern ADMIN g_admin;
extern TCHAR g_password[FSSTRING];	/* user-entered password */
#endif // PUBLISH

//extern OSVERSIONINFO g_version;
extern TCHAR g_cinversion[];

extern TCHAR g_indexclass[];	/* name of index window class */
extern TCHAR g_textclass[];	/* name of text window class */
extern TCHAR g_modclass[];	/* name of modify window class */
extern TCHAR g_containerclass[];	// name of container window class
extern TCHAR g_rcontainerclass[];// name of record container window class

extern DWORD g_modclassatom;		// class id for modify window

extern TCHAR * g_basefont;;	/* always the base font */

extern int g_keydelay;				/* user's preferred key delay before repeat */

extern TCHAR *CF_CINREC;
extern TCHAR *CF_HTML;
extern UINT g_recref, g_rtfref, g_htmlref;	/* ids of registered clipboard formats */

extern HINSTANCE g_hinst;	/* application instance */
extern HMENU g_popmenu;		// popup menu set

extern int g_slpix;		/* screen logical pixels */

extern HBRUSH g_highlightbrush;	// brush for text highlight;

extern BOOL g_shutdown;	/* TRUE when shutting down */

extern HWND g_hwframe, g_hwclient, g_hwstatus, g_hwtoolbar, g_mdlg, g_hwprog;
extern HWND g_findw, g_repw, g_abbw, g_clipw, g_spellw, g_keyw, g_toolw;		/* global windows */
extern HWND g_hwfcb, g_hwscb, g_hwscbed;	/* toolbar combo boxes */

extern HWND g_logw;

extern HCURSOR g_arrow, g_rightarrow, g_waitcurs,g_downarrow;

extern UINT g_revertstate;		// max/min state for reverting active index
extern RECT g_revertplacement;	// size pos for reverted window

extern time_t g_comstarttime;

extern INDEX * g_indexbase;	/* base of linked list */

extern char g_nullstr[];	/* a null string */
extern char g_nullrec[];	/* null record */
extern HBITMAP g_labelmaps[];		// label bitmap handles

extern struct prefs g_prefs;		/* preferences info */
extern TCHAR * g_prefname;		/* preferences */
