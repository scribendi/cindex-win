#include "stdafx.h"
#include "registry.h"

/* keys */
TCHAR * K_MASTER = TEXT("Software\\Indexing Research\\Cindex\\4.0");
TCHAR * K_TOOLBARS = TEXT("Toolbars");
TCHAR * K_PATHS = TEXT("Files");
TCHAR * K_SPELLPATHS = TEXT("AuxDics");
TCHAR * K_MACROS = TEXT("Macros");
TCHAR * K_GENERAL = TEXT("General");

#ifdef PUBLISH
TCHAR * K_ADMIN = TEXT("Admin");
TCHAR * K_UPDATES = TEXT("PubUpdates");
//WinSparkle key
char * K_WINSPARKLE = "Software\\Indexing Research\\Cindex\\4.0\\PubUpdates";
#else
TCHAR * K_UPDATES = TEXT("Updates");
//WinSparkle key
char * K_WINSPARKLE = "Software\\Indexing Research\\Cindex\\4.0\\Updates";
#endif

/* key values */

/* general */
TCHAR * FRAME = TEXT("frame");		/* frame window */
TCHAR *LANGUAGEDIC = TEXT("language");		/* language dictionary */
TCHAR *USERDIC = TEXT("userdic");		/* user dictionary */
TCHAR *CHECKSETTINGS = TEXT("checksettings");		// index check settings
TCHAR *SPLITSETTINGS = TEXT("splitsettings");		// heading split setting
TCHAR *COMPRESSSETTINGS = TEXT("compresssettings");		// index compress settings
TCHAR *MESSAGECHECK = TEXT("messagecheck");		// check for message

/* buttons */
TCHAR *F_TBREGISTERC = TEXT("fbuttons");	/* current frame toolbar config */
TCHAR *F_TBREGISTERD = TEXT("dfbuttons");	/* default frame toolbar config */
TCHAR *M_TBREGISTERC = TEXT("mbuttons");	/* current mod toolbar config */
TCHAR *M_TBREGISTERD = TEXT("dmbuttons");	/* default mod toolbar config */

/* files */
TCHAR *ALIS_ABBREV = TEXT("abbreviations");	/* abbrev file name */
TCHAR *ALIS_AUXDIC = TEXT("auxdic");		// aux dictionary
TCHAR *ALIS_DEFAULTDIR = TEXT("directory");	/* last used open directory */
TCHAR *ALIS_STYLESHEET = TEXT("stylesheet");	/* stylesheet save directory */

TCHAR *ALIS_RECENT[] = {TEXT("recent1"), TEXT("recent2"),TEXT("recent3"),TEXT("recent4"),TEXT("recent5"),TEXT("recent6"),TEXT("recent7"),TEXT("recent8"),TEXT("recent9"),TEXT("recent10"),TEXT("recent11"),TEXT("recent12")};

TCHAR *TAG_XML = TEXT("xmltags");	// path to default xml tags
TCHAR *TAG_SGML = TEXT("sgmltags");	// path to default sgml tags

TCHAR *MACROS[] = {TEXT("m0"),TEXT("m1"),TEXT("m2"),TEXT("m3"),TEXT("m4"),TEXT("m5"), TEXT("m6"),TEXT("m7"),TEXT("m8"),TEXT("m9")};		/* macros */
TCHAR *MSTRINGS[] = {TEXT("ms0"),TEXT("ms1"),TEXT("ms2"),TEXT("ms3"),TEXT("ms4"),TEXT("ms5"), TEXT("ms6"),TEXT("ms7"),TEXT("ms8"),TEXT("ms9")};		/* macro strings */ 

#ifdef PUBLISH
TCHAR *ALIS_CDXDIR = TEXT("cdx");	/* default index directory */
TCHAR *ALIS_STLDIR = TEXT("stl");	/* default style directory */
TCHAR *ALIS_CTGDIR = TEXT("ctg");	/* default tag directory */
TCHAR *ALIS_ABRDIR = TEXT("abr");	/* default abbrev directory */
TCHAR *ALIS_UDCDIR = TEXT("udc");	/* default udic directory */

TCHAR *AGENERAL = TEXT("General");		/* general admin entry */
#endif

/******************************************************************************/
static TCHAR * makefullkeyname(TCHAR * subkey)	//makes full key name

{
	static TCHAR kstring[STSTRING];

	nstrcpy(kstring,K_MASTER);
	if (subkey)	{
		nstrcat(kstring,TEXT("\\"));
		nstrcat(kstring,subkey);
	}
	return kstring;
}
/******************************************************************************/
void reg_makebasekeys(BOOL reset)	// builds base keys (resetting if required)

{
	if (reset)
		SHDeleteKey(HKEY_CURRENT_USER,K_MASTER);	// delete Cindex 4.0 subtree
	if (!reg_findkey(HKEY_CURRENT_USER,K_GENERAL))	{	// if don't have main keys
		reg_makeuserkey(K_GENERAL);
		reg_makeuserkey(K_TOOLBARS);
		reg_makeuserkey(K_PATHS);
		reg_makeuserkey(K_MACROS);
	}
}
#if 0
/******************************************************************************/
TCHAR * reg_indexedkeyname(TCHAR * base, int index)		// creates name for indexed key

{
	static TCHAR buffer[100];

	sprintf(buffer, "%s%d",base,index);
	return buffer;
}
#endif
/******************************************************************************/
HKEY reg_findkey(HKEY base, TCHAR * subkey)	/* gets handle for named registry key (or CINDEX 3.0) key */

{
	HKEY hk;
	if (RegOpenKeyEx(base,makefullkeyname(subkey),0,KEY_READ|KEY_WRITE,&hk) == ERROR_SUCCESS)
		return(hk);
	return (NULL);
}
/******************************************************************************/
HKEY reg_makeuserkey(TCHAR * subkey)		/* makes new subkey under CINDEX 3.0 */

{
	HKEY shk;
	DWORD disp;

	if (RegCreateKeyEx(HKEY_CURRENT_USER,makefullkeyname(subkey),0,TEXT(""),REG_OPTION_NON_VOLATILE,KEY_READ|KEY_WRITE,NULL,&shk,&disp) == ERROR_SUCCESS)
		return(shk);
	return (NULL);
}
/******************************************************************************/
LSTATUS reg_deleteuserkey(TCHAR * subkey)		// deletes key and its tree

{
	return SHDeleteKey(HKEY_CURRENT_USER,makefullkeyname(subkey));	// delete 					
}
/******************************************************************************/
TCHAR * reg_enumeratekey(HKEY base, TCHAR * subkey, int index)	/* gets handle for named registry key (or CINDEX 3.0) key */

{
	static HKEY ekey;
	static TCHAR name[STSTRING];
	int size = STSTRING;
	long result;
	
	if (!index)
		ekey = reg_findkey(base, subkey);	// open key
	result = RegEnumValue(ekey,index,name,&size,NULL,NULL,NULL,NULL);
	if (result == ERROR_SUCCESS)
		return name;
	else
		RegCloseKey(ekey);
	return NULL;
}
/******************************************************************************/
BOOL reg_getkeyvalue(TCHAR * key, TCHAR * valname, void *datap, DWORD *sizep)	/* returns true if value exists */

{
	HKEY hk;
	DWORD type;
	BOOL result;

	result = FALSE;
	if (hk = reg_findkey(HKEY_CURRENT_USER,key))	{	/* if have the key */
		if (RegQueryValueEx(hk,valname,NULL,&type,datap,sizep) == ERROR_SUCCESS)
			result = TRUE;
		RegCloseKey(hk);
	}
	return result;
}
/******************************************************************************/
BOOL reg_setkeyvalue(TCHAR * key, TCHAR * valname, DWORD type, void * datap, DWORD size)	/* sets registry data value */

{
	BOOL result = FALSE;
	HKEY hk;

	if (type == REG_SZ)
		size *= sizeof(TCHAR);
	if (hk = reg_findkey(HKEY_CURRENT_USER,key))	{	/* if have the key */
		if (RegSetValueEx(hk,valname,0,type,datap,size) == ERROR_SUCCESS)
			result = TRUE;
		RegCloseKey(hk);
	}
	return result;
}
/******************************************************************************/
BOOL reg_getmachinekeyvalue(TCHAR * key, TCHAR * valname, void *datap, DWORD *sizep)	/* returns true if value exists */

{
	HKEY hk;
	DWORD type;
	BOOL result;

	result = FALSE;

	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,makefullkeyname(key),0,KEY_READ,&hk) == ERROR_SUCCESS)	{
		if (RegQueryValueEx(hk,valname,NULL,&type,datap,sizep) == ERROR_SUCCESS)
			result = TRUE;
		RegCloseKey(hk);
	}
	return result;
}
/******************************************************************************/
BOOL reg_setmachinekeyvalue(TCHAR * key, TCHAR * valname, DWORD type, void * datap, DWORD size)	/* sets registry data value */

{
	BOOL result = FALSE;
	HKEY hk;

	if (type == REG_SZ)
		size *= sizeof(TCHAR);
	if (hk = reg_makemachinekey(key))	{	/* if have the key or can create */
		if (RegSetValueEx(hk,valname,0,type,datap,size) == ERROR_SUCCESS)
			result = TRUE;
		RegCloseKey(hk);
	}
	return result;
}
/******************************************************************************/
HKEY reg_makemachinekey(TCHAR * subkey)		/* makes new subkey under CINDEX 3.0 */

{
	HKEY shk;
	DWORD disp;

	if (RegCreateKeyEx(HKEY_LOCAL_MACHINE,makefullkeyname(subkey),0,TEXT(""),REG_OPTION_NON_VOLATILE,KEY_READ|KEY_WRITE,NULL,&shk,&disp) == ERROR_SUCCESS)
		return(shk);
	return (NULL);
}
