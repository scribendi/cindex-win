#pragma once
enum {				/* status bar segment ids */
	STATSEG_RECTOT = 0,
	STATSEG_NEW,
	STATSEG_SORT,
	STATSEG_SEARCH,
	STATSEG_MACRO
};

struct ttmatch {		/* for managing tooltip strings */
	int id;
	TCHAR * string;
};

#define getmem(COUNT) (calloc(COUNT,1))

#define CBSTRING_MAX 16	/* max # strings in dynamic combo box */

#define getMDIparam(cs) 	((void *)(((LPMDICREATESTRUCT)(cs->lpCreateParams))->lParam))

#define freemem(A) free(A)
#define SEC_PER_DAY ((unsigned long)86400)
#define dostomac_time(A) ((A)+2208988800L)	/* 1970 base time from 1900 based time (Metrowerks standard) */
#define mactodos_time(A) ((A)-2208988800L)	/* 1900 based time from 1970 based time */

#define enableitem(hwnd,item) EnableWindow((GetDlgItem(hwnd,item)),TRUE)
#define disableitem(hwnd,item) EnableWindow((GetDlgItem(hwnd,item)),FALSE)
#define setitemenable(hwnd,item,state) EnableWindow((GetDlgItem(hwnd,item)),state)
#define showitem(hwnd,item) ShowWindow((GetDlgItem(hwnd,item)),SW_SHOW)
#define hideitem(hwnd,item) ShowWindow((GetDlgItem(hwnd,item)),SW_HIDE)
#define checkitem(hwnd,item,state) SendMessage(GetDlgItem(hwnd,item),BM_SETCHECK,state,0)
#define checkcontrol(hwnd,state) SendMessage(hwnd,BM_SETCHECK,state,0)
#define isitemchecked(hwnd,item) SendMessage(GetDlgItem(hwnd,item),BM_GETCHECK,0,0)
#define iscontrolchecked(hwnd) SendMessage(((HWND)hwnd),BM_GETCHECK,0,0)
#define isitemenabled(hwnd,item) IsWindowEnabled(GetDlgItem(hwnd,item))
// #define bitpos(A) ((A) ? 1 << (A-1) : 0)	// returns bit set at position for value of A

#define TWIPS_PER_POINT 20	/* number of twips in a point */

__inline LONG_PTR getmdiactive(HWND hwnd)			/* gets active flag of mdi window */
{
	return GetWindowLongPtr(hwnd,0);
}

__inline void setmdiactive(HWND hwnd,int state)	/* sets active flag for mdi window */	
{
	SetWindowLongPtr(hwnd,0,(LONG_PTR)state);
}
__inline void * getdata(HWND hwnd)
{
	return (void *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
}

__inline INDEX * getowner(HWND hwnd)
{
	return ((WFLIST *)GetWindowLongPtr(hwnd, GWLP_USERDATA))->owner;
}

extern UINT WMM_DRAGMESSAGE;		/* registered drag list message */

extern char codecharset[];

NONCLIENTMETRICS * getconfigdpi(HWND hwnd);
int getsysmetrics(HWND hwnd, int index);
int scaleForDpi(HWND hwnd, int input);
int dpiforwindow(HWND hwnd);
int scaleForDpi(HWND hwnd, int input);
void scaleRectForDpi(HWND hwnd, RECT * rptr);	// scales standard to display on high res
void scaleRectFromDpi(HWND hwnd, RECT * rptr);	// scales high res to display on standard
void layoutForDpi(HWND hWnd, int x, int y, int w, int h);
void adjustWindowRect(HWND hwnd, LPRECT prect, DWORD style, BOOL menu, DWORD exstyle);
void configurestatusbar(HWND hwnd, int count, int * widths);	// sets up scaled segment sizes
void fixsmallcaps(TCHAR * buffer);	// cleans up small caps codes
void appendString(HWND hw, char *string);	// appends string to edit text
char * fromNative(TCHAR * string);	// converts to utf-8 from native
TCHAR * toNative(char * string);	// converts to native from utf-8
UINT getDItemText(HWND hwnd, int item, char * dest, int length);
BOOL setDItemText(HWND hwnd, int item, char * source);
BOOL iscodechar(short cc);	// tests if character is code char
int isfontcodeptr(char *cc);	// tests if code represents font
//unichar symbolcharfromroman(unsigned char schar);	// return unichar value of symbol font character
//char charfromsymbol(unichar uc);		// returns symbol font char for unicode
char * u8_back1(char * ptr);	// moves back one code point
char * u8_forward1(char * ptr);	// moves forward one code point
unichar u8_toU(char * ptr);	// returns single char from utf-8
unichar u8_prevU(char ** pptr);	// returns prev char from utf-8
unichar u8_nextU(char ** pptr);	// returns next char from utf-8
char * u8_appendU(char * ptr, unichar uc);	// appends utf8 string for unichar
char * u8_insertU(char * ptr, unichar uc, int gapchars);	// inserts utf8 string for unichar
int u8_countU(char * ptr, int length);	// counts uchars for length
BOOL u8_isvalidUTF8(char * ptr, int32_t length);	// checks validity of utf8
void u8_normalize(char * ptr, int length);		// normalizes utf-8 string to composed characters
void * setdata(HWND hwnd,void * vptr);		/* sets data associated with window */
void freedata(HWND hwnd);		/* frees data associated with window */
short numdigits(long number);		/* returns number of digits in the number */
void fixleadspace(TCHAR * buff);		// replaces leading spaces with fixed spaces
int toroman(char * string, int num, int upperflag);		/* makes roman numeral from int */
void menu_gettext(HMENU mh, int mid, TCHAR * text);		/* gets text of item ID */
//int menu_getid(HMENU mh, TCHAR * text);	/* returns ID of item matching text */
//int menu_getindex(HMENU mh, char * text);	/* returns position index of item matching text */
HMENU menu_gethandle(int id);	/* returns handle of submenu */
void menu_setlabelcolors(HMENU hmenu);	// builds and sets bcolored bitmaps for labels
int menu_getmid(HMENU hmenu);	// returns id of menu
void menu_delall(HMENU mh);		/* deletes all items */
void menu_addsubmenu(HMENU mh, HMENU sm, char * title);	// appends submenu
void menu_additem(HMENU mh, int mid, char * text);	/* appends item */
void menu_addsorteditem(HMENU mh, MENUITEMINFO * nmi);	// adds item in alpha position
HWND getactivemdi(int topflag);		/* gets active mdi window */
void showprogress(int id, long total, long done, ...);	/* displays progress */
void tbreturninfo(LPNMTOOLBAR tbnptr, TBBUTTON * tbb, int max, struct ttmatch *stbase);	/* fills out button info */
void setdc(HDC dc, BOOLEAN rtl, int viewoffset);		/* sets proper device context */
void scalerect(RECT * rptr, int mul, int div);		/* scales a rectangle */
void centerwindow(HWND dptr, int mode);	/* center dptr on bgnd */
int getmmstate(HWND hwnd, RECT *wrptr);	/* gets max/min state */
void fitrecttoclient(RECT * rptr);	//	adjusts rect to fit in MDI client window
int getcliplength(HWND hwnd, int *breaks);		/* gets clipboard text length & para breaks */
void enumclipformats(HWND hwnd);
void setint(HWND dptr, int item, long ival);		/* sets text int for dialog item */
void setfloat(HWND dptr, int item, float fval);	/* sets text int for dialog item */
BOOL checkdate(HWND dptr, int item);		// gets date from dlg item
BOOL getshort(HWND dptr, int item, short * val);	/* gets short from dialog item */
BOOL getlong(HWND dptr, int item,long * val);		/* gets long from dialog item */
short getfloat(HWND hwnd, short item, float * ddptr);	/* tests, loads double */
void selectitext(HWND hwnd, int item);	/* selects text and sets focus */
long findgroupcheck(HWND hwnd, int startitem, int enditem);		/* finds checked item in group */
short buildheadingmenu(HWND lbh, INDEXPARAMS * ig, int item);	 /* build heading menu */
short buildunitmenu(HWND lbh,int item);	 /* build unit menu */
void dprintf(HWND dptr, int item, char *fstring, ...);	 /* does printf to dialog item */
int isobscured (HWND hwnd, HWND hwtop);	/* finds top of obscured rect in window */
BOOL iscancelled(HWND hwnd);		/* looks at message queue for cancellation */
int builddraglist(HWND hwdl, char * string, short * list);	/* builds drag list */
void recoverdraglist(HWND hwdl, short * list);	/* recovers list order of drag items */
void switchdragitem (HWND hwdl);	/* handles double-click on drag item */
int handledrag (HWND hwnd, LPARAM lParam, int * startitem);	/* handles drag */
BOOL dodialoghelp(HWND hwnd, TCHAR * file, HELPINFO * hp, const int * idarray);	/* does dialog help */
void dowindowhelp(TCHAR * file);	/* does main help */
int geteditclass(HWND hwnd);	/* returns -1,0, 1 */
void adjustsortfieldorder(short * fieldorder, short oldtot, short newtot);	/* expands/contracts field order table, or warns */
time_t timegm(struct tm * rt);	// makes gm time from local time
void fixcombomenu(HWND hwnd, int item);	/* builds item menu for combo box */
void combo_addfiles(HWND cb, TCHAR * dir);		// adds files from dir
void checktextfield(HWND field, int limit);	// limits length of text in NSControl
void makelabelbitmaps(void);		// makes label bitmaps

void NSLog(char * message, ...);
