#include "stdafx.h"
#include <string.h>
#include "strings.h"
#include "errors.h"
#include "records.h"
#include "sort.h"
#include "refs.h"
#include "regex.h"
#include "util.h"
#include "index.h"
#include "typestuff.h"
#include "collate.h"

static char *rllist = "mdclxvi";
static char *rulist = "MDCLXVI";

char *r_nlist = "1234567890";	/* numerals for detecting numbers */
short rf_nullorder[]={0,1,2,3,4,5,6,7,8,-1};		/* null page order sequence */

#define PARSEMAX 40		// max number of components that can be parsed
#define REFMAX 300      /* max length of individual page ref */

static short parseref(INDEX * FF, char *base, PARSEDREF * strptr, char * residue, char *sptr);   /* parses source page ref into series of component strings */
static unsigned short rnum(char *start);     /* converts roman number to unsigned leaves pointing beyond */
static short month(LMONTH * months, char *string);      /* returns month number if legit spelling, else -1 */
static short typeref(INDEX * FF, char *sptr);      /* identifies ref segment,  returns class */
static BOOL inlimits(INDEX * FF, char * rptr);	/* checks ref against entry limits */

/******************************************************************************/
static unsigned short rnum(char *start)     /* converts roman number to unsigned leaves pointing beyond */

{
	register char *rlist;
	register unsigned short val;
	static unsigned short valtab[]= { 1000,500,100,50,10,5,1};
	char *xptr, *rptr, *tptr;


	rlist = isupper(*start) ? rulist : rllist;  /* choose upper or lower case by which appears first */

	for (val = 0; *start;)  {            /* while not at end of string */
		for (rptr = rlist; !(xptr = strchr(start,*rptr)); rptr++) /* point to highest val numeral */
			;
		val += valtab[rptr-rlist];			/* extract value from table */
		tptr = xptr--;                      /* pointer to end of segment of number */
		while (xptr >= start)               /* while there are numerals to left of it */
			val -= valtab[strchr(rlist, *xptr--) - rlist];            /* subtract them in turn */
		start = ++tptr;                            /* now point to beginning of next seg */
	}
	return (val);
}
/******************************************************************************/
static short month(LMONTH * months, char *string)      /* returns month number if legit spelling, else -1 */

{
// currently use only long months; name comparison is case-sensitive
	UErrorCode error = U_ZERO_ERROR;
	unichar mname[sizeof(LMONTH)];
	int length, mindex;
	
	u_strFromUTF8(mname, sizeof(LMONTH), &length, string,-1,&error);
	if (length >= 3)	{	// if at least 3 chars
		for (mindex = 0; mindex < 12; mindex++)	{
			if (!u_strncmp(months[mindex],mname,length))	// if match
				return mindex;
		}
	}
	return -1;
}
/******************************************************************************/
static short typeref(INDEX * FF, char *sptr)	/* identifies ref segment, returns class */

{
	char * tptr;
	
	if (*sptr)   {    /* if not empty string */
		tptr = FF->head.sortpars.reftab;		/* set base of priority table */
		if (isdigit(*sptr) && tptr[ARAB] != INVALID)      /* if arabic numeral && valid */
			return (ARAB);     /* identify */
		else {
			if (tptr[ROMAN] != INVALID)		{   /* if legit roman */
				int matchlen = strlen(sptr);
				if (strspn(sptr, rulist) == matchlen || strspn(sptr, rllist) == matchlen)	// if whole string is roman numerals
					return (ROMAN);   /* priority for roman */
			}
			if (tptr[MONTH] != INVALID && month(FF->collator.longmonths, sptr) >= 0)       /* if a legit month */
				return (MONTH);
			else if (tptr[ALPHA] != INVALID)
				return (ALPHA);
		}
		return (INVALID);
	}
	return (EMPTY);
}
/******************************************************************************/
static short parseref(INDEX * FF, char *base, PARSEDREF * strptr, char * residue, char *sptr)   /* parses source page ref into series of component strings */

{
	unsigned char *dptr, *dlimit, *tbase;
	char *tptr;
	char style = 0;
	short count;

	dlimit = base + REFMAX - 2; 		/* limit of what can be put in destination string */
	*residue = '\0';		// set empty residue
	strptr[0].ref = dptr = base;
	strptr[0].style = '\0';

	for (count = 0; *sptr && *sptr != FF->head.refpars.psep && dptr < dlimit;) {	  /* for whole of page ref */
		unichar uc = u8_nextU(&sptr);
		switch (uc) {
		case FONTCHR:
			sptr++;
			continue;
		case CODECHR:
			if (*sptr) {
#if 0
				if (!(*sptr&FX_OFF))	// if not turning off style
					strptr[count].style = *sptr&(FX_BOLD | FX_ITAL | FX_ULINE);	// save style
#else
				if (!(*sptr&FX_OFF))	// if not turning off style
					style |= *sptr&(FX_BOLD | FX_ITAL | FX_ULINE);	// add permissible style
				else
					style &= ~(*sptr&(FX_BOLD | FX_ITAL | FX_ULINE));	// strip permissible style
				strptr[count].style = style;
#endif
				sptr++;
			}
			continue;
		case ESCCHR:
			if (*sptr)
				sptr++;
			continue;
		case OBRACKET:
			tptr = sptr;
			sptr = str_skiptoclose(sptr, CBRACKET);
			strncat(residue, tptr, sptr - tptr - 1);
			continue;
		default:
			tbase = dptr;		/* save start in case component discarded as invalid */
			if (u_isdigit(uc)) {       /* an arabic numeral */
				do {
					dptr = u8_appendU(dptr, uc);
				} while (dptr < dlimit && *sptr && (uc = u8_toU(sptr)) && u_isdigit(uc) && (sptr = u8_forward1(sptr)));
				*dptr = '\0'; 		/* terminate it */
			}
			else if (u_isalpha(uc)) {	// if letter
				do {
					dptr = u8_appendU(dptr, uc);
				} while (dptr < dlimit && *sptr && (uc = u8_toU(sptr)) && u_isalpha(uc) && (sptr = u8_forward1(sptr)));
				*dptr = '\0'; 		/* terminate it */
			}
			else
				continue;
			if (typeref(FF, strptr[count].ref) == INVALID) { /* if this to be ignored */
				dptr = tbase;		/* restore orig pointer posn */
				strptr[count].style = '\0';
				continue;
			}
			if (++count == PARSEMAX) 		/* if reached limit of components */
				return (count);
			strptr[count].ref = ++dptr;      //  set base for next field */
//			strptr[count].style = '\0';		// set no style
			strptr[count].style = style;	// initial style is current style
		}
	}
	return (count);       /* return # of components recognized */
}
/******************************************************************************/
short ref_match(INDEX * FF, char *s1, char *s2, short *order, char flags)	/* compares strings by page ref sorting rules */
																			/* order sets evaluation order of components */

{
	register short val1, val2;
	long lval1, lval2;
	char *tp1, *tp2;
	short key1, key2;
	BOOL serialorder;

	char base1[REFMAX], base2[REFMAX], residue1[REFMAX], residue2[REFMAX];
	PARSEDREF strptr1[PARSEMAX], strptr2[PARSEMAX];
	short count1, count2, pos, last1, last2, rpos;
	short sense;
	short stylesign = 0;

	if (flags&PMSENSE && !FF->head.sortpars.ascendingorder)	/* if want to set sense, & negative */
		sense = -1;		/* set sense of result from flags */
	else
		sense = 1;

	count1 = parseref(FF, base1, strptr1, residue1, s1);	   /* parse strings */
	count2 = parseref(FF, base2, strptr2, residue2, s2);
	serialorder = count1 > COMPMAX || count2 > COMPMAX;	// force serial order if more segments than can order

	for (pos = 0, last1 = count1 - 1, last2 = count2 - 1; count1 && count2; count1--, count2--, pos++) {	/* for each component */
		if (serialorder)	// if serial order
			rpos = pos;
		else {		// attend to specified order
			while ((rpos = order[pos]) > last1 && rpos > last2)	   /* while current component exists in neither reference */
				pos++;          /* advance to next in list */
			if (rpos < 0)       // if no components to compare (last order entry is -1)
				return (0);
	}
		if (rpos > last1 || rpos > last2)       /* if component exists in only one */
			return (last1 >= rpos ? sense : -sense);
		if ((key1 = typeref(FF, tp1 = strptr1[rpos].ref)) != (key2 = typeref(FF, tp2 = strptr2[rpos].ref)))      /* if segments of different class */
			return (FF->head.sortpars.reftab[key1] > FF->head.sortpars.reftab[key2] ? sense : -sense);		/* need no further comparison */
		switch (key1) {       /* compare within identical classes */
		case ARAB:
			if ((lval1 = atol(tp1)) != (lval2 = atol(tp2))) 		/* if diff values */
				return (lval1 > lval2 ? sense : -sense);	  /* send different */
			break;                              /* on to next segment */
		case ROMAN:
			if ((val1 = rnum(tp1)) != (val2 = rnum(tp2)))       /* if roman #'s different */
				return (val1 > val2 ? sense : -sense);		 /* show different */
			break;
		case ALPHA:
			while (*tp1 == *tp2) {	// skip while alpha strings same [don't need uchars because comparing for identity]
				if (!*tp1++)
					goto loop;
				tp2++;
			}
			return u8_toU(tp1) - u8_toU(tp2);
		case MONTH:
			if ((val1 = month(FF->collator.longmonths, tp1)) != (val2 = month(FF->collator.longmonths, tp2)))       /* if months differ */
				return (val1 > val2 ? sense : -sense);	  /* return difference */
			break;
		case INVALID:
			break;		/* ignore altogether */
		case EMPTY:
			return (0);      /* no field of any class found */
		}
	loop:
		stylesign = FF->head.sortpars.styletab[strptr1[rpos].style] - FF->head.sortpars.styletab[strptr2[rpos].style];	// save most recent style sign
}
#if 0
	if (!count1 && (!count2 || !(flags&PMEXACT)))	/* if both refs at end or will accept match to end of first */
		return ((flags&PMSTYLE) ? stylesign : 0);
#else
	if (!count1 && (!count2 || !(flags&PMEXACT))) {	/* if both refs at end or will accept match to end of first */
		if ((flags&PMSTYLE) && stylesign)		// if style can be tiebreaker
			return stylesign;
		return col_match(FF, &FF->head.sortpars, residue1, residue2, MATCH_IGNORECODES);	// residue is tiebreaker
	}
#endif
	return (count1 ? sense : -sense);  /* return 1 if first has more segs */
}
/******************************************************************************/
char *ref_next(char * s1, char sep)	/* finds next unprotected reference separator */

{
	while (*s1)	{
		if (*s1 == sep)		/* if we've got our char */
			return (s1);
		switch (*s1++)	{	/* test protecton */
			case KEEPCHR:
			case ESCCHR:
				if (*s1)
					s1++;
				break;
			case OBRACKET:	/* hidden */
//				s1 = str_skipbrackets(s1);
				s1 = str_skiptoclose(s1,CBRACKET);
				break;
		}
	}
	return (NULL);
}
/******************************************************************************/
char *ref_last(char * s1, char sep)	/* finds last unprotected reference separator */

{
	char * lptr;
	
	for (lptr = NULL; s1 = ref_next(s1,sep); s1++)
		lptr = s1;
	return (lptr);
}
/******************************************************************************/
char * ref_incdec(INDEX *FF, char * rstring, BOOL increment)	// increments/decrements last numerical component

{
	char * last;
	PARSEDREF strptr[PARSEMAX];
	char component[MAXREC];
	char base[REFMAX], residue[REFMAX];
	int count;

	if (last = ref_last(rstring, FF->head.refpars.psep))	// if last of several
		last++; // skip over separator
	else
		last = rstring;
	count = parseref(FF, base, strptr, residue, last);	   /* parse strings */
	if (count) {
		strcpy(component, strptr[count - 1].ref);		// copy last component
		if (typeref(FF, component) == ARAB) {	// if adjustable
			long val = atol(component);
			long baseval = 1;
			char * cptr = strchr(last, FF->head.refpars.rsep);	// find any range connector in last ref

																// if have possible range && our second last component sits before range connector and is arab
			if (cptr && count >= 2 && !strstr(cptr + 1, strptr[count - 2].ref) && typeref(FF, strptr[count - 2].ref) == ARAB)
				baseval += atol(strptr[count - 2].ref);
			if (increment || val > baseval) {
				val += increment ? 1 : -1;
				strcpy(rstring, component);	// get old value of component
				sprintf(rstring + strlen(rstring) + 1, "%ld", val);	// make double string with old and new vals of changed component
				return rstring;
			}
		}
	}
	return NULL;
}
/******************************************************************************/
char * ref_autorange(INDEX *FF, char * rstring)	// generates second part of range automatically

{
	char * last;
	PARSEDREF strptr[PARSEMAX];
	char base[REFMAX], residue[REFMAX];
	int count;

	if (last = ref_last(rstring, FF->head.refpars.psep))	// if last of several
		last++; // skip over separator
	else
		last = rstring;
	count = parseref(FF, base, strptr, residue, last);	// parse strings
	if (count) {
		char component[MAXREC];
		strcpy(component, strptr[count - 1].ref);		// copy last component
		if (typeref(FF, component) == ARAB) {	// if extensible
			long val = atol(component) + 1;
			char * dest = rstring + strlen(rstring);	// will append new text
//			sprintf(dest, "%c%ld", FF->head.refpars.rsep,val);	// make string containing second part of range
			char tcodes[10];	// holds any trailing codes to be restored
			while (*last == SPACE)	// strip leading spaces from ref
				last++;
			strcpy(tcodes, str_rskipcodes(last));	// capture trailing codes
			long plength = strlen(last) - strlen(component) - strlen(tcodes);	// net length of prefix to final numeric segment
			sprintf(dest, "%c%.*s%ld%.*s", FF->head.refpars.rsep, (int)plength, last, val, (int)strlen(tcodes), tcodes);	// make string for second part of range
			return dest;
		}
	}
	return NULL;
}
/******************************************************************************/
char * ref_isinpage(INDEX * FF, RECORD * recptr,unsigned long low,unsigned long high)  /* scans page field for value >= low and <= high
		     returns pointer to page field (or NULL) */

{
	register char *base, *pptr;
	char *mark, *endc;
	unsigned long val1, val2;

	pptr = str_xlast(recptr->rtext);		/* point to page # */
	if (!*pptr || str_crosscheck(FF,pptr))     /* if empty or cross ref */
		return (NULL);

	base = pptr;

	while (base && (base = ref_goodnum(base))) {	  /* for legal numerical strings */
		val1 = atol(base);	      /* get value */
		base += strspn(base,r_nlist);	      /* point to char beyond */
		endc = ref_next(base,FF->head.refpars.psep);     /* mark end of reference  (if any) */
		if ((mark = strchr(base, FF->head.refpars.rsep)) && mark < (base = ref_goodnum(base)) && (!endc || endc > base)) {
			/* if next val is second of a range and within same reference field */
			val2 = atol(base);		      /* collect it */
			base += strspn(base,r_nlist); 	      /* advance pointer */
		}
		else
			val2 = val1;		   /* no second value in field */
		if (!(val1 < low && val2 < low) && !(val1 >high && val2 > high))   /* if any val in range */
			return (pptr);			/* ret pointer to page field */
	}
	return (NULL);	       /* if get to here, must be no match */
}
/******************************************************************************/
char *ref_goodnum(char *posn)	 /* returns pointer to first arabic number in field; ignores escaped ascii codes */

{
	register char c;

	while ((c = *posn++)) {
		if (isdigit(c)) 	     /* if at real digit */
			return(--posn);	     /* return its posn */
#if 0
		if (c == ESCCHR)	   /* if an escape sequence */
			str_transnum(&posn);	   /* discard it */
#endif
	}
	return (NULL);	    /* no number in string */
}
/******************************************************************************/
char *ref_sortfirst(INDEX * FF, char *s1)	   /* finds first (in sort order) ref in page field */

{
	char *firstptr, *xptr;
	char base1[REFMAX], base2[REFMAX], residue1[REFMAX], residue2[REFMAX];
	PARSEDREF strptr1[PARSEMAX], strptr2[PARSEMAX];
	short count1, count2;

	firstptr = s1;
	if (FF->head.sortpars.ordered) { /* if want page refs ordered */
		while (s1 = ref_next(s1, FF->head.refpars.psep)) {	/* find ref beyond current */
			xptr = ++s1;            		/* skip separator */
			count1 = parseref(FF, base1, strptr1, residue1, firstptr);	/* parse current lowest */
			count2 = parseref(FF, base2, strptr2, residue2, xptr);		/* and next in list */
			if (count1 && count2) {	/* if both have valid refs */
				if (ref_match(FF, firstptr, xptr, FF->head.sortpars.partorder, PMEXACT | PMSENSE | PMSTYLE) > 0)		/* if new smaller than old */
					firstptr = xptr;      /* reset first */
			}
			else if (count2)		/* if second is only valid ref */
				firstptr = xptr;	/* it becomes lowest */
		}
	}
	return (firstptr);   /* return pointer to first ref */
}
/******************************************************************************/
char *ref_isinrange(INDEX * FF, char *pptr, char *low, char *high, short * errtype)

/* finds first ref in range in field */
/* If low is empty string, does special internal check on ranges, and returns NULL if OK */

{
	char base1[REFMAX], base2[REFMAX], firstref[REFMAX], secref[REFMAX], residue1[REFMAX], residue2[REFMAX];
	PARSEDREF strptr1[PARSEMAX], strptr2[PARSEMAX];
	short count1, count2, tcount, mcount;
	char *nextptr, *secptr;
	short lowhit, highhit;

	if (str_crosscheck(FF, pptr))
		return (NULL);
	do {
		if (nextptr = ref_next(pptr, FF->head.refpars.psep))	/* end of this ref (if another follows) */
			nextptr++;
		if (*low) {
			if (!(lowhit = ref_match(FF, low, pptr, FF->head.sortpars.partorder, 0)) ||
				lowhit < 0 && high && ref_match(FF, high, pptr, FF->head.sortpars.partorder, 0) >= 0)
				/* if (first) ref is exact or within range defined by low & high */
				break;	/* good hit */
		}
		else if (!inlimits(FF, pptr)) {
			*errtype = ERR_VALUETOOLARGE;
			break;
		}
		if ((secptr = strchr(pptr, FF->head.refpars.rsep)) && (!nextptr || secptr < nextptr)) {	/* if second component to ref */
			strncpy(firstref, pptr, secptr - pptr);	 /* get isolated first ref */
			*(firstref + (secptr - pptr)) = '\0';		 /* terminate it */
			count1 = parseref(FF, base1, strptr1, residue1, firstref);
			count2 = parseref(FF, base2, strptr2, residue2, ++secptr);
			if (count1 && count1 >= count2) {   /* if second ref has same or fewer components than first */
												/* build up the missing parts of the second ref */
				do {
					for (mcount = count2; mcount && typeref(FF, strptr1[count1 - 1].ref) != typeref(FF, strptr2[mcount - 1].ref); mcount--)   /* while trailing segs are diff types (assume second is abbreviated) */
						;	/* ends with # of segs in second that we assume match first */
				} while (!mcount && --count1);		/* while can't find matching segs by truncating second, truncate first */
				if (mcount) {		/* if any matching components of same type */
					for (*secref = '\0', tcount = 0; tcount < count1 - mcount + count2; tcount++) {	 /* for all components */
						strcat(secref, tcount < count1 - mcount ? strptr1[tcount].ref : strptr2[tcount - (count1 - mcount)].ref); /* copy successively from first or second seg */
						strcat(secref, ":");
					}
					secptr = secref;	  /* set new copy for comparison */
				}
			}
			if (*low) {	/* if normal test */
				if (!(highhit = ref_match(FF, low, secptr, FF->head.sortpars.partorder, 0))
					|| highhit < 0 && high && (highhit = ref_match(FF, high, secptr, FF->head.sortpars.partorder, 0)) >= 0)
					/* if second ref is exact or within range defined by low and high */
					break;	/* good hit */
				if (lowhit > 0 && highhit < 0)	/* if both components of ref fall outside low-high range	*/
					break;
			}
			else {	/* special checks on range (only during record entry) */
				if (ref_match(FF, secptr, firstref, FF->head.sortpars.partorder, PMEXACT) <= 0) {	/*  if higher of range <= lower */
					*errtype = ERR_BADRANGEFIELD;
					break;	 /* second less than first */
				}
				if (!inlimits(FF, secptr)) {	/* second out of allowable range */
					*errtype = ERR_VALUETOOLARGE;
					break;
				}
				if (FF->head.refpars.maxspan && count1 && count2 && isdigit(*strptr1[count1 - 1].ref) && isdigit(*strptr2[count2 - 1].ref)		/* if last components are numbers */
					&& atol(strptr2[count2 - 1].ref) > atol(strptr1[count1 - 1].ref) + FF->head.refpars.maxspan) { /* and range is excessive */
					*errtype = ERR_SPANTOOLARGE;
					break;
				}
			}
		}
	} while (pptr = nextptr);	/* while there are further refs to check */
	return (pptr);
}
/******************************************************************************/
static BOOL inlimits(INDEX * FF, char * rptr)	/* checks ref against entry limits */

{
	if (*FF->head.refpars.maxvalue)
		return (ref_match(FF, rptr, FF->head.refpars.maxvalue, FF->head.sortpars.partorder,PMEXACT) <= 0);
	return (TRUE);
}
/******************************************************************************/
long ref_adjust(INDEX * FF, struct adjstruct * adjptr)		 /* adjusts entries in locator field */

{
	register char *i, *j;
	char tempstring[100], ts1[MAXREC];
	char *pptr, *tptr, *xptr, *tp0, *tp1, *tp2, *mid;
	char *remaind, tsort;
	unsigned short len1, templen;
	short avail, amount;
	long ecount;
	long val1, val2, toplim;
	RECORD * recptr;

	if (adjptr->cut && !adjptr->hold)	  {  /* if cut and not holding page nos */
		adjptr->shift -= adjptr->high-adjptr->low+1;	   /* make auto adjustment for cut on pages > highlim */
		toplim = LONG_MAX;		  /* allow adjustment of pages > highlim */
	}
	else
		toplim = adjptr->high;    /* range up to highlim */
	index_cleartags(FF);	// clear tags
	tsort = FF->head.sortpars.ison;
	FF->head.sortpars.ison = FALSE;		/* always do with sort off */
	for (recptr = sort_top(FF), ecount = 0; recptr; recptr = sort_skip(FF,recptr,1)) {	  /* for all records */
		if (!(pptr = ref_isinpage(FF, recptr,adjptr->low, toplim)))		/* if no target pages */
			continue;			/* skip this record */
		avail = FF->head.indexpars.recsize - str_xlen(recptr->rtext)-1;	/* get free space in record */
		for (j = pptr, tptr = ts1; *j && *j != EOCS; j++) {		  /* copy each entry in page field, terminating with 0 */
			short matchlen;
			char * mptr;
			
			if (*j == FF->head.refpars.psep)		      /* if get separator in wrong place */
				continue;			/* ignore it */
			for (i = tptr; *j != FF->head.refpars.psep && *j; *i++ = *j++)		  /* copy chars as long as no field sep */
				;
			*i = '\0';					/* terminate string */
			xptr = tptr;				/* set temp pointer */
//			if (!adjptr->patflag || re_exec(tptr, adjptr->buffer) && *expemark) {	  /* if no prefix wanted, or one is found with following chars	*/
			if (!adjptr->patflag || (mptr = regex_find(adjptr->regex, tptr,0,&matchlen))) {	  /* if no prefix wanted, or one is found with following chars	*/
				if (adjptr->patflag) 		// if found a pattern
					xptr = mptr+matchlen;
/* NB: following changed to ensure that only first arabic segment (after any pattern match) is adjusted. prev was all segs */
#if 1
				tp0 = xptr;
				if (tp1 = ref_goodnum(tp0))	{  /* if numerical segment in field */
#else
				for (tp0 = xptr; (tp1 = ref_goodnum(tp0)); tp0 = tp1+templen)	{  /* while numerical segments in field */
#endif
					val1 = atol(tp1);           /* convert numerals */
					len1 = strspn(tp1, r_nlist);   /* length */
#if 0			// fix 8/4/18 to recognize skip pattern in second part of locator range; formerly didn't
					if ((mid = strchr(tp1, FF->head.refpars.rsep)) && mid < (tp2 = ref_goodnum(tp1 + len1))) {       /* if next # is end of sequence */
						val2 = atol(tp2);       /* get second number */
						remaind = tp2 + strspn(tp2, r_nlist);
					}
					else {
						mid = NULL;         /* ensure don't count wrong end of sequence */
						val2 = 0;			/* no second value */
						tp2 = tp1 + len1;		/* now at char beyond end of first num */
						remaind = tp2;
					}
#else
					val2 = 0;		// initialize assuming no range
					tp2 = remaind = tp1 + len1;
					if ((mid = strchr(tp1, FF->head.refpars.rsep))) {       // if there's range separator after first ref [NB: to fix: if bad pattern, first ref might be found after range separator]
						matchlen = 0;
//						if (!adjptr->patflag || (mid = regex_find(adjptr->regex, mid,0,&matchlen))) {	// skip pattern if any
						if (adjptr->patflag) {		// check for pattern in second segment
							char * secondmatch = regex_find(adjptr->regex, mid, 0, &matchlen);
							if (secondmatch && ref_goodnum(secondmatch + matchlen))		// if second seg matches pattern, with number following, skip pattern, otherwise ignore
								mid = secondmatch;
						}
						if (mid) {
							tp2 = ref_goodnum(mid + matchlen);
							if (tp2) {	// if good number
								val2 = atol(tp2);       /* get second number */
								remaind = tp2 + strspn(tp2, r_nlist);
							}
						}
					}
#endif
					if (adjptr->cut)  {                      /* if want pages removed */
						if (val1 >= adjptr->low && val1 <= adjptr->high)  {     /* if val1 in cut range */
							if (val2 > adjptr->high)         /* if val2 outside it */
								val1 = adjptr->high+1 + adjptr->shift;       /* set val1 at high limit + shift */
							else                        /* otherwise */
								val1 = 0;               /* remove val1 */
						}
						else if (val1 > adjptr->high)
							val1 += adjptr->shift;                  /* apply shift if > highlim */

						if (val2 >= adjptr->low && val2 <= adjptr->high)   {       /* if val2 in cut range */
							if (val1 < adjptr->low && val1 > 0)      /* if val1 outside  cut */
								val2 = adjptr->low -1;               /* set below limit */
							else
								val2 = 0;               /* otherwise kill it */
						}
						else if (val2 > adjptr->high)
							val2 += adjptr->shift;          /* apply shift if val2 > highlim */
					}
					else {      /* if only simple adjustment */
						if (val1 >= adjptr->low && val1 <= adjptr->high)  /* if val1 within limits */
							val1 += adjptr->shift;
						if (val2 >= adjptr->low && val2 <= adjptr->high)  /* if val2 within limits */
							val2 += adjptr->shift;
						if (val1 <= 0 && val2 > adjptr->low)           /* if first but not second <= 0 */
							val1 = 1;
					}

					*tempstring = '\0';         /* initialize temp string */
					if (val1 >0)   {            /* if a val1 */
						sprintf(tempstring, "%ld", val1); /* convert it */
						if (mid != NULL)        /* if also started with a val2 */
							strncat(tempstring, tp1+len1, mid - tp1-len1);      /* add string up to rsep */
					}
					if (val2 >0)  {             /* if a val2 */
						if (val1 >0)            /* if also a val1 */
							strncat(tempstring, mid, tp2-mid);  	/* add chars from rsep */
						sprintf(tempstring+strlen(tempstring), "%ld", val2);     /* add second conversion */
					}

					if (!(templen = strlen(tempstring)))  {         /* if no string at all */
						tp1 = tp0;			/* set to remove any leading chars before val1 */
						if (!ref_goodnum(remaind))       /* if no further numbers in field */
							remaind += strlen(remaind);     /* set to remove all trailing chars */
					}
					if (avail >= (amount = templen-(remaind-tp1)))   {  /* if room for adjusted strings */
						memmove(remaind+amount,remaind,strlen(remaind)+1);      /* slide remainder of string */
						strncpy(tp1, tempstring, templen);
						avail -= amount;
					}
					else  {							/* if there's no room */
						recptr->ismark = TRUE;		/* else mark record as having no room */
						ecount++;					/* count error */
						goto oflo;					/* abandon adjustments */
					}
				}
			}
			if (*xptr) {						/* if field not empty */
				tptr += strlen(tptr);			/* adjust base pointer to end of reference field */
				*tptr++ = FF->head.refpars.psep;	 /* insert separator */
			}
		}
		if (tptr != ts1)	/* if something in field */
			tptr--;			/* go back to overwrite last separator */
		*tptr++ = '\0';		/* clear last separator */
		*tptr = EOCS;		/* add EOCS to end */
		str_xcpy(pptr, ts1);	/* copy fields back to record */
		if (!*pptr)			/* if field empty */
			recptr->isdel = TRUE;		/* delete record */
oflo:						/* break out to here if overflow */
		rec_stamp(FF,recptr);
	}
	FF->head.sortpars.ison = tsort;		/* restore sort */
	return (ecount);
}
/******************************************************************************/
void ref_expandfromsource(INDEX * FF, char * dest, char * source)	// builds dest to have same number of segments as source

{
	char base1[REFMAX], base2[REFMAX], residue1[REFMAX], residue2[REFMAX];
	PARSEDREF strptr1[PARSEMAX], strptr2[PARSEMAX];
	short count1, count2, tcount, mcount;

	count1 = parseref(FF, base1, strptr1, residue1, source);
	count2 = parseref(FF, base2, strptr2, residue2, dest);
	if (count1 && count1 >= count2) {   /* if dest ref has same or fewer components than source */
										/* build up the missing parts of the dest ref */
		do {
			for (mcount = count2; mcount && typeref(FF, strptr1[count1 - 1].ref) != typeref(FF, strptr2[mcount - 1].ref); mcount--)   /* while trailing segs are diff types (assume dest is abbreviated) */
				;	/* ends with # of segs in dest that we assume match source */
		} while (!mcount && --count1);		/* while can't find matching segs by truncating dest, truncate source */
		if (mcount) {		/* if any matching components of same type */
			for (*dest = '\0', tcount = 0; tcount < count1 - mcount + count2; tcount++) {	 /* for all components */
				strcat(dest, tcount < count1 - mcount ? strptr1[tcount].ref : strptr2[tcount - (count1 - mcount)].ref); /* copy successively from source or dest seg */
				if (tcount < count1 - mcount + count2 - 1)
					strcat(dest, ":");
			}
		}
	}
}
