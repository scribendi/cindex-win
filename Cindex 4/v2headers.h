
#pragma once
#define V2STSTRING 256		/* basic standard string */
#define V2FTSTRING 128
#define V2FSSTRING 32
#define V2FMSTRING 16
#define V2STYLESTRINGLEN 256

#define V2COMPMAX 10

/************************************************************/

#define V2FONTNAMELEN 32

typedef struct {	/* maps font name to local ID */
	char name[V2FONTNAMELEN];	/* alt (working) font name */
	char pname[V2FONTNAMELEN];	/* name of preferred font */
	short flags;	
} V2FONTMAP;

/************************************************************/
//indexparams.h

#define V2FNAMELEN 12		/* max length of field name */
#define V2PATTERNLEN 64

typedef struct {		/* specs of field */
	char name[V2FNAMELEN];	/* name */
	unsigned short minlength;		/* minimum */
	unsigned short maxlength;		/* max */
	char matchtext[V2PATTERNLEN];/* field must match this pattern */
	long spare[4];			/* !!spare */
} V2FIELDPARAMS;

typedef struct 	{		/* index structure */
	unsigned short recsize;			/* record size */
	unsigned short minfields;		/* min fields required in record */
	unsigned short maxfields;		/* max fields allowed in record */
	V2FIELDPARAMS field[FIELDLIM];		/* field information */
	short required;		// last text field is required
	short sspare;		// spare
	long spare[3];		// spare
} V2INDEXPARAMS;

/************************************************************/
//privateparams

#define V2FLAGLIMIT 8		/* max # of flags*/

typedef struct {		// properties of display filter
	unsigned char label[V2FLAGLIMIT];	// labels (coded by bit positions)
	unsigned char on;	// filtering enabled
	unsigned char spare[3];
} V2DISPLAYFILTER;

typedef struct	{		/* general config parameters to remember silently */
	char vmode;				/* full format/ draft flag */
	char wrap;				/* line wrap flag */
	char shownum;			/* show record numbers */
	char hidedelete;		/* hide deleted records */
	short hidebelow;		/* hide headings below this level */
	short size;				/* default font size */
	RECT rwrect;			/* size/position of editing window */
	char eunit;				/* unit in which measurements are expressed */
	char filterenabled;		// display filtering enabled
	V2DISPLAYFILTER filter;	// display filter
	long spare[3];			/* !!spare */
} V2PRIVATEPARAMS;

/************************************************************/
//formatparams.h

typedef struct {	/* character style/position structure */
	short style;	/* character style */
	short cap;		/* capitalization type */
	char allowauto;	// auto cap allowed
	char spare;	/* !!spare */
} V2CSTYLE;

typedef struct {	/* locator segment style structure */
	V2CSTYLE loc;		/* locator itself */
	V2CSTYLE punct;	/* trailing punctuation */
	long spare;		/* !!spare */
} V2LSTYLE;

typedef struct	{		/* margins & columns */
	unsigned short top;
	unsigned short bottom;
	unsigned short left;
	unsigned short right;
	unsigned short ncols;
	unsigned short gutter;
	short reflect;
	short pgcont;		/* repeat broken head after break */
	char continued[V2FSSTRING];	/* 'continued' text */
	V2CSTYLE cstyle;
	short clevel;		/* level down to which want continuation */
	long spare[4];		/* !!spare */
} V2MARGINCOLUMN;

typedef struct	{	/* header & footer content */
	char left[V2FTSTRING];
	char center[V2FTSTRING];
	char right [V2FTSTRING];
	V2CSTYLE hfstyle;		/* style */
	char hffont[V2FSSTRING];		/* font */
	short size;			/* size */
	short sspare;		/* !! spare */
	long spare[3];		/* !!spare */
} V2HEADERFOOTER;

typedef struct {	/* for the moment just a placeholder */
	short porien;			/* paper orientation */
	short psize;			/* encoded paper size */
	short pwidth;			/* override width in 1/10 mm */
	short pheight;			/* override length in 1/10 mm */
	short pwidthactual;		/* width in points */
	short pheightactual;	/* height in points */
	short xoffset;			/* offset from edge to printable area */
	short yoffset;			/* offset from top to printable area */
	long spare[4];
} V2PAPERINFO;

typedef struct {		/*	page format */
	V2MARGINCOLUMN mc;
	V2HEADERFOOTER lefthead;
	V2HEADERFOOTER leftfoot;
	V2HEADERFOOTER righthead;
	V2HEADERFOOTER rightfoot;
	short linespace;		/* line spacing (single, double, etc.) */
	short firstpage;
	short lineheight;		/* line height */
	short entryspace;		/* space between entries*/
	short above;			/* space above group header */
	char lineunit;			/* unit of measurement for vertical spacing */
	char autospace;			/* TRUE if line-spacing derived automatically */
	char dateformat;		/* date format index */
	char timeflag;			/* TRUE when to add time to date */
	V2PAPERINFO pi;			/* paper info */
	char numformat;			/* page numbering format */
	char orientation;		// 0 portrait; 1 landscape
	char cspare[2];			/* !!spare */
	long spare[3];			/* !!spare */
} V2PAGEFORMAT;

typedef struct {			/* cross-ref punctuation structure */
	char cleada[V2FMSTRING];	/* lead text for open ref (see also) */
	char cenda[V2FMSTRING];	/* end text */
	char cleadb[V2FMSTRING];	/* lead text for blind ref (see) */
	char cendb[V2FMSTRING];	/* end text */
} V2CROSSPUNCT;

typedef struct {			/* cross-reference formatting */
	V2CROSSPUNCT level[2];	/* array of 2 punctuation structures (head & subhead) */
	V2CSTYLE leadstyle;		/* lead style */
	V2CSTYLE bodystyle;		/* body style */
	char subposition;		/* how subhead 'see also' placed in entry */
	char mainposition;		/* how main head 'see also' placed in entry */
	char sortcross;			/* TRUE: arrange refs */
	char suppressall;		/* suppress all cross refs */
	char subseeposition;	/* how subhead 'see' placed in entry */
	char mainseeposition;	/* how main head 'see' placed in entry */
	char spare[14];			/* !!spare */
} V2CROSSREFFORMAT;

typedef struct	{		/* locator formatting */
	char sortrefs;			/* TRUE: arrange refs */
	char rjust;				/* right justify */
	char suppressall;		/* suppress whole ref */
	char suppressparts;		/* suppress repeated parts */
	char llead1[V2FMSTRING];	/* lead text for single ref */
	char lleadm[V2FMSTRING];	/* lead text for multiple refs */
	char trail[V2FMSTRING];	/* trailing text for refs */
	char connect[V2FMSTRING];	/* connecting sequence */
	short conflate;			/* threshold for conflation */
	short abbrevrule;		/* abbreviation rule */
	char suppress[V2FMSTRING];	/* suppress to last of these chars */
	char concatenate[V2FMSTRING];	/* concatenate with this sequence */
	V2LSTYLE lstyle[V2COMPMAX];	/* segment style sequence */
	char leader;			/* leader type */
	char sparex[3];	
	long spare[3];			/* !!spare */
} V2LOCATORFORMAT;

typedef struct	{	/* grouping of entries */
	short method;		/* grouping method */
	char gfont[V2FSSTRING];	/* font */
	V2CSTYLE gstyle;		/* style */
	short gsize;		/* text size */
	char title[V2FSSTRING];	/* title/format string */
	long spare[4];			/* !!spare */
} V2GROUPFORMAT;

typedef struct	{	/* individual field layout */
	char font[V2FSSTRING];	/* font */
	short size;			/* size */
	V2CSTYLE style;		/* style */
	float leadindent;	/* first line indent (points) */
	float runindent;	/* runover indent (points) */
	char trailtext[V2FMSTRING];	/* trailing text for when no ref */
	long flags;
	char leadtext[V2FMSTRING-4];	/* leading text */
} V2FIELDFORMAT;

typedef struct	{		/* overall heading layout */
	short runlevel;			/* runon level */
	short collapselevel;	/* level at which entries collapsed */
	char style;				/* index style modifiers */
	char itype;				/* indentation type (auto, etc) */
	char adjustpunct;		/* adjust punctuation */
	char adjstyles;			/* adjust style codes around punctuation */
	char fixedunit;			/* unit for fixed spacing (0 is em) */
	char autounit;			/* unit for auto spacing (0 is em) */
	float autolead;			/* space for auto lead */
	float autorun;			/* space for autorunin */
	V2GROUPFORMAT eg;
	V2CROSSREFFORMAT cf;
	V2LOCATORFORMAT lf;
	V2FIELDFORMAT field[FIELDLIM-1];	/* field info */
	long spare[8];			/* !!spare */
} V2ENTRYFORMAT;

typedef struct	{		/* overall format structure */
	unsigned long fsize;	/* size of structure */
	unsigned short version;	/* version number */
	V2PAGEFORMAT pf; 
	V2ENTRYFORMAT ef;
	long spare[32];			/* !!spare */
} V2FORMATPARAMS;

typedef struct {		/* stylesheet */
	V2FORMATPARAMS fg;
	V2FONTMAP fm;
	short fontsize;
	long spare[16];
} V2STYLESHEET;

/***************************************************************/
//printformat.h

typedef struct 	{	/* page formatting structure */
	long first;			/* first page to produce */
	long last;			/* last page to produce */
	short oddeven;		/* flags for odd even pages */
	RECN firstrec;		/* record to start at */
	RECN lastrec;		/* last record */
	short pagenum;		/* number given starting page */ 
	long uniquemain;	// number of unique main headings
	long entries;		/* # entries produced */
	long prefs;			/* page refs produced */
	long crefs;			/* # cross refs */
	long lines;			/* # lines */
	short pageout;		/* number of pages that would be output */
	short lastpage;		/* number of last page formatted */
	RECN rnum;			/* record at top of first page */
	RECN lastrnum;		// record at bottom of last page
	short offset;		/* # lines into record */
	short labelmark;	/* print bullet on labeled records */
} V2PRINTFORMAT;
/***************************************************************/
//refernceparams.h

typedef struct {		/* reference parameters */
	char crosstart[V2STSTRING];	/* words that can begin cross-refs */
	char crossexclude[V2FTSTRING];	/* word that begin general cross ref */
	char maxvalue[V2FTSTRING];	/* highest valued page reference */
	char csep;			/* cross-ref separator */
	char psep;			/* page ref separator */
	char rsep;			/* page ref connector */
	unsigned char clocatoronly;	// TRUE when recognizing cross-refs in subheadings
	long maxspan;		/* maximum span allowed in range */
	long spare[3];		/* !!spare */
} V2REFPARAMS;
/***************************************************************/
// recordparams.h
typedef struct {
	RECN num;		/* holds record number */
	RECN parent;		/* number of parent record */
	RECN lchild;		/* number of left child record */
	RECN rchild;		/* number of right child record */
	time_c time;		/* time of last change */
	char user[4];		/* up to four initials of user who last changed */
	unsigned short ismod : 1;	// unused
	unsigned short isdel : 1;	/* TRUE if record deleted */
	unsigned short ismark : 1;	/* TRUE if record marked */
	unsigned short islock : 1;	// unused
	unsigned short balance : 2;	/* holds balance of subtree on record */
	unsigned short isgen : 1;	/* TRUE if autogenerated */
	unsigned short isttag : 1;	/* TRUE if temporarily tagged */
	unsigned short label : 3;	/* holds tag ID  */
	char rtext[0];				/* text of record */
} V2RECORD;

#define V2RECSIZE (sizeof(V2RECORD))
/***************************************************************/
//sortparams.h

#define V2PREFIXLEN 256

typedef struct {		/* struct for sort pars */
	char type;			/* sort type */
	short fieldorder[FIELDLIM+1];		/* order of fields */
	short charpri[CHARPRI+1];	/* character priority */
	char ignorepunct;	/* ignore l-by-l punctuation */
	char ignoreslash;	/* ignore /- in word sort */
	char ignoreparen;	/* ignore text in parens */
	char evalnums;		/* evaluate numbers */
	char crossfirst;	/* cross-refs first [UNUSED] */
	char ignore[V2PREFIXLEN];	/* ignored leading words in subheads */
	char skiplast;			/* skip last heading field */
	char ordered;		/* references ordered */
	char ascendingorder;	/* refs in ascending order */
	char ison;			/* sort is on */
	short refpri[REFTYPES+1];		/* reference priority */
	short partorder[V2COMPMAX+1];	/* component comparison order */
	unsigned char chartab[V2STSTRING];	/* character value table */
	char reftab[REFTYPES];	/* priority table for ref types */
	short styleorder[STYLETYPES+1];	/* priority table for style types */
	short unused;
	long spare;			/* !!spare */
} V2SORTPARAMS;

/***************************************************************/
// headparams.h

typedef struct {
	long headsize;				/* size of header */
	unsigned short version;		/* version # */
	RECN rtot;					/* total number of records */
	time_c elapsed;				/* time spent executing commands */
	time_c createtime; 			/* time of creation  */
	short resized;				/* true if resized */
	short spare1;				/* spare */
	time_c squeezetime;			/* time of last squeeze */
	V2INDEXPARAMS indexpars;/* index structure pars */
	V2SORTPARAMS sortpars;	 /* sort parameters */
	V2REFPARAMS refpars;	 /* reference parameters */
	V2PRIVATEPARAMS privpars;	/* private preferences */
	V2FORMATPARAMS formpars;	/* format parameters */
	char stylestrings[V2STYLESTRINGLEN];		/* strings for auto style */
	V2FONTMAP fm[FONTLIMIT];		/* mapping of local IDs to font names (0 is default) */
	RECN root;					/* number of root of alpha tree */
	short dirty;				/* TRUE if any record written before close */
	char styletab[STYLETYPES*2];	// !! displaced sort-parameter: ref style precedence order
	RECT vrect;				// view rect
	UINT mmstate;			// max min state
	char spare[228];		/* !!spare */
} V2HEAD;
#define V2HEADSIZE (sizeof(V2HEAD))

//For groups
/***************************************************************/
/***************************************************************/
//searchparams.h

#define V2LISTSTRING 256
typedef struct {		/* structure for finding text in records */
	char string[V2LISTSTRING];	/* string being sought (or expression) */
	char * auxptr;		/* pointer to auxiliary buffer */
	char * ref2ptr;		/* pointer to second ref in a range */
	short field;		/* field to search */
	short expcount;		/* number of subexpressions in pattern */
	char patflag;		/* is pattern */
	char caseflag;		/* is case sensitive */
	char notflag;		/* logical not */
	char andflag;		/* logical and */
	char evalrefflag;	/* evaluate references (if page field) */
	char wordflag;		/* match whole word */
	unsigned char style;	/* style, etc */
	unsigned char font;
	long spare[4];		// spare
} V2LIST;

typedef struct {		/* set of list structures */
	char userid[5];		// user id
	short lflags;		/* flags that say something about selection */
	short size;			/* number in set */
	char revflag;		/* search backwards */
	char excludeflag;	// excludes records with specified attributes
	char newflag;		/* search for new */
	char modflag;		/* search for modified */
	char markflag;		/* search for marked */
	char delflag;		/* search for deleted */
	char genflag;		/* search for generated */
	char sortmode;		/* search with sort on/off */
	char tagflag;		/* search for flagged */
	int tagvalue;		// kind of tags to search for
	RECN firstr;		/* first record to find */
	RECN lastr;			/* record to stop on */
	time_c firstdate;	/* first date */
	time_c lastdate;	/* last date */
	long spare[4];
	V2LIST lsarray[MAXLISTS];
} V2LISTGROUP;

/***************************************************************/
//groupparams.h

#define V2GROUPLEN 64
#define V2GPNOTELEN 60

typedef struct {		/* header to group */
	int size;
	time_c tstamp;		/* time created */
	time_c indextime;	/* time owning index created */
	char gname[V2GROUPLEN];	/* name of group */
	char comment[V2GPNOTELEN];	/* comment */
	RECN limit;			/* max number of records that can be in group */
	short gflags;		/* flags describing group status */
	V2LISTGROUP lg;		/* search structure */
	V2SORTPARAMS sg;	/* sort structure */
	RECN rectot;		/* # records in it */
	long spare[4];		// spare
	RECN recbase[];		/* array of record numbers */
} V2GROUP;

typedef V2GROUP *V2GROUPHANDLE;
/***************************************************************/

