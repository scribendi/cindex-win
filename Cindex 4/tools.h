#pragma once
#define ORPHANARRAYBLOCK 500
enum {
	OR_ABSORB = 0,
	OR_DELETE,
	OR_PRESERVE
};

enum {
	SPLIT_PHRASE,
	SPLIT_NAME_S,
	SPLIT_NAME_F,
	SPLIT_USER,
};

typedef struct {
	VERIFY * crossrefs;
	int refcount;
	unsigned int fields[];
} CHECKERROR;

typedef struct  {		/* contains parameters for joining records */
	short firstfield;	/* highest level field to join */
	char jchar;			/* joining character */
	short nosplit;		/* don't split modified fields */
	short protectnames;	/* don't split where cap follows join char */
	BOOL orphanaction;	// specifies orphan action
	int * orphans;		// array of orphan records
	int orphancount;	// count of orphans
	CHECKERROR ** errors;
} JOINPARAMS;

#define SPLITPATTERNLEN 256
typedef struct {		// contains parameters for splitting records
	char userpattern[SPLITPATTERNLEN];
	int patternindex;
	BOOL preflight;		// just run parser and show string output
	BOOL markmissing;	// mark records with missing targets
	BOOL cleanoriginal;	// clean original heading
	BOOL removestyles;	// removes styles
	RECN gencount;
	RECN markcount;
	RECN modcount;
	char ** reportlist;
} SPLITPARAMS;

enum errorTypes {	// item tags are shift size +1
	CE_MULTISPACE = 1,
	CE_PUNCTSPACE = 1 << 1,
	CE_MISSINGSPACE = 1 << 2,
	CE_UNBALANCEDPAREN = 1 << 3,
	CE_UNBALANCEDQUOTE = 1 << 4,
	CE_MIXEDCASE = 1 << 5,
	CE_MISUSEDESCAPE = 1 << 6,
	CE_MISUSEDBRACKETS = 1 << 7,
	CE_BADCODE = 1 << 8,

	CE_INCONSISTENTCAPS = 1 << 9,
	CE_INCONSISTENTSTYLE = 1 << 10,
	CE_INCONSISTENTPUNCT = 1 << 11,
	CE_INCONSISTENTLEADPREP = 1 << 12,
	CE_INCONSISTENTENDPLURAL = 1 << 13,
	CE_INCONSISTENTENDPREP = 1 << 14,
	CE_INCONSISTENTENDPHRASE = 1 << 15,
	CE_ORPHANEDSUBHEADINGINDEX = 16,
	CE_ORPHANEDSUBHEADING = 1 << CE_ORPHANEDSUBHEADINGINDEX,

	CE_EMPTYPAGE = 1 << 17,
	CE_TOOMANYPAGEINDEX = 18,
	CE_TOOMANYPAGE = 1 << CE_TOOMANYPAGEINDEX,
	CE_OVERLAPPINGINDEX = 19,
	CE_OVERLAPPING = 1 << CE_OVERLAPPINGINDEX,
	CE_HEADINGLEVEL = 1 << 20,

	CE_CROSSERR = 1 << 21,

};

typedef struct {
	int version;
	BOOL reportKeys[32];
	int pagereflimit;
	JOINPARAMS jng;
	VERIFYGROUP vg;
	CHECKERROR ** errors;
} CHECKPARAMS;

RECN tool_join (INDEX * FF, JOINPARAMS *js);	 /* joins fields of records that have redundant subheadings */
RECN tool_explode(INDEX * FF, SPLITPARAMS *sp);	 // explodes headings by separating entities
void tool_check(INDEX * FF, CHECKPARAMS *cp);	 // makes comprehensive checks on entries
