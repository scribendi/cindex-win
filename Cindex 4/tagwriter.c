//
//  xmlwriter.m
//  Cindex
//
//  Created by Peter Lennie on 1/29/11.
//  Copyright 2011 Peter Lennie. All rights reserved.
//

#include "stdafx.h"
#include "tagwriter.h"
#include "strings.h"
#include "tagstuff.h"
#include "util.h"

static char newparaset[FSSTRING+2];			/* assembled string for new paragraph + space for \r\n */
static char * structtags[T_STRUCTCOUNT];	/* pointers to strings that define structure tags */
static char * styletags[T_STYLECOUNT];		/* pointers to strings that define style codes */
static char * fonttags[T_FONTCOUNT];		/* pointers to strings that define font codes */
static char * auxtags[T_OTHERCOUNT];					// pointers to auxiliary tags
static char protchars[MAXTSTRINGS+1];		/* string of protected ASCII characters */

static BOOL taginit(INDEX * FF, FCONTROLX * fxp);	/* initializes control struct and emits header */
static void tagcleanup(INDEX * FF, FCONTROLX * fxp);	/* cleans up */
static void charwriter(FCONTROLX * fxp,unichar uc);

FCONTROLX tagcontrol = {
	0,				// file type
//	NULL,			/* file pointer */
	NULL,			/* string pointer */
	taginit,		/* initialize */
	tagcleanup,		/* cleanup */
	NULL,			/* embedder */
	charwriter,		// character writer
	structtags,		/* structure tags */
	styletags,		/* styles */
	fonttags,		/* fonts */
	auxtags,		// auxiliary tags
	NULL,			/* line break */
	newparaset,		/* new para */
	NULL,			/* obligatory newline (set at format time) */
	NULL,			/* tab */
	protchars,		/* characters needing protection */
	{
		NULL,		/* translation strings for protected characters */
	},
	FALSE,			/* define lead indent with tag */
	FALSE,			/* don't suppress ref lead/end */
	FALSE,			// don't tag refs individually
	FALSE,			// don't tag individual crossrefs
	FALSE,			// no nested tags
	FALSE			/* not internal code set */
};

static TAGSET * t_th;
/**********************************************************************/
static BOOL taginit(INDEX * FF, FCONTROLX * fxp)	/* initializes control struct and emits header */

{
	if (t_th = ts_openset(ts_getactivetagsetpath(SGMLTAGS)))	{	/* if can get tag set */
		char * fsptr;
		short count;

		fxp->suppressrefs = t_th->suppress;		/* set suppression flag */
		fxp->nested = t_th->nested;			// nested heading tags
		fxp->individualrefs = t_th->individualrefs;	// tag individual refs
		fxp->individualcrossrefs = t_th->individualrefs;	// tag individual crossrefs (always same as for page refs)
		for (fsptr = str_xatindex(t_th->xstr,0), count = 0; count < T_STRUCTCOUNT; count++, fsptr += strlen(fsptr++))
			structtags[count] = fsptr;		/* pointer for struct tag string */
		for (fsptr = str_xatindex(t_th->xstr,T_STYLEBASE), count = 0; count < T_STYLECOUNT; count++, fsptr += strlen(fsptr++))
			styletags[count] = fsptr;		/* pointer for style tag string */
		for (fsptr = str_xatindex(t_th->xstr,T_FONTBASE), count = 0; count < T_FONTCOUNT; count++, fsptr += strlen(fsptr++))
			fonttags[count] = fsptr;		/* pointer for font ID string */
		for (fsptr = str_xatindex(t_th->xstr,T_OTHERBASE+OT_PR1),count = 0; count < MAXTSTRINGS; count++)	{
			protchars[count] = *fsptr;		/* add protected char to string */
			fsptr += strlen(fsptr++);		/* point to partner translation string */
			fxp->pstrings[count] = fsptr;	/* set translation string */
			fsptr += strlen(fsptr++);
		}
		for (fsptr = str_xatindex(t_th->xstr,T_OTHERBASE),count = 0; count < T_OTHERCOUNT; count++,	fsptr += strlen(fsptr++))
			auxtags[count] = fsptr;		// get auxiliary tags
		auxtags[OT_STARTTEXT] = g_nullstr;		// body text start (won't have been set in previous loop because of T_OTHERCOUNT)
		auxtags[OT_ENDTEXT] = g_nullstr;		// body text end
		fxp->newline = str_xatindex(t_th->xstr,T_OTHERBASE+OT_ENDLINE);
		sprintf(newparaset,"%s%s",str_xatindex(t_th->xstr,T_OTHERBASE+OT_PARA), fxp->newlinestring);	/* new para string + newline */
		fxp->tab = str_xatindex(t_th->xstr,T_OTHERBASE+OT_TAB);
		if (*structtags[STR_BEGIN])		/* if have start tag */
			fxp->esptr += sprintf(fxp->esptr,"%s%s",structtags[STR_BEGIN], fxp->newlinestring);		/* doc header */
		return (TRUE);
	}
	return (FALSE);
}
/**********************************************************************/
static void tagcleanup(INDEX * FF, FCONTROLX * fxp)	/* cleans up */

{
	if (*structtags[STR_END])	/* if ending tag */
		fxp->esptr += sprintf(fxp->esptr,"%s%s",structtags[STR_END], fxp->newlinestring);		/* doc footer string */
	freemem(t_th);
	t_th = NULL;
}
/**********************************************************************/
static void charwriter(FCONTROLX * fxp,unichar uc)	// emits unichar

{
	if (uc > 0x80 && !t_th->useUTF8) {	// if want encoding
		char * formatstring = t_th->hex ? "%s%04x%s" : "%s%04d%s";
		fxp->esptr += sprintf(fxp->esptr, formatstring, auxtags[OT_UPREFIX], uc, auxtags[OT_USUFFIX]);
	}
	else
		fxp->esptr = u8_appendU(fxp->esptr, uc);
}
