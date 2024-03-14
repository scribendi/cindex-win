#include "stdafx.h"
#include "files.h"
#include "errors.h"
#include "dde.h"
#include "apiservice.h"
#include "registry.h"

BOOL d_ddeopen;

static HSZ service, apitopic;
DWORD d_ddeinst;	/* ddeml instance */
static TCHAR * ddecommlist[] = {TEXT("open")};	/* list of recognized commands */
#define MAXDDECOMMANDS (sizeof(ddecommlist)/sizeof(char *))

static HDDEDATA CALLBACK callback(UINT type, UINT cfmt, HCONV cvhandle, HSZ str1, HSZ str2,HDDEDATA gdata, ULONG_PTR data1, ULONG_PTR data2);
static BOOL docommand(TCHAR *string);	/* executes dde command */
static BOOL doconnect(HSZ topic);	/* checks if being sent our regnum */
static void restoreframe(void);	/* forces frame to full size */
/************************************************************************/
void dde_setup(void)		/* initializes ddeml and sets up server */

{
	if (DdeInitialize(&d_ddeinst,callback,
		CBF_FAIL_ADVISES|CBF_SKIP_REGISTRATIONS|CBF_SKIP_UNREGISTRATIONS
		|CBF_FAIL_SELFCONNECTIONS|CBF_SKIP_CONNECT_CONFIRMS,0) == DMLERR_NO_ERROR)	{	/* if can initialize */
		service = DdeCreateStringHandle(d_ddeinst, TEXT("cindex"), CP_WINUNICODE);
#ifdef PUBLISH
		apitopic = DdeCreateStringHandle(d_ddeinst,TEXT(CINAPI_TOPICNAME),CP_WINUNICODE);
#endif
		DdeNameService(d_ddeinst,service,NULL,DNS_REGISTER|DNS_FILTERON);
	}
}
/************************************************************************/
void dde_close(void)		/*  cleans up ddeml */

{
	DdeNameService(d_ddeinst,0,0,DNS_UNREGISTER);
	DdeFreeStringHandle(d_ddeinst,service);
	DdeUninitialize(d_ddeinst);
}
/************************************************************************/
BOOL dde_sendcheck(void)	/*  returns TRUE if this copy of CINDEX running */

{
	TCHAR regnum[STSTRING];
	DWORD vsize = STSTRING;
	HSZ rh;
	HCONV cerror;
	int err;

	reg_getmachinekeyvalue(NULL,TEXT("Sernum"),regnum,&vsize);
	rh = DdeCreateStringHandle(d_ddeinst,regnum,CP_WINUNICODE);
	cerror = DdeConnect(d_ddeinst,service,rh,NULL);
	DdeFreeStringHandle(d_ddeinst,rh);
	if (cerror)	{	/* if succeeded */
		DdeDisconnect(cerror);
		return (TRUE);
	}
	else
		err = DdeGetLastError(d_ddeinst);
	return FALSE;
}
#ifdef PUBLISH
/************************************************************************/
static HDDEDATA CALLBACK callback(UINT type, UINT cfmt, HCONV cvhandle, HSZ str1, HSZ str2,HDDEDATA hdata, ULONG_PTR data1, ULONG_PTR data2)

{
	void * data;
	BOOL ok;
	HDDEDATA okdata;

	switch (type)	{
		case XTYP_EXECUTE:
			restoreframe();
			data = DdeAccessData(hdata,NULL);
			if (DdeCmpStringHandles(str1,apitopic))	/* not an api topic */
				ok = docommand(data);
			else	{
				api_active = TRUE;
				ok = api_docommand(data);
				api_active = FALSE;
			}
			DdeUnaccessData(hdata);
			return (ok ? (HDDEDATA)DDE_FACK :(HDDEDATA)DDE_FNOTPROCESSED);
		case XTYP_REQUEST:
			restoreframe();
			okdata = (HDDEDATA)DDE_FNOTPROCESSED;
			if (!DdeCmpStringHandles(str1,apitopic))	{	/* if an api topic */
				TCHAR tstring[256];
//				if (DdeQueryString(d_ddeinst,str2,tstring,256,CP_WINANSI))	{
				if (DdeQueryString(d_ddeinst,str2,tstring,256,CP_WINUNICODE))	{
					api_active = TRUE;
					okdata = api_senddata(tstring);
					api_active = FALSE;
				}
			}
			return (okdata);
		case XTYP_POKE:
			restoreframe();
			okdata = (HDDEDATA)DDE_FNOTPROCESSED;
			if (!DdeCmpStringHandles(str1,apitopic))	{	/* if an api topic */
				TCHAR tstring[256];
				unsigned char * rdata;
				DWORD dlength;
				if (DdeQueryString(d_ddeinst,str2,tstring,256,CP_WINUNICODE))	{
					if (rdata = DdeAccessData(hdata, &dlength))	{	// if OK
						api_active = TRUE;
						okdata = api_receivedata(tstring, rdata, dlength);
						api_active = FALSE;
						DdeUnaccessData(hdata);
						DdeFreeDataHandle(hdata);
					}
				}
			}
			return (okdata);
		case XTYP_CONNECT:
#if 0
			if (doconnect(str1))	/* if can handle */
				return ((HDDEDATA)FALSE);
			else
				return ((HDDEDATA)TRUE);
#else
			return ((HDDEDATA)doconnect(str1));	/* if can handle */
#endif
		case XTYP_DISCONNECT:
			if (api_quitondisconnect)
				SendMessage(g_hwframe,WM_CLOSE,0,0);
		case XCLASS_NOTIFICATION:
			return ((HDDEDATA)TRUE);
	}
	return ((HDDEDATA)TRUE);
}
#else
/************************************************************************/
static HDDEDATA CALLBACK callback(UINT type, UINT cfmt, HCONV cvhandle, HSZ str1, HSZ str2,HDDEDATA hdata, ULONG_PTR data1, ULONG_PTR data2)

{
	void * data;

	switch (type)	{
		case XTYP_EXECUTE:
			data = DdeAccessData(hdata,NULL);
			docommand(data);
			DdeUnaccessData(hdata);
			return ((HDDEDATA)DDE_FACK);
		case XTYP_CONNECT:
			return ((HDDEDATA)TRUE);
		case XCLASS_NOTIFICATION:
			return ((HDDEDATA)TRUE);
	}
	return ((HDDEDATA)TRUE);
}
#endif
/************************************************************************/
static BOOL docommand(TCHAR *string)	/* executes dde command */

{
	TCHAR tstring[STSTRING], *comstring, *eptr;
	int comindex, length;

	length = nstrlen(string);
	if (length > STSTRING || *string != '[' || string[length-1] != ']')	/* if bad parse */
		return (FALSE);
	nstrcpy(tstring,string+1);
	comstring = nstrchr(tstring,'(');
	if (!comstring)
		return (FALSE);
	*comstring++ = '\0';	/* terminate command name */
	eptr = nstrrchr(comstring,')');
	if (!eptr)
		return (FALSE);
	*eptr = '\0';
	
	for (comindex = 0; comindex < MAXDDECOMMANDS; comindex++)	{
		if (!nstricmp(tstring,ddecommlist[comindex]))		/* if a command we know */
			break;
	}
	if (comindex == MAXDDECOMMANDS)		/* if we don't know the command */
		return (FALSE);
	switch (comindex)	{
		case 0:			/* open */
			d_ddeopen = TRUE;		/* use to inhibit opening of last index */
#if 0
			if ((eptr = file_getextn(comstring)) && !nstricmp(++eptr,f_indexext))
				file_openindex(comstring,OP_VISIBLE);
			else if ((eptr = file_getextn(comstring)) && (!nstricmp(++eptr,f_arcext) || !nstricmp(++eptr,f_xmlext)))
				file_openarchive(comstring);
			else if ((eptr = file_getextn(comstring)) && !nstricmp(++eptr,f_tplext))
				file_opentemplate(comstring,NULL);
			else
				return (FALSE);
			return (TRUE);
#else
			file_open(comstring,OP_VISIBLE);
#endif
		default:
			return (FALSE);
	}
}
#ifdef PUBLISH
/************************************************************************/
static BOOL doconnect(HSZ topic)	/* checks if can handle topic */

{
	TCHAR tstring[STSTRING];
	int len1;
	char regnum[STSTRING];
	DWORD vsize = STSTRING;

	reg_getmachinekeyvalue(NULL,TEXT("Sernum"),regnum,&vsize);
	len1 = DdeQueryString(d_ddeinst, topic,tstring,STSTRING,CP_WINUNICODE);
	if (len1 && (!nstrncmp(tstring,TEXT(CINAPI_TOPICNAME),len1) || !nstrncmp(tstring,SZDDESYS_TOPIC,len1)))	/* if found a match */
		return TRUE;
	return (FALSE);
}
/************************************************************************/
static void restoreframe(void)	/* forces frame to full size */
{
	if (IsIconic(g_hwframe))	// if window minimized
		ShowWindow(g_hwframe,SW_RESTORE);//   maximize cindex
}
#else
/************************************************************************/
static BOOL doconnect(HSZ topic)	/* checks if being sent our regnum */

{
	TCHAR tstring[STSTRING];
	int len1;
	TCHAR regnum[STSTRING];
	DWORD vsize = STSTRING;

	reg_getmachinekeyvalue(NULL,TEXT("Sernum"),regnum,&vsize);
	len1 = DdeQueryString(d_ddeinst, topic,tstring,STSTRING,CP_WINUNICODE);
	if (len1 && !nstrncmp(tstring,regnum,len1))	/* if found a match */
		return (TRUE);
	return (FALSE);
}
#endif