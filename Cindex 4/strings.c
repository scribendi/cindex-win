#include "stdafx.h"
#include "strings.h"
#include "typestuff.h"
#include "util.h"
#include "errors.h"

static char * _rbase;
static unsigned long _rcur;
static unsigned long _rlimit;

static BOOL switchflip(char *target, char *list);		// returns TRU if target preceded by ~ in list
static char *skiplist(char *base, char *list, short * tokens, int xflags);		/* points to first word in base that isn't in list */
static void setleadcaps(char * s1, char * s2);	// switches lead chars as appropriate
static void strinsert(char * s1, char *s2);	// inserts ss2 at s1
static void striptext(char * s1, int count);	// strips text out of string, preserving codes

/*******************************************************************************/
 /* This code is derived from software contributed to Berkeley by
 * Chris Torek. */
char * strnstr(const char *s, const char *find, size_t slen)

{
	char c, sc;
	size_t len;

	if ((c = *find++) != '\0') {
		len = strlen(find);
		do {
			do {
				if (slen-- < 1 || (sc = *s++) == '\0')
					return (NULL);
			} while (sc != c);
			if (len > slen)
				return (NULL);
		} while (strncmp(s, find, len) != 0);
		s--;
	}
	return ((char *)s);
}
/*******************************************************************************/
void str_upr(char * str)		// converts UTF_8 to upper case

{
	while (*str)	{
		if (*str != KEEPCHR && !iscodechar(*str)) {	// if not special char
			unichar uc = u8_toU(str);
			if (u_islower(uc))
				u8_appendU(str, u_toupper(uc));	// assumes uc has same number of bytes as lc
		}
		else if (*(str+1))	// if special char not at end
			str++;
		str = u8_forward1(str);
	}
}
/*******************************************************************************/
void str_lwr(char * str)		// converts UTF-8 to lower case

{
	while (*str)	{
		if (*str != KEEPCHR && !iscodechar(*str)) {	// if not special char
			unichar uc = u8_toU(str);
			if (u_isupper(uc))
				u8_appendU(str, u_tolower(uc));	// assumes lc has same number of bytes as uc
		}
		else if (*(str+1))	// if special char not at end
			str++;
		str = u8_forward1(str);
	}
}
/*******************************************************************************/
void str_title(char * string, char * skiplist)		// converts string to title case

{
	boolean firstcap = TRUE;
	char *tptr;
	unichar uc;
	short tokens;

	str_lwr(string);
	tptr = string;
	do {
		tptr = str_skiptoword(tptr);
		if (firstcap || (tptr = str_skiplist(tptr, skiplist, &tokens))) {	// if first word in string or exempt
			uc = u8_toU(tptr);
			if (u_islower(uc) && (tptr == string || *(tptr - 1) != KEEPCHR))	// if lowercase and not protected
				u8_appendU(tptr, u_toupper(uc));	// assumes uc has same number of bytes as lc
		}
		firstcap = FALSE;
	} while ((tptr = str_skiptowordbreak(tptr)) && *tptr);
}
/*******************************************************************************/
char * str_extend(char * string)		/* converts string to extended string */

{
	*(string+strlen(string)+1) = EOCS;
	return (string);
}
/*******************************************************************************/
void str_flip(char * fields, char * presuf, BOOL smart, BOOL half, BOOL page)	//  in-place flip of fields

{
	char f0[MAXREC], f1[MAXREC], temp[MAXREC], *tptr;
	CSTR flist[FIELDLIM];
	
	str_xparse(fields,flist);
	strcpy(f0,flist[0].str);	// get fields separated
	strcpy(f1,flist[1].str);
	if (smart)	{ // if want smart
		if (flist[1].ln)	{		// if have anything in lower field
			char * skipptr, *insertpos;
			short tokens;
			
			if ((skipptr = str_skiplist(flist[1].str,presuf,&tokens)) > flist[1].str)	{	// if lead prefixes
				str_textcpylimit(temp,flist[1].str,skipptr);	// get lead text
				if (page && tokens > 1)	{		//  page field contains presumed 'see also'
					striptext(f1,skipptr-flist[1].str);	// strip prefix out of lower string, up to count chars
					setleadcaps(f0,f1);		// set caps
					insertpos = str_skipcodes(f0);	// find insertion point after opening codes
					strinsert(insertpos,temp);	// prepend what was prefix from lower string
				}
				else {	// normal heading, or page field 'see'
					if (!page)	{	// if about-to-be lower field isn't page field
						if (switchflip(temp,presuf))	{
							insertpos = str_rskipcodes(f0);	// find insertion point before terminal codes
							if (insertpos > f0)		// if upper field isn't empty
								strinsert(insertpos++," ");	// append space
						}
						else
							insertpos = str_skipcodes(f0);
						strinsert(insertpos,temp);	// append what was prefix from lower string
					}
					striptext(f1,skipptr-flist[1].str);	// strip text out of lower string, up to count chars
					setleadcaps(f0,f1);
				}
			}
			else if ((skipptr = str_skiplistrev(flist[1].str,presuf, &tokens)) < flist[1].str+flist[1].ln && !u_ispunct(u8_toU(skipptr)))	{
				// if trailing suffixes, and not immediately preceded by punctuation
				setleadcaps(f0,f1);
				while (*skipptr == ' ')		// while found trailing text has lead spaces
					skipptr++;		// skip over them
				str_textcpy(temp,skipptr);
				if (switchflip(temp,presuf))	{	// if need to switch to start
					strcat(temp," ");	// append trailing space
					insertpos = str_skipcodes(f0);
				}
				else {		// preserve position at end
					insertpos = str_rskipcodes(f0);	// find insertion point before terminal codes
					if (insertpos > f0)	// if upper field isn't empty
						strinsert(insertpos++," ");	// append space
				}
				strinsert(insertpos,temp);	// prepend what was suffix
				striptext(f1+(skipptr-flist[1].str),-1);	// strip text out of lower string, to end of string
			}
			else	// no preps/articles to move
				setleadcaps(f0,f1);
		}
	}
	tptr = fields;
	strcpy(tptr,f1);	// what was lower becomes upper field
	tptr += strlen(tptr++);	// set to base of second field
	strcpy(tptr, half ? "" : f0);	// second is empty if half flip
	tptr += strlen(tptr++);	// set to EOCS
	*tptr = EOCS;
	str_adjustcodes(fields,CC_ONESPACE|CC_TRIM);
}
/**********************************************************************************/
static BOOL switchflip(char *target, char *list)		// returns TRUE if target preceded by ~ in list

{
	static char ttarget[100] = "~";

	strcpy(ttarget+1,target);
	return strstr(list,ttarget) ? FALSE : TRUE;
}
/*******************************************************************************/
BOOL str_swapparen(char * field, char * presuf,BOOL real)	//  in-place swap of text in parens and outside

{
	if (strlen(field))	{		// if have any text
		char f0[MAXREC+1], ptext[MAXREC], otext[MAXREC],*tptr;
		char * skipptr, *dptr, *pptr = ptext, *optr = otext;
		BOOL opened = 0;
		short tokens;
		
		str_xcpy(f0,field);	// get field
		*pptr = *optr = '\0';	// clear the strings
		dptr = optr;	// assume start with non-paren text
		skipptr = str_skiplist(f0,presuf,&tokens);	// skip any swap prefixes
		for (tptr = skipptr; *tptr; tptr++)	{	// for all chars
			if (*tptr == OPAREN && !opened)	{	// if first opening paren
				dptr = pptr;			// copy to paren buffer
				opened = TRUE;
			}
			else if (*tptr == CPAREN && opened)	// if first closing after opening
				break;	// done; any residues just to be copied
			else	{
				*dptr++ = *tptr;	// copy to correct buffer
				if ((*tptr == ESCCHR || *tptr == KEEPCHR) && *(tptr+1))		// if next is to be protected
					*dptr++ = *++tptr;	// copy next to correct buffer
				*dptr = '\0';
			}
		}
		if (*tptr == CPAREN && *otext)	{	// if eligible for swap
			if (real)	{	// if want to do it
				int len;
				char *sptr;
				
				for (sptr = otext+strlen(otext); *--sptr == SPACE;)	// trim trailing spaces from new paren text
					*sptr = '\0';
				len = sprintf(field,"%.*s%s (%s)%s",(int)(skipptr-f0),f0,ptext,otext,++tptr);
				field[len+1] = EOCS;
				str_adjustcodes(field,CC_ONESPACE|CC_TRIM);
				return TRUE;
			}
		}
	}
	return FALSE;
}
/**********************************************************************************/
static void setleadcaps(char * s1, char * s2)	// switches lead chars as appropriate

{
	unichar uc1;
	unichar uc2;

	s1 = str_skipcodes(s1);
	s2 = str_skipcodes(s2);
#if 0
	if (islower(*s2) && isupper(*s1) && !isupper(*(s1+1)))	{	/* if need case change */
		*s1 =_tolower(*s1);	/* switch case of leads */
		*s2 =_toupper(*s2);	/* switch case of leads */
	}
#else
	uc1 = u8_toU(s1);
	uc2 = u8_toU(s2);
//	if (u_islower(uc2) && u_isupper(uc1) && !u_isupper(u8_toU(u8_back1(s1))))	{	/* if need case change */
	if (u_islower(uc2) && u_isupper(uc1) && !u_isupper(u8_toU(u8_forward1(s1))))	{	/* if need case change */
		u8_appendU(s1, u_tolower(uc1));	/* switch case of leads */
		u8_appendU(s2, u_toupper(uc2));	/* switch case of leads */
	}
#endif
}
/**********************************************************************************/
static void strinsert(char * s1, char *s2)	// inserts ss2 at s1

{
	str_shift(s1,strlen(s2));	// open up space in dest string
	strncpy(s1,s2, strlen(s2));
}
/**********************************************************************************/
static void striptext(char * s1, int count)	// strips text out of string, preserving codes

{
	while (*s1 && count)	{
//$$		if (*s1 == CODECHR && *(s1+1))	{
		if (iscodechar(*s1) && *(s1+1))	{
			s1 += 2;
			count -= 2;
		}
		else {
			str_shift(s1+1,-1);
			count--;
		}
	}
}
/*******************************************************************************/
CSTATE str_codesatposition(char * string, int offset, int * span, char stylemask, char fontmask)	// returns net code value at offset

																									// returns ptr to style/font codes at offset from string; if span != NULL returns span for style(s) that match style mask
{
	CSTATE cs;
	char * cptr;
	cs.code = cs.font = 0;

	for (cptr = string; cptr < string + offset || iscodechar(*cptr); cptr++) {
		if (iscodechar(*cptr) && *(cptr + 1)) {
			char type = *cptr++;
			char tcode = *cptr;
			if (type == CODECHR) {
				if (tcode&FX_OFF)
					cs.code &= ~(tcode&FX_STYLEMASK);
				else
					cs.code |= tcode;
			}
			else	// font
				cs.font = tcode;
		}
	}
	if (span) {	// if want span
		*span = 0;
		if ((!stylemask || (cs.code&stylemask) == stylemask) && (!fontmask || cs.font == (fontmask | FX_FONT))) {	// if not interested in styles/fonts, or we've a match to those we want
			for (; *cptr; cptr++) {	// while still in string
				if (iscodechar(*cptr) && *(cptr + 1)) {
					char type = *cptr++;
					char tcode = *cptr;
					if (type == CODECHR && stylemask) {
						if (cs.code && (tcode&FX_OFF)) {		// if turning off
							tcode &= stylemask; 		// make sure it's style we care about
							if (cs.code&tcode) {	// if we're turning off one we care about
								*span = cptr - (string + offset);
								break;
							}
						}
					}
					else if (fontmask) {		// font
						if (cs.font != tcode) {
							*span = cptr - (string + offset);
							break;
						}
					}
				}
			}
		}
	}
	return (cs);
}
/*******************************************************************************/
char * str_spanforcodes(char * base, char style, char font, char forbiddenstyle, char forbiddenfont, short * span)	// returns position and span of specified style/font

{
	// finds span of wanted codes, vetoed by forbidden codes
	// will fail in cases where there's been a prior match within a field,
	// and we reenter search at point in field where some required or forbidden attribute is already set

	char * tptr = base, *fptr = NULL, *stptr = NULL, *xsptr = NULL;
	char * sptr = NULL;
	BOOL wantfont = font || forbiddenfont, wantstyle = style || forbiddenstyle;
	short curstyle = 0;
	int charcount = 0;

	if (font && font == FX_FONT || forbiddenfont && forbiddenfont != FX_FONT)	// if want default font or not forbidding it; assume it from start
		fptr = base;
	do {
		for (; *tptr; tptr++) {	// while there's a code
			if (*tptr == FONTCHR) {	/* if a font code */
				tptr++;
				if (wantfont) {
					if (font && *tptr == font)	// start run of required font
						fptr = tptr + 1;
					else if (forbiddenfont && *tptr == forbiddenfont) {	// entering forbidden font
						if (charcount && fptr && (stptr || !wantstyle)) {	// if previously in wanted font, and got any wanted style
							tptr++;
							break;
						}
						fptr = NULL;
					}
					else {		// starting neither forbidden nor wanted
						if (font) {
							if (fptr && charcount && (stptr || !wantstyle)) {	// if ending wanted font and got any wanted style
								tptr++;
								break;
							}
							fptr = NULL;	// kill any found font
						}
						else if (!fptr) 	// if not already in an allowed font
							fptr = tptr + 1;	// allow it
					}
				}
			}
			else if (*tptr == CODECHR) {	/* a style code */
				tptr++;
				if (wantstyle) {
					if (*tptr&FX_OFF) {	/* if off style */
						curstyle &= ~*tptr;	/* remove the relevant codes */
						if (stptr && (curstyle&style) != style) {	// if ending wanted style
							if (fptr || !wantfont) {	// if got any wanted font
								tptr++;		// ensure we end on char beyond code
								break;
							}
							stptr = NULL;	// kill any found style
						}
						else if (xsptr && !(curstyle&forbiddenstyle)) {	// if forbidden ends
							xsptr = NULL;
							if ((curstyle&style) == style)	// if we're left with wanted style
								stptr = tptr + 1;			// mark start
						}
					}
					else {	// turning on style
						curstyle |= *tptr;	/* add the relevant codes */
						if (!xsptr && (curstyle&forbiddenstyle)) {	// if starting forbidden style
							xsptr = tptr + 1;
							if (stptr && (fptr || !wantfont)) {		// if were in wanted style and have any wanted font
								tptr++;
								break;
							}
							else		// kill any found style
								stptr = NULL;
						}
						else if (!stptr && (curstyle&style) == style && !xsptr)	// if starting wanted and not vetoed
							stptr = tptr + 1;			/* mark start */
					}
				}
			}
			else {
				charcount++;
				if (!stptr && !xsptr && !style && forbiddenstyle)	// must want runs with no style
					stptr = tptr;
			}
		}
		if ((fptr || !wantfont) && (stptr || !wantstyle)) {	// if got required hits
			sptr = fptr > stptr ? fptr : stptr;	// find the right start
			if (sptr && *sptr) {	// if found a match
				while (iscodechar(*sptr))	// forward over any codes
					sptr += 2;
				while (iscodechar(*(tptr - 2)) && tptr > sptr)	// back up over any codes
					tptr -= 2;
				*span = tptr - sptr;	// length is to char before code/font point, or end of field
				if (*span > 0)		// if ended up with good span
					return sptr;	// return
				fptr = stptr = NULL;	// clear markers for another pass
			}
		}
	} while (sptr && *tptr);	// can get here only after finding zero length span
	return NULL;
}
/*******************************************************************************/
BOOL str_containscodes(char * base, char style, char  font, short span)	// returns position and span of specified style/font

{
	// returns true if style(s) or font found within span
	char * tptr;

	for (tptr = base; (tptr = strpbrk(tptr, codecharset)) && tptr < base + span; tptr++) {	// while there's a code
		if (*tptr++ == FONTCHR) {	/* if a font code */
			if (font && (*tptr&FX_FONTMASK) == font)	// if hit font we care about
				return TRUE;
		}
		else {	/* a style code */
			if (style && (*tptr&style))
				return TRUE;
		}
	}
	return FALSE;
}
/*******************************************************************************/
char * str_encloseinstyle(char * string, CSTATE style)	// encloses string in style
														// assumes there's space for enlarged string
{
	if (style.code || style.font) {
		int shift = style.code && style.font ? 4 : 2;
		memmove(string + shift, string, strlen(string) + 1);
		if (style.code) {
			*string++ = CODECHR;
			*string++ = style.code;
		}
		if (style.font) {
			*string++ = FONTCHR;
			*string++ = style.font;
		}
		string += strlen(string);
		if (style.code) {
			*string++ = CODECHR;
			*string++ = style.code | FX_OFF;
		}
		if (style.font) {
			*string++ = FONTCHR;
			*string++ = FX_FONT;		// ??
		}
		*string = '\0';
	}
	return string;
}
/*******************************************************************************/
char str_capturecodes(char * string, int length)	// returns net code value at length

{
	char code = 0;
	for (char * cptr = string; cptr < string + length; cptr++) {
		if (iscodechar(*cptr) && *(cptr + 1)) {
			char tcode = ++*cptr;
			cptr++;
			if (tcode&FX_OFF)
				code &= tcode & FX_STYLEMASK;
			else
				code |= tcode;
		}
	}
	return (code);
}
/*******************************************************************************/
char * str_skipcodes(char * string)		/* returns ptr to first non- codesequence */

{
//$$	while (*string == CODECHR && *(string+1))
	while (iscodechar(*string) && *(string+1))
		string += 2;
	return (string);
}
/*******************************************************************************/
char * str_rskipcodes(char * string)		/* returns ptr to last non- codesequence */

{
	char * eptr = string+strlen(string);

//$$	while (eptr > string+1 && *(eptr-2)== CODECHR)
	while (eptr > string+1 && iscodechar(*(eptr-2)))
		eptr -= 2;
	return (eptr);
}
/*******************************************************************************/
char * str_skiptoword(char * string)		// returns ptr to first non-punct, non code

{
	unichar uc;

	while ((uc = u8_toU(string)) && (iscodechar(*string) && *++string || !u_isalnum(uc)))
		string = u8_forward1(string);
	return (string);
}
/*******************************************************************************/
char * str_skiptowordbreak(char * string)		// returns ptr to first non-word character

{
	unichar uc;

	while ((uc = u8_toU(string)) && (iscodechar(*string) && *++string || u_isalnum(uc) || uc == 0x27 || uc == 0x2019)) // apostophe/quote
		string = u8_forward1(string);
	return (string);
}
/*******************************************************************************/
char * str_xstr(char * list, char * string)		/* finds string in array */

{
	while (list && *list != EOCS) {
		if (!strcmp(list, string))		/* if match */
			return (list);
		list += strlen(list++);
	}
	return (NULL);
}
/*******************************************************************************/
int str_xcount(char * list)		/* # strings in list */

{
	int count;
	
	for (count = 0; list && *list != EOCS; count++)
		list += strlen(list++);
	return (count);
}
/*******************************************************************************/
int str_xparse(char * list, CSTR * array)		/* puts string pointers in array */


{
	int count;
	
	for (count = 0; *list != EOCS; count++, array++)	{
		array->str = list;
		array->ln = strlen(list++);
		list += array->ln;
	}
	return (count);		/* number of strings */
}
/******************************************************************************/
char *str_xlast(char * text)    /* returns pointer to start of last field */

{
	register char *ptr, *pptr;
	
	for (pptr = ptr = text; *ptr != EOCS; ptr += strlen(ptr++))
		pptr = ptr;		/* holds pointer to last decent field */
	return (pptr);
}
/*******************************************************************************/
int str_xindex(char * list, char * string)	/* finds index of string in array */

{
	int index;
	
	for (index = 0; list && *list != EOCS; index++, list += strlen(list++)) {
		if (!strcmp(list, string))		/* if match */
			return (index);
	}
	return (-1);
}
/*******************************************************************************/
char * str_xatindex(char * list, int index)	/* finds string at indexed position */

{	
	int count;
	
	for (count = 0; list && *list != EOCS; count++, list += strlen(list++)) {
		if (count == index)		/* if match */
			return (list);
	}
	return (NULL);
}
/*******************************************************************************/
TCHAR * str_uxatindex(TCHAR * list, int index)	/* finds string at indexed position */

{	
	int count;
	
	for (count = 0; list && *list != EOCS; count++, list += nstrlen(list++)) {
		if (count == index)		/* if match */
			return (list);
	}
	return (NULL);
}
/******************************************************************************/
char * str_xcpy(char *to, char *from)	/* copies compound string */

{
	return(memcpy(to,from,str_xlen(from)+1));
}
/******************************************************************************/
short str_xcmp(register char *str1, register char *str2)	  /* compares two compound strings */

{
	for ( ;*str1 == *str2; str1++, str2++)	  /* while not at end of string */
		if (*str1 == EOCS)	      /* if strings match at end */
			return (0);
	return (*str1 - *str2);	  /* return difference between chars */
}
/******************************************************************************/
void str_xswap(char *string, short index1, short index2)	  /* swaps two component strings */

{
	register char * str1, *str2;
	char * tstr;
	register char tchar;
	
	if (index1 != index2 && (str1 = str_xatindex(string,index1)) && (str2 = str_xatindex(string, index2)))	{
		if (str1 > str2)	{
			tstr = str2;		/* swap pointers */
			str2 = str1;
			str1 = tstr;
		}
		while (*str1 && *str2)	{	/* while not at end of either string */
			tchar = *str2;		/* swap chars in place */
			*str2++ = *str1;
			*str1++ = tchar;
		} 
		while (*str1)		{	/* still some left in string 1 */
			tchar = *str1;		/* move intervening text char by char */
			memmove(str1, str1+1, str2-str1);
			*(str2-1) = tchar;
		}
		while (*str2)		{	/* still some left in string 2 */
			tchar = *str2;		/* move intervening text char by char */
			memmove(str1+1, str1, str2-str1);
			str2++;
			*str1++ = tchar;
		}
	}
}
/******************************************************************************/
char *str_xfind(char *source, char *target, unsigned short flags, unsigned short maxlen, unsigned short * actuallen)	   /* finds substring in compound string */

{
	UErrorCode error = U_ZERO_ERROR;
	unichar uc1;
	unichar lc, llc;
	int count, scount, word, nocodes, trailcode, ulen;
	char *sp, *sourcebase;
	unichar *tp;
	unichar utarget[MAXREC];
	
	u_strFromUTF8(utarget,MAXREC,&ulen,target,maxlen,&error);
	count = u8_countU(source, flags&CSINGLE ? strlen(source) : str_xlen(source)) - ulen + 1;	   // number of potential comparisons;
	word = flags&(CWORD|CSWORD);
	nocodes = flags&CNOCODE;
    flags &= CCASE;				/* clear all other flags */
	sourcebase = source;
	
	if (!flags)	{
		for (tp = utarget; *tp; tp++)
			if (u_isupper(*tp))
				*tp = u_tolower(*tp);
	}
	lc = uc1 = 0;		// no prior chars
	while (count > 0) {
		do {				    /* slide through source */
			llc = lc;
			lc = uc1;
			sourcebase = source;	// save possible match point
			uc1 = u8_nextU(&source);
			if (u_isupper(uc1) && !flags)  /* if source char uc and want insens comp */
				uc1 = u_tolower(uc1);		/* lower it */
			else if (nocodes && iscodechar(uc1))	{	/* if ignoring code chars */
				llc = lc;
				lc = uc1;
				sourcebase = source;	// save possible match point
				uc1 = u8_nextU(&source);	/* skip */
				count--;
			}
		} while (--count > 0 && uc1 != utarget[0]);    /* while start char doesn't match & not at end of source */

		for (sp = source, tp = utarget, scount = ulen; uc1 == *tp && scount; tp++)  {	/* now start comparing chars if a match */
			for (trailcode = 0; nocodes && iscodechar(*sp); sp += 2, trailcode +=2)	/* if ignoring code chars */
				;	/* skip */
			if (!--scount && (!word || (!u_isalnum(lc) || iscodechar(llc)) && (!u_isalnum(u8_toU(sp)) || (word&CSWORD))))	{	/* if at end of target && not word, or ok match */
				*actuallen = sp-sourcebase-trailcode;	/* actual length includes all codes but trailing ones */
				return (sourcebase);	/* must have a match */
			}
			if (u_isupper(uc1 = u8_nextU(&sp)) && !flags)	   /* if source uc and want insens comp */
				uc1 = u_tolower(uc1);		/* lower it */
		}
	}
	return (NULL);	/* no match */
}
/******************************************************************************/
void str_xpad(char *string, short max, short addonend)	  /* expands string to max fields */

{
	short ftot;
	char * tptr;
	
	ftot = str_xcount(string);		/* get present count */
	if (ftot < max)	{
		tptr = str_xlast(string);	/* point to last string */
		if (addonend && *tptr != EOCS)		/* if want to add empties beyond last and not already there */
			tptr += strlen(tptr)+1;		/* point beyond last string */
		memmove(tptr+max-ftot, tptr, str_xlen(tptr)+1);		/* shift strings */
		memset(tptr,0,max-ftot);		/* clear memory */
	}	
}
/******************************************************************************/
short str_xstrip(char *pos, short min)	  /* strips empty strings from extended (to min) */

{
	char * base;
	short count;	
	
	count = str_xcount(pos);
	while (*pos != EOCS && count > min)	{/* for all components */
		for (base = pos; !*pos && count > min; pos++)	/* while empty strings */
			count--;
		if (pos > base)		/* if any empty string to remove */	
			memmove(base,pos,str_xlen(pos)+1);	/* remove it */
		if (*(pos = base) != EOCS)	/* if start of next string isn't EOCS  */
			pos += strlen(base)+1;
	}
	return (count);		/* # strings left */
}
/******************************************************************************/
int str_adjustcodes(char * dest, int flags)	/* cleans up codes, removes surplus spaces (if trimming) */

{
	struct codeseq ca, cp, ct;
	char * sptr, *dptr, tbuff[MAXREC*2], tchar;
	int spcount,fcount, pending;
	
	/* Limitation (feature?):
		will always ignore style/font change for a range consisting solely of spaces */
	
	cp.style = cp.font = cp.color = '\0';
	ca = cp;
	pending = spcount = fcount = 0;
	if (flags&CC_INPLACE)
		sptr = dptr = dest;		// in place cleanup
	else {
		str_xcpy(tbuff,dest);	/* copy becomes source; write back to original */
		sptr = tbuff;
		dptr = dest;
	}
	do {	/* until end of string */
		if (*sptr == FONTCHR)	{
			pending = TRUE;		/* mark pending code sequence */
			if (*++sptr&FX_COLOR)
				cp.color = *sptr&FX_COLORMASK;	// get color id
			else
				cp.font = *sptr&FX_FONTMASK;	/* get font ID without font flag */
		}
		else if (*sptr == CODECHR)	{	/* accumulate any codes */
			pending = TRUE;		/* mark pending code sequence */
			if (*++sptr&FX_OFF)		/* if off style code */
				cp.style &= ~(*sptr&FX_STYLEMASK);	/* remove from set */
			else						/* on code */
				cp.style |= *sptr;		/* add to set */
		}
		else if (*sptr == SPACE)	{
			if (!spcount || !(flags&CC_ONESPACE))	/* if space and want it */
				spcount++;		/* accumulate it */
		}
		else if (*sptr == (char)0xc2 && *(sptr+1) == (char)0xad)	{	// if soft hypen
			sptr++;
			continue;		// discard it
		}
		else if (!*sptr)	/* new field */
			fcount++;		/* accumulate it */
		else {		/* normal character */
			if ((fcount || dptr == dest) && flags&CC_TRIM)	/* if at trim point && want field trimmed */
				spcount = 0;
			if (fcount)		{		/* if any fields to close */
				ct = cp;				/* hold pending temporarily */
				if (ca.style || ca.font || ca.color)	{	/* if anything active */
					cp.style = cp.font = cp.color = '\0';	/* set clear style as pending */
					pending = TRUE;		/* force close codes at end of field */
				}
				else		/* no codes active; will hold any pending codes */
					pending = FALSE;
			}
			if (pending)	{	/* if have code sequence pending */
				if ((ca.style&cp.style) != ca.style)	{	/* if removing style */
					*dptr++ = CODECHR;
					*dptr++ = (ca.style^cp.style)&ca.style|FX_OFF;	/* turn off what is changed in old style */
					ca.style &= cp.style;		/* current is any residual style */
				}
				if (cp.style&~ca.style || cp.font != ca.font || cp.color != ca.color){	/* if adding style, or font change: any fonts come first */
					if (cp.font != ca.font)	{	/* if changing font */
						if (cp.font) {			/* if to non-default font */
							while (spcount)	{	/* emit any spaces we have pending */
								*dptr++ = SPACE;
								spcount--;
							}
						}
						*dptr++ = FONTCHR;
						*dptr++ = cp.font|FX_FONT;	/* set font */
						ca.font = cp.font;		/* active font is now what previously was potential */
					}
					if (cp.color != ca.color)	{	/* if changing color */
						if (cp.color) {			/* if to non-default color */
							while (spcount)	{	/* emit any spaces we have pending */
								*dptr++ = SPACE;
								spcount--;
							}
						}
						*dptr++ = FONTCHR;
						*dptr++ = cp.color|FX_COLOR;	/* set color */
						ca.color = cp.color;		/* active color is now what previously was potential */
					}
					if (ca.style != cp.style)	{
						while (spcount)	{	/* emit any spaces we have pending */
							*dptr++ = SPACE;
							spcount--;
						}
						*dptr++ = CODECHR;
						*dptr++ = (ca.style^cp.style)&cp.style;		/* turn on what is changed in pending style */
						ca.style |= cp.style;	/* current is current + pending */
					}
				}
				pending = FALSE;
			}
			while (spcount)	{	/* while spaces to emit */
				*dptr++ = SPACE;
				spcount--;
			}
			if (fcount)	{	/* if field(s) to close */
				while (fcount)	{
					*dptr++ = '\0';		/* emit */
					fcount--;
				}
				if ((ct.style || ct.font || ct.color) && *sptr != EOCS)	{	/* if need to restore pending codes and not finished page field */
					/* (NB: EOCS test needed because pre 1.0.5 replace might have left some records badly coded */
					cp = ct;			/* restore them */
					pending = TRUE;
					sptr--;			/* discount forthcoming advance */
					continue;		/* go round again with same trigger character to force emission of codes */
				}
			}
			*dptr++ = *sptr;
		}
	} while (*sptr++ != EOCS);
		/* now fix any misplaced codes around special chars [code positions around <> and {} reversed from v2 to v3] */
	for (dptr = dest; *dptr != EOCS; dptr++)	{	/* for whole xstring */
		if (*dptr == ESCCHR || *dptr == KEEPCHR || *dptr == OBRACE || *dptr == OBRACKET)	{	/* for chars that should be after codes */
			for (sptr = dptr++; iscodechar(*dptr); dptr+=2)	/* while code sequences follow */
				;		// skip them
			if (dptr > sptr+1)	{	/* if passed over codes */
				tchar = *sptr;
				memmove(sptr,sptr+1,dptr-sptr);	// move special char after code sequences
				*--dptr = tchar;
			}
		}
		else if (*dptr == CBRACE || *dptr == CBRACKET)	{	/* for chars that should be before codes */
			for (sptr = dptr; sptr >= dest+2 && iscodechar(*(sptr-2)); sptr -=2)	/* while code sequences precede */
				;
			if (sptr < dptr)	{		/* if passed over codes */
				tchar = *dptr;		// move char before code sequence
				memmove(sptr+1,sptr,dptr-sptr);
				*sptr = tchar;
			}
		}
	}
	return (dptr-dest);
}
/******************************************************************************/
char * str_xfindcross(INDEX * FF, register char * string, short sflag)	/* find cross-ref (if any) in string */

{
#if 0
	unsigned short matchlen, tlen;
	
	for (matchlen = 0; isgraph(FF->head.refpars.crosstart[matchlen]); matchlen++)	/* find length of segment to search for */
		; 
	if (matchlen)	{	/* if anything to seek */
		while (string = str_xfind(string,FF->head.refpars.crosstart,sflag,matchlen,&tlen)) {	/* while possible refs */
			if (str_crosscheck(FF,string))		/* if a real cross ref */
				return (string);
			string++;
		}
	}
	return (NULL);
#else
	char * base = FF->head.refpars.crosstart;
	char * tptr = strchr(base, SPACE);
	
	if (tptr)	{	/* if anything to seek */
		unsigned short tlen;
		while (string = str_xfind(string,base,sflag,tptr-base,&tlen)) {	/* while possible refs */
			if (str_crosscheck(FF,string))		/* if a real cross ref */
				return (string);
			string++;
		}
	}
	return (NULL);
#endif
}
/******************************************************************************/
short str_crosscheck(INDEX * FF, char *str)    /* checks string to see if begins with 'see' */

{
	char *prevptr = str;
	char *ptr = FF->head.refpars.crosstart;
	unichar uc1, uc2, lc;

	lc = u8_prevU(&prevptr);
	if (lc != KEEPCHR && lc != ESCCHR && (!u_isalpha(lc) || iscodechar(u8_prevU(&prevptr))))	{	/* if not preceded by unescaped alpha */
		while (*str == '(' || *str == '[' || *str == OBRACE || iscodechar(*str) && *++str)	/* while acceptable lead */
			str++;				/* skip it */
		do {
			uc1 = u8_nextU(&ptr);
			if (u_isupper(uc1))       /* if upper case */
				uc1 =  u_tolower(uc1);
			while (iscodechar(*str) && *++str)		/* if code char */
				str++;               /* skip it */
			uc2 = u8_nextU(&str);
			if (u_isupper(uc2))      /* if upper */
				uc2 = u_tolower(uc2);
		} while (uc1 == uc2 && uc1 != SPACE && *ptr && uc1);  /* while match and not at end of elist */
		return (!(uc1-uc2));			/* return true if a match */
	}
	return (FALSE);	
}
/******************************************************************************/
static char *skiplist(char *base, char *list, short * tokens, int xflags)		/* points to first word in base that isn't in list */

{
	register char *sptr, *eptr;
	unsigned short tlen;

	eptr = base;		 /* set start */
	*tokens = 0;		/* number of tokens parsed */
	do {
		for (base = sptr = eptr; *sptr == SPACE || iscodechar(*sptr);)  {	  /* discard leading trash */
			if (*sptr++ == SPACE)
				base = sptr;	     /* base is always char after last space */
			else if (*sptr)			/* if code follows CODECHR */
				sptr++;				/* lose the code */
		}
		for (eptr = sptr; *eptr != SPACE && !iscodechar(*eptr) && *eptr; eptr++)		 /* until find end of good string */
			;		/* eptr now at end of real word */
	} while (str_xfind(list,sptr,(short)xflags,(unsigned short)(eptr-sptr),&tlen) && ++(*tokens));	/* while lead words in base exist as full words in list (within right substring) */
	return (base);	/* return position of mismatch */
}
#if 0
/******************************************************************************/
char *str_skipbrackets(char *sptr)	/* skips to char beyond closing bracket */

{
//	while (*sptr && (*sptr++ != CBRACKET || *(sptr-2) == ESCCHR || *(sptr-2) == KEEPCHR))   /* skip to closing bracket */
//		;
	while (*sptr)	{
		if ((*sptr == ESCCHR || *sptr == KEEPCHR) && *(sptr+1))	/* if have escaped char */
			sptr++;		/* skip it */
		if (*sptr++ == CBRACKET)	/* if hit closing bracket */
			break;
	}
	return (sptr);
}
#endif
/******************************************************************************/
char *str_skiptoclose(char *sptr, unichar tc)	/* skips to char beyond closing char */

{
#if 0
	while (*sptr)	{
		unichar cc = u8_nextU(&sptr);
		if ((cc == ESCCHR || cc == KEEPCHR) && *sptr)	/* if have escaped char */
			u8_forward1(sptr);		/* skip it */
		else if (cc == CBRACE)	/* if hit closing brace */
			break;
	}
	return (sptr);
#else
	int sindex = 0;
	unichar cc;

	do {
		U8_NEXT_UNSAFE(sptr,sindex,cc);
		if (cc == ESCCHR || cc == KEEPCHR)	{	// if escaping succeeding char
			U8_NEXT_UNSAFE(sptr,sindex,cc);		// skip it
		}
		else if (cc == tc)	// if hit target
			break;
	} while (cc);
	return (cc ? sptr+sindex : sptr);	// don't advance if no closer
#endif
}
/******************************************************************************/
char *str_skiplist(char *base, char *list, short * tokens)		/* points to first word in base that isn't in list */

{
	return (skiplist(base, list, tokens, CSINGLE|CWORD));
}
/******************************************************************************/
char *str_skiplistrev(char *base, char *list, short * tokens)		/* points to end of last word in base that isn't in list */

{
	char copy[MAXREC];
	char * eptr, *sptr, *lasteptr;
	short dtokens;

	strcpy(copy, base);
	eptr = copy + strlen(copy);
	lasteptr = eptr;
	do {
		*eptr = '\0';
		while (eptr >= copy && *eptr != SPACE)	// step back to start of word
			eptr--;
		sptr = skiplist(eptr + 1, list, &dtokens, CSINGLE | CWORD);
	} while (sptr > eptr + 1 && eptr >= copy && (lasteptr = eptr));
	return base + (lasteptr - copy);
}
/******************************************************************************/
char *str_skiplistmax(char *base, char *list)		/* points to first char in base that isn't start of potential word in list */

{
	short tokens;

	return (skiplist(base, list, &tokens, CSINGLE| CWORD));
}
/******************************************************************************/
int str_xtextlen(char *cptr)	   /* counts # of text chars in xstring (ignores codes) */

{
	int count = 0;
	
	while (*cptr != EOCS)	{
		if (iscodechar(*cptr++))
			cptr++;
		else
			count++;
	}
	return (count);
}
/******************************************************************************/
int str_textlen(char *cptr)	   /* counts # of text chars in string (ignores codes) */

{
	int count;

	for (count = 0; *cptr;)	{
		if (iscodechar(*cptr++) && *cptr)
			cptr++;
		else
			count++;
	}
	return (count);
}
/******************************************************************************/
int str_utextlen(char *cptr, int length)	   /* counts # of unicode chars in string (ignores codes) */

{
	int ucount = 0, ccount = 0;

	if (!length)
		length = strlen(cptr);
	while (ccount < length) {
		if (iscodechar(cptr[ccount]) && cptr[ccount + 1])	// skip code chars
			ccount += 2;
		else {
			U8_FWD_1_UNSAFE(cptr, ccount);
			ucount++;
		}
	}
	return (ucount);
}
/******************************************************************************/
void str_textcpy(char *dptr, char *cptr)	   /* copy text, stripping codes */

{
	while (*dptr = *cptr)	{
		if (iscodechar(*cptr++) && *cptr)
			cptr++;
		else
			dptr++;
	}
}
/******************************************************************************/
int str_textcpylimit(char *base, char *cptr, char *limit)	   /* copy text up to limit, stripping codes */

{
	char *dptr = base;
	
	while ((*dptr = *cptr) && (!limit || cptr < limit))	{
		if (iscodechar(*cptr++) && *cptr)
			cptr++;
		else
			dptr++;
	}
	*dptr = '\0';	// ensure termination
	return dptr-base;	// length of string
}
/******************************************************************************/
int str_texticmp(char *s1, char *s2)	   /* compares case insensitive, ignoring codes, up s1 len */

{
	unichar uc1, uc2;

	do	{
		while (iscodechar(*s1) && *++s1)
			s1++;
		while (iscodechar(*s2) && *++s2)
			s2++;
		uc1 = u8_nextU(&s1);
		if (u_isupper(uc1))
			uc1 = u_tolower(uc1);
		uc2 = u8_nextU(&s2);
		if (u_isupper(uc2))
			uc2 = u_tolower(uc2);
	} while (*s1 && uc1 == uc2);
	return (uc1-uc2);
}
/******************************************************************************/
void str_setgetlimits(unsigned char *base, unsigned long limit)	// sets limits on read buffer
{
	_rlimit = limit;
	_rbase = base;
	_rcur = 0;	// current index;
}
/******************************************************************************/
char * str_getline(char * buff, int max, int * ctptr)	// reads input line from buffer

{
	// strips empty lines
	int count;

	for (count = 0; count < max && _rcur < _rlimit; count++)	{
		if (_rbase[_rcur] == RETURN || _rbase[_rcur] == NEWLINE)	{		// collapse over line endings; return 0
			while (_rcur < _rlimit && (_rbase[_rcur] == RETURN || _rbase[_rcur] == NEWLINE))
				_rcur++;
			if (count)	// if we're ending some string
				break;
		}
		buff[count] = _rbase[_rcur++];
		if (iscodechar(buff[count]) && _rcur < _rlimit)		// if code char
			buff[++count] = _rbase[_rcur++];	// capture next so can't be treated as CR or LF
	}
	if (count == max)	// if reached buffer limit
		count--;		// make room for terminating null
	buff[count] = '\0';		// terminate every line
	*ctptr = count;
	return (count || _rcur < _rlimit ? &_rbase[_rcur] : NULL);
}
