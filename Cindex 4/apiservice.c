#include "stdafx.h"
#include "records.h"
#include "strings.h"
#include "files.h"
#include "edit.h"
#include "index.h"
#include "sort.h"
#include "group.h"
#include "viewset.h"
#include "import.h"
#include "export.h"
#include "formattedexport.h"
#include "dde.h"
#include "tagstuff.h"
#include "replaceset.h"
#include "tools.h"
#include "refs.h"
#include "apiservice.h"
#include "cindexAPI.h"
#include "util.h"
#include "errors.h"
#include "commands.h"

#define CINAPI_VERSION 320
#define CINAPI_TOPICNAME "cindex_API_320"

static char * ddecommlist[] = {	/* list of recognized commands */
	CINAPI_QUIT,
	CINAPI_CREATE,
	CINAPI_OPEN,
	CINAPI_CLOSE,
	CINAPI_SAVE,
	CINAPI_SAVEBACKUP,
	CINAPI_SAVEAS,
	CINAPI_IMPORT,
	CINAPI_PRINT,
	CINAPI_SELECTALL,
	CINAPI_DELETE,
	CINAPI_COMPRESS,
	CINAPI_EXPAND,
	CINAPI_SORT,
	CINAPI_VIEW,
	CINAPI_GOTO,
	CINAPI_SETSIZE,
	CINAPI_SHOWINDEX,
	CINAPI_RECONCILE,
	CINAPI_GENERATE,
	CINAPI_ALTER,
	CINAPI_LABEL,
// commands that pass data
	CINAPI_FINDALL,
	CINAPI_REPLACEALL,
	CINAPI_ADDRECORD,
// commands that return data
	CINAPI_GETRECORDINFO,
	CINAPI_GETSORTINFO,
	CINAPI_GETFONTINFO,
	CINAPI_GETSELECTEDRECORD,
	CINAPI_GETNEXTRECORD,
	CINAPI_GETVERSIONINFO
};
#define MAXDDECOMMANDS (sizeof(ddecommlist)/sizeof(char *))

enum {
	DD_QUIT = 0,
	DD_CREATE,
	DD_OPEN,
	DD_CLOSE,
	DD_SAVE,
	DD_SAVEBACKUP,
	DD_SAVEAS,
	DD_IMPORT,
	DD_PRINT,
	DD_SELECTALL,
	DD_DELETE,
	DD_COMPRESS,
	DD_EXPAND,
	DD_SORT,
	DD_VIEW,
	DD_GOTO,
	DD_SETRECORDSIZE,
	DD_SHOWINDEX,
	DD_RECONCILE,
	DD_GENERATE,
	DD_ALTER,
	DD_LABEL,

	DD_FINDALL,
	DD_REPLACEALL,
	DD_ADDRECORD,

	DD_GETRECORDINFO,
	DD_GETSORTINFO,
	DD_GETFONTINFO,
	DD_GETSELECTEDRECORD,
	DD_GETNEXTRECORD,
	DD_GETVERSION
};

BOOL api_active;	// TRUE when api command in process
BOOL api_quitondisconnect;	// TRUE forces quit on disconnect
TCHAR * api_printfile;	// path for print output file

static BOOL command_sort(INDEX * FF, char * type);	/* changes sort */
static BOOL command_view(INDEX * FF, char * type);	/* changes view type */
static BOOL command_squeeze(INDEX * FF, int command);	/* compresses or squeezes */
static BOOL command_create(INDEX * FF, char *path, char * template);	/* opens file */
static BOOL command_open(INDEX * FF, char * path, char *type);	/* opens file */
static BOOL command_savebackup(INDEX * FF, char * path);	/* duplicates file */
static BOOL command_saveas(INDEX * FF, char * path, char *type);	/* saves as */
static BOOL command_print(INDEX * FF, char * path);	/* prints whole index in current view */
static BOOL command_goto(INDEX * FF, char * recspec);	/* goto record */
static BOOL command_setsize(INDEX * FF, char * arg);	/* sets record size */
static BOOL command_showindex(INDEX * FF, char * path);	/* shows named index */
static BOOL command_reconcile(INDEX * FF, char * level, char * joinchar);	/* reconciles */
static BOOL command_generate(INDEX * FF, char * path, char * seeonly);	/* generates crossrefs */
static BOOL command_alter(INDEX * FF, char * offset);	/* alters refs */
static BOOL command_label(INDEX * FF, char * label);	/* labels all records */

static HDDEDATA senddata_recordinfo(INDEX * FF);	/* returns record info as data */
static HDDEDATA senddata_sortinfo(INDEX * FF);	/* returns sort info as data */
static HDDEDATA senddata_fontinfo(INDEX * FF);	/* returns font info as data */
static HDDEDATA senddata_record(INDEX * FF, int type);	/* returns specified record */
static HDDEDATA senddata_versioninfo(INDEX * FF);	/* returns cindex version info */

static HDDEDATA data_dofind(INDEX * FF, CINDEXFINDREPLACE * fr, int datalength);	/* finds specified records */
static HDDEDATA data_doreplace(INDEX * FF, CINDEXFINDREPLACE * fr, int datalength);	/* replaces text in record(s) */
static HDDEDATA data_doaddrecord(INDEX * FF, char * rtext);	/* adds record */

static BOOL setupsearch(INDEX * FF, LISTGROUP * lg, CINDEXFINDREPLACE * fr, int type);

/************************************************************************/
BOOL api_docommand(TCHAR *string)	/* executes api command */

{
	char command[1024], arg1[512], arg2[512], *argp1, *argp2;
	int comindex;
	INDEX * FF;

	strcpy(command,fromNative(string));
	argp1 = strchr(command,'\t');
	*argp1++ = '\0';	/* terminate command */
	argp2 = strchr(argp1, '\t');
	*argp2++ = '\0';	/* terminate arg1 */
	strcpy(arg1, argp1);	// copy so that string can be modified
	strcpy(arg2, argp2);	// copy so that string can be modified

	for (comindex = 0; comindex < MAXDDECOMMANDS; comindex++)	{
		if (!stricmp(command,ddecommlist[comindex]))		/* if a command we know */
			break;
	}
	if (comindex == MAXDDECOMMANDS)		/* if we don't know the command */
		return (FALSE);

	FF = index_front();
	if (comindex > DD_OPEN && !FF)		/* if need open index && none open */
		return (FALSE);
	if (FF && FF->rwind)	/* if have record window open */
		SendMessage(FF->rwind,WM_CLOSE,0,0);	// close any open record window
	switch (comindex)	{
		case DD_QUIT:			/* quit */
			api_quitondisconnect = TRUE;
			return TRUE;
		case DD_OPEN:			/* open */
			return (command_open(FF,arg1,arg2));
		case DD_CLOSE:			/* close */
			SendMessage(FF->cwind,WM_CLOSE,0,0);
			return TRUE;
		case DD_CREATE:			/* create */
			return (command_create(FF,arg1,arg2));
		case DD_SAVE:			/* save */
			SendMessage(FF->vwind,WM_COMMAND,(WPARAM)(IDM_FILE_SAVE),0);
			return TRUE;
		case DD_SAVEBACKUP:		/* save */
			return (command_savebackup(FF,arg1));
		case DD_SAVEAS:			/* save as */
			return (command_saveas(FF,arg1,arg2));
		case DD_IMPORT:			/* import */
			return (command_open(FF,arg1,"import"));
		case DD_PRINT:			/* print */
			return (command_print(FF, arg1));
		case DD_SELECTALL:		/* select all */
			SendMessage(FF->vwind,WM_COMMAND,(WPARAM)(IDM_EDIT_SELECTALL),0);
			return TRUE;
		case DD_DELETE:			/* delete/restore */
			SendMessage(FF->vwind,WM_COMMAND,(WPARAM)(IDM_EDIT_DELETE),0);
			return TRUE;
		case DD_COMPRESS:		/* squeeze */
			return (command_squeeze(FF,DD_COMPRESS));
		case DD_EXPAND:		/* expand */
			return (command_squeeze(FF,DD_EXPAND));
		case DD_SORT:			/* sort */
			return (command_sort(FF,arg1));
		case DD_VIEW:		/* set view */
			return (command_view(FF,arg1));
		case DD_GOTO:		/* goto record */
			return (command_goto(FF,arg1));
		case DD_SETRECORDSIZE:		/* set record size */
			return (command_setsize(FF,arg1));
		case DD_SHOWINDEX:		/* activates specified index */
			return command_showindex(FF, arg1);
		case DD_RECONCILE:		/* reconciles */
			return command_reconcile(FF, arg1, arg2);
		case DD_GENERATE:		/* generates crossrefs */
			return command_generate(FF, arg1, arg2);
		case DD_ALTER:		/* alters refs */
			return command_alter(FF, arg1);
		case DD_LABEL:		/* alters refs */
			return command_label(FF, arg1);
		default:
			return (FALSE);
	}
}
/************************************************************************/
HDDEDATA api_senddata(TCHAR *command)	/* returns requested data */

{
	int comindex;
	INDEX * FF;

	for (comindex = 0; comindex < MAXDDECOMMANDS; comindex++)	{
		if (!nstricmp(command,toNative(ddecommlist[comindex])))		/* if a command we know */
			break;
	}
	if (comindex == MAXDDECOMMANDS)		/* if we don't know the command */
		return (FALSE);

	FF = index_front();
	if (!FF)		/* if none open */
		return (FALSE);
	if (FF->rwind)	/* if have record window open */
		SendMessage(FF->rwind,WM_CLOSE,0,0);	// close any open record window
	switch (comindex)	{
		case DD_GETRECORDINFO:			/* returns record info */
			return (senddata_recordinfo(FF));
		case DD_GETSORTINFO:			/* returns sort info */
			return (senddata_sortinfo(FF));
		case DD_GETFONTINFO:			/* returns font info */
			return (senddata_fontinfo(FF));
		case DD_GETSELECTEDRECORD:			/* returns selected record */
			return (senddata_record(FF, DD_GETSELECTEDRECORD));
		case DD_GETNEXTRECORD:			/* returns next record */
			return (senddata_record(FF, DD_GETNEXTRECORD));
		case DD_GETVERSION:			/* returns next record */
			return (senddata_versioninfo(FF));
		default:
			return ((HDDEDATA)DDE_FNOTPROCESSED);
	}
}
/************************************************************************/
HDDEDATA api_receivedata(TCHAR *command, void * data, unsigned long length)	/* receives data */

{
	int comindex;
	INDEX * FF;

	for (comindex = 0; comindex < MAXDDECOMMANDS; comindex++)	{
		if (!nstricmp(command,toNative(ddecommlist[comindex])))		/* if a command we know */
			break;
	}
	if (comindex == MAXDDECOMMANDS)		/* if we don't know the command */
		return (FALSE);

	FF = index_front();
	if (!FF)		/* if none open */
		return (FALSE);
	if (FF->rwind)	/* if have record window open */
		SendMessage(FF->rwind,WM_CLOSE,0,0);	// close any open record window
	switch (comindex)	{
		case DD_FINDALL:			/* finds records */
			return (data_dofind(FF, data, length));
		case DD_REPLACEALL:			/* replaces text */
			return (data_doreplace(FF, data, length));
		case DD_ADDRECORD:			/* adds record */
			return (data_doaddrecord(FF, data));
		default:
			return ((HDDEDATA)DDE_FNOTPROCESSED);
	}
}
/************************************************************************/
static BOOL command_sort(INDEX * FF, char * type)	/* changes sort */

{
	if (!stricmp(type, CINAPI_SORTALPHA))
		SendMessage(FF->vwind,WM_COMMAND,(WPARAM)(IDB_VIEW_SORTALPHA),0);
	else if (!stricmp(type, CINAPI_SORTPAGE))
		SendMessage(FF->vwind,WM_COMMAND,(WPARAM)(IDB_VIEW_SORTPAGE),0);
	else if (!stricmp(type, CINAPI_SORTOFF) && FF->head.sortpars.ison || !stricmp(type, CINAPI_SORTON) && !FF->head.sortpars.ison)
			/* if not already in the state we want */
		SendMessage(FF->vwind,WM_COMMAND,(WPARAM)(IDM_VIEW_SHOWSORTED),0);
	else	// bad arg
		return (FALSE);
	return TRUE;
}
/************************************************************************/
static BOOL command_view(INDEX * FF, char * type)	/* changes view type */

{
	if (!stricmp(type, CINAPI_VIEWFORMATTED))
		SendMessage(FF->vwind,WM_COMMAND,(WPARAM)(IDM_VIEW_FULLFORMAT),0);
	else if (!stricmp(type, CINAPI_VIEWDRAFT))
		SendMessage(FF->vwind,WM_COMMAND,(WPARAM)(IDM_VIEW_DRAFTFORMAT),0);
	else if (!stricmp(type, CINAPI_VIEWALL))
		SendMessage(FF->vwind,WM_COMMAND,(WPARAM)(IDM_VIEW_ALLRECORDS),0);
	else	// bad arg
		return (FALSE);
	return TRUE;
}
/************************************************************************/
static BOOL command_squeeze(INDEX * FF, int command)	/* compresses or squeezes */

{
	int tsort;

	if (FF->head.sortpars.fieldorder[0] == PAGEINDEX)	/* used on entry to control setup */
		return (FALSE);
	tsort = FF->head.sortpars.ison;	/* get sort state */
	FF->head.sortpars.ison = TRUE;	/* make sure its on if removing dup or consolidating */
	if (command == DD_COMPRESS)
		sort_squeeze(FF, SQDELDUP|SQDELDEL|SQDELEMPTY);
	else
		sort_squeeze(FF, SQSINGLE);
	FF->head.sortpars.ison = (char)tsort;
	view_allrecords(FF->vwind);
	grp_buildmenu(FF);		/* rebuild group menu (groups will be invalidated) */
	return (TRUE);
}
/************************************************************************/
static BOOL command_create(INDEX * FF, char *path, char * template)	/* opens file */

{
	if (*template)	{	/* if want new index from template */
		TCHAR tpath[MAX_PATH];

		nstrcpy(tpath,toNative(template));
		return (file_opentemplate(tpath,toNative(path)));
	}
	else
		return (file_createindex(toNative(path),&g_prefs.indexpars,NULL));
}
/************************************************************************/
static BOOL command_open(INDEX * FF, char * path, char *type)	/* opens file */

{
	IMPORTPARAMS imp;
	
	memset(&imp,0,sizeof(imp));		/* clear imp struct */
	if (!stricmp(type, CINAPI_OPENINDEX))
		return file_openindex(toNative(path),OP_VISIBLE);
	else if (!stricmp(type, CINAPI_OPENSTYLESHEET))
		return file_openstylesheet(toNative(path));
	else if (FF && !stricmp(type, CINAPI_OPENRECORDS))	{
		imp.type = file_type(toNative(path)) == FTYPE_XMLRECORDS ? I_CINXML : I_CINARCHIVE ;		/* set indicator */
		return (imp_loaddata(FF,&imp,toNative(path)));
	}
	else if (FF && !stricmp(type, "import"))	{
		imp.type = I_DOSDATA;		/* set indicator */
		return (imp_loaddata(FF,&imp,toNative(path)));
	}
	return FALSE;
}
/************************************************************************/
static BOOL command_savebackup(INDEX * FF, char * path)	/* duplicates file */

{
	strcat(path,fromNative(file_extensionfortype(FTYPE_INDEX)));
	return (file_duplicateindex(FF, toNative(path)));
}
/************************************************************************/
static BOOL command_saveas(INDEX * FF, char * path, char *type)	/* saves as */

{
	struct expfile ef;
	char tsort, olddel;
	TCHAR tpath[MAX_PATH];

	memset(&ef,0,sizeof(ef));
	ef.exp.includedeleted = !FF->head.privpars.hidedelete;
	ef.exp.sorted = FF->head.sortpars.ison;
	*tpath = '\0';
	if (!strchr(path,'\\'))	{	// if don't have full path
		GetCurrentDirectory(MAX_PATH,tpath);	// get cwd
		nstrcat(tpath,TEXT("\\"));
	}
	nstrcat(tpath,toNative(path));		// now file is on fully qualified path
	if (!stricmp(type, CINAPI_SAVEASINDEX))	{
		nstrcat(tpath,file_extensionfortype(FTYPE_INDEX));
		if (file_duplicateindex(FF,tpath)) {
			SendMessage(FF->cwind,WM_CLOSE,0,0);
			return (file_openindex(tpath,OP_VISIBLE));
		}
		return FALSE;
	}
	else if (!stricmp(type, CINAPI_SAVEASARCHIVE))	{
		ef.type = ef.exp.type = E_ARCHIVE;
		ef.exp.extendflag = TRUE;	/* always save extended info */
		ef.exp.includedeleted = FALSE;
		ef.exp.sorted = FALSE;
		nstrcat(tpath,TEXT(".arc"));
	}
	else if (!stricmp(type, CINAPI_SAVEASXMLRECORDS))	{
		ef.type = ef.exp.type = E_XMLRECORDS;
		nstrcat(tpath,TEXT(".ixml"));
	}
	else if (!stricmp(type, CINAPI_SAVEASRTF))	{
		ef.type = ef.exp.type = E_RTF;
		ef.exp.usetabs = g_prefs.gen.indentdef;
		nstrcat(tpath,TEXT(".rtf"));
	}
	else {					// assume tagged
		TAGSET * tgptr;
		if ((tgptr = ts_open(toNative(type))))	{	/* if can open tag set */
			nstrcat(tpath,TEXT("."));
			if (strstr(strlwr(type),".cxtg"))		{	// if xml
				ef.type = E_XMLTAGGED;
				nstrcat(tpath,TEXT("xml"));
			}
			else	{
				ef.type = E_TAGGED;
				nstrcat(tpath, *tgptr->extn ? toNative(tgptr->extn) :TEXT("tag"));
			}
			freemem(tgptr);
		}
		else
			return FALSE;
	}
	memset(&FF->pf,0,sizeof(PRINTFORMAT));	/* clear format info structure */
	tsort = FF->head.sortpars.ison;
	FF->head.sortpars.ison = ef.exp.sorted;
	olddel = FF->head.privpars.hidedelete;		/* save del status */
	FF->head.privpars.hidedelete = !ef.exp.includedeleted;
	ef.exp.first = rec_number(sort_top(FF));	/* default first record */
	ef.exp.last = ULONG_MAX;
	FF->head.privpars.hidedelete = olddel;
	FF->head.sortpars.ison = tsort;

	ef.FF = FF;
	ef.ofn.lpstrFile = tpath;

	if (ef.type == E_ARCHIVE || ef.type == E_XMLRECORDS)	
		return exp_rawexport(FF,&ef);
	else	/* formatted text of some sort */
		return formexport_write(FF,&ef,typesetters[ef.type]);
}
/************************************************************************/
static BOOL command_print(INDEX * FF, char * path)	/* prints whole index in current view */

{
	api_printfile = toNative(path);
	SendMessage(FF->vwind,WM_COMMAND,(WPARAM)(IDM_FILE_PRINT),0);
	api_printfile = NULL;
	return TRUE;
}
/************************************************************************/
static BOOL command_goto(INDEX * FF, char * recspec)	/* goto record */

{
	char *eptr;
	RECORD * recptr;
	RECN rnum;
	int num;

	num = strtoul(recspec,&eptr,10);
	if (num < 0)	{		/* if want top record */
		if (recptr = sort_top(FF))
			rnum = recptr->num;
		else
			rnum = 0;
	}
	else 
		rnum = com_findrecord(FF,recspec,FALSE);	/* if found record */
	if (rnum)
		view_selectrec(FF,rnum,VD_SELPOS,-1,-1);
	return (rnum ? TRUE : FALSE);
}
/************************************************************************/
static BOOL command_setsize(INDEX * FF, char * arg)	/* sets record size */

{
	char * eptr;
	unsigned long size;
	COUNTPARAMS cs;

	size = strtoul(arg,&eptr,10);
	memset(&cs,0,sizeof(cs));

	if (*eptr || !size || size >= MAXREC)
		return FALSE;	
	search_count(FF, &cs, SF_OFF);	/* do count of records to find longest */
	if (size < cs.longest)
		return FALSE;
	return (file_resizeindex(FF,(short)size));
}
/************************************************************************/
static BOOL command_showindex(INDEX * FF, char * path)	/* shows named index */

{
	INDEX * XX = index_byfspec(toNative(path));

	if (XX)	{
		SendMessage(g_hwclient,WM_MDIACTIVATE,(WPARAM)(XX->vwind),0);
		return TRUE;
	}
	return FALSE;
}
/************************************************************************/
static BOOL command_reconcile(INDEX * FF, char * level, char * joinchar)	/* reconciles */

{
	JOINPARAMS tjn;

	memset(&tjn,0,sizeof(tjn));
	tjn.firstfield = atol(level);
	if (tjn.firstfield >= 0 && strlen(joinchar) == 1)	{
		tjn.jchar = *joinchar;
		tool_join(FF,&tjn);
		view_redisplay(FF,0,VD_CUR);
		return TRUE;
	}
	return FALSE;
}
/************************************************************************/
static BOOL command_generate(INDEX * FF, char * path, char * seeonly)	/* generates crossrefs */

{
	INDEX * XF;
	AUTOGENERATE ag;

	memset(&ag,0,sizeof(ag));
	ag.seeonly = atol(seeonly) ? TRUE : FALSE;
	if (file_openindex(toNative(path),OP_READONLY))	{
		if (XF = index_byfspec(toNative(path)))	{
			search_autogen(FF,XF, &ag);/* generate refs */
			if (!XF->vwind)		/* if cref index wasn't already open */
				index_close(XF);
			view_redisplay(FF,0,VD_CUR);
			return TRUE;
		}
	}
	return FALSE;
}
/************************************************************************/
static BOOL command_alter(INDEX * FF, char * offset)	/* alters refs */

{
	struct adjstruct tadj;

	memset(&tadj, 0, sizeof(struct adjstruct));
	tadj.low = 1;
	tadj.high = LONG_MAX;
	tadj.shift = atol(offset);
	ref_adjust(FF, &tadj);
	view_redisplay(FF,0,VD_CUR);
	return TRUE;
}
/************************************************************************/
static BOOL command_label(INDEX * FF, char * label)	/* labels all records in view */

{
	int color = atol(label);

	if (color >= 0 && color <= 7)	{
		SendMessage(FF->vwind,WM_COMMAND,(WPARAM)(IDM_EDIT_SELECTALL),0);
		edit_switchtag(FF->vwind,color);
		view_clearselect(FF->vwind);
		return TRUE;
	}
	return FALSE;
}
/************************************************************************/
static HDDEDATA senddata_recordinfo(INDEX * FF)	/* returns record info as data */

{
	CINDEXRECORDINFO ri;
	HSZ cstring;
	
	memset(&ri,0,sizeof(ri));
	ri.recordLength = FF->head.indexpars.recsize;
	ri.totalRecords = FF->head.rtot;

//	cstring = DdeCreateStringHandle(d_ddeinst, CINAPI_GETRECORDINFO, CP_WINANSI);
	cstring = DdeCreateStringHandle(d_ddeinst, TEXT(CINAPI_GETRECORDINFO), CP_WINUNICODE);
	return DdeCreateDataHandle(d_ddeinst,(unsigned char *)&ri,sizeof(ri),0,cstring,CF_TEXT,0);
}
/************************************************************************/
static HDDEDATA senddata_sortinfo(INDEX * FF)	/* returns sort info as data */

{
	CINDEXSORTINFO si;
	HSZ cstring;
	int index;
	
	memset(&si,0,sizeof(si));
	si.sortType = FF->head.sortpars.type;
	for (index = 0; index < MAXRECORDFIELDS; index++)
		si.fieldOrder[index] = FF->head.sortpars.fieldorder[index];
	si.sortActive = FF->head.sortpars.ison;
	strcpy(si.language,FF->head.sortpars.language);
	
//	cstring = DdeCreateStringHandle(d_ddeinst, CINAPI_GETSORTINFO, CP_WINANSI);
	cstring = DdeCreateStringHandle(d_ddeinst, TEXT(CINAPI_GETSORTINFO), CP_WINUNICODE);
	return DdeCreateDataHandle(d_ddeinst,(unsigned char *)&si,sizeof(si),0,cstring,CF_TEXT,0);
}
/************************************************************************/
static HDDEDATA senddata_fontinfo(INDEX * FF)	/* returns font info as data */

{
	CINDEXFONTINFO fi;
	HSZ cstring;
	int index;
	
	memset(&fi,0,sizeof(fi));
	for (index = 0; index < MAXFONTS && *FF->head.fm[index].pname; index++)	{
		strcpy(fi.fontArray[index].name,FF->head.fm[index].name);
		strcpy(fi.fontArray[index].pname,FF->head.fm[index].pname);
	}
	fi.totalFonts = index;
//	cstring = DdeCreateStringHandle(d_ddeinst, CINAPI_GETFONTINFO, CP_WINANSI);
	cstring = DdeCreateStringHandle(d_ddeinst, TEXT(CINAPI_GETFONTINFO), CP_WINUNICODE);
	return DdeCreateDataHandle(d_ddeinst,(unsigned char *)&fi,sizeof(fi),0,cstring,CF_TEXT,0);
}
/************************************************************************/
static HDDEDATA senddata_record(INDEX * FF, int type)	/* returns specified record */

{
	enum {
		W_DELFLAG = 1,
		W_TAGFLAG = 2,
		W_GENFLAG = 4
	};
//	static RECN current;

	LFLIST * lfp;
	RECN firstsel;
	RECORD * recptr;
	unsigned char record[MAXREC+100],*tptr;
	CSTR fields[FIELDLIM];
	int ftot, fieldcount;
	char recflags;
	HSZ cstring;
	
	lfp = getdata(FF->vwind);
	firstsel = lfp->sel.first;
	if (type == DD_GETSELECTEDRECORD)	{
		if (!firstsel || !(recptr = lfp->getrec(FF, firstsel)))
			return 0L;
	}
	else	{
		if (firstsel && (recptr = lfp->getrec(FF, firstsel)))	{
			if (!(recptr = lfp->skip(FF, recptr,1)))
				return 0L;
			view_selectrec(FF,recptr->num,VD_SELPOS,-1,-1);
		}
		else
			return 0L;
	}
//	current = recptr->num;
	ftot = str_xparse(recptr->rtext, fields);		/* parse */
	for (tptr = record, fieldcount = 0; fieldcount < ftot-1; fieldcount++) /* for all regular fields */
		tptr += sprintf(tptr, "%s\t", fields[fieldcount].str);  /* tab delimited */
	tptr += sprintf(tptr,"%s",fields[fieldcount].str);	/* locator field */
	recflags = 0;
	if (recptr->label)
		recflags |= W_TAGFLAG;
	if (recptr->isgen)
		recflags |= W_GENFLAG;
	if (recptr->isdel)
		recflags |= W_DELFLAG;
	if (recflags)
		recflags |= 64;			/* allows DOS value to be 'A' */
	else
		recflags = SPACE;
	tptr += sprintf(tptr, "\023%c%lu %.4s", recflags, recptr->time, recptr->user);				
//	cstring = DdeCreateStringHandle(d_ddeinst, type == DD_GETSELECTEDRECORD ? CINAPI_GETSELECTEDRECORD : CINAPI_GETNEXTRECORD, CP_WINANSI);
	cstring = DdeCreateStringHandle(d_ddeinst, type == DD_GETSELECTEDRECORD ? TEXT(CINAPI_GETSELECTEDRECORD) : TEXT(CINAPI_GETNEXTRECORD), CP_WINUNICODE);
	return DdeCreateDataHandle(d_ddeinst,record,(tptr+1)-record,0,cstring,CF_TEXT,0);
}
/************************************************************************/
static HDDEDATA senddata_versioninfo(INDEX * FF)	/* returns cindex version info */

{
	CINDEXVERSIONINFO vi;
	HSZ cstring;
	
	memset(&vi,0,sizeof(vi));
	vi.version = CINVERSION;

//	cstring = DdeCreateStringHandle(d_ddeinst, CINAPI_GETVERSIONINFO, CP_WINANSI);
	cstring = DdeCreateStringHandle(d_ddeinst, TEXT(CINAPI_GETVERSIONINFO), CP_WINUNICODE);
	return DdeCreateDataHandle(d_ddeinst,(unsigned char *)&vi,sizeof(vi),0,cstring,CF_TEXT,0);
}
/************************************************************************/
static HDDEDATA data_dofind(INDEX * FF, CINDEXFINDREPLACE * fr, int datalength)	/* finds specified records */

{
	LISTGROUP lg;
	GROUPHANDLE gh;

	if (datalength == sizeof(CINDEXFINDREPLACE) && setupsearch(FF,&lg,fr,DD_FINDALL)) {
		if (gh = grp_startgroup(FF))	{	/* initialize a group */
			gh->lg = lg;				/* load current search pars */
			lg.lsarray[0].auxptr = NULL;		/* empty pointer (mem is freed by grp) */
			if (grp_buildfromsearch(FF,&gh))	{	/* make temporary group */
				grp_installtemp(FF,gh);
				SendMessage(FF->vwind,WM_COMMAND,IDM_VIEW_TEMPORARYGROUP,0);	/* display group */
				view_selectrec(FF,gh->recbase[0],VD_SELPOS,-1,-1);
				return (HDDEDATA)DDE_FACK;
			}
			else
				grp_dispose(gh);
		}
		search_clearauxbuff(&lg);		/* release any buffers */
	}	
	return (HDDEDATA)DDE_FNOTPROCESSED;
}
/************************************************************************/
static HDDEDATA data_doreplace(INDEX * FF, CINDEXFINDREPLACE * fr, int datalength)	/* replaces text in record(s) */

{
	LISTGROUP lg;
	REPLACEGROUP rg;
	struct numstruct *nptr;
	REPLACEATTRIBUTES ra;		/* attrib structure */
	short mlength;
	RECN repcount;			/* replacement count */
	RECN markcount;			/* count of marked records */
	RECORD * recptr;
	char * sptr, *tptr;

	if (datalength == sizeof(CINDEXFINDREPLACE) && setupsearch(FF,&lg,fr,DD_REPLACEALL)) {	// if spec OK
		if (recptr = search_findfirst(FF,&lg,TRUE,&sptr,&mlength))	{		/* while target in invis part of rec */
			memset(&ra,0,sizeof(ra));
			repcount = markcount = 0;
			if (nptr = rep_setup(FF,&lg,&rg,&ra,fr->replaceText))	{	/* if have target & have/can set up structures */
				do {
					while (sptr && (tptr = search_reptext(FF,recptr, sptr, mlength, &rg, &lg.lsarray[0])))	{	/* while can replace a target */
						repcount++;
						sort_addtolist(nptr,recptr->num);		/* add to sort list */
						sptr = search_findbycontent(FF, recptr,tptr, &lg, &mlength); /* more in current record? */
					}
					if (sptr && !tptr)		/* if couldn't make a replacement */
						markcount++;
					str_adjustcodes(recptr->rtext,CC_TRIM|(g_prefs.gen.remspaces ? CC_ONESPACE : 0));	/* adjust codes etc */
					rec_strip(FF,recptr->rtext);		/* remove empty fields */
				} while ((recptr = search_findfirst(FF,&lg,FALSE,&sptr, &mlength)));
				view_clearselect(FF->vwind);
				sort_resortlist(FF,nptr);		/* make new nodes */
				freemem(nptr);
				search_clearauxbuff(&lg);		/* release any buffers */
				view_redisplay(FF,0,VD_CUR|VD_IMMEDIATE);
#if 1
				if (markcount)	/* if some marked records */
					sendinfo(INFO_REPLACEMARKED,repcount,markcount);
				else	
					sendinfo(INFO_REPLACECOUNT,repcount);
				return (HDDEDATA)DDE_FACK;
#endif
			}
		}
	}
	return (HDDEDATA)DDE_FNOTPROCESSED;
}
/************************************************************************/
static HDDEDATA data_doaddrecord(INDEX * FF, char * data)	/* adds record */

{
	char fbuff[MAXREC*2];		/* space for fields */
	IMPORTPARAMS imp;
	RECORD trec, *recptr;

	memset(&imp,0,sizeof(imp));
	strcpy(fbuff,data);	// work with copy of data in larger buffer
	imp.type = I_PLAINTAB;
	imp.subtype = TEXTKEY_UTF8;
	if (!imp_checkline(FF, fbuff,&imp, &trec))	{	/* if no error forming record */
		if (recptr = rec_makenew(FF,fbuff,FF->head.rtot+1)) { 	/* if can make new record */
			if (recptr->ismark = trec.ismark)	/* always flag a bad translation */
				imp.markcount++;		/* count it */
			if (trec.num)	{		/* if have to pick up extended info */
				recptr->isdel = trec.isdel;
				recptr->label = trec.label;
				recptr->isgen = trec.isgen;
				recptr->time = trec.time;
				strncpy(recptr->user,trec.user,4);
			}
			sort_makenode(FF,++FF->head.rtot);		/* make nodes */
			return (HDDEDATA)DDE_FACK;
		}
	}
	return (HDDEDATA)DDE_FNOTPROCESSED;
}
/************************************************************************/
static BOOL setupsearch(INDEX * FF, LISTGROUP * lg, CINDEXFINDREPLACE * fr, int type)

{
	RECORD * recptr;
	short group;

	if (type == DD_REPLACEALL)	{	// if bad fields for replace
		if (!*fr->searchText || fr->evaluateRefs)
			return FALSE;
	}
	memset(lg,0,sizeof(LISTGROUP));
	if (recptr = sort_top(FF))	{
		lg->firstr = recptr->num;
		lg->lastr = ULONG_MAX;
	}
	else
		return 0;
	lg->sortmode = FF->head.sortpars.ison;
	lg->size = 1;		// one group
	strncpy(lg->userid,fr->userId,4);
	if (fr->firstDate || fr->lastDate)	{	// if any date specified
		lg->firstdate = fr->firstDate;	// set up
		lg->lastdate = fr->lastDate;
	}
	else
		lg->lastdate = LONG_MAX;
	lg->lflags = COMR_ALL;		/* scope is all records */
	lg->excludeflag = (char)fr->excludeForAttributes;
	lg->newflag = (char)fr->findInNew;
	lg->delflag = (char)fr->findInDeleted;
	lg->markflag = (char)fr->findInMarked;
	lg->genflag = (char)fr->findInGenerated;
	lg->tagflag = (char)LOWORD(fr->findInLabeled);	// look for label
	lg->tagvalue = HIWORD(fr->findInLabeled);	// which label
	lg->lsarray[0].notflag = fr->not;
	lg->lsarray[0].field = (short)fr->field;
	lg->lsarray[0].caseflag = (char)fr->caseSensitive;
	lg->lsarray[0].wordflag = (char)fr->wordSensitive;
	lg->lsarray[0].evalrefflag = (char)fr->evaluateRefs;
	if (lg->lsarray[0].patflag = (char)fr->isPattern)	{
		lg->lsarray[0].wordflag = FALSE;
		lg->lsarray[0].caseflag = TRUE;
		lg->lsarray[0].evalrefflag = FALSE;
	}
	strcpy(lg->lsarray[0].string,fr->searchText);
	if (!search_setupfind(FF, lg, &group))	{		// bad setup
		search_clearauxbuff(lg);		/* release any buffers */
		return FALSE;
	}
	return (*lg->lsarray[0].string || *lg->userid || lg->newflag || lg->delflag || lg->markflag || lg->tagflag || lg->lsarray[0].field > 0);
}
