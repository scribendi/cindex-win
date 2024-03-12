#include "stdafx.h"

#include "export.h"
#include "import.h"
#include "formattedexport.h"
#include "rtfwriter.h"
#include "indesign.h"
#include "xpresswriter.h"
#include "textwriter.h"
#include "tagstuff.h"
#include "files.h"
#include "iconv.h"
#include "util.h"
#include "translate.h"
#include "records.h"
#include "errors.h"
#include "index.h"
#include "sort.h"
#include "strings.h"
#include "typestuff.h"
#include "viewset.h"
#include "commands.h"
#include "xmlwriter.h"
#include "imwriter.h"
#include "tagwriter.h"
#include "group.h"
#include "utime.h"
#include "registry.h"

static char * xmlheader =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<!DOCTYPE indexdata [\n\
<!ELEMENT indexdata (source, fonts, records) >\n\
<!ELEMENT source EMPTY >\n\
<!ATTLIST source\n\
	creator CDATA #REQUIRED\n\
	version CDATA #REQUIRED\n\
	time CDATA #REQUIRED >\n\
<!-- creator is code source, e.g., \"cindex\" -->\n\
<!-- time value is UTC in this format: 2011-03-03T03:41:14 -->\n\
<!ELEMENT fonts (font+) >\n\
<!ELEMENT font (fname, aname) >\n\
<!ATTLIST font\n\
	id CDATA #REQUIRED >\n\
<!ELEMENT fname (#PCDATA) >\n\
<!ELEMENT aname (#PCDATA) >\n\
<!ELEMENT records (record)* >\n\
<!ATTLIST records\n\
	type CDATA #IMPLIED\n\
	loc-separator CDATA #IMPLIED\n\
	loc-connector CDATA #IMPLIED\n\
	xref-separator CDATA #IMPLIED >\n\
<!-- type value is integer (Cindex: 1 for required last field) -->\n\
<!-- loc-separator value is single ASCII character (Cindex default: ,) -->\n\
<!-- loc-connector value is single ASCII character (Cindex default: -) -->\n\
<!-- xref-separator value is single ASCII character (Cindex default: ;) -->\n\
<!ELEMENT record (field+) >\n\
<!ATTLIST record\n\
	time CDATA #REQUIRED\n\
	id CDATA #IMPLIED\n\
	user CDATA #IMPLIED\n\
	label CDATA #IMPLIED\n\
	deleted (y | n) #IMPLIED\n\
	type CDATA #IMPLIED >\n\
<!-- time value is UTC in this format: 2008-08-02T16:27:44 -->\n\
<!-- id value is integer (unique within file) -->\n\
<!-- label value is integer -->\n\
<!-- type value can be \"generated\" (automatically generated) -->\n\
<!ELEMENT field (#PCDATA | text | literal | hide | sort)* >\n\
<!ATTLIST field\n\
	class CDATA #IMPLIED >\n\
<!-- class value can be \"locator\" -->\n\
<!ELEMENT text EMPTY >\n\
<!ATTLIST text\n\
	font CDATA #IMPLIED\n\
	color CDATA #IMPLIED\n\
	smallcaps ( y | n ) #IMPLIED\n\
	style ( b | i | u | bi | bu | iu | biu ) #IMPLIED\n\
	offset ( u | d ) #IMPLIED\n\
>\n\
<!-- font and color attribute values are integers in range 0-31 -->\n\
<!-- color attribute currently not used by Cindex -->\n\
<!ELEMENT literal EMPTY >\n\
<!-- literal: forces the succeeding character to be used in sort (Cindex: ~) -->\n\
<!ELEMENT hide (#PCDATA) >\n\
<!-- hide: contains text to be ignored in sorting (Cindex: < >) -->\n\
<!ELEMENT sort (#PCDATA) >\n\
<!-- sort: contains text to be used in sorting but not displayed (Cindex: { }) -->\n\
]>\n";

static const char *protected = "<>&'\"";
static const char *protectedstrings[] = {"&lt;","&gt;","&amp;","&apos;","&quot;"};

static TCHAR efilters[] =
TEXT("Cindex Index (*.ucdx)\0*.ucdx\0")\
TEXT("Template (*.utpl)\0*.utpl\0")\
TEXT("XML Records (*.ixml)\0*.ixml\0")\
TEXT("Cindex Archive (*.arc)\0*.arc\0")\
TEXT("Delimited Records (*.txt)\0*.txt\0")\
TEXT("Plain Text (*.txt)\0*.txt\0")\
TEXT("Rich Text Format (RTF) (*.rtf)\0*.rtf\0")\
TEXT("QuarkXPress (*.xtg)\0*.xtg\0")\
TEXT("InDesign (*.txt)\0*.txt\0")\
TEXT("Index-Manager (*.txt)\0*.txt");	// omit terminal '\0', because tagsets to be added

#define EFILTERSIZE (sizeof(efilters))

FCONTROLX * typesetters[] =	{	// indexed to efilters
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	&textcontrol,
	&rtfcontrol,
	&xpresscontrol,
	&indesigncontrol,
	&imcontrol,
	&xmlcontrol,
	&tagcontrol
};

struct styleset {
	STYLENAMES sn;
	char *nativenames;
};

static const DWORD wh_saveasid[] = {
	IDC_SAVEAS_OPTIONS, HIDC_SAVEAS_OPTIONS,
	0,0
};

static const DWORD wh_ufwriteid[] = {
	IDC_FIND_ALL, HIDC_FIND_ALL,
	IDC_FIND_SELECTED, HIDC_FIND_SELECTED,
	IDC_FIND_RANGE, HIDC_FIND_RANGE,
	IDC_FIND_RANGESTART, HIDC_FIND_RANGE,
	IDC_FIND_RANGEEND, HIDC_FIND_RANGE,
	IDC_WRITE_UNFORMAPPEND, HIDC_WRITE_UNFORMAPPEND,
	IDC_WRITE_UNFORMDEL, HIDC_WRITE_UNFORMDEL,
	IDC_WRITE_UNFORMEXTEND, HIDC_WRITE_UNFORMEXTEND,
	IDC_WRITE_UNFORMNUMORDER, HIDC_WRITE_UNFORMNUMORDER,
	IDC_WRITE_UNFORMMINFIELD, HIDC_WRITE_UNFORMMINFIELD,
	IDC_WRITEUNFORM_UTF8, HIDC_WRITEUNFORM_UTF8,
	IDC_WRITEUNFORM_ANSI, HIDC_WRITEUNFORM_ANSI,
	0,0
};

static const DWORD wh_fwriteid[] = {
	IDC_FORMWRITE_ALL, HIDC_FORMWRITE_ALL,
	IDC_FORMWRITE_PAGE, HIDC_FORMWRITE_PAGE,
	IDC_FORMWRITE_PSTART, HIDC_FORMWRITE_PAGE,
	IDC_FORMWRITE_PEND, HIDC_FORMWRITE_PAGE,
	IDC_FORMWRITE_SELECT, HIDC_FORMWRITE_SELECT,
	IDC_FORMWRITE_RECORDS, HIDC_FORMWRITE_RECORDS,
	IDC_FORMWRITE_RSTART, HIDC_FORMWRITE_RECORDS,
	IDC_FORMWRITE_REND, HIDC_FORMWRITE_RECORDS,
	IDC_FORMWRITE_USESTYLE, HIDC_FORMWRITE_USESTYLE,
	IDC_FORMWRITE_USETAB, HIDC_FORMWRITE_USETAB,
	IDC_FORMWRITE_USEOTHER, HIDC_FORMWRITE_USEOTHER,
	IDC_FORMWRITE_USEOTHERTEXT, HIDC_FORMWRITE_USEOTHER,
	0,0
};

static iconv_t converter;
static TCHAR e_path[MAX_PATH];	/* initial output directory */
static char regKey[100];

static INT_PTR CALLBACK exporthook(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static void setdefaultexportparams(INDEX *FF, EXPORTPARAMS * exp, int type);		// sets default export params
static BOOL reconcilefiletype(HWND parent, OPENFILENAME * ofp);	/* reconciles file type in name with type droplist */
static INT_PTR CALLBACK seteoptions(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK setfoptions(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static short stationeryexport(INDEX * FF, struct expfile * ef);	/* makes stationery pad from current index */
static BOOL nativeexport(INDEX * FF, struct expfile * ef);	/* duplicates index */
//static short writearchiverecords(INDEX * FF, TCHAR * path, EXPORTPARAMS * exp, short *fontids);	/* opens & writes export file */
static char * writearchiverecords(char * wfbase, INDEX * FF, EXPORTPARAMS * exp, short *fontids);	/* opens & writes export file */
static BOOL convertrecordtext(char * outbuff, char * text);	// converts record to V2 form
static char charfromsymbol(unichar uc);		// returns symbol font char for unicode
static char * writexmlrecords(char * wfptr, INDEX * FF, EXPORTPARAMS * exp, short *fontids);	// writes record as xml
static char * writerawtext(char * fptr, char *sptr);	/* write field as plain text */
static char * writefordos(char * fptr, char *sptr);	/* write field for DOS */
//static void writetext(FILE * fptr, char *sptr);	/* write field as plain text */
static char * timestring(time_c time);	// returns time string
static char * xmlchartostring( char xc);		// appends character (escaped as necessary)
static void dostyles(HWND hwnd, struct expfile *ef);	// sets up stylenames
static INT_PTR CALLBACK stylenamesproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

/*******************************************************************************/
static TCHAR * buildtaglist(TCHAR * bptr, TCHAR *limit, int type)	// builds customized descriptor for tagged output

{
	TCHAR name[STSTRING] = TEXT("");
	int length = STSTRING;
	TCHAR *eptr, *nptr;

	if (type == XMLTAGS)	{
		eptr = TEXT("xml");
		nptr = TEXT("XML Tagged Text");
	}
	else	{
		TAGSET * ts = ts_openset(ts_getactivetagsetpath(SGMLTAGS));
		eptr = *ts->extn ? toNative(ts->extn) : TEXT("tag");
		free(ts);
		nptr = TEXT("SGML Tagged Text");
	}
	bptr += u_sprintf(bptr,"%S [%S] (*.%S)",nptr,ts_getactivetagsetname(type),eptr,eptr)+1;
	bptr += u_sprintf(bptr,"*.%S",eptr)+1;
	*bptr = '\0';	/* terminate compound string */
	return bptr;
}
/*******************************************************************************/
short exp_export(HWND wptr)		/* exports index */

{
#define FSELENGTH 3000

	struct expfile ef;
	INDEX * FF;
	short ok;
	TCHAR path[MAX_PATH],title[_MAX_FNAME+_MAX_EXT];
	TCHAR fstrings[FSELENGTH], *bptr, *nameptr;
	HCURSOR ocurs;
	
	memcpy(fstrings,efilters,EFILTERSIZE);
	bptr = fstrings+EFILTERSIZE/sizeof(TCHAR);
	bptr = buildtaglist(bptr,fstrings+(FSELENGTH-MAX_PATH),XMLTAGS);
	buildtaglist(bptr,fstrings+(FSELENGTH-MAX_PATH),SGMLTAGS);
	memset(&ef,0,sizeof(ef));	/* clear everything */
	ef.FF = FF = getowner(wptr);
	ef.ofn.lStructSize = sizeof(OPENFILENAME);
	ef.ofn.hwndOwner = g_hwframe;
	ef.ofn.lpstrFilter = fstrings;
	ef.ofn.lpstrFile = path;		/* holds path on return */
	ef.ofn.nMaxFile = MAX_PATH;
	ef.ofn.lpstrFileTitle = title;	/* holds file title on return */
	ef.ofn.nMaxFileTitle = _MAX_FNAME+_MAX_EXT;
	ef.ofn.Flags = OFN_OVERWRITEPROMPT|OFN_HIDEREADONLY|OFN_PATHMUSTEXIST
		|OFN_EXPLORER|OFN_ENABLETEMPLATE|OFN_ENABLEHOOK|OFN_NOCHANGEDIR|OFN_ENABLESIZING;
	ef.ofn.lpstrDefExt = str_uxatindex(fstrings,1)+2;	// set default extension from first file type
	ef.ofn.lpfnHook = exporthook;
	ef.ofn.hInstance = g_hinst;
	ef.ofn.lCustData = (LONG_PTR)&ef;	/* for export params */
	ef.ofn.lpTemplateName = MAKEINTRESOURCE(IDD_SAVEAS_OPTIONBUTTON);
	nameptr = nstrrchr(FF->pfspec,'\\');	/* get ptr to name only */
	nstrcpy(path,++nameptr);
	*file_getextn(path) = '\0';	/* strip extension */
	*title = '\0';
	GetCurrentDirectory(MAX_PATH,e_path);
	ef.ofn.lpstrInitialDir = e_path;		/* set it */
	if (GetSaveFileName(&ef.ofn))	{	/* if want to use */
		ocurs = SetCursor(g_waitcurs);
		if (ef.type == E_NATIVE)	/* copy current */
			ok = nativeexport(FF,&ef);	/* as native file */
		else if (ef.type == E_STATIONERY)
			ok = stationeryexport(FF,&ef);
		else if (ef.type < E_TEXTNOBREAK)	{	// some kind of unformatted export
			if (ef.type == E_XMLRECORDS)	{	// if need to check syntax
				GROUPHANDLE gh = grp_startgroup(FF); 	/* initialize a group */
		
				if (grp_buildfromcheck(FF,&gh))	{
					grp_installtemp(FF,gh);
					SendMessage(FF->vwind,WM_COMMAND,IDM_VIEW_TEMPORARYGROUP,0);	/* display group */
					senderr(ERR_RECORDSYNTAXERR,WARN,ef.ofn.lpstrFile);
					return 0;
				}
				grp_dispose(gh);
			}
			ok = exp_rawexport(FF,&ef);
		}
		else	/* format text of some sort */
			ok = formexport_write(FF,&ef,typesetters[ef.type]);
		if (!ok)
			senderr(ERR_WRITEERR,WARN,ef.ofn.lpstrFile);
		SetCursor(ocurs);
	}
	return (0);
}
/**********************************************************************************/	
static INT_PTR CALLBACK exporthook(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)

{
	HWND parent;
	struct expfile *ef;
	OPENFILENAME * ofp;

	ef = getdata(hwnd);
	parent = GetParent(hwnd);
	switch (msg)	{
		case WM_INITDIALOG:
			if (ef = setdata(hwnd, (void *)((OPENFILENAME*)lParam)->lCustData))	{
//				memset(&ef->FF->pf,0,sizeof(PRINTFORMAT));	/* clear format info structure */
				setdefaultexportparams(ef->FF,&ef->exp,E_NATIVE);
				disableitem(hwnd,IDC_SAVEAS_OPTIONS);	/* no options for native format */
				centerwindow(parent,0);
				file_setaccess(hwnd,parent,FOLDER_CDX);
			}
			return (TRUE);
		case WM_COMMAND:
			switch (LOWORD(wParam))	{
				case IDC_SAVEAS_OPTIONS:
					if (ef->type < E_TEXTNOBREAK)	/* unformatted export */	{
						if (DialogBoxParam(g_hinst,MAKEINTRESOURCE(IDD_WRITE_UNFORM),parent,seteoptions,(LPARAM)ef))	/* if OK */
							if (ef->exp.appendflag)
								ef->ofn.Flags &= ~OFN_OVERWRITEPROMPT;	/* clear overwrite warning */	
					}
					else
						DialogBoxParam(g_hinst,MAKEINTRESOURCE(IDD_WRITE_FORM),parent,setfoptions,(LPARAM)ef);
					return TRUE;
			}
			return FALSE;
		case WM_NOTIFY:
			ofp = ((LPOFNOTIFY)lParam)->lpOFN;
			switch (((LPOFNOTIFY)lParam)->hdr.code)	{
				case CDN_FILEOK:
					if (!reconcilefiletype(parent,ofp))	{	// if doesn't have extension to match type
						if (!sendwarning(WARN_BADEXTENSION,ofp->lpstrFile))	{	// if cancel
							SetWindowLongPtr(hwnd, DWLP_MSGRESULT,TRUE);
							return (TRUE);
						}
					}
					if (ef->type == E_NATIVE && index_byfspec(ofp->lpstrFile))	{	/* if overwriting existing file */
						senderr(ERR_OVERWRITEOPEN,WARN,file_getname(ofp->lpstrFile));	/* always save extended info */
						SetWindowLongPtr(hwnd, DWLP_MSGRESULT,TRUE);
						return (TRUE);
					}
					break;
				case CDN_TYPECHANGE:
					ef->type = (short)ofp->nFilterIndex-1;
					ef->ofn.lpstrDefExt = str_uxatindex(ofp->lpstrFilter,ofp->nFilterIndex*2-1)+2;	// set default extension from type
					ef->ofn.Flags |= OFN_OVERWRITEPROMPT;	/* reinstate any cleared overwrite warning */
					setdefaultexportparams(ef->FF,&ef->exp,ef->type);
					if (ef->type == E_NATIVE || ef->type == E_STATIONERY)	/* if native format */
						disableitem(hwnd,IDC_SAVEAS_OPTIONS); 	/* disable options */
					else
						enableitem(hwnd,IDC_SAVEAS_OPTIONS);
			}
			break;
		case WM_HELP:
			return (dodialoghelp(hwnd,NULL,(HELPINFO *)lParam,wh_saveasid));
		default:
			;
	}
	return (FALSE);
}
/******************************************************************************/
static void setdefaultexportparams(INDEX *FF, EXPORTPARAMS * exp, int type)		// sets default params

{
	char tsort = FF->head.sortpars.ison;
	char tdel = FF->head.privpars.hidedelete;		/* save del status */

	memset(exp,0,sizeof(EXPORTPARAMS));
	memset(&FF->pf,0,sizeof(PRINTFORMAT));	/* clear format info structure */
	if (type == E_ARCHIVE)		/* if archive  */
		exp->extendflag = TRUE;	/* always save extended info */
	if (type == E_TAB || type >= E_TEXTNOBREAK)	// if tabbed or formatted (other record formats all default unsorted)
		exp->sorted = FF->head.sortpars.ison;
	exp->type = type;
	exp->usetabs = g_prefs.gen.indentdef;
	FF->head.sortpars.ison = exp->sorted;
	FF->head.privpars.hidedelete = !exp->includedeleted;	// default hide deleted
	exp->first = rec_number(sort_top(FF));	/* default first record */
	exp->last = ULONG_MAX;
	FF->head.sortpars.ison = tsort;
	FF->head.privpars.hidedelete = tdel;
}
/******************************************************************************/
static BOOL reconcilefiletype(HWND parent, OPENFILENAME * ofp)	/* reconciles file type in name with type droplist */

{
	TCHAR *exptr, *tptr, *nxptr;
	int index;
	
	if (exptr = nstrrchr(ofp->lpstrFile,'.'))	{	/* if have extension */
		for (index = 0, tptr = ofp->lpstrFilter; *tptr; tptr += nstrlen(tptr++), index++)	{	/* find it in list */
			tptr += nstrlen(tptr++);
			if (!nstrcmp(exptr,tptr+1))		{	// if a match
				nxptr = str_uxatindex(ofp->lpstrFilter,ofp->nFilterIndex*2-1);	// find extension for type
				return !nstrcmp(exptr,nxptr+1);	// return TRUE if using standard extension
			}
		}
	}
	return FALSE;
}
/******************************************************************************/
static INT_PTR CALLBACK seteoptions(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)

{
	struct expfile *ef, tef;
	int scope, err;
	char tsort;

	ef = getdata(hwnd);
	switch (msg)	{

		case WM_INITDIALOG:
			if (ef = setdata(hwnd, (struct expfile*)lParam))	{
				centerwindow(hwnd,0);
				setint(hwnd,IDC_WRITE_UNFORMMINFIELD,ef->exp.minfields);
				if (*ef->rangestr1 || *ef->rangestr2)	{	/* if a range specified */
					setDItemText(hwnd,IDC_FIND_RANGESTART,ef->rangestr1);
					setDItemText(hwnd,IDC_FIND_RANGEEND,ef->rangestr2);
					CheckRadioButton(hwnd,IDC_FIND_ALL,IDC_FIND_RANGE, IDC_FIND_RANGE);
				}
				else
					CheckRadioButton(hwnd,IDC_FIND_ALL,IDC_FIND_RANGE, IDC_FIND_ALL);
				checkitem(hwnd,IDC_WRITE_UNFORMAPPEND,ef->exp.appendflag);
				checkitem(hwnd,IDC_WRITE_UNFORMEXTEND,ef->exp.extendflag);
				checkitem(hwnd,IDC_WRITE_UNFORMNUMORDER,!ef->exp.sorted);
				checkitem(hwnd,IDC_WRITE_UNFORMDEL,ef->exp.includedeleted);
				if (!view_recordselect(ef->FF->vwind))	/* if no selection */	
					disableitem(hwnd,IDC_FIND_SELECTED);	/* disable item */
				if (ef->type == E_TAB)	
					CheckRadioButton(hwnd,IDC_WRITEUNFORM_UTF8,IDC_WRITEUNFORM_ANSI, IDC_WRITEUNFORM_UTF8);
				else {
					hideitem(hwnd,IDC_WRITEUNFORM_UTF8);	// encoding
					hideitem(hwnd,IDC_WRITEUNFORM_ANSI);
					hideitem(hwnd,IDC_WRITE_UNFORMMINFIELD);	/* forbid min field spec */
					hideitem(hwnd,IDC_WRITE_UNFORMMINFIELDPROMPT);	/* forbid min field spec */
				}
				if (ef->type == E_ARCHIVE || ef->type == E_XMLRECORDS  || ef->type == E_TAB)	{	/* if archive or xml */
					hideitem(hwnd,IDC_WRITE_UNFORMAPPEND);		// forbid appending
					hideitem(hwnd,IDC_WRITE_UNFORMEXTEND);		// forbid extended info
				}
				selectitext(hwnd,IDC_FIND_RANGESTART);
			}
			return (TRUE);
		case WM_COMMAND:
			switch (LOWORD(wParam))	{
				case IDOK:
					tef = *ef;
					tef.exp.appendflag = (short)isitemchecked(hwnd,IDC_WRITE_UNFORMAPPEND);
					getshort(hwnd,IDC_WRITE_UNFORMMINFIELD,&tef.exp.minfields);
					if (tef.exp.minfields > FIELDLIM)	{
						selectitext(hwnd,IDC_WRITE_UNFORMMINFIELD);
						break;
					}
					tef.exp.encoding = findgroupcheck(hwnd,IDC_WRITEUNFORM_UTF8,IDC_WRITEUNFORM_ANSI)-IDC_WRITEUNFORM_UTF8;
					tef.exp.extendflag = (short)isitemchecked(hwnd,IDC_WRITE_UNFORMEXTEND);
					tef.exp.sorted = !(short)isitemchecked(hwnd,IDC_WRITE_UNFORMNUMORDER);
					tef.exp.includedeleted = (short)isitemchecked(hwnd,IDC_WRITE_UNFORMDEL);
					scope = findgroupcheck(hwnd,IDC_FIND_ALL,IDC_FIND_RANGE)-IDC_FIND_ALL;
					tsort = tef.FF->head.sortpars.ison;
					tef.FF->head.sortpars.ison = tef.exp.sorted;	// sort order from spec
					getDItemText(hwnd,IDC_FIND_RANGESTART,tef.rangestr1,STSTRING);
					getDItemText(hwnd,IDC_FIND_RANGEEND,tef.rangestr2,STSTRING);
					if (err = com_getrecrange(tef.FF,(short)scope,tef.rangestr1,tef.rangestr2,&tef.exp.first, &tef.exp.last))	{	/* bad range */
						selectitext(hwnd,err < 0 ? IDC_FIND_RANGESTART : IDC_FIND_RANGEEND);
						tef.FF->head.sortpars.ison = tsort;	/* restore sort */
						break;
					}
					tef.FF->head.sortpars.ison = tsort;	/* restore sort */
					*ef = tef;
				case IDCANCEL:
					EndDialog(hwnd,LOWORD(wParam) == IDOK ? TRUE : FALSE);
					return TRUE;
				case IDC_FIND_ALL:
				case IDC_FIND_SELECTED:
				case IDC_FIND_RANGE:
					if (isitemchecked(hwnd,IDC_FIND_SELECTED))	{
						checkitem(hwnd,IDC_WRITE_UNFORMNUMORDER,FALSE);
						disableitem(hwnd,IDC_WRITE_UNFORMNUMORDER);
					}
					else
						enableitem(hwnd,IDC_WRITE_UNFORMNUMORDER);
					break;
				case IDC_FIND_RANGESTART:
				case IDC_FIND_RANGEEND:
					if (HIWORD(wParam) == EN_CHANGE)	{
						CheckRadioButton(hwnd,IDC_FIND_ALL,IDC_FIND_RANGE, IDC_FIND_RANGE);
						enableitem(hwnd,IDC_WRITE_UNFORMNUMORDER);
					}
					break;
			}
			break;
		case WM_HELP:
			{
				TCHAR * hptr;
				if (ef->type == E_ARCHIVE)
					hptr = TEXT("base\\Export_optionsarchive.htm");
				else
					hptr = TEXT("base\\Export_optionsdelimited.htm");
				return (dodialoghelp(hwnd,hptr,(HELPINFO *)lParam,wh_ufwriteid));
			}
	}
	return (FALSE);
}
/******************************************************************************/
static INT_PTR CALLBACK setfoptions(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)

{
	struct expfile *ef, tef;
	int scope, err, stylemode, nsize;
	long tnum;
	char tbuff[2];

	ef = getdata(hwnd);
	switch (msg)	{

		case WM_INITDIALOG:
			if (ef = setdata(hwnd, (struct expfile*)lParam))	{
				sprintf(regKey, "exportStyles%d", ef->exp.type);	// create right reg key
				nsize = sizeof(STYLENAMES);
				*ef->exp.stylenames.levelnames = '\177';	// set as empty by default
				reg_getkeyvalue(K_GENERAL, toNative(regKey), &ef->exp.stylenames, &nsize);	// get any existing setting
				if (ef->rangemethod == RR_PAGES && (ef->exp.firstpage != 1 || ef->exp.lastpage != SHRT_MAX))	{	/* if not default page range */
					setint(hwnd,IDC_FORMWRITE_PSTART,ef->exp.firstpage);
					setint(hwnd,IDC_FORMWRITE_PEND,ef->exp.lastpage);
					CheckRadioButton(hwnd,IDC_FORMWRITE_ALL,IDC_FORMWRITE_PAGE, IDC_FORMWRITE_PAGE);
				}
				else if (ef->rangemethod == RR_RECORDS)	{	/* if a range specified */
					setDItemText(hwnd,IDC_FORMWRITE_RSTART,ef->rangestr1);
					setDItemText(hwnd,IDC_FORMWRITE_REND,ef->rangestr2);
					CheckRadioButton(hwnd,IDC_FORMWRITE_ALL,IDC_FORMWRITE_RECORDS, IDC_FORMWRITE_RECORDS);
				}
				else if (ef->rangemethod == RR_SELECTION)	/* if a selection specified */
					CheckRadioButton(hwnd,IDC_FORMWRITE_ALL,IDC_FORMWRITE_PAGE, IDC_FORMWRITE_SELECT);
				else
					CheckRadioButton(hwnd,IDC_FORMWRITE_ALL,IDC_FORMWRITE_PAGE, IDC_FORMWRITE_ALL);
				if (!view_recordselect(ef->FF->vwind))	/* if no selection */	
					disableitem(hwnd,IDC_FORMWRITE_SELECT);	/* disable item */
				selectitext(hwnd,IDC_FORMWRITE_PSTART);
				if (ef->type == E_XMLTAGGED || ef->type == E_TAGGED)	{	/* if tagged */
					hideitem(hwnd,IDC_FORMWRITE_USEPROMPT);	/* forbid use */
					hideitem(hwnd,IDC_FORMWRITE_USESTYLE);	/* forbid use */
					hideitem(hwnd,IDC_FORMWRITE_USETAB);	/* forbid use */
					hideitem(hwnd,IDC_FORMWRITE_USEOTHER);	/* forbid use */
					hideitem(hwnd,IDC_FORMWRITE_USEOTHERTEXT);	/* forbid use */
					hideitem(hwnd, IDC_FORMWRITE_STYLENAMES);	/* forbid use */
				}
				else if (ef->type == E_TEXTNOBREAK)
					hideitem(hwnd, IDC_FORMWRITE_STYLENAMES);	/* forbid use */
				else {
					if (ef->exp.usetabs)	{
						if (ef->exp.usetabs == '\t')
							stylemode = 1;
						else	{
							stylemode = 2;
							dprintf(hwnd,IDC_FORMWRITE_USEOTHERTEXT, "%c", ef->exp.usetabs);
						}
					}
					else
						stylemode = 0;
					Edit_LimitText(GetDlgItem(hwnd,IDC_FORMWRITE_USEOTHERTEXT),1);		/* limit to 1 char */
					CheckRadioButton(hwnd,IDC_FORMWRITE_USESTYLE,IDC_FORMWRITE_USEOTHER, IDC_FORMWRITE_USESTYLE+stylemode);
				}
				centerwindow(hwnd,0);
			}
			return (TRUE);
		case WM_COMMAND:
			if (HIWORD(wParam) == EN_UPDATE)	{
				WORD id = LOWORD(wParam);
				int length = 0;

				if (id == IDC_FORMWRITE_USEOTHERTEXT)
					length = 2;
				checktextfield((HWND)lParam,length);
				return TRUE;
			}
			switch (LOWORD(wParam))	{
				case IDOK:
					tef = *ef;
					tef.rangemethod = findgroupcheck(hwnd,IDC_FORMWRITE_ALL,IDC_FORMWRITE_RECORDS)-IDC_FORMWRITE_ALL;
					if (tef.rangemethod == RR_PAGES)	{	/* if want page range */
						if (getlong(hwnd,IDC_FORMWRITE_PSTART,&tnum))	/* if not empty */
							tef.exp.firstpage = tnum;
						if (getlong(hwnd,IDC_FORMWRITE_PEND,&tnum))		/* if not empty */
							tef.exp.lastpage = tnum;
						if (tef.exp.firstpage > tef.exp.lastpage)	{
							senderr(ERR_INVALPAGERANGE,WARN);
							selectitext(hwnd,IDC_FORMWRITE_PEND);
							return(TRUE);
						}
					}
					scope = tef.rangemethod;
					if (scope != COMR_ALL)	/* if not all */
						scope--;			/* to discount page range stuff */
					getDItemText(hwnd,IDC_FORMWRITE_RSTART,tef.rangestr1,STSTRING);
					getDItemText(hwnd,IDC_FORMWRITE_REND,tef.rangestr2,STSTRING);
					if (err = com_getrecrange(tef.FF,(short)scope,tef.rangestr1,tef.rangestr2,&tef.exp.first, &tef.exp.last))	{	/* bad range */
						selectitext(hwnd,err < 0 ? IDC_FORMWRITE_RSTART : IDC_FORMWRITE_REND);
						return (TRUE);
					}
					stylemode = findgroupcheck(hwnd,IDC_FORMWRITE_USESTYLE,IDC_FORMWRITE_USEOTHER)-IDC_FORMWRITE_USESTYLE;
					if (stylemode)	{
						if (stylemode == 1)
							tef.exp.usetabs = '\t';
						else	{
							getDItemText(hwnd,IDC_FORMWRITE_USEOTHERTEXT,tbuff,2);
							tef.exp.usetabs = *tbuff;
						}
					}
					else
						tef.exp.usetabs = 0;
					*ef = tef;
					reg_setkeyvalue(K_GENERAL, toNative(regKey), REG_BINARY, &ef->exp.stylenames, sizeof(STYLENAMES));	// save
				case IDCANCEL:
					EndDialog(hwnd,LOWORD(wParam) == IDOK ? TRUE : FALSE);
					return TRUE;
				case IDC_FORMWRITE_STYLENAMES:
					dostyles(hwnd, ef);
					break;
				case IDC_FORMWRITE_RSTART:
				case IDC_FORMWRITE_REND:
					if (HIWORD(wParam) == EN_CHANGE)
						CheckRadioButton(hwnd,IDC_FORMWRITE_ALL,IDC_FORMWRITE_RECORDS, IDC_FORMWRITE_RECORDS);
					break;
				case IDC_FORMWRITE_PSTART:
				case IDC_FORMWRITE_PEND:
					if (HIWORD(wParam) == EN_CHANGE)
						CheckRadioButton(hwnd,IDC_FORMWRITE_ALL,IDC_FORMWRITE_RECORDS, IDC_FORMWRITE_PAGE);
					break;
				case IDC_FORMWRITE_USEOTHERTEXT:
					if (HIWORD(wParam) == EN_CHANGE)
						CheckRadioButton(hwnd,IDC_FORMWRITE_USESTYLE,IDC_FORMWRITE_USEOTHER, IDC_FORMWRITE_USEOTHER);
					break;
			}
			break;
		case WM_HELP:
			return (dodialoghelp(hwnd,TEXT("base\\Export_optionsformatted.htm"),(HELPINFO *)lParam,wh_fwriteid));
	}
	return (FALSE);
}
/*******************************************************************************/
static void dostyles(HWND hwnd, struct expfile *ef)	// sets up stylenames

{
	struct styleset ss;
	char buffer[512], *sptr;

	ss.sn = ef->exp.stylenames;
	sptr = ss.nativenames = buffer;
	strcpy(sptr, ef->exp.type == E_RTF ? "ahead" : "Ahead");
	sptr += strlen(sptr) + 1;
	for (int findex = 0; findex < ef->FF->head.indexpars.maxfields; findex++, sptr += strlen(sptr)+1)
		strcpy(sptr,ef->FF->head.indexpars.field[findex].name);
	*sptr = EOCS;
	if (DialogBoxParam(g_hinst, MAKEINTRESOURCE(IDD_WRITE_FORM_EXTRAS), hwnd, stylenamesproc, (LPARAM)&ss)) {
		ef->exp.stylenames = ss.sn;
	}
}
/******************************************************************************/
static INT_PTR CALLBACK stylenamesproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)

{
	struct styleset *ss = getdata(hwnd);
	char * sptr;
	TCHAR buffer[1024];
	int findex;

	switch (msg) {

	case WM_INITDIALOG:
		ss = (struct styleset *)setdata(hwnd, (void *)lParam);
		sptr = ss->sn.type ? ss->sn.levelnames : ss->nativenames;
		findex = IDC_FORM_L0;
		for (; *sptr != EOCS; sptr +=strlen(sptr)+1) {
			HWND tw = (GetDlgItem(hwnd, findex++));
			SetWindowText(tw, toNative(sptr));
			EnableWindow(tw, ss->sn.type);
		}
		while (findex <= IDC_FORM_L15) {
			HWND tw = GetDlgItem(hwnd, findex++);
			SetWindowText(tw, TEXT(""));
			EnableWindow(tw, ss->sn.type);
		}
		CheckRadioButton(hwnd, IDC_FORM_FIELDNAMES, IDC_FORM_TABLENAMES, IDC_FORM_FIELDNAMES + ss->sn.type);
		centerwindow(hwnd, 1);
		return FALSE;
	case WM_NOTIFY:
		switch (((NMHDR *)lParam)->code) {
		}
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
			case IDOK:
				if (ss->sn.type) {	// if current type is table, recover names
					sptr = ss->sn.levelnames;
					for (int findex = IDC_FORM_L0; findex <= IDC_FORM_L15; findex++, sptr += strlen(sptr) + 1) {
						GetDlgItemText(hwnd, findex, buffer, sizeof(buffer));
						strcpy(sptr, fromNative(buffer));
					}
					*sptr = EOCS;
				}
			case IDCANCEL:
				if ((HIWORD(wParam) == 1 || HIWORD(wParam) == 0)) {
					EndDialog(hwnd, LOWORD(wParam) == IDOK ? TRUE : FALSE);
					return TRUE;
				}
			case IDC_FORM_FIELDNAMES:
			case IDC_FORM_TABLENAMES:
				if (ss->sn.type) {	// if current type is table, recover names
					sptr = ss->sn.levelnames;
					for (int findex = IDC_FORM_L0; findex <= IDC_FORM_L15; findex++, sptr += strlen(sptr) + 1) {
						GetDlgItemText(hwnd, findex, buffer, sizeof(buffer));
						strcpy(sptr, fromNative(buffer));
					}
					*sptr = EOCS;
				}
				ss->sn.type = LOWORD(wParam) - IDC_FORM_FIELDNAMES;	// set new type
				sptr = ss->sn.type ? ss->sn.levelnames : ss->nativenames;
				findex = IDC_FORM_L0;
				for (; *sptr != EOCS; sptr += strlen(sptr) + 1) {
					HWND tw = (GetDlgItem(hwnd, findex++));
					SetWindowText(tw, toNative(sptr));
					EnableWindow(tw, ss->sn.type);
				}
				while (findex <= IDC_FORM_L15) {
					HWND tw = GetDlgItem(hwnd, findex++);
					SetWindowText(tw, TEXT(""));
					EnableWindow(tw, ss->sn.type);
				}
				break;
			default:
			{
				;
			}
		}

		break;
	case WM_HELP:
		return (dodialoghelp(hwnd, TEXT("_Sorting\\Sorting_substitutions.htm"), (HELPINFO *)lParam, NULL));
	default:
		;
	}
	return FALSE;
}
/******************************************************************************//******************************************************************************/
short exp_rawexport(INDEX * FF, struct expfile * ef)	/* opens & writes raw export file */

{
	short ok = FALSE;
	MFILE mf;

	if (mfile_open(&mf,ef->ofn.lpstrFile,GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ,CREATE_ALWAYS,0,2*FF->head.rtot*FF->head.indexpars.recsize+sizeof(HEAD)))	{
		char * wfptr;
		RECORD * curptr;
		RECN rnum;
		short farray[FONTLIMIT];
		char oldsort, olddel;

		oldsort = FF->head.sortpars.ison;			/* save sort type */
		FF->head.sortpars.ison = ef->exp.sorted;		/* take from setting */
		olddel = FF->head.privpars.hidedelete;		/* save del status */
		FF->head.privpars.hidedelete = !ef->exp.includedeleted;
		memset(farray,0,sizeof(farray));
		for (rnum = 1; (curptr = rec_getrec(FF,rnum)); rnum++)		/* for all records */
			type_tagfonts(curptr->rtext,farray);		/* marks fonts used */
		if (ef->type == E_XMLRECORDS)
			wfptr = writexmlrecords(mf.base,FF,&ef->exp,farray);	/* open file, write the records */
		else
			wfptr = writearchiverecords(mf.base,FF,&ef->exp,farray);	/* open file, write the records */
		FF->head.privpars.hidedelete = olddel;
		FF->head.sortpars.ison = oldsort;
		if (mfile_resize(&mf,wfptr-mf.base) && mfile_close(&mf))	{
			ok = TRUE;
			if (ef->exp.errorcount)
				sendinfo(INFO_WRITERECWITHERROR,ef->exp.records,file_getname(ef->ofn.lpstrFile),ef->exp.longest,ef->exp.errorcount);
			else
				sendinfo(INFO_WRITEREC,ef->exp.records,file_getname(ef->ofn.lpstrFile),ef->exp.longest);
		}
	}
	return (ok);
}
/******************************************************************************/
static short stationeryexport(INDEX * FF, struct expfile * ef)	/* makes stationery pad from current index */

{
	HANDLE fid;
	long headsize;
	
	if ((fid = CreateFile(ef->ofn.lpstrFile,GENERIC_READ|GENERIC_WRITE,0,NULL,OPEN_ALWAYS,FILE_FLAG_RANDOM_ACCESS,0)) != INVALID_HANDLE_VALUE)	{
		if (WriteFile(fid,&FF->head,HEADSIZE,&headsize,NULL) && CloseHandle(fid))
			return (TRUE);
	}
	return (FALSE);
}
/******************************************************************************/
static BOOL nativeexport(INDEX * FF, struct expfile * ef)	/* duplicates index */

{
	BOOL ok = FALSE;

	if (file_duplicateindex(FF,ef->ofn.lpstrFile))	{	/* if can copy */
		g_revertstate = getmmstate(FF->cwind,&g_revertplacement);	// use to force window to be positioned over old one
		ok = file_openindex(ef->ofn.lpstrFile,OP_VISIBLE);	// open new one on top of old
		SendMessage(FF->cwind,WM_CLOSE,0,0);				// close old
		g_revertstate = 0;
	}
	return (ok);
}
#if 0
/******************************************************************************/
static short writearchiverecords(INDEX * FF, TCHAR * path, EXPORTPARAMS * exp, short *fontids)	/* opens & writes export file */

{
	FILE *wfptr;
	RECORD * curptr;
	char outbuff[MAXREC];
	CSTR fields[FIELDLIM];
	RECN rcount;
	short fieldcnt, ftot, efield;
	int curmax, newlen, deepest, count, writeindex;
	short err;

	if (wfptr = nfopen(path,exp->appendflag ? TEXT("ab") : TEXT("wb")))   {	/* open file */
		if (exp->type == E_ARCHIVE)	
			fprintf(wfptr,"xxxxxxxxxxxxxxxx");	/* reserve space for id stuff */
		else if (exp->type == E_TAB && exp->encoding == 0)	
			fprintf(wfptr,utf8BOM);	// write utf-8 BOM
		curptr = rec_getrec(FF,exp->first);	/* get start record */
		for (rcount = curmax = deepest = 0; !ferror(wfptr) && curptr && curptr->num != exp->last && !iscancelled(NULL); curptr = sort_skip(FF,curptr,1)) {	  /* for all records */
			if (!*curptr->rtext)	 /* if not wanted or empty */
				continue;			/* don't write */
			if (exp->type == E_TAB && exp->encoding == 0)		// if want tabbed text
				str_xcpy(outbuff, curptr->rtext);		// just copy utf-8 to output buffer
			else
				exp->errorcount += convertrecordtext(outbuff, curptr->rtext);	// convert to code page
			showprogress(PRG_SAVING,FF->head.rtot,rcount++);	  /* send message */
			if ((ftot = str_xparse(outbuff, fields)) > deepest)		/* parse */
				deepest = ftot;
			if ((newlen = str_xlen(outbuff)) > curmax)     /* if this is longest field */
				curmax = newlen;
			for (newlen = fieldcnt = 0; fieldcnt < ftot-1; fieldcnt++)   { /* for all regular fields */
				if (exp->type == E_ARCHIVE)
					fprintf(wfptr, "%s\t", fields[fieldcnt].str);  /* tab delimited */
				else if (exp->type == E_TAB)	{
					writetext(wfptr,fields[fieldcnt].str);		/* write as plain text */
					fputc('\t',wfptr);
				}
				else	{
					writefordos(wfptr,fields[fieldcnt].str);	/* write string, translating & stripping */
					fputc('\t',wfptr);
				}
			}
			for (efield = fieldcnt; efield < exp->minfields-1; efield++)     /* while not enough fields */
				fprintf(wfptr, "\t");     /* pad with empty fields */
			if (exp->type == E_ARCHIVE)
				fprintf(wfptr,"%s",fields[fieldcnt].str);	/* locator field */
			else if (exp->type == E_TAB)
				writetext(wfptr,fields[fieldcnt].str);		/* write as plain text */
			else 
				writefordos(wfptr,fields[fieldcnt].str);	/* write string, translating & stripping */
			if (exp->extendflag)	{
				char recflags;

				recflags = 0;
				if (exp->type == E_ARCHIVE)	{	/* if archive, save extra flags */
					if (curptr->label)	{	// build label value in compatible way
						// low order bit shifted 1 to left two high order bits shifted 2 to left
						recflags = ((curptr->label&1) << 1) + ((curptr->label&6) << 2);
					}
					if (curptr->isgen)
						recflags |= W_GENFLAG;
					if (FF->head.indexpars.required && fields[ftot-2].ln)	// if required field populated
						recflags |= W_PUSHLAST;
				}
				if (curptr->isdel)
					recflags |= W_DELFLAG;
				// DOS legacy: either flag is 64 (deleted) or space (not deleted)
				if (recflags)
					recflags |= 64;			/* allows DOS value to be 'A' */
				else
					recflags = SPACE;
				fprintf(wfptr, "\023%c%lu %.4s", recflags, curptr->time, curptr->user);				
			}
			fputc('\r', wfptr);		/* cr */
			fputc('\n', wfptr);		/* line feed */
		}
		if (exp->type == E_ARCHIVE)	{	/* if archive, add fixups */
			long curpos;

			curpos = ftell(wfptr);
#if 0
			for (count = 0; count < FONTLIMIT; count++)	{
				if (farray[count] || count < VOLATILEFONTS)		/* if this font was used or is protected */
					fprintf(wfptr,"%d@@%s@@%s\r\n",count,FF->head.fm[count].pname,FF->head.fm[count].name);	/* add its name */
			}
#else
		for (writeindex = count = 0; count < FONTLIMIT; count++, writeindex++)	{
			if (fontids[count] || count < VOLATILEFONTS)	{	/* if this font was used or is protected */
				fprintf(wfptr,"%d@@%s@@%s\r\n",writeindex,FF->head.fm[count].pname,FF->head.fm[count].name);	/* add its name */
				if (!writeindex)	{	// if have written first font, insert line for symbol
					fprintf(wfptr,"%d@@%s@@%s\r\n",1,"Symbol","Symbol");
					writeindex++;	// increment id index
				}
			}
		}
#endif
			fseek(wfptr,0,SEEK_SET);	/* to beginning */
			fprintf(wfptr,"\1\1\1\1%10ld\r\n",curpos);	/* write version and info offset as header */
			/* !!NB size of lead must be set in ARCHIVEOFFSET; must match dummy string length written at start */
		}
		err = ferror(wfptr);
		fclose(wfptr);
		showprogress(0,0,0);	  /* kill message */
		exp->records = rcount;
		exp->longest = curmax+1;
		return (err ? FALSE : TRUE);
	}
	return (FALSE);
}
#else
/******************************************************************************/
static char * writearchiverecords(char * wfbase, INDEX * FF, EXPORTPARAMS * exp, short *fontids)	/* opens & writes export file */

{
	char * wfptr = wfbase;
	RECORD * curptr;
	char outbuff[MAXREC];
	CSTR fields[FIELDLIM];
	RECN rcount;
	short fieldcnt, ftot, efield;
	int curmax, newlen, deepest, count, writeindex;

	if (exp->type == E_ARCHIVE)		
		wfptr += sprintf(wfptr,"xxxxxxxxxxxxxxxx");	/* reserve space for id stuff */
	if (exp->type == E_TAB && exp->encoding == 0)		
		wfptr += sprintf(wfptr,utf8BOM);	//write UTF-8 marker
	curptr = rec_getrec(FF,exp->first);	/* get start record */
	for (rcount = curmax = deepest = 0; curptr && curptr->num != exp->last && !iscancelled(NULL); curptr = sort_skip(FF,curptr,1)) {	  /* for all records */
		if (!*curptr->rtext)	 /* if not wanted or empty */
			continue;			/* don't write */
		if (exp->type == E_TAB && exp->encoding == 0)		// if want tabbed text
			str_xcpy(outbuff, curptr->rtext);		// just copy utf-8 to output buffer
		else
			exp->errorcount += convertrecordtext(outbuff, curptr->rtext);	// convert to code page
		showprogress(PRG_SAVING,FF->head.rtot,rcount++);	  /* send message */
		if ((ftot = str_xparse(outbuff, fields)) > deepest)		/* parse */
			deepest = ftot;
		if ((newlen = str_xlen(outbuff)) > curmax)     /* if this is longest field */
			curmax = newlen;
		for (newlen = fieldcnt = 0; fieldcnt < ftot-1; fieldcnt++)   { /* for all regular fields */
			if (exp->type == E_ARCHIVE)
				wfptr += sprintf(wfptr, "%s\t", fields[fieldcnt].str);  /* tab delimited */
			else if (exp->type == E_TAB)	{
				wfptr = writerawtext(wfptr,fields[fieldcnt].str);		/* write as plain text */
				*wfptr++ = '\t';
			}
			else	{
				wfptr = writefordos(wfptr,fields[fieldcnt].str);	/* write string, translating & stripping */
				*wfptr++ = '\t';
			}
		}
		for (efield = fieldcnt; efield < exp->minfields-1; efield++)     /* while not enough fields */
			wfptr += sprintf(wfptr, "\t");     /* pad with empty fields */
		if (exp->type == E_ARCHIVE)
			wfptr += sprintf(wfptr,"%s",fields[fieldcnt].str);	/* locator field */
		else if (exp->type == E_TAB)
			wfptr = writerawtext(wfptr,fields[fieldcnt].str);		/* write as plain text */
		else 
			wfptr = writefordos(wfptr,fields[fieldcnt].str);	/* write string, translating & stripping */
		if (exp->extendflag)	{
			char recflags;

			recflags = 0;
			if (exp->type == E_ARCHIVE)	{	/* if archive, save extra flags */
				if (curptr->label)	{	// build label value in compatible way
					// low order bit shifted 1 to left two high order bits shifted 2 to left
					recflags = ((curptr->label&1) << 1) + ((curptr->label&6) << 2);
				}
				if (curptr->isgen)
					recflags |= W_GENFLAG;
				if (FF->head.indexpars.required && fields[ftot-2].ln)	// if required field populated
					recflags |= W_PUSHLAST;
			}
			if (curptr->isdel)
				recflags |= W_DELFLAG;
			// DOS legacy: either flag is 64 (deleted) or space (not deleted)
			if (recflags)
				recflags |= 64;			/* allows DOS value to be 'A' */
			else
				recflags = SPACE;
			wfptr += sprintf(wfptr, "\023%c%lu %.4s", recflags, curptr->time, curptr->user);				
		}
		*wfptr++ = '\r';
		*wfptr++ = '\n';
	}
	if (exp->type == E_ARCHIVE)	{	/* if archive, add fixups */
		long curpos = wfptr-wfbase;

		for (writeindex = count = 0; count < FONTLIMIT; count++, writeindex++)	{
			if (fontids[count] || count < VOLATILEFONTS)	{	/* if this font was used or is protected */
				wfptr += sprintf(wfptr,"%d@@%s@@%s\r\n",writeindex,FF->head.fm[count].pname,FF->head.fm[count].name);	/* add its name */
				if (!writeindex)	{	// if have written first font, insert line for symbol
					wfptr += sprintf(wfptr,"%d@@%s@@%s\r\n",1,"Symbol","Symbol");
					writeindex++;	// increment id index
				}
			}
		}
		wfptr++;	// move beyond terminal '0'
		sprintf(wfbase,"\1\1\1\1%10ld\r",curpos);	/* write version and info offset as header */
		wfbase[ARCHIVEOFFSET-1] = '\n';	// overwrite '\0' string terminator with terminal newline
		/* !!NB size of lead must be set in ARCHIVEOFFSET; must match dummy string length written at start */
	}
	showprogress(0,0,0);	  /* kill message */
	exp->records = rcount;
	exp->longest = curmax+1;
	return wfptr;
}
#endif
/******************************************************************************/
static BOOL convertrecordtext(char * outbuff, char * text)	// converts record to V2 form

{
	char currentfont = FX_FONT;	// default font
	BOOL error = FALSE;
	char * sptr, *dptr;
	
	if (!converter)
		converter = iconv_open(V2_CHARSET,"UTF-8");
	for (sptr = text, dptr = outbuff; *sptr != EOCS; sptr = u8_forward1(sptr))		{
		unichar uc = u8_toU(sptr);
		
		if (uc >0x80)	{	// if needs conversion
			char * source = sptr;
			size_t sourcecount = u8_forward1(sptr)-sptr;	// number of bytes to convert
			size_t destcount = MAXREC-(dptr-outbuff);
			size_t length = iconv(converter,&source,&sourcecount,&dptr,&destcount);
			
			if ((int)length < 0)	{	// if error, assume unencodable char
				char symbolchar = charfromsymbol(uc);
				if (symbolchar)	{	// if good conversion
					char * tptr = sptr;
					*dptr++ = CODECHR;
					*dptr++ = FX_FONT|1;	// set symbol font
					do  {
						*dptr++ = symbolchar;
						tptr = u8_forward1(tptr);
					} while ((symbolchar = charfromsymbol(u8_toU(tptr))));		// while symbols to convert
					sptr = u8_back1(tptr);
					*dptr++ = CODECHR;
					*dptr++ = currentfont;	// restore prev font
				}
				else	{
					*dptr++ = 191;	// unknown char (inverted ?)
					error = TRUE;
				}
			}
		}
		else if (iscodechar(uc))	{
			if (uc == FONTCHR)	{	// if font
				if (*++sptr&FX_FONT)	{	// if font
					*dptr++ = CODECHR;	// change to code char
					currentfont = *sptr;
					if (currentfont&FX_FONTMASK)	// if it's not the default font
						currentfont++;	// increase id by 1 to allow for symbol font
					*dptr++ = currentfont;
				}
				else if (*sptr&FX_COLOR)	{	// if color
					*dptr++ = CODECHR;		// change to code char
					*dptr++ = *sptr|FX_FONT;	// add obligatory FX_FONT bit
				}
			}
			else {		// style code
				*dptr++ = *sptr++;	// copy codechar
				if (*sptr&FX_OFF)	// if off code
					*dptr++ = (*sptr&FX_STYLEMASK)|FX_OLDOFF;
				else		// normal 'on' style code
					*dptr++ = *sptr;
			}
		}
		else		// simple ascii char
			*dptr++ = uc;
	}
	*dptr = EOCS;
	return error;
}
/*******************************************************************************/
static char charfromsymbol(unichar uc)		// returns symbol font char for unicode

{
	int sindex;

	for (sindex = 0; sindex < 256; sindex++)	{
		if (t_specialfonts[0].ucode[sindex] == uc)
			return sindex;
	}
	return 0;
}
/******************************************************************************/
static char * writexmlrecords(char * wfptr, INDEX * FF, EXPORTPARAMS * exp, short *fontids)	// writes record as xml

{
	static const char *special = "\\~<>{}";
	static const char *specialstrings[] = {"<esc/>","<literal/>","<hide>","</hide>","<sort>","</sort>"};
	RECORD * curptr;
	CSTR fields[FIELDLIM];
	RECN rcount;
	short fieldcnt, ftot;
	char scapsstring[100], stylestring[100], fontstring[100], colorstring[100], offsetstring[100];
	char recordstype[100], recordstring[100], lseparator[100], xseparator[100], lconnector[100];
	char fieldclass[100];
	int font, style, color;
	int curmax, deepest,newlen, sindex;
	char * string, *tptr;
	int findex;

	wfptr += sprintf(wfptr, "%s", xmlheader); 
	wfptr += sprintf(wfptr, "<indexdata>\n"); 
	wfptr += sprintf(wfptr, "<source creator=\"cindex\" version=\"%d.%d\"%s/>\n", CINVERSION/100, CINVERSION%100,timestring(time(NULL))); 
	wfptr += sprintf(wfptr, "<fonts>\n"); 
	for (findex = 0; *FF->head.fm[findex].name; findex++)	{	// for all fonts claimed in index
		if (!findex || fontids[findex])	// if base font or another used
			wfptr += sprintf(wfptr, "<font id=\"%d\">\n\t<fname>%s</fname>\n\t<aname>%s</aname>\n</font>\n",findex,FF->head.fm[findex].pname,FF->head.fm[findex].name);
	}
	wfptr += sprintf(wfptr, "</fonts>\n"); 
	*recordstype = *lseparator = *xseparator = '\0';
	if (FF->head.indexpars.required)	// if required last field
		sprintf(recordstype," type=\"1\"");		// 1 is required last field
//	if (FF->head.refpars.psep != ',')	// if page separator not comma
	sprintf(lseparator," loc-separator=\"%s\"",xmlchartostring(FF->head.refpars.psep));	// emit it
	sprintf(lconnector," loc-connector=\"%s\"",xmlchartostring(FF->head.refpars.rsep));	// form it
//	if (FF->head.refpars.csep != ';')	// if page separator not semicolon
	sprintf(xseparator," xref-separator=\"%s\"",xmlchartostring(FF->head.refpars.csep));	// emit it
	wfptr += sprintf(wfptr, "<records%s%s%s%s>\n",recordstype,lseparator,xseparator,lconnector);
	curptr = rec_getrec(FF,exp->first);	/* get start record */
	for (rcount = curmax = deepest = 0; curptr && curptr->num != exp->last; curptr = sort_skip(FF,curptr,1), rcount++) {	  /* for all records */
		
		*recordstring = '\0';
		tptr = recordstring;
		if (!*curptr->rtext)	 /* if not wanted or empty */
			continue;			/* don't write */
		if ((ftot = str_xparse(curptr->rtext, fields)) > deepest)		/* parse */
			deepest = ftot;
		if ((newlen = str_xlen(curptr->rtext)) > curmax)     /* if this is longest field */
			curmax = newlen;
			
		// set record attributes
		tptr += sprintf(tptr,"%s",timestring(curptr->time));
		tptr += sprintf(tptr, " id=\"%d\"", curptr->num);
		if (*curptr->user)
			tptr += sprintf(tptr," user=\"%.4s\"",curptr->user);
		if (curptr->label)
			tptr += sprintf(tptr," label=\"%d\"",curptr->label);
		if (curptr->isdel)
			tptr += sprintf(tptr," deleted=\"y\"");
		if (curptr->isgen)
			sprintf(tptr," type=\"generated\"");
		wfptr += sprintf(wfptr, "<record%s>\n",recordstring); 
		for (fieldcnt = 0; fieldcnt < ftot; fieldcnt++)   { /* for all fields */
			if (fields[fieldcnt].ln || fieldcnt == ftot-1)	{	// if field has content or is locator
				font = style = color = 0;
				*fieldclass = '\0';
				// set field attributes
				
				if (fieldcnt == ftot-1)
					sprintf(fieldclass," class=\"locator\"");
				wfptr += sprintf(wfptr, "\t<field%s>", fieldclass);
				*fontstring = '\0';			// 4/17/2017
				for (sindex = 0, string = fields[fieldcnt].str; sindex < fields[fieldcnt].ln; sindex++)	{				
					if (iscodechar(string[sindex]))	{
//						*scapsstring = *stylestring = *fontstring = *colorstring = *offsetstring = '\0';
						*scapsstring = *stylestring = *colorstring = *offsetstring = '\0';	// 4/17/2017
						do	{	// while in code chars, accumulate composite code
							if (string[sindex++] == FONTCHR)	{
								if (string[sindex]&FX_COLOR)	{
									color = string[sindex]&FX_COLORMASK;
									if (color)
										// configure color value
										sprintf(colorstring, " color=\"%d\"",color);
								}
								else {
									font = string[sindex]&FX_FONTMASK;
//									if (font)
										sprintf(fontstring, " font=\"%d\"",font);
								}
							}
							else	{	// style code
								if (string[sindex]&FX_OFF)
									style &= ~(string[sindex]&FX_STYLEMASK);
								else
									style |= string[sindex];
							}
							if (style)	{
								char stylechars[7];
								int stindex = 0;
								memset(stylechars,0, sizeof(stylechars));
								if (style&FX_BOLD)
									stylechars[stindex++] = 'b';
								if (style&FX_ITAL)
									stylechars[stindex++] = 'i';
								if (style&FX_ULINE)
									stylechars[stindex++] = 'u';
#if 0
								if (style&FX_SMALL)
									stylechars[stindex++] = 's';
								if (style&FX_SUPER)
									stylechars[stindex++] = 'u';
								if (style&FX_SUB)
									stylechars[stindex++] = 'd';
#endif
								if (style&FX_SMALL)
									strcpy(scapsstring, " smallcaps=\"y\"");
								if (stindex)	// if any style chars to emit
									sprintf(stylestring, " style=\"%s\"",stylechars);
								if (style&FX_SUPER)
									strcpy(offsetstring," offset=\"u\"");
								else if (style&FX_SUB)
									strcpy(offsetstring," offset=\"d\"");
							}
							sindex++;
						} while (iscodechar(string[sindex]));
						wfptr += sprintf(wfptr,"<text%s%s%s%s%s/>",scapsstring,stylestring, offsetstring,fontstring, colorstring);
					}
					if (string[sindex]) {
						if ((tptr = strchr(special, string[sindex])))	{	// ~\{} <>
							switch (*tptr)	{
								case KEEPCHR:
									if (string[++sindex])	// if a char follows
										wfptr += sprintf(wfptr,specialstrings[tptr-special]);	// emit tag; follow through with char
									break;
								case ESCCHR:
									if (string[++sindex])	// if a char follows
										break;		// break to emit it
								case OBRACE:
								case CBRACE:
								case OBRACKET:
								case CBRACKET:
									wfptr += sprintf(wfptr,specialstrings[tptr-special]);	// emit tag and continue
								continue;
							}
						}
						if ((tptr = strchr(protected, string[sindex])))	// if protected char
							wfptr += sprintf(wfptr,"%s",protectedstrings[tptr-protected]);
						else			// normal write
							*wfptr++ = string[sindex];
					}
				}
				wfptr += sprintf(wfptr, "</field>\n");
			}
		}
		wfptr += sprintf(wfptr, "</record>\n");
	}
	wfptr += sprintf(wfptr, "</records>\n"); 
	wfptr += sprintf(wfptr, "</indexdata>\n");
	exp->records = rcount;
	exp->longest = curmax+1;
	return wfptr;
}
/******************************************************************************/
static char * writerawtext(char * fptr, char *sptr)	/* write field as plain text */

{	
	while (*sptr)	{			/* for all chars */
		if (iscodechar(*sptr))	{	/* if codechar */
			if (*++sptr)
				sptr++;			/* discard code */
		}
		else if (*sptr == ESCCHR || *sptr == KEEPCHR)	{	// if escaped char
			if (*++sptr)	// pass over it
				*fptr++ = *sptr++;	// force write of next char
		}
		else if (*sptr == OBRACKET || *sptr == CBRACKET)	// if open or close ignored text
			sptr++;		// discard
		else if (*sptr == OBRACE)
			sptr = str_skiptoclose(++sptr, CBRACE);	// skip to char after closing brace
		else
			*fptr++ = *sptr++;
	}
	return fptr;
}
#if 0
/******************************************************************************/
static void writefordos(FILE * fptr, char *sptr)	/* write field for DOS */

{
	int index, codeindex, transsymbol;
	unsigned char obuff[6];
	char *xptr;
	
	transsymbol = FALSE;
	// text is already converted to v2 form
	while (*sptr)	{	/* for all chars */
		if (*sptr == FONTCHR)	{
			if (*++sptr && ((*sptr++)&FX_FONTMASK) == 1)	/* if symbol font */
				transsymbol = TRUE;
			else
				transsymbol = FALSE;
		}
		else if (*sptr == CODECHR)	{
			if (*++sptr)	{		/* if a style code follows */
				for (index = 0, codeindex = 1; codeindex < FX_FONT; codeindex <<= 1, index++)	{
					if (*sptr&codeindex)	{	/* if this code is active */
						fputc('\\',fptr);	/* send code(s) */
						fputc(tr_escname[(index<<1) + (*sptr&FX_OFF ? 1 : 0)],fptr);	/* set code */
					}
				}
				sptr++;
			}
		}
		else {
			if (*sptr < 0)	{	/* extended char */
				obuff[0] = *sptr++;
				obuff[1] = '\0';
				CharToOemA(obuff,obuff);
				if (obuff[0] < SPACE)	/* if control character */
					sprintf(obuff,"\\%03.3d",(int)obuff[0]);	/* generate escape sequence */
				fputs(obuff,fptr);
			}
			else	{	/* ordinary character */
				if (transsymbol && (xptr = strchr(dos_greek,*sptr)))	/* if in symbol font and char for translation */
					fputc(dos_fromsymbol[xptr-dos_greek],fptr);
				else
					fputc(*sptr,fptr);
				sptr++;
			}
		}
	}
}
#else
/******************************************************************************/
static char * writefordos(char * fptr, char *sptr)	/* write field for DOS */

{
	int index, codeindex, transsymbol;
	unsigned char obuff[6];
	char *xptr;
	
	transsymbol = FALSE;
	// text is already converted to v2 form
	while (*sptr)	{	/* for all chars */
		if (*sptr == FONTCHR)	{
			if (*++sptr && ((*sptr++)&FX_FONTMASK) == 1)	/* if symbol font */
				transsymbol = TRUE;
			else
				transsymbol = FALSE;
		}
		else if (*sptr == CODECHR)	{
			if (*++sptr)	{		/* if a style code follows */
				for (index = 0, codeindex = 1; codeindex < FX_FONT; codeindex <<= 1, index++)	{
					if (*sptr&codeindex)	{	/* if this code is active */
						*fptr++ = '\\';	/* send code(s) */
						*fptr++ = tr_escname[(index<<1) + (*sptr&FX_OFF ? 1 : 0)];	/* set code */
					}
				}
				sptr++;
			}
		}
		else {
			if (*sptr < 0)	{	/* extended char */
				obuff[0] = *sptr++;
				obuff[1] = '\0';
				CharToOemA(obuff,obuff);
				if (obuff[0] < SPACE)	/* if control character */
					sprintf(obuff,"\\%03.3d",(int)obuff[0]);	/* generate escape sequence */
//				fputs(obuff,fptr);
				strcpy(fptr,obuff);
				fptr += strlen(fptr);
			}
			else	{	/* ordinary character */
				if (transsymbol && (xptr = strchr(dos_greek,*sptr)))	/* if in symbol font and char for translation */
					*fptr++ = dos_fromsymbol[xptr-dos_greek];
				else
					*fptr++ = *sptr;
				sptr++;
			}
		}
	}
	return fptr;
}
#endif
#if 0
/******************************************************************************/
static void writetext(FILE * fptr, char *sptr)	/* write field as plain text */

{	
	while (*sptr)	{			/* for all chars */
		if (iscodechar(*sptr))	{	/* if codechar */
			if (*++sptr)
				sptr++;			/* discard code */
		}
		else
			fputc(*sptr++,fptr);
	}
}
#else
/******************************************************************************/
static void writetext(FILE * fptr, char *sptr)	/* write field as plain text */

{	
	while (*sptr)	{			/* for all chars */
		if (iscodechar(*sptr))	{	/* if codechar */
			if (*++sptr)
				sptr++;			/* discard code */
		}
		else if (*sptr == ESCCHR || *sptr == KEEPCHR)	{	// if escaped char
			if (*++sptr)	// pass over it
				fputc(*sptr++,fptr);	// force write of next char
		}
		else if (*sptr == OBRACKET || *sptr == CBRACKET)	// if open or close ignored text
			sptr++;		// discard
		else if (*sptr == OBRACE)	{
			while (*++sptr && (*sptr != CBRACE || *(sptr-1) != ESCCHR || *(sptr-1) != KEEPCHR))	// while not done or not on unescaped closing brace
				;
		}
		else
			fputc(*sptr++,fptr);
	}
}
#endif
/******************************************************************************/
static char * timestring(time_c time)	// returns time string

{
	static char ts[40];
#if 1
	sprintf(ts," time=\"%s\"",time_stringFromTime(time, FALSE));
	return ts;
#else
	time_t ttime = time;
	struct tm * rectime = gmtime(&ttime);
	
	if (rectime) {
		sprintf(ts," time=\"%d-%02d-%02dT%02d:%02d:%02d\"",rectime->tm_year+1900,rectime->tm_mon+1,rectime->tm_mday,rectime->tm_hour,rectime->tm_min,rectime->tm_sec);
		return ts;
	}
	return " time=\"1970-01-01T00:00:00\"";
#endif
}
/******************************************************************************/
static char * xmlchartostring( char xc)		// appends character (escaped as necessary)

{
	static char cs[2];
	
	char * tptr;
	if ((tptr = strchr(protected, xc)))	// if protected char
		return (char *)protectedstrings[tptr-protected];
	else {		// normal write
		cs[0] = xc;
		cs[1] = '\0';
		return cs;
	}
}
