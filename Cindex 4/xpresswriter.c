#include "stdafx.h"
#include "errors.h"
#include "viewset.h"
#include "formattedexport.h"
#include "formstuff.h"
#include "tagstuff.h"
#include "xpresswriter.h"

static char * structtags[T_STRUCTCOUNT];
static char * styletags[] = {
	"<B>",
	"<B>",
	"<I>",
	"<I>",
	"<U>",
	"<U>",
	"<H>",
	"<H>",
	"<+>",
	"<+>",
	"<->",
	"<->",
	"<BI>",
	"<BI>"
};
static char * fonttags[T_FONTCOUNT];	/* pointers to strings that define font codes */
static char * auxtags[T_OTHERCOUNT];	// pointers to auxiliary tags
static char protchars[MAXTSTRINGS] = "\\@<";	/* characters needing protection */
static BOOL xpressinit(INDEX * FF, FCONTROLX * fxp);	/* initializes control struct and emits header */
void xpresscleanup(INDEX * FF, FCONTROLX * fxp);	/* cleans up */
static void xpresswriter(FCONTROLX * fxp,unichar uc);	//

FCONTROLX xpresscontrol = {
	0,	// file type
	NULL,			/* string pointer */
	xpressinit,		/* initialize */
	xpresscleanup,	/* cleanup */
	NULL,			/* embedder */
	xpresswriter,	// character writer
	structtags,		/* structure tags */
	styletags,		/* styles */
	fonttags,		/* fonts */
	auxtags,		// auxiliary tags
	"<\\n>",		/* line break */
	NULL,			/* new para (set at format time) */
	NULL,			/* obligatory newline (set at format time) */
	"<\\t>",		/* tab */
	protchars,		/* characters needing protection */
	{				/* translation strings for protected characters */
		"<\\\\>",
		"<\\@>",
		"<\\<>"
	},
	FALSE,			/* define lead indent with style (not tabs) */
	FALSE,			/* don't suppress ref lead/end */
	FALSE,			// don't tag refs individually
	FALSE,			// don't tag individual crossrefs
	FALSE,			// no nested tags
	TRUE			/* internal code set */
};
#if 0
/**********************************************************************/
static BOOL xpressinit(INDEX * FF, FILE * sptr, char * typename, FCONTROLX * fxp)	/* initializes control struct and emits header */

{
	short count, lspace, rtabpos, width;
	int baseindent, firstindent;
	char * fsptr, rtabchar;
	char tabstops[200];
	char tabstring[FIELDLIM];
	
	width = FF->head.formpars.pf.pi.pwidthactual-(FF->head.formpars.pf.mc.right+FF->head.formpars.pf.mc.left); /* overall width less margins (points) */
	rtabpos = (width-(FF->head.formpars.pf.mc.ncols-1)*FF->head.formpars.pf.mc.gutter)/FF->head.formpars.pf.mc.ncols; /* fix for columns */
	rtabchar = FF->head.formpars.ef.lf.leader ? '.' : SPACE;
	fprintf(sptr,"%s%s","<v1.70><e1>",fxp->newlinestring);	/* load start stuff && set default font */
	for (fsptr = f_stringbase, count = 0; *FF->head.fm[count].pname && count < T_NUMFONTS; count++)	{	/* for all fonts we use */
		fonttags[count*2] = fsptr;		/* pointer for font id string */
		if (!count)		/* if default font */
			fsptr += sprintf(fsptr,"<f$>");
		else
			fsptr += sprintf(fsptr,"<f\"%s\">", FF->head.fm[count].pname)+1;	/* add string for name */
		fonttags[count*2+1] = fsptr;		/* pointer for font id string */
		*fsptr++ = '\0';		/* no off string */
	}
	if (FF->head.formpars.pf.autospace)
		lspace = 0;
	else
		lspace = FF->head.formpars.pf.lineheight;
	for (count = 0; count < FF->head.indexpars.maxfields-1; count++)	{	/* for all text fields */
		formexport_gettypeindents(FF,count,fxp->usetabs, 1,&firstindent,&baseindent,"%d,0,\"1  \",",tabstops,tabstring);	/* gets base and first indents */
		fprintf(sptr,"@%s=[S\"\",\"%s\"]<*L*h\"Standard\"*kn0*kt0*ra0*rb0*d0*p(%d,%d,0,%d,0,0,g,\"U.S. English\")*t(%s%d,2,\"2 %c\")Ps100t0h100z%dk0b0c\"Black\"f\"%s\">%s",
			 FF->head.indexpars.field[count].name,FF->head.indexpars.field[count].name,(short)(baseindent), (short)firstindent,lspace,
			 tabstops,rtabpos,rtabchar,
			 FF->head.privpars.size,FF->head.fm[0].pname,fxp->newlinestring);	/* emit header spec */
		structtags[count+STR_MAIN] = fsptr;				/* pointer for lead string */
		structtags[count+STR_MAINEND] = g_nullstr;		/* pointer for trailing string (none) */
		fsptr += sprintf(fsptr,"@%s:%s", FF->head.indexpars.field[count].name,tabstring)+1;		/* generate lead tag */
	}
	structtags[STR_PAGE] = g_nullstr;		/* pointer for page tag */
	structtags[STR_PAGEND] = g_nullstr;		/* pointer for page tag */
	structtags[STR_CROSS] = g_nullstr;		/* pointer for cross tag */
	structtags[STR_CROSSEND] = g_nullstr;	/* pointer for cross end tag */
	fxp->newpara = fxp->newlinestring;	/* set end of line string */
	fprintf(sptr,"@Ahead=[S\"\",\"Ahead\"]<*L*h\"Standard\"*kn0*kt0*ra0*rb0*d0*p(%d,-%d,0,%d,0,0,g,\"U.S. English\")Ps100t0h100z%dk0b0c\"Black\"f\"%s\">%s",
		 27, 27,lspace,FF->head.privpars.size,FF->head.fm[0].pname,fxp->newlinestring);	/* emit header spec */
	structtags[STR_AHEAD] = fsptr;		/* set ahead tag */
	fsptr += sprintf(fsptr, "@Ahead:");	/* group header tag (style 1) */
	structtags[STR_AHEADEND] = g_nullstr;	/* set ahead end tag */
	return (TRUE);
}
/**********************************************************************/
static void xpresscleanup(INDEX * FF, FILE * sptr)	/* cleans up */

{
	;			/* end of file */
}
#else
/**********************************************************************/
static BOOL xpressinit(INDEX * FF, FCONTROLX * fxp)	/* initializes control struct and emits header */

{
	short count, lspace, rtabpos, width, fieldlimit;
	int baseindent, firstindent, fontsize;
	char * fsptr, rtabchar;
	char tabstops[200];
	char tabstring[FIELDLIM];
	int spacebefore;
	float lineheightmultiple = 1;
	
	width = FF->head.formpars.pf.pi.pwidthactual-(FF->head.formpars.pf.mc.right+FF->head.formpars.pf.mc.left); /* overall width less margins (points) */
	rtabpos = (width-(FF->head.formpars.pf.mc.ncols-1)*FF->head.formpars.pf.mc.gutter)/FF->head.formpars.pf.mc.ncols; /* fix for columns */
	rtabchar = FF->head.formpars.ef.lf.leader ? '.' : SPACE;
	fxp->esptr += sprintf(fxp->esptr,"%s%s","<v4.0><e1>",fxp->newlinestring);	/* load start stuff && set default font */
	for (fsptr = f_stringbase, count = 0; *FF->head.fm[count].pname && count < T_NUMFONTS; count++)	{	/* for all fonts we use */
		fonttags[count*2] = fsptr;		/* pointer for font id string */
		if (!count)		/* if default font */
			fsptr += sprintf(fsptr,"<f$>");
		else
			fsptr += sprintf(fsptr,"<f\"%s\">", FF->head.fm[count].pname)+1;	/* add string for name */
		fonttags[count*2+1] = fsptr;		/* pointer for font id string */
		*fsptr++ = '\0';		/* no off string */
	}
#if 0
	if (FF->head.formpars.pf.autospace)
		lspace = 0;
	else
		lspace = FF->head.formpars.pf.lineheight;
#endif
	if (FF->head.formpars.pf.linespace)	// if have greater than single spacing
		lineheightmultiple = FF->head.formpars.pf.linespace == 1 ? 1.5 : 2;
	if (FF->head.formpars.pf.autospace)	{
		if (FF->head.formpars.pf.linespace)	// if anything other than single space
			lspace = lineheightmultiple*FF->head.privpars.size;	// set space explicitly
		else
			lspace = 0;
	}
	else	// fixed spacing
		lspace = FF->head.formpars.pf.lineheight*lineheightmultiple;	// set space explicitly
	fieldlimit = FF->head.indexpars.maxfields < FIELDLIM ? FF->head.indexpars.maxfields : FF->head.indexpars.maxfields-1;	// need 1 extra level for subhead cref
	for (count = 0; count < fieldlimit; count++)	{	/* for all text fields + 1 extra */
		fontsize = FF->head.formpars.ef.field[count].size ? FF->head.formpars.ef.field[count].size : FF->head.privpars.size;
		if (!count && FF->head.formpars.pf.entryspace)	// if main head and want extra space
			// use fontsize as amount of space until we can get proper extra space metrics
			spacebefore = FF->head.formpars.pf.entryspace*fontsize;
		else
			spacebefore = 0;
		formexport_gettypeindents(FF,count,fxp->usetabs, 1,&firstindent,&baseindent,"%d,0,\"1  \",",tabstops,tabstring);	/* gets base and first indents */
		fxp->esptr += sprintf(fxp->esptr,"@%s=[S\"\",\"%s\"]<*L*h\"Standard\"*kn0*kt0*ra0*rb0*d0*p(%d,%d,0,%d,%d,0,g,\"U.S. English\")*t(%s%d,2,\"2 %c\")Ps100t0h100z%dk0b0c\"Black\"f\"%s\">%s",
//			FF->head.indexpars.field[count].name,FF->head.indexpars.field[count].name,(short)(baseindent), (short)firstindent,lspace,spacebefore,
			fxp->stylenames[count], fxp->stylenames[count], (short)(baseindent), (short)firstindent, lspace, spacebefore,
			tabstops,rtabpos,rtabchar,
			fontsize,FF->head.fm[0].pname,fxp->newlinestring);	/* emit header spec */
		structtags[count+STR_MAIN] = fsptr;				/* pointer for lead string */
		structtags[count+STR_MAINEND] = g_nullstr;		/* pointer for trailing string (none) */
//		fsptr += sprintf(fsptr,"@%s:%s", FF->head.indexpars.field[count].name,tabstring)+1;		/* generate lead tag */
		fsptr += sprintf(fsptr, "@%s:%s", fxp->stylenames[count], tabstring) + 1;		/* generate lead tag */
	}
	structtags[STR_PAGE] = g_nullstr;		/* pointer for page tag */
	structtags[STR_PAGEND] = g_nullstr;		/* pointer for page tag */
	structtags[STR_CROSS] = g_nullstr;		/* pointer for cross tag */
	structtags[STR_CROSSEND] = g_nullstr;	/* pointer for cross end tag */
	auxtags[OT_STARTTEXT] = g_nullstr;		// body text start
	auxtags[OT_ENDTEXT] = g_nullstr;		// body text end
	fxp->newpara = fxp->newlinestring;	/* set end of line string */
	fontsize = FF->head.formpars.ef.eg.gsize ? FF->head.formpars.ef.eg.gsize : FF->head.privpars.size;
	if (FF->head.formpars.pf.above)	// if want extra space before header
		// use fontsize as amount of space until we can get proper extra space metrics
		spacebefore = FF->head.formpars.pf.above*fontsize;
	else
		spacebefore = 0;
//	fxp->esptr += sprintf(fxp->esptr,"@Ahead=[S\"\",\"Ahead\"]<*L*h\"Standard\"*kn0*kt0*ra0*rb0*d0*p(%d,-%d,0,%d,%d,0,g,\"U.S. English\")Ps100t0h100z%dk0b0c\"Black\"f\"%s\">%s",
//		 27, 27,lspace,spacebefore,fontsize,FF->head.fm[0].pname,fxp->newlinestring);	/* emit header spec */
	fxp->esptr += sprintf(fxp->esptr, "@%s=[S\"\",\"%s\"]<*L*h\"Standard\"*kn0*kt0*ra0*rb0*d0*p(%d,-%d,0,%d,%d,0,g,\"U.S. English\")Ps100t0h100z%dk0b0c\"Black\"f\"%s\">%s",
		fxp->stylenames[FIELDLIM - 1], fxp->stylenames[FIELDLIM - 1], 27, 27, lspace, spacebefore, fontsize, FF->head.fm[0].pname, fxp->newlinestring);	/* emit header spec */	structtags[STR_GROUP] = g_nullstr;	// group start tag
	structtags[STR_GROUPEND] = g_nullstr;	// group end tag
	structtags[STR_AHEAD] = fsptr;		/* set ahead tag */
//	fsptr += sprintf(fsptr, "@Ahead:");	/* group header tag (style 1) */
	sprintf(fsptr, "@%s:", fxp->stylenames[FIELDLIM - 1]);	/* group header tag (style 1) */
	structtags[STR_AHEADEND] = g_nullstr;	/* set ahead end tag */
	return (TRUE);
}
/**********************************************************************/
static void xpresscleanup(INDEX * FF, FCONTROLX * fxp)	/* cleans up */

{
	*fxp->esptr = '\0';		/* end of file */
}
/**********************************************************************/
static void xpresswriter(FCONTROLX * fxp,unichar uc)	// emits unichar

{
	fxp->esptr += sprintf(fxp->esptr, "<\\#%d>",uc);
}
#endif