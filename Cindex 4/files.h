#pragma once

enum {			/* open flags for index */
	OP_VISIBLE = 1,
	OP_READONLY = 2,
	OP_CONVERT = 4
};

enum {
	KEY_WIN = 0x01010101,		/* Windows archive key */
	KEY_MAC =  0x02020202,		/* Mac archive key */
	KEY_ABBREV = 0x41414141		/* abbreviation file key ("AAAA") */
};

enum {				// subtype keys
	TEXTKEY_NATIVE = 0,
	TEXTKEY_UTF8 = 4,
};

enum {			/* default folder access ids */
	FOLDER_CDX,
	FOLDER_STL,
	FOLDER_ABR,
	FOLDER_CTG,
	FOLDER_UDC,
	FOLDER_PREF,
	FOLDER_DEFAULT
};

enum {
	FTYPE_UNKNOWN = -1,
	FTYPE_INDEX,
	FTYPE_INDEXV2,
	FTYPE_INDEXDOS,
	FTYPE_TEMPLATE,
	FTYPE_TEMPLATEV2,
	FTYPE_STYLESHEET,
	FTYPE_STYLESHEETV2,
	FTYPE_ABBREV,
	FTYPE_ABBREVV2,
	FTYPE_ARCHIVE,
	FTYPE_XMLRECORDS,
	FTYPE_PLAINTEXT,
//	FTYPE_TABTEXT,
	FTYPE_MACREX,
	FTYPE_DOSDATA,
	FTYPE_SKY,
	FTYPE_SKY7,
	FTYPE_SKY8
};

#define _OFN_NAMELEN (4*MAX_PATH)
extern OPENFILENAME ofn;
extern TCHAR newindexfilter[];

#define file_getextn(A) (nstrrchr((A),'.'))

HRESULT file_makedatapath(TCHAR * path, TCHAR * file);	// makes path to user data
HRESULT file_makepublicpath(TCHAR * path, TCHAR * file);	// makes path to public data
short file_loadconfig(short noload);		/* loads configuration variables */
void file_saveconfig(void);		/* saves configuration variables */
void file_makecinpath(TCHAR *path, TCHAR * file);	/* puts file in path to cindex directory */
void file_maketemppath(TCHAR *path, TCHAR * file);	/* puts file in path to temp directory */
BOOL file_newabbrev(void);	/* sets up new abbreviation file */
long file_openabbrev(void);	/* attaches abbreviation */
BOOL file_open(TCHAR * path, short flags);	// opens file of type
BOOL file_openindex(TCHAR * path, short openflags);		/* opens file and loads index */
BOOL file_openarchive(TCHAR * path);	/* opens file and loads records */
BOOL file_opentemplate(TCHAR * path, TCHAR * createpath);		/* opens file and creates index */
BOOL file_createindex(TCHAR * path, INDEXPARAMS * ip, HEAD * hp);	/* starts new index */
BOOL file_openstylesheet(TCHAR * path);	 /* sets up and loads format */
int file_type(TCHAR * path);		//	 identifies file type from path
TCHAR * file_extensionfortype(int type);	//	 identifies file type from path
long file_archivetype(TCHAR * path);	/* returns true if file is archive */
BOOL file_resizeindex(INDEX * FF, short newrecsize);	/* resizes open index file */
INDEX * file_buildsummary(INDEX * FF);		/* builds summary index */
void file_disposesummary(INDEX * FF);	/* disposes of any summary index */
__int64 file_diskspace(TCHAR * path);	/* returns free on drive containing file */
TCHAR * file_getname(TCHAR * path);	/* returns pointer to name part of path */
INDEX * file_setupindex(TCHAR * path, DWORD accessflags, DWORD flags);	/* sets up new index structures; opens file */
short file_savestylesheet(HWND hwnd);		/* saves format in file */
BOOL file_saveacopy(HWND hwnd);		/* saves a copy of the active index */
BOOL file_duplicateindex(INDEX * FF, TCHAR * path);	/* copies index on to path */
void file_setwritetime(TCHAR * path);	// sets current time as file write time
BOOL file_getuserpath(TCHAR * path, TCHAR * type);		/* finds path of type */
BOOL file_saveuserpath(TCHAR * path, TCHAR * type);		/* saves path of type */
void file_trimpath(TCHAR * path);		/* trims name from path */
INT_PTR CALLBACK file_generichook(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

BOOL file_saveprivatebackup(INDEX *FF);	// save private backup of index
BOOL file_revertprivatebackup(INDEX *FF);	// reverts to private backup of index
BOOL file_deleteprivatebackup(INDEX *FF);	// deletes private backup of index

#ifdef PUBLISH
BOOL file_openstationery(void);	/* opens stationery */
BOOL file_getmachinepath(TCHAR * path, TCHAR * type);		/* finds path of type */
BOOL file_savemachinepath(TCHAR * path, TCHAR * type);		/* saves path of type */
#endif //PUBLISH
void file_setaccess(HWND hwnd, HWND parent, int id);	/* sets directory/access */
BOOL file_getdefpath(TCHAR * path, int id, TCHAR * file);	/* sets sets default path */
