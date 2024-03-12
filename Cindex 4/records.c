#include "stdafx.h"
#include "errors.h"
#include "index.h"
#include "records.h"
#include "sort.h"
#include "util.h"
#include "strings.h"
#include "mfile.h"

/******************************************************************************/
RECN rec_number(RECORD * recptr)
{
	if (recptr)
		return recptr->num;
	return 0;
}
/******************************************************************************/
RECORD *rec_getrec(INDEX * FF, RECN n)	 /* returns pointer to record */

{
	RECORD * p;

	if (!n || n > FF->head.rtot || n > TOPREC)        /* if record doesn't exist */
		return (NULL);
	p = getaddress(FF,n);
	__try {
		if (p->num == n)	{
			char * list = p->rtext;		// base ptr
			char *eptr = list+FF->head.indexpars.recsize;	// limit ptr
			int fcount;

			for (fcount = 0; *list != EOCS &&  list < eptr ; fcount++)
				list += strlen(list++);
			if (list < eptr && fcount <= FF->head.indexpars.maxfields && fcount >= FF->head.indexpars.minfields)
				return p;
		}
		RaiseException(ERR_DAMAGEDRECORD,0,1,(ULONG_PTR*)&n);
	}
	__except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH){
		RaiseException(ERR_DAMAGEDRECORD,0,1, (ULONG_PTR*)&n);
	}
	return NULL;		// just to stop compiler warning
}
/******************************************************************************/
void rec_stamp(INDEX * FF, RECORD * recptr)   /* stamps record with date, initials */

{
	strncpy(recptr->user, g_prefs.hidden.user,4);	/* user id */
	recptr->time = time(NULL);		/* time made */
	if (!FF->head.dirty)	{	/* if haven't marked index as dirty */
		FF->head.dirty = TRUE;	/* index is dirty */
		index_writehead(FF);	/* write header marked as dirty */
	}
}
/******************************************************************************/
int rec_writerec(INDEX * FF, RECORD * p)		/* writes record */

{
	if (copymappeddata(getaddress(FF,p->num),p,FF->wholerec))
		return TRUE;
	senderr(ERR_RECWRITEERR,WARN,1,p->num);		/* some access error */
	return (FALSE);
}
/******************************************************************************/
RECORD * rec_writenew(INDEX * FF, char *rtext)   /* forms & writes new rec to index */

{
	RECORD * recptr;
	
	if (recptr = rec_makenew(FF,rtext,FF->head.rtot+1))	/* if can make new record */
		FF->head.rtot++;		/* increment total */
	return (recptr);
}
/******************************************************************************/
RECORD * rec_makenew(INDEX * FF, char * rtext, RECN num)   /* forms new record */

{
	if (FF->head.rtot < TOPREC)	{		/* if can accommodate the record */
		if (num < FF->recordlimit || index_setworkingsize(FF,MAPMARGIN))	{	/* if would be beyond end of file */
			RECORD * recptr = getaddress(FF,num);
			__try	{
				memset(recptr,0,sizeof(RECORD));
				recptr->num = num;  	/* assign number to record */
				str_xcpy(recptr->rtext,rtext);	/* copy text */
				rec_stamp(FF,recptr);			/* stamp it */
				return (recptr);			/* return error */
			}
			__except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION){
				senderr(ERR_RECWRITEERR,WARN,2,num);		/* some access error */
				return (NULL);
			}
		}
	}	
	senderr(ERR_INDEXFULLERR, WARN);		/* index is full */
	return (NULL);
}
/******************************************************************************/
unsigned short rec_propagate (INDEX * FF, RECORD * recptr, char * origptr, struct numstruct * nptr)		/* propagates changes to identical lower records */

{
	short ncount, slen, mlevel, olevel, nmax, omax, ntot;
	CSTR oarray[FIELDLIM], marray[FIELDLIM], narray[FIELDLIM];
	RECORD * nextptr;
	unsigned short proptot;
	
	omax = str_xparse(origptr, oarray)-1;	/* index of page field in original text */
	olevel = omax-1;	/* index of last text field in original text */
	mlevel = str_xparse(recptr->rtext, marray)-2;	/* index of last text field in mod text */
    proptot = 0;		/* counts records propagated */
    
	while (olevel >= 0 && mlevel >= 0)	{			/* work backwards from last fields */
		if (strcmp(marray[mlevel].str, oarray[olevel].str))		/* if fields differ */
			break;
		olevel--;
		mlevel--;
	}
	if (olevel >= 0 || mlevel >= 0)  { 	/* if records differ at all */
		nextptr = recptr;
		while (nextptr = sort_skip(FF,nextptr,1)) 	{		/* if a next record */
			ntot = str_xparse(nextptr->rtext,narray);
			nmax = ntot-1;
			if (nmax > omax)	/* max # of string comparisons is # text fields in shorter record */
				nmax = omax;
			for (ncount = 0; ncount < nmax && !strcmp(oarray[ncount].str, narray[ncount].str); ncount++)	  /* for all fields that match */
				;
			if (!ncount || ncount <= olevel) 	/* if don't match to some level below changes in base record */
				break;				   /* we don't want to change this one */
			slen = (marray[mlevel+1].str-marray[0].str)-(narray[olevel+1].str-narray[0].str);		/* net length change */ 	
			if (slen + str_xlen(nextptr->rtext) < FF->head.indexpars.recsize && mlevel-olevel+ntot <= FF->head.indexpars.maxfields)  {	/* if room & within field limit */
				str_xshift(narray[olevel+1].str, slen);
				memmove(narray[0].str, marray[0].str, marray[mlevel+1].str-marray[0].str);	 /* copy relevent part of xstring */
				rec_pad(FF,nextptr->rtext);
				sort_addtolist(nptr,nextptr->num);
				nextptr->ismark = FALSE;		// remove any existing mark
			}
			else		/* no room, so mark record */
				nextptr->ismark = TRUE;
			rec_stamp(FF,nextptr);
			proptot++;					/* count one done */
		}
	}
	return (proptot);
}
/******************************************************************************/
short rec_strip(INDEX * FF, char * pos)	  /* strips empty strings from record; to min */

{
	CSTR fields[FIELDLIM];
	int fcount = str_xparse(pos, fields);
	int index, used, count;
	char * base;

	for (index = 0, used = 1; index < fcount-1; index++)	{	// from main head through last subhead (count 1 used for page)
		if (fields[index].ln || FF->head.indexpars.required && index == fcount-2)	// if used or required
			used++;
	}
	if (used < FF->head.indexpars.minfields)
		used = FF->head.indexpars.minfields;
	count = fcount-used;	/* number of fields we can remove */
	while (*pos != EOCS && count > 0)	{	/* for all components */
		for (base = pos; !*base && *(base+1) != EOCS && count; base++)	/* while dud/empty strings */
			count--;
		if (base > pos)			{/* if any empty string to remove */	
			memmove(pos,base,str_xlen(base)+1);	/* remove it */
			fcount--;		// count one field removed
		}
		else
			pos += strlen(pos++);
	}
	return (fcount);
}
/******************************************************************************/
void rec_pad(INDEX *FF, char *string)	  /* expands xstring to min fields */

{
	CSTR fields[FIELDLIM];
	int fcount = str_xparse(string, fields);
	int pad = FF->head.indexpars.minfields-fcount;
	
	if (pad > 0)	{	// if need to pad
		char * tptr;
		int index;

		if (fcount < 3)	// if only main heading or main+page
			index = 0;		// insert after main (before page)
		else if (FF->head.indexpars.required)	// at least 3 fields and last subhead required
			index = fcount-3;	// insert before required field
		else
			index = fcount-2;	// insert before page field
		tptr = fields[index].str+fields[index].ln+1;	// base of first field to move
		memmove(tptr+pad, tptr, str_xlen(tptr)+1);		/* shift strings */
		memset(tptr,0,pad);		/* clear memory */
	}	
}
/******************************************************************************/
short rec_compress(INDEX * FF, RECORD * curptr, char jchar)	  /* compresses excess fields to maxfields, by combining from lowest */

{
	short freechars, ftot, newmark;
	CSTR br[FIELDLIM];

	freechars = FF->head.indexpars.recsize - str_xlen(curptr->rtext)-1;	/* set up to fix if we have too many fields */
	newmark = FALSE;
	for (ftot = str_xparse(curptr->rtext, br); ftot > FF->head.indexpars.maxfields; ftot--)	{	/* while we have too many fields */
		*(br[ftot-2].str-1) = jchar;		/* replace char before indexed text field (backwards from last) */
		if (freechars)	{					/* if can replace space */
			str_xshift(br[ftot-2].str,1);	/* make room for space */
			*br[ftot-2].str = SPACE;
			freechars--;
		}
		else if (!curptr->ismark)	{	/* if not already marked */
			curptr->ismark = TRUE;
			newmark = TRUE;
		}
	}
	return (newmark);
}
/******************************************************************************/
char * rec_uniquelevel(INDEX * FF, RECORD * recptr, short *hlevel, short * sprlevel, short *hidelevel, short * clevel)   /* finds level at which heading is unique */

{
#define FH_SUPPRESS 1

	CSTR scur[FIELDLIM], sprev[FIELDLIM];
	short curtexttot = str_xparse(recptr->rtext,scur)-1;
	short suppressed = 0;
	short prevtexttot = 0;
	int rindex;
	
	*hlevel = 0;		/* set heading level to 0 */
	*sprlevel = *hidelevel = *clevel = PAGEINDEX;
	for (rindex = 1; rindex < curtexttot; rindex++)	{	// for all text fields
#ifdef PUBLISH
		char *cp = scur[rindex].str-2;		// set to last char of preceding field
		if (*cp++ == FORCEDRUN	// if last char of preceding field (new auto runon format)
			|| *++cp == ';'	// or first char of this field (old auto runon format)
			|| *cp == '<' && *++cp == ';'	// or first after protection
			)		// if want special run-on
#else
		if (*(scur[rindex].str-2) == FORCEDRUN && *sprlevel == PAGEINDEX)	// if last char of preceding field is colon

#endif
			*sprlevel = rindex;
		if (*hidelevel == PAGEINDEX &&		// if not already set suppression level, and field has text
			((suppressed |= FF->head.formpars.ef.field[rindex].flags&FH_SUPPRESS)	// trick to enable suppression by first level at which suppression wanted, even if not this field
			|| rindex == curtexttot-1 && FF->head.indexpars.required && (FF->head.formpars.ef.field[FF->head.indexpars.maxfields-2].flags&FH_SUPPRESS)) && scur[rindex].ln)
			*hidelevel = rindex;		// suppress this record
		if (*clevel == PAGEINDEX &&	FF->head.formpars.ef.collapselevel &&	// if not already set collapse level
			(rindex == FF->head.formpars.ef.collapselevel
			|| FF->head.indexpars.required && rindex == curtexttot-1 && FF->head.formpars.ef.collapselevel <= FF->head.indexpars.maxfields-2))
			*clevel = rindex;		// collapse from this level
	}
	if (recptr = sort_skip(FF,recptr, -1))		{
		prevtexttot = str_xparse(recptr->rtext,sprev)-1;		/* prev */
		while (*hlevel < curtexttot && *hlevel < prevtexttot && !strcmp(scur[*hlevel].str, sprev[*hlevel].str))  /* while text fields match */
			(*hlevel)++;
	}
	if (*hlevel == curtexttot)		{	/* if all current text fields exist in prev */
		if (curtexttot == prevtexttot)	/* if this has as many fields as prev */
			*hlevel = PAGEINDEX; 		/* set level to page */
		else if (!str_crosscheck(FF,scur[curtexttot].str))	/* prev has more fields than current, && not cross-ref in page field */
			 (*hlevel)--;				/* set unique to last common field */
	}
	return (scur[*hlevel < PAGEINDEX ? *hlevel : curtexttot].str);	/* text of unique field */
}
/******************************************************************************/
void rec_getprevnext(INDEX * FF, RECORD * recptr, RECN * prevptr, RECN * nextptr, RECORD * (*skip)(INDEX *, RECORD *, short))   /* finds next & prev records */

{
	RECORD * tptr;
	
	*prevptr = *nextptr = 0;
	if (recptr)		{		/* if a real record */
		if (tptr = skip(FF,recptr,-1))		/* if have previous record */
			*prevptr = tptr->num;
		if (tptr = skip(FF,recptr,1))		/* if have following record */
			*nextptr = tptr->num;
	}
}
/******************************************************************************/
int rec_checkfields(INDEX * FF, RECORD * recptr)	// checks syntax of fields

{
	enum {		/* field errors */
		KEEPCS = 1,		/* bad code after ~ */
		ESCS,		/* bad code after \ */
		BRACES,		/* mismatched {} */
		BRACKETS,	/* mismatched <> */
		PAREN,		/* mismatched () */
		SQBR,		/* mismatched [] */
		QUOTE,		// mismatched fancy quotes
		DQUOTE,		// mismatched double quote
		CCODES		/* some incomplete code */
	};
	
	CSTR field[FIELDLIM];
	int fcount = str_xparse(recptr->rtext, field);
	int findex;
	
	for (findex = 0; findex < fcount; findex++)	{
		char * source = field[findex].str;
		short bcount, brcount, parencnt, sqbrcnt, qcnt, dqcnt, parenbad, sqbrbad, qbad;
		unichar uc;
		
		bcount = brcount = parencnt = sqbrcnt = qcnt = dqcnt = parenbad = sqbrbad = qbad = 0;
		
		while (*source)     {       	/* for all chars in string */
			uc = u8_nextU((char **)&source);
			switch (uc)      {      /* check chars */
				case CODECHR:
				case FONTCHR:
					if (!*source++)		/* if end of line */
						return (CCODES);	/* error return (should never happen) */
					continue;
				case KEEPCHR:       	/* next is char literal */
					if (!*source++)    /* if no following char */
						return (KEEPCS);       /* error return */
					continue;   	/* round for next */
				case ESCCHR:       	/* next is escape seq */
					if (!*source++)    /* if at end of line */
						return (ESCS);     /* return error */
					continue;
				case OBRACE:        /* opening brace */
					if (bcount++)
						goto end;
					continue;
				case CBRACE:      /* closing brace */
					if (--bcount)
						goto end;
					continue;
				case OBRACKET:       /* opening < */
					if (brcount++)
						goto end;
					continue;
				case CBRACKET:      /* closing > */
					if (--brcount)
						goto end;
					continue;
			}
		}
	end:
		if (bcount)
			return (BRACES);
		if (brcount)
			return (BRACKETS);
	}
	return (0);
}
