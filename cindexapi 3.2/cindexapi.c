// cindexapi.c : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include "cindexapi.h"

#define CINAPI_VERSION 320
#define CINAPI_TOPICNAME "cindex_API_320"

static HSZ service, topic;
static DWORD apidd = 0L;
static HCONV apich;

typedef struct {
	char * name;
	int minargs;
	int maxargs;
} APICOMMAND;

static APICOMMAND apicommlist[] = {
	CINAPI_QUIT,0,0,
	CINAPI_CREATE,1,2,
	CINAPI_OPEN, 2,2,
	CINAPI_CLOSE,0,0,
	CINAPI_SAVE,0,0,
	CINAPI_SAVEBACKUP,1,1,
	CINAPI_SAVEAS,2,2,
	CINAPI_IMPORT,1,1,
	CINAPI_PRINT,0,1,	// takes file arg in extended API
	CINAPI_SELECTALL,0,0,
	CINAPI_DELETE,0,0,
	CINAPI_COMPRESS,0,0,
	CINAPI_EXPAND,0,0,
	CINAPI_SORT,1,1,
	CINAPI_VIEW,1,1,
	CINAPI_GOTO,1,1,
	CINAPI_SETSIZE,1,1,
	CINAPI_SHOWINDEX,1,1,
	CINAPI_RECONCILE,2,2,	// extended API
	CINAPI_GENERATE,2,2,	// extended API
	CINAPI_ALTER,1,1,		// extended API
	CINAPI_LABEL,1,1,		// extended API

	CINAPI_FINDALL, 1,1,
	CINAPI_REPLACEALL, 1,1,
	CINAPI_ADDRECORD, 1,1,

	CINAPI_GETRECORDINFO,1,1,
	CINAPI_GETSORTINFO,1,1,
	CINAPI_GETFONTINFO,1,1,
	CINAPI_GETSELECTEDRECORD, 1,1,
	CINAPI_GETNEXTRECORD, 1,1,
	CINAPI_GETVERSIONINFO, 1, 1

};	/* list of recognized commands */
#define MAXDDECOMMANDS (sizeof(apicommlist)/sizeof(APICOMMAND))

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
	DD_GETVERSIONINFO
};

static int cconnect(DWORD * ddeinstptr,HCONV * chptr);	/*  connects for API */
static BOOL cdisconnect(DWORD ddeinst, HCONV chandle);	/*  disconnects for API */
static int csendcommand(HCONV chandle, char * command, char * arg1, char * arg2);	/*  sends API command */
static int csenddata(HCONV chandle, char * command, void * data, unsigned long dlength);	/*  sends data */
static int cgetdata(HCONV chandle, char * command, void * data);	/*  gets data */
static int csendquit(HCONV chandle);	/*  sends quit message */
static HDDEDATA CALLBACK ccallback(UINT type, UINT cfmt, HCONV cvhandle, HSZ str1, HSZ str2,HDDEDATA gdata,DWORD data1, DWORD data2);

/************************************************************************/
BOOL APIENTRY DllMain( HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call) {
		case DLL_PROCESS_ATTACH:
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			break;
    }
    return TRUE;
}
/************************************************************************/
CINDEXAPI_API INT32 cindex_getversion(void)	/*  gets API version */

{
	return CINAPI_VERSION;
}
/************************************************************************/
CINDEXAPI_API INT32 cindex_connect(void)	/*  connects for API */

{
	if (apich)
		return TRUE;
	return cconnect(&apidd,&apich);
}
/************************************************************************/
CINDEXAPI_API INT32 cindex_disconnect(void)	/*  disconnects for API */

{
	int ok;

	if (!apich)
		return CINAPI_ERR_NOSERVER;
	ok = cdisconnect(apidd,apich);
	if (ok)	{
		apidd = 0;
		apich = NULL;
	}
	return ok;
}
/************************************************************************/
CINDEXAPI_API INT32 cindex_command(char * command, char * arg1, char * arg2)	/*  executes command */

{
	int comindex;

	if (!apidd || !apich)
		return CINAPI_ERR_NOSERVER;
	if (!command)		/* no command */
		return (CINAPI_ERR_UNKNOWNCOMMAND);
	for (comindex = 0; comindex < MAXDDECOMMANDS; comindex++)	{
		if (!stricmp(command,apicommlist[comindex].name))		/* if a command we know */
			break;
	}
	if (comindex == MAXDDECOMMANDS)		/* if we don't know the command */
		return (CINAPI_ERR_UNKNOWNCOMMAND);
#if 0
	if (!apicommlist[comindex].minargs && (arg1 || arg2)
		|| apicommlist[comindex].minargs == 1 && !arg1 && arg2
		|| apicommlist[comindex].minargs == 2 && (!arg1 || !arg2))
#else
	if (apicommlist[comindex].minargs == 1 && !arg1 && arg2
		|| apicommlist[comindex].minargs == 2 && (!arg1 || !arg2)
		|| apicommlist[comindex].maxargs == 0 && (arg1 || arg2)
		|| apicommlist[comindex].maxargs == 1 && arg2
	)
#endif
		return (CINAPI_ERR_NUMARGUMENTS);		// NB need more checking for maxargs
	if (comindex < DD_FINDALL)
		return csendcommand(apich,command,arg1,arg2);
	else if (comindex <= DD_ADDRECORD)	{	// if sending data
		unsigned long dlength;
		switch (comindex) {
			case DD_FINDALL:
				dlength = sizeof(CINDEXFINDREPLACE);
				break;
			case DD_REPLACEALL:
				dlength = sizeof(CINDEXFINDREPLACE);
				break;
			case DD_ADDRECORD:
				dlength = strlen(arg1)+1;
				break;
			default:
				return FALSE;
		}
		return csenddata(apich,command,arg1, dlength);
	}
	else	// called a getdata command
		return CINAPI_ERR_UNKNOWNCOMMAND;
}
/************************************************************************/
CINDEXAPI_API INT32 cindex_getdata(char * command, void *data)	/*  executes command */

{
	int comindex;

	if (!apidd || !apich)
		return CINAPI_ERR_NOSERVER;
	if (!command)		/* no command */
		return (CINAPI_ERR_UNKNOWNCOMMAND);
	for (comindex = 0; comindex < MAXDDECOMMANDS; comindex++)	{
		if (!stricmp(command,apicommlist[comindex].name))		/* if a command we know */
			break;
	}
	if (comindex == MAXDDECOMMANDS || comindex < DD_GETRECORDINFO)		/* if we don't know the command */
		return (CINAPI_ERR_UNKNOWNCOMMAND);
	return cgetdata(apich,command,data);
}
/************************************************************************/
static int cconnect(DWORD * ddeinstptr,HCONV * chptr)	/*  connects for API */

{
	HCONV ddc;

	if (DdeInitialize(ddeinstptr,ccallback,
		CBF_FAIL_ADVISES|CBF_FAIL_POKES|CBF_SKIP_REGISTRATIONS|CBF_SKIP_UNREGISTRATIONS
		|CBF_FAIL_SELFCONNECTIONS|CBF_SKIP_CONNECT_CONFIRMS,0) == DMLERR_NO_ERROR)	{	/* if can initialize */
		service = DdeCreateStringHandle(*ddeinstptr, "cindex", CP_WINANSI);
		topic = DdeCreateStringHandle(*ddeinstptr,CINAPI_TOPICNAME,CP_WINANSI);
		if (ddc = DdeConnect(*ddeinstptr,service,topic,NULL))	{
			*chptr = ddc;
			return TRUE;
		}
		if (DdeGetLastError(*ddeinstptr)== DMLERR_NO_CONV_ESTABLISHED)
			return CINAPI_ERR_NOCONVERSATION;
	}
	return CINAPI_ERR_NOSERVER;
}
/************************************************************************/
static BOOL cdisconnect(DWORD ddeinst, HCONV chandle)	/*  disconnects for API */

{
	if (DdeDisconnect(chandle))	{
		if (DdeFreeStringHandle(ddeinst,service))	{
			if (DdeFreeStringHandle(ddeinst,topic))	{
				DdeUninitialize(ddeinst);
				return (TRUE);
			}
		}
	}
	return FALSE;
}
/************************************************************************/
static int csendcommand(HCONV chandle, char * command, char * arg1, char * arg2)	/*  sends API command */

{
	int len;
	char comstring[1024];	// need to fix this later
	DWORD result;
	HDDEDATA okflag;

	if (strchr(command, '\t') || arg1 && strchr(arg1, '\t') || arg2 && strchr(arg2, '\t'))	// if any string contains a tab
		return (FALSE);
	len = sprintf(comstring,"%s%c%s%c%s",command, '\t', arg1 ? arg1 : "", '\t', arg2 ? arg2 : "");
	okflag = DdeClientTransaction(comstring,len+1,chandle,0L,0,XTYP_EXECUTE,1000*300,&result);
	return okflag ? TRUE : CINAPI_ERR_TRANSACTIONFAILED;
}
/************************************************************************/
static int csenddata(HCONV chandle, char * command, void * data, unsigned long dlength)	/*  sends data */

{
	DWORD result;
	HDDEDATA okdata;
	HSZ cstring;
	HDDEDATA hdata;
	int err;

	cstring = DdeCreateStringHandle(apidd, command, CP_WINANSI);
	hdata = DdeCreateDataHandle(apidd,(unsigned char *)data,dlength,0,cstring,CF_TEXT,0);
	okdata = DdeClientTransaction((char *)hdata,-1,chandle,cstring,CF_TEXT,XTYP_POKE,1000*300,&result);
	err = DdeGetLastError(apidd);
	return okdata ? TRUE : CINAPI_ERR_TRANSACTIONFAILED;
}
/************************************************************************/
static int cgetdata(HCONV chandle, char * command, void * data)	/*  gets data */

{
	DWORD result;
	HDDEDATA okdata;
	DWORD dlength;
	void * rdata;
	HSZ cstring;

	cstring = DdeCreateStringHandle(apidd, command, CP_WINANSI);
	okdata = DdeClientTransaction(NULL,-1,chandle,cstring,CF_TEXT,XTYP_REQUEST,1000*5,&result);
	if (okdata)	{
		if (rdata = DdeAccessData(okdata, &dlength))	{	// if OK
			if (data)
				memcpy(data,rdata,dlength);
			DdeUnaccessData(okdata);
			DdeFreeDataHandle(okdata);
			return (dlength);
		}
	}
	return CINAPI_ERR_TRANSACTIONFAILED;
}
/************************************************************************/
static HDDEDATA CALLBACK ccallback(UINT type, UINT cfmt, HCONV cvhandle, HSZ str1, HSZ str2,HDDEDATA hdata,DWORD data1, DWORD data2)

{
	void * data;

	switch (type)	{
		case XTYP_EXECUTE:
			data = DdeAccessData(hdata,NULL);
			DdeUnaccessData(hdata);
			return ((HDDEDATA)DDE_FACK);
		case XTYP_CONNECT:
			return ((HDDEDATA)TRUE);
		case XCLASS_NOTIFICATION:
			return ((HDDEDATA)TRUE);
	}
	return ((HDDEDATA)TRUE);
}