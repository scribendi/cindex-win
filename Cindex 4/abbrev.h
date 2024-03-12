#pragma once

#define ABBREVLEN 140   /* max length of expanded phrase */
#define ABBNAMELEN 8       /* length of name for abbrev */

extern TCHAR a_prefix[];
extern TCHAR a_suffix[];
extern short a_dirty;	/* flag for changed abbreviation list */
extern short a_tot;		/* total number of abbreviations */
extern char a_abname[];		/* name of currently selected abbrev */	

BOOL abbrev_hasactiveset(void);		// closes existing abrevviation set
void abbrev_close(HWND hw);		// closes existing abrevviation set
char *abbrev_checkentry(char *str);     /* returns pointer to string, or NULL */
void abbrev_view(void);		/* displays abbreviations */
void abbrev_makenew(HWND hwnd);	/* makes new abbrev */
short abbrev_checkok(void);	/* checks/saves before discarding abbrevs */
BOOL abbrev_save(void);	  /* saves abbrevs */
BOOL abbrev_open(TCHAR * path, BOOL visflag);	 /* loads abbreviations from file */
BOOL abbrev_new(TCHAR * path);	 /* starts new set of abbrevs */
