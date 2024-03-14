#pragma once

enum	{	/* cross-reference position */
	CP_AFTERPAGE,
	CP_HEADNOPAGE,
	CP_FIRSTSUB,
	CP_LASTSUB,
	CP_LASTSUBNOSUB
};

enum	{	/* squeeze flags */
	SQDELDEL = 1,		/* remove deleted records */
	SQCOMBINE = 2,		/* combine duplicate records */
	SQDELGEN = 4,		/* remove generated records */
	SQDELEMPTY = 8,		/* remove empty records */
	SQDELDUP = 16,		/* remove duplicate records */
	SQSINGLE = 128,		/* one reference per record */
	SQIGNORELABEL = 256,	// ignore different labels when combining identical records

	SQPAGESORTFLAG = 512	// pseudo flag for dialog management
};

enum {		// sort filters
	SF_OFF = 0,
	SF_VIEWDEFAULT = 1,
	SF_HIDEDELETEONLY = 2
};

#define RECNUMBUFF 5000		/* number of record #s saved for sort */

struct numstruct {		/* control struct for records to be resorted */
	time_t time;		/* time after which changes made */
	short tot;			/* total number of records in list */
	short max;			/* max # entries list can contain */
	RECN basenum;		/* highest numbered record in index when list started */
	RECN array[];		/* array of record numbers */
};

void sort_resort(INDEX * FF);        /* completely resort index by current sort rules */
void sort_sortgroup(INDEX * FF);		/* sorts group */
struct numstruct * sort_setuplist(INDEX * FF);   /* sets up sort list */
void sort_addtolist(struct numstruct * nptr, RECN num);   /* adds record to list for resorting */
void sort_resortlist(INDEX * FF, struct numstruct * nptr);	/* replaces nodes for records in list */
void sort_makenode(INDEX * FF, RECN num);	/* finds place in tree for record num */
RECN sort_remnode(INDEX * FF, RECN num);   /* removes node for record num. Replaces children, if any */
short sort_crosscompare(INDEX * FF, SORTPARAMS * sgp, char *s1, char *s2); /* compares cross-refs */
BOOL sort_isignored(INDEX * FF, RECORD * recptr);	// returns TRUE if record ignored
void sort_setfilter(INDEX * FF, int filter);	// configures transfer filter
RECORD * sort_top(INDEX * FF);	/* moves to extreme left end of tree */
RECORD * sort_bottom(INDEX * FF);	/* moves to extreme right-hand end of tree */
RECORD *sort_skip(INDEX * FF, RECORD *curptr, short n);	/* returns pointer to text of record +/-n from current */
RECN sort_viewindexforrecord(INDEX * FF, RECN record);	// gets record index for actual record
RECORD * sort_recordforviewindex(INDEX * FF, RECN rindex);	// gets record for record index
RECORD * sort_jump(INDEX * FF, float position);		/* moves to record at ordered posn target */
float sort_findpos(INDEX * FF, RECN target);		/* finds ordinal position in index */
short sort_relpos(INDEX * FF, RECN t1, RECN t2);		/* finds relative positions of recs in index */
BOOL sort_isinfieldorder(short * fieldorder, short maxfields);	/* returns TRUE if straight field order for text fields */
void sort_buildfieldorder(short * fieldorder, short oldlimit, short limit);	/* builds straight text field order */
void sort_squeeze(INDEX * FF, short flags);		/* squeezes index */
