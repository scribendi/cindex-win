#include "stdafx.h"
#include "locale.h"
#include "errors.h"
#include "viewset.h"
#include "strings.h"
#include "sort.h"
#include "collate.h"
#include "locales.h"
#include "util.h"
#include "formstuff.h"
#include "formattedexport.h"
#include "tagstuff.h"
#include "rtfwriter.h"

static char * stylespec = 
"{\\stylesheet{\\sbasedon222 Normal;}";
	
static char *family[] = {
	"fnil",
	"froman",
	"fswiss",
	"fmodern",
	"fscript",
	"fdecor",
};
#define MAXFAMILIES (sizeof(family)/sizeof(char *))

static char parabase[32];	/* assembled string for new paragraph */
static char * structtags[T_STRUCTCOUNT];
#if 0
static char * styletags[] = {	/* braced tags that can get us in trouble */
	"{\\b ",
	"}",
	"{\\i ",
	"}",
	"{\\ul ",
	"}",
	"{\\scaps ",
	"}",
	"{\\super ",
	"}",
	"{\\sub ",
	"}",
	g_nullstr,
	g_nullstr
};
#else
static char * styletags[] = {
	"\\b ",
	"\\b0 ",
	"\\i ",
	"\\i0 ",
	"\\ul ",
	"\\ul0 ",
	"\\scaps ",
	"\\scaps0 ",
	"{\\super ",
	"}",
	"{\\sub ",
	"}",
	g_nullstr,
	g_nullstr
};
#endif
static char * fonttags[T_FONTCOUNT];	/* pointers to strings that define font codes */
static char * auxtags[T_OTHERCOUNT];	// pointers to auxiliary tags
static char protchars[MAXTSTRINGS] = "\\{}";	/* characters needing protection */
static char embedprotchars[MAXTSTRINGS] = "\\{};";	/* characters needing protection */

static BOOL rtfinit(INDEX * FF, FCONTROLX * fxp);	/* initializes control struct and emits header */
static void rtfcleanup(INDEX * FF, FCONTROLX * fxp);	/* cleans up */
static void rtfembedder(INDEX * FF, FCONTROLX * fxp, RECORD * recptr);	/* forms embedded entry */
static void rtfwriter(FCONTROLX * fxp, unichar uc);	//

FCONTROLX rtfcontrol = {
	0,				// file type
//	NULL,			/* file pointer */
	NULL,			/* string pointer */
	rtfinit,		/* initialize */
	rtfcleanup,		/* cleanup */
	rtfembedder,	/* embedding fn */
	rtfwriter,		// character writer
	structtags,		/* structure tags */
	styletags,		/* styles */
	fonttags,		/* fonts */
	auxtags,		// auxiliary tags
	"\\line ",		/* line break */
	parabase,		/* new para */
	NULL,			/* obligatory newline (set at format time) */
	"\\tab ",		/* tab */
	protchars,		/* characters needing protection */
	{				/* translation strings for protected characters */
		"\\\\",
		"\\{",
		"\\}",
		"\\\\;",	/* this is used only when doing embedding */
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
static BOOL rtfinit(INDEX * FF, FCONTROLX * fxp)	/* initializes control struct and emits header */

{
	int count, rtabpos, width, fieldlimit;
	int baseindent, firstindent, fontsize;
	char * fsptr, lspace[30], spacebefore[30], *rtabtype;
	char tabstops[200];
	char tabstring[FIELDLIM];
	long lang;
	float lineheightmultiple = 1;
	TCHAR locale[10], *eptr;
	int lcount;
	
	rtfcontrol.protected = protchars;	/* standard protection set */
	width = FF->head.formpars.pf.pi.pwidthactual-(FF->head.formpars.pf.mc.right+FF->head.formpars.pf.mc.left); /* overall width less margins (points) */
	rtabpos = TWIPS_PER_POINT*(width-(FF->head.formpars.pf.mc.ncols-1)*FF->head.formpars.pf.mc.gutter)/FF->head.formpars.pf.mc.ncols; /* fix for columns */
	rtabtype = FF->head.formpars.ef.lf.leader ? "\\tldot" : g_nullstr;
	lcount = GetLocaleInfo(LOCALE_USER_DEFAULT,LOCALE_IDEFAULTLANGUAGE,locale,10);
	lang = nstrtol(locale,&eptr,16);	// value return is ascii representation of hex
	fxp->esptr += sprintf(fxp->esptr,"{\\rtf1\\ansi\\deff%d\\deflang%ld",1,lang);	/* load start stuff && set default font (assigned id 1) */
	fsptr = rtf_setfonts(FF,fxp);
	fxp->esptr += sprintf(fxp->esptr,stylespec);		/* start style definitions */
	if (FF->head.formpars.pf.linespace)
		lineheightmultiple = FF->head.formpars.pf.linespace == 1 ? 1.5 : 2;
	if (FF->head.formpars.pf.autospace) {
		if (FF->head.formpars.pf.linespace)	// if anything other than single space
			sprintf(lspace,"\\sl%d\\slmult1",(int)(TWIPS_PER_POINT*lineheightmultiple*12));	// set space explicitly
		else
			*lspace = '\0';
	}
	else {		// fixed spacing
		if (FF->head.formpars.pf.linespace)	// if anything other than single space
			sprintf(lspace,"\\sl%d\\slmult1",(int)(-FF->head.formpars.pf.lineheight*TWIPS_PER_POINT*lineheightmultiple));
		else
			sprintf(lspace,"\\sl%d\\slmult0",-FF->head.formpars.pf.lineheight*TWIPS_PER_POINT);
	}
	fieldlimit = FF->head.indexpars.maxfields < FIELDLIM ? FF->head.indexpars.maxfields : FF->head.indexpars.maxfields-1;	// need 1 extra level for subhead cref
	for (count = 0; count < fieldlimit; count++)	{	/* for all text fields in index + 1 extra level */
		fontsize = FF->head.formpars.ef.field[count].size ? FF->head.formpars.ef.field[count].size : FF->head.privpars.size;
		if (!count && FF->head.formpars.pf.entryspace)	{	// if main head and want extra space
			// use fontsize as amount of space until we can get proper extra space metrics
//			sprintf(spacebefore, "\\sb%d", (int)(FF->head.formpars.pf.entryspace*fontsize*TWIPS_PER_POINT));
			FONTMETRICS levelspacing = type_getfontmetrics(FF, FF->head.formpars.ef.field[count].font,fontsize);
			sprintf(spacebefore, "\\sb%d", (int)(FF->head.formpars.pf.entryspace*levelspacing.height*TWIPS_PER_POINT));
		}
		else
			*spacebefore = '\0';
		formexport_gettypeindents(FF,count,fxp->usetabs, TWIPS_PER_POINT,&firstindent,&baseindent,"\\tx%d",tabstops,tabstring);	/* gets base and first indents */
		fxp->esptr += sprintf(fxp->esptr,"{\\s%d%s\\tqr%s\\tx%d\\fi%d\\li%d%s%s\\fs%d\\sbasedon222 %s;}", count+2, tabstops, rtabtype, rtabpos,(short)(firstindent), (short)baseindent, lspace, spacebefore, fontsize*2, FF->head.indexpars.field[count].name);	/* emit header spec */
		structtags[count+STR_MAIN] = fsptr;				/* pointer for lead string */
		structtags[count+STR_MAINEND] = g_nullstr;		/* pointer for trailing string (none) */
		fsptr += sprintf(fsptr,"\\pard\\plain\\s%d%s\\tqr%s\\tx%d\\fi%d\\li%d%s%s\\fs%d %s", count+2, tabstops, rtabtype, rtabpos,(short)(firstindent), (short)baseindent,lspace, spacebefore, fontsize*2,tabstring)+1;		/* generate lead tag */
	}
	structtags[STR_PAGE] = g_nullstr;		/* pointer for page tag */
	structtags[STR_PAGEND] = g_nullstr;		/* pointer for page tag */
	structtags[STR_CROSS] = g_nullstr;		/* pointer for cross tag */
	structtags[STR_CROSSEND] = g_nullstr;		/* pointer for cross end tag */
	sprintf(parabase,"\\par%s",fxp->newlinestring);	/* end para string + newline */
	fontsize = FF->head.formpars.ef.eg.gsize ? FF->head.formpars.ef.eg.gsize : FF->head.privpars.size;
	if (FF->head.formpars.pf.above)	{	// if want extra space before header
		// use fontsize as amount of space until we can get proper extra space metrics
//		sprintf(spacebefore, "\\sb%d", (int)(FF->head.formpars.pf.above*fontsize*TWIPS_PER_POINT));
		FONTMETRICS headerspacing = type_getfontmetrics(FF, FF->head.formpars.ef.eg.gfont, fontsize);
		sprintf(spacebefore, "\\sb%d", (int)(FF->head.formpars.pf.above*headerspacing.height*TWIPS_PER_POINT));
	}
	else
		*spacebefore = '\0';
	structtags[STR_GROUP] = g_nullstr;	// group start tag
	structtags[STR_GROUPEND] = g_nullstr;	// group end tag
	fxp->esptr += sprintf(fxp->esptr,"{\\s1\\fi-540\\li540%s%s\\keepn\\fs%d\\sbasedon222 ahead;}", lspace, spacebefore, fontsize*2);	/* emit header spec */
	structtags[STR_AHEAD] = fsptr;		/* set ahead tag */
	fsptr += sprintf(fsptr, "\\pard\\plain\\s1%s%s\\keepn\\fs%d ", lspace, spacebefore, fontsize*2);	/* group header tag (style 1) */
	structtags[STR_AHEADEND] = g_nullstr;	/* set ahead end tag */
	*fxp->esptr++ = '}';
	fxp->esptr += sprintf(fxp->esptr,"\\margt%d\\margb%d\\margl%d\\margr%d\\widowctrl\\notabind\\ftnbj\\sectd\\cols%d\\colsx%d\\endnhere ",  
		FF->head.formpars.pf.mc.top*TWIPS_PER_POINT, FF->head.formpars.pf.mc.bottom*TWIPS_PER_POINT,
		FF->head.formpars.pf.mc.left*TWIPS_PER_POINT, FF->head.formpars.pf.mc.right*TWIPS_PER_POINT,
		FF->head.formpars.pf.mc.ncols,FF->head.formpars.pf.mc.gutter*TWIPS_PER_POINT);	/* start doc bits */
	fxp->esptr += sprintf(fxp->esptr,"{\\info{\\title %s}}", "CINDEX Index");		/* doc info */
	fxp->esptr += sprintf(fxp->esptr,"\\f1 ");		/* set default font */
	return (TRUE);
}
#else
/**********************************************************************/
static BOOL rtfinit(INDEX * FF, FCONTROLX * fxp)	/* initializes control struct and emits header */

{
	int count, fieldlimit;
	int baseindent, firstindent, fontsize;
	char * fsptr, lspace[30], spacebefore[30];
	char tabstops[200];
	char tabstring[FIELDLIM];
	char rtabstring[50];
	float lineheightmultiple = 1;
	int lcid;
	char *docdirection , *paradirection ;
	
	if (FF->righttoleftreading)	{	// if rtl
		docdirection = "\\rtldoc";
		paradirection = "\\rtlpar";
	}
	else {		// left to right is default
//		docdirection = "\\ltrdoc";
//		paradirection = "\\ltrpar";
		docdirection = paradirection = g_nullstr;
	}
	if (FF->head.formpars.ef.lf.rjust)	{	// if right flushed
		int width = FF->head.formpars.pf.pi.pwidthactual-(FF->head.formpars.pf.mc.right+FF->head.formpars.pf.mc.left); /* overall width less margins (points) */
		int rtabpos = TWIPS_PER_POINT*(width-(FF->head.formpars.pf.mc.ncols-1)*FF->head.formpars.pf.mc.gutter)/FF->head.formpars.pf.mc.ncols; /* fix for columns */
		char * rtab = FF->righttoleftreading ? "\\tqr" : "\\tqr";	// don't need this because Word reverses tab sense
		if (FF->head.formpars.ef.lf.leader)
			sprintf(rtabstring, "%s\\tldot\\tx%d",rtab,rtabpos);
		else
			sprintf(rtabstring, "%s\\tx%d",rtab,rtabpos);
	}
	else
		*rtabstring = '\0';
	rtfcontrol.protected = protchars;	/* standard protection set */
//	width = FF->head.formpars.pf.pi.pwidthactual-(FF->head.formpars.pf.mc.right+FF->head.formpars.pf.mc.left); /* overall width less margins (points) */
//	rtabpos = TWIPS_PER_POINT*(width-(FF->head.formpars.pf.mc.ncols-1)*FF->head.formpars.pf.mc.gutter)/FF->head.formpars.pf.mc.ncols; /* fix for columns */
//	rtabtype = FF->head.formpars.ef.lf.leader ? "\\tldot" : g_nullstr;
//	lcount = GetLocaleInfo(LOCALE_USER_DEFAULT,LOCALE_IDEFAULTLANGUAGE,locale,10);
//	lang = nstrtol(locale,&eptr,16);	// value return is ascii representation of hex
//	fxp->esptr += sprintf(fxp->esptr,"{\\rtf1\\ansi\\deff%d\\deflang%ld",1,lang);	/* load start stuff && set default font (assigned id 1) */
//	lcid = uloc_getLCID(col_getLocaleInfo(&FF->head.sortpars)->localeID);
	lcid = LCIDforLocale(col_getLocaleInfo(&FF->head.sortpars)->localeID);
	fxp->esptr += sprintf(fxp->esptr,"{\\rtf1\\ansi\\deff%d\\deflang%d",1,lcid);	/* load start stuff && set default font (assigned id 1) */
	fsptr = rtf_setfonts(FF,fxp);
	fxp->esptr += sprintf(fxp->esptr,stylespec);		/* start style definitions */
	if (FF->head.formpars.pf.linespace)
		lineheightmultiple = FF->head.formpars.pf.linespace == 1 ? 1.5 : 2;
	if (FF->head.formpars.pf.autospace) {
		if (FF->head.formpars.pf.linespace)	// if anything other than single space
			sprintf(lspace,"\\sl%d\\slmult1",(int)(TWIPS_PER_POINT*lineheightmultiple*12));	// set space explicitly
		else
			*lspace = '\0';
	}
	else {		// fixed spacing
		if (FF->head.formpars.pf.linespace)	// if anything other than single space
			sprintf(lspace,"\\sl%d\\slmult1",(int)(-FF->head.formpars.pf.lineheight*TWIPS_PER_POINT*lineheightmultiple));
		else
			sprintf(lspace,"\\sl%d\\slmult0",-FF->head.formpars.pf.lineheight*TWIPS_PER_POINT);
	}
	fieldlimit = FF->head.indexpars.maxfields < FIELDLIM ? FF->head.indexpars.maxfields : FF->head.indexpars.maxfields-1;	// need 1 extra level for subhead cref
	for (count = 0; count < fieldlimit; count++)	{	/* for all text fields in index + 1 extra level */
		fontsize = FF->head.formpars.ef.field[count].size ? FF->head.formpars.ef.field[count].size : FF->head.privpars.size;
		if (!count && FF->head.formpars.pf.entryspace)	{	// if main head and want extra space
//			use fontsize as amount of space until we can get proper extra space metrics
//			sprintf(spacebefore, "\\sb%d", (int)(FF->head.formpars.pf.entryspace*fontsize*TWIPS_PER_POINT));
			FONTMETRICS levelspacing = type_getfontmetrics(FF, FF->head.formpars.ef.field[count].font,fontsize);
			sprintf(spacebefore, "\\sb%d", (int)(FF->head.formpars.pf.entryspace*levelspacing.height*TWIPS_PER_POINT));
		}
		else
			*spacebefore = '\0';
		// style number for heading is count+3 (style 0: normal; style 1: group header; style 2: alpha header)
		formexport_gettypeindents(FF,count,fxp->usetabs, TWIPS_PER_POINT,&firstindent,&baseindent,"\\tx%d",tabstops,tabstring);	/* gets base and first indents */
//		fxp->esptr += sprintf(fxp->esptr,"{\\s%d%s%s%s\\fi%d\\lin%d%s%s\\fs%d\\sbasedon222 %s;}", count+3, paradirection, tabstops, rtabstring,(short)(firstindent), (short)baseindent, lspace, spacebefore, fontsize*2, FF->head.indexpars.field[count].name);	/* emit header spec */
		fxp->esptr += sprintf(fxp->esptr, "{\\s%d%s%s%s\\fi%d\\li%d%s%s\\fs%d\\sbasedon222 %s;}", count+3, paradirection, tabstops, rtabstring, (short)(firstindent), (short)baseindent, lspace, spacebefore, fontsize * 2, fxp->stylenames[count]);	/* emit header spec */
		structtags[count+STR_MAIN] = fsptr;				/* pointer for lead string */
		structtags[count+STR_MAINEND] = g_nullstr;		/* pointer for trailing string (none) */
		fsptr += sprintf(fsptr,"\\pard\\plain\\s%d%s%s%s\\fi%d\\li%d%s%s\\fs%d %s", count+3, paradirection, tabstops, rtabstring,(short)(firstindent), (short)baseindent,lspace, spacebefore, fontsize*2,tabstring)+1;		/* generate lead tag */
	}
	structtags[STR_PAGE] = g_nullstr;		/* pointer for page tag */
	structtags[STR_PAGEND] = g_nullstr;		/* pointer for page tag */
	structtags[STR_CROSS] = g_nullstr;		/* pointer for cross tag */
	structtags[STR_CROSSEND] = g_nullstr;		/* pointer for cross end tag */
	auxtags[OT_STARTTEXT] = g_nullstr;		// body text start
	auxtags[OT_ENDTEXT] = g_nullstr;		// body text end
	sprintf(parabase,"\\par%s",fxp->newlinestring);	/* end para string + newline */
	fontsize = FF->head.formpars.ef.eg.gsize ? FF->head.formpars.ef.eg.gsize : FF->head.privpars.size;
	if (FF->head.formpars.pf.above)	{	// if want extra space before header
//		use fontsize as amount of space until we can get proper extra space metrics
//		sprintf(spacebefore, "\\sb%d", (int)(FF->head.formpars.pf.above*fontsize*TWIPS_PER_POINT));
		FONTMETRICS headerspacing = type_getfontmetrics(FF, FF->head.formpars.ef.eg.gfont, fontsize);
		sprintf(lspace, "\\sl%d", (int)(FF->head.formpars.pf.above*headerspacing.height*TWIPS_PER_POINT));
		fxp->esptr += sprintf(fxp->esptr,"{\\s1%s%s%s\\fs%d\\sbasedon222 group;}", paradirection, lspace, "", fontsize*2);	/* emit group spec */
		structtags[STR_GROUP] = fsptr;	// group start tag
		fsptr += sprintf(fsptr, "\\pard\\plain\\s1%s%s%s\\fs%d ", paradirection, lspace, "", fontsize*2)+1;	/* group tag (style 1) */

	}
	else
		structtags[STR_GROUP] = g_nullstr;	// group start tag
	structtags[STR_GROUPEND] = g_nullstr;	// group end tag
//	fxp->esptr += sprintf(fxp->esptr,"{\\s2%s\\fi-540\\lin540%s%s\\keepn\\fs%d\\sbasedon222 ahead;}", paradirection, lspace, "", fontsize*2);	/* emit header spec */
	fxp->esptr += sprintf(fxp->esptr, "{\\s2%s\\fi-540\\lin540%s%s\\keepn\\fs%d\\sbasedon222 %s;}", paradirection, lspace, "", fontsize * 2, fxp->stylenames[FIELDLIM - 1]);	/* emit header spec */
	structtags[STR_AHEAD] = fsptr;		/* set ahead tag */
	fsptr += sprintf(fsptr, "\\pard\\plain\\s2%s%s%s\\keepn\\fs%d ", paradirection, lspace, "", fontsize*2);	/* group header tag (style 2) */
	structtags[STR_AHEADEND] = g_nullstr;	/* set ahead end tag */
	*fxp->esptr++ = '}';
	fxp->esptr += sprintf(fxp->esptr,docdirection);
	fxp->esptr += sprintf(fxp->esptr,"\\margt%d\\margb%d\\margl%d\\margr%d\\widowctrl\\notabind\\ftnbj\\sectd\\cols%d\\colsx%d\\endnhere ",  
		FF->head.formpars.pf.mc.top*TWIPS_PER_POINT, FF->head.formpars.pf.mc.bottom*TWIPS_PER_POINT,
		FF->head.formpars.pf.mc.left*TWIPS_PER_POINT, FF->head.formpars.pf.mc.right*TWIPS_PER_POINT,
		FF->head.formpars.pf.mc.ncols,FF->head.formpars.pf.mc.gutter*TWIPS_PER_POINT);	/* start doc bits */
	fxp->esptr += sprintf(fxp->esptr,"{\\info{\\title %s}}", "CINDEX Index");		/* doc info */
	fxp->esptr += sprintf(fxp->esptr,"\\f1 ");		/* set default font */
	return (TRUE);
}
#endif
/**********************************************************************/
static void rtfcleanup(INDEX * FF, FCONTROLX * fxp)	/* cleans up */

{
	*fxp->esptr++ = '}';	/* end of file */
	*fxp->esptr = '\0';
}
#if 0
/**********************************************************************/
//	strcpy(e_control->esptr, "{\\rtf1\\ansi{\\plain \\v{\\XE {main heading\\:}{\\b subheading}{\\:subsub}}}}");
//{\xe {cross head\:cross sub}{\txe {\i See}{ junk here}}}}
//{\xe {here is\:bold\bxe }}
static void rtfembedder(INDEX * FF, FCONTROLX * fxp, RECORD * recptr)	/* forms embedded entry */

{
	CSTR scur[FIELDLIM];
	int curcount, fcount;
	char tfield[MAXREC];

	rtfcontrol.protected = embedprotchars;	/* protected set for embedding */
	curcount = str_xparse(recptr->rtext, scur);	/* parse string */
	strcpy(fxp->esptr,"{\\plain \\v{\\xe ");	/* entry lead */
	fxp->esptr += strlen(fxp->esptr);
	for (fcount = 0; fcount < curcount; fcount++)	{	/* for all fields */
		if (fcount == curcount-1)	{	/* if page field */
			if (!str_crosscheck(FF,scur[fcount].str))	{	/* if page ref */
				if (strchr(scur[fcount].str,FF->head.refpars.rsep))	{		/* if page field contains range */
					strcpy(fxp->esptr,"{\\rxe $$$}");
					fxp->esptr += strlen(fxp->esptr);
				}
				*fxp->esptr = '\0';			/* assume no ref styles */
				if (FF->head.formpars.ef.lf.lstyle[0].loc.style&FX_BOLD)
					strcat(fxp->esptr,"\\bxe ");
				if (FF->head.formpars.ef.lf.lstyle[0].loc.style&FX_ITAL)
					strcat(fxp->esptr,"\\ixe ");
				fxp->esptr += strlen(fxp->esptr);
			}
			else {
				strcpy(fxp->esptr,"{\\txe ");
				fxp->esptr += strlen(fxp->esptr);
				formexport_makefield(FF,form_formatcross(FF,scur[fcount].str));
				*fxp->esptr++ = '}';
			}
		}
		else {
			*fxp->esptr++ = '{';	/* open a field */
			if (fcount)	{	/* if a subhead */
				strcpy(fxp->esptr,"\\:");
				fxp->esptr += strlen(fxp->esptr);
			}
			formexport_makefield(FF,scur[fcount].str);
			*fxp->esptr++ = ';';		/* prefix for sort key */
			sort_buildkey(FF,tfield,scur[fcount].str);	/* build sort key */
			formexport_makefield(FF,tfield);	/* translate and add it */
			*fxp->esptr++ = '}';
		}
	}
	strcpy(fxp->esptr,"}}");
	fxp->esptr += 2;
}
#else
/**********************************************************************/
//	strcpy(e_control->esptr, "{\\rtf1\\ansi{\\plain \\v{\\XE {main heading\\:}{\\b subheading}{\\:subsub}}}}");
//{\xe {cross head\:cross sub}{\txe {\i See}{ junk here}}}}
//{\xe {here is\:bold\bxe }}
static void rtfembedder(INDEX * FF, FCONTROLX * fxp, RECORD * recptr)	/* forms embedded entry */

{
	CSTR scur[FIELDLIM];
	int curcount, fcount;
	char tfield[MAXREC];

	rtfcontrol.protected = embedprotchars;	/* protected set for embedding */
	curcount = str_xparse(recptr->rtext, scur);	/* parse string */
	strcpy(fxp->esptr,"{\\plain \\v{\\xe ");	/* entry lead */
	fxp->esptr += strlen(fxp->esptr);
	for (fcount = 0; fcount < curcount; fcount++)	{	/* for all fields */
		if (fcount == curcount-1)	{	/* if page ref */
			if (!str_crosscheck(FF,scur[fcount].str))	{	/* if page ref */
				if (strchr(scur[fcount].str,FF->head.refpars.rsep))	{		/* if page field contains range */
					strcpy(fxp->esptr,"{\\rxe $$$}");
					fxp->esptr += strlen(fxp->esptr);
				}
				*fxp->esptr = '\0';			/* assume no ref styles */
				if (FF->head.formpars.ef.lf.lstyle[0].loc.style&FX_BOLD)
					strcat(fxp->esptr,"\\bxe ");
				if (FF->head.formpars.ef.lf.lstyle[0].loc.style&FX_ITAL)
					strcat(fxp->esptr,"\\ixe ");
				fxp->esptr += strlen(fxp->esptr);
			}
			else {
				strcpy(fxp->esptr,"{\\txe ");
				fxp->esptr += strlen(fxp->esptr);
				formexport_makefield(fxp,FF,form_formatcross(FF,scur[fcount].str));
				*fxp->esptr++ = '}';
			}
		}
		else {
			char * fieldbase = scur[fcount].str;
			
			*fxp->esptr++ = '{';	/* open a field */
			if (fcount)	{	/* if a subhead */
				strcpy(fxp->esptr,"\\:");
				fxp->esptr += strlen(fxp->esptr);
			}
			formexport_makefield(fxp,FF,fieldbase);
			if (g_prefs.gen.embedsort)	{	// if want to add sort info
				if (fcount) {	// if a subhead field
					short tokens;
					fieldbase = str_skiplist(fieldbase,FF->head.sortpars.ignore, &tokens);     /* skip words to be ignored */
				}
				*fxp->esptr++ = ';';		/* prefix for sort key */
				col_buildkey(FF,tfield,fieldbase);	/* build sort key */
				formexport_makefield(fxp,FF,tfield);	/* translate and add it */
			}
			*fxp->esptr++ = '}';
		}
	}
	strcpy(fxp->esptr,"}}");
	fxp->esptr += 2;
}
#endif
/**********************************************************************/
static void rtfwriter(FCONTROLX * fxp,unichar uc)	// emits unichar

{
//	fxp->esptr += sprintf(fxp->esptr, "\\uc0\\u%d ",uc);
	fxp->esptr += sprintf(fxp->esptr, "\\u%d?", (short)uc);	// need signed decimal
}
/**********************************************************************/
char * rtf_setfonts(INDEX * FF, FCONTROLX * fxp)	/* sets up font table */

{
	int count, index, familyindex;
	char * fsptr;

	fxp->esptr += sprintf(fxp->esptr,"{\\fonttbl");	/* lead to font table */
	/* in what follows we use the local index+1 (count+1) as the font id */
	for (fsptr = f_stringbase, count = 0; *FF->head.fm[count].name && count < T_NUMFONTS; count++)	{	/* for all fonts we use */
		if ((index = type_findindex(FF->head.fm[count].pname) < 0))	/* if can't get preferred font */
			index = type_findindex(FF->head.fm[count].name);		/* use substitute */
		familyindex = t_fontlist.lf[index].elfLogFont.lfPitchAndFamily>>4;	/* get family specifier */
		if (familyindex >= MAXFAMILIES)
			familyindex = 0;
		fxp->esptr += sprintf(fxp->esptr, "{\\f%d\\%s\\fcharset%d\\fprq%d %s;}", count+1, family[familyindex],
			t_fontlist.lf[index].elfLogFont.lfCharSet,t_fontlist.lf[index].elfLogFont.lfPitchAndFamily&0xF, FF->head.fm[count].pname);	/* create string */
		fonttags[count*2] = fsptr;		/* set pointer to it */
		if (!count)	{		/* if default font */
			*fsptr++ = '\0';	/* we never encode explicitly */
			fonttags[count*2+1] = fsptr;		/* set pointer to it */
			*fsptr++ = '\0';
		}
		else {
			fsptr += sprintf(fsptr,"{\\f%d ", count+1)+1;	/* add string for turning font on */
			fonttags[count*2+1] = fsptr;		/* set pointer to it */
			fsptr += sprintf(fsptr, "}")+1;		/* and string for turning off */
		}
	}
	*fxp->esptr++ = '}';			/* end of font table */
	return fsptr;
}