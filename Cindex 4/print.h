#pragma once
extern short p_firstpage, p_lastpage;
extern RECN p_firstrec, p_lastrec;
extern PRINTDLG p_dlg;
extern BOOL p_abort;			/* print abort flag */
extern HDC p_dc;		// printing dc

//extern HWND p_hwnd;		/* cancel print dialog handle */
extern int p_err;		/* print error flag */

HDC print_getic(void);	/* gets ic for current printer	*/
BOOL print_setpage(MARGINCOLUMN *mcp, PAPERINFO * pip);	/* sets & gets page info */
int print_begin(DWORD flags, TCHAR * title, PAPERINFO * pip, HWND hwnd);	/* sets up printing for window */
void print_end(int pages, HWND hwnd);	/* cleans up after printing	*/
void pr_printdoc(HWND wptr);	/* prints document for window */
void pr_defaulthead(HDC dc, TCHAR * title, short pnum, MARGINCOLUMN *margins, RECT * trect);	/* prints default header */
void pr_headfoot(INDEX * FF, HDC dc, short headflag, long page, RECT *textrect);	/* prints header/footer */
