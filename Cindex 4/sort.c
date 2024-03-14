#include "stdafx.h"
#include "strings.h"
#include "errors.h"
#include "index.h"
#include "records.h"
#include "group.h"
#include "sort.h"
#include "typestuff.h"
#include "refs.h"
#include "files.h"
#include "util.h"
#include "viewset.h"
#include "attributedstrings.h"

#include "collate.h"

#define LLEFT 2	   /* defines directions of imbalance on tree */
#define RRIGHT 1

static INDEX * s_index;
static SORTPARAMS * s_sg;

static int gsort(const void * r1, const void * r2);	/* sort compare for group sort */

static RECORD * balance(INDEX * FF, register RECORD * P, unsigned short bshift);       /* balances subtree on current record */
static void lrotate(INDEX * FF, RECORD * X);  /* rotates to left the subtree based on record X */
static void rrotate(INDEX * FF, RECORD * X);  /* rotates to right the subtree based on record X */
static short match(INDEX * FF, SORTPARAMS * sgp, char *s1, char *s2);	 /* compares record strings by sort rules */

/******************************************************************************/
void sort_resort(INDEX * FF)        /* completely resort index by current sort rules */

{
	register RECN n;
	long onepercent;

	FF->head.root = 0;          /* clear root of tree */
	onepercent = FF->head.rtot/100;
	for (n = 1; n <= FF->head.rtot;) 	{	/* for all records, fetch in turn */
		if (onepercent && !(n%onepercent))
			showprogress(PRG_SORTING,FF->head.rtot,n);
		sort_makenode(FF,n++);			   /* and insert in tree */
	}
	showprogress(0,0,0);	  /* kill message */
	FF->head.sortpars.ison = TRUE;		/* sort is on */
	FF->needsresort = FALSE;
}
/******************************************************************************/
void sort_sortgroup(INDEX * FF)		/* sorts group */

{	
	s_index = FF;			/* index ptr (need static for qsort) */
	s_sg = &FF->curfile->sg;	/* need static pointer to sort group */
	qsort(FF->curfile->recbase, FF->curfile->rectot, sizeof(RECN), gsort);
	if (FF->curfile != FF->lastfile)	/* if already a group */
		grp_make(FF,FF->curfile, FF->curfile->gname, TRUE);/* save changed group */
	FF->curfilepos = 0;		/* force invalid */
	FF->curfile->lg.sortmode = TRUE;
}
/******************************************************************************/
static int gsort(const void * r1, const void * r2)	/* sort compare for group sort */

{
	RECORD * r1ptr, *r2ptr;
	int result;
	
	if (r1ptr = rec_getrec(s_index,*(RECN *)r1))	{
		if (r2ptr = rec_getrec(s_index, *(RECN *)r2))	{
			result = match(s_index,s_sg,r1ptr->rtext, r2ptr->rtext);
			return (result ? result : (r1ptr->num > r2ptr->num ? 1 : -1));
		}
	}
    return (0);
}
/******************************************************************************/
struct numstruct * sort_setuplist(INDEX * FF)   /* sets up sort list */

{
	struct numstruct * nptr;

	if (nptr = getmem(sizeof(struct numstruct)+RECNUMBUFF*sizeof(RECN)))	{
		nptr->max = RECNUMBUFF;
		nptr->time = g_comstarttime;	/* time at which command started */
		nptr->tot = 0;
		nptr->basenum = FF->head.rtot;
	}
	return (nptr);
}
/******************************************************************************/
void sort_addtolist(struct numstruct * nptr, RECN num)   /* adds record to list for resorting */

{
	short count;
	
	if (nptr->tot < nptr->max)	{	/* if room to store another number */
		for (count = 0; count < nptr->tot; count++)	{	/* scan to see if in list */
			if (nptr->array[count] == num)		/* if record already in list */
				return;
		}
		nptr->array[count] = num;		/* add to list */
	}
	nptr->tot++;				/* count the attempt anyway */
}
/***************************************************************************/
void sort_resortlist(INDEX * FF, struct numstruct * nptr)	/* replaces nodes for records in list */

{
	short count;
	RECN rnum;
	RECORD * recptr; 

	if (nptr->tot <= nptr->max)		{		/* if can do the job from the list */
		for (count = 0; count < nptr->tot; count++)	{
			if (nptr->array[count] <= nptr->basenum)	/* if not a new record */
				sort_remnode(FF, nptr->array[count]);
		}
		for (count = 0; count < nptr->tot; count++)
			sort_makenode(FF, nptr->array[count]);
	}
	else	{			/* need to do a time scan cause list is full */
		for (rnum = 1; rnum <= FF->head.rtot; rnum++)	{
			if (recptr = rec_getrec(FF, rnum))	{
				if (recptr->time >= nptr->time && recptr->num <= nptr->basenum)	/* if modified at or after list time && old rec */
					sort_remnode(FF, recptr->num);
			}
			else		/* nasty place to get out of */
				return;
		}			
		for (rnum = 1; rnum <= FF->head.rtot; rnum++)	{
			if (recptr = rec_getrec(FF, rnum))	{
				if (recptr->time >= nptr->time)		/* if modified at or after list time */
					sort_makenode(FF, recptr->num);
			}
			else		/* nasty place to get out of */
				return;
		}
	}
	nptr->tot = 0;		/* clear list */			
	nptr->basenum = FF->head.rtot;		/* reset base number  */
}
/*****************************************************************************/
void sort_makenode(INDEX * FF, RECN num)               /* finds place in tree for record num */

{
	register RECORD *P, *curptr, *lastptr;
	register RECN rnum;
	short sign;

	if (P = rec_getrec(FF, num))	{
		P->lchild = P->rchild = 0;  /* clear links to other records */
		P->balance = 0;			/* starts balanced */
		if (curptr = rec_getrec(FF,FF->head.root)) {	  /* if tree already has entries */
			do  {       		/* select next record in tree */
				if ((sign = match(FF, &FF->head.sortpars, P->rtext, curptr->rtext)) < 0)	 /* get direction of shift */
					rnum = curptr->lchild;
				else if (sign > 0)
					rnum = curptr->rchild;
				else
					rnum = P->num < curptr->num ? curptr->lchild : curptr->rchild;  /* go by record number */
				lastptr = curptr;
			} while (curptr = rec_getrec(FF,rnum));
			curptr = lastptr;
			if (!sign)      		/* if matched exactly */
				sign = P->num < curptr->num ? -1 : 1;   /* select correct child */
			if (sign > 0)
				curptr->rchild = P->num;
			else if (sign < 0)
				curptr->lchild = P->num;
			P->parent = curptr->num;      /* install thisrec as parent of new record */
			
			do  {          /* set new balance on node and rebalance as necessary */
				register unsigned short bshift;
	
				bshift = (P->num == curptr->rchild) ? RRIGHT : LLEFT;		/* set unbalance flag */
				if (curptr->balance == bshift)  {  /* if would be too unbalanced */
					balance(FF,curptr, bshift);		    /* balance it and exit */
					return;
				}
				curptr->balance = curptr->balance ? 0 : bshift;   /* shift balance */
				P = curptr;		    /* this becomes new child */
			} while (P->balance && (curptr = rec_getrec(FF,P->parent)));  /* while nodes are unbalanced and not at root */
		}
	
		else    {		    /* no entries, so make new root */
			P->parent = 0;
			FF->head.root = P->num;
		}
		index_markdirty(FF);
	}
}
/*****************************************************************************/
RECN sort_remnode(INDEX * FF, RECN num)          /* removes node for record num. Replaces children, if any
			at right places; rebalances tree */

{
	register RECORD * P, *Q, *curptr, *lastptr;
	unsigned short bshift;
	RECN newchild, startbal;

	if (P = rec_getrec(FF,num))	{
		if (P->rchild && P->lchild) {     /* if our rec has two children */
			Q = rec_getrec(FF,P->lchild);	       /* take left child */
			while (lastptr = rec_getrec(FF,Q->rchild))   /* find first record without rchild in tree above old lchild */
				Q = lastptr;
			bshift = RRIGHT;	/* assume that Q is lchild of P (unbalance P's parent to right) */
			startbal = Q->num;	/* and will start balancing at Q after it's moved */
			if (Q->parent != P->num) {       /* if replacement is not lchild of P */
				bshift = LLEFT;	  /* removal of Q will unbalance its parent to left */
				if (curptr = rec_getrec(FF,Q->lchild))   {	     /* if replacement record has lchild */
					curptr->parent = Q->parent;     /* give the lchild a new parent */
				}
				curptr = rec_getrec(FF,Q->parent);		/* get Q's old parent */
				curptr->rchild = Q->lchild;	/* give it new rchild (if any) */
				startbal = curptr->num; 	/* will start balancing from here (Q's old parent) */
				Q->lchild = P->lchild;	/* now put replacement record in new place */
				curptr = rec_getrec(FF,Q->lchild);		/* get new lchild */
				curptr->parent = Q->num;	/* give it new parent */
			}
			Q->rchild = P->rchild;
			curptr = rec_getrec(FF,Q->rchild);	/* fetch new rchild */
			curptr->parent = Q->num;	/* give new parent */
			Q->parent = P->parent;	 /* give replacement rec new parent */
			Q->balance = P->balance;    /* initially give it old balance (may change) */
			newchild = Q->num;	/* Q is new child of original parent */
		}				
	
		else {		     /* must be one or no child */
			if (P->lchild)	{	/* if our rec has only lchild */
				newchild = P->lchild;   /*	P's parent gets P's lchild */
				curptr = rec_getrec(FF,P->lchild);	/* get the child */
				curptr->parent = P->parent;	    /* modify the child */
			}
			else if (P->rchild) {		  /* else if our record has only rchild */
				newchild = P->rchild;   /*	P's parent gets rchild */
				curptr = rec_getrec(FF,P->rchild);	/* get the child */
				curptr->parent = P->parent;	    /* modify the child */
			}
			else			 /* if P has no children */
				newchild = 0;          /* P's parent just loses P */
	
			startbal = P->parent;		  /* start balancing at record that replaced P */
		}
		if (curptr = rec_getrec(FF,P->parent))  {	   /* if P isn't root */
			if (!(P->rchild && P->lchild))		/* if P didn't have 2 children */
				bshift = (curptr->lchild == P->num) ? RRIGHT : LLEFT;	/* set bshift */
			if (curptr->lchild == P->num)      /* give its original parent a new child */
				curptr->lchild = newchild;
			else 
			    curptr->rchild = newchild;
		}
		else		    /* else set new root */
			FF->head.root = newchild;
	
	
		if (curptr = rec_getrec(FF,startbal)) {	     /* if not already at root */
			do  {		   /* set new balance on node and rebalance as necessary */
				if (curptr->balance == bshift)	 /* if would be too unbalanced */
					curptr = balance(FF,curptr, bshift);		    /* balance it */
				else {
					curptr->balance = curptr->balance ? 0 : bshift;   /* shift balance */
				}
				P = curptr;
				if (curptr = rec_getrec(FF,P->parent))
					bshift = (P->num == curptr->rchild) ? LLEFT : RRIGHT;		 /* change balance flag appropriately */
			}  
			while (!P->balance && P->parent);  /* while nodes are balanced and not at root */
		}
	}
	return (num);
}
/*****************************************************************************/
static RECORD * balance(INDEX * FF, register RECORD * P, unsigned short bshift)       /* balances subtree on current record
			    in direction away from bshift */

{
	register RECORD *Q, *curptr;

	Q = rec_getrec(FF, bshift == RRIGHT ? P->rchild : P->lchild);	   /* get child from direction of imbalance */
	if (bshift == Q->balance) {    /* if child unbalanced in same direction */
		if (bshift == RRIGHT)
			lrotate(FF,P);
		else
			rrotate(FF,P); 	/* simple rotation */
		P->balance = 0; 	/* fix balances */
		Q->balance = 0; 	/* any rotation of P will mark Q as modified */
	}
	else if (!Q->balance)   {	/* child balanced - only happens through deletion */
		if (bshift == RRIGHT)
			lrotate(FF,P);
		else
			rrotate(FF,P);
		P->balance = bshift;
		Q->balance = ~bshift;
	}
	else {			/* else must be unbalanced in opposite directions */
		if (bshift == RRIGHT) {	  /* if want left rotation */
			rrotate(FF, Q);     /* right rotate Q */
			lrotate(FF, P);     /* left rotate P */
		}
		else  { 	    /* else must want right rotation */
			lrotate(FF, Q);
			rrotate(FF, P);
		}
		curptr = rec_getrec(FF, P->parent);		/* get new parent of P */
		if (!curptr->balance)   {	 /* if was originally balanced */
			P->balance = 0;
			Q->balance = 0;	/* clear balances on rotated subtrees */
		}
		else if (curptr->balance == bshift) {      /* if new parent was originally unbalanced in same direction as P */
			P->balance = ~bshift;
			Q->balance = 0;
		}
		else {		    /* parent was unbalanced in opp direction from P */
			P->balance = 0;
			Q->balance = bshift;
		}
		curptr->balance = 0;	/* new parent is always balanced */
	}
	return (rec_getrec(FF,P->parent));		/* leaves pointing at new root of subtree */
}
/*****************************************************************************/
static void lrotate(INDEX * FF, RECORD * X)  /* rotates to left the subtree based on record X
		assumes X locked on entry */

{
	register RECORD * curptr, * newptr;
	
	curptr = rec_getrec(FF, X->rchild);			/* get its right child */
	curptr->parent = X->parent;	/* old rchild of X takes old parent of X */
	X->rchild = curptr->lchild;	/* insert as rchild any lchild of old rchild */
	curptr->lchild = X->num;	/* X now lchild of old rchild */
	X->parent = curptr->num;	/* insert new parent of X */
	if (!(newptr = rec_getrec(FF, curptr->parent)))	/* get old parent of X */
		FF->head.root = curptr->num;    /* make new root if none */
	else    {
		if (X->num == newptr->rchild)   /* if X was rchild */
			newptr->rchild = X->parent; /* insert new parent as rchild of old parent */
		else				    /* else */
			newptr->lchild = X->parent; /* insert new parent as lchild */
	}
	if (curptr = rec_getrec(FF, X->rchild)) 		/* if X has a new rchild */
		curptr->parent = X->num;	/* give it new parent */
}
/*****************************************************************************/
static void rrotate(INDEX * FF, RECORD * X)  /* rotates to right the subtree based on record X
		assumes X locked on entry */

{
	register RECORD * curptr, * newptr;

	curptr = rec_getrec(FF,X->lchild);			/* get its left child */
	curptr->parent = X->parent;	/* old lchild of X takes old parent of X */
	X->lchild = curptr->rchild;	/* insert as lchild any rchild of old lchild */
	curptr->rchild = X->num;	/* X now rchild of old lchild */
	X->parent = curptr->num;	/* insert new parent of X */
	if (!(newptr = rec_getrec(FF,curptr->parent)))	/* get old parent of X */
		FF->head.root = curptr->num;    /* make new root if none */
	else {
		if (X->num == newptr->lchild)   /* if X was lchild */
			newptr->lchild = X->parent; /* insert new parent as lchild of old parent */
		else				    /* else */
			newptr->rchild = X->parent; /* insert new parent as rchild */
	}
	if (curptr = rec_getrec(FF,X->lchild)) 		/* if X has a new lchild */
		curptr->parent = X->num;	/* give it new parent */
}
/*****************************************************************************/
static short match(INDEX * FF, SORTPARAMS * sgp, char *s1, char *s2)	 /* compares record strings by sort rules */

{
	short sign, pagedone, skipindex, lastchance;
	CSTR s1array[FIELDLIM], s2array[FIELDLIM];	/* pointers to component strings */
	short s1tot, s2tot, count, fcount, lindex, findex, maxcount, tokens, s1lindex, s2lindex;
	char * c1ptr, *c2ptr;

	s1tot = str_xparse(s1, s1array);
	s2tot = str_xparse(s2, s2array);
	for (s1lindex = s1tot - 2; s1lindex && !s1array[s1lindex].ln; s1lindex--)		// find index of last non-empty text field
		;
	for (s2lindex = s2tot - 2; s2lindex && !s2array[s2lindex].ln; s2lindex--)		// find index of last non-empty text field
		;
	maxcount = (s2lindex < s1lindex ? s2lindex : s1lindex) + 2;		// max # fields to compare (up to first non-empty + page)
	lindex = s2lindex > s1lindex ? s2lindex : s1lindex;		// index of last non-empty text field in longer record
	skipindex = sgp->skiplast ? (s1tot > s2tot ? s1tot-2 : s2tot-2) : 0;		// index of field to ignore 
	for (lastchance = fcount = count = pagedone = 0; count < maxcount && (findex = sgp->fieldorder[fcount]) >= 0; fcount++, count++)	{	/* for all strings */
		if (findex != PAGEINDEX && (count < maxcount-1 || pagedone))	{	/* a text field to examine */
			if (findex < maxcount-1 || FF->head.indexpars.required && findex == FF->head.indexpars.maxfields-2)	{		/* if requested field in both records */
				int f1index = findex, f2index = findex;
				if (FF->head.indexpars.required)	{	// if have required fields
					if (findex == FF->head.indexpars.maxfields-2) {	// if findex is that of required field
						f1index = s1tot-2;		// set to compare last fields
						f2index = s2tot-2;
					}
					else if (findex == s1tot-2 || findex == s2tot-2)	{	// if findex hits last field in at least one record
						int rindex;
						if (s1tot != s2tot)		// if have diff # fields
							return s1tot-s2tot;		// return diff in number of fields
						// this loop could be put in earlier-called function, so that we could store flag
						for (rindex = 0; rindex < FF->head.indexpars.maxfields-1; rindex++)	{	// check whether required field in sort list
							if (sgp->fieldorder[rindex] == FF->head.indexpars.maxfields-2)		// if required field in list
								break;		// found it
						}
						if (rindex == FF->head.indexpars.maxfields-1)	// if aren't using required field
							return s1tot-s2tot;		// return difference in number of fields
					}
				}
				if (sign = col_match(FF, sgp, s1array[f1index].str, s2array[f2index].str, findex ? MATCH_CHECKPREFIX : FALSE)){ /* if can compare && differ */
					if (!skipindex || findex != skipindex)	/* if not skipping last field */
						return (sign);
					lastchance = sign;	/* save result in case need to use later */
				}
			}
			else if (findex <= lindex)		/* if requested field in one only record */
				return (s1lindex-s2lindex);		/* just compare field counts */
			else        				/* not compared any fields, so discount the field increment */
				count--;
		}
		else if (!pagedone) {		// should compare page fields or handle mismatched number
			char c1, c2;

			c1 = (char)str_crosscheck(FF,s1array[s1tot-1].str);
			c2 = (char)str_crosscheck(FF,s2array[s2tot-1].str);
			if ((s1lindex == s2lindex || findex == PAGEINDEX) /* && !pagedone */) {	// if should compare page fields
				if (!c1 && !c2)	{		/* neither is cross ref */
					s1 = ref_sortfirst(FF,s1array[s1tot-1].str);	  /* get component with lowest numbers */
					s2 = ref_sortfirst(FF,s2array[s2tot-1].str);
					if ((sign = ref_match(FF,s1, s2, sgp->partorder, PMEXACT|PMSENSE|PMSTYLE)))
						return (sign);
				}
				else if (c1 && c2)	{			/* if both cross refs, check text */
					c1ptr = str_skiplist(s1array[s1tot-1].str, FF->head.refpars.crosstart,&tokens);
					c2ptr = str_skiplist(s2array[s2tot-1].str, FF->head.refpars.crosstart,&tokens);
					if (sign = sort_crosscompare(FF, sgp, c1ptr,c2ptr))	/* if cross-refs differ */
						return (sign);
					/* if same in body, check for different length prefixes */
					if (c1ptr-s1array[s1tot-1].str != c2ptr-s2array[s2tot-1].str)
						return ((c1ptr-s1array[s1tot-1].str) - (c2ptr-s2array[s2tot-1].str));
				}
				else	// one is cross
					return (c1 ? 1 : -1);
//				pagedone = TRUE;		/* done the page field */
			}
			else	{	//	mismatched # fields
				if (maxcount == 2 && !sgp->fieldorder[0] && FF->head.formpars.ef.cf.mainposition >= CP_LASTSUB	/* if cref from main heading should be after subheads */
					|| maxcount > 2 && count == maxcount-1 && FF->head.formpars.ef.cf.subposition >= CP_LASTSUB)	{	/* if cref from subheading should be after subsubs */
					if (c1 && s1lindex <= s2lindex || c2 && s2lindex <= s1lindex)	/* if cross-ref in shorter record */
						return (s2lindex-s1lindex);
				}
//				return (s1lindex-s2lindex);
			}
			pagedone = TRUE;		/* done the page field */
		}
	}
	if (s1lindex != s2lindex)	// if mismatched # fields
		return s1lindex-s2lindex;	// shortest record is first
	return (lastchance);	// return any difference from skipped last field
}
/*****************************************************************************/
short sort_crosscompare(INDEX * FF, SORTPARAMS * sgp, char *s1, char *s2) /* compares cross-refs */

{
	short tokens1, tokens2;
	
	str_skiplist(s1, FF->head.refpars.crossexclude, &tokens1);	 /* count general terms in first */
	str_skiplist(s2, FF->head.refpars.crossexclude, &tokens2);	 /* count general terms in second */
	if (tokens1 != tokens2)			/* only one has a general term */
		return (tokens1-tokens2);	/* one with more tokens will be higher */
	return (col_match(FF, sgp, s1, s2, 0));
}
/*****************************************************************************/
void sort_setfilter(INDEX * FF, int filter)	// configures transfer filter

{
	if (filter == SF_VIEWDEFAULT)	{
		if (FF->head.privpars.vmode == VM_FULL)	{	// full format
			FF->head.privpars.hidedelete = TRUE;
			FF->head.privpars.filterenabled = TRUE;
		}
		else {	// draft or unformatted default is no filtering
			FF->head.privpars.hidedelete = FALSE;
			FF->head.privpars.filterenabled = FALSE;
		}
	}
	else if (filter == SF_HIDEDELETEONLY)	{		// hide only deleted
		FF->head.privpars.hidedelete = TRUE;
		FF->head.privpars.filterenabled = FALSE;
	}
	else if (filter == SF_OFF)	{	// no filtering of any kind
		FF->head.privpars.hidedelete = FALSE;
		FF->head.privpars.filterenabled = FALSE;
	}
}
/*****************************************************************************/
BOOL sort_isignored(INDEX * FF, RECORD * recptr)	// returns TRUE if record ignored

{
	if (FF->head.privpars.hidedelete)	{	//  hiding deleted permitted outside filter
		if (recptr->isdel || !*recptr->rtext)	// always ignore deleted & empty
			return TRUE;
		if (FF->head.privpars.filterenabled && FF->head.privpars.filter.on)	{	// if filtering enabled & on
			if (FF->head.privpars.filter.label[recptr->label])	// if has wanted attrib
				return TRUE;	// ignore
		}
	}
	return FALSE;
}
/*****************************************************************************/
void sort_adjustrangetovisible(INDEX * FF, RECN * first, RECN * last)	// adjusts range to visible

{
	if (FF->head.privpars.vmode == VM_FULL)	{		//  need to check only if full view
		RECORD * firstptr = rec_getrec(FF,*first);
		RECORD * lastptr = rec_getrec(FF,*last);
		
		while (firstptr && sort_isignored(FF,firstptr))	{
			if (firstptr == lastptr)
				break;
			firstptr = sort_skip(FF,firstptr,1);
		}
		while (lastptr && sort_isignored(FF,lastptr))	{
			if (lastptr == firstptr)
				break;
			lastptr = sort_skip(FF,lastptr,-1);
		}
		*first = *last = 0;
		if (firstptr && !sort_isignored(FF, firstptr))
			*first = rec_number(firstptr);
		if (lastptr && !sort_isignored(FF, lastptr))
			*last = rec_number(lastptr);
	}
}
/*****************************************************************************/
RECORD * sort_top(INDEX * FF)	/* moves to extreme left end of tree */

{
	RECN newrec;
	register RECORD *curptr, *nextptr;

	if (FF->curfile) {			/* if working with group */
		if (FF->curfile->rectot)			{/* if have any records */
			newrec = FF->curfile->recbase[0];
			FF->curfilepos = 0;
			curptr = rec_getrec(FF,newrec);
		}
		else
			return (NULL);
	}
	else if (FF->head.sortpars.ison && FF->viewtype != VIEW_NEW) {
		if (!(curptr = rec_getrec(FF,FF->head.root)))	/* if no records */
			return (NULL);
		while (nextptr = rec_getrec(FF,curptr->lchild))	/* take all left children until none left */
			curptr = nextptr;
	}
	else		/* if sort is off or on hold */ 
		curptr = rec_getrec(FF,FF->viewtype == VIEW_NEW ? FF->startnum+1 :1);
//	if (curptr && (curptr->isdel || !*curptr->rtext) && FF->head.privpars.hidedelete)	/* if want to skip deleted recs */
	if (curptr && sort_isignored(FF,curptr))	/* if want to skip deleted recs */
		curptr = sort_skip(FF,curptr,1);	
	return (curptr);
}
/*****************************************************************************/
RECORD * sort_bottom(INDEX * FF)	/* moves to extreme right-hand end of tree */

{
	register RECORD *curptr, *nextptr;
	RECN newrec;
	
	if (FF->curfile) {			/* if working with group */
		if (FF->curfile->rectot)		{	/* if have any records */
			newrec = FF->curfile->recbase[FF->curfile->rectot-1];
			FF->curfilepos = FF->curfile->rectot-1;
			curptr = rec_getrec(FF,newrec);
		}
		else
			return (NULL);
	}
	else if (FF->head.sortpars.ison && FF->viewtype != VIEW_NEW) {	/* normal sorted */
		if (!(curptr = rec_getrec(FF,FF->head.root)))	/* if no records */
			return (NULL);
		while (nextptr = rec_getrec(FF,curptr->rchild))	/* take all right children until none left */
			curptr = nextptr;
	}
	else		/* if sort is off or on hold */
		curptr = rec_getrec(FF,FF->head.rtot);
//	if (curptr && (curptr->isdel || !*curptr->rtext) && FF->head.privpars.hidedelete)	/* if want to skip deleted recs */
	if (curptr && sort_isignored(FF,curptr))	/* if want to skip deleted recs */
		curptr = sort_skip(FF,curptr,-1);	
	return (curptr);
}
/******************************************************************************/
RECORD *sort_skip(INDEX * FF, register RECORD *curptr, short n)	/* returns pointer to record +/-n from current */

{
	register RECN currec;
	register RECORD * nextptr;
	RECN newrec;

	if (FF->curfile) {			/* if working from a file list */
		if (FF->curfilepos >= FF->curfile->rectot)
			FF->curfilepos = 0;
		if (FF->curfilepos >= FF->curfile->rectot || curptr->num != FF->curfile->recbase[FF->curfilepos])	{
			/* if bad starting record or it's not the one last delivered */
			for (FF->curfilepos = 0; FF->curfilepos < FF->curfile->rectot; FF->curfilepos++)
				if (curptr->num == FF->curfile->recbase[FF->curfilepos])	/* if found it */
					break;
			if (FF->curfilepos == FF->curfile->rectot)	{	/* ERROR! record isn't in group */
				FF->curfilepos = 0;
				return (NULL);
			}
		}
		do {
			if ((FF->curfilepos += n) < FF->curfile->rectot)	{	/* NB: since it's unsigned, < 0 puts out of range */
				newrec = FF->curfile->recbase[FF->curfilepos];
				curptr = rec_getrec(FF,newrec);
			}
			else	{
				curptr = NULL;
				FF->curfilepos = 0;
			}
//		} while (curptr && (curptr->isdel || !*curptr->rtext) && FF->head.privpars.hidedelete); 
		} while (curptr && sort_isignored(FF,curptr)); 
	}
	else if (FF->head.sortpars.ison && FF->viewtype != VIEW_NEW) {	  /* if sort is not disabled */
		if (n > 0)	{
//			while (n > 0 || (curptr->isdel || !*curptr->rtext) && FF->head.privpars.hidedelete) {
			while (n > 0 || sort_isignored(FF,curptr)) {
				if (!(nextptr = rec_getrec(FF,curptr->rchild))) {      /* if no right child to got to */
					do  {
						currec = curptr->num;		/* save number of this record */
						if (!(nextptr = rec_getrec(FF,curptr->parent)))  {   /* if at bottom through rchildren */
							return (NULL);				/* means no more records to skip */
						}	
						curptr = nextptr;
					} while (currec == curptr->rchild);    /* while passing down tree from right */
				}					/* now at first parent not entered from lchild */
				else	{			/* with right child */
					curptr = nextptr;
					while (nextptr = rec_getrec(FF,curptr->lchild))    /* while can move up left branch */
						curptr = nextptr;                   /* move up */
				}
				n--;
			}
		}
		else {
//			while (n < 0 || (curptr->isdel || !*curptr->rtext) && FF->head.privpars.hidedelete)  {	/* while recs to skip and not at root */
			while (n < 0 || sort_isignored(FF,curptr))  {	/* while recs to skip and not at root */
				if (!(nextptr = rec_getrec(FF,curptr->lchild))) {      /* if no left child to got to */
					do {
						currec = curptr->num;		/* save number of this record */
						if (!(nextptr = rec_getrec(FF,curptr->parent)))  	{ /* if get to bottom while passing down lchildren */
							return (NULL);
						}
						curptr = nextptr;				/* means no more records to skip */
					} while (currec == curptr->lchild);	/* while passing down tree from left */
				}				   /* now at first parent not entered from lchild */
				else	{					 /* with left child */
					curptr = nextptr;
					while (nextptr = rec_getrec(FF,curptr->rchild))    /* while can move up right branch */
						curptr = nextptr;                     /* move up */
				}
				n++;
			}
		}
	}
	else {		/* must be unsorted */
		if (FF->viewtype != VIEW_NEW || curptr->num+n > FF->startnum)	{	/* if looking at new records & OK */
			do {
				curptr = rec_getrec(FF,curptr->num+n);
				n = n < 0 ? -1 : 1;			/* set up for possible skipping of deleted records */
//			} while (curptr && (curptr->isdel || !*curptr->rtext) && FF->head.privpars.hidedelete);
			} while (curptr && sort_isignored(FF,curptr));
			if (FF->viewtype == VIEW_NEW && curptr && curptr->num <= FF->startnum)	/* if need new record and gone back too far */
				curptr = NULL;
		}
		else
			curptr = NULL;
	}
	return (curptr);			  /* pointer to text of next rec */
}
/*****************************************************************************/
RECN sort_viewindexforrecord(INDEX * FF, RECN record)	// gets record index for actual record

{
	RECORD * curptr;
	RECN vindex;
	
	for (vindex = 0, curptr = sort_top(FF); curptr; curptr = sort_skip(FF,curptr,1), vindex++)	{
		if (curptr->num == record)
			return vindex;
	}
	return 0;
}
/*****************************************************************************/
RECORD * sort_recordforviewindex(INDEX * FF, RECN rindex)	// gets record for record index

{
	RECORD * curptr;
	RECN vindex;
	
	for (vindex = 0, curptr = sort_top(FF); curptr; curptr = sort_skip(FF,curptr,1), vindex++)	{
		if (vindex == rindex)
			return curptr;
	}
	return NULL;
}
#if 0
/*****************************************************************************/
RECORD * sort_jump(INDEX * FF, RECN target)		/* moves to record at ordinal posn target */

{
	register RECORD * curptr;
	
	if (FF->curfile) {	/* if working from a file list */
		if (target < FF->curfile->rectot)	{
			RECN newrec = FF->curfile->recbase[target];
			FF->curfilepos = target;
			curptr = rec_getrec(FF, newrec);
		}
		else
			return (NULL);		/* asking for record that isn't in group */
	}
	else if (FF->head.sortpars.ison)	{		/* if sorted */
		register RECN cpos, basepos;
		register RECORD * nextptr;
		
		curptr = rec_getrec(FF, FF->head.root); 
		basepos = FF->head.rtot >> 1;		/* position starts at root (assumed half way through) */
		for (cpos = basepos; curptr && cpos != target; curptr = nextptr)	{	/* starting at root */
			basepos >>= 1;
			if (cpos > target)	{		/* if present posn too high */
				if (!(nextptr = rec_getrec(FF, curptr->lchild)))		/* if can't go lower */
					break;
				cpos -= basepos;
			}
			else	{	/*  present too low */
				if (!(nextptr = rec_getrec(FF, curptr->rchild)))
					break;
				cpos += basepos;
			}
		}
	}
	else
		curptr = rec_getrec(FF, target);	// direct addressing
//	if (FF->head.privpars.hidedelete && curptr && curptr->isdel)	// if deleted and hiding
	if (curptr && sort_isignored(FF,curptr))	// if deleted and hiding
		curptr = sort_skip(FF, curptr,1);		// skip to visible one
	return curptr; 	/* get record */
}	
/*****************************************************************************/
RECN sort_findpos(INDEX * FF, RECN target)	/* finds ordinal position of target in index */

{
	RECN curpos, basepos, basesize;
	register short m;
	RECORD *curptr, *tptr;
	RECN mask;

	if (FF->curfile) {	/* if working from a file list */
		for (curpos = 0; curpos < FF->curfile->rectot; curpos++)	{
			if (target == FF->curfile->recbase[curpos])	{	/* if found it */
				FF->curfilepos = curpos;
				return (curpos);
			}
		}
		return (0);
	}
	if (FF->head.sortpars.ison && FF->viewtype != VIEW_NEW)	{
		if (tptr = rec_getrec(FF,target))	{
			basepos = FF->head.rtot;
#if 1
			for (mask = (unsigned long)~0; basepos & mask; mask <<= 1)		/* mask to largest power of 2 */
				basepos &= mask;
#else
			basepos >>=1;
#endif
			for (curpos = basesize = basepos, curptr = rec_getrec(FF, FF->head.root); curptr;) {       /* while more records to check */
				if (!(m = match(FF,&FF->head.sortpars,tptr->rtext, curptr->rtext)))		{	/* if new string == old */
					if (curptr->num == target)		/* if found the one we want */
						break;	/* out */
				}
				basepos >>= 1;
				if (m < 0 || m == 0 && curptr->num > tptr->num)	{	/* need to go to left of parent */
					curpos -= basepos;
					curptr = rec_getrec(FF,curptr->lchild);
				}
				else if (m > 0 || m == 0 && curptr->num < tptr->num) {		/* to right */
					curpos += basepos;
					curptr = rec_getrec(FF,curptr->rchild);
				}
			}
#if 1
			return (RECN)(((float)curpos*FF->head.rtot)/(basesize<<1));
#else
			return (curpos);
#endif
		}
		return (0);		/* can't get record */
	}
	else		/* no sort (or new displaying new) */
		return (FF->viewtype == VIEW_NEW ? target- FF->startnum : target);
}
#else
/*****************************************************************************/
RECORD * sort_jump(INDEX * FF, float position)		/* moves to record at relative posn position */

{
	RECORD * curptr;
	RECN target;
	
	// should never be called with position == 0 or position == 1
	
	if (FF->curfile) {	/* if working from a file list */
		target = position*FF->curfile->rectot;
		if (target < FF->curfile->rectot)	{
			RECN newrec = FF->curfile->recbase[target];
			FF->curfilepos = target;
			curptr = rec_getrec(FF, newrec);
		}
		else
			return (NULL);		/* asking for record that isn't in group */
	}
	else if (FF->viewtype == VIEW_NEW)	{
		target = FF->startnum + position*(FF->head.rtot-FF->startnum);
		curptr = rec_getrec(FF, target);	// direct addressing
	}
	else if (FF->head.sortpars.ison)	{		/* if sorted */
		register RECN cpos, basepos;
		register RECORD * nextptr;
		
		target = position*FF->head.rtot;
		curptr = rec_getrec(FF, FF->head.root);
		basepos = FF->head.rtot >> 1;		/* position starts at root (assumed half way through) */
		for (cpos = basepos; curptr && cpos != target; curptr = nextptr)	{	/* starting at root */
			basepos >>= 1;
			if (cpos > target)	{		/* if present posn too high */
				if (!(nextptr = rec_getrec(FF, curptr->lchild)))		/* if can't go lower */
					break;
				cpos -= basepos;
			}
			else	{	/*  present too low */
				if (!(nextptr = rec_getrec(FF, curptr->rchild)))
					break;
				cpos += basepos;
			}
		}
	}
	else	{
		target = position*FF->head.rtot;
		curptr = rec_getrec(FF, target);	// direct addressing
	}
	if (curptr && sort_isignored(FF,curptr))	{	// if would be hidden
		if (curptr == sort_bottom(FF))			// if at bottom
			return sort_skip(FF, curptr, -1);	// skip to first prior visible
		curptr = sort_skip(FF, curptr,1);		// skip to visible one
	}
	return curptr; 	/* get record */
}
/*****************************************************************************/
float sort_findpos(INDEX * FF, RECN target)	/* finds ordinal position of target in index */

{
	RECN curpos, basepos, basesize;
	int m;
	RECORD *curptr, *tptr;
	RECN mask;
	
	if (FF->curfile) {	/* if working from a file list */
		for (curpos = 0; curpos < FF->curfile->rectot; curpos++)	{
			if (target == FF->curfile->recbase[curpos])	{	/* if found it */
				FF->curfilepos = curpos;
				return ((float)curpos/FF->curfile->rectot);
			}
		}
		return (0);
	}
	if (FF->viewtype == VIEW_NEW)
		return (float)(target-FF->startnum)/(FF->head.rtot-FF->startnum);
	if (FF->head.sortpars.ison)	{
		if (tptr = rec_getrec(FF,target))	{
			basepos = FF->head.rtot;
			for (mask = (unsigned long)~0; basepos & mask; mask <<= 1)		/* mask to largest power of 2 */
				basepos &= mask;
			for (curpos = basesize = basepos, curptr = rec_getrec(FF, FF->head.root); curptr;) {       /* while more records to check */
				if (!(m = match(FF,&FF->head.sortpars,tptr->rtext, curptr->rtext)))		{	/* if new string == old */
					if (curptr->num == target)		/* if found the one we want */
						return (float)curpos/(basesize<<1);
				}
				basepos >>= 1;
				if (m < 0 || m == 0 && curptr->num > tptr->num)	{	/* need to go to left of parent */
					curpos -= basepos;
					curptr = rec_getrec(FF,curptr->lchild);
				}
				else if (m > 0 || m == 0 && curptr->num < tptr->num) {		/* to right */
					curpos += basepos;
					curptr = rec_getrec(FF,curptr->rchild);
				}
			}
		}
		return (0);		/* can't get record */
	}
	// unsorted
	return ((float)target/FF->head.rtot);
}
#endif
/*****************************************************************************/
short sort_relpos(INDEX * FF, RECN t1, RECN t2)		/* finds relative positions of recs in index */

{
	RECORD *trp1, *trp2;
	short sign;
	RECN curpos, newrec;
	
	if (t1 != t2)	{
		if (FF->curfile) {	/* if working from a file list */
			for (curpos = 0; curpos < FF->curfile->rectot; curpos++)	{
				newrec = FF->curfile->recbase[curpos];
				if (newrec == t1 || newrec == t2)
					break;
			}
			return (newrec == t1 ? -1 : 1);
		}
		if (FF->head.sortpars.ison && FF->viewtype != VIEW_NEW)	{	/* if sort is on (& not showing new) */
			if (trp1 = rec_getrec(FF,t1))	{
				if (trp2 = rec_getrec(FF,t2))
					if (sign = match(FF,&FF->head.sortpars,trp1->rtext, trp2->rtext))	 /* get direction of diff */
						return (sign);
			}
		}
		return (t1 > t2 ? 1 : -1);
	}
	return (0); 
}
/*******************************************************************************/
BOOL sort_isinfieldorder(short * fieldorder, short maxfields)	/* returns TRUE if straight field order for text fields */
		/* can contain page field anywhere in the list */
{
	int count, index;

	for (index = count = 0; count < maxfields; count++, index++)	{	/* for all text fields */
		if (fieldorder[count] == PAGEINDEX)	/* if page field is in list */
			index--;			/* drop one from index for subsequent passes */
		else if (index != fieldorder[count])
			return (FALSE);
	}
	return (TRUE);	/* check only that last field is enabled */
}
/******************************************************************************/
void sort_buildfieldorder(short * fieldorder, short oldlimit, short limit)	/* builds straight text field order */
		/* retains existing page field anywhere in the list */

{
	int count, index;

	for (index = count = 0; count < limit; count++, index++)	{	/* set simple order for text fields */
		if (fieldorder[count] == PAGEINDEX && count < oldlimit-1)	/* if page field is not last in current list */
			index--;			/* preserve it, drop one from index for subsequent passes */
		else		
			fieldorder[count] = index;
	}
	if (index == count)		/* if haven't yet done page field */
		fieldorder[count-1] = PAGEINDEX;	/* add page field */
	fieldorder[count] = -1;			/* and terminator */
}
/*****************************************************************************/
void sort_squeeze(INDEX * FF, short flags)		/* squeezes index */

{
	char *m, *mark;
	RECN rnum, delcnt, smark, otot;
	RECORD * lastptr, *thisptr, *newptr, *prevptr;
	short thislen, addlen, lcross, tokens;
	char *opptr, *npptr, sep, *thispage, *lastpage;
	char trtext[MAXREC];

	/* first stage is to tag records for removal */

//	delstat = FF->head.privpars.hidedelete;
//	FF->head.privpars.hidedelete = FALSE;	/* enable deleted records during first pass */
	sort_setfilter(FF,SF_OFF);
	err_eflag = FALSE;						/* clear error flag */
	index_cleartags(FF);	// clear tags
	if (!(flags&SQSINGLE))	{	/* if need to scan for tagging */
		if (thisptr = sort_top(FF))	{
			for (rnum = 0, prevptr = lastptr = NULL; thisptr; thisptr = sort_skip(FF, thisptr,1)) {    /* for all records in index */
				showprogress(PRG_CHECKING,FF->head.rtot,++rnum);
//				if (thisptr->isttag)		/* error check: should never be tagged on entry */
//					thisptr->isttag = FALSE;	/* untag it */
				if (thisptr->isdel && flags&SQDELDEL			/* if remove deleted */
						|| thisptr->isgen && flags&SQDELGEN		/* or remove generated */
						|| !*thisptr->rtext && flags&SQDELEMPTY	/* or remove empty */
						|| prevptr && flags&SQDELDUP && !str_xcmp(thisptr->rtext, prevptr->rtext) && !prevptr->isdel)	{	// or duplicate of undeleted prev record
					thisptr->isttag = TRUE;	/* tag it */
				}
				else if (flags&SQCOMBINE && lastptr)	{	/* if combining & have prev good record */
					lastpage = str_xlast(lastptr->rtext);	/* get base of page field */
					thispage = str_xlast(thisptr->rtext);	/* get base of page field */
					thislen = thispage-thisptr->rtext;	 	/* length up to page field */
					lcross = str_crosscheck(FF, lastpage);	/* find if dealing with cross-refs */
					if (lcross == str_crosscheck(FF, thispage) && thislen == lastpage-lastptr->rtext && !memcmp(thisptr->rtext,lastptr->rtext,thislen) /* if identical to ref */
							&& lastptr->isdel == thisptr->isdel && (lastptr->label == thisptr->label || flags&SQIGNORELABEL))	{	// and del/label status is the same
						mark = lastpage+strlen(lastpage);	/* end of last page field */
						if (lcross)		/* if dealing with cross-refs */
							thispage = str_skiplist(thispage,FF->head.refpars.crosstart,&tokens);
						addlen = strlen(thispage); 
						if (FF->head.indexpars.recsize - (mark-lastptr->rtext) - 2  > addlen) {  /* if avail space greater then length to add */
							if (*lastpage)     /* if there are already some refs */
								*mark++ = lcross ? FF->head.refpars.csep : FF->head.refpars.psep;	/* insert a separator */
							strcpy(mark, thispage);		/* copy new pages on to end */
							mark += addlen+1;		/* point at new end */
							*mark = EOCS;              /* and terminate it */
							thisptr->isttag = TRUE;   /* mark deleted */
							str_adjustcodes(lastptr->rtext, CC_TRIM | CC_ONESPACE);	// clean up codes around page ref separators (11/28/20)
						}
					}
				}
//				if (!thisptr->isdel)	{	// if this wasn't deleted
				if (!thisptr->isdel || !(flags&SQDELDEL))	{	// if this wasn't deleted (or we're sparing deleted records)
					prevptr = thisptr;		// set base for comparing identical records for removal
					if (!thisptr->isttag) 		// if current record not tagged
						lastptr = thisptr;		// set base record for consolidation
				}
			}
			showprogress(0,0,0);	  /* kill message */
		}
	}
//	FF->head.privpars.hidedelete = delstat;		/* restore show del status */
	sort_setfilter(FF,SF_VIEWDEFAULT);

	/* now remove all tagged records and renumber remainder */
	if (!err_eflag)	{		/* if no errors (examined all records) */
		for (FF->head.root = 0, otot = FF->head.rtot, rnum = 1, delcnt = 0, smark = FF->startnum; thisptr = rec_getrec(FF,rnum); rnum++) {    /* for all records in index */
			showprogress(PRG_SORTING,FF->head.rtot,rnum);
			if (thisptr->isttag) {      /* if record tagged */
				delcnt++;			     /* do nothing to it, but increase del count */
				if (rnum <= smark)	   /* if before or at starting record */
					FF->startnum--;		 /* reset start number */
			}
			else {
				struct codeseq cs;
				char * baseptr;
				
				if (flags&SQSINGLE && rnum <= otot)   { /* if want one ref per record (& not done all) */
					str_xcpy(trtext,thisptr->rtext);	/* make temp copy in case resize changes rec position */
					m = str_xlast(trtext);		/* get base of page field */
					lcross = str_crosscheck(FF,m);
					sep = lcross ? FF->head.refpars.csep : FF->head.refpars.psep;
					while (*m == sep)       /* while leading separators */
						memmove(m,m+1,str_xlen(m));	/* shift over them */
					for (mark = m+strlen(m)-1;;)	{		/* until done */
						while (*mark == sep || *mark == SPACE)  /* discard trailing junk */
							*mark-- = '\0';		/* terminate original field here */
						*(mark+2) = EOCS;		/* and record */
						if (opptr = ref_last(m, sep))     {       /* if this isn't last refernce */
							mark = opptr++;
							type_balancecodes(m, str_skipcodes(opptr), &cs, FALSE);	// skip past any misplaced codes after separator; catch codes continuing into real start of last ref
							while (*opptr == SPACE)		/* skip any leading spaces */
								opptr++;
							newptr = rec_writenew(FF, trtext);		/* make new record */
							thisptr = rec_getrec(FF,rnum);	// get new pointer in case rec_writenew forced file resize
							if (newptr)	{
								str_adjustcodes(opptr,CC_ONESPACE|CC_TRIM);		// remove any dangling codes at start of locator (4/17/2017)
								newptr->isdel = thisptr->isdel;		// transfer deleted status
								newptr->label = thisptr->label;	// transfer label(s)
								npptr = newptr->rtext+(m-trtext);    /* set pointer to base of new page field */
								if (lcross)		/* if doing a cross-reference */
									npptr = str_skiplist(npptr,FF->head.refpars.crosstart,&tokens);		/* keep lead */
								baseptr = npptr;
								while (*npptr++ = *opptr++)	/* copy string */
									;
								npptr = type_matchcodes(baseptr,&cs, FF->head.indexpars.recsize-(npptr-newptr->rtext)-1);	/* insert/match the codes in the ref */
								*++npptr = EOCS;		/* terminate compound string */
								continue;
							}
							else
								flags = FALSE;		/* prevent further duplications */
						}
						else if (FF->head.indexpars.recsize >= str_xlen(trtext)+4)	{	/* if room to close on first ref */
							mark = type_balancecodes(m,mark+1,&cs, TRUE);	/* fix any unclosed codes */
							*++mark = EOCS;		/* terminate compound string */
						}
						break;
					}
//					thisptr = rec_getrec(FF,rnum);	/* recover original record in case resizing file has changed ptr */
					str_xcpy(thisptr->rtext,trtext);	/* put modified text */
				}
				thisptr->num = rnum-delcnt;
				rec_writerec(FF,thisptr);			/* write it in correct place */
				sort_makenode(FF, thisptr->num);	/* make node */
			}
		}
		FF->head.rtot = --rnum-delcnt;		/* revise number of recs in index */
		FF->head.squeezetime = time(NULL);	/* reset time stamp to invalidate groups */
		index_flush(FF);					/* this ensures that truncation catches right records */
		FF->lastedited = 0;					/* won't find last-edited record */
		type_trimfonts(FF);
	}
	showprogress(0,0,0);	  /* kill message */
}
