#pragma once

typedef struct 	{	/* page formatting structure */
	int characters;		// total characters
	short totalpages;	// total pages examined/generated
	long first;			/* first page to produce */
	long last;			/* last page to produce */
	short oddeven;		/* flags for odd even pages */
	RECN firstrec;		/* record to start at */
	RECN lastrec;		/* last record */
	short pagenum;		/* number given starting page */
	long uniquemain;	// unique ain headings
	long entries;		/* # entries produced */
	long prefs;			/* page refs produced */
	long crefs;			/* # cross refs */
	long lines;			/* # lines */
	short pageout;		/* number of pages that would be output */
	short lastpage;		/* number of last page formatted */
	RECN rnum;			// record at top of first page
	RECN lastrnum;		// record at bottom of last page
	short offset;		/* # lines into record */
	short labelmark;	/* print bullet on labeled records */
	long spare[128];
} PRINTFORMAT;
