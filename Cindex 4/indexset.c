#include "stdafx.h"
#include "errors.h"
#include "search.h"
#include "sort.h"
#include "files.h"
#include "indexset.h"
#include "util.h"
#include "regex.h"
#include "viewset.h"
#include "records.h"
#include "index.h"

static const DWORD wh_recordstruct[] = {
	IDC_RECORDSTRUCT_TAB, HIDC_RECORDSTRUCT_TAB,
	IDC_RECORDSTRUCT_MAX,HIDC_RECORDSTRUCT_MAX,
	IDC_RECORDSTRUCT_CUR,HIDC_RECORDSTRUCT_CUR,
	IDC_RECORDSTRUCT_MAXF,HIDC_RECORDSTRUCT_MAXF,
	IDC_RECORDSTRUCT_MINF,HIDC_RECORDSTRUCT_MINF,
	IDC_RECORDSTRUCT_REQUIRED,HIDC_RECORDSTRUCT_REQUIRED,
	IDC_RECORDSTRUCT_FNAME,HIDC_RECORDSTRUCT_FNAME,
	IDC_RECORDSTRUCT_CMAX,HIDC_RECORDSTRUCT_CMAX,
	IDC_RECORDSTRUCT_CMIN,HIDC_RECORDSTRUCT_CMIN,
	IDC_RECORDSTRUCT_PAT,HIDC_RECORDSTRUCT_PAT,
	IDC_RECORDSTRUCT_CURLEN,HIDC_RECORDSTRUCT_CURLEN,
	0,0
};

static const DWORD wh_recordprops[] = {
	IDC_REFSYNTAX_CGENERAL,HIDC_REFSYNTAX_CGENERAL,
	IDC_REFSYNTAX_CLEAD,HIDC_REFSYNTAX_CLEAD,
	IDC_REFSYNTAX_CSEP,HIDC_REFSYNTAX_CSEP,
	IDC_REFSYNTAX_CSUBHEAD,HIDC_REFSYNTAX_CSUBHEAD,
	IDC_REFSYNTAX_MAXRANGE,HIDC_REFSYNTAX_MAXRANGE,
	IDC_REFSYNTAX_MAXVAL,HIDC_REFSYNTAX_MAXVAL,
	IDC_REFSYNTAX_PCONN,HIDC_REFSYNTAX_PCONN,
	IDC_REFSYNTAX_PSEP,HIDC_REFSYNTAX_PSEP,
	0,0
};

static const DWORD wh_fwordid[] = {
	IDC_FW_LIST, HIDC_FW_LIST,
	0,0
};

struct wrapper {
	INDEXPARAMS idx;
	COUNTPARAMS cs;
	int err, iflag, minlength;
};

#define MINLENGTH 20
#define MINDEPTH 2

static INT_PTR CALLBACK setstructproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);	/* handles About.. box */
static void setrequired(HWND hwnd, INDEXPARAMS * idxptr); // sets state of required field
static void settabs(HWND hwnd, struct wrapper * wp);		/* sets up tabs for structure dialog */
static void settabfields(HWND hwnd, HWND th, INDEXPARAMS * idxptr, short *flen,int flag);	/* sets field items */
static BOOL gettabfields(HWND hwnd, HWND th,INDEXPARAMS * idxptr);	/* gets field items */
static INT_PTR CALLBACK refproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK fwproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

/**********************************************************************************/	
short is_setstruct(HWND hwnd, INDEXPARAMS *idxptr)		/* sets up new record struct */

{
	struct wrapper ws;
	INDEX * FF;
	int oldmaxfieldcount;

	memset(&ws,0,sizeof(ws));
	ws.cs.deepest = MINDEPTH;		/* min value for max and min fields */	
	if (hwnd)	{			/* if an index */
		FF = WX(hwnd,owner);
		ws.idx = FF->head.indexpars;
		if (FF->head.rtot)	{		/* if have any entries */
			GROUPHANDLE tfile;

			ws.iflag = TRUE;		/* have an index */
			ws.cs.firstrec = 1;
			ws.cs.lastrec = ULONG_MAX;
			tfile = FF->curfile;	// save any group
			FF->curfile = NULL;		// disable any group
			search_count(FF, &ws.cs, SF_OFF);	/* do count of records to find longest */
			FF->curfile = tfile;	// restore any group;
		}
		oldmaxfieldcount = FF->head.indexpars.maxfields;
	}
	else	{	/* prefs, or new index options */
		ws.idx = *idxptr;
		oldmaxfieldcount = idxptr == &g_prefs.indexpars ? idxptr->maxfields : 0;
	}
	if (DialogBoxParam(g_hinst,MAKEINTRESOURCE(IDD_RECORDSTRUCTURE),GetActiveWindow(),setstructproc,(LPARAM)&ws) >0)	{
		if (hwnd)	{
			if (file_resizeindex(FF, ws.idx.recsize))	{	/* if index and can resize */ 
				int minfields = FF->head.indexpars.minfields;
				char requiredfieldchanged = FF->head.indexpars.required != ws.idx.required;	// note required change

				FF->head.indexpars = ws.idx;
				if (requiredfieldchanged)
					FF->head.indexpars.required ^= 1;	// do field number adjustments with old setting
				if (minfields != FF->head.indexpars.minfields)	{	// if changed min fields
					RECN count;
					for (count = 1;count <= FF->head.rtot;count++)	{	// for all records
						RECORD * recptr = getaddress(FF,count);	// bypass integrity check, which looks at minfields
						int fcount = rec_strip(FF,recptr->rtext);		// strip surplus

						if (fcount < FF->head.indexpars.minfields)		// if too few fields
							rec_pad(FF, recptr->rtext);
					}
				}
				if (requiredfieldchanged)	{
					FF->head.indexpars.required ^= 1;	// restore new setting
					sort_resort(FF);
				}
				adjustsortfieldorder(FF->head.sortpars.fieldorder, oldmaxfieldcount, FF->head.indexpars.maxfields);
				view_redisplay(FF,0,VD_CUR|VD_RESET);	/* force reset of display pars */
				index_markdirty(FF);
			}
		}
		else	{
			*idxptr = ws.idx;
			if (oldmaxfieldcount)	/* if setting prefs */
				adjustsortfieldorder(g_prefs.sortpars.fieldorder, oldmaxfieldcount,g_prefs.indexpars.maxfields);
		}
		return (TRUE);
	}
	return (FALSE);
}
/******************************************************************************/
static INT_PTR CALLBACK setstructproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)	/* handles About.. box */

{
	INDEXPARAMS * idxptr;
	int count;
	short chosenlength;
	struct wrapper * wp;
	HWND cbh1, cbh2;
	char tbuff[STSTRING];

	wp = getdata(hwnd);		/* get param struct (NULL until initialization) */
	idxptr = &wp->idx;
	switch (msg)	{
		case WM_INITDIALOG:
			if (wp = setdata(hwnd,(void *)lParam))	{	/* set data */
				idxptr = &wp->idx;
				if (wp->iflag)		{/* if working on index (restricted fields) */
					setint(hwnd,IDC_RECORDSTRUCT_CUR,wp->cs.longest);
					disableitem(hwnd,IDC_RECORDSTRUCT_CMIN);
				}
				else	{
					hideitem(hwnd,IDC_RECORDSTRUCT_CUR);
					hideitem(hwnd,IDC_RECORDSTRUCT_CURLEN);
				}
				wp->minlength = wp->cs.longest > MINLENGTH ? wp->cs.longest : MINLENGTH;	// save minimum permissible record length
				cbh1 = GetDlgItem(hwnd,IDC_RECORDSTRUCT_MINF);
				for (count = MINDEPTH; count < FIELDLIM+1; count++)	{	// for permissible fields
					_itoa(count,tbuff,10);
					ComboBox_AddItemData(cbh1,toNative(tbuff));
				}
				ComboBox_SetCurSel(cbh1,idxptr->minfields-2);	// select min fields used
				setrequired(hwnd,idxptr);	// fix required field
				checkitem(hwnd,IDC_RECORDSTRUCT_REQUIRED,idxptr->required);
				setint(hwnd,IDC_RECORDSTRUCT_MAX,idxptr->recsize);
				cbh2 = GetDlgItem(hwnd,IDC_RECORDSTRUCT_MAXF);
				for (count = wp->cs.deepest; count < FIELDLIM+1; count++)	{	/* build field menus */
					_itoa(count,tbuff,10);
					ComboBox_AddItemData(cbh2,toNative(tbuff));
				}
				ComboBox_SetCurSel(cbh2,idxptr->maxfields-wp->cs.deepest);
				settabs(hwnd,wp);
				Edit_LimitText(GetDlgItem(hwnd,IDC_RECORDSTRUCT_FNAME),FNAMELEN-1);	/* set field limits */
				Edit_LimitText(GetDlgItem(hwnd,IDC_RECORDSTRUCT_PAT),PATTERNLEN-1);
				SetFocus(GetDlgItem(hwnd,IDC_RECORDSTRUCT_MAX));
				centerwindow(hwnd,1);
			}
			return FALSE;
		case WM_COMMAND:
			if (HIWORD(wParam) == EN_UPDATE)	{
				WORD id = LOWORD(wParam);
				int length = 0;

				if (id == IDC_RECORDSTRUCT_FNAME)
					length = FNAMELEN;
				else if (id == IDC_RECORDSTRUCT_PAT)
					length = PATTERNLEN;
				checktextfield((HWND)lParam,length);
				return TRUE;
			}
			switch (LOWORD(wParam))	{
				case IDOK:
					getshort(hwnd,IDC_RECORDSTRUCT_MAX,&chosenlength);
					if (chosenlength < wp->minlength)	{
						if (sendwarning(WARN_SHORTRECORD,wp->minlength))
							chosenlength = wp->minlength;
						else
							return TRUE;
					}
					if (chosenlength > MAXREC)	{
						senditemerr(hwnd,IDC_RECORDSTRUCT_MAX);
						return TRUE;
					}
//					if (chosenlength&1)	// if odd # chars
//						chosenlength++;	// increment to even
					idxptr->recsize = (chosenlength + 3)&~3;	// record size has to be multiple of 4 bytes
					if (!gettabfields(hwnd,GetDlgItem(hwnd,IDC_RECORDSTRUCT_TAB),idxptr))
						return (TRUE);
				case IDCANCEL:
					EndDialog(hwnd,LOWORD(wParam) == IDOK ? TRUE : FALSE);
					break;
				case IDC_RECORDSTRUCT_MINF:
					if (HIWORD(wParam) == CBN_SELENDOK)	{
						int chosenmin = ComboBox_GetCurSel((HWND)lParam)+MINDEPTH;

						if (wp->cs.deeprec)	{	// if working on real index
							if (chosenmin-wp->cs.longestdepth > 0)	// if would need to expand longest record
								wp->minlength = chosenmin-wp->cs.longestdepth+wp->cs.longest;	// set new min record size
							else
								wp->minlength = wp->cs.longest;
						}
						if (gettabfields(hwnd,GetDlgItem(hwnd,IDC_RECORDSTRUCT_TAB),idxptr))	{
							idxptr->minfields = chosenmin;
							if (idxptr->minfields > idxptr->maxfields)	{
								idxptr->maxfields = idxptr->minfields;
								ComboBox_SetCurSel(GetDlgItem(hwnd,IDC_RECORDSTRUCT_MAXF),idxptr->maxfields-wp->cs.deepest);
							}
							setrequired(hwnd,idxptr);	// fix required field
							settabs(hwnd,wp);
						}
						else		/* error in tab field; restore old setting */
							ComboBox_SetCurSel((HWND)lParam,idxptr->minfields-wp->cs.deepest);
					}
					return (TRUE);
				case IDC_RECORDSTRUCT_MAXF:
					if (HIWORD(wParam) == CBN_SELENDOK)	{
						if (gettabfields(hwnd,GetDlgItem(hwnd,IDC_RECORDSTRUCT_TAB),idxptr))	{
							idxptr->maxfields = ComboBox_GetCurSel((HWND)lParam)+wp->cs.deepest;
							if (idxptr->minfields > idxptr->maxfields)	{
								idxptr->minfields = idxptr->maxfields;
								setrequired(hwnd,idxptr);	// fix required field
								ComboBox_SetCurSel(GetDlgItem(hwnd,IDC_RECORDSTRUCT_MINF),idxptr->minfields-wp->cs.deepest);
							}
							settabs(hwnd,wp);
						}
						else		/* error in tab field; restore old setting */
							ComboBox_SetCurSel((HWND)lParam,idxptr->maxfields-wp->cs.deepest);
					}
					return TRUE;
				case IDC_RECORDSTRUCT_REQUIRED:
					idxptr->required = isitemchecked(hwnd,IDC_RECORDSTRUCT_REQUIRED);
					settabs(hwnd,wp);		// fix tab titles
					return (TRUE);
			}
			break;
		case WM_NOTIFY:
			switch (LOWORD(wParam))	{
				case IDC_RECORDSTRUCT_TAB:
					if (((LPNMHDR)lParam)->code == TCN_SELCHANGING)	{
						if (!gettabfields(hwnd,((LPNMHDR)lParam)->hwndFrom,idxptr))
							SetWindowLongPtr(hwnd, DWLP_MSGRESULT,TRUE);
						return (TRUE);
					}
					else if (((LPNMHDR)lParam)->code == TCN_SELCHANGE)	{
						settabfields(hwnd,((LPNMHDR)lParam)->hwndFrom,idxptr,wp->cs.fieldlen,wp->iflag);
						return (TRUE);
					}
			}
			break;
		case WM_HELP:
			return (dodialoghelp(hwnd,TEXT("base\\Record Structure.htm"),(HELPINFO *)lParam,wh_recordstruct));
	}
	return (FALSE);
}
/**********************************************************************************/	
static void setrequired(HWND hwnd, INDEXPARAMS * idxptr) // sets state of required field

{
	if (idxptr->minfields == 2)	{	// if no room for special field
		disableitem(hwnd,IDC_RECORDSTRUCT_REQUIRED);
		checkitem(hwnd,IDC_RECORDSTRUCT_REQUIRED, FALSE);
		idxptr->required = FALSE;	// clear required
	}
	else
		enableitem(hwnd,IDC_RECORDSTRUCT_REQUIRED);
}
/**********************************************************************************/	
static void settabs(HWND hwnd, struct wrapper * wp)		/* sets up tabs for structure dialog */

{
	static TCHAR *fnames[] = {
		TEXT("First"), TEXT("Second"), TEXT("Third"), TEXT("Fourth"), TEXT("Fifth"), TEXT("Sixth"), TEXT("Seventh"), TEXT("Eighth"),
		TEXT("Ninth"), TEXT("Tenth"), TEXT("Eleventh"), TEXT("Twelfth"), TEXT("Thirteenth"), TEXT("Fourteenth"), TEXT("Fifteenth"),
		TEXT("Locator")
	};
	TC_ITEM tc;
	int count;
	HWND th;

	th = GetDlgItem(hwnd,IDC_RECORDSTRUCT_TAB);
	TabCtrl_DeleteAllItems(th);
	memset(&tc,0,sizeof(tc));
	tc.mask = TCIF_TEXT;
	for (count = 0; count < wp->idx.maxfields; count++)	{	/* build menu */
		if (count == wp->idx.maxfields-2 && wp->idx.required)	{	// if required field
			TCHAR fname[FNAMELEN+10];
			u_sprintf(fname,"%s *", fnames[count]);	// make special title
			tc.pszText = fname;
		}
		else
			tc.pszText = fnames[count < wp->idx.maxfields-1 ? count : PAGEINDEX];
		TabCtrl_InsertItem(th,count,&tc);
	}
	settabfields(hwnd,th,&wp->idx,wp->cs.fieldlen, wp->iflag);
}
/**********************************************************************************/	
static void settabfields(HWND hwnd, HWND th, INDEXPARAMS * idxptr, short *flen,int flag)	/* sets field items */

{
	int findex;

	findex = TabCtrl_GetCurSel(th);
	if (findex == idxptr->maxfields-1)
		findex = PAGEINDEX;
	setDItemText(hwnd,IDC_RECORDSTRUCT_FNAME,idxptr->field[findex].name);
//	setint(hwnd,IDC_RECORDSTRUCT_CMIN, flag ? flen[findex] : idxptr->field[findex].minlength);
	setint(hwnd,IDC_RECORDSTRUCT_CMIN, idxptr->field[findex].minlength);
	setint(hwnd,IDC_RECORDSTRUCT_CMAX, idxptr->field[findex].maxlength);
	setint(hwnd,IDC_RECORDSTRUCT_CURLEN, flen[findex]);
	setDItemText(hwnd,IDC_RECORDSTRUCT_PAT, idxptr->field[findex].matchtext);
}
/**********************************************************************************/	
static BOOL gettabfields(HWND hwnd, HWND th,INDEXPARAMS * idxptr)	/* gets field items */

{
	int findex;
	short tval;
	struct wrapper * wp;

	wp = getdata(hwnd);		/* get param struct (NULL until initialization) */
	findex = TabCtrl_GetCurSel(th);
	if (findex == idxptr->maxfields-1)
		findex = PAGEINDEX;
	getDItemText(hwnd,IDC_RECORDSTRUCT_FNAME,idxptr->field[findex].name,FNAMELEN);	// get name
	if (!wp->iflag)	{		/* if can load min chars */
		if (!getshort(hwnd,IDC_RECORDSTRUCT_CMIN,&tval) /* || wp->cs.fieldlen[findex] && tval > wp->cs.fieldlen[findex] */)	{	// if has content & would enlarge
			senditemerr(hwnd,IDC_RECORDSTRUCT_CMIN);
			return (FALSE);
		}
		idxptr->field[findex].minlength = tval;
	}
	if (!getshort(hwnd,IDC_RECORDSTRUCT_CMAX,&tval) || tval && tval < wp->cs.fieldlen[findex])	{	// if wd reduce length of field
		senditemerr(hwnd,IDC_RECORDSTRUCT_CMAX);
		return (FALSE);
	}
	idxptr->field[findex].maxlength = tval;
	if (idxptr->field[findex].maxlength && idxptr->field[findex].maxlength < idxptr->field[findex].minlength) {	/* if max < min || max < current max */
		senditemerr(hwnd,IDC_RECORDSTRUCT_CMAX);
		return (FALSE);
	}
	getDItemText(hwnd,IDC_RECORDSTRUCT_PAT, idxptr->field[findex].matchtext,PATTERNLEN);
//	if (*idxptr->field[findex].matchtext && re_comp(idxptr->field[findex].matchtext,tbuff) < 0)	{/* if bad expression */
	if (*idxptr->field[findex].matchtext && !regex_validexpression(idxptr->field[findex].matchtext,0))	{/* if bad expression */
		senditemerr(hwnd,IDC_RECORDSTRUCT_PAT);
		return (FALSE);
	}
	return (TRUE);
}
/*******************************************************************************/
void is_setrefsyntax(HWND hwnd)	// setes reference syntax

{
	REFPARAMS rgp;
	
	rgp = hwnd ? WX(hwnd,owner)->head.refpars : g_prefs.refpars;
	if (DialogBoxParam(g_hinst,MAKEINTRESOURCE(IDD_REFSYNTAX),hwnd,refproc,(LPARAM)&rgp))	{
		if (hwnd)	{
			view_redisplay(WX(hwnd,owner),0,VD_TOP);
			WX(hwnd,owner)->head.refpars = rgp;
			index_markdirty(WX(hwnd,owner));
		}
		else
			g_prefs.refpars = rgp;
	}
}
/******************************************************************************/
static INT_PTR CALLBACK refproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)

{
	REFPARAMS *rgp;
	char tarray[2];
	int err;
		
	rgp = getdata(hwnd);
	switch (msg)	{

		case WM_INITDIALOG:
			if (rgp = setdata(hwnd,(void *)lParam))	{	/* set data */
				tarray[1] = '\0';
				*tarray = rgp->psep;
				setDItemText(hwnd,IDC_REFSYNTAX_PSEP,tarray);
				*tarray = rgp->rsep;
				setDItemText(hwnd,IDC_REFSYNTAX_PCONN,tarray);
				setDItemText(hwnd,IDC_REFSYNTAX_MAXVAL,rgp->maxvalue);
				setint(hwnd,IDC_REFSYNTAX_MAXRANGE,rgp->maxspan);
				setDItemText(hwnd,IDC_REFSYNTAX_CLEAD,rgp->crosstart);
				*tarray = rgp->csep;
				setDItemText(hwnd,IDC_REFSYNTAX_CSEP,tarray);
				setDItemText(hwnd,IDC_REFSYNTAX_CGENERAL,rgp->crossexclude);
				Edit_LimitText(GetDlgItem(hwnd,IDC_REFSYNTAX_PSEP),1);
				Edit_LimitText(GetDlgItem(hwnd,IDC_REFSYNTAX_PCONN),1);
				Edit_LimitText(GetDlgItem(hwnd,IDC_REFSYNTAX_MAXVAL),FTSTRING-1);
				Edit_LimitText(GetDlgItem(hwnd,IDC_REFSYNTAX_CLEAD),STSTRING-1);
				Edit_LimitText(GetDlgItem(hwnd,IDC_REFSYNTAX_CSEP),1);
				Edit_LimitText(GetDlgItem(hwnd,IDC_REFSYNTAX_CGENERAL),FTSTRING-1);
				type_settextboxfont(GetDlgItem(hwnd,IDC_REFSYNTAX_PSEP));
				type_settextboxfont(GetDlgItem(hwnd,IDC_REFSYNTAX_PCONN));
				type_settextboxfont(GetDlgItem(hwnd,IDC_REFSYNTAX_CLEAD));
				type_settextboxfont(GetDlgItem(hwnd,IDC_REFSYNTAX_CSEP));
				type_settextboxfont(GetDlgItem(hwnd,IDC_REFSYNTAX_CGENERAL));
				checkitem(hwnd,IDC_REFSYNTAX_CSUBHEAD,rgp->clocatoronly);
				centerwindow(hwnd,1);
			}
			return FALSE;
		case WM_COMMAND:
			if (HIWORD(wParam) == EN_UPDATE)	{
				WORD id = LOWORD(wParam);
				int length = 0;

				if (id == IDC_REFSYNTAX_PSEP || id == IDC_REFSYNTAX_PCONN || id == IDC_REFSYNTAX_CSEP)
					length = 2;
				else if (id == IDC_REFSYNTAX_MAXVAL)
					length = FTSTRING;
				else if (id == IDC_REFSYNTAX_CLEAD || id == IDC_REFSYNTAX_CGENERAL)
					length = STSTRING;
				checktextfield((HWND)lParam,length);
				return TRUE;
			}
			switch (LOWORD(wParam))	{
				case IDOK:
					err = 0;
					if (getDItemText(hwnd,IDC_REFSYNTAX_PSEP,tarray,2))
						rgp->psep = *tarray;
					else
						err = IDC_REFSYNTAX_PSEP;
					if (getDItemText(hwnd,IDC_REFSYNTAX_PCONN,tarray,2))
						rgp->rsep = *tarray;
					else
						err = IDC_REFSYNTAX_PCONN;
					getDItemText(hwnd,IDC_REFSYNTAX_MAXVAL,rgp->maxvalue, FTSTRING);
					getlong(hwnd,IDC_REFSYNTAX_MAXRANGE,&rgp->maxspan);
					if (!getDItemText(hwnd,IDC_REFSYNTAX_CLEAD,rgp->crosstart, STSTRING))
						err = IDC_REFSYNTAX_CLEAD;
					if (getDItemText(hwnd,IDC_REFSYNTAX_CSEP,tarray,2))
						rgp->csep = *tarray;
					else
						err = IDC_REFSYNTAX_CSEP;
					getDItemText(hwnd,IDC_REFSYNTAX_CGENERAL,rgp->crossexclude, FTSTRING);
					if (err)	{
						senditemerr(hwnd,err);
						return (TRUE);
					}
					rgp->clocatoronly = isitemchecked(hwnd,IDC_REFSYNTAX_CSUBHEAD);
				case IDCANCEL:
					EndDialog(hwnd,LOWORD(wParam) == IDOK ? TRUE : FALSE);
					return TRUE;
			}
			break;
		case WM_HELP:
			return (dodialoghelp(hwnd,TEXT("base\\Pageref_structure.htm"),(HELPINFO *)lParam,wh_recordprops));
	}
	return FALSE;
}
/*******************************************************************************/
void is_flipwords(HWND hwnd)	// sets up flipped words

{
	char fw[STSTRING];
	INDEX * FF;

	if (hwnd)	{
		FF = WX(hwnd,owner);
		strcpy(fw,FF->head.flipwords);
	}
	else
		strcpy(fw,g_prefs.flipwords);
	if (DialogBoxParam(g_hinst,MAKEINTRESOURCE(IDD_FLIPWORDS),g_hwframe,fwproc,(LPARAM)fw))	{
		if (hwnd)	{
			strcpy(FF->head.flipwords,fw);
			if (FF->head.privpars.vmode == VM_FULL)
				view_redisplay(FF,0,VD_CUR|VD_RESET);	/* force reset of display pars */
			index_markdirty(FF);
		}
		else
			strcpy(g_prefs.flipwords,fw);
	}
}
/******************************************************************************/
static INT_PTR CALLBACK fwproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)

{
	char * fwp;

	fwp = getdata(hwnd);
	switch (msg)	{
		case WM_INITDIALOG:
			if (fwp = setdata(hwnd,(void *)lParam))	{	/* set data */
				type_settextboxfont(GetDlgItem(hwnd,IDC_FW_LIST));
				setDItemText(hwnd,IDC_FW_LIST,fwp);
				centerwindow(hwnd,1);	
			}
			return FALSE;
		case WM_COMMAND:
			if (HIWORD(wParam) == EN_UPDATE)	{
				WORD id = LOWORD(wParam);
				int length = 0;

				if (id == IDC_FW_LIST)
					length = STSTRING;
				checktextfield((HWND)lParam,length);
				return TRUE;
			}
			switch (LOWORD(wParam))	{
				case IDOK:
					getDItemText(hwnd,IDC_FW_LIST,fwp,STSTRING);
				case IDCANCEL:
					EndDialog(hwnd,LOWORD(wParam) == IDOK ? TRUE : FALSE);
					return TRUE;
			}
			break;
		case WM_HELP:
			return (dodialoghelp(hwnd,TEXT("_Adding and Editing\\Add_rearranging.htm"),(HELPINFO *)lParam,wh_fwordid));
	}
	return (FALSE);
}
