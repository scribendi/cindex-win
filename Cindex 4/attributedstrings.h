//
//  attributedstrings.h
//  Cindex
//
//  Created by Peter Lennie on 1/19/11.
//  Copyright 2011 Peter Lennie. All rights reserved.
//
#pragma once

#define SUPSUBSTEP 3 	/* offset for super/subscript */

#define SMALLCAPSCALE (0.7)	// size reduction for small caps
#define SUPSUBCALE (0.7)		// size reduction for super sub
#define SUBDROP (0.2)		// sub drop as fraction of normal font size
#define SUPERRISE (1.-SUPSUBCALE+SUBDROP/2.)	// super lift as fraction of normal font size (1 - reduced scale + half of sub drop)

#define FSTACKSIZE 10

#define pushfont(AS,CD) ((AS)->fstack[(AS)->findex < FSTACKSIZE-1 ? ++(AS)->findex : FSTACKSIZE-1] = (CD))
#define popfont(AS)		((AS)->fstack[(AS)->findex > 0 ? (AS)->findex-- : 0])
#define getprevfont(AS)	((AS)->fstack[(AS)->findex > 0 ? --(AS)->findex : 0])
#define secondfont(AS)	((AS)->fstack[(AS)->findex-1 > 0 ? (AS)->findex-1 : 0])
#define currentfont(AS)	((AS)->fstack[(AS)->findex])
#define atbasefont(AS)	((AS)->basefont == (AS)->findex || !(AS)->findex)
#define setbasefont(AS)	((AS)->basefont = (AS)->findex)

enum {		/* font/style code indexes for count array */
	FX_BOLDX = 0,
	FX_ITALX,
	FX_ULINEX,
	FX_SMALLX,
	FX_SUPERX,
	FX_SUBX,
	FX_BOLDITALX,
	
	FX_XTOT
};
enum {		// attributed string modes
	ATS_STRIP = 1,
	ATS_NEWLINES = 2,
	ATS_XSTRING = 4
};

typedef struct {		/* character attribute structure */
	short attr;			/* textface attributes */
	short attcount[FX_XTOT];	/* counts # of outstanding calls to attribute */
	short nsize;		/* size for standard characters */
	short cursize;		/* current size */
	float soffset;			/* current offset from baseline, -ve for sub, +ve for super */
	short findex;		/* font stack index */
	short fstack[FSTACKSIZE];	/* stack of font ids */
	short basefont;		/* local id of current base font */
	FONTMAP * fmp;		/* font mapping struct */
	short color;		// text color
} ATTRIBUTES;

typedef struct {
	int offset;
	char code;
} CODESET;

typedef struct {
	HEAD * headparams;
	unichar * string;
	unsigned int length;
	CODESET * codesets;
	unsigned int codecount;
} ATTRIBUTEDSTRING;

int astr_levelatindex(ATTRIBUTEDSTRING * as, int index);	// finds level at index
int astr_attributesatindex(ATTRIBUTEDSTRING * as, int index, ATTRIBUTES *attr);	// fills attributes for char at index
int astr_toUTF8string(char *xstring, ATTRIBUTEDSTRING * as);	// converts astring to xstring
ATTRIBUTEDSTRING * astr_fromformattedUTF8string(char *string, HEAD * hp, int mode);	// returns ATTRIBUTEDSTRING from formatted string
ATTRIBUTEDSTRING * astr_fromUTF8string(char *string, int mode);	// creates & loads ATTRIBUTEDSTRING from xstring
void astr_loadUTF8string(ATTRIBUTEDSTRING * as,char *string, unsigned char termchar);	// loads ATTRIBUTEDSTRING from xstring
ATTRIBUTEDSTRING * astr_createforsize(int length);
void astr_setattributesforheading(ATTRIBUTEDSTRING * as, int level, ATTRIBUTES * attr);	// sets attributes for heading level
void astr_free(ATTRIBUTEDSTRING * as);
void astr_normalize(ATTRIBUTEDSTRING * as);		// normalizes string to composed characters
