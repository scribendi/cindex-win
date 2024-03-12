#pragma once

#include "expat.h"
#include "translate.h"

enum	{		/* import file types */
	I_CINARCHIVE,
	I_CINXML,		// xml records
	I_PLAINTAB,		// tab delimited text
	I_DOSDATA,
	I_MACREX,
	I_SKY,
};


enum {		/* record formation errors */
	BADCHAR = -10,	/* illegal character */
	TOOLONGFORINDEX,		/* exceeds max record size */
	BADMACREX,		/* macrex record parsing error */
	BADDELIMIT,		/* missing delimiter at start or end of line */
	TOOMANYFIELDS,	/* too many fields */
	TOOLONGFORRECORD,	/* exceeds record size */
	MISSINGFONT,		// uses missing font
	EMPTYRECORD			// record would be empty
};

enum {
	SKYTYPE_7,
	SKYTYPE_8
}; 

enum {
	PM_SCAN = 0,
	PM_READ = 1
};

struct rerr {		/* read error structure */
	short type;
	long line;
};
#define IMP_MAXERRBUFF 200
#define ARCHIVEOFFSET 16	/* offset from start of file for reading archive records */


typedef struct {
	XML_Parser parser;
	int activefont;		// id of active font
	int activefield;	// index of active field
	char * destination;	// where to put character data
	char * limit;		// limit char position
	int errorline;	// line number of element unknown
	BOOL overflow;		// TRUE when element value overflow
	BOOL collect;		// can collect character data
	int textfont;		// currrent font info
	char textcode;		// current style info
	char textcolor;		// current color info
	BOOL inindex;		// is inside the index
	BOOL infonts;		// gathering font info
	BOOL inrecords;		// gathering records
	BOOL fontsOK;		// got good font info;
	RECN activerecord;	// record being parsed
	int error;			// error id
	BOOL protectedchar;	// protectedchar
} PARSERDATA;

typedef struct {		/* import structure */
	int mode;			// parser mode
	RECN recordcount;		/* linecount */
	short xflags;		/* control flags */
	BOOL conflictingseparators;	// import specifies conflicting locator and/or xref separators
	struct ghstruct gh;	/* for translation of DOS gh codes */
	long type;			/* file type */
	long subtype;		/* distinguishes subtypes */
	long skytype;		// distinguishes sky subtypes
	BOOL delimited;		// true if quote-delimited
	struct rerr errlist[IMP_MAXERRBUFF];	/* record error info */
	RECN ecount;		/* error count */
	RECN emptyerrcnt;	/* count of empty rec errors */
	RECN lenerrcnt;		/* count of length errors */
	RECN fielderrcnt;	/* number of records with too many fields */
	RECN fonterrcnt;	// number of records with font errors
	RECN markcount;		/* number of marked records */
	char sepstr[4];		/* string for delimited separators */
	__int64 freespace;	/* space available to expand index */
	short longest;		/* longest record scanned */
	short deepest;		/* # fields in deepest record scanned */
	FONTMAP tfm[FONTLIMIT];	/* font map for archive */
	short farray[FONTLIMIT];	/* for remapping import fonts */
	char buffer[MAXREC+2];	// record buffer
	INDEX * FF;
	PARSERDATA pdata;		// parser data
	RECORD prec;		// temporary record
} IMPORTPARAMS;


extern char * r_err[];
extern FILE * e_fptr;

short imp_loaddata(INDEX * FF, IMPORTPARAMS * imp, TCHAR *path);	/* opens & reads import file */
BOOL imp_findtexttype(IMPORTPARAMS * imp, char * data, unsigned long datalength);
short imp_checkline(INDEX * FF, char * fbuff, IMPORTPARAMS * imp, RECORD * recptr);	/* parses and checks input line */
int imp_resolveerrors (INDEX *FF, IMPORTPARAMS * imp);	/* resolves (or not) import errors */
BOOL imp_adderror(IMPORTPARAMS *imp, int type, int line);		// adds a new error to the list
