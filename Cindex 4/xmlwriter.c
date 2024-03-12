//
//  xmlwriter.m
//  Cindex
//
//  Created by Peter Lennie on 1/29/11.
//  Copyright 2011 Peter Lennie. All rights reserved.
//

#include "stdafx.h"
#include "xmlwriter.h"
#include "strings.h"
#include "tagstuff.h"
#include "util.h"

static char * structtags[T_STRUCTCOUNT];
static char * styletags[T_STYLECOUNT];
static char * fonttags[T_FONTCOUNT];	// pointers to strings that define font codes
static char * auxtags[T_OTHERCOUNT];	// pointers to auxiliary tags
static char protchars[MAXTSTRINGS] = "<>&'\"";	/* characters needing protection */

static BOOL xmlinit(INDEX * FF, FCONTROLX * fxp);	/* initializes control struct and emits header */
static void xmlcleanup(INDEX * FF, FCONTROLX * fxp);	/* cleans up */

FCONTROLX xmlcontrol = {
	0,				// file type
//	NULL,			/* file pointer */
	NULL,			/* string pointer */
	xmlinit,		/* initialize */
	xmlcleanup,		/* cleanup */
	NULL,			/* embedder */
	NULL,			// character writer
	structtags,		/* structure tags */
	styletags,		/* styles */
	fonttags,		/* fonts */
	auxtags,		// auxiliary tags
	"<0x000A>",		/* line break */
	"\n",			// new para (set at format time)
	NULL,			/* obligatory newline (set at format time) */
	"\t",		/* tab */
	protchars,		/* characters needing protection */
	{				/* translation strings for protected characters */
		"&lt;",
		"&gt;",
		"&amp;",
		"&apos;",
		"&quot;"
	},
	FALSE,			/* define lead indent with style (not tabs) */
	FALSE,			/* don't suppress ref lead/end */
	FALSE,			// don't tag individual refs
	FALSE,			// don't tag individual crossrefs
	FALSE,			// no nested tags
	TRUE			/* internal code set*/
};
static char stringbase[1500];	/* strings that define dynamically-built formatting tags */
static TAGSET * t_th;
/**********************************************************************/
static BOOL xmlinit(INDEX * FF, FCONTROLX * fxp)	/* initializes control struct and emits header */

{
	static char *start = "<%s>";
	static char *end = "</%s>";
	char * sptr = stringbase;
	int count;
	char *tagptr;
	int tagindex;
	char * tagtype;
	
	
	if (t_th = ts_openset(ts_getactivetagsetpath(XMLTAGS)))	{			/* if can get tag set */
		fxp->suppressrefs = t_th->suppress;	// ref lead/end control
		fxp->nested = t_th->nested;			// nested heading tags
		fxp->individualrefs = t_th->individualrefs;	// tag individual refs
		fxp->individualcrossrefs = t_th->individualrefs;	// tag individual crossrefs (always same as for page refs)
		for (count = 0; count < STR_MAIN; count++)	{
			if (count&1)	{		// a closing tag
				tagindex = T_STRUCTBASE+count-1;
				tagtype = end;
			}
			else {
				tagindex = T_STRUCTBASE+count;
				tagtype = start;
			}
			structtags[count] = sptr;	// pointer to full tag string
			tagptr = str_xatindex(t_th->xstr,tagindex);
			if (*tagptr)
				sptr += sprintf(sptr, tagtype, tagptr);
			*sptr++ = '\0';
		}
		for (; count < STR_PAGE; count++)	{	// headings and subheadings
			if (count >= STR_MAINEND)	{		// a closing tag
				char endstring[256], * eptr;

				strcpy(endstring,str_xatindex(t_th->xstr,count-15));
				if ((eptr = strchr(endstring,SPACE)))	// if space in string, terminate it
					*eptr = '\0';
				tagptr = endstring;
				tagtype = end;
			}
			else {				
				char levelstring[256];

				if (t_th->levelmode && count >= T_STRUCTBASE+STR_MAIN)	// identify by id
					sprintf(levelstring,"<%%s level=\"%d\">",count-(T_STRUCTBASE+STR_MAIN)+1);	// id is 1-based
				else
					strcpy(levelstring,start);
				tagptr = str_xatindex(t_th->xstr,count);
				tagtype = levelstring;
			}
			structtags[count] = sptr;	// pointer to complete tag string
			if (*tagptr)
				sptr += sprintf(sptr, tagtype, tagptr);
			*sptr++ = '\0';
		}
		for (; count < T_STRUCTCOUNT; count++)	{	// page and cross refs
			if (count&1)	{		// a closing tag
				tagindex = count-1;
				tagtype = end;
			}
			else {
				tagindex = count;
				tagtype = start;
			}
			structtags[count] = sptr;	// pointer to full tag string
			tagptr = str_xatindex(t_th->xstr,tagindex);
			if (*tagptr)
				sptr += sprintf(sptr, tagtype, tagptr);
			*sptr++ = '\0';
		}
		for (count = 0; count < T_STYLECOUNT; count++)	{
			if (count&1)	{		// a closing tag
				tagindex = T_STYLEBASE+count-1;
				tagtype = end;
			}
			else {
				tagindex = T_STYLEBASE+count;
				tagtype = start;
			}
			styletags[count] = sptr;	// pointer to full tag string
			tagptr = str_xatindex(t_th->xstr,tagindex);
			if (*tagptr)
				sptr += sprintf(sptr, tagtype, tagptr);
			*sptr++ = '\0';
		}
		for (count = 0; count < T_FONTCOUNT; count++)	{
			if (count&1)	{		// a closing tag
				tagindex = T_FONTBASE+count-1;
				tagtype = end;
			}
			else {
				char fontstring[50];
				
				if (t_th->fontmode == 1)	// identify by id
					sprintf(fontstring,"<%%s id=\"%d\">",count/2);
				else if (t_th->fontmode == 2)	// identify by name
					sprintf(fontstring,"<%%s name=\"%s\">",FF->head.fm[count].pname);
				else
					strcpy(fontstring,start);
				tagindex = T_FONTBASE+count;
				tagtype = fontstring;
			}
			fonttags[count] = sptr;	// pointer to full tag string
			tagptr = str_xatindex(t_th->xstr,tagindex);
			if (*tagptr)
				sptr += sprintf(sptr, tagtype, tagptr);
			*sptr++ = '\0';
		}
		// protected chars already dealt with
		
		// get body text tag out of the auxiliary set
		tagptr = str_xatindex(t_th->xstr, T_OTHERBASE + OT_STARTTEXT);
		auxtags[OT_STARTTEXT] = sptr;
		if (*tagptr)
			sptr += sprintf(sptr, start, tagptr);
		*sptr++ = '\0';
		auxtags[OT_ENDTEXT] = sptr;
		if (*tagptr)
			sptr += sprintf(sptr, end, tagptr);
		*sptr++ = '\0';

		fxp->esptr += sprintf(fxp->esptr,"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
		fxp->esptr += sprintf(fxp->esptr,"<!-- Cindex index -->\n");
		fxp->esptr += sprintf(fxp->esptr,"%s\n",structtags[STR_BEGIN]);		/* doc header */
		if (t_th->fontmode == 1)	{	// if encoding font by id
			int findex;
			fxp->esptr += sprintf(fxp->esptr,"<fonts>\n");
			for (findex = 0; *FF->head.fm[findex].name; findex++)	// for all fonts claimed in index
				fxp->esptr += sprintf(fxp->esptr, "<font id=\"%d\">\n\t<fname>%s</fname>\n\t<aname>%s</aname>\n</font>\n",findex,FF->head.fm[findex].pname,FF->head.fm[findex].name);
			fxp->esptr += sprintf(fxp->esptr,"</fonts>\n");
		}
		return (TRUE);
	}
	return FALSE;
}
/**********************************************************************/
static void xmlcleanup(INDEX * FF, FCONTROLX * fxp)	/* cleans up */

{
	fxp->esptr += sprintf(fxp->esptr,"%s\n",structtags[STR_END]);
	freemem(t_th);
	t_th = NULL;
}
