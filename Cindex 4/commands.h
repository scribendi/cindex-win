#pragma once
typedef struct comgroup {
	int comid;		/* command base id */
	int lowlimit;	/* command id lower limit */
	int highlimit;	/* command id upper limit */
	long (*flst) (HWND hwnd,int comid,HWND chandle,UINT notify);		/* ptr to cmmnd function */
	short mkeys;	/* master enable/disable keys for item */
} COMGROUP;

/* command menu management flags */

enum {	
	ENEVER = 0,		/* command never touched */
	WPASS = 1,		/* command passed directly to window */
	EIDLE = 2,		/* command enable in idle */
	EVIEW = 4,		/* command enabled for view window */
	ERVIEW = 8,		/* enabled when record window open */
	ESCONT = 16,	/* command enabled for continuous selection */
	EVSEL = 32,		/* command enabled for selection in any view window */
	EVFSEL = 64,	/* command enabled for selection in full view window */
	EFIND = 128,	/* enabled with find window */
	EREP = 256,		/* enabled with replace window */
	ESPELL = 512,	/* enabled when spell window open */
	EABBREV = 1024,	/* enabled when abbrev window open */
	ETEXT = 2048,	/* enabled when child text window open */
	ECLIP = 4096,	/* enabled when clip window open */
	ERSVIEW = 8192	/* enabled in main view when record window open */
};
/* modes for com_setenable */

enum {
	ONLY = 1,		/* turns these on or off by flag */
	XONLY = 2,		/* turns these on or off by flag; others to opposite */
	ALLBUT = 4	/* turns all but these on or off by flag */
};

enum {	/* window messages */
	WMM_SETGROUP = WM_USER+1,
	WMM_UPDATESTATUS,
	WMM_GETCURREC,

	WMM_PAGENUM			/* sends page number to print dialog */
};

struct recentlist {
	int order;
	TCHAR path[MAX_PATH];
};

extern struct recentlist c_recent[MAXRECENT];

/* commands.c */
long mc_new(HWND hwnd,int comid,HWND chandle,UINT notify);	/* sets up new index */
long mc_open(HWND hwnd,int comid,HWND chandle,UINT notify);	/* opens document */
long mc_import(HWND hwnd,int comid,HWND chandle,UINT notify);	/* imports records */

long mc_setgroup(HWND hwnd,int comid,HWND chandle,UINT notify);	/* finds group to use */
long mc_summary(HWND hwnd,int comid,HWND chandle,UINT notify);	/* shows summary */
long mc_unform(HWND hwnd,int comid,HWND chandle,UINT notify);	/* shows unformatted */
long mc_hidebelow(HWND hwnd,int comid,HWND chandle,UINT notify);	/* hides fields below specified */
long mc_shownum(HWND hwnd,int comid,HWND chandle,UINT notify);	/* shows numbers on records */
long mc_wraplines(HWND hwnd,int comid,HWND chandle,UINT notify);	/* wraps lines */

long mc_closeall(HWND hwnd,int comid,HWND chandle,UINT notify);

long mc_about(HWND hwnd,int comid,HWND chandle,UINT notify);		/* about CINDEX */


COMGROUP * com_findcommand(int comid);	/* returns structure ptr for command */
void com_check(int comid, int onoff);	/* checks/unchecks menu items */
void com_set(int comid, int onoff);	/* disables/enables menu items */
void com_setname(int comid, TCHAR *name);	/* sets name of menu item */
void com_setenable(int selector, int mode, int onoff);	/* disables menus items by selection keys */
short com_getdates(short scope, char * first, char * last, time_c * low, time_c * high);		/* finds & parses date range */
short com_getrecrange(INDEX * FF, short scope, char *lptr, char * hptr, RECN * first, RECN * last);		/* finds start and stop recs */
RECN com_findrecord(INDEX * FF, char *arg, short lastmatchflag);		/* finds record by lead or number */
void com_setbyindexstate(HWND wptr,HMENU hMenu, UINT item);		/* sets menu items by index context */
void com_setdefaultbuttons(int simple);	/* sets default buttons */
void com_tbsaverestore(HWND hwtb,HWND parent, BOOL sflag, TCHAR * set);	/* saves or retrieves tb */
void com_settextmenus(INDEX * FF,int font,int size);	/* sets default menus */
void com_pushrecent(TCHAR * newpath);		/* manages recent file list */
void com_setrecentfiles(HMENU mh);		/* sets recent file items in file menu */
TCHAR *com_findrecent(int recentorder);		/* returns path for file in order */
