//
//  imwriter.m
//  Cindex
//
//  Created by PL on 5/24/20.
//

#include "stdafx.h"
#include "errors.h"
#include "viewset.h"
#include "formstuff.h"
#include "formattedexport.h"
#include "tagstuff.h"
#include "imwriter.h"

static char * structtags[T_STRUCTCOUNT] = {
	"",
	"",
	"",
	"",
	"",
	"",
	"",	// main heading
	"– ",
	"– – ",
	"– – – ",
	"– – – – ",
	"– – – – – ",
	"– – – – – – ",
	"– – – – – – – ",
	"– – – – – – – – ",
	"– – – – – – – – – ",
	"– – – ",
	"– – – ",
	"– – – ",
	"– – – ",
	"– – – ",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
};
static char * styletags[] = {
	"<b>",
	"</b>",
	"<i>",
	"</i>",
	"<u>",
	"</u>",
	"<s>",
	"</s>",
	"<+>",
	"</+>",
	"<->",
	"</->",
	"",
	"",
};
static char * fonttags[T_FONTCOUNT] = {	/* pointers to strings that define font codes */
	g_nullstr,g_nullstr,
	g_nullstr,g_nullstr,
	g_nullstr,g_nullstr,
	g_nullstr,g_nullstr,
	g_nullstr,g_nullstr,
	g_nullstr,g_nullstr,
	g_nullstr,g_nullstr,
	g_nullstr,g_nullstr,
	g_nullstr,g_nullstr,
	g_nullstr,g_nullstr,
	g_nullstr,g_nullstr,
	g_nullstr,g_nullstr
};
static char * auxtags[T_OTHERCOUNT];	// pointers to auxiliary tags
static char protchars[MAXTSTRINGS];		/* characters needing protection */

static FORMATPARAMS savedFormat;
static REFPARAMS savedRef;
static char * baseptr;		// base of buffer allocated for export

static BOOL iminit(INDEX * FF, FCONTROLX * fxp);	/* initializes control struct and emits header */
static void imcleanup(INDEX * FF, FCONTROLX * fxp);	/* cleans up */

FCONTROLX imcontrol = {
	0,	// file type
	NULL,			/* string pointer */
	iminit,		/* initialize */
	imcleanup,	/* cleanup */
	NULL,			/* embedder */
	NULL,	// character writer
	structtags,		/* structure tags */
	styletags,		/* styles */
	fonttags,		/* fonts */
	auxtags,		// auxiliary tags
	"",				/* line break */
	NULL,			// new para (set at format time)
	"\n",			// obligatory newline *must* be \n for IM
	"\t",			/* tab */
	protchars,		/* characters needing protection */
	{				/* translation strings for protected characters */
		NULL
	},
	FALSE,			/* define lead indent with style (not tabs) */
	TRUE,			// suppress reference leads / ends
	TRUE,			/* tag individual page refs */
	FALSE,			// don't tag individual crossrefs
	FALSE,			// no nested tags
	TRUE			/* internal code set */
};
/**********************************************************************/
static BOOL iminit(INDEX * FF, FCONTROLX * fxp)	/* initializes control struct and emits header */

{
	structtags[STR_PAGE] = " <idx>";		/* pointer for page tag */
	structtags[STR_PAGEND] = "</idx>";		/* pointer for page end tag */
	structtags[STR_CROSS] = " <idx>";		/* pointer for cross tag */
	structtags[STR_CROSSEND] = "</idx>";	/* pointer for cross end tag */
	structtags[STR_GROUP] = g_nullstr;		// group start tag
	structtags[STR_GROUPEND] = g_nullstr;	// group end tag
	structtags[STR_AHEAD] = g_nullstr;		/* set ahead tag */
	structtags[STR_AHEADEND] = g_nullstr;	/* set ahead end tag */
	auxtags[OT_STARTTEXT] = g_nullstr;		// body text start
	auxtags[OT_ENDTEXT] = g_nullstr;		// body text end
	fxp->newlinestring = "\n";				// special override for IM (won't accept Mac \r as newline)
	fxp->newpara = fxp->newlinestring;	/* set end of line string */
	baseptr = fxp->esptr;		// save base of string for later fixup
	
	memcpy(&savedFormat, &FF->head.formpars,sizeof(FORMATPARAMS));	// save current format params
	memset(&FF->head.formpars.ef.eg,0,sizeof(GROUPFORMAT));		// clear group format
	memset(&FF->head.formpars.ef.lf,0,sizeof(LOCATORFORMAT));		// clear locator format
	FF->head.formpars.ef.lf.sortrefs = TRUE;
	strcpy(FF->head.formpars.ef.lf.connect,"–");	// connector is en-dash
	memset(&FF->head.formpars.ef.cf,0,sizeof(CROSSREFFORMAT));	// clear crossref format
	FF->head.formpars.ef.cf.sortcross = TRUE;

	memcpy(&savedRef, &FF->head.refpars,sizeof(REFPARAMS));	// save current ref params
//	FF->head.refpars.psep = ';';		// page ref separator
	FF->head.refpars.csep = ';';		// cross-ref separator

	return (TRUE);
}
/**********************************************************************/
static void imcleanup(INDEX * FF, FCONTROLX * fxp)	/* cleans up */

{
	typedef struct {
		char * in;
		char * out;
	} ctrans;
	
	static ctrans translations[] = {
		{"<idx><b><i><u>","<idx format='biu'>"},		// on codes, in reverse order of complexity (otherwise bad substitutions)
		{"<idx><i><u>","<idx format='iu'>"},
		{"<idx><b><u>","<idx format='bu'>"},
		{"<idx><b><i>","<idx format='bi'>"},
		{"<idx><b>","<idx format='b'>"},
		{"<idx><i>","<idx format='i'>"},
		{"<idx><u>","<idx format='u'>"},
		
		{"</u></i></b></idx>","</idx>"},			// off codes, in reverse order of complexity
		{"</u></i></idx>","</idx>"},
		{"</u></b></idx>","</idx>"},
		{"</i></b></idx>","</idx>"},
		{"</b></idx>","</idx>"},
		{"</i></idx>","</idx>"},
		{"</u></idx>","</idx>"},
		{"</b></idx>","</idx>"},
		NULL
	};
	memcpy(&FF->head.formpars,&savedFormat, sizeof(FORMATPARAMS));	// restore old params
	memcpy(&FF->head.refpars,&savedRef,sizeof(REFPARAMS));	// restore old params

	//	transform codes for locators
	for (ctrans * tptr = translations; tptr->in; tptr++)	{	// for each possible translation
		char * xptr = baseptr;
		while (xptr = strstr(xptr,tptr->in))	{	// while we have matches in current string
			int inlen = (int)strlen(tptr->in);
			int outlen = (int)strlen(tptr->out);
			memmove(xptr+outlen,xptr+inlen,strlen(xptr)+1);   // shift
			strncpy(xptr,tptr->out,outlen);
			xptr += outlen;
		}
	}
	fxp->esptr = baseptr + strlen(baseptr);	 // reset end ptr
}
