#pragma once
#pragma pack (push, v6structures,2)

#define DOSEOCS '\377'
#define DOSENDASH '\196'

#define V61_PATHLEN 64 			/* max # chars in path */

#define V61_FNAMELEN 12		/* max length of field name */

#define V61_REFTYPES 4      /* number of valid page reference types */
#define V61_COMPMAX 10	   /* max # components evaluated in individual ref */
#define V61_CHARPRI 3		/* number of character classes */

#define V61_MAXLINE 80     /* max length of input line */
#define V61_FLSTRING 128
#define V61_FTSTRING 64
#define V61_FSSTRING 32
#define V61_FMSTRING 16
#ifdef BNA
#define V61_FXSTRING 7 		/* 6.1 -- aggregate of FXSTRING && FYSTRING == 8 for compat w 6.0 */
#define V61_FYSTRING 1 		/* 6.1 */
#else
#define V61_FXSTRING 5 		/* 6.1 -- aggregate of FXSTRING && FYSTRING == 8 for compat w 6.0 */
#define V61_FYSTRING 3 		/* 6.1 */
#endif

struct v61_margcol	{		/* margins & columns */
	short top;
	short bottom;
	short left;
	short right;
	short ncols;
	short gutter;
	char continued[V61_FSSTRING];	/* 'continued' text */
	long spare;				/* !!spare */
};

struct v61_headfoot	{	/* header & footer content */
	char left[V61_FTSTRING];
	char center[V61_FTSTRING];
	char right [V61_FTSTRING];
};
struct v61_pageform {		/*	page format */
	struct v61_margcol mc;
	struct v61_headfoot head;
	struct v61_headfoot foot;
	short firstpage;		/* number of first page */
	char reflect;			/* settings reflected for left and right pages */
	char spare1;
	short linespace;		/* line spacing */
	short entryspace;		/* space between entries*/
	short above;			/* space above group header */
	long spare;				/* !!spare */
};

struct v61_cpunct {			/* cross-ref punctuation structure */
	char cleada[V61_FXSTRING];	/* lead text for open ref (see also) */
	char cenda[V61_FYSTRING];	/* end text */
	char cleadb[V61_FXSTRING];	/* lead text for blind ref (see) */
	char cendb[V61_FYSTRING];	/* end text */
};

struct v6_crossform {			/* cross-reference formatting */
	char clead[V61_FMSTRING];	/* lead text */
	char cend[V61_FMSTRING];	/* end text */
	char leadstyle;		/* lead style */
	char bodystyle;		/* body style */
	long spare;				/* !!spare */
};
struct v61_crossform {			/* cross-reference formatting */
	struct v61_cpunct level[2];	/* array of 2 punctuation structures (head & subhead) */
	char leadstyle;		/* lead style */
	char bodystyle;		/* body style */
	char position;		/* how placed in entry */
	char sortcross;		/* TRUE: arrange refs */
	short spare;		/* spare */
};

struct v61_locatorform	{		/* locator formatting */
	char sortrefs;			/* sort references? */
	char spare1;
	char llead1[V61_FMSTRING];	/* lead text for single ref */
	char lleadm[V61_FMSTRING];	/* lead text for multiple refs */
	char connect[V61_FMSTRING];	/* connecting sequence */
	short conflate;			/* threshold for conflation */
	short abbrevrule;		/* abbreviation rule */
	char suppress[V61_FMSTRING];	/* suppress to last of these chars */
	char concatenate[V61_FMSTRING];	/* concatenate with this sequence */
	char styleseq[V61_FSSTRING];	/* style sequence */
	long spare;				/* !!spare */
};

struct v61_groupform	{	/* grouping of entries */
	short method;		/* grouping method */
	char title[V61_FSSTRING];	/* title/format string */
};

struct v61_fieldform	{	/* individual field layout */
	short font;			/* font */
	short size;			/* size */
	short style;		/* style */
	short capstype;		/* caps type */
	char leadtext[V61_FMSTRING];	/* first line indent text */
	char runtext[V61_FMSTRING];	/* runover indent text */
	char trailtext[V61_FMSTRING];	/* trailing text for when no ref */
};

struct v61_entryform	{		/* overall heading layout */
	short runlevel;			/* runon level */
	char style;				/* index style modifiers */
	char itype;				/* indentation type (auto, etc) */
	char adjustpunct;		/* adjust punctuation */
	char autostyle;			/* auto style */
	short autolead;			/* space for auto lead */
	short autorun;			/* space for autorunin */
	long spare;			/* !!spare */
	struct v61_groupform eg;
	struct v61_crossform cf;
	struct v61_locatorform lf;
	struct v61_fieldform field[FIELDLIM-1];	/* field info */
};

struct v61_formgroup	{		/* overall format structure */
	struct v61_pageform pf; 
	struct v61_entryform ef;
};

struct v61_hiddenpref {		/* hidden preferences (always become defaults) */
	long spare;			/* !!spare */
	char user[6];		/* user initials */
};

struct v61_typegroup	{	/* typesetting info */
	char wp[9];			/* wp driver */
	char inserttag;		/* insert tags */
	char fixlength;		/* fix line length */
	char useindent;		/* use indents */
	char usespacing;	/* use line spacing */
	long spare;			/* !!spare */
};

#define V61_PREFIXLEN 256
struct v61_sortgroup {		/* struct for sort pars */
	char type;			/* sort type */
	short fieldorder[FIELDLIM+1];		/* order of fields */
	short charpri[V61_CHARPRI+1];	/* character priority */
	char ignorepunct;	/* ignore l-by-l punctuation */
	char ignoreslash;	/* ignore /- in word sort */
	char ignoreparen;	/* ignore text in parens */
	char evalnums;		/* evaluate numbers */
	char crossfirst;	/* cross-refs first */
	char ignore[V61_PREFIXLEN];	/* ignored leading words in subheads */
	char skiplast;			/* skip last heading field */
	char ordered;		/* references ordered */
	char ascendingorder;	/* refs in ascending order */
	short refpri[V61_REFTYPES+1];		/* reference priority */
	short partorder[V61_COMPMAX+1];	/* component comparison order */
	unsigned char chartab[256];	/* character value table */
	char spare2;		/* spare */
	char ison;			/* sort is on */
	char reftab[V61_REFTYPES];	/* priority table for ref types */
};

struct v61_refgroup {		/* reference parameters */
	char crosstart[V61_FLSTRING];	/* words that can begin cross-refs */
	char crossexclude[V61_FLSTRING];	/* word that begin general cross ref */
	char csep;			/* cross-ref separator */
	char psep;			/* page ref separator */
	char rsep;			/* page range indicator */
};

struct v61_spellgroup	{	/* spelling params */
	char checkcaps;		/* check text in caps */
	char checkpage;		/* check page refs */
	char acceptalnums;	/* accept alphnumerics */
	long spare;			/* !!spare */
	char maindic[9];	/* main dictionary name */
	char userdic[9];	/* user dictionary */
};

struct v61_privgroup	{		/* general config parameters */
	char vmode;				/* full format/ draft flag */
	char wrap;				/* line wrap flag */
	char shownum;			/* show record numbers */
	char hidedelete;		/* hide deleted records */
	short hidebelow;		/* hide headings below this level */
	short font;				/* default font */
	short size;				/* default size */
	long spare;			/* !!spare */
};

struct v61_fieldstruct {		/* specs of field */
	char name[V61_FNAMELEN];	/* name */
	short minlength;		/* minimum */
	short maxlength;		/* max */
	char spare[V61_PATHLEN];		/* */
};

struct v61_indexgroup 	{		/* index structure */
	short recsize;			/* record size */
	short minfields;		/* min fields required in record */
	short maxfields;		/* max fields allowed in record */
	long spare;				/* !!spare */
	struct v61_fieldstruct field[FIELDLIM];		/* field information */
};	

#define V61_KEYLEN 256		/* lenghh of func key strings */
#define V61_STYLELEN 128	/* length of style strings */

typedef struct {
	RECN num;		/* holds record number */
	RECN parent;		/* number of parent record */
	RECN lchild;		/* number of left child record */
	RECN rchild;		/* number of right child record */
	time_c time;		/* time of last change */
	char user[4];		/* up to four initials of user who last changed */
	unsigned short ismod : 1;		/* true if modified since read */
	unsigned short isdel : 1;		/* true if marked for deletion */
	unsigned short ismark : 1;		/* true if record marked */
	unsigned short islock : 1;		/* true if record locked in cache */
	unsigned short balance : 2;		/* holds balance of subtree on record */
	char rtext[];			/* text of record */
} V61_RECORD;

#define V61_RECSIZE (sizeof(V61_RECORD))

typedef struct {
	unsigned short version;		/* version # */
	unsigned short serial;		/* serial # of copy that produced it */
	RECN rtot;					/* total number of records */
	time_c elapsed;				/* time spent executing commands */
	time_c createtime; 			/* time of creation  */
	short spare1; 			/* formerly date of file modification (DOS format) */	
	short spare2;			/* formerly time of modification (DOS format) */
	time_c squeezetime;			/* time of last squeeze */
	struct v61_indexgroup indexpars;/* index structure pars */
	struct v61_sortgroup sortpars;	 /* sort parameters */
	struct v61_refgroup refpars;	 /* reference parameters */
	struct v61_privgroup privpars;	/* private preferences */
	struct v61_formgroup formpars;	/* format parameters */
	struct v61_typegroup typepars;	/* typesetting params */
	char stylestrings[V61_STYLELEN];/* strings for auto style */
	char keystrings[V61_KEYLEN];	/* strings for keys */
	RECN root;					/* number of root of alpha tree */
	short dirty;				/* TRUE if any record written before close */
} V61_HEAD;

#define V61_HEADSIZE (sizeof(V61_HEAD))

enum {			/* view types */
	V61_VIEW_ALL,
	V61_VIEW_GROUP,
	V61_VIEW_LAST,
	V61_VIEW_NEW
};

#pragma pack (pop, v6structures)

enum	{		/* DOS translation flags */
	TR_DOFONTS = 1,
	TR_DOSYMBOLS = 2
};

enum {		/* import control flags */
	READ_IGNORERR = 1,
	READ_HASGH = 2,		/* dos import uses gh codes */
	READ_TRANSGH = 4,	/* dos import translates gh codes */
};

struct ghstruct		{	/* for handling DOS g & h code translations */
	short flags;
	short fcode[2];		/* font ids for translation of DOS g & h codes */
	FONTMAP * fmp;		/* fontmap for translation of g,h codes */
};

#define DOSUNKNOWNCHAR 0x81

extern char dos_greek[];	/* Win characters that in symbol font match dos greek */
extern char dos_fromsymbol[];	/* DOS extend chars that match chars from the symbol font */
extern char tr_escname[];	/* key chars for DOS escape sequences */


BOOL tr_DOStoV2(MFILE * mf);     /* converts DOS v6 index to v2 format */
BOOL tr_ghok(struct ghstruct * ghp);	/* gets spec and goes to record */
BOOL tr_dosxstring(char * buff,struct ghstruct *ghp, int flags);	/* translates DOS CINDEX control codes in record */
void tr_movesubcross(char * buff);	/* moves any subhead cross-ref to page field */
