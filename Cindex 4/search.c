#include "stdafx.h"
#include "search.h"
#include "strings.h"
#include "cglobal.h"
#include "records.h"
#include "group.h"
#include "sort.h"
#include "collate.h"
#include "errors.h"
#include "refs.h"
#include "typestuff.h"
#include "regex.h"
#include "files.h"
#include "index.h"
#include "util.h"

/*****************************************************************************/
RECORD * search_findbylead(INDEX * FF, char *string)

/* finds first record containing leads (or page ref) in string */
/* leads parsed as fields of xstring */

{
	RECORD *curptr;

	if (FF->viewtype == VIEW_NEW)	{	/* if new records; do linear search */
		if (curptr = sort_top(FF))
			curptr = search_linsearch(FF,curptr,string);
	}
	else if (FF->curfile)				/* if group do binary search */
		curptr = grp_lookup(FF,(GROUPHANDLE)FF->curfile,string, TRUE);
	else
		curptr = search_treelookup(FF,string);	/* find in full tree */
	return (curptr);
}
#if 0
/*****************************************************************************/
RECORD * search_treelookup(INDEX * FF, char *string)	/* finds record in full tree */

{
	register short m;
	RECORD *curptr;
	RECN matchnum;
	short pageflag;
	char * pptr, *rptr;

	pageflag = FF->head.sortpars.fieldorder[0] == PAGEINDEX;	/* TRUE if page sort */
	for (matchnum = 0, curptr = rec_getrec(FF, FF->head.root); curptr;) {       /* while more records to check */
		if (pageflag)	{	/* if in page order */
			pptr = str_xlast(curptr->rtext);
			if (str_crosscheck(FF,pptr))	/* if a cross ref in page field */
				m =  -1;		/* cross ref comes after */
			else {
				rptr = ref_sortfirst(FF,pptr);		/* get ref ptr */
				if (!(m = ref_match(FF, string, rptr, FF->head.sortpars.partorder, PMSENSE)))
					matchnum = curptr->num;
			}
		}
		else if (!(m = sort_textmatch(FF,&FF->head.sortpars, string, curptr->rtext, MATCH_TRUNCATED|MATCH_IGNORECODES|MATCH_IGNORECASE)))		/* if new string == old */
			matchnum = curptr->num;	/* save its number */
		curptr = rec_getrec(FF, m <= 0 ? curptr->lchild : curptr->rchild);	   /* move up tree until empty slot */
	}
	if ((curptr = rec_getrec(FF,matchnum)) && !pageflag)	/* if found a match to first field, set up for checks */
		curptr = search_linsearch(FF,curptr,string);
	return (curptr);
}
/*****************************************************************************/
RECORD * search_findbynumber(INDEX * FF, RECN num)	/* finds record by number */

{
	RECORD * curptr;

	if (num <= FF->head.rtot)	{	/* if record is in index */
		if (FF->curfile)	{	/* if using subset of index, do linear search */
			for (curptr = sort_top(FF); curptr; curptr = sort_skip(FF, curptr,1))	{
				if (curptr->num == num)		/* if right match */
					break;
			}
			return (curptr);
		}
		else if (FF->viewtype != VIEW_NEW || num > FF->startnum)	/* full index or is new */
			return (rec_getrec(FF, num));
	}
	return (NULL);
}
#else
/*****************************************************************************/
RECORD * search_treelookup(INDEX * FF, char *string)	/* finds record in full tree */

{
	register short m;
	RECORD *curptr;
	RECN matchnum;
	short pageflag;
	char * pptr, *rptr;

	pageflag = FF->head.sortpars.fieldorder[0] == PAGEINDEX;	/* TRUE if page sort */
	for (matchnum = 0, curptr = rec_getrec(FF, FF->head.root); curptr;) {       /* while more records to check */
		if (pageflag)	{	/* if in page order */
			pptr = str_xlast(curptr->rtext);
			if (str_crosscheck(FF,pptr))	/* if a cross ref in page field */
				m =  -1;		/* cross ref comes after */
			else {
				rptr = ref_sortfirst(FF,pptr);		/* get ref ptr */
				m = ref_match(FF, string, rptr, FF->head.sortpars.partorder, PMSENSE);
			}
		}
		else
			m = col_match(FF,&FF->head.sortpars, string, curptr->rtext, MATCH_LOOKUP|MATCH_IGNOREACCENTS|MATCH_IGNORECODES);
		if (!m)	// if a hit and not ignored
			matchnum = curptr->num;	/* save its number */
		curptr = rec_getrec(FF, m <= 0 ? curptr->lchild : curptr->rchild);	   /* move up tree until empty slot */
	}
	if ((curptr = rec_getrec(FF,matchnum)) && !pageflag)	/* if found a match to first field, set up for checks */
		curptr = search_linsearch(FF,curptr,string);
	return (curptr);
}
/*****************************************************************************/
RECORD * search_findbynumber(INDEX * FF, RECN num)	/* finds record by number */

{
	RECORD * curptr;

	if (num <= FF->head.rtot)	{	/* if record is in index */
		if (FF->curfile)	{	/* if using subset of index, do linear search */
			char delstat = FF->head.privpars.hidedelete;
			
			FF->head.privpars.hidedelete = FALSE;
			for (curptr = sort_top(FF); curptr; curptr = sort_skip(FF, curptr,1))	{
				if (curptr->num == num)		/* if right match */
					break;
			}
			FF->head.privpars.hidedelete = delstat;
			return (curptr);
		}
		else if (FF->viewtype != VIEW_NEW || num > FF->startnum)	/* full index or is new */
			return (rec_getrec(FF, num));
	}
	return (NULL);
}
#endif
/*****************************************************************************/
RECORD * search_lastmatch(INDEX * FF, RECORD * recptr, char * searchspec, short matchtype)	/* finds last rec that matches spec */

	/* returns sensible result only when sort is on */
{
	RECN matchnum;
	CSTR tleads[FIELDLIM];
	CSTR sleads[FIELDLIM];       
	short ttot, matchcount, leadtot;
	short errtype;
	
	leadtot = str_xparse(searchspec,sleads);	/* parse search string */
	do {
		matchnum = recptr->num; 	/* start assuming first record is a match */
		if (recptr = sort_skip(FF, recptr,1))	{
			ttot = str_xparse(recptr->rtext,tleads)-1;		/* parse strings; count points to page field */
			if (FF->head.sortpars.fieldorder[0] == PAGEINDEX)	{	/* if page sort */
			    if (!ref_isinrange(FF, tleads[ttot].str,searchspec,searchspec,&errtype))		/* if ref not in record */
			    	recptr = NULL;			/* kill search */ 
			}
			else	{	/* linear search in text sort */
				for (matchcount = 0; matchcount < leadtot && leadtot <= ttot; matchcount++)	{	/* for all fields that might match */
					if (col_match(FF,&FF->head.sortpars,sleads[matchcount].str,tleads[matchcount].str,matchtype))	/* some field doesn't match */
						break;
				}
				if (matchcount < leadtot)
					recptr = NULL;
			}
		}
	} while (recptr);	/* while records have matching leadstrings */
	return (rec_getrec(FF,matchnum));
}
/*****************************************************************************/
RECORD * search_linsearch(INDEX * FF, RECORD * curptr, char * searchspec)	/* does linear search for leads */

{
	CSTR tleads[FIELDLIM];
	CSTR sleads[FIELDLIM];       
	short ttot, matchcount, hashit, leadtot;
	
	leadtot = str_xparse(searchspec,sleads);	/* find how many leads */
	hashit = FALSE;		/* presume no hit initially */
	do {				/* do linear search for other fields && deleted checks */			
		ttot = str_xparse(curptr->rtext,tleads) - 1;		/* parse strings; knock off page field */
		for (matchcount = 0; matchcount < leadtot && matchcount < ttot; matchcount++)	{	/* for all fields that we expect already to match */
			if (col_match(FF,&FF->head.sortpars,sleads[matchcount].str,tleads[matchcount].str,MATCH_LOOKUP|MATCH_IGNOREACCENTS|MATCH_IGNORECODES))	/* some field doesn't match */
				break;		/* some mismatch in early strings */
		}
		if (matchcount)		/* if a hit */
			hashit = TRUE;
	} while ((sort_isignored(FF,curptr) || !hashit || matchcount && matchcount != leadtot) && (curptr = sort_skip(FF, curptr, 1)));
	if (matchcount != leadtot)
		curptr = NULL;
	return (curptr);	
}
/*****************************************************************************/
RECN search_count(INDEX * FF, COUNTPARAMS * csptr, int filter)	/* counts records */

{
	RECN total;
	short curlength, curdepth, findex, err, leadcount;
	RECORD * curptr, * lastptr;
	CSTR slist[FIELDLIM];
	char oldsort;
	
	oldsort = FF->head.sortpars.ison;
	FF->head.sortpars.ison = csptr->smode;	/* set sort mode */
	sort_setfilter(FF,filter);
	curptr = rec_getrec(FF,csptr->firstrec);	/* get start record */
	for (leadcount = -1, total = 0, lastptr = NULL; curptr && curptr->num != csptr->lastrec; lastptr = curptr, curptr = sort_skip(FF,curptr,1))    {  /* for all records */
		curdepth = str_xparse(curptr->rtext,slist);
		if ((!csptr->markflag || curptr->ismark) && (!csptr->genflag || curptr->isgen) && (!csptr->tagflag || curptr->label)
			&& (!csptr->modflag || curptr->time > FF->opentime)
			&& (csptr->delflag == CO_ALL || curptr->isdel && csptr->delflag == CO_ONLYDEL || !curptr->isdel && csptr->delflag == CO_NODEL)
			&& (!*csptr->firstref || *csptr->firstref && *csptr->lastref && ref_isinrange(FF,slist[curdepth-1].str,csptr->firstref,csptr->lastref,&err)))	{
			if (curptr->time > FF->opentime)
				csptr->modified++;
			if (curptr->ismark)
				csptr->marked++;
			if (curptr->isdel)
				csptr->deleted++;
			if (curptr->isgen)
				csptr->generated++;
			if (curptr->label)
				csptr->labeled[curptr->label]++;
			if (curdepth > csptr->deepest)	{		/* if deeper than current deepest */
				csptr->deeprec = curptr->num;
				csptr->deepest = curdepth;
			}
			for (findex = 0; findex < curdepth; findex++)	{	/* check field lengths */
				if (findex == curdepth-1)	{		/* if locator */
					if (slist[findex].ln > csptr->fieldlen[PAGEINDEX])	/* if longest */
						csptr->fieldlen[PAGEINDEX] = slist[findex].ln;
					curlength = (slist[findex].str+slist[findex].ln)-curptr->rtext+2;	/* check longest record */
					csptr->totaltext += curlength;		/* add to total of entry text */
					if (curlength > csptr->longest)	{
						csptr->longrec = curptr->num;
						csptr->longest = curlength;
						csptr->longestdepth = curdepth;
					}
				}
				else if (slist[findex].ln > csptr->fieldlen[findex])	/* if longest text field */
					csptr->fieldlen[findex] = slist[findex].ln;
			}
			if (leadcount < COUNTTABSIZE-1)	{
				unichar leadchar;
				BOOL new = col_newlead(FF,lastptr ? lastptr->rtext: NULL,curptr->rtext, &leadchar); // get new lead, if any
				if (new)	{	// if new
					if (leadchar == SYMBIT || leadchar == NUMBIT || leadchar == (SYMBIT|NUMBIT))
						leadchar = REPLACECHAR;
					leadcount++;		// move to next lead
					csptr->leads[leadcount].lead = leadchar;
				}
				if (leadcount >= 0)
					csptr->leads[leadcount].total += 1;	// add 1
			}
			total++;
		}
	}
	FF->head.sortpars.ison = oldsort;	/* restore sort mode */
	sort_setfilter(FF,SF_VIEWDEFAULT);
	return (total);
}
/*****************************************************************************/
char * search_findbycontent(INDEX * FF, RECORD * recptr, char * startptr, LISTGROUP * lg, short *mlength)	/* finds first record that matches */

{
	CSTR slist[FIELDLIM];
	short fieldtot, count, fmin, fmax, fcount, sflags, tlen;
	char * sptr, *base, *lasthit, *eptr;
	LIST * lptr;
	short errtype;
	short hasattributes;

	if (lg->delflag | lg->markflag | lg->newflag | lg->modflag | lg->genflag | lg->tagflag) {	// if using any attribute
		if (lg->excludeflag) {	// if record must have no tagged attributes
			hasattributes = lg->delflag && recptr->isdel ||
				lg->markflag && recptr->ismark ||
				lg->newflag && recptr->num > FF->startnum ||
#if 1
				lg->modflag && recptr->time > FF->opentime ||
#else
				lg->modflag && recptr->time > FF->opentime && recptr->num <= FF->startnum || 	// modified and not new
#endif
				lg->genflag && recptr->isgen ||
				lg->tagflag && recptr->label && (!lg->tagvalue || recptr->label == lg->tagvalue);
			if (hasattributes)
				return NULL;
		}
		else {	// if must have all tagged attributes
			hasattributes = (!lg->delflag || recptr->isdel) &&
				(!lg->markflag || recptr->ismark) &&
				(!lg->newflag || recptr->num > FF->startnum) &&
#if 1
				(!lg->modflag || recptr->time > FF->opentime) &&
#else
				(!lg->modflag || recptr->time > FF->opentime && recptr->num <= FF->startnum) && 	// modified and not new
#endif
				(!lg->genflag || recptr->isgen) && (!lg->tagflag || recptr->label && (!lg->tagvalue || recptr->label == lg->tagvalue));
			if (!hasattributes)
				return NULL;
		}
	}
	if (recptr->time < lg->firstdate || recptr->time > lg->lastdate ||
		*lg->userid && strncmp(lg->userid, recptr->user, 4))
		return (NULL);

	*mlength = count = tlen = 0;
	sptr = lasthit = recptr->rtext;
	if (lg->size) {
		fieldtot = str_xparse(recptr->rtext, slist);
		do {	/* for each text string to test */
			lptr = &lg->lsarray[count];
			switch (lptr->field) {	/* find which fields to search */
			case ALLFIELDS:
				fmin = 0;
				fmax = fieldtot;
				break;
			case ALLBUTPAGE:
				fmin = 0;
				fmax = fieldtot - 1;
				break;
			case LASTFIELD:
				fmin = fieldtot - 2;
				fmax = fieldtot - 1;
				break;
			case PAGEINDEX:
				fmin = fieldtot - 1;
				fmax = fieldtot;
				break;
			default:				/* a specified field */
				fmin = lptr->field;
				if (FF->head.indexpars.required) {
					if (fmin == FF->head.indexpars.maxfields - 2)	// if want special field
						fmin = fieldtot - 2;
					else if (fmin >= fieldtot - 2)	// if other field doesn't exist in record
						fmin = fieldtot;		// ensure failed search
				}
				if (fmin < fieldtot - 1)	/* if field is in record */
					fmax = fmin + 1;		/* set bounds */
				else
					fmax = fmin;		/* force failure in search */
				break;
			}
			sflags = lptr->caseflag ? CSINGLE | CCASE : CSINGLE;	/* case sensitive ? */
			if (lptr->wordflag)
				sflags |= CWORD;	/* word search */
			for (sptr = NULL, fcount = fmin; fcount < fmax && !sptr; fcount++) {	/* for all relevant fields */
				static char xbase[MAXREC + 1];
				char *rptr, *sbase = xbase+1;		// sbase guaranteed preceded by zero byte (needed for str_crosscheck look back)

				base = slist[fcount].str;
				if (base < startptr) {	/* if string before start point */
					if (base + slist[fcount].ln > startptr)	/* if entry point in this field */
						base = startptr;		/* set it */
					else			/* start not in this field */
						continue;
				}
				do {		// within field, loop over all possible candidates for text *and* style/font match
					rptr = NULL;
					if (lptr->style || lptr->font || lptr->forbiddenstyle || lptr->forbiddenfont) {	// if looking for style and/or font
						if (base = str_spanforcodes(base, lptr->style, lptr->font, lptr->forbiddenstyle, lptr->forbiddenfont, &tlen)) {	// get span of specified attributes
							if (!*lptr->string) {	// if looking for attributes only
								sptr = base;		// set match pt to start of attributes
								break;
							}
							strncpy(sbase, base, tlen);	// make searchable substring from attribute range
							*(sbase + tlen) = '\0';
							rptr = base + tlen;		// hold base for continuing search beyond current span
						}
						else		// failed
							break;
					}
					else	// no attributes
						strcpy(sbase, base);		// searchable string is whole field
					if (lptr->patflag) {	/* if want pattern */
						char stripped[MAXREC];
						str_textcpy(stripped, sbase);	// work with stripped text copy (no codes)
						if (*lptr->string == '^' && base > str_skipcodes(slist[fcount].str)		// if missed match constrained to start
							|| *(lptr->string + strlen(lptr->string) - 1) == '$' && rptr && rptr < str_rskipcodes(slist[fcount].str))	// or couldn't make one constrained to end
							sptr = NULL;	// force failure
						else
							sptr = regex_find(lptr->regex, stripped, 0, &tlen);
						// to manage zero-length matches [update manual to make clear what behavior is]
						if (sptr) {		// if found in stripped text
							lptr->ref2ptr = base;	// save ptr to original searchable string (for recovering capture groups)
							if (tlen) {		// if not a zero-length match
								*(sptr + tlen) = '\0';	// make a search string from match
								sptr = str_xfind(base + (sptr - stripped), sptr, sflags | CNOCODE, tlen, &tlen);	// find place of it in original text
							}
							else {	// zero-length match (e.g., .* or ^$)
								if (!strcmp(lptr->string, "^$"))		// if explicitly searching for empty string
									sptr = base + (sptr - stripped);	// mark start
								else		// void it
									sptr = NULL;
							}
						}
					}
					else if (lptr->evalrefflag && fmin == fieldtot - 1) {	// evaluating refs
						if (!*lptr->auxptr && !*(sptr = sbase) || (sptr = ref_isinrange(FF, sbase, lptr->auxptr, lptr->ref2ptr, &errtype))) {	/* check range */
							if (eptr = ref_next(sptr, FF->head.refpars.psep))
								tlen = eptr - sptr;
							else
								tlen = strlen(sptr);	/* set length of match */
							sptr = base + (sptr - sbase);	// translate start pointer to right base string
						}
					}
					else {	// simple text search
						if (tlen = strlen(lptr->string)) {	// if have search string
							sptr = str_xfind(sbase, lptr->string, sflags | CNOCODE, tlen, &tlen);
							if (sptr)
								sptr = base + (sptr - sbase);	// translate start pointer to right base string
						}
						else if (lptr->field > 0)	// if an explicit search for empty field */
							sptr = !*base ? base : NULL;	/* set appropriately */
						else		// must be search for record attributes (labeled, etc)
							sptr = base;
					}
				} while (!sptr && (base = rptr));	// while no target within an attribute span and there's more of field to examine
				if (sptr) {		// capture any attributes for potential replace
					lptr->entrycodes = str_codesatposition(slist[fcount].str, sptr - slist[fcount].str, NULL, lptr->style, lptr->font);	// get entry codes
					lptr->exitcodes = str_codesatposition(slist[fcount].str, sptr + tlen - slist[fcount].str, NULL, 0, 0);	// get exit codes
				}
			}
			if (lptr->notflag) 		/* if want "not" */
				sptr = (char *)(!sptr);
			else if (sptr >= lasthit) {	/* mark deepest hit */
				lasthit = sptr;
				*mlength = tlen;		/* use its length as match length */
			}
		} while ((lptr->andflag && sptr || !lptr->andflag && !sptr) && ++count < lg->size && *lg->lsarray[count].string);	/* while inconclusive */
	}
	return sptr ? lasthit : NULL;
}
/*****************************************************************************/
short search_setupfind(INDEX * FF, LISTGROUP * lg, short *field)	/* completes fields in search struct */

{
	short count;
	
	search_clearauxbuff(lg);		/* release any buffers */
	if (lg->lflags == COMR_ALL)		/* if isn't restricted */
		lg->firstr = rec_number(sort_top(FF));
	for (count = 0; count < lg->size; count++)	{	/* for all field specs */
		u8_normalize(lg->lsarray[count].string,strlen(lg->lsarray[count].string)+1);	// ensure we search for normalized unicode
		if (lg->lsarray[count].patflag)	{		/* if need regex */
				lg->lsarray[count].regex = regex_build(lg->lsarray[count].string,lg->lsarray[count].caseflag ? 0: UREGEX_CASE_INSENSITIVE);
				if (!lg->lsarray[count].regex)	{
					senderr(ERR_BADEXPERR, WARN,lg->lsarray[count].string);
					*field = count;
					return FALSE;
				}
		}
		else if (lg->lsarray[count].evalrefflag && lg->lsarray[count].field == PAGEINDEX)	{ 	/* if page reference for evaluation */
			if (lg->lsarray[count].auxptr = calloc(1,strlen(lg->lsarray[count].string)*2+2))	{	/* if can get memory */
				strcpy(lg->lsarray[count].auxptr,lg->lsarray[count].string);		/* copy string */
				if (lg->lsarray[count].ref2ptr = strchr(lg->lsarray[count].auxptr, FF->head.refpars.rsep)) {	/* if ref range */
					*lg->lsarray[count].ref2ptr++ = '\0';	/* terminate first component and make base for second */
					ref_expandfromsource(FF,lg->lsarray[count].ref2ptr,lg->lsarray[count].auxptr);	// build second ref to right number of segments
//					NSLog(@"%s, %s",lg->lsarray[count].auxptr, lg->lsarray[count].ref2ptr);
					if (*lg->lsarray[count].ref2ptr && ref_match(FF,lg->lsarray[count].auxptr,lg->lsarray[count].ref2ptr,FF->head.sortpars.partorder,FALSE) >= 0)	{
						senderr(ERR_REFORDERERR, WARN);
						*field = count;
						return (FALSE);
					}
				}
			}
			else
				return (FALSE);
		}
	}
	return (TRUE);
}
/*****************************************************************************/
void search_clearauxbuff(LISTGROUP * lg)	/* clears auxiliary buffer as necess */

{
	short count;
	
	for (count = 0; count < lg->size; count++)	{
		if (lg->lsarray[count].auxptr)	{	/* if have regex or auxiliary string */
			free(lg->lsarray[count].auxptr);
			lg->lsarray[count].auxptr = NULL;
		}
		if (lg->lsarray[count].regex)	{	// if have regex
			uregex_close(lg->lsarray[count].regex);
			lg->lsarray[count].regex = NULL;
		}
	}
}
/*****************************************************************************/
RECORD * search_findfirst(INDEX * FF, LISTGROUP * lg, short restart, char **sptr, short *mlptr)	/* finds first rec after rptr that contains string */

{
	static short direction;
	static RECN stoprec;
	RECORD * recptr;
	
	if (restart || !FF->lastfound)	{	/* if need a fresh start */
		if (lg->revflag)	{		/* if reverse search */
			direction = -1;
			recptr = lg->lastr == ULONG_MAX ? sort_bottom(FF) : rec_getrec(FF,lg->lastr);
			stoprec = lg->firstr;
		}
		else { 		/* forward search */
			direction = 1;
			recptr = rec_getrec(FF,lg->firstr);
			stoprec = lg->lastr;
		}	
	}
	else if (recptr = rec_getrec(FF, FF->lastfound)) 	/* pick up last search */
		recptr = sort_skip(FF,recptr, direction);	/* move one record away */
	while (recptr && recptr->num != stoprec && !iscancelled(g_mdlg))	{
		if (*sptr = search_findbycontent(FF, recptr, NULL, lg, mlptr))	{	/* if a hit */
			FF->lastfound = recptr->num;
			return (recptr);
		}
		recptr = sort_skip(FF, recptr, direction);
	}
	FF->lastfound = 0;
	return (NULL);				
}
/******************************************************************************/
char * search_reptext(INDEX * FF, RECORD * recptr, char * startptr, short matchlen, REPLACEGROUP * rgp, LIST * ll)	 /* replaces text in record */

{
	REPLACEATTRIBUTES tra = rgp->ra;	// work with copy of replace attributes, because we'll tinker with subscript/superscript
	char * tptr;
	char entryoncodes, entryoffcodes, runningcodes, cancelcodes, resumecodes;
	short count, replen, avail, fontcode;
	int seqlen;

	// need following because could be adding super or sub to text that has the other currently applied
	if (tra.onstyle&FX_SUPER)	// if super on
		tra.offstyle |= FX_SUB;	// force sub off
	if (tra.onstyle&FX_SUB)		// if sub on
		tra.offstyle |= FX_SUPER;	// force super off

	entryoncodes = tra.onstyle&~ll->entrycodes.code;	//  net style that needs to be turned on at start of replacement
	entryoffcodes = tra.offstyle&ll->entrycodes.code;	//  entering style that needs to be turned off
	runningcodes = (tra.onstyle | ll->entrycodes.code)&~tra.offstyle;	// full style set over replacement
	cancelcodes = runningcodes & ~ll->exitcodes.code;		// net styles to be turned off at end of replacement
	resumecodes = ll->exitcodes.code&~runningcodes;		// net styles to be added on post-replacement string

	fontcode = tra.fontchange ? type_findlocal(FF->head.fm, tra.font, 0) | FX_FONT : ll->entrycodes.font;	// recover id of replacement font if specified

	seqlen = entryoncodes ? 2 : 0;
	seqlen += entryoffcodes ? 2 : 0;
	seqlen += ll->entrycodes.font != fontcode ? 2 : 0;

	seqlen += cancelcodes ? 2 : 0;
	seqlen += resumecodes ? 2 : 0;
	seqlen += ll->exitcodes.font != fontcode ? 2 : 0;

	avail = rgp->maxlen - str_xlen(recptr->rtext) + matchlen - seqlen;	/* net space available for new text */
	for (tptr = rgp->repstring, replen = count = 0; count < rgp->reptot; count++) {	 /* replacement string */
		int capturelen = 0;
		char *capturep;

		if (rgp->rep[count].start)		/* if plain text */
			replen += rgp->rep[count].len;
		else if (ll->regex) {		// if regex
			char stripped[MAXREC];
			str_textcpy(stripped, ll->ref2ptr);	// work with stripped text copy (no codes)
			capturep = regex_textforgroup(rgp->regex, rgp->rep[count].index, stripped, &capturelen);
			replen += capturelen;
		}
		else {	// no text or regex (must be only code replacement?)
			capturep = startptr;
			capturelen = matchlen;
			replen += capturelen;
		}
		if (replen > avail || capturelen < 0) {	  // if would overflow, or bad regex replacement
			recptr->ismark = TRUE;
			rgp->failcount++;
			return (NULL);			/* too bad */
		}
		if (rgp->rep[count].start) {	/* if simple replacement */
			strncpy(tptr, rgp->rep[count].start, rgp->rep[count].len);
			tptr += rgp->rep[count].len;
			*tptr = '\0';       /* terminate replacement */
		}
		else {	 		// must want special action
			if (capturelen)		 // if substring to replace
				strncpy(tptr, capturep, capturelen);	  /* add special */
			*(tptr + capturelen) = '\0';	 /* terminate in case need case change */
			if (rgp->rep[count].flag > 0)	/* make case changes */
				str_upr(tptr);
			else if (rgp->rep[count].flag < 0)
				str_lwr(tptr);
			tptr += capturelen;
		}
	}
	str_xshift(startptr + matchlen, replen + seqlen - matchlen);		/* move original string to leave exact space for replacement */
	if (ll->entrycodes.font != fontcode) {	/* if changing font */
		*startptr++ = FONTCHR;
		*startptr++ = fontcode;
	}
	if (entryoncodes) {	/* if turning on style */
		*startptr++ = CODECHR;
		*startptr++ = entryoncodes;
	}
	if (entryoffcodes) {	/* if turning off style */
		*startptr++ = CODECHR;
		*startptr++ = entryoffcodes | FX_OFF;
	}
	strncpy(startptr, rgp->repstring, replen);     /* insert replacement string */
	startptr += replen;
	if (cancelcodes) {	/* if turning off style we turned on */
		*startptr++ = CODECHR;
		*startptr++ = cancelcodes | FX_OFF;
	}
	if (resumecodes) {	/* if turning on style we turned off */
		*startptr++ = CODECHR;
		*startptr++ = resumecodes;
	}
	if (ll->exitcodes.font != fontcode) {		/* if changing font */
		*startptr++ = FONTCHR;
		*startptr++ = ll->exitcodes.font;
	}
	rec_stamp(FF, recptr);
	return (startptr);	/* char beyond end of replacement */
}
/******************************************************************************/
char * search_testverify(INDEX * FF, char * rtext) // returns target on failure

{
	static char base[MAXREC];
	BOOL error = FALSE;	// assume targets found
	short tokens;
	char * str1;
	
	str1 = str_skiplist(str_xlast(rtext), FF->head.refpars.crosstart,&tokens);	/* point to text of ref */
	if (tokens)	{	// if have crossref
		char delstat = FF->head.privpars.hidedelete;
		char * eptr;
		
		FF->head.privpars.hidedelete = TRUE;
		do {
			int length;
			
			while (*str1 == SPACE || *str1 == FF->head.refpars.csep)	/* while padding spaces, etc and not at end of str */
				str1++;
			eptr = ref_next(str1,FF->head.refpars.csep);
			length = str_textcpylimit(base,str1,eptr);	// copy without codes
			*(base+length+1) = EOCS;
			if (str_skiplist(base, FF->head.refpars.crossexclude, &tokens) == base)	 {	// if cross ref doesn't start with general term
				RECORD * recptr = search_treelookup(FF,base);
				char * mptr;
				
				if (!recptr && (mptr = strrchr(base, ',')))  { /* if no hit, but might have as subhead */
					*mptr++ = '\0';		/* terminate first string for search */
					while (*mptr == SPACE)	/* while spaces would be lead to next field */
						str_xshift(mptr+1,-1);	/* shift over */
					recptr = search_treelookup(FF,base);
				}
				if (!recptr)	{	// failed to find target
					error = TRUE;
					break;
				}
			}
		} while (str1 = eptr);	// while more to examine
		FF->head.privpars.hidedelete = delstat;
		return error ? base : NULL;	// return text we failed to find
	}
	return NULL;
}
/******************************************************************************/
short search_verify(INDEX * FF, char * rtext, VERIFYGROUP * vp) /* checks valdity of cross refs */

{
	char *str1, delstat;
	char *tptr, *mptr, *residueptr, *eptr;
	short step, crosslevel, rftot, ftot;
	long dupcount, matchcount, crosscount, casecount;
	short refcount, tokens;
	RECORD *recptr;
	CSTR flist[FIELDLIM], rlist[FIELDLIM];

	delstat = FF->head.privpars.hidedelete;		// bypass sort_setfilter()
	FF->head.privpars.hidedelete = TRUE;
	for (residueptr = FF->head.refpars.crosstart; u_isgraph(u8_nextU(&residueptr));) /* find end of first word */
		;
	step = residueptr - FF->head.refpars.crosstart-1;	  /* length of beginning seq */
	refcount = 0;
	if (str1 = str_xfindcross(FF, rtext, FALSE)) {		// if have ref */
		vp->eflags = FALSE;
		str_xcpy(vp->t1,rtext);	/* copy record text */
		tptr = vp->t1+(str1-rtext);	/* set to pt of cross-ref */
		while (*tptr && (!*(tptr-1) || *(tptr-1) == SPACE || iscodechar(*(tptr-2)) && *(--tptr)))	/* clean search text */
			tptr--;				/* skip leading junk */
		vp->eoffset = tptr-vp->t1;		/* length of main text of rec making ref (for summary view) */
		*tptr++ = '\0';
		*tptr = EOCS;
		crosslevel = str_xcount(vp->t1);
		ftot = str_xparse(rtext,flist);		/* parse record */
		if (crosslevel == ftot-1 || !vp->locatoronly)	{	// if in page field or don't care
			for (dupcount = matchcount = 0, recptr = search_findbylead(FF, vp->t1); recptr; recptr = sort_skip(FF, recptr,1))	{	// while records match source spec
				if (memcmp(recptr->rtext,rtext,flist[crosslevel].str-flist[0].str))	{	// if this record doesn't match up to crossref
					if (matchcount)	// if have already found a matching record
						break;		// must be finished seeking matches
				}
				else	{	// we have a match
					char * lstr = str_xlast(recptr->rtext);
					if (*lstr && !str_crosscheck(FF,lstr))	// if not a crossref
						dupcount++;		// count a locator that's not a crossref
					matchcount++;		// count a match
				}
			}
			/* now have count of # records that match current up to point of cross-ref */
			tptr = str1+step;	/* first char after introductory word */
			while (*tptr == SPACE || iscodechar(*tptr) && *++tptr)		/* skip trailing junk */
				tptr++;
			str1 = str_skiplist(str1, FF->head.refpars.crosstart,&tokens);	/* point to text of ref */
			vp->tokens = tokens;		// save tokens
			if (str1 > tptr && !dupcount && strncmp(tptr,"under",5) || dupcount && str1 <= tptr)	/* if "also", etc, and no refs, or refs and no "also" */
				vp->eflags = V_TYPEERR;
			memset(&vp->cr,0, sizeof(vp->cr));	/* clear particulars for refs */
			for (; *str1 && refcount < VREFLIMIT; refcount++)  {	/* while cross refs in list & not beyond limit */
				while (*str1 == SPACE || *str1 == FF->head.refpars.csep)	/* while padding spaces, etc and not at end of str */
					str1++;
				vp->cr[refcount].offset = str1-rtext;	/* offset of text from base of record */
				eptr = ref_next(str1,FF->head.refpars.csep);
				for (tptr = vp->t1; *str1 && (!eptr || str1 < eptr);)  /* for chars up to delimiter */
					*tptr++ = *str1++;		/* copy to target string */
				*tptr++ = '\0';				/* terminate string */
				*tptr = EOCS;
				vp->cr[refcount].length = tptr-vp->t1-1;		/* length of ref text */
				if (str_skiplist(vp->t1, FF->head.refpars.crossexclude, &tokens) > vp->t1)	 /* if cross ref starts with general term */
					continue;		/* don't check */
				if (!col_match(FF, &FF->head.sortpars, vp->t1, rtext, MATCH_IGNOREACCENTS|MATCH_IGNORECODES))	{
					// if record containing crossref verifies against itself
					vp->cr[refcount].error = V_CIRCULAR;
					continue;
				}
				recptr = search_findbylead(FF,vp->t1);
				if (!recptr && !vp->fullflag && (mptr = strrchr(vp->t1, ',')))  { /* if no hit, but might have as subhead */
					*mptr++ = '\0';		/* terminate first string for search */
					while (*mptr == SPACE)	/* while spaces would be lead to next field */
						str_xshift(mptr+1,-1);	/* shift over */
					recptr = search_findbylead(FF,vp->t1);
					vp->cr[refcount].matchlevel = 1;	/* match (if any) must be at subhead of target */
				}
				for (crosscount = matchcount = casecount = 0; recptr; recptr = sort_skip(FF, recptr,1))		{		/* if have a candidate */
#if 0
					if (!col_match(FF, &FF->head.sortpars, vp->t1, recptr->rtext, vp->fullflag ? MATCH_IGNOREACCENTS|MATCH_IGNORECODES : MATCH_LOOKUP|MATCH_IGNOREACCENTS|MATCH_IGNORECODES))	{	/* if a match */
						rftot = str_xparse(recptr->rtext, rlist);
						if (vp->cr[refcount].matchlevel && col_match(FF, &FF->head.sortpars, mptr, rlist[1].str, MATCH_LOOKUP|MATCH_IGNOREACCENTS|MATCH_IGNORECODES))	/* if need sub and bad match */
							break;
						if (*rlist[rftot-1].str)	{		/* if not empty page field */
							if (str_crosscheck(FF, rlist[rftot-1].str))
								crosscount++;
							else {
								if (++matchcount == 1)		/* if first match */
									vp->cr[refcount].num = recptr->num;		/* save # of target */
								if (matchcount == vp->lowlim)
									break;
							}
						}
					}
#else
					if (!col_match(FF, &FF->head.sortpars, vp->t1, recptr->rtext, vp->fullflag ? MATCH_IGNOREACCENTS|MATCH_IGNORECODES : MATCH_LOOKUP|MATCH_IGNOREACCENTS|MATCH_IGNORECODES))	{	/* if a match */
						if (!vp->fullflag || !col_match(FF, &FF->head.sortpars, vp->t1, recptr->rtext, MATCH_IGNORECODES))	{	// if strict, then test again for case/accent
							rftot = str_xparse(recptr->rtext, rlist);
							if (vp->cr[refcount].matchlevel && col_match(FF, &FF->head.sortpars, mptr, rlist[1].str, MATCH_LOOKUP|MATCH_IGNOREACCENTS|MATCH_IGNORECODES))	/* if need sub and bad match */
								break;
							if (*rlist[rftot-1].str)	{		/* if not empty page field */
								if (str_crosscheck(FF, rlist[rftot-1].str))
									crosscount++;
								else {
									if (++matchcount == 1)		/* if first match */
										vp->cr[refcount].num = recptr->num;		/* save # of target */
									if (matchcount == vp->lowlim)
										break;
								}
							}
						}
						else {	// must be case or accent mismatch
							casecount = 1;
							break;
						}
					}
#endif
					else
						break;
				}
				if (matchcount)	{		/* if have a hit */
					if (matchcount < vp->lowlim)
						vp->cr[refcount].error = V_TOOFEW;
				}
				else if (crosscount)
					vp->cr[refcount].error = V_CIRCULAR;
				else if (casecount)
					vp->cr[refcount].error = V_CASE;
				else
					vp->cr[refcount].error = V_MISSING;
			}
		}
	}
	FF->head.privpars.hidedelete = delstat;
	return (refcount);
}
/*******************************************************************************/
RECN search_convertcross(INDEX * FF, int threshold)		// converts cross-refs to full postings

{
	RECN starttot = FF->head.rtot;
	VERIFYGROUP tvg;
	char trec[MAXREC];
	RECORD * recptr;
	int rcount, crosscount;
	int totalnew;

	memset(&tvg,0, sizeof(VERIFYGROUP));
	tvg.t1 = trec;	// ptr for temp string
	tvg.lowlim = threshold;
	tvg.locatoronly = TRUE;	// don't look at subhead cross-refs
	index_cleartags(FF);	// clear tags
	for (rcount = 0, recptr = sort_top(FF); recptr && !iscancelled(NULL); recptr = sort_skip(FF,recptr,1)) {	   /* for all records */
		showprogress(PRG_CONVERTING,FF->head.rtot,rcount++);
		if (!recptr->isdel && (crosscount = search_verify(FF,recptr->rtext,&tvg)))	{	/* if have cross-ref */
			int gencount, trycount, crcount;
			RECN basetot = FF->head.rtot;	// note in case we need to delete generated records

			gencount = trycount = 0;
			for (crcount = 0; crcount < crosscount; crcount++)	{	// for all cross refs
				if (!(tvg.eflags&V_TYPEERR) && tvg.tokens == 1 && tvg.cr[crcount].error == V_TOOFEW)	{	// if single ref & just too few
					RECORD * curptr, *lastptr;
					char sptr[MAXREC+1], *xptr;
					
					curptr = rec_getrec(FF,tvg.cr[crcount].num);
					str_xcpy(sptr, curptr->rtext);		/* get text */
					xptr = str_xatindex(sptr,tvg.cr[crcount].matchlevel);	// set up for matching fields
					xptr += strlen(xptr++);
					*xptr = EOCS;
					lastptr = search_lastmatch(FF,curptr,sptr,FALSE);	/* find last that matches at that level */
					do {	// until run out of records
						char nrtext[MAXREC], * lptr, *tptr;
						RECORD * trptr;

						str_xcpy(nrtext,recptr->rtext);	// copy base entry
						// if would want to convert subhead cross-refs, then following needs to be changed to find right field level
						lptr = str_xlast(nrtext);	// set to overwrite locator field
						tptr = str_xatindex(curptr->rtext,tvg.cr[crcount].matchlevel+1);	// tail text to be appended
						trycount++;	// count a try	
						if (lptr-nrtext + str_xlen(tptr) < FF->head.indexpars.recsize)	{	// if room for text
							str_xcpy(lptr,tptr);	// add rest of matched entry
							if (trptr = rec_writenew(FF,nrtext))	{	// make it
								trptr->isgen = TRUE;	// mark as generated
								gencount++;
								continue;	// all OK
							}
						}
						break;		// some error (too long, or couldn't make record)
					} while (curptr != lastptr && (curptr = sort_skip(FF,curptr,1)));
				}
			}
			if (gencount != trycount)	{	// some error
				recptr->ismark = TRUE;	// mark original record
				while (basetot < FF->head.rtot) {	// for records spawned from it
					RECORD * trptr;
					if (trptr = rec_getrec(FF, ++basetot))
						trptr->isdel = TRUE;	// delete
				}
			}
			else if (gencount)	// if generated records
				recptr->isdel = TRUE;	// mark original as deleted
		}
	}
	totalnew = FF->head.rtot - starttot;
	while (starttot < FF->head.rtot)	// for all new records added
		sort_makenode(FF,++starttot);
	showprogress(0,0,0);	  /* kill message */
	return totalnew;
}
/******************************************************************************/
RECN search_autogen(INDEX * FF, INDEX *XF, AUTOGENERATE * agp) /* generates cross refs for appropriate targets */

{
	char *residueptr;
	RECORD *recptr, *srptr;
	RECN rcount, trcount, crcount, srcount;
	CSTR slist[FIELDLIM], tlist[FIELDLIM];
	char tcopy[MAXREC], * cptr;
	short len1, len2, prefix, scount;
	unsigned short tlen;
	unsigned char delstats, delstatd;

	delstats = XF->head.privpars.hidedelete;
	XF->head.privpars.hidedelete = TRUE;
	delstatd = FF->head.privpars.hidedelete;
	FF->head.privpars.hidedelete = TRUE;
//	sort_setfilter(XF,SF_HIDEDELETEONLY);
//	sort_setfilter(FF,SF_HIDEDELETEONLY);
	for (len1 = len2 = 0, residueptr = FF->head.refpars.crosstart; *residueptr; residueptr = u8_forward1(residueptr)) { /* find end of first word */
		if (!u_isgraph(u8_toU(residueptr)))	{	/* if end of a word */
			if (!len1)
				len1 = residueptr - FF->head.refpars.crosstart;
			else	{
				len2 = residueptr - FF->head.refpars.crosstart;
				break;
			}
		}
	}
	for (srcount = rcount = 0, srptr = sort_top(XF); srptr; srptr = sort_skip(XF,srptr,1))	{	/* for all records in source file */
		showprogress(PRG_GENERATING,XF->head.rtot,srcount++);
		scount = str_xparse(srptr->rtext,slist);	/* parse record */
		str_xcpy(tcopy,srptr->rtext);	/* temp copy of source text */
		str_xparse(tcopy, tlist);
		/* now lookup up page content of source as heading in dest */
		if (!(recptr = search_findbylead(FF,tlist[scount-1].str)) && strrchr(tlist[scount-1].str, ','))  { /* if no hit */
			if (cptr = strrchr(tlist[scount-1].str, ','))	{	/* if possible subhead to seek */
				*cptr++ = '\0';			/* terminate first string for search */
				while (*cptr == SPACE)	/* while spaces would be lead to next field */
					str_xshift(cptr+1,-1);	/* shift over */
				recptr = search_findbylead(FF,tlist[scount-1].str);
			}
		}
		if (recptr)	{	/* if target exists in dest index */
			*tlist[scount-1].str = EOCS;	/* terminate xstring before page field */
			prefix = len1+1;		/* provisional prefix is 'see' */
			if (recptr = search_findbylead(FF,tcopy))	{	/* if any dest record like source */
				for (trcount = crcount = 0; recptr && (recptr = search_linsearch(FF,recptr,tcopy));trcount++, recptr = sort_skip(FF, recptr,1))	{	/* while we have matching records */
					if (cptr = str_xfindcross(FF,recptr->rtext,FALSE))	{	/* if has cross-ref */
						crcount++;
						if (str_xfind(cptr,slist[scount-1].str,FALSE,slist[scount-1].ln,&tlen))	/* if have our ref in already */
							break;	/* already have the right cross-ref */
					}
				}
				if (recptr)	/* have the ref already */
					continue;
				if (trcount > crcount)	{	/* if total recs under head more than those with cross-refs */
					if (agp->seeonly)		/* if don't want see also refs */
						continue;
					prefix = len2+1;	/* make a 'see also' */
				}
			}
			if (prefix+str_xlen(srptr->rtext) < FF->head.indexpars.recsize)	{	/* if a new record would fit */
				strncpy(tlist[scount-1].str, FF->head.refpars.crosstart, prefix);	/* build ref lead */
				strcpy(tlist[scount-1].str+prefix,slist[scount-1].str);		/* add ref */
				*(tlist[scount-1].str+strlen(tlist[scount-1].str)+1) = EOCS;	/* terminate string */
				if (recptr = rec_writenew(FF,tcopy))	{	/* if can make record */
					recptr->isgen = TRUE;
					sort_makenode(FF,recptr->num);
					rcount++;
				}
				else
					break;
			}
			else	{
				if (agp->maxneed < prefix+str_xlen(srptr->rtext))
					agp->maxneed = prefix+str_xlen(srptr->rtext);
				agp->skipcount++;
			}
		}
	}	
	showprogress(0,0,0);	  /* kill message */
	FF->head.privpars.hidedelete = delstatd;
	XF->head.privpars.hidedelete = delstats;
//	sort_setfilter(FF,SF_VIEWDEFAULT);
//	sort_setfilter(XF,SF_VIEWDEFAULT);
	return (rcount);
}