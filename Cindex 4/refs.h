#pragma once
#include "regex.h"

enum {		// ref comparison flags
	PMSENSE = 1,	   /* determines sense of comparison in pmatch */
	PMEXACT = 2,		/* determines whether exact match required */
	PMSTYLE = 4		// compare styles for identical refs
};

struct adjstruct {		/* struct for adjusting refs */
	long shift;		/* offset added/subtracted */
	long low;			/* low number */
	long high;			/* high number */
	short cut;		/* cutflag */
	short hold;		/* prevents adjustment of higher refs after cut */
	short patflag;	/* true if using pattern */
	URegularExpression * regex;		// regular expression handler
};

typedef struct {
	unsigned char *ref;
	int style;
} PARSEDREF;

enum {
	ROMAN,        /* indices for page ref priority */
	ARAB,
	ALPHA,
	MONTH,
	EMPTY,
	INVALID = -1
};

extern char *r_nlist;
extern short rf_nullorder[];			/* null page order sequence */

short ref_match(INDEX * FF,char *s1, char *s2, short *order, char flags);	/* compares strings by page ref sorting rules */
char *ref_next(char * s1, char sep);	/* finds next unprotected reference separator */
char *ref_last(char * s1, char sep);	/* finds last unprotected reference separator */
char * ref_incdec(INDEX *FF, char * rstring, BOOL increment);	// increments/decerments last numerical component
char * ref_autorange(INDEX *FF, char * rstring);	// generates second part of range automatically
char * ref_isinpage(INDEX * FF, RECORD * recptr,unsigned long low,unsigned long high);  /* scans page field for value >= low and <= high */
char *ref_goodnum(char *posn);	 /* returns pointer to first arabic number in field; ignores escaped ascii codes */
char *ref_sortfirst(INDEX * FF, char *s1);	   /* finds first (in sort order) ref in page field */
char *ref_isinrange(INDEX * FF, char *pptr, char *low, char *high, short * errtype);
long ref_adjust(INDEX * FF, struct adjstruct * adjptr);		 /* adjusts entries in locator field */
void ref_expandfromsource(INDEX * FF, char * dest, char * source);	// builds dest to have same number of segments as source
