#include "stdafx.h"
#include <shlobj.h>
#include "files.h"
#include "commands.h"
#include "errors.h"
#include "mfile.h"
#include "index.h"
#include "search.h"
#include "group.h"
#include "sort.h"
#include "collate.h"
#include "records.h"
#include "strings.h"
#include "typestuff.h"
#include "viewset.h"
#include "formstuff.h"
#include "import.h"
#include "indexset.h"
#include "text.h"
#include "util.h"
#include "abbrev.h"
#include "toolset.h"
#include "translate.h"
#include "registry.h"
#include "macros.h"
#include "containerset.h"
#include "v3handler.h"

static const DWORD wh_newid[] = {
	IDC_NEW_OPTIONS, HIDC_NEW_OPTIONS,
	0,0
};

static const DWORD wh_archid[] = {
	IDC_ARCHIVE_INCLUDE, HIDC_ARCHIVE_INCLUDE,
	IDC_ARCHIVE_NEW, HIDC_ARCHIVE_NEW,
	0,0
};

OPENFILENAME ofn = {
	sizeof(OPENFILENAME),
	NULL,	/* owner */
	NULL,	/* instance */
	NULL,	/* name filter strings */
	NULL,	/* custom filter buffer */
	0,		/* custom filter buffer size */
	0,		/* filter index */
	NULL,	/* filename & path */
	_OFN_NAMELEN,	/* length of path */
	NULL,	/* file title */
	_MAX_FNAME+_MAX_EXT,	/* length of filename+extn */
	NULL,	/* initial directory */
	NULL,	/* title of Dialog box */
	0,		/* flags */
	0,		/* offset to file name in path */
	0,		/* offset to extension in path */
	NULL,	/* default extension */
	0L,		/* data to hook function */
	NULL,	/* hook function */
	NULL	/* alternate dialog template */
};

static TCHAR ifilters[] =
TEXT("All Readable\0*.ixml;*.sky7;*.txtsky7;*.txtsky8;*.mbk;*.arc;*.txt;*.dat\0")\
TEXT("XML Records\0*.ixml\0")\
TEXT("Cindex Archive\0*.arc\0")\
TEXT("Delimited Text\0*.txt\0")\
TEXT("Sky Index 7 / 8\0*.sky7;*.txtsky7;*.txtsky8\0")\
TEXT("DOS Cindex Data\0*.dat\0")\
TEXT("Macrex Backup\0*.mbk\0\0");

static TCHAR ofilters[] =
/* TEXT("All Readable\0*.ucdx;*.cdx;*.arc;*.ixml;*.utpl;*.tpl;*.ucbr;*.ustl;*.stl;*.ndx\0")\ */
TEXT("All Readable\0*.ucdx;*.cdx;*.arc;*.ixml;*.utpl;*.tpl;*.ustl;*.stl;*.ndx\0")\
TEXT("Indexes (*.ucdx; *.cdx; *.ndx)\0*.ucdx; *.cdx; *.ndx\0")\
TEXT("XML Records (*.ixml)\0*.ixml\0")\
TEXT("Archives (*.arc)\0*.arc\0")\
TEXT("Templates (*.utpl; *.tpl)\0*.utpl;*.tpl\0")\
/* TEXT("Abbreviations (*.ucbr; *.cbr)\0*.ucbr;*.cbr\0")\	*/
TEXT("Style Sheets (*.ustl; *.stl)\0*.ustl;*.stl\0");

static TCHAR sfilters[] =		// stationery filters (Pub Ed)
TEXT("Style Sheets (*.ustl; *.stl)\0*.ustl;*.stl\0")\
TEXT("Templates (*.utpl; *.tpl)\0*.utpl;*.tpl\0");

static TCHAR openabfilter[] =
TEXT("Abbreviations (*.ucbr; *.cbr)\0*.ucbr;*.cbr\0");

TCHAR newindexfilter[] = TEXT("Index (*.ucdx)\0*.ucdx\0");

static TCHAR newabfilter[] =
/* TEXT("Index (*.ucdx)\0*.ucdx\0")\ */
TEXT("Abbreviations (*.ucbr)\0*.ucbr\0");

static TCHAR sfilter[] = TEXT("Style Sheets (*.ustl)\0*.ustl\0");

static TCHAR f_cpath[MAX_PATH];	/* initial output directory for save a copy */

typedef struct {
	int type;
	TCHAR * extn;
} FILETYPE;

FILETYPE filetypes[] = {
	{FTYPE_INDEX, TEXT(".ucdx")},
	{FTYPE_INDEXV2, TEXT(".cdx")},
	{FTYPE_INDEXDOS, TEXT(".ndx")},
	{FTYPE_TEMPLATE, TEXT(".utpl")},
	{FTYPE_TEMPLATEV2, TEXT(".tpl")},
	{FTYPE_STYLESHEET, TEXT(".ustl")},
	{FTYPE_STYLESHEETV2, TEXT(".stl")},
	{FTYPE_ABBREV, TEXT(".ucbr")},
	{FTYPE_ABBREVV2, TEXT(".cbr")},
	{FTYPE_ARCHIVE, TEXT(".arc")},
	{FTYPE_XMLRECORDS, TEXT(".ixml")},
	{FTYPE_PLAINTEXT, TEXT(".txt")},
//	{FTYPE_TABTEXT, TEXT(".tab")},
	{FTYPE_MACREX, TEXT(".mbk")},
	{FTYPE_DOSDATA, TEXT(".dat")},
	{FTYPE_SKY, TEXT(".sky7")},
	{FTYPE_SKY7, TEXT(".txtsky7")},
	{FTYPE_SKY8, TEXT(".txtsky8")},
};
#define TYPECOUNT (sizeof(filetypes)/sizeof(FILETYPE))

static void copyuserfiles(int folderType, TCHAR * path, TCHAR * type);	// for first run, copy files from previous version 
static INT_PTR CALLBACK ihook(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static BOOL startnewdoc(HEAD * hp, TCHAR * prompt, TCHAR * filter, TCHAR *name);	/* starts new index */
static INT_PTR CALLBACK newhook (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static long opendocument(TCHAR * prompt, TCHAR * filter);	/* opens document */
static LRESULT CALLBACK openhook(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK shook(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static short findfiletype(TCHAR * path, TCHAR * filters);	/* finds index of file type selected */
static TCHAR * findfileatindex(OPENFILENAME * ofn, int index);	// find indexed file from multiple
static BOOL styletype(TCHAR * path);	/* returns true if file is style file */
static long abbrevtype(TCHAR * path);	/* returns true if file is abbrevs */
static void setcurdir(TCHAR * path);	/* sets current directory */
static INDEX * openindexfile(TCHAR * path, short openflags);	/* opens and sets up index */
static BOOL installindex(INDEX * FF, short openflags);		// installs index
static void checkfixes(INDEX * FF);	/* deals with minor version fixes */
static INT_PTR getarchpars(void);	/* gets user id */
static INT_PTR CALLBACK archproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static BOOL convertpath(TCHAR * path, TCHAR * newpath, int type);	// checks whether file convertible
static BOOL convertdoc(TCHAR * path);	//	 converts doc if necessary; returns path to converted

/*******************************************************************************/
long mc_import(HWND hwnd,int comid,HWND chandle,UINT notify)	/* imports records */

{
	TCHAR path[_OFN_NAMELEN], title[_MAX_FNAME+_MAX_EXT];
	IMPORTPARAMS imp;

	ofn.hwndOwner = g_hwframe;
	*path = '\0';
	*title = '\0';
	ofn.lpstrFilter = ifilters;
	ofn.lpstrFile = path;		/* holds path on return */
	ofn.lpstrFileTitle = title;	/* holds file title on return */
	ofn.lpstrTitle = TEXT("Import Records");
	ofn.Flags = OFN_PATHMUSTEXIST|OFN_FILEMUSTEXIST|OFN_EXPLORER|OFN_HIDEREADONLY|OFN_ENABLEHOOK|OFN_NOCHANGEDIR|OFN_ENABLESIZING;
	ofn.lpstrDefExt = TEXT("arc");
	ofn.hInstance = NULL;
	ofn.lpfnHook = ihook;
	ofn.lCustData = (LPARAM)&imp;
	ofn.lpTemplateName = NULL;
	memset(&imp,0,sizeof(imp));	// clear imp structure
	
	if (GetOpenFileName(&ofn))	{	/* if want to use */
		if (!imp_loaddata(WX(hwnd,owner),&imp,ofn.lpstrFile))
			senderr(ERR_OPENERR,WARN,file_getname(ofn.lpstrFile));
	}
	return (0);
}
/**********************************************************************************/	
static INT_PTR CALLBACK ihook(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)

{
	OPENFILENAME * ofp;
	IMPORTPARAMS *imp;
	int type;

	switch (msg)	{
		case WM_INITDIALOG:
			centerwindow(GetParent(hwnd),0);
			return (TRUE);
		case WM_NOTIFY:
			ofp = ((LPOFNOTIFY)lParam)->lpOFN;
			switch (((LPOFNOTIFY)lParam)->hdr.code)	{
				case CDN_FILEOK:		/* about to close */
					imp = (IMPORTPARAMS *)ofp->lCustData;
					type = file_type(ofp->lpstrFile);
					if (type == FTYPE_PLAINTEXT)
						imp->type = I_PLAINTAB;
					else if (type == FTYPE_MACREX)
						imp->type = I_MACREX;
					else if (type == FTYPE_SKY || type == FTYPE_SKY7 || type == FTYPE_SKY8)
						imp->type = I_SKY;
					else if (type == FTYPE_XMLRECORDS)
						imp->type = I_CINXML;
					else if (type == FTYPE_ARCHIVE)
						imp->type = I_CINARCHIVE;
					else if (type == FTYPE_DOSDATA)	{
						imp->type = I_DOSDATA;	// will be reconfigured
					}
					else {
						if (!sendwarning(WARN_BADFILETYPE,file_getname(ofp->lpstrFile)))	{	/* if don't want to import */
							SetWindowLongPtr(hwnd,DWLP_MSGRESULT,TRUE);		/* bad file */		
							return (TRUE);
						}
						imp->type = I_PLAINTAB;	// default to plaintab
					}
			}
	}
	return (FALSE);
}
/*******************************************************************************/
static void copyuserfiles(int folderType,TCHAR * path, TCHAR * type)	// for first run, copy files from previous version 

{
	TCHAR cpath[MAX_PATH];
	HRESULT nofolder;

	nofolder = SHGetFolderPathAndSubDir(NULL, folderType,NULL,SHGFP_TYPE_CURRENT,TEXT("\\Indexing Research\\Cindex 3.0"),cpath);	// Cindex 3
	if (!nofolder) {
		WIN32_FIND_DATA fd;
		HANDLE fh;
		BOOL fok;
		TCHAR spath[MAX_PATH], sourcepath[MAX_PATH], destpath[MAX_PATH];
		
		nstrcpy(spath, cpath);
		PathAppend(spath,type);
		for (fh = FindFirstFile(spath,&fd), fok = TRUE; fh != INVALID_HANDLE_VALUE && fok; fok = FindNextFile(fh,&fd))	{
			nstrcpy(sourcepath, cpath);
			PathAppend(sourcepath,fd.cFileName);
			nstrcpy(destpath, path);
			PathAppend(destpath,fd.cFileName);
			CopyFile(sourcepath,destpath,TRUE);		// copy to our new home
		}
		if (fh)	/* if found anything */
			FindClose(fh);
	}
}
/*******************************************************************************/
short file_loadconfig(short noload)		/* loads configuration variables */

{
	TCHAR wpath[MAX_PATH];
	short err = 0;
	int index, count;
	DWORD attrib;

	reg_makebasekeys(noload);		// make or reset registry keys as necessary
#ifdef PUBLISH
	DWORD size = sizeof(ADMIN);
	reg_getmachinekeyvalue(K_ADMIN,AGENERAL,&g_admin,&size);	// get admin prefs if there are any
#endif //PUBLISH
	for (index = count = 0; count < MAXRECENT; count++)	{		/* load recent files */
		c_recent[count].order = count;	/* default order is 0...3 */
		if (file_getuserpath(c_recent[index].path,ALIS_RECENT[count]))	{	/* if have path */
			if ((attrib = GetFileAttributes(c_recent[index].path)) == INVALID_FILE_ATTRIBUTES	/* if not file */
				|| attrib&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_OFFLINE|FILE_ATTRIBUTE_HIDDEN))	/* or wrong kind */
				*c_recent[index].path = '\0';
			else		/* count one index (don't increment for bad paths) */
				index++;
		}
	}
	mcr_load();		/* load macros */
	if (file_getdefpath(wpath,FOLDER_PREF,g_prefname))	{		/* if can get spec for preferences */
		if (!noload)	{		/* if actually want to read preferences */
			MFILE mf;
			if (mfile_open(&mf,wpath,GENERIC_READ,FILE_SHARE_READ,OPEN_EXISTING,0,0))	{
				struct prefs * tpr = (struct prefs *)mf.base;
				if (mf.size == sizeof(g_prefs) && tpr->key == g_prefs.key)	{	// if good data
					g_prefs = *tpr;
					if (g_prefs.sortpars.sversion < LOCALESORTVERSION)	// patch sort collation params (avoids invalidating all prefs)
						col_fixLocaleInfo(&g_prefs.sortpars);
				}
				else 	// bad config; will use defaults
					sendinfo(INFO_BADCONFIG);
				mfile_close(&mf);
			}
		}
		// will use default config unless successful load
		return (TRUE);
	}
	return (FALSE);
}
/*******************************************************************************/
void file_saveconfig(void)		/* saves configuration variables */

{
	TCHAR wpath[MAX_PATH];
	HANDLE fref;
	long writesize;
	int index;

	for (index = 0; index < MAXRECENT; index++)
		file_saveuserpath(com_findrecent(index),ALIS_RECENT[index]);
	mcr_save();		/* save macros */

	if (file_getdefpath(wpath,FOLDER_PREF,g_prefname))	{		/* if can get spec for preferences */
		if ((fref = CreateFile(wpath,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,0)) != INVALID_HANDLE_VALUE)	{
			if (WriteFile(fref,&g_prefs,sizeof(struct prefs),&writesize,NULL))	{
				CloseHandle(fref);
				return;
			}
			CloseHandle(fref);
		}
		senderr(ERR_FILESYSERR, WARN, wpath);
	}
}
/**********************************************************************************/	
void file_makecinpath(TCHAR *path, TCHAR * file)	/* puts file in path to cindex directory */

{
	TCHAR * endpath;

	nstrcpy(path,toNative(__argv[0]));
	endpath = nstrrchr(path,'\\');
	nstrcpy(++endpath,file);
 }
/**********************************************************************************/	
void file_maketemppath(TCHAR *path, TCHAR * file)	/* puts file in path to temp directory */

{
	GetTempPath(MAX_PATH,path);
	nstrcat(path,file);
}
/**********************************************************************************/	
long mc_new(HWND hwnd,int comid,HWND chandle,UINT notify)	/* sets up new index */

{
	startnewdoc(NULL, TEXT("New Index"),newindexfilter, TEXT(""));
	return (0);
}
/**********************************************************************************/	
BOOL file_newabbrev(void)	/* sets up new abbreviation file */

{
	return startnewdoc(NULL, TEXT("New Abbreviations"),newabfilter, TEXT(""));
}
/*******************************************************************************/
static BOOL startnewdoc(HEAD * hp, TCHAR * prompt, TCHAR * filter, TCHAR * name)	/* starts new index */

{
	TCHAR path[_OFN_NAMELEN], title[_MAX_FNAME+_MAX_EXT], dir[MAX_PATH], *eptr;
	INDEXPARAMS idxpars;

	idxpars = hp ? hp->indexpars : g_prefs.indexpars;
	ofn.hwndOwner = g_hwframe;
	file_getdefpath(dir,FOLDER_CDX,TEXT(""));
	nstrcpy(path,name);		/* supply any default filename */
	if (eptr = file_getextn(path))	/* without extension */
		*eptr = '\0';
	*title = '\0';
	ofn.lpstrFilter = filter;
	ofn.nFilterIndex = 1;		/* default is index */
	ofn.lpstrFile = path;		/* holds path on return */
	ofn.lpstrInitialDir = dir;
	ofn.lpstrFileTitle = title;	/* holds file title on return */
	ofn.lpstrTitle = prompt;
//	ofn.Flags = OFN_OVERWRITEPROMPT|OFN_HIDEREADONLY|OFN_PATHMUSTEXIST
//		|OFN_EXPLORER|OFN_ENABLETEMPLATE|OFN_ENABLEHOOK|OFN_ENABLESIZING;
	ofn.Flags = OFN_OVERWRITEPROMPT|OFN_HIDEREADONLY|OFN_PATHMUSTEXIST
		|OFN_EXPLORER|OFN_ENABLEHOOK|OFN_ENABLESIZING;
	if (filter == newindexfilter)
		ofn.Flags |= OFN_ENABLETEMPLATE;	// enable template if new index
	ofn.lpstrDefExt = file_extensionfortype(FTYPE_INDEX)+1;
	ofn.lpfnHook = newhook;
	ofn.hInstance = g_hinst;
	ofn.lCustData = (LONG_PTR)&idxpars;	/* copy of default index params */
	ofn.lpTemplateName = MAKEINTRESOURCE(IDD_NEW_OPTBUTTON);
	
	if (GetSaveFileName(&ofn))	{	/* if want to use */
		if (file_type(ofn.lpstrFile) == FTYPE_INDEX)	// if wanting index
			return(file_createindex(ofn.lpstrFile,&idxpars,hp));
		else	/* new abbreviation set */
			return (abbrev_new(ofn.lpstrFile));
	}
	return (FALSE);	/* canceled */
}
/**********************************************************************************/	
BOOL file_createindex(TCHAR * path, INDEXPARAMS * ip, HEAD * hp)	/* starts new index */

{
	INDEX * FF;
	short oldmaxfieldcount;

	DeleteFile(path);	/* delete any existing */
	if (FF = file_setupindex(path,GENERIC_READ|GENERIC_WRITE,CREATE_NEW))	{
		index_setdefaults(FF);			/* set up default features */
		if (hp)	{		// if have source for header (opening template)
			// copy relevant head params
			FF->head.indexpars = hp->indexpars;
			FF->head.sortpars = hp->sortpars;
			FF->head.refpars = hp->refpars;
			FF->head.privpars = hp->privpars;
			FF->head.formpars = hp->formpars;
			str_xcpy(FF->head.stylestrings,hp->stylestrings);	/* copy style strings */
			strcpy(FF->head.flipwords,hp->flipwords);	/* copy style strings */
			memcpy(FF->head.fm,hp->fm,sizeof(hp->fm));	/* copy local font set */
		}
		oldmaxfieldcount = FF->head.indexpars.maxfields;
		FF->head.indexpars = *ip;		/* set any adjusted structure params */
		adjustsortfieldorder(FF->head.sortpars.fieldorder, oldmaxfieldcount, FF->head.indexpars.maxfields);
		if (installindex(FF,OP_VISIBLE))
			return TRUE;
		index_closefile(FF);	/* clean up after error */
		index_cut(FF);
		DeleteFile(path);	/* delete new file */
	}
	return FALSE;
}
/**********************************************************************************/	
static INT_PTR CALLBACK newhook(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)

{
	HWND parent;
	INDEXPARAMS * idxptr;
	OPENFILENAME * ofp;
	int type;

	idxptr = getdata(hwnd);
	parent = GetParent(hwnd);
	switch (msg)	{

		case WM_INITDIALOG:
			setdata(hwnd, (void *)((OPENFILENAME*)lParam)->lCustData);
			centerwindow(parent,0);
			setDItemText(parent,stc2,"File of &type:");
			setDItemText(parent,IDOK,"Create");
			setDItemText(parent, stc4,"Create &in:");		/* set text */
			return (TRUE);
		case WM_COMMAND:
			switch (LOWORD(wParam))	{
				case IDC_NEW_OPTIONS:
					is_setstruct(NULL, idxptr);		/* set up options */
					return TRUE;
			}
			return FALSE;
		case WM_NOTIFY:
			ofp = ((LPOFNOTIFY)lParam)->lpOFN;
			switch (((LPOFNOTIFY)lParam)->hdr.code)	{
				case CDN_FILEOK:
					type = file_type(ofp->lpstrFile);
					if (type == FTYPE_ABBREV)	/* if abbrevs */
						break;
					if (type == FTYPE_INDEX)	{	/* if want index */
						if (index_byfspec(ofp->lpstrFile))	/* if already open */
							senderr(ERR_OVERWRITEOPEN,WARN,file_getname(ofp->lpstrFile));		/* send open err */
#ifdef PUBLISH
						else if (!g_admin.permitwrite)	/* if writing to index forbidden */
							senderr(ERR_NOWRITEACCESS, WARN);	/* write access forbidden */
#endif //PUBLISH
						else
							break;
					}
					else	/* bad file type */
						senderr(ERR_BADNEWFILERR,WARN,&ofp->lpstrFile[ofp->nFileExtension]);
					SetWindowLongPtr(hwnd, DWLP_MSGRESULT,TRUE);		/* bad file */
					return (TRUE);
				case CDN_TYPECHANGE:
					if (ofp->nFilterIndex == 1)	{	/* if index */
						enableitem(hwnd,IDC_NEW_OPTIONS); 	/* enable options */
						file_setaccess(hwnd,parent,FOLDER_CDX);
					}
					else	{				/* abbreviations */
						disableitem(hwnd,IDC_NEW_OPTIONS);
						file_setaccess(hwnd,parent,FOLDER_ABR);
					}
					break;
			}
			break;
		case WM_HELP:
			return (dodialoghelp(hwnd,NULL,(HELPINFO *)lParam,wh_newid));
		default:
			;
	}
	return (FALSE);
}
/**********************************************************************************/	
INDEX * file_setupindex(TCHAR *path, DWORD accessflags, DWORD createflags)	/* sets up new index structures; opens file */

{
	INDEX * FF;
	TCHAR * titleptr;

	if (titleptr = file_getname(path))	{	/* if have a name */
		for (FF = g_indexbase; FF; FF = FF->inext)	{	/* for all indexes */
			if (nstricmp(FF->iname, titleptr) > 0)	/* if name higher */
				break;
		}
		if (FF = index_insert(FF))	{		/* if can get memory for new index */
			nstrcpy(FF->iname, titleptr);	/* copy name */
			nstrcpy(FF->pfspec,path);
			setcurdir(path);	/* make this the working directory */
#ifdef PUBLISH
			if (!g_admin.permitwrite)
				accessflags &= ~GENERIC_WRITE;		/* forbid write access */
			if (!mfile_open(&FF->mf,path,accessflags,FILE_SHARE_READ,createflags,FILE_FLAG_RANDOM_ACCESS,0))	{	// if failed to open
				if (GetLastError() == ERROR_SHARING_VIOLATION)	{	// if already open for writing
					if (g_admin.readaccess)	{
						if (sendwarning(WARN_INDEXINUSE,file_getname(path)))	{
							accessflags = GENERIC_READ;
							mfile_open(&FF->mf,path,accessflags,FILE_SHARE_READ|FILE_SHARE_WRITE,createflags,FILE_FLAG_RANDOM_ACCESS,0);
						}
					}
					else
						senderr(ERR_FILEALREADYOPEN,WARN,file_getname(path));
				}
			}
			if (FF->mf.fref)	{	// if ok
				FF->opentime = FF->lastflush = time(NULL);		/* set open time, flush time to now */
				index_getmodtime(FF);	/* get time of last change (or creation) */
//				FF->readonly = accessflags&GENERIC_WRITE ? FALSE : TRUE;	/* TRUE if no write access */
				return (FF);
			}
#else
			if (mfile_open(&FF->mf,path,accessflags,FILE_SHARE_READ,createflags,FILE_FLAG_RANDOM_ACCESS,0))	{
				FF->opentime = FF->lastflush = time(NULL);		/* set open time, flush time to now */
				index_getmodtime(FF);	/* get time of last change (or creation) */
//				FF->readonly = accessflags&GENERIC_WRITE ? FALSE : TRUE;	/* TRUE if no write access */
				return (FF);

			}
#endif //PUBLISH
			index_cut(FF);
		}
	}
	return (NULL);
}	
/**********************************************************************************/	
long mc_open(HWND hwnd,int comid,HWND chandle,UINT notify)	/* opens document */

{
	return opendocument(TEXT("Open"),ofilters);	// prompt filter
}
/**********************************************************************************/	
long file_openabbrev(void)	/* attaches abbreviation */

{
	return opendocument(TEXT("Open Abbreviations"),openabfilter);
}
/**********************************************************************************/	
static long opendocument(TCHAR * prompt, TCHAR * filter)	/* opens document */

{
	TCHAR path[_OFN_NAMELEN], title[_MAX_FNAME+_MAX_EXT], dir[MAX_PATH];

	ofn.hwndOwner = g_hwframe;
	file_getdefpath(dir,FOLDER_CDX,TEXT(""));
	*path = '\0';
	*title = '\0';
	ofn.lpstrFilter = filter;
	ofn.nFilterIndex = 1;		/* default is index */
	ofn.lpstrFile = path;		/* holds path on return */
	ofn.lpstrInitialDir = dir;
	ofn.lpstrFileTitle = title;	/* holds file title on return */
	ofn.lpstrTitle = prompt;
	ofn.Flags = OFN_PATHMUSTEXIST|OFN_FILEMUSTEXIST|OFN_EXPLORER|OFN_ENABLEHOOK|OFN_ALLOWMULTISELECT|OFN_ENABLESIZING;
	if (filter == openabfilter)	// if using abbrevs
		ofn.Flags = OFN_HIDEREADONLY;
	ofn.lpfnHook = openhook;
	ofn.lpstrDefExt = file_extensionfortype(FTYPE_INDEX)+1;
	ofn.lCustData = 0;	/* no param */
	ofn.lpTemplateName = NULL;
	
	if (GetOpenFileName(&ofn))	{	/* if want to use */
		int findex;
		TCHAR * fpath;
		for (findex = 0; fpath = findfileatindex(&ofn, findex); findex++)	{
			file_open(fpath,(ofn.Flags&OFN_READONLY) ? OP_READONLY|OP_VISIBLE : OP_VISIBLE);
			continue;
		}
	}
	return (0);
}
/**********************************************************************************/	
static LRESULT CALLBACK openhook(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)

{
	HWND parent;
	OPENFILENAME * ofp;
	int type;
	int findex;
	TCHAR * fpath;

	switch (msg)	{

		case WM_INITDIALOG:
			parent = GetParent(hwnd);

			centerwindow(parent,0);
#ifdef PUBLISH
			if (!g_admin.permitwrite)	{		/* if can't write */
				disableitem(parent,chx1); 		/* disable read only  */
				checkitem(parent,chx1,TRUE);	/* check read-only */
			}
			file_setaccess(hwnd,parent,FOLDER_CDX);
#endif //PUBLISH
			return (TRUE);
		case WM_NOTIFY:
			ofp = ((LPOFNOTIFY)lParam)->lpOFN;
			parent = GetParent(hwnd);
			switch (((LPOFNOTIFY)lParam)->hdr.code)	{
				case CDN_FILEOK:		/* about to close */
					for (findex = 0; fpath = findfileatindex(&ofn, findex); findex++)	{
						type = file_type(ofp->lpstrFile);
						switch (type)	{	/* check file types */
							case FTYPE_ARCHIVE:
								if (file_archivetype(ofp->lpstrFile))	/* if good archive */
									continue;
								goto error;
#ifdef PUBLISH
								if (!g_admin.permitwrite)	{	/* if writing to index forbidden */
									senderr(ERR_NOWRITEACCESS, WARN);	/* send message */
									SetWindowLongPtr(hwnd,DWLP_MSGRESULT,(LONG_PTR)TRUE);		/* bad file */
									return (TRUE);
								}
#endif //PUBLISH
							case FTYPE_INDEX:
							case FTYPE_INDEXV2:
							case FTYPE_TEMPLATE:
									continue;
							case FTYPE_ABBREV:
								if (abbrevtype(ofp->lpstrFile))		/* if good abbrevs */
									continue;
								goto error;
							case FTYPE_STYLESHEET:
								if (styletype(ofp->lpstrFile))		/* if good style file */
									continue;
								goto error;
						}
					}
					if (!fpath)		// if passed all files
						return FALSE;
error:
					senderr(ERR_UNKNOWNFILERR,WARN,file_getname(ofp->lpstrFile));
					SetWindowLongPtr(hwnd, DWLP_MSGRESULT,TRUE);		/* bad file */
					return (TRUE);
				case CDN_TYPECHANGE:
#ifdef PUBLISH
					if (g_admin.permitwrite)	{	/* if writing to index permitted */
#endif //PUBLISH
//						if (ofp->nFilterIndex-1 != FIL_INDEX && ofp->nFilterIndex-1 != FIL_V2INDEX && ofp->nFilterIndex-1 != FIL_DOSINDEX && ofp->nFilterIndex-1 != FIL_ALL)	{
						if (ofp->nFilterIndex < 2)	{
							disableitem(parent,chx1); 	/* disable read only for not'ALL' or 'INDEX' */
							checkitem(parent,chx1,FALSE);
						}
						else
							enableitem(parent,chx1);
#ifdef PUBLISH
					}
#endif //PUBLISH
#if 0
					CommDlg_OpenSave_SetControlText(parent,edt1,g_nullstr);	/* clear text */
#endif
					break;
				case CDN_SHAREVIOLATION:
					if (index_byfspec(((LPOFNOTIFY)lParam)->pszFile))	{	/* if we have it open */
						SetWindowLongPtr(hwnd, DWLP_MSGRESULT,OFN_SHAREFALLTHROUGH);
						return (TRUE);
					}
					else	{		/* opened by somebody else */
						SetWindowLongPtr(hwnd, DWLP_MSGRESULT,OFN_SHAREWARN);
						return (TRUE);
						/** NB: offer read-only option in network version */
					}
			}
			;
		default:
			;
	}
	return (FALSE);
}
/*******************************************************************************/
BOOL file_openstationery(void)	/* opens stationery (Pub Ed) */

{
	TCHAR path[_OFN_NAMELEN], title[_MAX_FNAME+_MAX_EXT], dir[MAX_PATH];

	ofn.hwndOwner = g_hwframe;
	file_getdefpath(dir,FOLDER_STL,TEXT(""));
	*path = '\0';
	*title = '\0';
	ofn.lpstrFilter = sfilters;
	ofn.lpstrFile = path;		/* holds path on return */
	ofn.lpstrInitialDir = dir;
	ofn.lpstrFileTitle = title;	/* holds file title on return */
	ofn.lpstrTitle = TEXT("Open Stationery");
	ofn.lpstrDefExt = file_extensionfortype(FTYPE_STYLESHEET)+1;
	ofn.Flags = OFN_PATHMUSTEXIST|OFN_FILEMUSTEXIST|OFN_EXPLORER|OFN_HIDEREADONLY|OFN_ENABLEHOOK|OFN_NOCHANGEDIR|OFN_ENABLESIZING;
	ofn.lpfnHook = shook;
	ofn.hInstance = NULL;
	ofn.lCustData = 0;
	ofn.lpTemplateName = NULL;
	
	if (GetOpenFileName(&ofn))	{	/* if want to use */
		file_open(path,0);
	}
	return (0);
}
/**********************************************************************************/	
static INT_PTR CALLBACK shook(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)

{
	HWND parent;

	switch (msg)	{
		case WM_INITDIALOG:
			parent = GetParent(hwnd);
			centerwindow(parent,0);
#if 0
			file_setaccess(hwnd,parent,FOLDER_STL);	/* ?? should we implement this */
#endif
			return (TRUE);
	}
	return (FALSE);
}
/**********************************************************************************/	
static TCHAR * findfileatindex(OPENFILENAME * ofn, int index)	// find indexed file from multiple

{
	static TCHAR path[MAX_PATH], *cptr;
	int count;

	if (ofn->nFileOffset > 0 && !ofn->lpstrFile[ofn->nFileOffset-1])	{	// if multiple files
		cptr = ofn->lpstrFile;
		nstrcpy(path, cptr);	// get path
		nstrcat(path, TEXT("\\"));	// terminate it
		cptr += nstrlen(cptr++);
		for (count = 0; count < index && *cptr; cptr += nstrlen(cptr++), count++)
			;
		if (*cptr)	{
			nstrcat(path, cptr);
			return path;
		}
		return NULL;
	}
	else if (!index)	{	// if want only file
		nstrcpy(path, ofn->lpstrFile);
		return path;
	}
	else
		return NULL;
}
/*******************************************************************************/
int file_type(TCHAR * path)	//	 identifies file type from path

{
	TCHAR * extn = PathFindExtension(path);
	int index;

	for (index = 0; index < TYPECOUNT; index++)	{
		if (!nstricmp(extn,filetypes[index].extn))
			return filetypes[index].type;
	}
	return -1;
}
/*******************************************************************************/
TCHAR * file_extensionfortype(int type)	//	 returns extension for type

{
	int index;

	for (index = 0; index < TYPECOUNT; index++)	{
		if (filetypes[index].type == type)
			return filetypes[index].extn;
	}
	return NULL;
}
/**********************************************************************************/	
static short findfiletype(TCHAR * path, TCHAR * filters)	/* finds index of file type selected */

{
	TCHAR *ebase = PathFindExtension(path);
	short ftype;
	TCHAR *fptr;

	for (ftype = 0, fptr = filters; *fptr;fptr += nstrlen(fptr++), ftype++)	{	/* for all filter strings */
		fptr += nstrlen(fptr++);		/* points to extension type */
		if (!nstricmp(ebase,fptr+1))	/* if matched extension */
			return (ftype);
	}
	return -1;
}
/**********************************************************************************/	
long file_archivetype(TCHAR * path)	/* returns true if file is archive */

{
	FILE * fptr;
	TCHAR fbuff[sizeof(long)];

	if (fptr = nfopen(path,TEXT("rb")))	{
		fread(fbuff,1,sizeof(long),fptr);
		fclose(fptr);
		if (*(long*)fbuff == KEY_WIN)
			return KEY_WIN;
		else if (*(long*)fbuff == KEY_MAC)
			return KEY_MAC;
	}
	return 0;
}
/**********************************************************************************/	
static BOOL styletype(TCHAR * path)	/* returns true if file is style file */

{
	FILE * fptr;
	TCHAR fbuff[100];
	STYLESHEET * ss;

	if (fptr = nfopen(path,TEXT("rb")))	{
		fread(fbuff,1,100,fptr);
		fclose(fptr);
		ss = (STYLESHEET *)fbuff;
		if (ss->fg.fsize == sizeof(FORMATPARAMS) && ss->fg.version == FORMVERSION)
			return TRUE;
	}
	return FALSE;
}
/**********************************************************************************/	
static void setcurdir(TCHAR * path)	/* sets current directory */

{
	TCHAR defdir[MAX_PATH];

	nstrcpy(defdir,path);	/* copy path */
	file_trimpath(defdir);
	SetCurrentDirectory(defdir);
}
/**********************************************************************************/	
static long abbrevtype(TCHAR * path)	/* returns true if file is abbrevs */

{
	FILE * fptr;
	TCHAR fbuff[sizeof(long)];

	if (fptr = nfopen(path,TEXT("rb")))	{
		fread(fbuff,1,sizeof(long),fptr);
		fclose(fptr);
		if (*(long*)fbuff == KEY_ABBREV)
			return KEY_ABBREV;
	}
	return 0;
}
/*******************************************************************************/
static int convertpath(TCHAR * path, TCHAR * newpath, int type)	// checks whether file convertible

{
	int code = -1;
	if (sendwarning(WARN_CONVERTINDEX,PathFindFileName(path)))	{	// if want conversion
		nstrcpy(newpath,path);
		PathRemoveExtension(newpath);
		nstrcat(newpath,file_extensionfortype(type));		// NB PathAddExtension doesn't add if name looks like it has extension
		if (!CopyFile(path,newpath,TRUE))		{	// if can't copy (file probably exists)
			if (GetLastError() == ERROR_FILE_EXISTS) {
				if (sendwarning(WARN_OVERWRITEINDEX,PathFindFileName(newpath)))	{	// if don't want overwrite
					if (CopyFile(path,newpath,FALSE))	// if copy now succeeds
						code = 1;
				}
				else
					code = 0;	// didn't want to overwrite
			}
		}
		else	// copied OK
			code = 1;
	}
	else	// didn't want to convert 
		code = 0;
	if (code > 0)	{
		DWORD attr = GetFileAttributes(newpath);
		attr &= ~FILE_ATTRIBUTE_READONLY;
		SetFileAttributes(newpath,attr);	// ensure writable
	}
	return code;
}
/*******************************************************************************/
static BOOL convertdoc(TCHAR * path)	//	 converts doc if necessary; returns path to converted

{
	int type = file_type(path);
	BOOL ok = FALSE;
	TCHAR npath[MAX_PATH];
	int creturn; 

	if (type == FTYPE_INDEXV2 || type == FTYPE_INDEXDOS)	{	// old index
		creturn = convertpath(path,npath,FTYPE_INDEX);
		if (creturn > 0)
			ok = v3_convertindex(npath,type);
		else if (creturn == 0)
			return NO;
	}
	else if (type == FTYPE_TEMPLATEV2)	{		// old template
		creturn = convertpath(path,npath,FTYPE_TEMPLATE);
		if (creturn > 0)
			ok = v3_convertindex(npath,type);
		else if (creturn == 0)
			return NO;
	}
	else if (type == FTYPE_STYLESHEETV2)	{	// old style sheet
		creturn = convertpath(path,npath,FTYPE_STYLESHEET);
		if (creturn > 0)
			ok = v3_convertstylesheet(npath,type);
		else
			return NO;
	}
	else if (type == FTYPE_ABBREVV2)	{	// old abbrevs
		creturn = convertpath(path,npath,FTYPE_ABBREV);
		if (creturn > 0)
			ok = v3_convertabbrev(npath,type);
		else if (creturn == 0)
			return NO;
	}
	else	// current file is ok
		return YES;
	if (ok)	{
		if (type == FTYPE_INDEXV2 || type == FTYPE_INDEXDOS || type == FTYPE_TEMPLATEV2)	{	// if converted index
			char warnings[500];
			if (v3_warnings(warnings))
				sendinfo(INFO_CONVERSIONCHANGES,toNative(warnings));
		}
		nstrcpy(path,npath);
		return YES;
	}
	DeleteFile(npath);			/* remove file */
	senderr(ERR_CONVERSIONERR, WARN);
	return NO;
}
/**********************************************************************************/
BOOL file_open(TCHAR * path, short flags)	// opens file of type

{
	if (convertdoc(path))	{	// if converted or doesn't need conversion
		int type = file_type(path);
			switch (type)	{	/* file type */
				case FTYPE_INDEX:
					return file_openindex(path,flags);
				case FTYPE_ARCHIVE:
				case FTYPE_XMLRECORDS:
					return file_openarchive(path);
				case FTYPE_TEMPLATE:
					return file_opentemplate(path,NULL);
				case FTYPE_ABBREV:
					return abbrev_open(path,TRUE);
				case FTYPE_STYLESHEET:
					return file_openstylesheet(path);
			}
	}
	return FALSE;
}
/**********************************************************************************/	
BOOL file_openindex(TCHAR * path, short openflags)	/* opens and sets up index */

{
	DWORD attrib = GetFileAttributes(path);
	INDEX * FF;
	DWORD accessflags;
	BOOL unreachedinstall = TRUE;	// assume
	
	if (FF = index_byfspec(path))	{		/* if already open */
		if (openflags&OP_VISIBLE)			/* if want to see it */
			SendMessage(g_hwclient,WM_MDIACTIVATE,(WPARAM)FF->cwind,0); /* activate it */
		return (TRUE);
	}
	if (openflags&OP_READONLY || attrib != INVALID_FILE_ATTRIBUTES && (attrib&FILE_ATTRIBUTE_READONLY))
		accessflags = GENERIC_READ;
	else
		accessflags = GENERIC_READ|GENERIC_WRITE;
	if (FF = file_setupindex(path,accessflags,OPEN_EXISTING))	{	/* if can set up */
		if (index_readhead(FF))	{	/* if can read header */
			if (FF->head.headsize == HEADSIZE)	{	// if a legal index
				if (unreachedinstall = installindex(FF,openflags))
					return TRUE;
			}
		}
		index_closefile(FF);	/* clean up after error */
		index_cut(FF);
	}
	if (unreachedinstall)	// didn't reach install
		senderr(ERR_OPENERR, WARN, file_getname(path));	/* error opening or reading header */
	return (FALSE);
}
/*******************************************************************************/
static BOOL installindex(INDEX * FF, short openflags)		// installs index

{
	short tdirty = FF->head.dirty;		/* temp dirty flag, cause flush clears dirty */
	short farray[FONTLIMIT];

	FF->wholerec = FF->head.indexpars.recsize+RECSIZE;	/* set complete record size */
	if (index_setworkingsize(FF,MAPMARGIN))	{
		RECN rtot = (FF->mf.size-FF->head.groupsize-HEADSIZE)/(FF->head.indexpars.recsize+RECSIZE);	// max possible capacity
		int error = index_checkintegrity(FF, rtot);

		if (FF->mf.readonly && (tdirty && !error || error > 0))	{	/* wanting readonly but dirty or damaged */
			sendinfo(INFO_INDEXNEEDSREPAIR);		/* send info */
			return FALSE;
		}
		if (error < 0)	{		// fatal damage to header
			senderr(ERR_FATALDAMAGE, WARN);
			return FALSE;
		}
		if (error > 0) {	// record error(s)
			if (sendwarning(WARN_DAMAGEDINDEX))	{
				RECN mcount;

				if (FF->head.rtot > rtot)	// if claim too many records
					FF->head.rtot = rtot;	// force from size of file
				tdirty = FALSE;
				mcount = index_repair(FF);		// do repairs
				FF->needsresort = TRUE;	// needs resort
				if (mcount)
					sendinfo(INFO_REPAIRMARKED,mcount);
			}
			else
				return FALSE;
		}
		if (FF->head.rtot <= rtot || sendwarning(WARN_MISSINGRECORDS, rtot, FF->head.rtot) && (FF->head.rtot = rtot) && index_writehead(FF))	{
			if (tdirty)		{	/* if badly closed */
				if (sendwarning(WARN_CORRUPTINDEX))		/* if want resort */
					FF->needsresort = TRUE;	// needs resort
				else
					FF->mf.readonly |=TRUE;	/* allow only readonly access */
			}
		}
		if (!grp_checkintegrity(FF))	{	// if corrupt groups
			if (sendwarning(WARN_DAMAGEDGROUPS))	// if want repair
				grp_repair(FF);
			else
				return FALSE;
		}
		col_init(&FF->head.sortpars, FF);		// initialize collator
		FF->startnum = FF->head.rtot;	/* set up initial number of records */
		if (FF->head.formpars.pf.pi.pwidthactual < 100)	// if possible bad settings from index created on Mac
			FF->head.formpars.pf.pi = g_prefs.formpars.pf.pi;	// use default page params
		checkfixes(FF);		/* deal with minor compatibilty fixes */
		grp_buildmenu(FF);			/* build group menu */
		if (FF->needsresort && !FF->mf.readonly)	{	// if need resorted
			sort_resort(FF);
			index_flush(FF);
		}
		if (FF->head.privpars.vmode == VM_SUMMARY)	/* if last used in summary */
			FF->head.privpars.vmode = VM_DRAFT;		/* set up in draft */
		type_findlostfonts(FF);
		if (!type_scanfonts(FF,farray))		// if there are unused fonts
			type_adjustfonts(FF,farray);	// remove them
		if (type_checkfonts(FF->head.fm) || ts_fontsub(FF,NULL))	{	/* if fonts ok or fixed */
			if (openflags&OP_VISIBLE)	{
				if (container_setwindow(FF,TRUE))	{	/* set up view window */
					file_saveprivatebackup(FF);
					view_allrecords(FF->vwind);		/* display records */
					if (FF->mf.readonly)
						sendinfo(INFO_READONLY);
				}
			}
			return TRUE;
		}
	}
	return FALSE;
}
/*******************************************************************************/
static void checkfixes(INDEX * FF)	/* deals with minor version fixes */

{
	if (FF->head.version < CINVERSION)	{
		if (FF->head.version == 300 && FF->head.formpars.ef.lf.sortrefs)	// if sorted refs enabled
			FF->head.formpars.ef.lf.noduplicates = TRUE;			// suppress duplicates
		FF->head.version = CINVERSION;	/* mark it as current version */
		FF->needsresort = TRUE;
	}
	if (FF->head.sortpars.sversion != SORTVERSION)	{
		LANGDESCRIPTOR * locale = col_fixLocaleInfo(&FF->head.sortpars); 	// always check/fix collation params
		
		if (locale)	{	// must be opening index from higher version with more locales
			strcpy(FF->head.sortpars.language, "en");	// default to english
			strcpy(FF->head.sortpars.localeID, "en");
			FF->head.sortpars.nativescriptfirst = TRUE;
		}
		grp_checkparams(FF);	// check/fix params for any groups
		FF->head.sortpars.sversion = SORTVERSION;
		FF->needsresort = TRUE;
	}
	if (!*FF->head.sortpars.substitutes)	// if empty substitutes (from version 3)
		*FF->head.sortpars.substitutes = EOCS;	// set as empty xstring
	FF->head.formpars.fsize = sizeof(FORMATPARAMS);	// these two lines to cover error in v3 before 3.0.2
	FF->head.formpars.version = FORMVERSION;
}
/*******************************************************************************/
BOOL file_openarchive(TCHAR * path)	/* opens file and loads records */

{
	IMPORTPARAMS imp;
	int type;
	INDEX * FF;
	TCHAR indexpath[MAX_PATH];
	
	nstrcpy(indexpath,path);
	PathRemoveExtension(indexpath);
	nstrcat(indexpath,file_extensionfortype(FTYPE_INDEX));		// NB PathAddExtension doesn't add if name looks like it has extension
	if (!PathFileExists(indexpath))	{	// if no index exists with name of archive
		if (!file_createindex(indexpath,&g_prefs.indexpars,NULL))	// if can't create it
			return FALSE;
	}
	else {
		if (type = getarchpars()){	// decide on import to current or create new
			if (type > 0)	{		/* if need to make new index */
				TCHAR prompt[MAX_PATH];

				nstrcpy(prompt,TEXT("New Index from "));
				nstrcat(prompt,file_getname(path));
				if (!startnewdoc(NULL, prompt,newindexfilter,file_getname(path)))
					return (FALSE);
			}
		}
		else
			return FALSE;
	}
	if (FF = index_front())	{	// front index is new or existing
		memset(&imp,0,sizeof(imp));		/* clear imp struct */
		imp.type = file_type(path) == FTYPE_XMLRECORDS ? I_CINXML : I_CINARCHIVE ;		/* set indicator */
		return (imp_loaddata(FF,&imp,path));
	}
	return FALSE;
}
/*******************************************************************************/
static INT_PTR getarchpars(void)	/* gets decision about what to do with archive */

{
	INDEX * FF;

	if (FF = index_front())	/* if there's an active index */
		return DialogBoxParam(g_hinst,MAKEINTRESOURCE(IDD_OPENARCHIVE),g_hwframe,archproc,(LPARAM)FF);
	return (1);	/* must create new index */
}
/******************************************************************************/
static INT_PTR CALLBACK archproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)

{
	int type;

	switch (msg)	{

		case WM_INITDIALOG:
			dprintf(hwnd,IDC_ARCHIVE_INCLUDE,"Include Records in \"%S\"",((INDEX *)lParam)->iname);
			CheckRadioButton(hwnd,IDC_ARCHIVE_INCLUDE,IDC_ARCHIVE_NEW,IDC_ARCHIVE_INCLUDE);
			centerwindow(hwnd,1);
			return FALSE;
		case WM_COMMAND:
			switch (LOWORD(wParam))	{
				case IDOK:
					type = isitemchecked(hwnd,IDC_ARCHIVE_INCLUDE) ? -1 : 1;
				case IDCANCEL:
					EndDialog(hwnd,LOWORD(wParam) == IDOK ? type : FALSE);
					return TRUE;
			}
			break;
		case WM_HELP:
			return (dodialoghelp(hwnd,TEXT("base\\Import_archive.htm"),(HELPINFO *)lParam,wh_archid));
	}
	return FALSE;
}
/******************************************************************************/
BOOL file_openstylesheet(TCHAR * path)	 /* sets up and loads format */

{
	long size;
	HANDLE fid;
	INDEX * FF;
	STYLESHEET *sp;
	FONTMAP * fmp;
	short * sizep;
	short ok;
	
	ok = FALSE;	/* default is error */	
	if ((fid = CreateFile(path,GENERIC_READ,0,NULL,OPEN_EXISTING,FILE_FLAG_RANDOM_ACCESS,0)) != INVALID_HANDLE_VALUE)	{
		if (sp = getmem(sizeof(STYLESHEET)))	{
			if (ok = ReadFile(fid,sp,sizeof(STYLESHEET),&size,NULL))	{	/* if no err reading header */
				if (sp->fg.fsize == sizeof(FORMATPARAMS) && sp->fg.version == FORMVERSION)	{	/* if our version */
					int index;
					for (index = 0; index < FIELDLIM-1; index++)	// !! clear heading font sizes
						sp->fg.ef.field[index].size = 0;	// default size
					if (FF = index_front())	{		/* if an index is open */
						FF->head.formpars = sp->fg;
						fmp = &FF->head.fm[0];
						sizep = &FF->head.privpars.size;
					}
					else	{
						g_prefs.formpars = sp->fg;
						fmp = &g_prefs.gen.fm[0];
						sizep = &g_prefs.privpars.size;

					}
					if (*sp->fm.pname)	{		/* if specified font */
						strcpy(fmp->pname,sp->fm.pname);		/* set preferred */
						if (type_available(sp->fm.pname))		/* if it exists */
							strcpy(fmp->name,sp->fm.pname);	/* set alt */
					}
					if (sp->fontsize)	/* if specified size */
						*sizep = sp->fontsize;
					if (FF)
						view_changedisplaysize(FF->vwind);
					com_settextmenus(FF,ON,ON);	/* update drop-down lists */
				}
				else
					senderr(ERR_INVALSTYLESHEET,WARN);
			}
			freemem(sp);
		}
		CloseHandle(fid);
	}
	if (!ok)
		senderr(ERR_OPENERR, WARN, file_getname(path));	/* error opening or reading stylesheet */
	return (ok);
}
/*******************************************************************************/
BOOL file_opentemplate(TCHAR * path, TCHAR * createpath)		/* opens file and creates index */

{
	BOOL ok = FALSE;	// default is error
	long size;
	HANDLE fid;
	HEAD * hp;
		
	if (hp = getmem(HEADSIZE))	{		/* if can get memory for header */
		if ((fid = CreateFile(path,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_FLAG_RANDOM_ACCESS,0)) != INVALID_HANDLE_VALUE)	{
			ok = ReadFile(fid,hp,HEADSIZE,&size,NULL);		/* if no err reading header */
			CloseHandle(fid);
			if (ok)	{
				HEAD nhp = *hp;

				nhp.rtot = 0;
				nhp.dirty = FALSE;
				nhp.root = 0;
				nhp.elapsed = 0;
				nhp.createtime = hp->squeezetime = time(NULL);
				if (createpath)
					ok = file_createindex(createpath,&nhp.indexpars,&nhp);
				else	{
					TCHAR prompt[MAX_PATH];
					nstrcpy(prompt,TEXT("New Index from "));
					nstrcat(prompt,file_getname(path));
					ok = startnewdoc(&nhp,prompt,newindexfilter,TEXT(""));
				}
			}
		}
		freemem(hp);
	}
	if (!ok)
		senderr(ERR_OPENERR, WARN, file_getname(path));	/* error opening or working with template */
	return (ok);
}
/*******************************************************************************/
BOOL file_resizeindex(INDEX *FF, short newrecsize)	/* resizes open index file */

{
	RECN count;
	RECORD * recptr;
	HCURSOR ocurs;
	
	newrecsize = (newrecsize + 3)&~3; // round up to nearest multiple of 4
	if (newrecsize == FF->head.indexpars.recsize)	/* if no change needed */
		return (TRUE);		/* do nothing */
	if (newrecsize < FF->head.indexpars.recsize || index_setsize(FF,FF->head.rtot,newrecsize,MAPMARGIN))	{	/* if enough space */
		short newwholerec = newrecsize+RECSIZE;

		if (FF->curfile)		/* if viewing a group */
			view_allrecords(FF->vwind);		/* set to show all records */
		UpdateWindow(FF->vwind);		/* force immediate update */
		ocurs = SetCursor(g_waitcurs);
		if (newrecsize < FF->head.indexpars.recsize)	{	/* if reducing size */
			for (count = 1; count <= FF->head.rtot; count++)	{
//				if (recptr = rec_getrec(FF,count))
				if (recptr = getaddress(FF, count))	
					movemappeddata(FF->mf.base+HEADSIZE+(count-1)*newwholerec,recptr,newwholerec);
			}
		}
		else {		/* enlarging size */
			for (count = FF->head.rtot; count > 0; count--)	{
//				if (recptr = rec_getrec(FF,count))
				if (recptr = getaddress(FF, count))
					movemappeddata(FF->mf.base+HEADSIZE+(count-1)*newwholerec,recptr,FF->wholerec);
			}
		}
		FF->head.indexpars.recsize = newrecsize;
		FF->wholerec = newwholerec;
		SetCursor(ocurs);
		return (TRUE);
	}
	else
		senderr(ERR_DISKFULLERR,WARN);
	return (FALSE);
}
/**********************************************************************/
INDEX * file_buildsummary(INDEX * FF)		/* builds summary index */

{
	RECORD * recptr, *newptr;
	RECN rcount;
	INDEX * XF;
	VERIFYGROUP vg;
	short count, crosscount;
	TCHAR tpath[MAX_PATH], path[MAX_PATH];
	HCURSOR ocurs;
	short curview;
	GROUPHANDLE curgroup;
	
	if (!GetTempPath(MAX_PATH,tpath) || !GetTempFileName(tpath,FF->iname,0,path))	/* if can't get temp path */
		return (NULL);
	if (XF = file_setupindex(path,GENERIC_READ|GENERIC_WRITE,CREATE_ALWAYS))	{	/* if can set up */
		index_setdefaults(XF);		/* set up default features */
		XF->head.indexpars.recsize = 24;
		XF->wholerec = XF->head.indexpars.recsize+RECSIZE;	/* set complete record size */
		col_init(&XF->head.sortpars, XF);		// initialize collator
		ocurs = SetCursor(g_waitcurs);
		if (index_setworkingsize(XF,MAPMARGIN))	{	// finish setup
			XF->head.sortpars.type = RAWSORT;
			memset(&vg,0,sizeof(vg));		/* clear verify info */
			vg.lowlim = 1;
			vg.locatoronly = FF->head.refpars.clocatoronly;	// set as necess for locator field only
			if (vg.t1 = getmem(FF->head.indexpars.recsize))		{
				curview = FF->viewtype;			/* save these cause always build summary for whole index */
				curgroup = FF->curfile;
				FF->viewtype = VIEW_ALL;		/* set temp values */
				FF->curfile = NULL;
				for (rcount = 0, recptr = sort_top(FF); recptr ; recptr = sort_skip(FF,recptr,1)) {	   /* for all records */
					showprogress(PRG_SUMMARY,FF->head.rtot,rcount++);
					if (crosscount = search_verify(FF,recptr->rtext,&vg))	{	/* if have cross-ref */
						for (count = 0; count < crosscount; count++)	{
							if (vg.cr[count].num)	{	/* if the target existed */
								char * sptr;
								if (!(newptr = rec_writenew(XF, g_nullrec)))	/* if can't get new record */
									break;
								sptr = newptr->rtext;
								sptr += sprintf(sptr,"%ld$",vg.cr[count].num)+1;	/* target number */
								sptr += sprintf(sptr,"%ld",recptr->num)+1;			/* source number */
								sptr += sprintf(sptr,"%d%c",vg.eoffset, vg.cr[count].matchlevel+'A')+1;	/* length of source body + level in target of match */
								*sptr = EOCS;
								newptr->ismark = vg.cr[count].error || vg.eflags ? TRUE :FALSE;		/* mark if bad ref */
								sort_makenode(XF,newptr->num);		/* make nodes */
							}
						}
					}
				}
				FF->viewtype = curview;		/* restore old view settings */
				FF->curfile = curgroup;
				index_flush(XF);		/* unconditional write */
				showprogress(0,0,0);
				XF->ishidden = TRUE;
				freemem(vg.t1);
				return (XF);
			}
		}
		else
			senderr(ERR_MEMERR,WARN);
		SetCursor(ocurs);
		index_closefile(XF);		/* clean up after error */
		DeleteFile(path);			/* remove file */
		index_cut(XF);
	}
	return (NULL);
}
/**********************************************************************************/	
void file_disposesummary(INDEX * FF)	/* disposes of any summary index */

{
	TCHAR tpath[MAX_PATH];

	if (FF->sumsource)	{			/* if have summary index around */
		nstrcpy(tpath,FF->sumsource->pfspec);	/* save file spec - lost when summary closed */
		index_close(FF->sumsource);	/* close index */
		FF->sumsource = NULL;		/* clear flag */
		FF->head.privpars.vmode = VM_DRAFT;		/* default view mode becomes draft */
		DeleteFile(tpath);			/* remove file */
	}
}
/******************************************************************************/
__int64 file_diskspace(TCHAR * path)	/* returns free on drive containing file */

{
	DWORD secpercluster,bytespersec,freecluster,totcluster;
	TCHAR tpath[MAX_PATH], *eptr;
	__int64 freebytes;
	
	nstrcpy(tpath,path);
	eptr = nstrchr(tpath,'\\');
	*(eptr+1) = '\0';
	if (GetDiskFreeSpace(tpath, &secpercluster,&bytespersec,&freecluster,&totcluster))	/* if can get disk space */
		freebytes = secpercluster*bytespersec*(__int64)freecluster;
	else
		freebytes = 0;
	return (freebytes);
}
/******************************************************************************/
TCHAR * file_getname(TCHAR * path)	/* returns pointer to name part of path */

{
	static TCHAR title[_MAX_FNAME+_MAX_EXT];

	if (GetFileTitle(path,title,_MAX_FNAME+_MAX_EXT) == 0)
		return(title);
	return (TEXT("Invalid Filename"));	// illegal filename
}
/*******************************************************************************/
short file_savestylesheet(HWND hwnd)		/* saves format in file */

{
	TCHAR path[_OFN_NAMELEN], title[_MAX_FNAME+_MAX_EXT], dir[MAX_PATH];
	HANDLE fref;
	long writesize;
	STYLESHEET ss;

	ofn.hwndOwner = g_hwframe;
	file_getdefpath(dir,FOLDER_STL,TEXT(""));
	*path = '\0';
	*title = '\0';
	ofn.lpstrFilter = sfilter;
	ofn.lpstrFile = path;		/* holds path on return */
	ofn.lpstrInitialDir = dir;
	ofn.lpstrFileTitle = title;	/* holds file title on return */
	ofn.lpstrTitle = TEXT("Save Stylesheet");
	ofn.Flags = OFN_OVERWRITEPROMPT|OFN_HIDEREADONLY|OFN_PATHMUSTEXIST
			|OFN_EXPLORER|OFN_ENABLEHOOK|OFN_NOCHANGEDIR|OFN_ENABLESIZING;
	ofn.lpstrDefExt = file_extensionfortype(FTYPE_STYLESHEET)+1;
	ofn.lpfnHook = file_generichook;
	ofn.hInstance = NULL;
	ofn.lCustData = FOLDER_STL;
	ofn.lpTemplateName = NULL;
	
	if (GetSaveFileName(&ofn))	{	/* if want to use */
		if ((fref = CreateFile(path,GENERIC_WRITE,0,NULL,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,0)) != INVALID_HANDLE_VALUE)	{
			if (hwnd)	{
				INDEX * FF = getowner(hwnd);
				ss.fg = FF->head.formpars;
				ss.fm = FF->head.fm[0];		/* set up default stuff */
				ss.fontsize = FF->head.privpars.size;
			}
			else {
				ss.fg = g_prefs.formpars;
				ss.fm = g_prefs.gen.fm[0];
				ss.fontsize = g_prefs.privpars.size;
			}
			WriteFile(fref,&ss,sizeof(STYLESHEET),&writesize,NULL);
			CloseHandle(fref);
			file_saveuserpath(path,ALIS_STYLESHEET);	// save as default path
		}
	}
	return (TRUE);
}
/*******************************************************************************/
BOOL file_saveacopy(HWND hwnd)		/* saves a copy of the active index */

{
	TCHAR path[MAX_PATH], title[_MAX_FNAME+_MAX_EXT];
	OPENFILENAME tfn;
	INDEX * FF = WX(hwnd, owner);

	tfn = ofn;
	tfn.hwndOwner = g_hwframe;
	nstrcpy(path, FF->pfspec);	// get original file path
	*file_getextn(path) = '\0';	/* strip extension */
	nstrcat(path, TEXT(" copy"));	//modify name
	*title = '\0';
	tfn.lpstrFilter = newindexfilter;
	tfn.lpstrFile = path;		/* holds path on return */
	tfn.lpstrFileTitle = title;	/* holds file title on return */
	tfn.lpstrTitle = TEXT("Save Backup Copy");
	tfn.Flags = OFN_OVERWRITEPROMPT|OFN_HIDEREADONLY|OFN_PATHMUSTEXIST
			|OFN_EXPLORER|OFN_ENABLEHOOK|OFN_NOCHANGEDIR|OFN_ENABLESIZING;
	tfn.lpstrDefExt = file_extensionfortype(FTYPE_INDEX)+1;
	tfn.lpfnHook = file_generichook;
	tfn.hInstance = NULL;
	tfn.lCustData = 0;
	tfn.lpTemplateName = NULL;
	if (*f_cpath)		/* if have an initial directory for copy */
		tfn.lpstrInitialDir = f_cpath;		/* set it */

	if (GetSaveFileName(&tfn))	{	/* if want to use */
		nstrcpy(f_cpath,path);			/* save directory */
		file_trimpath(f_cpath);
		if (file_duplicateindex(FF,path))	{	// if can write
			file_setwritetime(path);	// set time stamp
			return TRUE;
		}
	}
	return (FALSE);
}
/*******************************************************************************/
BOOL file_duplicateindex(INDEX * FF, TCHAR * path)	/* copies index on to path */

{
	if (index_flush(FF))	{
		if (CopyFile(FF->pfspec,path,FALSE))	/* if can copy index */
			return (TRUE);
	}
//	err = GetLastError();	// just use for debugging
	senderr(ERR_WRITEERR,WARN, path);
	return (FALSE);
}
/**********************************************************************************/	
void file_setwritetime(TCHAR * path)	// sets current time as file write time

{
	HANDLE hf = CreateFile(path,GENERIC_WRITE,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
	FILETIME ftime;

	if (hf != INVALID_HANDLE_VALUE)	{
		GetSystemTimeAsFileTime(&ftime);
		SetFileTime(hf,NULL,NULL,&ftime);
		CloseHandle(hf);
	}
}
/**********************************************************************************/	
BOOL file_saveprivatebackup(INDEX *FF)	// save private backup of index

{
	if (!*FF->backupspec)	{	// if don't have backup name
		nstrcpy(FF->backupspec,FF->pfspec);
		PathRemoveExtension(FF->backupspec);
		nstrcat(FF->backupspec,TEXT("~~.ucdx"));
	}
	SetFileAttributes(FF->backupspec,FILE_ATTRIBUTE_NORMAL);	// set index hidden/unhidden
	if (file_duplicateindex(FF,FF->backupspec))	{		// if can duplicate
		SetFileAttributes(FF->backupspec,FILE_ATTRIBUTE_HIDDEN);	// set index hidden/unhidden
		return TRUE;
	}
	*FF->backupspec = '\0';
	return FALSE;
}
/**********************************************************************************/	
BOOL file_revertprivatebackup(INDEX *FF)	// reverts to private backup of index

{
	if (FF->backupspec && sendwarning(WARN_REVERT))	{	// if have backup file
		TCHAR spath[MAX_PATH], dpath[MAX_PATH];

		SetFileAttributes(FF->backupspec,FILE_ATTRIBUTE_NORMAL);	// set index hidden/unhidden
		nstrcpy(dpath,FF->pfspec);	// get copy of dest path
		nstrcpy(spath,FF->backupspec);	// get copy of source path
		*FF->backupspec = '\0';		// prevent deletion of backup on close
		g_revertstate = getmmstate(FF->cwind,&g_revertplacement);	// hold appearance info for restoration
		SendMessage(FF->cwind,WM_CLOSE,0,0);	// close our index
		if (ReplaceFile(dpath,spath,NULL,REPLACEFILE_IGNORE_MERGE_ERRORS,0,0))	/* if can replace index */
			return file_openindex(dpath,OP_VISIBLE);
		else 
			senderr(ERR_UNABLETOREVERT,WARN);
	}
	return FALSE;
}
/**********************************************************************************/	
BOOL file_deleteprivatebackup(INDEX *FF)	// deletes private backup of index

{
	if (*FF->backupspec)	{
		if (DeleteFile(FF->backupspec))	// delete index
			*FF->backupspec = '\0';
		return TRUE;
	}
	return FALSE;
}
/**********************************************************************************/	
INT_PTR CALLBACK file_generichook(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)

{
	HWND parent;
	int filetype;

	switch (msg)	{

		case WM_INITDIALOG:
			parent = GetParent(hwnd);
			centerwindow(parent,0);
			ShowWindow(GetDlgItem(parent,stc2),SW_HIDE);	/* hide 'save as type' */
			ShowWindow(GetDlgItem(parent,cmb1),SW_HIDE);	/* hide 'type combo' */
			filetype = ((OPENFILENAME*)lParam)->lCustData;
			file_setaccess(hwnd,parent,filetype);	// disable folder combo as necessary
			return (TRUE);
	}
	return (FALSE);
}
/*******************************************************************************/
HRESULT file_makedatapath(TCHAR * path, TCHAR * file)	// makes path to user data

{
	HRESULT result;
	
	if (SHGetFolderPath(NULL,CSIDL_LOCAL_APPDATA|CSIDL_FLAG_CREATE,NULL,SHGFP_TYPE_CURRENT,path) == S_OK) {
		nstrcat(path,TEXT("\\Indexing Research\\Cindex 4.0"));
		if (GetFileAttributes(path) == INVALID_FILE_ATTRIBUTES)	{	// if directory doesn't exist
			result = SHCreateDirectoryEx(NULL,path,NULL);
			if (result == ERROR_SUCCESS || result == ERROR_CANCELLED) {
				copyuserfiles(CSIDL_LOCAL_APPDATA, path, TEXT("*.udc"));	// first run; copy any user dic from 3.0 installation
				copyuserfiles(CSIDL_LOCAL_APPDATA, path, TEXT("*.cxtg"));	// first run; copy any utag sets from 3.0 installation
			}
			else
				return FALSE;
		}
		return PathAppend(path,file);
	}
	return FALSE;
}
/*******************************************************************************/
HRESULT file_makepublicpath(TCHAR * path, TCHAR * file)	// makes path to public data

{
	HRESULT result;
	
	if (SHGetFolderPath(NULL,CSIDL_COMMON_DOCUMENTS|CSIDL_FLAG_CREATE,NULL,SHGFP_TYPE_CURRENT,path) == S_OK) {
		nstrcat(path,TEXT("\\Indexing Research\\Cindex 4.0"));
		if (GetFileAttributes(path) == INVALID_FILE_ATTRIBUTES) {// if directory doesn't exist
			result = SHCreateDirectoryEx(NULL, path, NULL);
			if (result == ERROR_SUCCESS || result == ERROR_CANCELLED)
				copyuserfiles(CSIDL_COMMON_DOCUMENTS, path,TEXT("*.*"));	// first run; copy any public files from 3.0 installation
			else
				return FALSE;
		}
		return PathAppend(path,file);
	}
	return FALSE;
}
/*******************************************************************************/
BOOL file_getuserpath(TCHAR * path, TCHAR * type)		/* finds path of type */

{
	DWORD size = MAX_PATH;

	if (reg_getkeyvalue(K_PATHS,type,path,&size) && size)	/* if got value from registry */
		return TRUE;
	*path = '\0';
	return FALSE;
}
/*******************************************************************************/
BOOL file_saveuserpath(TCHAR * path, TCHAR * type)		/* saves path of type */

{
	DWORD size = path ? nstrlen(path)+1 : 0;
	if (reg_setkeyvalue(K_PATHS,type,REG_SZ,path,size))	/* if can save value */
		return TRUE;
	return FALSE;
}
/*******************************************************************************/
void file_trimpath(TCHAR * path)		/* trims name from path */

{
	TCHAR * nameptr;

	if (nameptr = nstrrchr(path,'\\'))
		*nameptr = '\0';			/* trim to path */
}
#ifdef PUBLISH
/*******************************************************************************/
BOOL file_getmachinepath(TCHAR * path, TCHAR * type)		/* finds path of type */

{
	DWORD size = MAX_PATH;

	if (reg_getmachinekeyvalue(K_PATHS,type,path,&size) && size)	/* if got value from registry */
		return TRUE;
	return FALSE;
}
/*******************************************************************************/
BOOL file_savemachinepath(TCHAR * path, TCHAR * type)		/* saves path of type */

{
	DWORD size = path ? nstrlen(path)+1 : 0;

	if (reg_setmachinekeyvalue(K_PATHS,type,REG_SZ,path,size))	/* if can save value */
		return TRUE;
	return FALSE;
}
#endif //PUBLISH
/*******************************************************************************/
void file_setaccess(HWND hwnd, HWND parent, int id)	/* sets directory/access */

{
#ifdef PUBLISH
	if (g_admin.access[id].changefolder)	/* if can change folder */
		enableitem(parent,cmb2);
	else
		disableitem(parent,cmb2);
#endif
}
/*******************************************************************************/
BOOL file_getdefpath(TCHAR * path, int id, TCHAR * file)	/* makes default path for file type */

{
	int ok = FALSE;

	*path = '\0';	
#ifdef PUBLISH
	if (g_admin.access[id].opendef || id == FOLDER_PREF)	{	/* if want to set default folder */
		switch (id)	{
			case FOLDER_PREF:
//				ok = ExpandEnvironmentStrings("%HOMEPATH%",path,MAX_PATH) && strcmp(path,"\\") && strcmp(path,hp) && GetFileAttributes(path) != INVALID_FILE_ATTRIBUTES;
				ok = file_makedatapath(path,TEXT(""));	// get path to prefs
//				senderr(ERR_BADDATAFOLDER, WARN);
				break;
			case FOLDER_CDX:	// default index directory
				ok = file_getmachinepath(path,ALIS_CDXDIR);
				break;
			case FOLDER_STL:	// style directory
				ok = file_getmachinepath(path,ALIS_STLDIR);
				break;
			case FOLDER_ABR:	// abbrev directory
				ok = file_getmachinepath(path,ALIS_ABRDIR);
				break;
			case FOLDER_CTG:	// tagset directory
				if (!(ok = file_getmachinepath(path,ALIS_CTGDIR)))	// if no path set
					ok = file_makedatapath(path,TEXT(""));
				break;
			case FOLDER_UDC:	// user dic folder
				if (!(ok = file_getmachinepath(path,ALIS_UDCDIR)))	// if no path set
					ok = file_makedatapath(path,TEXT(""));
				break;
		}
	}
	else 
#endif
	{		// default folders for standard, or publish if no preferred spec
		switch (id)	{
			case FOLDER_PREF:
				ok = file_makedatapath(path,TEXT(""));	// get path to prefs
//				senderr(ERR_BADDATAFOLDER, WARN);
				break;
			case FOLDER_CDX:	// default index directory
//				ok = file_getuserpath(path,ALIS_DEFAULTDIR);
				ok = GetCurrentDirectory(MAX_PATH,path);
				break;
			case FOLDER_STL:	// style directory
				ok = file_getuserpath(path,ALIS_STYLESHEET);
				break;
			case FOLDER_ABR:	// abbrev directory
				ok = file_getuserpath(path,ALIS_ABBREV);
				break;
			case FOLDER_CTG:	// tagset directory
				ok = file_makedatapath(path,TEXT(""));
				break;
			case FOLDER_UDC:	// user dic folder
				ok = file_makedatapath(path,TEXT(""));
				break;
		}
	}
	if (ok)	// if have path
		return PathAppend(path,file);
	return (ok);
}
