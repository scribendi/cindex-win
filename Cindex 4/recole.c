#include "stdafx.h"
#include "recole.h"
#include "errors.h"
#include "commands.h"
#include "search.h"
#include "records.h"
#include "index.h"
#include "strings.h"
#include "sort.h"
#include "viewset.h"
#include "import.h"
#include "files.h"
#include "typestuff.h"
#include "formstuff.h"
#include "formattedexport.h"
#include "draftstuff.h"
#include "textwriter.h"
#include "rtfwriter.h"
#include "util.h"


typedef struct {		/* for copying records as data object */
	long textoffset;
	long textsize;
	long rtfoffset;
	long rtfsize;
	RECN rtot;
	COUNTPARAMS cs;
	FONTMAP fm[FONTLIMIT];
	char array[];
} RECCOPY;

#define cliprec(AA,BB)	((RECORD *)((AA)->array+(BB)*(sizeof(RECORD)+(AA)->cs.longest+1)))

// {5C74DAC0-05CC-11d1-9141-000000000000}
static const CLSID cr_clsID = 
{ 0x5c74dac0, 0x5cc, 0x11d1, { 0x91, 0x41, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 } };


#if 0
static STDMETHODIMP CLS_QueryInterface(IClassFactory __RPC_FAR * This,
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);

static STDMETHODIMP_(ULONG) CLS_AddRef(IClassFactory __RPC_FAR * This);

static STDMETHODIMP_(ULONG) CLS_Release(IClassFactory __RPC_FAR * This);

static STDMETHODIMP CLS_CreateInstance(IClassFactory __RPC_FAR * This,
    /* [unique][in] */ IUnknown __RPC_FAR *pUnkOuter,
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);

static STDMETHODIMP CLS_LockServer(IClassFactory __RPC_FAR * This,
    /* [in] */ BOOL fLock);
#endif

static STDMETHODIMP DTA_QueryInterface(IDataObject __RPC_FAR * This, REFIID riid, LPVOID FAR * lplpObj);		/* */
static STDMETHODIMP_(LONG) DTA_AddRef(IDataObject __RPC_FAR * This);		/* */
static STDMETHODIMP_(LONG) DTA_Release(IDataObject __RPC_FAR * This);		/* */
static STDMETHODIMP DTA_GetData(IDataObject __RPC_FAR * This, 
    /* [unique][in] */ FORMATETC __RPC_FAR *pformatetcIn,
    /* [out] */ STGMEDIUM __RPC_FAR *pmedium);		/* */
static STDMETHODIMP DTA_GetDataHere(IDataObject __RPC_FAR * This, 
    /* [unique][in] */ FORMATETC __RPC_FAR *pformatetc,
    /* [out][in] */ STGMEDIUM __RPC_FAR *pmedium);		/* */
static STDMETHODIMP DTA_QueryGetData(IDataObject __RPC_FAR * This, 
    /* [unique][in] */ FORMATETC __RPC_FAR *pformatetc);		/* */
static STDMETHODIMP DTA_GetCanonicalFormatEtc(IDataObject __RPC_FAR * This, 
    /* [unique][in] */ FORMATETC __RPC_FAR *pformatectIn,
    /* [out] */ FORMATETC __RPC_FAR *pformatetcOut);		/* */
static STDMETHODIMP DTA_SetData(IDataObject __RPC_FAR * This, 
    /* [unique][in] */ FORMATETC __RPC_FAR *pformatetc,
    /* [unique][in] */ STGMEDIUM __RPC_FAR *pmedium,
    /* [in] */ BOOL fRelease);		/* */
static STDMETHODIMP DTA_SetData(IDataObject __RPC_FAR * This, 
    /* [unique][in] */ FORMATETC __RPC_FAR *pformatetc,
    /* [unique][in] */ STGMEDIUM __RPC_FAR *pmedium,
    /* [in] */ BOOL fRelease);		/* */
static STDMETHODIMP DTA_EnumFormatEtc(IDataObject __RPC_FAR * This, 
    /* [in] */ DWORD dwDirection,
    /* [out] */ IEnumFORMATETC __RPC_FAR *__RPC_FAR *ppenumFormatEtc);		/* */
static STDMETHODIMP DTA_DAdvise(IDataObject __RPC_FAR * This, 
    /* [in] */ FORMATETC __RPC_FAR *pformatetc,
    /* [in] */ DWORD advf,
    /* [unique][in] */ IAdviseSink __RPC_FAR *pAdvSink,
    /* [out] */ DWORD __RPC_FAR *pdwConnection);		/* */
static STDMETHODIMP DTA_DUnadvise(IDataObject __RPC_FAR * This, 
    /* [in] */ DWORD dwConnection);		/* */
static STDMETHODIMP DTA_EnumDAdvise(IDataObject __RPC_FAR * This, 
    /* [out] */ IEnumSTATDATA __RPC_FAR *__RPC_FAR *ppenumAdvise);		/* */



static STDMETHODIMP DSC_QueryInterface(IDropSource __RPC_FAR * This,
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);

static STDMETHODIMP_(ULONG) DSC_AddRef(IDropSource __RPC_FAR * This);

static STDMETHODIMP_(ULONG) DSC_Release(IDropSource __RPC_FAR * This);

static STDMETHODIMP DSC_QueryContinueDrag(IDropSource __RPC_FAR * This,
    /* [in] */ BOOL fEscapePressed,
    /* [in] */ DWORD grfKeyState);

static STDMETHODIMP DSC_GiveFeedback(IDropSource __RPC_FAR * This,
    /* [in] */ DWORD dwEffect);


static STDMETHODIMP DTG_QueryInterface(IDropTarget __RPC_FAR * This,
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);

static STDMETHODIMP_(ULONG) DTG_AddRef(IDropTarget __RPC_FAR * This);

static STDMETHODIMP_(ULONG) DTG_Release(IDropTarget __RPC_FAR * This);

static STDMETHODIMP DTG_DragEnter(IDropTarget __RPC_FAR * This,
    /* [unique][in] */ IDataObject __RPC_FAR *pDataObj,
    /* [in] */ DWORD grfKeyState,
    /* [in] */ POINTL pt,
    /* [out][in] */ DWORD __RPC_FAR *pdwEffect);

static STDMETHODIMP DTG_DragOver(IDropTarget __RPC_FAR * This,
    /* [in] */ DWORD grfKeyState,
    /* [in] */ POINTL pt,
    /* [out][in] */ DWORD __RPC_FAR *pdwEffect);

static STDMETHODIMP DTG_DragLeave(IDropTarget __RPC_FAR * This);

static STDMETHODIMP DTG_Drop(IDropTarget __RPC_FAR * This,
    /* [unique][in] */ IDataObject __RPC_FAR *pDataObj,
    /* [in] */ DWORD grfKeyState,
    /* [in] */ POINTL pt,
    /* [out][in] */ DWORD __RPC_FAR *pdwEffect);


#if 0
static IClassFactoryVtbl clsvtable = {
	CLS_QueryInterface,
	CLS_AddRef,
	CLS_Release,
	CLS_CreateInstance,
	CLS_LockServer
};
#endif

static IDropSourceVtbl srcvtable = {
	DSC_QueryInterface,
	DSC_AddRef,
	DSC_Release,
	DSC_QueryContinueDrag,
	DSC_GiveFeedback
};

static IDropTargetVtbl trgvtable = {
	DTG_QueryInterface,
	DTG_AddRef,
	DTG_Release,
	DTG_DragEnter,
	DTG_DragOver,
	DTG_DragLeave,
	DTG_Drop
};

static IDataObjectVtbl objvtable = {
	DTA_QueryInterface,
	DTA_AddRef,
	DTA_Release,
	DTA_GetData,
	DTA_GetDataHere,
	DTA_QueryGetData,
	DTA_GetCanonicalFormatEtc,
	DTA_SetData,
	DTA_EnumFormatEtc,
	DTA_DAdvise,
	DTA_DUnadvise,
	DTA_EnumDAdvise
};

DSRCOBJ dsrc_call = {
	&srcvtable		/* drop source interface ptr */
					/* other members unassigned */
};

DTRGOBJ dtrg_call = {
	&trgvtable		/* drop target interface ptr */
					/* other members unassigned */
};

RECORDOBJ rec_call = {
	&objvtable		/* data object interface ptr */
					/* other members unassigned */
};

static char * gentext(INDEX * FF, char * tbase);	/* generates text */
static char * genrtf(INDEX * FF, char * tbase);	/* generates rtf */
static HRESULT recobj(RECORDOBJ * dop, STGMEDIUM __RPC_FAR *pmedium);	/* copies data */
static HRESULT rtfobj(RECORDOBJ * dop, STGMEDIUM __RPC_FAR *pmedium);	/* copies data */
static HRESULT textobj(RECORDOBJ * dop, STGMEDIUM __RPC_FAR *pmedium);	/* copies data */

/****************************************************************************/
BOOL recole_capture(HWND hwnd, RECORDOBJ * dp)		/* captures and holds selection */

{
	RECN rcount;
	RECCOPY * rc;
	RECORD * recptr;
	INDEX * FF;
	COUNTPARAMS cs;
	HCURSOR ocurs;
	long tsize, rsize, size;
	char *tbase, *eptr;

	FF = getowner(hwnd);
	dp->hwnd = hwnd;
	memset(&cs,0,sizeof(cs));
	cs.smode = FF->head.sortpars.ison;		/* sort is as the view */
	if (!com_getrecrange(FF,COMR_SELECT,NULL,NULL,&cs.firstrec, &cs.lastrec)) 	/* if range is ok */
		rcount = search_count(FF, &cs,SF_OFF);
#if 0
	rcount = search_count(FF, &cs, SF_OFF);
	if (FF->head.privpars.hidedelete)	/* if won't copy deleted */
		rcount -= cs.deleted;			/* adjust required count */
#else
	rcount = search_count(FF, &cs, SF_VIEWDEFAULT);
#endif
	if (dp->gh)		/* if have old data */
		GlobalFree(dp->gh);		/* get rid of it */
	tsize = cs.totaltext + 10*cs.deepest*rcount+		/* estimated size of text part */
		cs.totaltext + 50*cs.deepest*rcount+3000;	/* plus estimated size of rtf part */
	rsize = sizeof(RECCOPY)+rcount*(sizeof(RECORD)+cs.longest+1);	/* size of record part */
	if (dp->gh = GlobalAlloc(GMEM_MOVEABLE|GMEM_DDESHARE,tsize+rsize))	{	/* if can get memory */
		ocurs = SetCursor(g_waitcurs);
		rc = GlobalLock(dp->gh);
		rc->textoffset = rsize;	/* size of record segment (offset to text base) */
		rc->cs = cs;	/* copy stats */
		memcpy(rc->fm,FF->head.fm, sizeof(FF->head.fm));	/* and font map */
		rc->rtot = rcount;
		/* next for basic records */
		for (rcount = 0, recptr = rec_getrec(FF,cs.firstrec); recptr && recptr->num != cs.lastrec; recptr = sort_skip(FF,recptr,1), rcount++)	/* for all in sel range */
			memcpy(cliprec(rc,rcount),recptr,sizeof(RECORD)+cs.longest+1);	/* copy record */
		/* following for text in all forms */
		memset(&FF->pf,0,sizeof(PRINTFORMAT));	/* clear format info structure */
		FF->pf.firstrec = cs.firstrec;
		FF->pf.lastrec = cs.lastrec;
		FF->pf.first = 1;
		FF->pf.last = SHRT_MAX;
		tbase = (char *)rc+rc->textoffset;
		eptr = gentext(FF,tbase);	/* generate text */
		rc->textsize = eptr-tbase;
		tbase = eptr;
		rc->rtfoffset = eptr-(char *)rc;	/* offset to rtf base */
		eptr = genrtf(FF,tbase);
		rc->rtfsize = eptr-tbase;
		size = (char *)eptr-(char *)rc;
		GlobalUnlock(dp->gh);
		GlobalReAlloc(dp->gh,size,GMEM_ZEROINIT);
		SetCursor(ocurs);
		FF->typesetter = NULL;
		return (TRUE);
	}
	return (FALSE);
}
/****************************************************************************/
static char * gentext(INDEX * FF, char * tbase)	/* generates text */

{
	textcontrol.newlinestring = "\r\n";
	textcontrol.usetabs = g_prefs.gen.indentdef;		// set leader tab control
	textcontrol.esptr = tbase;
	FF->typesetter = &textcontrol;
	if (FF->head.privpars.vmode == VM_FULL)	{		/* if fully formatted */
		(FF->typesetter->efstart)(FF,FF->typesetter);	/*  initialize */
		if (!formexport_writepastable(FF))	// if error
			return (tbase);			/* nothing done */
	}
	else
		formexport_writeembedded(FF);
	(FF->typesetter->efend)(FF,FF->typesetter);	/* build end string */
	return (FF->typesetter->esptr);
}
/****************************************************************************/
static char * genrtf(INDEX * FF, char * tbase)	/* generates rtf */

{
//	rtfcontrol.efptr = NULL;
	rtfcontrol.newlinestring = "\r\n";
	rtfcontrol.usetabs = g_prefs.gen.indentdef;		// set leader tab control
	rtfcontrol.esptr = tbase;
	FF->typesetter = &rtfcontrol;
	if (FF->head.privpars.vmode == VM_FULL)	{		/* if fully formatted */
		(FF->typesetter->efstart)(FF,FF->typesetter);	/*  initialize */
#if 0
		if (!view_writeformatted(FF))		/* if error */
			return (tbase);			/* nothing done */
#else
		if (!formexport_writepastable(FF))	// if error
			return (tbase);			/* nothing done */
#endif
	}
	else	{
		strcpy(FF->typesetter->esptr,"{\\rtf1\\ansi");	/* generate header */
		FF->typesetter->esptr += strlen(FF->typesetter->esptr);
		rtf_setfonts(FF, FF->typesetter);			/* and font table */
		formexport_writeembedded(FF);
	}
	(FF->typesetter->efend)(FF,FF->typesetter);	/* build end string */
	return (FF->typesetter->esptr);
}
/****************************************************************************/
BOOL recole_ispastable(IDataObject * dp)		/* checks if pastable records */

{
	FORMATETC fm;

	fm.ptd = NULL;
	fm.dwAspect = DVASPECT_CONTENT;
	fm.lindex = -1;
	fm.tymed = TYMED_HGLOBAL;
	fm.cfFormat = g_recref;
	if (SUCCEEDED(dp->lpVtbl->QueryGetData(dp,&fm)))	/* check if records */
		return (TRUE);
	return (FALSE);
}
/******************************************************************************/
int recole_paste(HWND hwnd, IDataObject * dp)	/* pastes records */

{
	FORMATETC fm;
	STGMEDIUM sm;
	SCODE sc;
	RECCOPY * rc;
	INDEX * FF;
	__int64 freespace;
	short minsize;
	int nfindex, ok;
	RECORD *srecptr, *drecptr;
	RECN rcount, oldbase;
	short nfarray[FONTLIMIT], farray[FONTLIMIT];
	HCURSOR ocurs;

	ok = FALSE;
	fm.ptd = NULL;
	fm.dwAspect = DVASPECT_CONTENT;
	fm.lindex = -1;
	fm.tymed = TYMED_HGLOBAL;
	fm.cfFormat = g_recref;
	sc = dp->lpVtbl->GetData(dp,&fm,&sm);	/* check if right data */
	if (SUCCEEDED(sc))	{
		ocurs = SetCursor(g_waitcurs);
		FF = WX(hwnd,owner);
		rc = GlobalLock(sm.hGlobal);
		freespace = file_diskspace(FF->pfspec);		/* free space on disk */
		minsize = FF->head.indexpars.recsize > rc->cs.longest ? FF->head.indexpars.recsize : rc->cs.longest;
		if (freespace < minsize*rc->rtot)	{	/* if no room on disk */
			senderr(ERR_DISKFULLERR,WARN);
			goto abort;
		}
		if (rc->cs.deepest > FF->head.indexpars.maxfields)	{	/* if need to increase field limit */
			if (sendwarning(WARN_RECFIELDNUMWARN, rc->cs.deepest))	{
				short oldmaxfieldcount;
				oldmaxfieldcount = FF->head.indexpars.maxfields;
				FF->head.indexpars.maxfields = rc->cs.deepest;
				adjustsortfieldorder(FF->head.sortpars.fieldorder, oldmaxfieldcount, FF->head.indexpars.maxfields);
			}
			else
				goto abort;
		}
		if (rc->cs.longest > FF->head.indexpars.recsize)	{	/* if need record enlargement */
			if (!sendwarning(WARN_RECENLARGESIZE,rc->cs.longest-FF->head.indexpars.recsize) ||	/* if don't want resize */
			   !file_resizeindex(FF, rc->cs.longest))		/* or errors resizing */ 
				goto abort;		/* can't do it */
		}
		/* here we build font conversion map */
		memset(farray,0,FONTLIMIT * sizeof(short));		/* this array holds tags for fonts used */
		for (rcount = 0; rcount <rc->rtot; rcount++)	{/* for all records */
			srecptr = cliprec(rc,rcount);
			type_tagfonts(srecptr->rtext,farray);		/* marks source fonts used */
		}
		memset(nfarray,0,FONTLIMIT*sizeof(short));		/* this array holds distination ids indexed by source id */
		for (nfindex = VOLATILEFONTS; nfindex < FONTLIMIT; nfindex++)	{	/* for every font index */
			if (farray[nfindex])					/* if source local id is used */
				nfarray[nfindex] = type_findlocal(FF->head.fm,rc->fm[nfindex].name,VOLATILEFONTS);	/* get new local id for it */
		}
		if (index_setworkingsize(FF,rc->rtot+MAPMARGIN))	{	
			for (oldbase = FF->head.rtot, rcount = 0; rcount < rc->rtot; rcount++) {	/* for all records */
				srecptr = cliprec(rc,rcount);
				if (drecptr = rec_makenew(FF,srecptr->rtext,FF->head.rtot+1))	{
					int fcount;
					type_setfontids(drecptr->rtext,nfarray);
					str_adjustcodes(drecptr->rtext,CC_TRIM|(g_prefs.gen.remspaces ? CC_ONESPACE : 0));	/* clean up codes, etc */
					fcount = rec_strip(FF,drecptr->rtext);		/* strip surplus fields */
					if (fcount < FF->head.indexpars.minfields)		/* if too few fields */
						rec_pad(FF,drecptr->rtext);
					drecptr->isdel = srecptr->isdel;
					drecptr->label = srecptr->label;
					sort_makenode(FF,++FF->head.rtot);		/* make nodes */
				}
			}
			index_flush(FF);		/* force update on file */
			file_disposesummary(FF);	/* dispose of any summary */
			if (FF->curfile)		/* if viewing group */
				view_allrecords(hwnd);		/* show all records */
			else
				view_redisplay(FF,oldbase+1,VD_SELPOS|VD_IMMEDIATE);	/* set new screen */
			view_selectrec(FF,oldbase+1,VD_SELPOS,-1,-1);	/* select first record added */
		}
		SetCursor(ocurs);
		ok = TRUE;
abort:
		GlobalUnlock(sm.hGlobal);
	}
	return (ok);
}
/******************************************************************************/
static STDMETHODIMP DTA_QueryInterface(IDataObject __RPC_FAR * This, REFIID riid, LPVOID FAR * lplpObj)		/* */

{
	SCODE sc = S_OK;

	if (IsEqualIID(riid, &IID_IUnknown))
		DTA_AddRef(*lplpObj = This);
	else if (IsEqualIID(riid, &IID_IDataObject))
		DTA_AddRef(*lplpObj = This);
	else	{
		sc = E_NOINTERFACE;
		*lplpObj = NULL;
	}
	return (sc);
}
/******************************************************************************/
static STDMETHODIMP_(LONG) DTA_AddRef(IDataObject __RPC_FAR * This)		/* */

{
	((RECORDOBJ *)This)->refcount++;
	return (((RECORDOBJ *)This)->refcount);
}
/******************************************************************************/
static STDMETHODIMP_(LONG) DTA_Release(IDataObject __RPC_FAR * This)		/* */

{
	if (--((RECORDOBJ *)This)->refcount)
		;					/* free anything we need to */
	return (((RECORDOBJ *)This)->refcount);
}
/******************************************************************************/
static STDMETHODIMP DTA_GetData(IDataObject __RPC_FAR * This, 
    /* [unique][in] */ FORMATETC __RPC_FAR *pformatetcIn,
    /* [out] */ STGMEDIUM __RPC_FAR *pmedium)		/* */

{
	/* !! eventually need to fix transfers by file (TYMED_FILE) */

	if (!(pformatetcIn->dwAspect&DVASPECT_CONTENT))	/* if want other than content */
		return (DV_E_DVASPECT);
	if (!(pformatetcIn->tymed&TYMED_HGLOBAL))	/* if want unsupported medium */
		return (DV_E_TYMED);
	if (pformatetcIn->cfFormat == g_rtfref)
		return (rtfobj((RECORDOBJ *)This,pmedium));
	if (pformatetcIn->cfFormat == CF_TEXT)
		return (textobj((RECORDOBJ *)This,pmedium));
	if (pformatetcIn->cfFormat == g_recref)
		return (recobj((RECORDOBJ *)This,pmedium));
	return (E_FAIL);
}
/******************************************************************************/
static STDMETHODIMP DTA_GetDataHere(IDataObject __RPC_FAR * This, 
    /* [unique][in] */ FORMATETC __RPC_FAR *pformatetc,
    /* [out][in] */ STGMEDIUM __RPC_FAR *pmedium)		/* */

{
	return (E_NOTIMPL);
}
/******************************************************************************/
static STDMETHODIMP DTA_QueryGetData(IDataObject __RPC_FAR * This, 
    /* [unique][in] */ FORMATETC __RPC_FAR *pformatetc)		/* */

{
	if (!(pformatetc->dwAspect&DVASPECT_CONTENT))	/* if want other than content */
		return (DV_E_DVASPECT);
	if (!(pformatetc->tymed&TYMED_HGLOBAL))	/* if want unsupported medium */
		return (DV_E_TYMED);
	if (pformatetc->cfFormat == CF_TEXT			/* if a do-able format */
		|| pformatetc->cfFormat == g_rtfref
		|| pformatetc->cfFormat == g_recref)
		return (S_OK);
	return (E_FAIL); 	
}
/******************************************************************************/
static STDMETHODIMP DTA_GetCanonicalFormatEtc(IDataObject __RPC_FAR * This, 
    /* [unique][in] */ FORMATETC __RPC_FAR *pformatectIn,
    /* [out] */ FORMATETC __RPC_FAR *pformatetcOut)		/* */

{
	if (pformatetcOut == NULL)
		return (E_INVALIDARG);
	*pformatetcOut = *pformatectIn;
	pformatetcOut->ptd = NULL;
	return (DATA_S_SAMEFORMATETC);
}
/******************************************************************************/
static STDMETHODIMP DTA_SetData(IDataObject __RPC_FAR * This, 
    /* [unique][in] */ FORMATETC __RPC_FAR *pformatetc,
    /* [unique][in] */ STGMEDIUM __RPC_FAR *pmedium,
    /* [in] */ BOOL fRelease)		/* */

{
	return (E_NOTIMPL);
}
/******************************************************************************/
static STDMETHODIMP DTA_EnumFormatEtc(IDataObject __RPC_FAR * This, 
    /* [in] */ DWORD dwDirection,
    /* [out] */ IEnumFORMATETC __RPC_FAR *__RPC_FAR *ppenumFormatEtc)		/* */

{
	return OleRegEnumFormatEtc(&cr_clsID,dwDirection,ppenumFormatEtc);
}
/******************************************************************************/
static STDMETHODIMP DTA_DAdvise(IDataObject __RPC_FAR * This, 
    /* [in] */ FORMATETC __RPC_FAR *pformatetc,
    /* [in] */ DWORD advf,
    /* [unique][in] */ IAdviseSink __RPC_FAR *pAdvSink,
    /* [out] */ DWORD __RPC_FAR *pdwConnection)		/* */

{
	return (OLE_E_ADVISENOTSUPPORTED);
}
/******************************************************************************/
static STDMETHODIMP DTA_DUnadvise(IDataObject __RPC_FAR * This, 
    /* [in] */ DWORD dwConnection)		/* */

{
	return (OLE_E_ADVISENOTSUPPORTED);
}
/******************************************************************************/
static STDMETHODIMP DTA_EnumDAdvise(IDataObject __RPC_FAR * This, 
    /* [out] */ IEnumSTATDATA __RPC_FAR *__RPC_FAR *ppenumAdvise)		/* */

{
	return (OLE_E_ADVISENOTSUPPORTED);
}
/****************************************************************************/
static STDMETHODIMP DSC_QueryInterface(IDropSource __RPC_FAR * This,
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject)

{
	SCODE sc = S_OK;

	if (IsEqualIID(riid, &IID_IUnknown))
		DTA_AddRef(*ppvObject = This);
	else if (IsEqualIID(riid, &IID_IDropSource))
		DTA_AddRef(*ppvObject = This);
	else	{
		sc = E_NOINTERFACE;
		*ppvObject = NULL;
	}
	return (sc);
}
/****************************************************************************/
static STDMETHODIMP_(ULONG) DSC_AddRef(IDropSource __RPC_FAR * This)

{
	((DSRCOBJ *)This)->refcount++;
	return (((DSRCOBJ *)This)->refcount);
}
/****************************************************************************/
static STDMETHODIMP_(ULONG) DSC_Release(IDropSource __RPC_FAR * This)

{
	if (--((DSRCOBJ *)This)->refcount)
		;					/* free anything we need to */
	return (((DSRCOBJ *)This)->refcount);
}
/****************************************************************************/
static STDMETHODIMP DSC_QueryContinueDrag(IDropSource __RPC_FAR * This,
    /* [in] */ BOOL fEscapePressed,
    /* [in] */ DWORD grfKeyState)

{
	if (fEscapePressed)
		return (DRAGDROP_S_CANCEL);
	if (!(grfKeyState&MK_LBUTTON))
		return (DRAGDROP_S_DROP);
	return(NOERROR);
}
/****************************************************************************/
static STDMETHODIMP DSC_GiveFeedback(IDropSource __RPC_FAR * This,
    /* [in] */ DWORD dwEffect)

{
	return (DRAGDROP_S_USEDEFAULTCURSORS);
}
/****************************************************************************/
static STDMETHODIMP DTG_QueryInterface(IDropTarget __RPC_FAR * This,
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject)

{
	SCODE sc = S_OK;

	if (IsEqualIID(riid, &IID_IUnknown))
		DTA_AddRef(*ppvObject = This);
	else if (IsEqualIID(riid, &IID_IDropTarget))
		DTA_AddRef(*ppvObject = This);
	else	{
		sc = E_NOINTERFACE;
		*ppvObject = NULL;
	}
	return (sc);
}
/****************************************************************************/
static STDMETHODIMP_(ULONG) DTG_AddRef(IDropTarget __RPC_FAR * This)

{
	((DTRGOBJ *)This)->refcount++;
	return (((DTRGOBJ *)This)->refcount);
}
/****************************************************************************/
static STDMETHODIMP_(ULONG) DTG_Release(IDropTarget __RPC_FAR * This)

{
	if (--((DTRGOBJ *)This)->refcount)
		;					/* free anything we need to */
	return (((DTRGOBJ *)This)->refcount);
}
/****************************************************************************/
static STDMETHODIMP DTG_DragEnter(IDropTarget __RPC_FAR * This,
    /* [unique][in] */ IDataObject __RPC_FAR *pDataObj,
    /* [in] */ DWORD grfKeyState,
    /* [in] */ POINTL pt,
    /* [out][in] */ DWORD __RPC_FAR *pdwEffect)

{
	SCODE sc;

	sc = recole_ispastable(pDataObj);	/* check if records */
	if (SUCCEEDED(sc) && !WX(((DTRGOBJ *)This)->hwnd,owner)->rwind && !WX(((DTRGOBJ *)This)->hwnd,owner)->mf.readonly)	{
		if (grfKeyState&MK_CONTROL)
			*pdwEffect = DROPEFFECT_COPY;
		else
			*pdwEffect = DROPEFFECT_MOVE;
		((DTRGOBJ *)This)->accept = TRUE;
	}
	else	{
		*pdwEffect = DROPEFFECT_NONE;
		((DTRGOBJ *)This)->accept = FALSE;
	}
	return (NOERROR);
}
/****************************************************************************/
static STDMETHODIMP DTG_DragOver(IDropTarget __RPC_FAR * This,
    /* [in] */ DWORD grfKeyState,
    /* [in] */ POINTL pt,
    /* [out][in] */ DWORD __RPC_FAR *pdwEffect)

{
	if (((DTRGOBJ *)This)->accept && !LX(((DTRGOBJ *)This)->hwnd,dragsource))	{	/* if acceptable data */
#if 0
		if (grfKeyState&MK_CONTROL)
			*pdwEffect = DROPEFFECT_COPY;
		else
			*pdwEffect = DROPEFFECT_MOVE;
#else
			*pdwEffect = DROPEFFECT_COPY;	/* for the moment we always copy */
#endif
	}
	else
		*pdwEffect = DROPEFFECT_NONE;
	return (NOERROR);
}
/****************************************************************************/
static STDMETHODIMP DTG_DragLeave(IDropTarget __RPC_FAR * This)

{
	return(NOERROR);
}
/****************************************************************************/
static STDMETHODIMP DTG_Drop(IDropTarget __RPC_FAR * This,
    /* [unique][in] */ IDataObject __RPC_FAR *pDataObj,
    /* [in] */ DWORD grfKeyState,
    /* [in] */ POINTL pt,
    /* [out][in] */ DWORD __RPC_FAR *pdwEffect)

{
	if (LX(((DTRGOBJ *)This)->hwnd,dragsource))	/* if we're the source of the drag */
		return(E_FAIL);
	if (recole_paste(((DTRGOBJ *)This)->hwnd,pDataObj))	/* if can paste */
		return (NOERROR);
	return (E_FAIL);
}
/******************************************************************************/
static HRESULT recobj(RECORDOBJ * dop, STGMEDIUM __RPC_FAR *pmedium)	/* copies records as data */

{
	RECCOPY * rcs, *rcd;

	if (rcs = GlobalLock(dop->gh))	{	/* handle to saved data */
		if (pmedium->hGlobal = GlobalAlloc(GMEM_MOVEABLE|GMEM_DDESHARE,rcs->textoffset))	{	/* if can get memory */
			rcd = GlobalLock(pmedium->hGlobal);
			memcpy(rcd,rcs,rcs->textoffset);
			GlobalUnlock(pmedium->hGlobal);
			GlobalUnlock(dop->gh);
			pmedium->tymed = TYMED_HGLOBAL;
			pmedium->pUnkForRelease = NULL;
			return (S_OK);
		}
		GlobalUnlock(dop->gh);
	}
	return (E_OUTOFMEMORY);
}
/******************************************************************************/
static HRESULT textobj(RECORDOBJ * dop, STGMEDIUM __RPC_FAR *pmedium)	/* copies records as text */

{
	RECCOPY * rcs, *rcd;

	if (rcs = GlobalLock(dop->gh))	{	/* handle to saved data */
		if (pmedium->hGlobal = GlobalAlloc(GMEM_MOVEABLE|GMEM_DDESHARE,rcs->textsize))	{	/* if can get memory */
			rcd = GlobalLock(pmedium->hGlobal);
			memcpy(rcd,(char *)rcs+rcs->textoffset,rcs->textsize);
			GlobalUnlock(pmedium->hGlobal);
			GlobalUnlock(dop->gh);
			pmedium->tymed = TYMED_HGLOBAL;
			pmedium->pUnkForRelease = NULL;
			return (S_OK);
		}
		GlobalUnlock(dop->gh);
	}
	return (E_OUTOFMEMORY);
}
/******************************************************************************/
static HRESULT rtfobj(RECORDOBJ * dop, STGMEDIUM __RPC_FAR *pmedium)	/* copies data */

{
	RECCOPY * rcs, *rcd;

	if (rcs = GlobalLock(dop->gh))	{	/* handle to saved data */
		if (pmedium->hGlobal = GlobalAlloc(GMEM_MOVEABLE|GMEM_DDESHARE,rcs->rtfsize))	{	/* if can get memory */
			rcd = GlobalLock(pmedium->hGlobal);
			memcpy(rcd,(char *)rcs+rcs->rtfoffset,rcs->rtfsize);
			GlobalUnlock(pmedium->hGlobal);
			GlobalUnlock(dop->gh);
			pmedium->tymed = TYMED_HGLOBAL;
			pmedium->pUnkForRelease = NULL;
			return (S_OK);
		}
		GlobalUnlock(dop->gh);
	}
	return (E_OUTOFMEMORY);
}
