// CINDEX Application Programming Interface
// Copyright (c) Indexing Research 2002-2014
// All Rights Reserved.

// Version 3.2

#ifdef CINDEXAPI_EXPORTS
#define CINDEXAPI_API __declspec(dllexport)
#else
#define CINDEXAPI_API __declspec(dllimport)
#endif

#define MAXRECORDFIELDS 16
#define FONTNAMELENGTH 32
#define MAXFONTS 32
#define SEARCHLENGTH 250

typedef struct {
	UINT32 reserved0;
	UINT32 reserved1;
	UINT32 totalRecords;
	UINT32 recordLength;
} CINDEXRECORDINFO;

typedef struct {
	UINT32 reserved0;
	UINT32 reserved1;
	UINT32 version;
	UINT32 reserved2;
} CINDEXVERSIONINFO;

typedef struct {
	UINT32 reserved0;
	UINT32 reserved1;
	char language[4];
	UINT32 sortType;
	UINT32 fieldOrder[MAXRECORDFIELDS];
	UINT32 sortActive;
} CINDEXSORTINFO;

typedef struct {
	UINT32 reserved0;
	UINT32 reserved1;
	char name[FONTNAMELENGTH];		// name of font used if preferred unavailable
	char pname[FONTNAMELENGTH];		// name of preferred font
} CINDEXFONT;

typedef struct {
	UINT32 reserved0;
	UINT32 reserved1;
	UINT32 totalFonts;
	CINDEXFONT fontArray[MAXFONTS];	// name of font used 
} CINDEXFONTINFO;

// FIND/REPLACE
//  'excludeForAttributes' causes serach to exclude records with the any specified attributes...
//			findInNew, findInMarked, etc
//	'not' causes search to find cases that fail the match
//  'field' specifies a record field (0 = main heading) or a special constant from enum
//  'isPattern' etc are TRUE/FALSE flags
//	'caseSensitive' 'wordSensitive' are valid only when 'isPattern' is FALSE 
//  'evaluateRefs' is valid only when 'field' is PAGEFIELD and when 'isPattern' is FALSE 
//  'firstDate' and lastDate' specify time in seconds from Jan 1 1970.
//  'replaceText' may be an empty string, in which case the found text is removed

//REPLACE
//	If 'evaluateRefs' is TRUE, the call to CINAPI_REPLACEALL will fail
//  If 'searchText' is an empty string a call to CINAPI_REPLACEALL will fail

enum	{	// field selection specifiers 
	ALLRECORDFIELDS = -3,
	ALLBUTPAGEFIELD = -2,
	LASTTEXTFIELD = -1,
	PAGEFIELD = (MAXRECORDFIELDS-1)
};

typedef struct {	
	UINT32 reserved0;
	UINT32 excludeForAttributes;	// excludes records for which attributes are specified
	BOOL not;				// find cases that fail match
	char searchText[SEARCHLENGTH];// text to search for
	char userId[4];			// user id [up to four characters];
	UINT32 field;			// record field in which to search (-3, all; -2, last text; -1, locator; ...)
	UINT32 isPattern;		// search for a pattern (regular expression)
	UINT32 caseSensitive;	// match is case sensitive
	UINT32 wordSensitive;	// match only whole words
	UINT32 evaluateRefs;	// evaluate page references
	UINT32 findInNew;		// include/exclude new records
	UINT32 findInMarked;	// include/exclude marked records
	UINT32 findInDeleted;	// include/exclude deleted records
	UINT32 findInGenerated;	// include/exclude generated records
	UINT32 findInLabeled;	// include/exclude labeled records [HIWORD specifies label ID]
	INT32 firstDate;		// first date
	INT32 lastDate;			// last date
	char replaceText[SEARCHLENGTH];// replacement text
} CINDEXFINDREPLACE;

// API commands

#define	CINAPI_QUIT		"quitcindex"
#define CINAPI_CREATE	"createindex"
#define CINAPI_OPEN		"open"
#define CINAPI_CLOSE	"closeindex"
#define CINAPI_SAVE		"save"
#define CINAPI_SAVEBACKUP	"savebackup"
#define CINAPI_SAVEAS	"saveas"
#define CINAPI_IMPORT	"import"
#define CINAPI_PRINT	"print"
#define CINAPI_SELECTALL	"selectall"
#define CINAPI_DELETE	"delete"
#define CINAPI_COMPRESS	"compress"
#define CINAPI_EXPAND	"expand"	// supported in dll version >= 2
#define CINAPI_SORT		"sort"
#define CINAPI_VIEW		"view"
#define CINAPI_GOTO		"goto"
#define CINAPI_SETSIZE	"setsize"
#define CINAPI_SHOWINDEX	"showindex"
#define CINAPI_RECONCILE	"reconcile"
#define CINAPI_GENERATE	"generate"
#define CINAPI_ALTER	"alter"
#define CINAPI_LABEL	"label"
		// commands that pass data
#define CINAPI_FINDALL	"findall"
#define CINAPI_REPLACEALL	"replaceall"
#define CINAPI_ADDRECORD	"addrecord"
		// commands that return data
#define CINAPI_GETRECORDINFO	"getrecordinfo"
#define CINAPI_GETSORTINFO	"getsortinfo"
#define CINAPI_GETFONTINFO	"getfontinfo"
#define CINAPI_GETSELECTEDRECORD	"getselectedrecord"
#define CINAPI_GETNEXTRECORD	"getnextrecord"
#define CINAPI_GETVERSIONINFO	"getversion"	// supported in dll version >= 2

// command arguments

// SORT

enum {		//  Cindex V1 definitions of sort types
	V1_RAWSORTTYPE,
	V1_WORDSORTTYPE,
	V1_LETTERSORTTYPE
};
enum {		//  Cindex V2 definitions of sort types
	V2_RAWSORT,
	V2_LETTERSORT,
	V2_LETTERSORT_CMS,
	V2_LETTERSORT_ISO,
	V2_WORDSORT,
	V2_WORDSORT_CMS,
	V2_WORDSORT_ISO,
};

enum {		/*  V3 definitions of sort types */
	RAWSORT,
	LETTERSORT,
	LETTERSORT_CMS,
	LETTERSORT_ISO,
	WORDSORT,
	WORDSORT_CMS,
	WORDSORT_ISO,
	LETTERSORT_SBL,
	WORDSORT_SBL
};

#define CINAPI_SORTALPHA "alpha"
#define CINAPI_SORTPAGE "page"
#define CINAPI_SORTON "on"
#define CINAPI_SORTOFF "off"

// VIEW
#define CINAPI_VIEWFORMATTED "formatted"
#define CINAPI_VIEWDRAFT "draft"
#define CINAPI_VIEWALL "all"

// OPEN
#define CINAPI_OPENINDEX "index"
#define CINAPI_OPENRECORDS "records"
#define CINAPI_OPENSTYLESHEET "stylesheet"

// SAVEAS
#define CINAPI_SAVEASINDEX "index"
#define CINAPI_SAVEASXMLRECORDS "xmlrecords"	// api version 3.2
#define CINAPI_SAVEASARCHIVE "archive"
#define CINAPI_SAVEASRTF "richtext"

// API ERRORS
enum {
	CINAPI_ERR_UNSPECIFIED = 0,
	CINAPI_ERR_UNKNOWNCOMMAND = -1,		// unknown command
	CINAPI_ERR_BADARGUMENT = -2,		// invalid argument
	CINAPI_ERR_NUMARGUMENTS = -3,		// wrong number of arguments to command
	CINAPI_ERR_NOSERVER = -4,			// no connection to server
	CINAPI_ERR_TRANSACTIONFAILED = -5,	// server couldn't complete transaction
	CINAPI_ERR_NOCONVERSATION = -6	// server couldn't start conversation; probably mismatched vsersions of API
};

// API interface

CINDEXAPI_API INT32 cindex_getversion(void);	//  returns version number of api 
CINDEXAPI_API INT32 cindex_connect(void);	//  connects to running CINDEX 
CINDEXAPI_API INT32 cindex_disconnect(void);	//  disconnects from CINDEX
CINDEXAPI_API INT32 cindex_command(char * command, char * arg1, char * arg2);	//  sends command to CINDEX 
CINDEXAPI_API INT32 cindex_getdata(char * command, void *data);	// obtains data from CDINEX

