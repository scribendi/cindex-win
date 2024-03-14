#pragma once

//WinSparkle key
char * K_WINSPARKLE;

/* keys */
extern TCHAR * K_MASTER;
extern TCHAR * K_TOOLBARS;
extern TCHAR * K_PATHS;
extern TCHAR * K_SPELLPATHS;
extern TCHAR * K_MACROS;
extern TCHAR * K_GENERAL;
extern TCHAR * K_UPDATES;

#ifdef PUBLISH
extern TCHAR * K_ADMIN;
#endif //PUBLISH

/* key values */
extern TCHAR * FRAME;		/* frame window */
extern TCHAR *LANGUAGEDIC;		/* language dictionary */
extern TCHAR *USERDIC;		/* user dictionary */
extern TCHAR *CHECKSETTINGS;		// index check settings
extern TCHAR *SPLITSETTINGS;		// heading split settings
extern TCHAR *COMPRESSSETTINGS;		// index compress settings
extern TCHAR *MESSAGECHECK;		// check for message

extern TCHAR *F_TBREGISTERC;	/* current frame toolbar config */
extern TCHAR *F_TBREGISTERD;	/* default frame toolbar config */
extern TCHAR *M_TBREGISTERC;	/* current mod toolbar config */
extern TCHAR *M_TBREGISTERD;	/* default mod toolbar config */

extern TCHAR *ALIS_ABBREV;	/* abbrev file name */
extern TCHAR *ALIS_AUXDIC;		// aux dictionary
extern TCHAR *ALIS_DEFAULTDIR;	/* last used open directory */
extern TCHAR *ALIS_STYLESHEET;	/* stylesheet save directory */
extern TCHAR *ALIS_RECENT[];	/* recent files */

extern TCHAR *TAG_XML;		// path to default xml tags
extern TCHAR *TAG_SGML;		// path to default sgml tags

extern TCHAR *MACROS[];		/* macros */
extern TCHAR *MSTRINGS[];		/* macro strings */ 

#ifdef PUBLISH
extern TCHAR *ALIS_CDXDIR;	/* default index directory */
extern TCHAR *ALIS_STLDIR;	/* default style directory */
extern TCHAR *ALIS_CTGDIR;	/* default tag directory */
extern TCHAR *ALIS_ABRDIR;	/* default abbrev directory */
extern TCHAR *ALIS_UDCDIR;	/* default udic directory */

extern TCHAR *AGENERAL;		/* general admin entry */
#endif //PUBLISH

void reg_makebasekeys(BOOL reset);	// builds base keys (resetting if required)
TCHAR * reg_indexedkeyname(TCHAR * base, int index);		// creates name for indexed key
HKEY reg_findkey(HKEY base,TCHAR * subkey);		/* gets handle for named registry key */
HKEY reg_makeuserkey(TCHAR * subkey);		/* makes new subkey under CINDEX 1.0 */
LSTATUS reg_deleteuserkey(TCHAR * subkey);		// deletes key and its tree
TCHAR * reg_enumeratekey(HKEY base, TCHAR * subkey, int index);	/* gets handle for named registry key (or CINDEX 3.0) key */
BOOL reg_getkeyvalue(TCHAR * key, TCHAR * valname, void *datap, DWORD *sizep);	/* returns true if value exists */
BOOL reg_setkeyvalue(TCHAR * key, TCHAR * valname, DWORD type,void * datap, DWORD sizep);	/* sets registry data value */
BOOL reg_getmachinekeyvalue(TCHAR * key, TCHAR * valname, void *datap, DWORD *sizep);	/* returns true if value exists */

BOOL reg_setmachinekeyvalue(TCHAR * key, TCHAR * valname, DWORD type, void * datap, DWORD size);	/* sets registry data value */
HKEY reg_makemachinekey(TCHAR * subkey);		/* makes new subkey under CINDEX 1.0 */
