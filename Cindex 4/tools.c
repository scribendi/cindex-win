#include "stdafx.h"
#include "malloc.h"
#include <string.h>
#include "strings.h"
#include "records.h"
#include "index.h"
#include "sort.h"
#include "regex.h"
#include "formstuff.h"
#include "typestuff.h"
#include "tools.h"
#include "search.h"
#include "util.h"

static BOOL goodbreak(char * base, char *cpos);	// checks if potentially good break point
static int checkfield(unsigned char * source);		/* checks record field */
static CHECKERROR * errorforrecord(RECN number, INDEX * FF, CHECKERROR ** earray, int type);
/******************************************************************************/
RECN tool_join(INDEX * FF, JOINPARAMS *js)	 /* joins fields of records that have redundant subheadings */

{
	register char *markp, *markq, *nextfield, *fbase;
	char *lasttext, modflag;
	short hdlevel, diff1, diff2, codelen;
	CSTR br[FIELDLIM], cr[FIELDLIM];		/* pointers to component strings */
	short curtot, bptot, freefields;
	RECN rcount, rlimit, matchcount;
	char style, font;
	RECN markcount = 0;
	char dumtext[] = { 0,EOCS };
	short tfindex[FIELDLIM + 1];
	RECORD *lastptr, *curptr, *baseptr;
	BOOL needresort = FALSE;
	unsigned char delstat;

	delstat = FF->head.privpars.hidedelete;
	FF->head.privpars.hidedelete = TRUE;
	if (js->orphanaction != OR_PRESERVE && !js->nosplit) {	/* if not forbidding splits */
		index_cleartags(FF);	// clear tags
		/* use temp sort field list in case need to sort on temporary extra fields later joined */
		memcpy(tfindex, FF->head.sortpars.fieldorder, sizeof(tfindex));	/* save sort field array */
		sort_buildfieldorder(FF->head.sortpars.fieldorder, FF->head.indexpars.maxfields, FIELDLIM);	/* builds temp full field order */
		for (rcount = 1; (curptr = rec_getrec(FF, rcount)); rcount++) {       /* for all recs */
			curtot = str_xparse(curptr->rtext, cr);
			for (freefields = FF->head.indexpars.maxfields - curtot, modflag = FALSE, fbase = markp = curptr->rtext, hdlevel = 0; hdlevel < curtot - 1; fbase = markp = nextfield, hdlevel++) {     /* while not at page field */
				nextfield = markp + strlen(markp) + 1;
				if (*(nextfield) == EOCS)		/* if this is the page field */
					break;					/* get out */
				if (hdlevel >= js->firstfield) {	/* if in field range */
					while ((markq = strchr(markp, js->jchar)) && freefields) {     /* while separators in field */
						int ok = goodbreak(fbase, markq);
						if ((ok && (!js->protectnames || !u_isupper(u8_toU((str_skipcodes(markq + 2)))))) || !*(markq + 1)) {	/* if good parsing (& not protected name) or null */
							if (*(markq - 1) && *(markq + 1)) {     /*  if not first or last char in field */
								if (codelen = type_findcodestate(markp, markq, &style, &font)) {	/* if style/font is active */
									if (str_xlen(curptr->rtext) + (codelen << 2) < FF->head.indexpars.recsize) {
										str_xshift(markq, codelen << 2);		/* make gap */
										if (style) {
											*markq++ = CODECHR;
											*markq++ = style | FX_OFF;
										}
										if (font) {
											*markq++ = FONTCHR;
											*markq++ = FX_FONT;	/* default font */
										}
										*markq++ = '\0';       /*  make a new field  */
										if (font) {
											*markq++ = FONTCHR;
											*markq++ = font | FX_FONT;	/* restore font */
										}
										if (style) {			/* restore style */
											*markq++ = CODECHR;
											*markq++ = style;
										}
										modflag = TRUE;
									}
									else {				/* can't make adjustment */
										curptr->ismark = TRUE;
										markcount++;
										break;			/* escape from this record */
									}
								}
								else	/* simple break for new field */
									*markq++ = '\0';
							}
							for (markp = markq; *markq == SPACE || !*markq && !*(markp - 1) && *(markq - 1) || *markq == js->jchar; markq++)       /* while trailing junk */
								;     						/* count chars to discard */
							str_xshift(markq, -(markq - markp));      /*  shift over discard */
							modflag = TRUE;
							freefields--;
							fbase = markp; // base for testing shifts to start of new field
						}
						else		// unacceptable break
							markp = markq + 1;    /* else shift beyond to check again */
					}
				}
			}
			if (modflag) {        /* if modified */
				rec_stamp(FF, curptr);
				curptr->isttag = TRUE;		// mark as split
				sort_makenode(FF, sort_remnode(FF, rcount));   /* replace node */
			}
		}
		memcpy(FF->head.sortpars.fieldorder, tfindex, sizeof(tfindex));	/* restore sort field array */
	}

	for (lastptr = NULL, lasttext = dumtext, baseptr = sort_top(FF); baseptr; baseptr = sort_skip(FF, baseptr, 1)) {	   /* for all records */
		bptot = str_xparse(baseptr->rtext, br);
		for (hdlevel = 0, rlimit = FF->head.rtot; hdlevel < bptot - 2 && bptot > FF->head.indexpars.minfields;) {   /* for all fields in base record with field beyond n, and some beyond min */
			matchcount = 0;				/* counts # of records with matching fields at level n */
			curptr = baseptr;
			do {
				curtot = str_xparse(curptr->rtext, cr);
				if (hdlevel < curtot - 1)				/* if can compare level n field */
					diff1 = strcmp(br[hdlevel].str, cr[hdlevel].str);     /* compare level n */
				else		/* force break */
					diff1 = TRUE;
				if (hdlevel < curtot - 2) {			/* if can compare n+1 field */
					diff2 = strcmp(br[hdlevel + 1].str, cr[hdlevel + 1].str);	/* and n+1 */
					if (str_crosscheck(FF, cr[hdlevel + 1].str))		/* if a cross-ref */
						diff2 = TRUE;		/* force break */
				}
				else
					diff2 = TRUE;
			} while (!diff1 && !diff2 && ++matchcount < rlimit && (curptr = sort_skip(FF, curptr, 1)));  /* while match at n and at n+1 and not at last rec */

			if ((diff1 || matchcount == rlimit || !curptr) && strcmp(lasttext, br[hdlevel].str) && hdlevel >= js->firstfield) {	/* if fields n differ or at end of index */
				curptr = baseptr;
				do {
					int lasttextlevel = str_xparse(curptr->rtext, cr) - 2;
					if (js->orphanaction == OR_PRESERVE) {		// if just flagging orphans
						if (js->orphans)
							js->orphans[js->orphancount++] = curptr->num;
						else
						if (js->errors) {
							CHECKERROR * ep = errorforrecord(curptr->num, FF, js->errors, 0);
							ep->fields[hdlevel + 1] |= CE_ORPHANEDSUBHEADING;
						}
						bptot = hdlevel;	// prevent action at any lower level
						break;
					}
					else if (js->orphanaction == OR_DELETE && hdlevel == lasttextlevel - 1 && !curptr->isttag) {	// if deleting orphans and this is genuine
						str_xshift(cr[hdlevel + 2].str, -(cr[hdlevel + 1].ln + 1));	// slide all higher fields downward
						if (baseptr == curptr)		/* if modified base record */
							bptot = str_xparse(baseptr->rtext, br);	/* revise pointer table */
						rec_stamp(FF, curptr);
						needresort = TRUE;
					}
					else {		// absorbing orphans and everything else
						*(cr[hdlevel].str + cr[hdlevel].ln) = js->jchar;       /* convert null to comma */
						if (FF->head.indexpars.recsize > str_xlen(curptr->rtext) + 1) {		/* if there's room */
							str_xshift(cr[hdlevel + 1].str, 1);	    /* make a gap */
							*cr[hdlevel + 1].str = SPACE;	    /* insert a space */
						}
						else if (!curptr->ismark) {	/* if not already marked */
							curptr->ismark = TRUE;
							markcount++;
						}
						str_adjustcodes(curptr->rtext, CC_TRIM);	// might have redundant codes from earlier splitting
						if (baseptr == curptr)		/* if modified base record */
							bptot = str_xparse(baseptr->rtext, br);	/* revise pointer table */
						rec_stamp(FF, curptr);
					}
				} while ((long)(--matchcount) > 0 && (curptr = sort_skip(FF, curptr, 1)));	/* while still in range of matching field n */
			}
			else {
				hdlevel++;				/* advance field counter */
				rlimit = matchcount;	/* set max # recs that can be joined */
				if (*(nextfield = lasttext + strlen(lasttext) + 1) != EOCS)		/* if more text fields remain in last record */
					lasttext = nextfield;	/* advance comparison field */
			}
		}
		if (js->orphanaction == OR_PRESERVE)
			markcount += rec_compress(FF, baseptr, js->jchar);	/* compress record as necessary */
		lastptr = baseptr;
		lasttext = lastptr->rtext;		/* set up for skip */
	}
	FF->head.privpars.hidedelete = delstat;
	if (needresort)
		sort_resort(FF);
	return (markcount);
}
/******************************************************************************/
static BOOL goodbreak(char * base, char *cpos)	// checks if potentially good break point

{
	if (*(cpos+1) == SPACE)	{
		int ocount = 0;
		int qtoggle = 0;

		while (base < cpos)	{	// scan for codes up to potential break
			unichar uc = u8_nextU((char **)&base);
			switch (uc)      {      /* check chars */
//			switch (*base++) {
				case KEEPCHR:
				case ESCCHR:
				case CODECHR:
				case FONTCHR:
					base++;
					continue;
				case OBRACE:
				case OBRACKET:
				case OPAREN:
				case '[':
				case OQUOTE:
					ocount++;
					continue;
				case CBRACE:
				case CBRACKET:
				case CPAREN:
				case ']':
				case CQUOTE:
					ocount--;
					continue;
				case '"':
					qtoggle ^= 1;
					ocount += qtoggle ? 1 : -1;
					continue;
			}
		}
		return ocount <= 0;
	}
	return FALSE;
}
/******************************************************************************/
RECN tool_explode(INDEX * FF, SPLITPARAMS *sp)	 // explodes headings by separating entities

{
	// CODECHR is ctrl-Z; FONTCHAR is ctrl-Y
	// forename(s): (?:[:lu:][:l:]+|[:lu:]\\.)(?:[- ](?:[:lu:][:l:]+|[:lu:]\\.))*	// !! order of | terms is important
	// surname : (?:[:l:]+[’' ])?[:l:][:l:]+(?:[- ][:lu:][:l:]*)?

	// OLD surname, forname(s): (?:[:l:]+[’ ])?[:lu:][:l:]*(?:[- ][:lu:][:l:]*)?, (?:[:lu:][:l:]+|[:lu:]\.)(?:[- ](?:[:lu:][:l:]+|[:lu:]\.))*
	// OLD forname(s) surname: (?:[:lu:][:l:]+|[:lu:]\.)(?:[- ](?:[:lu:][:l:]+|[:lu:]\.))*(?:[:l:]+[’ ])?[:lu:][:l:]*(?:[- ][:lu:][:l:]*)?

	static char * patterns[] = {
		// NB: ICU recommends using possessive operator after * when possible
		"(?:[:l:](?:[-'’][:l:])*(?:~.)*\\s*)+",				// one or more space separated words. // can have ’'- in middle  // allows ~.
		"(?:[:l:]+[’' ])?[:l:][:l:]+(?:[- ][:lu:][:l:]*)?, (?:[:lu:][:l:]+|[:lu:]\\.)(?:[- ](?:[:lu:][:l:]+|[:lu:]\\.))*",
		"(?:[:lu:][:l:]+|[:lu:]\\.)(?:[- ](?:[:lu:][:l:]+|[:lu:]\\.))* (?:[:l:]+[’' ])?[:l:][:l:]+(?:[- ][:lu:][:l:]*)?",		// forenames and/or initials  plus surname
	};

	CSTR cr[FIELDLIM];		/* pointers to component strings */
	RECORD *curptr;
	unsigned char delstat;
	char mainbase[MAXREC], newbase[MAXREC], firstbase[MAXREC];
	RECN starttot = FF->head.rtot;
	RECN totalnew;
	RECN reportlines = 0;

	delstat = FF->head.privpars.hidedelete;
	FF->head.privpars.hidedelete = TRUE;
	index_cleartags(FF);	// clear tags

	URegularExpression * regex = regex_build(sp->patternindex >= 0 ? patterns[sp->patternindex] : sp->userpattern, 0);
	if (regex) {
		for (curptr = sort_top(FF); curptr; curptr = sort_skip(FF, curptr, 1)) {
			int rindex = 0;
			short length;
			char * mptr;

			str_xcpy(mainbase, curptr->rtext);	// save copy of record text
			str_xparse(mainbase, cr);
			firstbase[0] = '\0';

			for (mptr = mainbase; mptr = regex_find(regex, str_skipcodes(mptr), 0, &length); rindex++) {
				CSTATE codestate = str_codesatposition(mainbase, (int)(mptr - mainbase), NULL, 0, 0);	// capture codes up to start of match
#if 0
				for (int gi = 0; gi < regex_groupcount(regex); gi++) {
					int length;
					char * gp = regex_textforgroup(regex, gi, mptr, &length);
					strncpy(newbase, gp, length);
					newbase[length] = '\0';
	//				NSLog(@"++Record %u[%d]: %s",curptr->num,gi,newbase);
				}
#endif
				if (sp->patternindex == SPLIT_NAME_S || sp->patternindex == SPLIT_NAME_S) {	// if splitting names
					short tokens;	// number of tokens parsed
					char * skipptr = str_skiplist(mptr, FF->head.flipwords, &tokens);	// skip any leading conjunction/prep
					length -= skipptr - mptr;
					mptr = skipptr;
				}
				strncpy(newbase, mptr, length);	// copy the matched text (after any skipping)
				newbase[length] = '\0';
				if (!sp->removestyles)
					str_encloseinstyle(newbase, codestate);	// restore any styles to matched text
				*(newbase + strlen(newbase) + 1) = EOCS;	//  compound string needed for adjustment
				str_adjustcodes(newbase, CC_TRIM | CC_ONESPACE);
				if (sp->preflight) {
					strcat(firstbase, "[");
					strcat(firstbase, newbase);
					strcat(firstbase, "]  ");
				}
				else {
					if (rindex) {	// if beyond first match, make new record
						RECN current = curptr->num;	// save in case index resized
						RECORD *nrptr;

						str_xcpy(newbase + strlen(newbase) + 1, cr[1].str);	// append all the other fields
						if (nrptr = rec_writenew(FF, newbase)) {	// make it, with any restored codes
							nrptr->isgen = TRUE;	// mark as generated
							sp->gencount++;
						}
						curptr = rec_getrec(FF, current);	// recover pointer in case memory moved
					}
					else	// first match; save for revising original record
						strcpy(firstbase, newbase);
				}
				mptr += length;
			}
			if (!rindex) {	// if record contained no target
				if (sp->preflight) {
					char tbase[MAXREC];
					int tlength = sprintf(tbase, "\t%u\tNo Match: %s\r", curptr->num, curptr->rtext);
					sp->reportlist[reportlines] = malloc(tlength + 1);
					strcpy(sp->reportlist[reportlines++], tbase);
				}
				else if (sp->markmissing) {		// if want to mark untouched records
					curptr->ismark = TRUE;
					sp->markcount++;
				}
			}
			else {
				if (sp->preflight) {
					char tbase[MAXREC];
					int tlength = sprintf(tbase, "\t%u\t%s\r", curptr->num, firstbase);
					sp->reportlist[reportlines] = malloc(tlength + 1);
					strcpy(sp->reportlist[reportlines++], tbase);
				}
				else if (rindex > 1 || sp->cleanoriginal) {	// if created any new record or want original clean anyway, fix original
					str_xcpy(firstbase + strlen(firstbase) + 1, cr[1].str);	// build adjusted record text
					str_xcpy(curptr->rtext, firstbase);	// replace record contents
					rec_stamp(FF, curptr);
					sp->modcount++;
				}
			}
		}
	}
	uregex_close(regex);
	FF->head.privpars.hidedelete = delstat;
	totalnew = FF->head.rtot - starttot;
	if (!sp->preflight)
		sort_resort(FF);
	return totalnew;
}
/******************************************************************************/
void tool_check(INDEX * FF, CHECKPARAMS *cp)		 // makes comprehensive checks on entries

{
	static char * patterns[] = {
		" [.,?:;\\])’”!]|([.,?:;’”!“‘])\\1|[,:;][\\p{Alphabetic}.)\\]]",	// space before punct | repeated punct | irregular punct
		"[^ 0-9][\\[(](?!s\\)|es\\)|ies\\))", // missing space before paren/bracket, unless digit precedes, or content is (s) or (es) or (ies) [uses neg lookahead]
//		"[:ll:]+[:lu:]|[:lu:][:lu:]+[:ll:]",	// mixed case word (forgiving leading cap)
		"[:ll:]+[:lu:]|[:lu:][:lu:]+[a-rt-z]",	// mixed case word (forgiving leading cap, and trailing 's')
		"[-,.;:'’”!“‘]+",	// punctuation
//		" *\\(.*\\)$",		// parenthetical ending
		" \\(.*\\)$",		// parenthetical ending
	};

	int pcount = sizeof(patterns) / sizeof(char *);
	URegularExpression * regexes[10];
	RECORD *curptr, *lastptr = NULL;
	CSTR cr[FIELDLIM], lr[FIELDLIM];
	unsigned char delstat, sortstat;
	int fcount = 0, lfcount = 0;
	char trec[MAXREC];

	for (int pindex = 0; pindex < pcount; pindex++)
		regexes[pindex] = regex_build(patterns[pindex], 0);
	delstat = FF->head.privpars.hidedelete;
	sortstat = FF->head.sortpars.ison;
	FF->head.privpars.hidedelete = TRUE;
	FF->head.sortpars.ison = TRUE;

	cp->vg.t1 = trec;

	for (curptr = sort_top(FF); curptr; curptr = sort_skip(FF, curptr, 1)) {
		fcount = str_xparse(curptr->rtext, cr);
		char curbase[MAXREC];
		short length;

		int findex = 0, matchindex = -1;
		if (lastptr) {		// if not first record, skip identical fields down to page (flag only errors at unique heading levels)
			for (; findex < fcount - 1 && findex < lfcount - 1 && !strcmp(cr[findex].str, lr[findex].str); findex++)
				matchindex = findex;
		}
		for (; findex < fcount; findex++) {
			int errors = 0;
			errors |= checkfield(cr[findex].str);	// if any of several field errors
			if (findex == fcount - 1) {	// page field
				int crosscount = search_verify(FF, curptr->rtext, &cp->vg);	// check if cross-ref anywhere in record
				if (crosscount) {	// if have a cross-reference
					CHECKERROR * ep = NULL;
					for (int count = 0, crossindex = 0; count < crosscount; count++) {
						if (cp->vg.cr[count].error || cp->vg.eflags&V_TYPEERR) {	// if an error
							cp->vg.cr[count].error |= cp->vg.eflags&V_TYPEERR;	// make sure we encode type error
							if (!ep) {
								ep = errorforrecord(curptr->num, FF, cp->errors, CE_CROSSERR);
								ep->fields[FF->head.indexpars.maxfields - 1] |= CE_CROSSERR;
							}
							ep->crossrefs[crossindex++] = cp->vg.cr[count];
						}
					}
				}
				else if (!*cr[findex].str)
					errors |= CE_EMPTYPAGE;
				if (lastptr && matchindex >= 0 && findex > lfcount-1 && !str_crosscheck(FF, lr[lfcount-1].str)) {	// page ref: if more fields in this record than in previous, and prev not crossref, potential ref position error on prev
					CHECKERROR * ep = errorforrecord(lastptr->num, FF, cp->errors, 0);
					ep->fields[FF->head.indexpars.maxfields - 1] |= CE_HEADINGLEVEL;
				}
			}
			else {	// these checks for headings only
				str_textcpy(curbase, cr[findex].str);	// strip codes
				if (strstr(curbase, "  "))				// if multiple spaces
					errors |= CE_MULTISPACE;
				if (regex_find(regexes[0], curbase, 0, &length))	// if space before punctuation
					errors |= CE_PUNCTSPACE;
				if (regex_find(regexes[1], curbase, 0, &length) > curbase)	// if missing space before ([ & not at start of field
					errors |= CE_MISSINGSPACE;
				if (regex_find(regexes[2], curbase, 0, &length))	// if mixed case word
					errors |= CE_MIXEDCASE;
				if (lastptr && findex < lfcount - 1) {		// if not first record, and there's corresponding heading in prior, check against corresponding heading fields in prior
					if (strcmp(cr[findex].str, lr[findex].str)) {	// if differ in corresponding heading field
						char lastbase[MAXREC];
						char * scptr, *slptr;
						str_textcpy(lastbase, lr[findex].str);	// strip codes
						if (!strcmp(curbase, lastbase))		// if differ only in codes
							errors |= CE_INCONSISTENTSTYLE;
						else {	// don't match; check 1: case diff
							str_lwr(curbase);	// in place conversion (dangerous if U and L have diff utf-8 length)
							str_lwr(lastbase);
							if (!strcmp(curbase, lastbase))	// if now identical
								errors |= CE_INCONSISTENTCAPS;
							else {	// check 2: inconsistent starting conjunctions/prepositions
								short tokens;	// number of tokens parsed
								scptr = str_skiplist(curbase, FF->head.flipwords, &tokens);
								slptr = str_skiplist(lastbase, FF->head.flipwords, &tokens);
								if (!strcmp(scptr, slptr))	// if now identical
									errors |= CE_INCONSISTENTLEADPREP;
								else {	// check 2a: inconsistent ending conjunctions/prepositions
									scptr = str_skiplistrev(curbase, FF->head.flipwords, &tokens);
									slptr = str_skiplistrev(lastbase, FF->head.flipwords, &tokens);
									if (*scptr)		// if have end word
										*scptr = '\0';	// truncate before it
									if (*slptr)		// if have end word
										*slptr = '\0';	// truncate before it
									if (!strcmp(curbase, lastbase))	// if now identical
										errors |= CE_INCONSISTENTENDPREP;
									else {	// check 3: inconsistent punctuation
										regex_replace(regexes[3], curbase, "");	// strip all punct
										regex_replace(regexes[3], lastbase, "");
										if (!strcmp(curbase, lastbase))	// if now identical
											errors |= CE_INCONSISTENTPUNCT;
										else {		// check 4: parenthetical endings
											regex_replace(regexes[4], curbase, "");	// strip parens and contents
											regex_replace(regexes[4], lastbase, "");
											if (!strcmp(curbase, lastbase))	// if now identical (one or both had parenthetical phrase)
												errors |= CE_INCONSISTENTENDPHRASE;
											else {	// check 5: plural endings
												// https://en.oxforddictionaries.com/spelling/plurals-of-nouns
												char * cptr, *lptr;
												for (cptr = curbase, lptr = lastbase; *cptr && *cptr == *lptr; cptr++, lptr++)
													;
												if (!*lptr && (!strcmp(cptr, "s") || !strcmp(cptr, "es")))	// if simple suffix
													errors |= CE_INCONSISTENTENDPLURAL;
												else if (!strcmp(lptr, "ies") && !strcmp(cptr, "y"))	// special y to ies
													errors |= CE_INCONSISTENTENDPLURAL;
												else if (!strcmp(lptr, "f") && !strcmp(cptr, "ves"))	// special f to ves
													errors |= CE_INCONSISTENTENDPLURAL;
											}
										}
									}
								}
							}
						}
					}
				}

			}
			if (errors) {
				CHECKERROR * ep = errorforrecord(curptr->num, FF, cp->errors, 0);
				ep->fields[findex == fcount - 1 ? FF->head.indexpars.maxfields - 1 : findex] |= errors;
			}
		}
		lastptr = curptr;		// transfer info for becoming previous record
		for (int xindex = 0; xindex < FIELDLIM; xindex++)
			lr[xindex] = cr[xindex];
		lfcount = fcount;
	}
	for (int pindex = 0; pindex < pcount; pindex++)
		uregex_close(regexes[pindex]);

	// now find orphans
	if (cp->reportKeys[CE_ORPHANEDSUBHEADINGINDEX])	// do only if wanted, because requires full index pass
		tool_join(FF, &cp->jng);

	// now check page reference counts
	if (cp->reportKeys[CE_TOOMANYPAGEINDEX] || cp->reportKeys[CE_OVERLAPPINGINDEX]) {	// do only if wanted, because requires full index pass
		char vmode = FF->head.privpars.vmode;
		short runlevel = FF->head.formpars.ef.runlevel;
		char sortrefs = FF->head.formpars.ef.lf.sortrefs;
		char suppressduplicates = FF->head.formpars.ef.lf.noduplicates;
		FF->head.privpars.vmode = VM_FULL;	// force into full form, full indented, sort refs and suppress duplicates
		FF->head.formpars.ef.runlevel = 0;
		FF->head.formpars.ef.lf.sortrefs = YES;
		FF->head.formpars.ef.lf.noduplicates = YES;
		for (curptr = sort_top(FF); curptr; curptr = form_skip(FF, curptr, 1)) {	   /* for all records */
			ENTRYINFO es;

			FF->singlerefcount = TRUE;		// page range to counts as 1 ref
			FF->overlappedrefs = 0;
			form_buildentry(FF, curptr, &es);
			if (FF->overlappedrefs || es.prefs > cp->pagereflimit) {
				CHECKERROR * ep = errorforrecord(curptr->num, FF, cp->errors, 0);
				if (es.prefs > cp->pagereflimit) {
					ep->fields[FF->head.indexpars.maxfields - 1] |= CE_TOOMANYPAGE;
					ep->refcount = es.prefs;
				}
				if (FF->overlappedrefs)
					ep->fields[FF->head.indexpars.maxfields - 1] |= CE_OVERLAPPING;
			}
		}
		FF->head.privpars.vmode = vmode;
		FF->head.formpars.ef.runlevel = runlevel;
		FF->head.formpars.ef.lf.noduplicates = suppressduplicates;
		FF->head.formpars.ef.lf.sortrefs = sortrefs;
	}
	FF->head.privpars.hidedelete = delstat;
	FF->head.sortpars.ison = sortstat;
}
/****************************************************************************/
static CHECKERROR * errorforrecord(RECN number, INDEX * FF, CHECKERROR ** earray, int type)

{
	if (!earray[number])	// if have no existing error record
		earray[number] = calloc(1, sizeof(CHECKERROR) + FF->head.indexpars.maxfields * sizeof(int)); // make one
	if (type&CE_CROSSERR) {	// if need crossref info
		if (!earray[number]->crossrefs)	// if don't already have memory
			earray[number]->crossrefs = calloc(VREFLIMIT, sizeof(VERIFY));
	}
	return earray[number];
}
/****************************************************************************/
static int checkfield(unsigned char * source)		/* checks record field */

{
	int result = 0;
	short bcount, brcount, parencnt, sqbrcnt, qcnt, dqcnt, parenbad, sqbrbad, qbad;
	unichar uc;

	bcount = brcount = parencnt = sqbrcnt = qcnt = dqcnt = parenbad = sqbrbad = qbad = 0;

	while (*source) {       	/* for all chars in string */
		uc = u8_nextU((char **)&source);
		switch (uc) {      /* check chars */
		case CODECHR:
		case FONTCHR:
			if (!*source++) {	// skip code; if end of line
				result |= CE_BADCODE;
				goto end;
			}
			continue;
		case KEEPCHR:       /* next is char literal */
		case ESCCHR:       	/* next is escape seq */
			if (!*source) {	/* if no following char */
				result |= CE_MISUSEDESCAPE;
				goto end;
			}
			source = u8_forward1(source);	// skip protected char
			continue;   	/* round for next */
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
		case '(':       /* opening paren */
			parencnt++;
			continue;
		case ')':       /* closing paren */
			if (--parencnt < 0)     /* if closing ever precedes opening */
				parenbad++;
			continue;
		case '[':       /* opening sqbr */
			sqbrcnt++;
			continue;
		case ']':       /* closing sqbr */
			if (--sqbrcnt < 0)      /* if closing ever precedes opening */
				sqbrbad++;
			continue;
		case OQUOTE:       /* opening quote */
			qcnt++;
			continue;
		case CQUOTE:       /* closing quote */
			if (--qcnt < 0)      /* if closing ever precedes opening */
				qbad++;
			continue;
		case '"':       /* dquote */
			dqcnt++;
			continue;
		}
	}
end:
	if (bcount)
		result |= CE_MISUSEDBRACKETS;		// bad braces
	if (brcount)
		result |= CE_MISUSEDBRACKETS;	// bad brackets
	if (parencnt || parenbad)   /* if mismatched parens */
		result |= CE_UNBALANCEDPAREN;
	if (sqbrcnt || sqbrbad)   /* if mismatched sqbr */
		result |= CE_UNBALANCEDPAREN;
	if (qcnt || qbad)   /* if mismatched curly quotes */
		result |= CE_UNBALANCEDQUOTE;
	if (dqcnt & 1)   /* if mismatched simple quotes */
		result |= CE_UNBALANCEDQUOTE;
	return (result);
}