#include "stdafx.h"
#include "errors.h"
#include "viewset.h"
#include "formstuff.h"
#include "formattedexport.h"
#include "tagstuff.h"
#include "indesign.h"

static char * structtags[T_STRUCTCOUNT];
static char * styletags[] = {
	"<cTypeface:Bold>",
	"<cTypeface:>",
	"<cTypeface:Oblique>",
	"<cTypeface:>",	// Regular
	"<cUnderline:1><cUnderlineColor:Black>",
	"<cUnderline:><cUnderlineColor:>",
	"<cCase:Small Caps>",
	"<cCase:>",
	"<cPosition:Superscript>",
	"<cPosition:>",		// Normal
	"<cPosition:Subscript>",
	"<cPosition:>",
	"<cTypeface:Bold Oblique>",
	"<cTypeface:>"
};
static char * fonttags[T_FONTCOUNT];	/* pointers to strings that define font codes */
static char * auxtags[T_OTHERCOUNT];		// pointers to auxiliary tags
static char protchars[MAXTSTRINGS] = "\\<>";	/* characters needing protection */

static BOOL indesigninit(INDEX * FF, FCONTROLX * fxp);	/* initializes control struct and emits header */
void indesigncleanup(INDEX * FF, FCONTROLX * fxp);	/* cleans up */
static void indesignwriter(FCONTROLX * fxp,unichar uc);	//

FCONTROLX indesigncontrol = {
	0,	// file type
//	NULL,			/* file pointer */
	NULL,			/* string pointer */
	indesigninit,		/* initialize */
	indesigncleanup,	/* cleanup */
	NULL,			/* embedder */
	indesignwriter,	// character writer
	structtags,		/* structure tags */
	styletags,		/* styles */
	fonttags,		/* fonts */
	auxtags,		// auxiliary tags
	"<0x000A>",		/* line break */
	NULL,			// new para (set at format time)
	NULL,			/* obligatory newline (set at format time) */
	"\t",			/* tab */
	protchars,		/* characters needing protection */
	{				/* translation strings for protected characters */
		"\\\\",
		"\\<",
		"\\>"
	},
	FALSE,			/* define lead indent with style (not tabs) */
	FALSE,			/* don't suppress ref lead/end */
	FALSE,			// don't tag refs individually
	FALSE,			// don't tag individual crossrefs
	FALSE,			// no nested tags
	TRUE			/* internal code set */
};
/**********************************************************************/
static BOOL indesigninit(INDEX * FF, FCONTROLX * fxp)	/* initializes control struct and emits header */

{
	short count, rtabpos, width, fieldlimit;
	int baseindent, firstindent, fontsize;
	char * fsptr, *rtabtype,lspace[30];
	char tabstops[200];
	char tabstring[FIELDLIM];
	float lineheightmultiple = 1;
	float spacebefore;
	
	width = FF->head.formpars.pf.pi.pwidthactual-(FF->head.formpars.pf.mc.right+FF->head.formpars.pf.mc.left); /* overall width less margins (points) */
	rtabpos = (width-(FF->head.formpars.pf.mc.ncols-1)*FF->head.formpars.pf.mc.gutter)/FF->head.formpars.pf.mc.ncols; /* fix for columns */
	rtabtype = FF->head.formpars.ef.lf.leader ? "." : g_nullstr;
	fxp->esptr += sprintf(fxp->esptr,"<ANSI-WIN>%s%s%s",fxp->newlinestring,"<Version:5><FeatureSet:InDesign-Roman><ColorTable:=<Black:COLOR:CMYK:Process:0,0,0,1>>",fxp->newlinestring);	// load start stuff
	for (fsptr = f_stringbase, count = 0; *FF->head.fm[count].pname && count < T_NUMFONTS; count++)	{	/* for all fonts we use */
		fonttags[count*2] = fsptr;		/* pointer for font id string */
		fsptr += sprintf(fsptr,"<cFont:%s>", FF->head.fm[count].pname)+1;	/* add string for name */
		fonttags[count*2+1] = fsptr;		/* pointer for font id off string */
		fsptr += sprintf(fsptr,"<cFont:>")+1;	// off string
	}
	if (FF->head.formpars.pf.linespace)	// if have greater than single spacing
		lineheightmultiple = FF->head.formpars.pf.linespace == 1 ? 1.5 : 2;
	if (FF->head.formpars.pf.autospace)	{
		if (FF->head.formpars.pf.linespace)	// if anything other than single space
			sprintf(lspace,"%d",(int)(lineheightmultiple*FF->head.privpars.size));	// set space explicitly
		else
			*lspace = '\0';
	}
	else	{	// fixed spacing
		sprintf(lspace,"%d",(int)(FF->head.formpars.pf.lineheight*lineheightmultiple));	// set space explicitly
	}
	fieldlimit = FF->head.indexpars.maxfields < FIELDLIM ? FF->head.indexpars.maxfields : FF->head.indexpars.maxfields-1;	// need 1 extra level for subhead cref
	for (count = 0; count < fieldlimit; count++)	{	/* for all text fields + 1 extra */
		fontsize = FF->head.formpars.ef.field[count].size ? FF->head.formpars.ef.field[count].size : FF->head.privpars.size;
		if (!count && FF->head.formpars.pf.entryspace)	// if main head and want extra space
			// use fontsize as amount of space until we can get proper extra space metrics
			spacebefore = FF->head.formpars.pf.entryspace*fontsize;
		else
			spacebefore = 0;
		formexport_gettypeindents(FF,count,fxp->usetabs, 1,&firstindent,&baseindent,"<pTabRuler:%d\\,Left\\,.\\,0\\,\\;>",tabstops,tabstring);	/* gets base and first indents */
		fxp->esptr += sprintf(fxp->esptr,"<DefineParaStyle:%s=<pHyphenationLadderLimit:0><pLeftIndent:%.4f><pFirstLineIndent:%.4f><pHyphenation:0><pHyphenationZone:0.0><pSpaceBefore:%.4f><cLeading:%s><pTabRuler:%d\\,Right\\,.\\,0\\,%s\\;><cFont:%s><cSize:%d><pKeepWithNext:1>>%s",
//			 FF->head.indexpars.field[count].name,(float)(baseindent), (float)firstindent, spacebefore,lspace,rtabpos,rtabtype,FF->head.formpars.ef.field[count].font,fontsize,fxp->newlinestring);
			fxp->stylenames[count], (float)(baseindent), (float)firstindent, spacebefore, lspace, rtabpos, rtabtype, FF->head.formpars.ef.field[count].font, fontsize, fxp->newlinestring);
		structtags[count+STR_MAIN] = fsptr;				/* pointer for lead string */
		structtags[count+STR_MAINEND] = g_nullstr;		/* pointer for trailing string (none) */
//		fsptr += sprintf(fsptr,"<ParaStyle:%s>", FF->head.indexpars.field[count].name)+1;		/* generate lead tag */
		fsptr += sprintf(fsptr, "<ParaStyle:%s>", fxp->stylenames[count]) + 1;		/* generate lead tag */
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
//	fxp->esptr += sprintf(fxp->esptr,"<DefineParaStyle:Ahead=<pHyphenationLadderLimit:0><pLeftIndent:27.0><pFirstLineIndent:-27.0><pHyphenation:0><pHyphenationZone:0.0><pSpaceBefore:%.4f><cFont:%s><cSize:%d><pKeepWithNext:1>>%s",
//		 spacebefore,FF->head.formpars.ef.eg.gfont,fontsize,fxp->newlinestring);
	fxp->esptr += sprintf(fxp->esptr, "<DefineParaStyle:%s=<pHyphenationLadderLimit:0><pLeftIndent:27.0><pFirstLineIndent:-27.0><pHyphenation:0><pHyphenationZone:0.0><pSpaceBefore:%.4f><cFont:%s><cSize:%d><pKeepWithNext:1>>%s",
		fxp->stylenames[FIELDLIM - 1], spacebefore, FF->head.formpars.ef.eg.gfont, fontsize, fxp->newlinestring);
	structtags[STR_GROUP] = g_nullstr;	// group start tag
	structtags[STR_GROUPEND] = g_nullstr;	// group end tag
	structtags[STR_AHEAD] = fsptr;		/* set ahead tag */
//	fsptr += sprintf(fsptr, "<ParaStyle:Ahead>");	/* group header tag (style 1) */
	sprintf(fsptr, "<ParaStyle:%s>", fxp->stylenames[FIELDLIM - 1]);	/* group header tag (style 1) */
	structtags[STR_AHEADEND] = g_nullstr;	/* set ahead end tag */
	return (TRUE);
}
/**********************************************************************/
static void indesigncleanup(INDEX * FF, FCONTROLX * fxp)	/* cleans up */

{
	*fxp->esptr = '\0';		/* end of file */
}
/**********************************************************************/
static void indesignwriter(FCONTROLX * fxp,unichar uc)	// emits unichar

{
	fxp->esptr += sprintf(fxp->esptr, "<0x%04x>",uc);
}
