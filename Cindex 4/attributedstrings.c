//
//  attributedstrings.m
//  Cindex
//
//  Created by Peter Lennie on 1/19/11.
//  Copyright 2011 Peter Lennie. All rights reserved.
//

#include "stdafx.h"
#include "attributedstrings.h"
#include "strings.h"
#include "util.h"

static void setattributesfromcode(ATTRIBUTES * as, unsigned char codes);	/* set up attributes from code byte */

/***************************************************************************/
int astr_levelatindex(ATTRIBUTEDSTRING * as, int index)	// finds level at index

{
	int charindex, level;
		
	for (level = charindex = 0; charindex <= index; charindex++)	{
		unichar uc = as->string[charindex];
		
		if (uc == FO_NEWLEVEL || uc == FO_LEVELBREAK)		// if a heading break
			level++;
	}
	return level;
}
/***************************************************************************/
int astr_attributesatindex(ATTRIBUTEDSTRING * as, int index, ATTRIBUTES *attr)	// fills attributes for char at index

{
	int limitcharindex = as->length; 
	int basecharindex, codeindex;
	
//	memset(&as->currentattributes,0,sizeof(ATTRIBUTES));	// clear attributes
	for (codeindex = 0; codeindex < as->codecount; codeindex++)	{
		if (as->codesets[codeindex].offset > index) {
			limitcharindex = as->codesets[codeindex].offset;
			break;
		}
		setattributesfromcode(attr, as->codesets[codeindex].code);
	}
	// find span up to next code or line break
	for (basecharindex = index+1; basecharindex < limitcharindex; basecharindex++) {
		unichar uc = as->string[basecharindex];
		
		if (uc == FO_NEWLEVEL || uc == FO_LEVELBREAK)		// if a heading break
			break;
	}
	return basecharindex-index;
}
/***************************************************************************/
int astr_toUTF8string(char *xstring, ATTRIBUTEDSTRING * as)	// converts astring to xstring

{
	char * dptr = xstring;
	int sindex, codeindex, charlength;
	
	for (codeindex = sindex = 0; sindex < as->length; sindex++)	{
		UErrorCode error = U_ZERO_ERROR;
		while (codeindex < as->codecount && as->codesets[codeindex].offset == sindex)	{	// while need codes at this offset 
			char code = as->codesets[codeindex].code;
			
			if (code&FX_AUTOFONT)
				*dptr++ = FONTCHR;
			else
				*dptr++ = CODECHR;
			*dptr++ = code&~FX_AUTOFONT;
			codeindex++;
		}
		if (!u_iscntrl(as->string[sindex]) || !as->string[sindex])	{	// if isn't a control character (could be through drop/paste in record) or is 0
			u_strToUTF8(dptr,10,&charlength,&as->string[sindex],1,&error);	// get string for char
			if (error == U_ZERO_ERROR)
				dptr += charlength;
			else
				dptr = u8_appendU(dptr,REPLACECHAR);	// unknown char
		}
	}
	*dptr++ = '\0';	// always terminate last field (presumed locator)
	*dptr++ = EOCS;
	return dptr-xstring;
}
/***************************************************************************/
ATTRIBUTEDSTRING * astr_fromformattedUTF8string(char *string, HEAD * hp, int mode)	// returns ATTRIBUTEDSTRING from formatted string

{
	ATTRIBUTEDSTRING * as = astr_fromUTF8string(string,mode);	// for string
	
	as->headparams = hp;
	return as;
}
/***************************************************************************/
ATTRIBUTEDSTRING * astr_fromUTF8string(char *string, int mode)	// creates & loads ATTRIBUTEDSTRING from xstring

{
	int size = mode&ATS_XSTRING ? str_xlen(string) : strlen(string);
	ATTRIBUTEDSTRING * as = astr_createforsize(size+1);	//	+1 is to accommodate any terminating char added later
	int sindex;
	
	for (sindex = 0; sindex < size;)	{	// build text string
		unichar c;
		
		while (iscodechar(string[sindex]))	{	// while in code chars
			as->codesets[as->codecount].offset = as->length;	// position of code
			if (string[sindex++] == FONTCHR)
				as->codesets[as->codecount].code = string[sindex]|FX_AUTOFONT;
			else
				as->codesets[as->codecount].code = string[sindex];
			as->codecount++;
			sindex++;
			goto donecodes;
		}
		U8_NEXT_UNSAFE(string,sindex,c);	// convert char, increment sindex
		if (c == 0 && mode&ATS_NEWLINES)	// if want to make newlines (for record editing)
			c = '\n';
		else if ((c == KEEPCHR || c == ESCCHR) && mode&ATS_STRIP && sindex < size)	// if want to strip (for formatted display)
			U8_NEXT_UNSAFE(string,sindex,c);	// convert char, increment sindex
		as->string[as->length++] = c;
		donecodes:		// skips to here after doing codes
			;
	}
	if (mode&ATS_XSTRING)	// if dealing with Xstring
		as->length--;		// strip newline from end of last field
	return as;
}
/***************************************************************************/
void astr_loadUTF8string(ATTRIBUTEDSTRING * as,char *string, unsigned char termchar)	// loads ATTRIBUTEDSTRING from xstring

{
	int size = termchar == EOCS ? str_xlen(string) : strlen(string);
	int sindex;
	
	as->codecount = as->length = 0;
	for (sindex = 0; sindex < size;)	{	// build text string
		unichar c;
		
		while (string[sindex] == CODECHR || string[sindex] == FONTCHR)	{	// while in code chars
			as->codesets[as->codecount].offset = as->length;	// position of code
			if (string[sindex++] == FONTCHR)
				as->codesets[as->codecount].code = string[sindex]|FX_AUTOFONT;
			else
				as->codesets[as->codecount].code = string[sindex];
			as->codecount++;
			sindex++;
			goto donecodes;
		}
		U8_NEXT_UNSAFE(string,sindex,c);	// convert char, increment sindex
		if (c == 0)
			c = '\n';
		as->string[as->length++] = c;
		donecodes:		// skips to here after doing codes
			;
	}
	if (termchar == EOCS)	// if dealing with Xstring
		as->length--;		// strip newline from end of last field
}
/***************************************************************************/
ATTRIBUTEDSTRING * astr_createforsize(int length)

{
	ATTRIBUTEDSTRING * as;
	
	as = calloc(1,sizeof(ATTRIBUTEDSTRING));
	as->string = malloc(length*sizeof(unichar));
	as->codesets = malloc((length/2+FIELDLIM*2)*sizeof(CODESET));	// allow some extra because could have empty fields with codes
	return as;
}
/***************************************************************************/
void astr_setattributesforheading(ATTRIBUTEDSTRING * as, int level, ATTRIBUTES * attr)	// sets attributes for heading level

{
	int size;
	// don't set default heading styles, because these are provided by buildentry 
	if (as->headparams->privpars.vmode == VM_FULL)	{	// if full format
		if (level >= 0)	// if a normal heading
			size = as->headparams->formpars.ef.field[level].size;
		else	// group header
			size = as->headparams->formpars.ef.eg.gsize;
		if (!size)
			size = as->headparams->privpars.size;
	}
	else 		// draft; use default size
		size = as->headparams->privpars.size;
	memset(attr,0,sizeof(ATTRIBUTES));	// clear attributes
	attr->fmp = as->headparams->fm;
	attr->nsize = size ? size : 12;	// must have real size
	pushfont(attr,0);	// set up base font
}
/***************************************************************************/
void astr_free(ATTRIBUTEDSTRING * as)

{
	free(as->codesets);
	free(as->string);
	free(as);
}
/***************************************************************************************/
void astr_normalize(ATTRIBUTEDSTRING * as)		// normalizes string to composed characters

{
	static const UNormalizer2 * n2;
	UErrorCode error = U_ZERO_ERROR;
	unsigned int cleanlength;
	
	if (!n2)	{
		n2 = unorm2_getInstance(NULL,"nfc",UNORM2_COMPOSE,&error);
		error = U_ZERO_ERROR;
	}
	cleanlength = unorm2_spanQuickCheckYes(n2,as->string,as->length,&error);
	if (cleanlength < as->length && error == U_ZERO_ERROR)	{		// if need normalization
		unichar * newstring = malloc(2*as->length*sizeof(unichar));
		int newlength;

		u_strncpy(newstring,as->string,cleanlength);
		newlength = unorm2_normalizeSecondAndAppend(n2,newstring,cleanlength,2*as->length,&as->string[cleanlength],as->length-cleanlength,&error);
		if (error == U_ZERO_ERROR)	{
			free(as->string);
			as->string = newstring;
			as->length = newlength;
		}
	}
}
/**********************************************************************/
static void setattributesfromcode(ATTRIBUTES * as, unsigned char codes)	/* set up attributes from code byte */

{
	if (codes&FX_AUTOFONT)	{	// if a font/color
		codes &= ~FX_AUTOFONT;
		if (codes&FX_COLOR)		// color change
			as->color = codes&FX_COLORMASK;
		else {		// font change
			int newbase;

			currentfont(as);
			newbase = codes&FX_AUTOFONT;		/* flags change of base font */
			codes &= FX_FONTMASK;
			if (!codes)	{				/* if want to go back to current base */
				while (!atbasefont(as))	/* while not at base font */
					getprevfont(as);
				currentfont(as);
			}
			else if (codes == secondfont(as))	/* if reverting to last font */
				getprevfont(as);		/* pop current; get previous */
			else
				pushfont(as,codes);
			if (newbase)
				setbasefont(as);		/* set new base font */
		}
	}
	else {				/* a style code */
		if (codes&FX_OFF)	{	/* if turning off */
			if (codes&FX_BOLD && as->attcount[FX_BOLDX] && !--as->attcount[FX_BOLDX])
				as->attr &= ~FX_BOLD;
			if (codes&FX_ITAL && as->attcount[FX_ITALX] && !--as->attcount[FX_ITALX])
				as->attr &= ~FX_ITAL;
			if (codes&FX_ULINE && as->attcount[FX_ULINEX] && !--as->attcount[FX_ULINEX])
				as->attr &= ~FX_ULINE;
			if (codes&FX_SMALL && as->attcount[FX_SMALLX] && !--as->attcount[FX_SMALLX])
				as->attr &= ~FX_SMALL;
			if (codes&(FX_SUPER|FX_SUB))		/* if sub or superscript */
				as->soffset = 0;
		}
		else {					/* turning on */
			if (codes&FX_BOLD && !as->attcount[FX_BOLDX]++)
				as->attr |= FX_BOLD;
			if (codes&FX_ITAL && !as->attcount[FX_ITALX]++)
				as->attr |= FX_ITAL;
			if (codes&FX_ULINE && !as->attcount[FX_ULINEX]++)
				as->attr |= FX_ULINE;
			if (codes&FX_SMALL && !as->attcount[FX_SMALLX]++)
				as->attr |= FX_SMALL;
			if (codes&FX_SUPER && as->soffset <= 0)	/* if not already in super position */
				as->soffset = 1;
			if (codes&FX_SUB && as->soffset >= 0)	/* if not already in sub position */
				as->soffset = -1;
		}
	}
}

