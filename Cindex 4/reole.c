#include "stdafx.h"
#include "reole.h"
#include "modify.h"

#define RPP ((RECALLBACK *)This)

static STDMETHODIMP REC_QueryInterface (THIS_ REFIID riid, LPVOID FAR * lplpObj);
static STDMETHODIMP_(LONG) REC_AddRef  (THIS);
static STDMETHODIMP_(LONG) REC_Release (THIS);

    // *** IRichEditOleCallback methods ***
static STDMETHODIMP REC_GetNewStorage (THIS_ LPSTORAGE FAR * lplpstg);
static STDMETHODIMP REC_GetInPlaceContext (THIS_ LPOLEINPLACEFRAME FAR * lplpFrame,
								  LPOLEINPLACEUIWINDOW FAR * lplpDoc,
								  LPOLEINPLACEFRAMEINFO lpFrameInfo);
static STDMETHODIMP REC_ShowContainerUI  (THIS_ BOOL fShow);
static STDMETHODIMP REC_QueryInsertObject (THIS_ LPCLSID lpclsid, LPSTORAGE lpstg,
									LONG cp);
static STDMETHODIMP REC_DeleteObject (THIS_ LPOLEOBJECT lpoleobj);
static STDMETHODIMP REC_QueryAcceptData (THIS_ LPDATAOBJECT lpdataobj,
								CLIPFORMAT FAR * lpcfFormat, DWORD reco,
								BOOL fReally, HGLOBAL hMetaPict);
static STDMETHODIMP REC_ContextSensitiveHelp (THIS_ BOOL fEnterMode);
static STDMETHODIMP REC_GetClipboardData (THIS_ CHARRANGE FAR * lpchrg, DWORD reco,
								LPDATAOBJECT FAR * lplpdataobj);
static STDMETHODIMP REC_GetDragDropEffect (THIS_ BOOL fDrag, DWORD grfKeyState,
									LPDWORD pdwEffect);
static STDMETHODIMP REC_GetContextMenu (THIS_ WORD seltype, LPOLEOBJECT lpoleobj,
									CHARRANGE FAR * lpchrg,
									HMENU FAR * lphmenu);

static IRichEditOleCallbackVtbl recvtable = {
	REC_QueryInterface,
	REC_AddRef,
	REC_Release,
	REC_GetNewStorage,
	REC_GetInPlaceContext,
	REC_ShowContainerUI,
	REC_QueryInsertObject,
	REC_DeleteObject,
	REC_QueryAcceptData,
	REC_ContextSensitiveHelp,
	REC_GetClipboardData,
	REC_GetDragDropEffect,
	REC_GetContextMenu
};

RECALLBACK rw_callback = {
	&recvtable		/* callback for rich edit window */
					/* other members unassigned */
};

/******************************************************************************/
static STDMETHODIMP REC_QueryInterface(THIS_ REFIID riid, LPVOID FAR * lplpObj)		/* */

{
	SCODE sc = S_OK;

	if (IsEqualIID(riid, &IID_IUnknown))
		REC_AddRef(*lplpObj = This);
	else if (IsEqualIID(riid, &IID_IRichEditOleCallback))
		REC_AddRef(*lplpObj = This);
	else	{
		sc = E_NOINTERFACE;
		*lplpObj = NULL;
	}
	return (sc);
}
/******************************************************************************/
static STDMETHODIMP_(LONG) REC_AddRef(THIS)		/* */

{
	return (1);		/* we don't do anything yet */
}
/******************************************************************************/
static STDMETHODIMP_(LONG) REC_Release(THIS)		/* */

{
	return (0);		/* we don't do anything yet */
}
/******************************************************************************/
static STDMETHODIMP REC_GetNewStorage(THIS_ LPSTORAGE FAR * lplpstg)		/* */

{
	return (E_NOTIMPL);
}
/******************************************************************************/
static STDMETHODIMP REC_GetInPlaceContext(THIS_ LPOLEINPLACEFRAME FAR * lplpFrame,
								  LPOLEINPLACEUIWINDOW FAR * lplpDoc,
								  LPOLEINPLACEFRAMEINFO lpFrameInfo)		/* */

{
	return (E_NOTIMPL);
}
/******************************************************************************/
static STDMETHODIMP REC_ShowContainerUI(THIS_ BOOL fShow)		/* */

{
	return (E_NOTIMPL);
}
/******************************************************************************/
static STDMETHODIMP REC_QueryInsertObject(THIS_ LPCLSID lpclsid, LPSTORAGE lpstg,LONG cp)		/* */

{
	return (E_NOTIMPL);
}
/******************************************************************************/
static STDMETHODIMP REC_DeleteObject(THIS_ LPOLEOBJECT lpoleobj)		/* */

{
	return (S_OK);
}
#if 0
/******************************************************************************/
static STDMETHODIMP REC_QueryAcceptData(THIS_ LPDATAOBJECT lpdataobj,
								CLIPFORMAT FAR * lpcfFormat, DWORD reco,
								BOOL fReally, HGLOBAL hMetaPict)		/* */

{
	FORMATETC fm;
	STGMEDIUM sm;
	SCODE sc;
	TCHAR * string,*sptr;
	int err;

	fm.ptd = NULL;
	fm.dwAspect = DVASPECT_CONTENT;
	fm.lindex = -1;
	fm.tymed = TYMED_HGLOBAL;
	fm.cfFormat = CF_UNICODETEXT;
//	fm.cfFormat = g_rtfref;
	sc = lpdataobj->lpVtbl->GetData(lpdataobj,&fm,&sm);	/* check if we can paste/drop text */
	if (SUCCEEDED(sc))	{
//		RECALLBACK *XX = RPP;
		if (sm.tymed == TYMED_HGLOBAL)	{	/* if have handle */
			sptr = string = GlobalLock(sm.hGlobal);	/* get handle & lock */
			RPP->length = nstrlen(sptr);
			for (RPP->bcount = 0; sptr = nstrchr(sptr,'\n'); RPP->bcount++, sptr++)
				;		/* count field breaks in import text */
			if (fReally)	{/* if really doing it */
				if (GetAsyncKeyState(VK_SHIFT) < 0)	// if want text only paste
					*lpcfFormat = CF_UNICODETEXT;		// (default is best available)
				err = (RPP->droptest)((RECALLBACK *)This,reco,string);	// test paste/drop
			}
			else
				err = FALSE;
			GlobalUnlock(sm.hGlobal);
			ReleaseStgMedium(&sm);
		}
		return (err ? E_FAIL : S_OK);
	}
	return (E_FAIL);
}
#else
/******************************************************************************/
static STDMETHODIMP REC_QueryAcceptData(THIS_ LPDATAOBJECT lpdataobj,
								CLIPFORMAT FAR * lpcfFormat, DWORD reco,
								BOOL fReally, HGLOBAL hMetaPict)		/* */

{
	FORMATETC fm;
	STGMEDIUM sm;
	SCODE sc;
	int err;

	fm.ptd = NULL;
	fm.dwAspect = DVASPECT_CONTENT;
	fm.lindex = -1;
	fm.tymed = TYMED_HGLOBAL;
	fm.cfFormat = CF_UNICODETEXT;
//	fm.cfFormat = g_rtfref;
	sc = lpdataobj->lpVtbl->GetData(lpdataobj,&fm,&sm);	/* check if we can paste/drop unicode text */
	if (SUCCEEDED(sc))	{
		if (sm.tymed == TYMED_HGLOBAL)	{	/* if have handle */
			TCHAR * string,*sptr;
			sptr = string = GlobalLock(sm.hGlobal);	/* get handle & lock */
			RPP->length = GlobalSize(sm.hGlobal)/2;	// number of characters
			for (RPP->bcount = 0; sptr < string + RPP->length; sptr++)	{
				if (*sptr == '\n')		// if field break
					RPP->bcount++;
			}
			if (fReally)	{	/* if really doing it */
//				// key is down when GetAsyncKeyState(key) < 0
//				BOOL wantplain = GetAsyncKeyState(VK_LMENU) >= 0 && (GetAsyncKeyState(VK_LSHIFT) <0 && g_prefs.gen.pastemode != PASTEMODE_PLAIN || GetAsyncKeyState(VK_LSHIFT) >=0 && g_prefs.gen.pastemode == PASTEMODE_PLAIN);
//				if (wantplain)	// if want text only paste
//					*lpcfFormat = CF_UNICODETEXT;		// (default is best available)
				err = (RPP->droptest)((RECALLBACK *)This,reco,string);	// test paste/drop
				if (RPP->pastemode == PASTEMODE_PLAIN)
					*lpcfFormat = CF_UNICODETEXT;		// (default is best available)
			}
			else
				err = FALSE;
			GlobalUnlock(sm.hGlobal);
			ReleaseStgMedium(&sm);
		}
		return (err ? E_FAIL : S_OK);
	}
	else {
		fm.cfFormat = CF_TEXT;
		sc = lpdataobj->lpVtbl->GetData(lpdataobj,&fm,&sm);	/* check if we can paste/drop ANSI text */
		if (SUCCEEDED(sc))	{
			if (sm.tymed == TYMED_HGLOBAL)	{	/* if have handle */
				char *string;
				TCHAR ustr[4096], *sptr;

				string = GlobalLock(sm.hGlobal);	/* get handle & lock */
				RPP->length = GlobalSize(sm.hGlobal);
				MultiByteToWideChar(CP_ACP,0,string,RPP->length,ustr,4096);
				for (RPP->bcount = 0, sptr = ustr; sptr < ustr + RPP->length; sptr++)	{
					if (*sptr == '\n')		// if field break
						RPP->bcount++;
				}
				if (fReally)	{	/* if really doing it */
					if (GetAsyncKeyState(VK_SHIFT) < 0)	// if want text only paste
						*lpcfFormat = CF_TEXT;		// (default is best available)
					err = (RPP->droptest)((RECALLBACK *)This,reco,ustr);	// test paste/drop
				}
				else
					err = FALSE;
				GlobalUnlock(sm.hGlobal);
				ReleaseStgMedium(&sm);
			}
			return (err ? E_FAIL : S_OK);
		}
	}
	return (E_FAIL);
}
#endif
/******************************************************************************/
static STDMETHODIMP REC_ContextSensitiveHelp(THIS_ BOOL fEnterMode)		/* */

{
	return (E_NOTIMPL);
}
/******************************************************************************/
static STDMETHODIMP REC_GetClipboardData(THIS_ CHARRANGE FAR * lpchrg, DWORD reco,
									LPDATAOBJECT FAR * lplpdataobj)		/* */

{
	return (E_NOTIMPL);
}
/******************************************************************************/
static STDMETHODIMP REC_GetDragDropEffect(THIS_ BOOL fDrag, DWORD grfKeyState,
									LPDWORD pdwEffect)		/* */

{
	if (!(grfKeyState&MK_LBUTTON) && RPP->dropsite(RPP))	/* if dropsite bad */
		*pdwEffect &= ~(DROPEFFECT_COPY|DROPEFFECT_MOVE|DROPEFFECT_LINK);
	return (S_OK);
}
/******************************************************************************/
static STDMETHODIMP REC_GetContextMenu(THIS_ WORD seltype, LPOLEOBJECT lpoleobj,
									CHARRANGE FAR * lpchrg,
									HMENU FAR * lphmenu)		/* */

{
	if (RPP->contextmenu)	{	/* if have context menu */
		if (*lphmenu = (HMENU)RPP->contextmenu(RPP))
			return (S_OK);
	}
	return (E_FAIL);
}