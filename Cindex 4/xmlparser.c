//
//  xmlparser.m
//  Cindex
//
//  Created by Peter Lennie on 2/2/11.
//

#include "stdafx.h"
#include "xmlparser.h"
#include "recordparams.h"
#include "strings.h"
#include "records.h"
#include "util.h"
#include "typestuff.h"
#include "toolset.h"
#include "sort.h"
#include "import.h"
#include "files.h"
#include "index.h"

enum {		// xml content errors
	XML_BADELEMENT = 1,
	XML_BADFONTS,
	XML_BADFONT,
	XML_BADDATE,
	XML_BADRECORDS,
	XML_INVALIDSEPARATOR,
	XML_UNKNOWN
};

char * errorstrings[] = {
	": unknown element.",
	": invalid or missing fonts specification.",
	": record uses an unknown font.",
	": record has invalid time stamp.",
	": file contains invalid records.",
	": page or cross-reference separator is invalid.",
	"."
};

static XML_Parser getparser(INDEX * FF, IMPORTPARAMS * imp);
static void startElement(void *userData, const XML_Char *name,const XML_Char **atts);		// starts element
static void endElement(void *userData, const XML_Char *name);		// ends element
static void characterData(void *userData, const XML_Char *s ,int len);		// collects character data
static void sendparsererror(PARSERDATA * pd, int error);	// aborts parser with error
static char * getavalue(const char ** atts, char * attr);	// gets value for attribute
static void startfont(IMPORTPARAMS * imp, char * destination);	// starts font
BOOL setupfonts(IMPORTPARAMS *imp);	// sets up font conversion ids

/***************************************************************************/
int xml_parserecords(INDEX * FF, IMPORTPARAMS * imp, char * datastring, int length, char ** errmessage)		// parses data string

{
	XML_Parser parser= getparser(FF,imp);
	int err = 1;
	static char errstring[200];

	if (parser)	{
		PARSERDATA * pd = &imp->pdata;
		
		imp->FF = FF;
		imp->mode = PM_SCAN;
		if (!XML_Parse(parser,datastring,length, TRUE) || pd->error)	{	// if some error
			char * sptr = errstring;
			sptr += sprintf(sptr,"Parsing stopped at line %ld", pd->error ? pd->errorline : XML_GetCurrentLineNumber(parser));
			if (pd->error) {
				if (pd->activerecord)
					sprintf(sptr, " %s [Record has id %d]", errorstrings[pd->error - 1], pd->activerecord);
				else
					sprintf(sptr, " %s", errorstrings[pd->error - 1]);
			}
			else	{
//				int error = XML_GetErrorCode(parser);
//				NSLog(@"%s\nLine: %d; Column: %d",XML_ErrorString(error),XML_GetCurrentLineNumber(parser),XML_GetCurrentColumnNumber(parser));
				sptr += sprintf(sptr, ": invalid XML.");
			}
			*errmessage = errstring;
			return ERR_BADXML;
		}
		XML_ParserFree(parser);
		err = imp_resolveerrors(FF, imp);
		if (err > 0) {	// no XML errors
			if (index_setworkingsize(FF,imp->recordcount+MAPMARGIN))	{	// if can resize index
				parser = getparser(FF,imp);
				imp->mode = PM_READ;
				if (!XML_Parse(parser,datastring,length, TRUE))	{
					int error = XML_GetErrorCode(parser);
					NSLog("%s\nLine: %ld; Column: %ld",XML_ErrorString(error),XML_GetCurrentLineNumber(parser),XML_GetCurrentColumnNumber(parser));
				}
				XML_ParserFree(parser);
			}
		}
	}
	return (err);
}
/***************************************************************************/
static XML_Parser getparser(INDEX * FF, IMPORTPARAMS * imp)

{
	XML_Parser parser;
	
	if ((parser = XML_ParserCreate("UTF-8")))	{
		PARSERDATA * pd = &imp->pdata;
		
		XML_SetElementHandler(parser,startElement, endElement);
		XML_SetCharacterDataHandler(parser,characterData);
		XML_SetUserData(parser,imp);
		memset(pd,0,sizeof(PARSERDATA));
		pd->parser = parser;
		return parser;
	}
	return NULL;
}
/***************************************************************************/
static void startElement(void *userData, const XML_Char *name,const XML_Char **atts)		// starts element

{
	IMPORTPARAMS * imp = (IMPORTPARAMS *)userData;
	PARSERDATA * pd = &imp->pdata;
	
//	NSLog(@"Starting:%s",name);
	if (!strcmp(name,"indexdata"))	{
		pd->inindex = TRUE;
	}
	else if (!strcmp(name,"source"))	{
//		NSLog(@"%s | %s", getavalue(atts, "creator"),getavalue(atts, "time"));
	}
	else if (!strcmp(name,"fonts"))	{
		pd->infonts = TRUE;
	}
	else if (!strcmp(name,"font"))	{
		char * id = getavalue(atts, "id");
		if (id)	{
			int fid = atol(id);
			if (fid >= 0 && fid < FONTLIMIT)
				pd->activefont = fid;
			else
				pd->activefont = -1;	// prevent read
		}
	}
	else if (!strcmp(name,"fname"))	{
		if (imp->mode == PM_SCAN)
			startfont(imp,imp->tfm[pd->activefont].pname);
	}
	else if (!strcmp(name,"aname"))	{
		if (imp->mode == PM_SCAN)
			startfont(imp,imp->tfm[pd->activefont].name);
	}
	else if (!strcmp(name,"records"))	{
		INDEX * FF = imp->FF;
		char * att, latt = '\0', xatt = '\0', lcatt = '\0';
		BOOL seperror = FALSE;
		
		if (!pd->fontsOK)	// if haven't set up fonts
			sendparsererror(pd,XML_BADFONTS);
		att = getavalue(atts, "type");
		if (att)	{
			int mode = atol(att);
			
			if (mode == 1)
				imp->xflags = TRUE;
		}
		att = getavalue(atts, "loc-separator");
		if (att)	{
			if (strlen(att) == 1)
				latt = *att;
			else
				seperror = TRUE;
		}
		att = getavalue(atts, "loc-connector");
		if (att)	{
			if (strlen(att) == 1)
				lcatt = *att;
			else
				seperror = TRUE;
		}
		att = getavalue(atts, "xref-separator");
		if (att)	{
			if (strlen(att) == 1)
				xatt = *att;
			else
				seperror = TRUE;
		}
		if (seperror)
			sendparsererror(pd,XML_INVALIDSEPARATOR);
		else if (latt || lcatt || xatt)	{	// if have any valid separator/connector
			if (imp->mode == PM_SCAN)
				imp->conflictingseparators = (FF->head.refpars.psep != latt || FF->head.refpars.rsep != lcatt || FF->head.refpars.csep != xatt);
			else if (FF->head.rtot == 0) {
				if (latt)
					FF->head.refpars.psep = latt;
				if (lcatt)
					FF->head.refpars.rsep = lcatt;
				if (xatt)
					FF->head.refpars.csep = xatt;
			}
		}
		pd->inrecords = TRUE;
	}
	else if (!strcmp(name,"record"))	{	// start record
		char * att;
		
		if (!pd->inrecords)	// if not in records
			sendparsererror(pd,XML_BADRECORDS);
		memset(&imp->prec,0,sizeof(RECORD));	// clear record
		att = getavalue(atts, "id");
		if (att)		// if have record id
			pd->activerecord = atoi(att);
		att = getavalue(atts, "time");
		if (att)	{
			struct tm rt;
			int scount = sscanf(att,"%d-%d-%dT%d:%d:%d", &rt.tm_year,&rt.tm_mon,&rt.tm_mday,&rt.tm_hour,&rt.tm_min,&rt.tm_sec);
			if (scount == 6)	{
				rt.tm_year -= 1900;
				rt.tm_mon -= 1;
				imp->prec.time = timegm(&rt);
			}
			else
				sendparsererror(pd,XML_BADDATE);
		}
		att = getavalue(atts, "user");
		if (att)
			strncpy(imp->prec.user,att,4);
		att = getavalue(atts, "label");
		if (att)	{
			int label = atol(att);
			if (label >= 0 && label <= 7)
				imp->prec.label = label;
		}
		att = getavalue(atts, "deleted");
		if (att)
			imp->prec.isdel = !strcmp(att,"y");
		att = getavalue(atts, "type");
		if (att)
			imp->prec.isgen = !strcmp(att,"generated");
		pd->activefield = 0;
		pd->destination = imp->buffer;
		pd->limit = pd->destination+MAXREC-1;
		pd->overflow = FALSE;
	}
	else if (!strcmp(name,"field"))	{
		pd->collect = TRUE;		// read is enabled
	}
	else if (!strcmp(name,"text"))	{
		int textfont = 0, textcolor = 0, textcode = 0;
		char * att;
		
		att = getavalue(atts, "font");
		if (att)	{		// if starting font
			textfont = atol(att);
			if (!*imp->tfm[textfont].pname)	// if we've not got this font
				sendparsererror(pd,XML_BADFONT);
		}
		att = getavalue(atts, "color");
		if (att)
			textcolor = atol(att);
		att = getavalue(atts, "smallcaps");
		if (att)	{
			if (!strcmp(att, "y"))
				textcode |= FX_SMALL;
		}
		att = getavalue(atts, "style");
		if (att)	{
			if (strchr(att,'b'))
				textcode |= FX_BOLD;
			if (strchr(att,'i'))
				textcode |= FX_ITAL;
			if (strchr(att,'u'))
				textcode |= FX_ULINE;
			if (strchr(att,'s'))
				textcode |= FX_SMALL;
#if 0
			if (strchr(att,'u'))
				textcode |= FX_SUPER;
			if (strchr(att,'d'))
				textcode |= FX_SUB;
			*destination++ = CODECHR;
			*pd->destination++ = textcode;
			*pd->destination = '\0';
#endif
		}
		att = getavalue(atts, "offset");
		if (att)	{
			if (!strcmp(att,"u"))
				textcode |= FX_SUPER;
			if (!strcmp(att,"d"))
				textcode |= FX_SUB;
		}
		if (pd->textfont != textfont)	{	// if some font change
			*pd->destination++ = FONTCHR;
			*pd->destination++ = textfont|FX_FONT;
			*pd->destination = '\0';
			pd->textfont = textfont;
		}
		if (pd->textcolor != textcolor)	{	// if some color change
			*pd->destination++ = FONTCHR;
			*pd->destination++ = textcolor|FX_COLOR;
			*pd->destination = '\0';
			pd->textcolor = textcolor;
		}
		if (pd->textcode != textcode)	{	// if some style change
			int newon = textcode & ~pd->textcode;
			int newoff = pd->textcode & ~textcode;

			if (newoff)	{						// removing;
				*pd->destination++ = CODECHR;
				*pd->destination++ = newoff|FX_OFF;
			}
			if (newon)		{	// if adding code
				*pd->destination++ = CODECHR;
				*pd->destination++ = newon;
			}
			*pd->destination = '\0';
			pd->textcode = textcode;
		}
	}
	else if (!strcmp(name, "esc"))	 {
		pd->protectedchar = TRUE;
		*pd->destination++ = ESCCHR;
		*pd->destination = '\0';
	}
	else if (!strcmp(name, "literal"))	 {
		pd->protectedchar = TRUE;
		*pd->destination++ = KEEPCHR;
		*pd->destination = '\0';
	}
	else if (!strcmp(name, "hide"))	 {
		*pd->destination++ = '<';
		*pd->destination = '\0';
	}
	else if (!strcmp(name, "sort"))	 {
		*pd->destination++ = '{';
		*pd->destination = '\0';
	}
	else {	// unknown element; abort parser
		if (pd->inindex)	// if we're within the index
			sendparsererror(pd,XML_BADELEMENT);
	}
}
/***************************************************************************/
static void endElement(void *userData, const XML_Char *name)		// ends element

{
	IMPORTPARAMS * imp = (IMPORTPARAMS *)userData;
	PARSERDATA * pd = &imp->pdata;
	
//	NSLog(@"Ending:%s",name);
	if (!strcmp(name,"indexdata"))
		pd->inindex = FALSE;
	else if (!strcmp(name,"fonts"))	{
		pd->infonts = FALSE;
		pd->fontsOK = setupfonts(imp);
	}
	else if (!strcmp(name,"font"))	{
		;
	}
	else if (!strcmp(name,"fname") || !strcmp(name,"aname"))	{
		pd->destination = NULL;
		pd->collect = FALSE;		// read is disabled
	}
	else if (!strcmp(name,"records"))	{
		pd->inrecords = FALSE;
		pd->activerecord = 0;
	}
	else if (!strcmp(name,"record"))	{
		int line = XML_GetCurrentLineNumber(pd->parser);
		INDEX * FF = imp->FF;
		int reclen, fcount;
		
		if (pd->overflow)
			imp_adderror(imp,TOOLONGFORINDEX,line);
		else {
			*pd->destination = EOCS;
			u8_normalize(imp->buffer,str_xlen(imp->buffer)+1);
			fcount = rec_strip(FF, imp->buffer);	/* strip surplus fields */
			if (fcount < FF->head.indexpars.minfields)		/* if too few fields */
				rec_pad(FF,imp->buffer);	/* pad the record */
			if (fcount > imp->deepest)	{
				imp->deepest = fcount;
				if (fcount > FF->head.indexpars.maxfields) 	{	/* if too many fields */
					imp->fielderrcnt++;				/* add to tally of field # errors */
					imp_adderror(imp,TOOMANYFIELDS,line);
				}
			}
			reclen = str_adjustcodes(imp->buffer,CC_TRIM|(g_prefs.gen.remspaces ? CC_ONESPACE : 0))+1;
			if (reclen > imp->longest)	{
				imp->longest = reclen;
				if (reclen > FF->head.indexpars.recsize)	{	/* if record too long */
					imp->lenerrcnt++;				/* add to tally of length errors */
					imp_adderror(imp,TOOLONGFORRECORD,line);
				}
			}
			if (type_setfontids(imp->buffer,imp->farray)) {		// if called fonts available
				imp->fonterrcnt++;		// count font error
				imp_adderror(imp,MISSINGFONT,line);
			}
			imp->recordcount++;
			pd->destination = NULL;
			if (imp->mode == PM_READ) {
				RECORD * recptr = rec_makenew(FF,imp->buffer,FF->head.rtot+1);
				if (recptr) { 	// if can get new record
//					if (recptr->ismark = imp->prec.ismark)	/* always flag a bad translation */
//						imp->markcount++;
					recptr->isdel = imp->prec.isdel;
					recptr->label = imp->prec.label;
					recptr->isgen = imp->prec.isgen;
					recptr->time = imp->prec.time;
					strncpy(recptr->user,imp->prec.user,4);
					sort_makenode(FF,++FF->head.rtot);		/* make nodes */
				}
				else
					sendparsererror(pd,XML_UNKNOWN);
			}
		}
	}
	else if (!strcmp(name,"field"))	{	// end field
		// close any unclosed attributes
		if (pd->textfont)	{	// if some font active
			*pd->destination++ = FONTCHR;
			*pd->destination++ = FX_FONT;
			*pd->destination = '\0';
			pd->textfont = 0;
		}
		if (pd->textcolor)	{	// if some color active
			*pd->destination++ = FONTCHR;
			*pd->destination++ = FX_COLOR;
			*pd->destination = '\0';
			pd->textcolor = 0;
		}
		if (pd->textcode)	{	// if some style active
			*pd->destination++ = CODECHR;
			*pd->destination++ = pd->textcode|FX_OFF;
			*pd->destination = '\0';
			pd->textcode = 0;
		}
		pd->destination++;		// point to base of next field
		*pd->destination = '\0';	// next field starts empty
		pd->activefield++;	// increment index
		pd->collect = FALSE;	// read is disabled
	}
	else if (!strcmp(name, "hide"))	 {
		*pd->destination++ = '>';
		*pd->destination = '\0';
	}
	else if (!strcmp(name, "sort"))	 {
		*pd->destination++ = '}';
		*pd->destination = '\0';
	}
}
#if 0
/***************************************************************************/
static void characterData(void *userData, const XML_Char *s ,int len)		// collects character data

{
	IMPORTPARAMS * imp = (IMPORTPARAMS *)userData;
	PARSERDATA * pd = &imp->pdata;
	
	if (pd->collect)	{
		if (pd->destination && pd->destination+len < pd->limit)	{
			strncpy(pd->destination, s, len);
			pd->destination += len;
			*pd->destination = '\0';
		}
		else
			pd->overflow = TRUE;
	}
}
#else
/***************************************************************************/
static void characterData(void *userData, const XML_Char *s ,int len)		// collects character data

{
	static char * pchars = "<>{}\\~";
	IMPORTPARAMS * imp = (IMPORTPARAMS *)userData;
	PARSERDATA * pd = &imp->pdata;
	char * dptr = pd->destination;
	char * sptr = s;
	int count;	

	if (pd->collect)	{
		if (pd->protectedchar)	{	// if protected char
			if (strchr(pchars, *sptr))	{	// if it's special char
				*dptr++ = *sptr++;
				len--;
			}
			pd->protectedchar = FALSE;
		}
		for (count = 0; count < len && dptr < pd->limit-3; count++)	{
			if (strchr(pchars, *sptr))	// if special character
				*dptr++ = '\\';		// protect it
			*dptr++ = *sptr++;
		}
		if (dptr < pd->limit-1)	{
			pd->destination = dptr;
			*pd->destination = '\0';
		}
		else
			pd->overflow = TRUE;
	}
}
#endif
/***************************************************************************/
static void sendparsererror(PARSERDATA * pd, int error)	// aborts parser with error

{
	pd->error = error;
	pd->errorline = XML_GetCurrentLineNumber(pd->parser);
	XML_StopParser(pd->parser,NO);
}
/***************************************************************************/
static char * getavalue(const char ** atts, char * attr)	// gets value for attribute

{
	int aindex;

	for (aindex = 0; atts[aindex]; aindex += 2)	{	// for all attributes
		if (!strcmp(atts[aindex], attr))	// if found our attribute
			return (char *)atts[aindex+1];
	}
	return NULL;
}
/***************************************************************************/
static void startfont(IMPORTPARAMS * imp, char * destination)	// starts font

{
	PARSERDATA * pd = &imp->pdata;
	if (pd->activefont >= 0 && pd->activefont < FONTLIMIT)	{	// if in acceptable id range
		pd->destination = destination;
		pd->limit = pd->destination+FONTNAMELEN;
		pd->overflow = FALSE;	// reset overflow
		pd->collect = TRUE;		// read is enabled
	}
}
/***************************************************************************/
BOOL setupfonts(IMPORTPARAMS *imp)	// sets up font conversion ids

{
	int index;

	if (type_checkfonts(imp->tfm) ||  ts_fontsub(NULL,imp->tfm))	{	/* if fonts ok or substituted */
		FONTMAP ttfm[FONTLIMIT];	// placeholder font map for scan pass
		FONTMAP * fmp;
	
		if (imp->mode == PM_SCAN)	{	// if scanning
			fmp = ttfm;			// use placeholder map
			memset(ttfm, 0, sizeof(ttfm));
		}
		else
			fmp = imp->FF->head.fm;	// use real font map
		if (!imp->FF->head.rtot)		{	/* if index has no records */
			strcpy(fmp[0].pname,imp->tfm[0].pname);	// take default font from archive
			strcpy(fmp[0].name,imp->tfm[0].name);
		}
		for (index = VOLATILEFONTS; index < FONTLIMIT; index++)	{	/* for all possible import fonts */
			if (*imp->tfm[index].pname)	{	// if one is identified
				int lfnum = type_makelocal(fmp,imp->tfm[index].pname,imp->tfm[index].name,VOLATILEFONTS);	// make local font
			
				imp->farray[index] = lfnum;	// build translation entry
			}
		}
		return TRUE;
	}
	return FALSE;
}
