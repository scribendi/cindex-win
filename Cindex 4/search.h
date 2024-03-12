#pragma once

RECORD * search_findbylead(INDEX * FF, char *string);	/* finds by lead text */
RECORD * search_treelookup(INDEX * FF, char *string);	/* finds record in full tree */
RECORD * search_linsearch(INDEX * FF, RECORD * curptr, char * searchspec);	/* does linear search for leads */
RECORD * search_findbynumber(INDEX * FF, RECN num);	/* finds record by number */
RECORD * search_lastmatch(INDEX * FF, RECORD * recptr, char * searchspec, short matchtype);	/* finds last rec that matches spec */
RECN search_count(INDEX * FF, COUNTPARAMS * csptr, int filter);	/* counts records */
char * search_findbycontent(INDEX * FF, RECORD * recptr, char * startptr, LISTGROUP * lg, short *mlength);	/* finds first record that matches */
short search_setupfind(INDEX * FF, LISTGROUP * lg, short * field);	/* completes fields in search struct */
void search_clearauxbuff(LISTGROUP * lg);	/* clears auxiliary buffer as necess */
RECORD * search_findfirst(INDEX * FF, LISTGROUP * lg, short restart, char **sptr, short *mlptr);	/* finds first rec after rptr that contains string */
char * search_reptext(INDEX * FF, RECORD * recptr, char * startptr, short matchlen, REPLACEGROUP * rgp, LIST * ll);	 /* replaces text in record */
char * search_testverify(INDEX * FF, char * rtext); // returns target on failure
short search_verify(INDEX * FF, char * rtext, VERIFYGROUP * vp); /* checks valdity of cross refs */
RECN search_convertcross(INDEX * FF, int threshold);		// converts cross-refs to full postings
RECN search_autogen(INDEX * FF, INDEX *XF, AUTOGENERATE * agp); /* generates cross refs for appropriate targets */
