#include "stdafx.h"
#include "strings.h"
#include "records.h"
#include "sort.h"
#include "index.h"
#include "spell.h"
#include "util.h"

#define ENDLEN 3		/* longest parenthetical suffix */

static short stripcopy(INDEX * FF, char *sptr, char * pptr, SPELL *sp);

/*****************************************************************************/
RECORD * sp_findfirst(INDEX * FF, SPELL * sp, short restart, char **sptr, short *mlptr)	/* finds first rec after rptr that contains err */

{
	static RECN stoprec;
	RECORD * recptr;
	
	if (restart || !FF->lastfound)	{	/* if need a fresh start */
		recptr = rec_getrec(FF,sp->firstr);
		stoprec = sp->lastr;
	}
	else if (recptr = rec_getrec(FF, FF->lastfound)) 	/* pick up last search */
		recptr = sort_skip(FF,recptr, 1);	/* move one record away */
	while (recptr && recptr->num != stoprec /* && !main_comiscancel() */)	{
		if ((!sp->newflag || recptr->num >= FF->startnum) && (!sp->modflag || recptr->time >= FF->opentime))	{		/* if meets new/mod criteria */
			if (*sptr = sp_checktext(FF, recptr, NULL, sp, mlptr))	{	/* if a hit */
				FF->lastfound = recptr->num;
				return (recptr);
			}
		}
		recptr = sort_skip(FF, recptr, 1);
	}
	FF->lastfound = 0;
	return NULL;				
}
/******************************************************************************/
char * sp_checktext(INDEX * FF, RECORD * recptr, char * startptr, SPELL * sp, short * mlptr)	/* marks spell error */

{
	char tbuff[MAXREC];
	CSTR field[FIELDLIM];
	short fcount, len, ulevel, sprlevel,hidelevel,clevel;
	char * wptr, *base, *prevword;

	
	str_xcpy(tbuff,recptr->rtext);
	fcount = str_xparse(tbuff, field);
	wptr = tbuff;	/* default start pos */
	if (startptr)		/* continuing in record */
		wptr += startptr-recptr->rtext;
	else	{			/* starting new search */
		if (sp->field != ALLFIELDS)	{			/* if not searching in all fields */
			if (sp->field == ALLBUTPAGE)		/* all text fields */
				*field[fcount-1].str = EOCS;	/* terminate before page field */
			else if (sp->field == LASTFIELD)	{		/* if last text field */
				wptr = field[fcount-2].str;	/* start pos is start of field */	
				*field[fcount-1].str = EOCS;	/* terminate before page field */
			}
			else if (sp->field < fcount-1) 	{	/* if specified field exists */
				wptr = field[sp->field].str;	/* start pos is start of field */	
				*field[sp->field+1].str = EOCS;	/* terminate it */
			}
			else if (sp->field == PAGEINDEX)	/* page field */
				wptr = field[fcount-1].str;
			else
				return NULL;	/* field doesn't exist in record */
		}
	}
	rec_uniquelevel(FF,recptr,&ulevel,&sprlevel,&hidelevel,&clevel);		/* find first field to check */
	if (ulevel == PAGEINDEX)
		ulevel = fcount-1;
	if (wptr < field[ulevel].str)			/* if start point before it */
		wptr = field[ulevel].str;			/* set start point to unique field */
	len = stripcopy(FF,tbuff,field[fcount-1].str,sp);
	for (prevword = NULL; wptr < tbuff+len; wptr++, prevword = base)	{
		while (*wptr == SPACE)
			wptr++;
		base = wptr;
		while (*wptr && *wptr != SPACE)	/* while in word */
			wptr++;
		*wptr = '\0';
		if (wptr > base+1)	{		/* if not a single character */
			if (!hspell_spellword(base))	{ // if bad word
				*mlptr = strlen(base);
//				NSLog (@"Bad: %s",base);
				return (recptr->rtext+(base-tbuff));	/* ptr to bad word */
			}
		}
		if (prevword && !strcmp(base, prevword))	{	/* if double word */
			if (prevword + strlen(recptr->rtext+(prevword-tbuff)) > base)	{	/* if they're in same field */
				*mlptr = wptr-base;	/* length of word */
				sp->doubleflag = TRUE;
				return (recptr->rtext+(base-tbuff));	/* ptr to bad word */
			}
		}
	}
	return NULL;
}
/******************************************************************************/
char * sp_reptext(INDEX * FF, RECORD * recptr, char * startptr, short matchlen, char * corrtext)	 /* replaces text in record */

{
	short correctlen;
	
	if (corrtext)	{	/* if replacement */
		correctlen = strlen(corrtext);
		if (str_xlen(recptr->rtext)+correctlen-matchlen > 0)	{	/* if have space */
			str_xshift(startptr+matchlen,correctlen-matchlen);		/* move original string to leave exact space for replacement */
			strncpy(startptr,corrtext,correctlen);     /* insert replacement */
			rec_stamp(FF,recptr);
			return (startptr + correctlen);	/* char beyond end of replacement */
		}
		return (startptr + matchlen);	/* couldn't replace; send a message ?? */
	}
	else	{	/* removing target */
		while (*(startptr-1) == SPACE)	{
			startptr--;
			matchlen++;
		}
		str_xshift(startptr+matchlen,-matchlen);
		return startptr;
	}
}
/******************************************************************************/
static short stripcopy(INDEX * FF, char *sptr, char * pptr, SPELL *sp)

{
	char *tptr, *eptr, *abase;
	char *base = sptr;
	static char ucend[] = "IES", lcend[] = "ies";
	
	unichar uc, lc;

	for (lc = 0, uc = u8_toU(sptr); *sptr != EOCS; lc = uc, uc = u8_toU(sptr))	{	/* clean it up for checking */
		eptr = sptr;
		switch (uc)  {
			case CODECHR:	
			case FONTCHR:
				if (*++eptr)	/* if any char follows */
					eptr++;		/* ignore it too */
				break;
			case SPACE:
			case '\'':
			case '-':
				sptr = u8_forward1(sptr);
				continue;
			case 0:
				if (sptr == pptr-1) 	{		// if at start of page field
					if (!sp->checkpage && !str_crosscheck(FF, pptr))	// if don't want ref check & not crossref
						goto done;
				}
				break;
			case OPAREN:	/* possible parenthetical ending to ignore */
				if (u_isalpha(lc))	{	/* if paren succeeds alpha */
					for (tptr = sptr+1; *tptr && tptr < sptr+ENDLEN+2; tptr++)  {  /* check for parenthetical endings (up to three letters) */
						if (*tptr == CPAREN)   { // if got closing paren and right length
							if (!strncmp((u_isupper(lc) ? ucend : lcend),sptr+1,ENDLEN))	// strncmp is fine because looking for exact match
								eptr = tptr+1;
							break;
						}
					}
				}
				break;
			case OBRACE:
				while (*++eptr && (*eptr != CBRACE || *(eptr-1) == KEEPCHR || *(eptr-1) == ESCCHR))
					;
				break;
			default:
				if (u_isalpha(uc))	{
					if (!u_isalpha(lc))	// if last char wasn't alpha
						abase = sptr;	// mark start of alphas
					if (u_isupper(uc) && sp->lp.ignallcaps && !u_isalpha(lc))	{	// if first uc letter in word
						unichar tc;
						for (tptr = sptr; (tc = u8_nextU(&tptr)) && (u_isupper(tc) || tc == DASH);)	/* move on until not u.c. */
							;
						if (!u_isalnum(tc))	{	// if not followed by lowercase text or number
							eptr = tptr;
							break;
						}
					}
					sptr = u8_forward1(sptr);
					continue;
				}
				else if (u_isdigit(uc))	{
					if (sp->lp.ignalnums && u_isalpha(lc))	{
						unichar tc;
						for (tptr = sptr; (tc = u8_nextU(&tptr)) && u_isalnum(tc);)	// move beyond end of alnums
							;
						sptr = abase;
						eptr = tptr;
						break;
					}
					sptr = u8_forward1(sptr);
					continue;
				}
		}
		// if we get here we're ignoring current character
		if (eptr == sptr)	// if dealing with single char
			eptr = u8_forward1(sptr);
		memset(sptr,SPACE,eptr-sptr);
		sptr = eptr;
		uc = SPACE;
	}
	if (*sptr == EOCS)	// if reached end of xstring
		*--sptr = '\0';	// restore NULL at end of page field
done:
	while (*(sptr-1) == SPACE)	/* while trailing spaces */
		*--sptr = '\0';
//	NSLog(@"%@", [NSString stringWithUTF8String:base]);
	return (sptr-base); 	/* length of text to check */
}
#if 0
/******************************************************************************/
static short stripcopy(INDEX * FF, unsigned char *sptr, char * pptr, SPELL *sp)

{
	short field = 0;		/* true when in page field */
	char *tptr;
	char *base = sptr, *cbase;
	static char ucend[] = "IES", lcend[] = "ies";
	unsigned short count, crossflag;

#define ENDLEN 3		/* longest parenthetical suffix */

	while (*sptr != EOCS)	{	/* clean it up for checking */
		switch (*sptr)  {
			case CODECHR:		  /* this char is already treated as soft hypen */
#if 0
				if (*++sptr)   {	/* if any char follows */
					*sptr++ = CODECHR;	/* ignore it too */
					if (!isalnum(*sptr))	{	/* if soft hypens would be dangling */
						*(sptr-2) = SPACE;	/* kill them so they're not flagged */
						*(sptr-1) = SPACE;
					}
				}
#else
				*sptr++ = SPACE;
				if (*sptr)	/* if any char follows */
					*sptr++ = SPACE;	/* ignore it too */
				while (*sptr == DASH || *sptr == ENDASH)	/* discard trailing hyphens */
					*sptr++ = SPACE;
#endif
				break;
#if defined SBREAK
			case SORTBREAK:
#endif
//#ifdef PUBLISH
			case FORCEDRUN:
//#endif //PUBLISH
			case ENDASH:
				*sptr++ = DASH;	/* change en-dash to ordinary hypen */
				break;
			case '`':			/* grave accent (sometimes used as opening quote) */
			case OLDFSPACE:		/* change all following to ordinary space */
			case EMDASH:
			case ELIPSIS:
			case ESCCHR:
			case KEEPCHR:
			case CBRACKET:
			case OBRACKET:
				*sptr++ = SPACE;
				break;
			case 0:
				if (sptr == pptr-1) 	{		/* if at NULL before page field */
					if (!(crossflag = str_crosscheck(FF, pptr)) && !sp->checkpage)	/* if not cross-ref & don't want other */
						goto done;
				}
				*sptr++ = SPACE;	/* clear */
				break;
			case OPAREN:	/* possible parenthetical ending to ignore */
#ifdef PUBLISH
				if (*(sptr+2) == CPAREN && isalnum(*(sptr+1)))	/* if single alnum in parens */
					*sptr = *(sptr+1) = *(sptr+2) = SPACE;		/* kill sequence */
				else
#endif //PUBLISH
				if (sptr++ > base && isalpha(*(sptr-2)))	{	/* if paren succeeds alpha */
					for (tptr = sptr; *tptr && tptr < sptr+ENDLEN+1; tptr++)  {  /* check for parenthetical endings (up to three letters) */
						if (*tptr == CPAREN)   { /* if got closing paren preceding */
							count = tptr-sptr;		/* # of chars to compare */
							cbase = ENDLEN-1 + (isupper(*(sptr-2)) ? ucend : lcend);   /* set case of comp string by preceding char */
							while (count && sptr[count-1] == *cbase--)	 /* check back from end of strings */
								count--;
							if (!count) {	/* if whole ending matches */
								sptr--;		/* reset to opening paren */
								while (sptr <= tptr)	 /* set all u.c. chars to SPACE */
									*sptr++ = SPACE;
							}
						}
					}
				}
				break;
			case OBRACE:
				do {			/* clear all text in unescaped braces */
			    	*sptr++ = SPACE;
				} while (*sptr && (*sptr != CBRACE || *(sptr-1) == KEEPCHR || *(sptr-1) == ESCCHR));
				if (*sptr)
					*sptr++ = SPACE;		/* replace closing brace */
				break;
			default:
				if (isupper(*sptr) && sp->lp.ignallcaps && (sptr == base || !isalpha(*(sptr-1))))	{	/* if u.c and want ign caps && no letter precedes */
					for (tptr = sptr; isupper(*tptr) || *tptr == DASH || *tptr == ENDASH; tptr++)	/* move on until not u.c. */
						;
					if (!isalpha(*tptr) && tptr > sptr+1)   {	   /* if not encountered l.c. and not isolated letter */
						while (sptr < tptr)	/* set all u.c. chars to SPACE */
//							*sptr++ = SPACE;
							*sptr++ = DASH;	// 7/31/05 to prevent false double word detection
						break;
					}
				}
				else if (sptr >= pptr)	{	/* if page field */
					if (!crossflag)	{		/* if checking normal ref */
						if (!isalnum(*sptr))    /* if in page field and non alnum */
							*sptr = SPACE;
					}
					else if (*sptr == FF->head.refpars.csep)	/* if cross-ref separator */
						*sptr = SPACE;
				}
				if (sp->lp.ignalnums && isdigit(*sptr))	{	/* if not checking alphanumeric combos, and it's digit */
					for (tptr = sptr-1; tptr >= base && isalnum(*tptr); tptr--)	/* clear alnums backwards */
						*tptr = SPACE;
					while (isalnum(*sptr))	/* clear alnums forwards */
						*sptr++ = SPACE;
				}
				else
					sptr++;
		}
	}
	*--sptr = '\0';		/* restore NULL at end of page field */
done:
	while (*(sptr-1) == SPACE)	/* while trailing spaces */
		*--sptr = '\0';
	return (sptr-base); 	/* length of text to check */
}
#endif