#include "stdafx.h"
#include <shlobj.h>
#include <htmlhelp.h>
#include "errors.h"
#include "commands.h"
#include "files.h"
#include "group.h"
#include "findset.h"
#include "viewset.h"
#include "modify.h"
#include "typestuff.h"
#include "search.h"
#include "index.h"
#include "records.h"
#include "sort.h"
#include "util.h"
#include "edit.h"
#include "utime.h"
#include "strings.h"
#include "registry.h"

static const DWORD wh_prefid[] = {
	IDC_PREFS_PGENCINMAX, HIDC_PREFS_PGENCINMAX,
	IDC_PREFS_PGENDIRECTORY, HIDC_PREFS_PGENDIRECTORY,
	IDC_PREFS_PGENDOCMAX, HIDC_PREFS_PGENDOCMAX,
	IDC_PREFS_PGENNOACTION,HIDC_PREFS_PGENNOACTION,
	IDC_PREFS_PGENOPEN,HIDC_PREFS_PGENOPEN,
	IDC_PREFS_PGENOPENPANEL,HIDC_PREFS_PGENOPENPANEL,
	IDC_PREFS_PGENTIME,HIDC_PREFS_PGENTIME,
	IDC_PREFS_PGENRECENT, HIDC_PREFS_PGENRECENT,
	IDC_PREFS_PGENUID,HIDC_PREFS_PGENUID,
	IDC_PREFS_PGENUIDTEXT,HIDC_PREFS_PGENUIDTEXT,
	IDC_PREFS_PGENUPDATE,HIDC_PREFS_PGENUPDATE,
	IDC_PREFS_PGENDIRECTORY,HIDC_PREFS_PGENDIRECTORY,

	IDC_PREFS_PEDIT_NEWWINDOW,HIDC_PREFS_PEDIT_NEWWINDOW,
	IDC_PREFS_PEDIT_SWITCHVIEW,HIDC_PREFS_PEDIT_SWITCHVIEW,
	IDC_PREFS_PEDITREMOVEDUP, HIDC_PREFS_PEDITREMOVEDUP,
	IDC_PREFS_PEDIT_SMARTFLIP,HIDC_PREFS_PEDIT_SMARTFLIP,
	IDC_PREFS_PEDIT_AUTORANGE,HIDC_PREFS_PEDIT_AUTORANGE,
	IDC_PREFS_PEDITTRACK,HIDC_PREFS_PEDITTRACK,
	IDC_PREFS_PEDITCARRY,HIDC_PREFS_PEDITCARRY,
	IDC_PREFS_PEDITPASTEALL, HIDC_PREFS_PEDITPASTEALL,
	IDC_PREFS_PEDITPASTESTYLE, HIDC_PREFS_PEDITPASTESTYLE,
	IDC_PREFS_PEDITPASTEPLAIN, HIDC_PREFS_PEDITPASTEPLAIN,
	IDC_PREFS_PEDITAUTOTYPE, HIDC_PREFS_PEDITAUTOTYPE,
	IDC_PREFS_PEDITIGNORESTYLE, HIDC_PREFS_PEDITIGNORESTYLE,
	IDC_PREFS_PEDITTRACKSOURCE,HIDC_PREFS_PEDITTRACKSOURCE,
	IDC_PREFS_PEDITPROPAGATE,HIDC_PREFS_PEDITPROPAGATE,
	IDC_PREFS_PEDITRETURN, HIDC_PREFS_PEDITRETURN,
	IDC_PREFS_PEDIT_LABELCHANGESDATE,HIDC_PREFS_PEDIT_LABELCHANGESDATE,
	
	IDC_PREFS_PEDITPASTEALL, HIDC_PREFS_PEDITPASTEALL,
	IDC_PREFS_PEDITPASTESTYLE, HIDC_PREFS_PEDITPASTESTYLE,
	IDC_PREFS_PEDITPASTEPLAIN,HIDC_PREFS_PEDITPASTEPLAIN,
	IDC_PREFS_PEDITASAVE, HIDC_PREFS_PEDITASAVE,
	IDC_PREFS_PEDITASK, HIDC_PREFS_PEDITASK,
	IDC_PREFS_PEDITDISCARD,HIDC_PREFS_PEDITDISCARD,
	IDC_PREFS_PEDITLACCEPT, HIDC_PREFS_PEDITLACCEPT,
	IDC_PREFS_PEDITLWARN, HIDC_PREFS_PEDITLWARN,
	IDC_PREFS_PEDITLFORBID, HIDC_PREFS_PEDITLFORBID,
	IDC_PREFS_PEDITCACCEPT, HIDC_PREFS_PEDITCACCEPT,
	IDC_PREFS_PEDITCWARN, HIDC_PREFS_PEDITCWARN,
	IDC_PREFS_PEDITCFORBID, HIDC_PREFS_PEDITCFORBID,
	IDC_PREFS_PEDITMACCEPT, HIDC_PREFS_PEDITMACCEPT,
	IDC_PREFS_PEDITMWARN, HIDC_PREFS_PEDITMWARN,
	IDC_PREFS_PEDITMFORBID, HIDC_PREFS_PEDITMFORBID,

	IDC_PREFS_PTEXTFONT,HIDC_PREFS_PTEXTFONT,
	IDC_PREFS_PTEXTSIZE ,HIDC_PREFS_PTEXTSIZE,
	IDC_PREFS_PRTEXTSIZE, HIDC_PREFS_PRTEXTSIZE,
	IDC_PREFS_PTEXTSAMPLE ,HIDC_PREFS_PTEXTSAMPLE,
	IDC_PREFS_PTEXTLABELC1 ,HIDC_PREFS_PTEXTLABELC1,
	IDC_PREFS_PTEXTLABELC2 ,HIDC_PREFS_PTEXTLABELC1,
	IDC_PREFS_PTEXTLABELC3 ,HIDC_PREFS_PTEXTLABELC1,
	IDC_PREFS_PTEXTLABELC4 ,HIDC_PREFS_PTEXTLABELC1,
	IDC_PREFS_PTEXTLABELC5 ,HIDC_PREFS_PTEXTLABELC1,
	IDC_PREFS_PTEXTLABELC6 ,HIDC_PREFS_PTEXTLABELC1,
	IDC_PREFS_PTEXTLABELC7 ,HIDC_PREFS_PTEXTLABELC1,
	IDC_PREFS_PTEXTFSHOWFORM, HIDC_PREFS_PTEXTFSHOWFORM,

	IDC_PREFS_PFORMSTYLE, HIDC_PREFS_PFORMSTYLE,
	IDC_PREFS_PFORMTAB, HIDC_PREFS_PFORMTAB,
	IDC_PREFS_PFORMOTHER, HIDC_PREFS_PFORMOTHER,
	IDC_PREFS_PFORMOTHERTEXT, HIDC_PREFS_PFORMOTHERTEXT,
	IDC_PREFS_PFORMEMBED, HIDC_PREFS_PFORMEMBED,
	IDC_PREFS_PFORM_UTF8, HIDC_PREFS_PFORM_UTF8,
	IDC_PREFS_PFORM_NATIVE, HIDC_PREFS_PFORM_NATIVE,
#ifdef PUBLISH
	IDC_PPREFS_FILE_SHOWCDX,HIDC_PPREFS_FILE_SHOWCDX,
	IDC_PPREFS_FILE_SHOWSTL,HIDC_PPREFS_FILE_SHOWCDX,
	IDC_PPREFS_FILE_SHOWCTG,HIDC_PPREFS_FILE_SHOWCDX,
	IDC_PPREFS_FILE_SHOWABR,HIDC_PPREFS_FILE_SHOWCDX,
	IDC_PPREFS_FILE_SHOWUDC,HIDC_PPREFS_FILE_SHOWCDX,
	IDC_PPREFS_FILE_SETCDX,HIDC_PPREFS_FILE_SETCDX,
	IDC_PPREFS_FILE_SETSTL,HIDC_PPREFS_FILE_SETCDX,
	IDC_PPREFS_FILE_SETCTG,HIDC_PPREFS_FILE_SETCDX,
	IDC_PPREFS_FILE_SETABR,HIDC_PPREFS_FILE_SETCDX,
	IDC_PPREFS_FILE_SETUDC,HIDC_PPREFS_FILE_SETCDX,
	IDC_PPREFS_ADM_PWD1,HIDC_PPREFS_ADM_PWD1,
	IDC_PPREFS_ADM_PWD2,HIDC_PPREFS_ADM_PWD1,
	IDC_PPREFS_ADM_REQID,HIDC_PPREFS_ADM_REQID,
	IDC_PPREFS_ADM_VALID,HIDC_PPREFS_ADM_VALID,
	IDC_PPREFS_ADM_IDFILE,HIDC_PPREFS_ADM_IDFILE,
//	IDC_PPREFS_ADM_TAB,HIDC_PPREFS_ADM_TAB,
	IDC_PPREFS_ADM_OPENDEF,HIDC_PPREFS_ADM_OPENDEF,
	IDC_PPREFS_ADM_SETDEF,HIDC_PPREFS_ADM_SETDEF,
	IDC_PPREFS_ADM_CHANGEDEF,HIDC_PPREFS_ADM_CHANGEDEF,
	IDC_PPREFS_ADM_READONLY,HIDC_PPREFS_ADM_READONLY,
	IDC_PPREFS_ADM_PERMITWRITE,HIDC_PPREFS_ADM_PERMITWRITE,
	IDC_PPREFS_ADM_MULTIPLE,HIDC_PPREFS_ADM_MULTIPLE,
#endif
	0,0
};

static TCHAR * pr_prefhelp = TEXT("Preferences");

static const DWORD wh_groupid[] = {
	IDC_MANAGEGROUPS_ALL, HIDC_MANAGEGROUPS_ALL,
	IDC_MANAGEGROUPS_DELETE, HIDC_MANAGEGROUPS_DELETE,
	IDC_MANAGEGROUPS_GROUP,HIDC_MANAGEGROUPS_GROUP,
	IDC_MANAGEGROUPS_INFO,HIDC_MANAGEGROUPS_INFO,
	IDC_MANAGEGROUPS_LINK, HIDC_MANAGEGROUPS_LINK,
	IDC_MANAGEGROUPS_LIST, HIDC_MANAGEGROUPS_GROUP,
	IDC_MANAGEGROUPS_REBUILD, HIDC_MANAGEGROUPS_REBUILD,
	0,0
};

static const DWORD wh_ginfoid[] = {
	IDC_GROUPINFO_ATTRIB, HIDC_GROUPINFO_ATTRIB,
	IDC_GROUPINFO_CASE, HIDC_GROUPINFO_CASE,
	IDC_GROUPINFO_CHANGES,HIDC_GROUPINFO_CHANGES,
	IDC_GROUPINFO_DATE,HIDC_GROUPINFO_DATE,
	IDC_GROUPINFO_EVAL, HIDC_GROUPINFO_EVAL,
	IDC_GROUPINFO_FIELD, HIDC_GROUPINFO_FIELD,
	IDC_GROUPINFO_METHOD, HIDC_GROUPINFO_METHOD,
	IDC_GROUPINFO_NAME, HIDC_GROUPINFO_NAME,
	IDC_GROUPINFO_PATTERN, HIDC_GROUPINFO_PATTERN,
	IDC_GROUPINFO_RECNUM,HIDC_GROUPINFO_RECNUM,
	IDC_GROUPINFO_TEXT,HIDC_GROUPINFO_TEXT,
	IDC_GROUPINFO_WORD, HIDC_GROUPINFO_WORD,
	0,0
};

#define	PIXELSTOPOINT ((float)1.)
#define	PIXELSTOPICA ((float)(PIXELSTOPOINT*12.))
#define	PIXELSTOINCH ((float)72.)
#define	PIXELSTOMM ((float)(PIXELSTOINCH/25.4))

#ifdef PUBLISH
struct cgen {
	struct genpref gen;
	PRIVATEPARAMS priv;
	HFONT fh;
	TCHAR cdxpath[MAX_PATH];
	TCHAR stlpath[MAX_PATH];
	TCHAR abrpath[MAX_PATH];
	TCHAR ctgpath[MAX_PATH];
	TCHAR udcpath[MAX_PATH];
	ADMIN adm;
};
#else
struct cgen {
	struct genpref gen;
	PRIVATEPARAMS priv;
	HFONT fh;
	TCHAR *dpath;
};
#endif //PUBLISH

struct ginfo {
	GROUPHANDLE gh;
	INDEX * FF;
};

static INT_PTR CALLBACK egenproc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK eeditproc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK etextproc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK eformproc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
#ifdef PUBLISH
static INT_PTR CALLBACK efolderproc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK eadminproc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static void	setupvalidate(HWND hwnd, ADMIN * adm);	/* fixes validation fields per id requirement */
static void setatabs(HWND hwnd, HWND th, ADMIN *ap);	/* sets admin tab items */
static void getatabs(HWND hwnd, HWND th, ADMIN *ap);	/* gets admin tab items */
static BOOL setuserfile(HWND hwnd, TCHAR * fpath);	/* gets user file name */
#endif //PUBLISH
static BOOL setdefdir(HWND hwnd, TCHAR * dpath, BOOL set);	/* sets default directory */
static INT_PTR CALLBACK dirhook(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static void displayfont(HWND hwnd, int id, HFONT * fhp, int type);	/* shows font sample */
static INT_PTR CALLBACK gnameproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK gmanageproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK showginfoproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

/**********************************************************************************/	
void edit_delrestore(HWND hwnd)	/* deletes/restores records */

{
	INDEX * FF = getowner(hwnd);
	RECORD * trptr;
	
	if (trptr = rec_getrec(FF,LX(hwnd,sel).first))	{/* get first record */
		BOOL delflag = !trptr->isdel;
		RECN lastnum = view_getsellimit(hwnd);	/* get record beyond end of selection */

		do {	/* for all records in range */
			trptr->isdel = delflag;
			rec_stamp(FF,trptr);	/* new in 1.5.4: deletion/insertion causes stamp */
		} while ((trptr = sort_skip(FF,trptr,1)) && trptr->num != lastnum);
		SendMessage(g_hwtoolbar,TB_CHECKBUTTON,IDM_EDIT_DELETE,delflag);	/* set button -- might have come from menu */
		if (FF->head.privpars.vmode == VM_FULL)	{	/* if in formatted display */
			view_redisplay(FF,0,VD_CUR|VD_TOP);		/* redisplay to show/hide */
			if (delflag)							/* if deleting */
				view_clearselect(FF->vwind);		/* selection will be invisible anyway */
			else if (trptr)							/* if restoring can only be from record window */
				view_selectrec(FF,trptr->num,VD_CUR,-1,-1);	/* select restored record */
		}
		else
			view_updatestatus(FF->vwind, TRUE);			/* just redisplay status */
	}
}
/**********************************************************************************/	
void edit_switchtag(HWND hwnd,int newlabel)	/* tags/untags records */

{
	INDEX * FF = getowner(hwnd);
	RECORD * trptr;
	
	if (trptr = rec_getrec(FF,LX(hwnd,sel).first))	{	/* get first record */
		RECN lastnum = view_getsellimit(hwnd);	/* get record beyond end of selection */
		BOOL apply = trptr->label != newlabel && newlabel;
		
		do {	/* for all records in range */
			if (apply)	// if want to label
				trptr->label = newlabel;	// apply it
			else if (trptr->label && (trptr->label == newlabel || !newlabel))	// if has label to be removed
				trptr->label = 0;
			else			// don't touch this one
				continue;
			if (g_prefs.gen.labelsetsdate)
				rec_stamp(FF,trptr);
		} while ((trptr = sort_skip(FF,trptr,1)) && trptr->num != lastnum);
		SendMessage(g_hwtoolbar,TB_CHECKBUTTON,IDM_EDIT_TAG,newlabel > 0);	/* set button -- might have come from menu */
		if (FF->head.privpars.vmode != VM_FULL)	/* if draft */
			view_updatestatus(FF->vwind,FALSE);	/* redisplay */
		else if (g_prefs.gen.showlabel)			/* if fully formatted */
			view_resetrec(FF,LX(FF->vwind,sel).first);	/* reset from first record */
	}
}
/**********************************************************************************/
void edit_removemark(HWND hwnd)		// removes marks from records

{
	INDEX * FF = getowner(hwnd);
	RECORD * trptr;

	if (trptr = rec_getrec(FF, LX(hwnd, sel).first)) {/* get first record */
		RECN lastnum = view_getsellimit(hwnd);	/* get record beyond end of selection */

		do {	/* for all records in range */
			if (trptr->ismark) {
				trptr->ismark = FALSE;
				if (!FF->head.dirty) {	/* if haven't marked index as dirty */
					FF->head.dirty = TRUE;	/* index is dirty */
					index_writehead(FF);	/* write header marked as dirty */
				}
			}
		} while ((trptr = sort_skip(FF, trptr, 1)) && trptr->num != lastnum);
		if (FF->head.privpars.vmode != VM_FULL) 	// if not formatted view
			view_updatestatus(FF->vwind, TRUE);		// redisplay staus
	}
}
/**********************************************************************************/
short edit_duplicate(HWND hwnd)	/* duplicates record(s) */

{
	INDEX * FF;
	RECORD * trptr;
	GROUPHANDLE gh;
	RECN rnum, fgnum;
	COUNTPARAMS cs;
	RECN tot;
	HCURSOR ocurs;
	LFLIST * lfp;
	char propstate;
	
	FF = getowner(hwnd);
	if (hwnd == FF->vwind)	{		/* if duplicate from view window */
		lfp = getdata(hwnd);
		memset(&cs,0,sizeof(cs));
		cs.smode = FF->head.sortpars.ison;		/* sort is as the view */
		if (!com_getrecrange(FF,COMR_SELECT,NULL,NULL,&cs.firstrec, &cs.lastrec)) 	{	/* if range is ok */
			tot = search_count(FF, &cs, SF_OFF);
			if (FF->head.privpars.hidedelete)	/* if won't duplicate deleted */
				tot -= cs.deleted;				/* adjust required count */
			if (tot) 	{		/* if have any records */
				if (index_setworkingsize(FF,tot+MAPMARGIN))		{
					ocurs = SetCursor(g_waitcurs);
					for (rnum = FF->head.rtot, trptr = rec_getrec(FF,lfp->sel.first); trptr && trptr->num != cs.lastrec; trptr = sort_skip(FF,trptr,1))	{
						if (!rec_makenew(FF,trptr->rtext,++rnum))	/* if error making new record */
							break;
					}
					fgnum = FF->head.rtot+1;		/* save first new record # in case make group */
					while (FF->head.rtot < rnum)	/* for new records */
						sort_makenode(FF,++FF->head.rtot);		/* sort */
					lfp->sel.first = lfp->sel.last = 0;
					index_flush(FF);			/* force update on file */
					view_setstatus(FF->vwind);
					if (tot > 1)	{		/* if more than one record duplicated */
						gh = grp_startgroup(FF);
						if (grp_buildfromrange(FF,&gh,fgnum,FF->head.rtot,GF_RANGE))	{	/* build group from range */
							grp_installtemp(FF,gh);
							SendMessage(FF->vwind,WM_COMMAND,IDM_VIEW_TEMPORARYGROUP,0);	/* display group */
							view_selectrec(FF,gh->recbase[0],VD_SELPOS,-1,-1);
						}
						else
							grp_dispose(gh);
					}
					else if (trptr = rec_getrec(FF,FF->head.rtot))	{	/* if can get record */
						propstate = g_prefs.gen.propagate;	/* save prop state */
						g_prefs.gen.propagate = FALSE;		/* set off */
						mod_settext(FF,trptr->rtext,trptr->num, NULL);
						g_prefs.gen.propagate = propstate;	/* restore */
					}
					SetCursor(ocurs);
				}
				else
					senderr(ERR_DISKFULLERR,WARN);		/* not enough disk space */
			}
		}
	}
	else		/* duplicate currently open record */
		mod_settext(FF,OLDTEXT,NEWREC, NULL);	/* make record from whatever is in window */
	return (0);
}
/**********************************************************************************/
void edit_demote(HWND hwnd)	// demotes headings

{
	INDEX * FF = getowner(hwnd);
	LFLIST * lfp = getdata(hwnd);
	RECN lastnum = view_getsellimit(hwnd);	// record beyond end of selection
	RECN firstnum = lfp->sel.first;		// and first selected
//	struct selstruct selx = lfp->sel;	// need to keep a copy in case of resizing
	RECORD * trptr;

	if (trptr = rec_getrec(FF, firstnum)) {	// check record lengths, depths for fit
		int maxlength = 0;
		int maxfields = 0;
		char placeholder[60];
		int pgap = sprintf(placeholder, "__%s__", time_stringFromTime(time(NULL), TRUE)) +1;	// get heading and gap
		do {
			int length = str_xlen(trptr->rtext);
			if (length > maxlength)
				maxlength = length;
			int fcount = str_xcount(trptr->rtext);
			if (fcount > maxfields)
				maxfields = fcount;
		} while ((trptr = sort_skip(FF, trptr, 1)) && trptr->num != lastnum);
		if (maxfields > FF->head.indexpars.maxfields) {	/* if need to increase field limit */
			if (sendwarning(WARN_DEMOTEFIELD, maxfields)) {
				int oldmaxfieldcount = FF->head.indexpars.maxfields;
				FF->head.indexpars.maxfields = maxfields;
				adjustsortfieldorder(FF->head.sortpars.fieldorder, oldmaxfieldcount, FF->head.indexpars.maxfields);
			}
			else
				return;
		}
		maxlength = maxlength + pgap + 10;
		maxlength -= maxlength % 10;			// rounded up to nearest 10 above original maxlength + gap
		if (maxlength > FF->head.indexpars.recsize) {	/* if need record enlargement */
			if (!sendwarning(WARN_DEMOTEENLARGE, maxlength - FF->head.indexpars.recsize) || !file_resizeindex(FF, maxlength))	// if don't want or can't do
				return;
		}
		struct numstruct * slptr = sort_setuplist(FF);
		trptr = rec_getrec(FF, firstnum);
		do {
			memmove(trptr->rtext + pgap, trptr->rtext, str_xlen(trptr->rtext) + 1);	// create space for heading
			strcpy(trptr->rtext, placeholder);
			sort_addtolist(slptr, trptr->num);
			rec_stamp(FF, trptr);
		} while ((trptr = sort_skip(FF, trptr, 1)) && trptr->num != lastnum);
		sort_resortlist(FF, slptr);
		view_resetrec(FF, firstnum);	/* reset from first record */
		trptr = rec_getrec(FF, firstnum);
		mod_settext(FF, trptr->rtext, trptr->num, NULL);
		mod_selectfield(FF->rwind, 0);
	}
}
#ifdef PUBLISH
/**********************************************************************************/	
void edit_preferences(void)	/* sets preferences */

{
#define NPAGES 6

	PROPSHEETHEADER psh;
	PROPSHEETPAGE psp[NPAGES];
	int count, pagecount;
	struct cgen tcpref;
	INDEX * FF;

	memset(&tcpref,0,sizeof(tcpref));
	tcpref.gen = g_prefs.gen;	/* gen prefs */
	tcpref.priv = g_prefs.privpars;	/* private pars */
	tcpref.adm = g_admin;		/* admin pars */
	file_getmachinepath(tcpref.cdxpath,ALIS_CDXDIR);	/* default index directory */
	file_getmachinepath(tcpref.stlpath,ALIS_STLDIR);	/* default style directory */
	file_getmachinepath(tcpref.ctgpath,ALIS_CTGDIR);	/* default tag directory */
	file_getmachinepath(tcpref.abrpath,ALIS_ABRDIR);	/* default abbrev directory */
	file_getmachinepath(tcpref.udcpath,ALIS_UDCDIR);	/* default udic directory */
	memset(psp,0,sizeof(psp));
	pagecount = NPAGES;
	if (nstrcmp(g_admin.psswd,g_password))		/* if not admin privileges */
		pagecount -= 1;		/* remove admin page */
	for (count = 0; count < pagecount; count++)	{
		psp[count].dwSize = sizeof(PROPSHEETPAGE);
		psp[count].dwFlags = PSP_HASHELP;
		psp[count].hInstance = g_hinst;
		psp[count].pszTemplate = MAKEINTRESOURCE(count+IDD_PREFS_PGEN);
		psp[count].lParam = (LONG_PTR)&tcpref;
	}
	psp[0].pfnDlgProc = egenproc;
	psp[1].pfnDlgProc = eeditproc;
	psp[2].pfnDlgProc = etextproc;
	psp[3].pfnDlgProc = eformproc;
	psp[4].pfnDlgProc = efolderproc;
	psp[5].pfnDlgProc = eadminproc;

	memset(&psh,0,sizeof(psh));
	psh.dwSize = sizeof(psh);
	psh.dwFlags = PSH_PROPSHEETPAGE|PSH_HASHELP|PSH_NOAPPLYNOW;
	psh.hwndParent = g_hwframe;
	psh.hInstance = g_hinst;
	psh.pszCaption = TEXT("Preferences");
	psh.nPages = pagecount;
	psh.ppsp = psp;

	if (PropertySheet(&psh))	{		/* ok */
		g_prefs.gen = tcpref.gen;
		g_prefs.privpars = tcpref.priv;
		g_admin = tcpref.adm;
		file_savemachinepath(tcpref.cdxpath,ALIS_CDXDIR);	/* save the default path */
		file_savemachinepath(tcpref.stlpath,ALIS_STLDIR);	/* save the style path */
		file_savemachinepath(tcpref.ctgpath,ALIS_CTGDIR);	/* save the tag path */
		file_savemachinepath(tcpref.abrpath,ALIS_ABRDIR);	/* save the abr path */
		file_savemachinepath(tcpref.udcpath,ALIS_UDCDIR);	/* save the udc path */
		reg_setmachinekeyvalue(K_ADMIN,AGENERAL,REG_BINARY,&tcpref.adm,sizeof(ADMIN));	// save admin settings
		for (count = 0; count < FLAGLIMIT; count++)	{	// check changed colors
			if (g_prefs.gen.flagcolor[count] != tcpref.gen.flagcolor[count])
				break;
		}
		if (g_prefs.gen.showlabel != tcpref.gen.showlabel || count < FLAGLIMIT)	{
			for (FF = g_indexbase; FF; FF = FF->inext)	{	/* for all indexes */
				if (FF->vwind)	/* if has window */
					view_redisplay(FF,0,VD_CUR);	/* force repaint */
			}
		}
		if (!g_indexbase)	/* if no indexes open */
			com_settextmenus(NULL,OFF,OFF);			/* display default font/size */
	}
	if (tcpref.fh)		/* if have used a font */
		DeleteFont(tcpref.fh);	/* get rid of it */
}
#else
/**********************************************************************************/	
void edit_preferences(void)	/* sets preferences */

{
#define NPAGES 4

	PROPSHEETHEADER psh;
	PROPSHEETPAGE psp[NPAGES];
	int count;
	struct cgen tcpref;
	TCHAR tdirpath[MAX_PATH];
	INDEX * FF;

	tcpref.gen = g_prefs.gen;	/* gen prefs */
	tcpref.priv = g_prefs.privpars;	/* private pars */
	tcpref.fh = NULL;			/* font handle for display */
	tcpref.dpath = tdirpath;
	file_getuserpath(tdirpath,ALIS_DEFAULTDIR);	/* initialize the default path */
	memset(psp,0,sizeof(psp));
	for (count = 0; count < NPAGES; count++)	{
		psp[count].dwSize = sizeof(PROPSHEETPAGE);
		psp[count].dwFlags = PSP_HASHELP;
		psp[count].hInstance = g_hinst;
		psp[count].pszTemplate = MAKEINTRESOURCE(count+IDD_PREFS_PGEN);
		psp[count].lParam = (LONG_PTR)&tcpref;
	}
	psp[0].pfnDlgProc = egenproc;
	psp[1].pfnDlgProc = eeditproc;
	psp[2].pfnDlgProc = etextproc;
	psp[3].pfnDlgProc = eformproc;

	memset(&psh,0,sizeof(psh));
	psh.dwSize = sizeof(psh);
	psh.dwFlags = PSH_PROPSHEETPAGE|PSH_HASHELP|PSH_NOAPPLYNOW;
	psh.hwndParent = g_hwframe;
	psh.hInstance = g_hinst;
	psh.pszCaption = TEXT("Preferences");
	psh.nPages = NPAGES;
	psh.ppsp = psp;

	if (PropertySheet(&psh))	{		/* ok */
		for (count = 0; count < FLAGLIMIT; count++)	{	// check changed colors
			if (g_prefs.gen.flagcolor[count] != tcpref.gen.flagcolor[count])
				break;
		}
		g_prefs.gen = tcpref.gen;
		g_prefs.privpars = tcpref.priv;
		file_saveuserpath(tdirpath,ALIS_DEFAULTDIR);	/* save the default path */
		SetCurrentDirectory(tdirpath);
		if (g_prefs.gen.showlabel != tcpref.gen.showlabel || count < FLAGLIMIT)	{
			makelabelbitmaps();	// reset label menu bitmaps
			for (FF = g_indexbase; FF; FF = FF->inext)	{	/* for all indexes */
				if (FF->vwind)	/* if has window */
					view_redisplay(FF,0,VD_CUR);	/* force repaint */
			}
		}
		if (!g_indexbase)	/* if no indexes open */
			com_settextmenus(NULL,OFF,OFF);			/* display default font/size */
		file_saveconfig();		/* save new configuration */
	}
	if (tcpref.fh)		/* if have used a font */
		DeleteFont(tcpref.fh);	/* get rid of it */
}
#endif
/**********************************************************************************/	
static INT_PTR CALLBACK egenproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)

{
	struct genpref * tgp = getdata(hwnd);
	HWND cbh;
	int count;
	char tbuff[STSTRING];

	switch (msg)	{

		case WM_INITDIALOG:
			if (tgp = setdata(hwnd,(void *)((LPPROPSHEETPAGE)lParam)->lParam))	{	/* set data */
				HWND ewnd;

#ifdef PUBLISH
				hideitem(hwnd,IDC_PREFS_PGENUID);
				hideitem(hwnd,IDC_PREFS_PGENDIRECTORY);
				if (g_admin.validate)	// if need validation of user id
					disableitem(hwnd,IDC_PREFS_PGENUIDTEXT);	// disable	
#endif 
				CheckRadioButton(hwnd,IDC_PREFS_PGENNOACTION,IDC_PREFS_PGENOPENPANEL,IDC_PREFS_PGENNOACTION+tgp->openflag);
				checkitem(hwnd,IDC_PREFS_PGENUID,tgp->setid);		/* user id on start */
				checkitem(hwnd,IDC_PREFS_PGENCINMAX,tgp->maxcin);		/* maximize cindex on start */
				checkitem(hwnd,IDC_PREFS_PGENDOCMAX,tgp->maxdoc);		/* maximize indexes on start */
				setint(hwnd,IDC_PREFS_PGENTIME,tgp->saveinterval/60);	/* save interval in minutes */
				ewnd = GetDlgItem(hwnd,IDC_PREFS_PGENUIDTEXT);
				Edit_LimitText(ewnd,4);		/* limit to 4 chars */
				SetWindowText(ewnd,toNative(g_prefs.hidden.user));
				checkitem(hwnd,IDC_PREFS_PGENUPDATE,tgp->autoupdate);	// checks for update automatically
				cbh = GetDlgItem(hwnd,IDC_PREFS_PGENRECENT);
				for (count = MINRECENT; count <= MAXRECENT; count++)	{
					_itoa(count,tbuff,10);
					ComboBox_AddString(cbh,toNative(tbuff));
				}
				ComboBox_SetCurSel(cbh,tgp->recentlimit-MINRECENT);
				SetFocus(GetDlgItem(hwnd,IDC_PREFS_PGENTIME));	/* set focus to text */
				centerwindow(GetParent(hwnd),0);		/* need to center whole prop sheet at this stage (when init first page) */
			}
			return (FALSE);
		case WM_COMMAND:
			if (HIWORD(wParam) == EN_UPDATE)	{
				WORD id = LOWORD(wParam);
				int length = 0;

				if (id == IDC_PREFS_PGENUIDTEXT)
					length = 5;
				checktextfield((HWND)lParam,length);
				return TRUE;
			}
#ifndef PUBLISH
			if (LOWORD(wParam) == IDC_PREFS_PGENDIRECTORY)	{
				setdefdir(hwnd,((struct cgen *)tgp)->dpath, TRUE);
				return (TRUE);
			}
#endif
			return FALSE;
		case WM_NOTIFY:
			switch (((NMHDR *)lParam)->code)	{
				case PSN_KILLACTIVE:
					tgp->openflag = findgroupcheck(hwnd,IDC_PREFS_PGENNOACTION,IDC_PREFS_PGENOPENPANEL)-IDC_PREFS_PGENNOACTION;
					tgp->setid = (char)isitemchecked(hwnd,IDC_PREFS_PGENUID);
					tgp->maxcin = (char)isitemchecked(hwnd,IDC_PREFS_PGENCINMAX);
					tgp->maxdoc = (char)isitemchecked(hwnd,IDC_PREFS_PGENDOCMAX);
					tgp->autoupdate = (char)isitemchecked(hwnd,IDC_PREFS_PGENUPDATE);
					tgp->recentlimit = (char)ComboBox_GetCurSel(GetDlgItem(hwnd,IDC_PREFS_PGENRECENT))+MINRECENT;
					getshort(hwnd,IDC_PREFS_PGENTIME,&tgp->saveinterval);
					tgp->saveinterval *=60;
					getDItemText(hwnd,IDC_PREFS_PGENUIDTEXT,g_prefs.hidden.user,5);
					SetWindowLongPtr(hwnd, DWLP_MSGRESULT,FALSE);
					return (TRUE);		/* check our results; SetwindowLong TRUE to hold page; FALSE to free */
				case PSN_HELP:
					dowindowhelp(TEXT("base\\Prefs_gen.htm"));
					return (TRUE);
			}
			break;
		case WM_HELP:
			return (dodialoghelp(hwnd,TEXT("base\\Prefs_gen.htm"),(HELPINFO *)lParam,wh_prefid));
	}
	return (FALSE);
}
/**********************************************************************************/	
static INT_PTR CALLBACK eeditproc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)

{
	struct genpref * tgp;

	tgp = getdata(hwnd);
	switch (msg)	{

		case WM_INITDIALOG:
			if (tgp = setdata(hwnd,(void *)((LPPROPSHEETPAGE)lParam)->lParam))	{	/* set data */
				checkitem(hwnd,IDC_PREFS_PEDITCARRY,tgp->carryrefs);	/* cary page refs */
				checkitem(hwnd,IDC_PREFS_PEDITPROPAGATE,tgp->propagate);	/* propagate */
				checkitem(hwnd,IDC_PREFS_PEDITTRACK,tgp->track);	/* track new entries */
				checkitem(hwnd,IDC_PREFS_PEDITAUTOTYPE,tgp->autoextend);	/* auto-typing */
				if (tgp->autoextend)	{
					checkitem(hwnd,IDC_PREFS_PEDITIGNORESTYLE,tgp->autoignorecase);	/* case-sensitivity of autotyping */
					checkitem(hwnd,IDC_PREFS_PEDITTRACKSOURCE,tgp->tracksource);	/* source tracking */
				}
				else	{
					disableitem(hwnd,IDC_PREFS_PEDITIGNORESTYLE);	/* case-sensitivity of autotyping */				
					disableitem(hwnd,IDC_PREFS_PEDITTRACKSOURCE);	/* source tracking */
				}
				checkitem(hwnd,IDC_PREFS_PEDITRETURN,tgp->vreturn);	/* return to entry point */
				checkitem(hwnd,IDC_PREFS_PEDIT_LABELCHANGESDATE,tgp->labelsetsdate);	// label change sets date stamp
				checkitem(hwnd,IDC_PREFS_PEDIT_NEWWINDOW,tgp->newwindow);	/* open new window */
				checkitem(hwnd,IDC_PREFS_PEDIT_SWITCHVIEW,tgp->switchview);	/* switch to draft view */
				checkitem(hwnd,IDC_PREFS_PEDITREMOVEDUP,tgp->remspaces);	/* remove duplicate spaces */
				checkitem(hwnd,IDC_PREFS_PEDIT_SMARTFLIP,tgp->smartflip);	// smart flip
				checkitem(hwnd,IDC_PREFS_PEDIT_AUTORANGE,tgp->autorange);	// auto creat locator range
				CheckRadioButton(hwnd,IDC_PREFS_PEDITASAVE,IDC_PREFS_PEDITDISCARD,IDC_PREFS_PEDITASAVE+tgp->saverule);
				CheckRadioButton(hwnd, IDC_PREFS_PEDITPASTEALL, IDC_PREFS_PEDITPASTEPLAIN, IDC_PREFS_PEDITPASTEALL + tgp->pastemode);
				CheckRadioButton(hwnd,IDC_PREFS_PEDITLACCEPT,IDC_PREFS_PEDITLFORBID,IDC_PREFS_PEDITLACCEPT+tgp->pagealarm);
				CheckRadioButton(hwnd,IDC_PREFS_PEDITCACCEPT,IDC_PREFS_PEDITCFORBID,IDC_PREFS_PEDITCACCEPT+tgp->crossalarm);
				CheckRadioButton(hwnd,IDC_PREFS_PEDITMACCEPT,IDC_PREFS_PEDITMFORBID,IDC_PREFS_PEDITMACCEPT+tgp->templatealarm);
			}
			return (TRUE);
		case WM_NOTIFY:
			switch (((NMHDR *)lParam)->code)	{
				case PSN_KILLACTIVE:
					tgp->carryrefs = (char)isitemchecked(hwnd,IDC_PREFS_PEDITCARRY);
					tgp->propagate = (char)isitemchecked(hwnd,IDC_PREFS_PEDITPROPAGATE);
					tgp->track = (char)isitemchecked(hwnd,IDC_PREFS_PEDITTRACK);
					tgp->autoextend = (char)isitemchecked(hwnd,IDC_PREFS_PEDITAUTOTYPE);
					tgp->autoignorecase = (char)isitemchecked(hwnd,IDC_PREFS_PEDITIGNORESTYLE);
					tgp->tracksource = (char)isitemchecked(hwnd,IDC_PREFS_PEDITTRACKSOURCE);
					tgp->saverule = findgroupcheck(hwnd,IDC_PREFS_PEDITASAVE,IDC_PREFS_PEDITDISCARD)-IDC_PREFS_PEDITASAVE;
					tgp->pastemode = findgroupcheck(hwnd, IDC_PREFS_PEDITPASTEALL, IDC_PREFS_PEDITPASTEPLAIN) - IDC_PREFS_PEDITPASTEALL;
					tgp->pagealarm = findgroupcheck(hwnd,IDC_PREFS_PEDITLACCEPT,IDC_PREFS_PEDITLFORBID)-IDC_PREFS_PEDITLACCEPT;
					tgp->crossalarm = findgroupcheck(hwnd,IDC_PREFS_PEDITCACCEPT,IDC_PREFS_PEDITCFORBID)-IDC_PREFS_PEDITCACCEPT;
					tgp->templatealarm = findgroupcheck(hwnd,IDC_PREFS_PEDITMACCEPT,IDC_PREFS_PEDITMFORBID)-IDC_PREFS_PEDITMACCEPT;
					tgp->track = (char)isitemchecked(hwnd,IDC_PREFS_PEDITTRACK);
					tgp->vreturn = (char)isitemchecked(hwnd,IDC_PREFS_PEDITRETURN);
					tgp->labelsetsdate = (char)isitemchecked(hwnd,IDC_PREFS_PEDIT_LABELCHANGESDATE);
					tgp->switchview = (char)isitemchecked(hwnd,IDC_PREFS_PEDIT_SWITCHVIEW);
					tgp->remspaces = (char)isitemchecked(hwnd,IDC_PREFS_PEDITREMOVEDUP);
					tgp->smartflip = (char)isitemchecked(hwnd,IDC_PREFS_PEDIT_SMARTFLIP);
					tgp->autorange = (char)isitemchecked(hwnd,IDC_PREFS_PEDIT_AUTORANGE);
					tgp->newwindow = (char)isitemchecked(hwnd,IDC_PREFS_PEDIT_NEWWINDOW);
					SetWindowLongPtr(hwnd, DWLP_MSGRESULT,FALSE);
					return (TRUE);		/* check our results; SetwindowLong TRUE to hold page; FALSE to free */
				case PSN_HELP:
					dowindowhelp(TEXT("base\\Prefs_edit.htm"));
					return (TRUE);
			}
			break;
		case WM_HELP:
			return (dodialoghelp(hwnd,TEXT("base\\Prefs_edit.htm"),(HELPINFO *)lParam,wh_prefid));
		case WM_COMMAND:
			switch (LOWORD(wParam))	{
				case IDC_PREFS_PEDITAUTOTYPE:
					if (isitemchecked(hwnd,IDC_PREFS_PEDITAUTOTYPE))	{	/* if auto-typing */
						enableitem(hwnd,IDC_PREFS_PEDITIGNORESTYLE);	/* case-sensitivity of autotyping */
						enableitem(hwnd,IDC_PREFS_PEDITTRACKSOURCE);	/* source tracking */
					}
					else	{
						disableitem(hwnd,IDC_PREFS_PEDITIGNORESTYLE);	/* case-sensitivity of autotyping */				
						checkitem(hwnd,IDC_PREFS_PEDITIGNORESTYLE,FALSE);	/* case-sensitivity of autotyping */
						disableitem(hwnd,IDC_PREFS_PEDITTRACKSOURCE);	/* source tracking */
						checkitem(hwnd,IDC_PREFS_PEDITTRACKSOURCE,FALSE);	/* source tracking */
					}
					break;
			}
	}
	return (FALSE);
}
/**********************************************************************************/	
static INT_PTR CALLBACK etextproc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
// !! find way to use DeleteObject to delete brushes after use
{
	struct cgen * tgc;
	TCHAR fname[FONTNAMELEN];
	TCHAR sbuff[32];

	tgc = getdata(hwnd);
	switch (msg)	{

		case WM_INITDIALOG:
			if (tgc = setdata(hwnd,(void *)((LPPROPSHEETPAGE)lParam)->lParam))	{	/* set data */
				type_setfontnamecombo(hwnd,IDC_PREFS_PTEXTFONT,tgc->gen.fm[0].pname);
				type_setfontsizecombo(GetDlgItem(hwnd,IDC_PREFS_PTEXTSIZE),tgc->priv.size);
				type_setfontsizecombo(GetDlgItem(hwnd,IDC_PREFS_PRTEXTSIZE),tgc->gen.recordtextsize);
				displayfont(hwnd,IDC_PREFS_PTEXTFONT,&tgc->fh,CBN_EDITCHANGE);
				checkitem(hwnd,IDC_PREFS_PTEXTFSHOWFORM,tgc->gen.showlabel);	/* show labels in formatted */
			}
			return (TRUE);
		case WM_NOTIFY:
			switch (((NMHDR *)lParam)->code)	{
				case PSN_KILLACTIVE:
					tgc->gen.showlabel = (char)isitemchecked(hwnd,IDC_PREFS_PTEXTFSHOWFORM);
					GetWindowText(GetDlgItem(hwnd,IDC_PREFS_PTEXTFONT),fname,FONTNAMELEN);
					strcpy(tgc->gen.fm[0].pname,fromNative(fname));
					strcpy(tgc->gen.fm[0].name,tgc->gen.fm[0].pname);		/* make alternate same as preferred */
					GetWindowText(GetDlgItem(hwnd,IDC_PREFS_PTEXTSIZE),sbuff,32);
//					if ((tgc->priv.size = (short)natoi(sbuff)) > 0 )	/* if a good number */
					SetWindowLongPtr(hwnd, DWLP_MSGRESULT,FALSE);	// assume clean font sizes
					if (!getshort(hwnd,IDC_PREFS_PTEXTSIZE,&tgc->priv.size) || tgc->priv.size < 0)	{
						SetFocus(GetDlgItem(hwnd,IDC_PREFS_PTEXTSIZE));
						SetWindowLongPtr(hwnd, DWLP_MSGRESULT,TRUE);
					}
					if (!getshort(hwnd,IDC_PREFS_PRTEXTSIZE,&tgc->gen.recordtextsize) || tgc->gen.recordtextsize < 0)	{
						SetFocus(GetDlgItem(hwnd,IDC_PREFS_PRTEXTSIZE));
						SetWindowLongPtr(hwnd, DWLP_MSGRESULT,TRUE);
					}
					return (TRUE);
				case PSN_HELP:
					dowindowhelp(TEXT("base\\Prefs_view.htm"));
					return (TRUE);
			}
			break;
		case WM_CTLCOLORSTATIC:
			{
				int id = GetDlgCtrlID((HWND)lParam);
				if (id >= IDC_PREFS_PTEXTLABELC1 && id <= IDC_PREFS_PTEXTLABELC7)		// if for color
					return (INT_PTR)CreateSolidBrush(tgc->gen.flagcolor[id-IDC_PREFS_PTEXTLABELC1+1]);
			}
			return FALSE;
		case WM_COMMAND:
			switch (LOWORD(wParam))	{
				case IDC_PREFS_PTEXTFONT:
					if (HIWORD(wParam) == CBN_SELENDOK)
						displayfont(hwnd,LOWORD(wParam),&tgc->fh,CBN_EDITCHANGE);
					return TRUE;
				case IDC_PREFS_PTEXTSIZE:
				case IDC_PREFS_PRTEXTSIZE:
					if (HIWORD(wParam) == CBN_EDITCHANGE || HIWORD(wParam) == CBN_SELENDOK)
						displayfont(hwnd,LOWORD(wParam),&tgc->fh,HIWORD(wParam));
					return TRUE;
				default:
					if (HIWORD(wParam) == STN_DBLCLK){
						int id = LOWORD(wParam);
						if (id >= IDC_PREFS_PTEXTLABELC1 && id <= IDC_PREFS_PTEXTLABELC7)	{
							CHOOSECOLOR cs;
							memset(&cs,0,sizeof(CHOOSECOLOR));
							cs.lStructSize = sizeof(CHOOSECOLOR);
							cs.hwndOwner = hwnd;
							cs.rgbResult = tgc->gen.flagcolor[id-IDC_PREFS_PTEXTLABELC1+1];
							cs.lpCustColors = tgc->gen.custcolors;
							cs.Flags = CC_RGBINIT;
							if (ChooseColor(&cs))	{
								tgc->gen.flagcolor[id-IDC_PREFS_PTEXTLABELC1+1] = cs.rgbResult;
								RedrawWindow((HWND)lParam,NULL,NULL,RDW_INVALIDATE);
							}
							return TRUE;
						}
					}
			}
			break;
		case WM_HELP:
			return (dodialoghelp(hwnd,TEXT("base\\Prefs_view.htm"),(HELPINFO *)lParam,wh_prefid));
	}
	return (FALSE);
}
/**********************************************************************************/	
static INT_PTR CALLBACK eformproc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)

{
	struct genpref * tgp;
	int stylemode;
	char tbuff[2];

	tgp = getdata(hwnd);

	switch (msg)	{
		case WM_INITDIALOG:
			if (tgp = setdata(hwnd,(void *)((LPPROPSHEETPAGE)lParam)->lParam))	{	/* set data */
				if (tgp->indentdef)	{
					if (tgp->indentdef == '\t')
						stylemode = 1;
					else	{
						stylemode = 2;
						dprintf(hwnd,IDC_PREFS_PFORMOTHERTEXT, "%c",tgp->indentdef);
					}
				}
				else
					stylemode = 0;
				Edit_LimitText(GetDlgItem(hwnd,IDC_PREFS_PFORMOTHERTEXT),1);		/* limit to 1 char */
				CheckRadioButton(hwnd,IDC_PREFS_PFORMSTYLE,IDC_PREFS_PFORMOTHER,IDC_PREFS_PFORMSTYLE+stylemode);
				checkitem(hwnd,IDC_PREFS_PFORMEMBED,tgp->embedsort);
				CheckRadioButton(hwnd,IDC_PREFS_PFORM_UTF8,IDC_PREFS_PFORM_NATIVE,IDC_PREFS_PFORM_UTF8+tgp->nativetextencoding);
				SetFocus(GetDlgItem(hwnd,IDC_PREFS_PFORMOTHERTEXT));	/* set focus to text */
			}
			return (FALSE);
		case WM_COMMAND:
			if (HIWORD(wParam) == EN_UPDATE)	{
				WORD id = LOWORD(wParam);
				int length = 0;

				if (id == IDC_PREFS_PFORMOTHERTEXT)
					length = 2;
				checktextfield((HWND)lParam,length);
				CheckRadioButton(hwnd,IDC_PREFS_PFORMSTYLE,IDC_PREFS_PFORMOTHER, IDC_PREFS_PFORMOTHER);
				return TRUE;
			}
#if 0
			if (LOWORD(wParam) == IDC_PREFS_PFORMOTHERTEXT)
				if (HIWORD(wParam) == EN_CHANGE)
					CheckRadioButton(hwnd,IDC_PREFS_PFORMSTYLE,IDC_PREFS_PFORMOTHER, IDC_PREFS_PFORMOTHER);
#endif
			return (FALSE);

		case WM_NOTIFY:
			switch (((NMHDR *)lParam)->code)	{
				case PSN_KILLACTIVE:
					stylemode = findgroupcheck(hwnd,IDC_PREFS_PFORMSTYLE,IDC_PREFS_PFORMOTHER)-IDC_PREFS_PFORMSTYLE;
					if (stylemode)	{
						if (stylemode == 1)
							tgp->indentdef= '\t';
						else	{
							getDItemText(hwnd,IDC_PREFS_PFORMOTHERTEXT,tbuff,2);
							tgp->indentdef = *tbuff;
						}
					}
					else
						tgp->indentdef = 0;
					tgp->embedsort = isitemchecked(hwnd,IDC_PREFS_PFORMEMBED);
					tgp->nativetextencoding = findgroupcheck(hwnd,IDC_PREFS_PFORM_UTF8,IDC_PREFS_PFORM_NATIVE)-IDC_PREFS_PFORM_UTF8;
					SetWindowLongPtr(hwnd, DWLP_MSGRESULT,FALSE);
					return (TRUE);		/* check our results; SetwindowLong TRUE to hold page; FALSE to free */
				case PSN_HELP:
					dowindowhelp(TEXT("base\\Prefs_export.htm"));
					return (TRUE);
			}
			break;
		case WM_HELP:
			return (dodialoghelp(hwnd,TEXT("base\\Prefs_export.htm"),(HELPINFO *)lParam,wh_prefid));
	}
	return (FALSE);
}
#ifdef PUBLISH
/**********************************************************************************/	
static INT_PTR CALLBACK efolderproc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)

{
	struct cgen * cgp;

	cgp = getdata(hwnd);
	
	switch (msg)	{

		case WM_INITDIALOG:
			if (cgp = setdata(hwnd,(void *)((LPPROPSHEETPAGE)lParam)->lParam))	{	/* set data */
				SetDlgItemText(hwnd, IDC_PPREFS_FILE_SHOWCDX,cgp->cdxpath);	/* set text */
				SetDlgItemText(hwnd, IDC_PPREFS_FILE_SHOWSTL,cgp->stlpath);	/* set text */
				SetDlgItemText(hwnd, IDC_PPREFS_FILE_SHOWCTG,cgp->ctgpath);	/* set text */
				SetDlgItemText(hwnd, IDC_PPREFS_FILE_SHOWABR,cgp->abrpath);	/* set text */
				SetDlgItemText(hwnd, IDC_PPREFS_FILE_SHOWUDC,cgp->udcpath);	/* set text */
				if (!cgp->adm.access[FOLDER_CDX].setdef && nstrcmp(cgp->adm.psswd,g_password))	{	/* if ok or admin access */
					disableitem(hwnd,IDC_PPREFS_FILE_SETCDX);
					disableitem(hwnd,IDC_PPREFS_FILE_CLEARCDX);
				}
				if (!cgp->adm.access[FOLDER_STL].setdef && nstrcmp(cgp->adm.psswd,g_password))	{
					disableitem(hwnd,IDC_PPREFS_FILE_SETSTL);
					disableitem(hwnd,IDC_PPREFS_FILE_CLEARSTL);
				}
				if (!cgp->adm.access[FOLDER_ABR].setdef && nstrcmp(cgp->adm.psswd,g_password))	{
					disableitem(hwnd,IDC_PPREFS_FILE_SETABR);
					disableitem(hwnd,IDC_PPREFS_FILE_CLEARABR);
				}
				if (!cgp->adm.access[FOLDER_CTG].setdef && nstrcmp(cgp->adm.psswd,g_password))	{
					disableitem(hwnd,IDC_PPREFS_FILE_SETCTG);
					disableitem(hwnd,IDC_PPREFS_FILE_CLEARCTG);
				}
				if (!cgp->adm.access[FOLDER_UDC].setdef && nstrcmp(cgp->adm.psswd,g_password))	{
					disableitem(hwnd,IDC_PPREFS_FILE_SETUDC);
					disableitem(hwnd,IDC_PPREFS_FILE_CLEARUDC);
				}
			}
			return (FALSE);
		case WM_COMMAND:
			switch (LOWORD(wParam))	{
				case IDC_PPREFS_FILE_SETCDX:
				case IDC_PPREFS_FILE_CLEARCDX:
					if (setdefdir(hwnd,cgp->cdxpath,LOWORD(wParam) == IDC_PPREFS_FILE_SETCDX))
						SetDlgItemText(hwnd, IDC_PPREFS_FILE_SHOWCDX,cgp->cdxpath);	/* set text */
					break;
				case IDC_PPREFS_FILE_SETSTL:
				case IDC_PPREFS_FILE_CLEARSTL:
					if (setdefdir(hwnd,cgp->stlpath,LOWORD(wParam) == IDC_PPREFS_FILE_SETSTL))
						SetDlgItemText(hwnd, IDC_PPREFS_FILE_SHOWSTL,cgp->stlpath);	/* set text */
					break;
				case IDC_PPREFS_FILE_SETCTG:
				case IDC_PPREFS_FILE_CLEARCTG:
					if (setdefdir(hwnd,cgp->ctgpath,LOWORD(wParam) == IDC_PPREFS_FILE_SETCTG))
						SetDlgItemText(hwnd, IDC_PPREFS_FILE_SHOWCTG,cgp->ctgpath);	/* set text */
					break;
				case IDC_PPREFS_FILE_SETABR:
				case IDC_PPREFS_FILE_CLEARABR:
					if (setdefdir(hwnd,cgp->abrpath,LOWORD(wParam) == IDC_PPREFS_FILE_SETABR))
						SetDlgItemText(hwnd, IDC_PPREFS_FILE_SHOWABR,cgp->abrpath);	/* set text */
					break;
				case IDC_PPREFS_FILE_SETUDC:
				case IDC_PPREFS_FILE_CLEARUDC:
					if (setdefdir(hwnd,cgp->udcpath,LOWORD(wParam) == IDC_PPREFS_FILE_SETUDC))
						SetDlgItemText(hwnd, IDC_PPREFS_FILE_SHOWUDC,cgp->udcpath);	/* set text */
					break;
			}
			return (TRUE);
		case WM_NOTIFY:
			switch (((NMHDR *)lParam)->code)	{
				case PSN_KILLACTIVE:
					SetWindowLongPtr(hwnd,DWLP_MSGRESULT,(LONG_PTR)FALSE);
					return (TRUE);		/* check our results; SetwindowLong TRUE to hold page; FALSE to free */
				case PSN_HELP:
					dowindowhelp(pr_prefhelp);
					return (TRUE);
			}
			break;
		case WM_HELP:
			return (dodialoghelp(hwnd,NULL,(HELPINFO *)lParam,wh_prefid));
	}
	return (FALSE);
}
/**********************************************************************************/	
static INT_PTR CALLBACK eadminproc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)

{
	struct cgen * cgp;
	HWND th;
	TC_ITEM tc;
	int count;
	TCHAR string1[FSSTRING], string2[FSSTRING];
	struct fpermiss * afp;
	static TCHAR *fsname[] = {TEXT("Index"), TEXT("Stationery"), TEXT("Abbreviations"), TEXT("Tags"), TEXT("Dictionary")};

#define FS_NSIZE (sizeof(fsname)/sizeof(char *))

	cgp = getdata(hwnd);

	switch (msg)	{

		case WM_INITDIALOG:
			if (cgp = setdata(hwnd,(void *)((LPPROPSHEETPAGE)lParam)->lParam))	{	/* set data */
				th = GetDlgItem(hwnd,IDC_PPREFS_ADM_TAB);
				memset(&tc,0,sizeof(tc));
				tc.mask = TCIF_TEXT;
				for (count = 0; count < FS_NSIZE; count++)	{		/* build tabs */
					tc.pszText = fsname[count]; /* tab title */
					TabCtrl_InsertItem(th,count,&tc);
				}
				TabCtrl_SetCurSel(th,0);
				setatabs(hwnd,th,&cgp->adm);		/* set for first tab */
				SetDlgItemText(hwnd, IDC_PPREFS_ADM_PWD1,cgp->adm.psswd);	/* set passwd text */
				SetDlgItemText(hwnd, IDC_PPREFS_ADM_PWD2,cgp->adm.psswd);	/* set passwd text */
				checkitem(hwnd,IDC_PPREFS_ADM_REQID,cgp->adm.requireid);	/* require ID */
				setupvalidate(hwnd, &cgp->adm);		/* set up validation items */			
				checkitem(hwnd,IDC_PPREFS_ADM_PERMITWRITE,cgp->adm.permitwrite);/* permit write access */
				checkitem(hwnd,IDC_PPREFS_ADM_READONLY,cgp->adm.readaccess);/* permit read-only access */
				checkitem(hwnd,IDC_PPREFS_ADM_MULTIPLE,cgp->adm.allowmultiple);/* permit multiple instances */
			}
			return (FALSE);
		case WM_COMMAND:
			switch (LOWORD(wParam))	{
				case IDC_PPREFS_ADM_REQID:			/* id requirement */
					cgp->adm.requireid = (char)isitemchecked(hwnd,IDC_PPREFS_ADM_REQID);
					setupvalidate(hwnd, &cgp->adm);		/* set up validation items */			
					break;
				case IDC_PPREFS_ADM_IDFILE:
					setuserfile(hwnd,cgp->adm.userpath);
					break;
				case IDC_PPREFS_ADM_OPENDEF:		/* if changing move properties for default folder */
					afp = &cgp->adm.access[TabCtrl_GetCurSel(GetDlgItem(hwnd,IDC_PPREFS_ADM_TAB))];
					if (afp->changefolderok)	{	/* if can show save folder */
						if (!isitemchecked(hwnd,IDC_PPREFS_ADM_OPENDEF))	{		/* if not set move to default folder */
							afp->changefolder = TRUE;	/* always allow user to change save folder */
							disableitem(hwnd,IDC_PPREFS_ADM_CHANGEDEF);
						}
						else
							enableitem(hwnd,IDC_PPREFS_ADM_CHANGEDEF);
					}
			}
			return (TRUE);
		case WM_NOTIFY:
			switch (LOWORD(wParam))	{
				case 0:			/* from property sheet */
					switch (((NMHDR *)lParam)->code)	{
						case PSN_KILLACTIVE:
							cgp->adm.requireid = (char)isitemchecked(hwnd,IDC_PPREFS_ADM_REQID);
							cgp->adm.validate = (char)isitemchecked(hwnd,IDC_PPREFS_ADM_VALID);
							cgp->adm.permitwrite = (char)isitemchecked(hwnd,IDC_PPREFS_ADM_PERMITWRITE);
							cgp->adm.readaccess = (char)isitemchecked(hwnd,IDC_PPREFS_ADM_READONLY);
							cgp->adm.allowmultiple = (char)isitemchecked(hwnd,IDC_PPREFS_ADM_MULTIPLE);
							GetDlgItemText(hwnd,IDC_PPREFS_ADM_PWD1,string1,FSSTRING);
							GetDlgItemText(hwnd,IDC_PPREFS_ADM_PWD2,string2,FSSTRING);
							if (nstrcmp(string1,string2))	{	/* if passwd error */
								senderr(ERR_BADPASS,WARN);
								SetFocus(GetDlgItem(hwnd,IDC_PPREFS_ADM_PWD1));
								SetWindowLongPtr(hwnd,DWLP_MSGRESULT,(LONG_PTR)TRUE);
							}
							else	{
								nstrcpy(cgp->adm.psswd,string1);		/* save password */
								nstrcpy(g_password,string1);
								SetWindowLongPtr(hwnd,DWLP_MSGRESULT,(LONG_PTR)FALSE);
							}
							getatabs(hwnd,GetDlgItem(hwnd,IDC_PPREFS_ADM_TAB),&cgp->adm);
							return (TRUE);		/* check our results; SetwindowLong TRUE to hold page; FALSE to free */
						case PSN_HELP:
							dowindowhelp(pr_prefhelp);
							return (TRUE);
					}
					break;
				case IDC_PPREFS_ADM_TAB:
					if (((LPNMHDR)lParam)->code == TCN_SELCHANGING)	
						getatabs(hwnd,((LPNMHDR)lParam)->hwndFrom,&cgp->adm);
					else if (((LPNMHDR)lParam)->code == TCN_SELCHANGE)
						setatabs(hwnd,((LPNMHDR)lParam)->hwndFrom,&cgp->adm);
					break;
				default:
					;
			}
			break;
		case WM_HELP:
			return (dodialoghelp(hwnd,NULL,(HELPINFO *)lParam,wh_prefid));
	}
	return (FALSE);
}
/*******************************************************************************/
static void	setupvalidate(HWND hwnd, ADMIN * adm)	/* fixes validation fields per id requirement */

{
	if (adm->requireid)	{	/* if required */
		checkitem(hwnd,IDC_PPREFS_ADM_VALID,adm->validate);		/* require validation */
		enableitem(hwnd,IDC_PPREFS_ADM_VALID);
		enableitem(hwnd,IDC_PPREFS_ADM_IDFILE);
	}
	else {
		adm->validate = FALSE;
		checkitem(hwnd,IDC_PPREFS_ADM_VALID,adm->validate);		/* require validation */
		disableitem(hwnd,IDC_PPREFS_ADM_VALID);
		disableitem(hwnd,IDC_PPREFS_ADM_IDFILE);
	}
}
/*******************************************************************************/
static void setatabs(HWND hwnd, HWND th, ADMIN *ap)	/* sets admin tab items */

{
	struct fpermiss * afp;

	afp = &ap->access[TabCtrl_GetCurSel(th)];
	if (afp->opendefok)			{		/* if can show default open/save folder */
		showitem(hwnd,IDC_PPREFS_ADM_OPENDEF);
	}
	else
		hideitem(hwnd,IDC_PPREFS_ADM_OPENDEF);
	checkitem(hwnd,IDC_PPREFS_ADM_OPENDEF,afp->opendef);
	if (afp->changefolderok)	{	/* if can show save folder */
		if (!afp->opendef)	{		/* if not set move to default folder */
			afp->changefolder = TRUE;	/* always allow user to change save folder */
			disableitem(hwnd,IDC_PPREFS_ADM_CHANGEDEF);
		}
		else
			enableitem(hwnd,IDC_PPREFS_ADM_CHANGEDEF);
		showitem(hwnd,IDC_PPREFS_ADM_CHANGEDEF);
	}
	else
		hideitem(hwnd,IDC_PPREFS_ADM_CHANGEDEF);
	checkitem(hwnd,IDC_PPREFS_ADM_CHANGEDEF,afp->changefolder);
	if (afp->setdefok)					/* if can show set default folder */
		showitem(hwnd,IDC_PPREFS_ADM_SETDEF);
	else
		hideitem(hwnd,IDC_PPREFS_ADM_SETDEF);
	checkitem(hwnd,IDC_PPREFS_ADM_SETDEF,afp->setdef);
}
/*******************************************************************************/
static void getatabs(HWND hwnd, HWND th, ADMIN *ap)	/* gets admin tab items */

{
	int tindex;

	tindex = TabCtrl_GetCurSel(th);
	ap->access[tindex].opendef = (char)isitemchecked(hwnd,IDC_PPREFS_ADM_OPENDEF);
	ap->access[tindex].changefolder = (char)isitemchecked(hwnd,IDC_PPREFS_ADM_CHANGEDEF);
	ap->access[tindex].setdef = (char)isitemchecked(hwnd,IDC_PPREFS_ADM_SETDEF);
}
/*******************************************************************************/
static BOOL setuserfile(HWND hwnd, TCHAR * fpath)	/* gets user file name */

{
	TCHAR path[MAX_PATH];
	OPENFILENAME of;
	TCHAR fstring[] = TEXT("User (*.usr)\0*.usr\0");

	memset(&of,0,sizeof(OPENFILENAME));
	nstrcpy(path,fpath);
	of.lStructSize = sizeof(OPENFILENAME);
	of.hwndOwner = hwnd;
	of.lpstrFile = path;		/* holds path on return */
	of.lpstrDefExt = TEXT("usr");
	of.lpstrFilter = fstring;
	of.nMaxFile =MAX_PATH;
	of.lpstrTitle = TEXT("User ID File");
	of.Flags = OFN_HIDEREADONLY|OFN_PATHMUSTEXIST|OFN_EXPLORER|OFN_NOCHANGEDIR|OFN_FILEMUSTEXIST;

	if (GetOpenFileName(&of))	{
		nstrcpy(fpath,path);
		return (TRUE);
	}
	return (FALSE);
}
#endif //PUBLISH
/*******************************************************************************/
static BOOL setdefdir(HWND hwnd, TCHAR * dpath, BOOL set)	/* sets default directory */

{
	if (set)	{	// if setting
		TCHAR path[MAX_PATH], tdir[MAX_PATH];
		OPENFILENAME of;
		TCHAR fstring[] = TEXT("^\0*.___\0");

		memset(&of,0,sizeof(OPENFILENAME));
		nstrcpy(tdir,dpath);
		of.lStructSize = sizeof(OPENFILENAME);
		of.lpstrInitialDir = tdir;
		of.hwndOwner = hwnd;
		of.lpstrFile = path;		/* holds path on return */
		of.lpstrFilter = fstring;
		of.nMaxFile =MAX_PATH;
		of.lpstrTitle = TEXT("Default Folder");
	//	of.Flags = OFN_HIDEREADONLY|OFN_PATHMUSTEXIST|OFN_EXPLORER|OFN_ENABLETEMPLATE|OFN_ENABLEHOOK|OFN_NOVALIDATE|OFN_NOTESTFILECREATE|OFN_NOCHANGEDIR;
		of.Flags = OFN_HIDEREADONLY|OFN_PATHMUSTEXIST|OFN_EXPLORER|OFN_ENABLEHOOK|OFN_NOVALIDATE|OFN_NOTESTFILECREATE|OFN_NOCHANGEDIR;
	//	of.hInstance = g_hinst;
	//	of.lpTemplateName = MAKEINTRESOURCE(IDD_PREFS_DEFBUTTON);
		of.lpfnHook = dirhook;
		of.lCustData = (LONG_PTR)tdir;	/* directory path */
		nstrcpy(path, TEXT("xx"));	// need this dummy text

		if (GetOpenFileName(&of))	{
			nstrcpy(dpath,tdir);
			return (TRUE);
		}
		return (FALSE);
	}
	*dpath = '\0';	// clearing path
	return TRUE;
}
/**********************************************************************************/	
static INT_PTR CALLBACK dirhook(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)

{
	HWND parent;
	OPENFILENAME * ofp;
	char * strptr;

	parent = GetParent(hwnd);
	strptr = getdata(hwnd);
	switch (msg)	{
		case WM_INITDIALOG:
			setdata(hwnd, (void *)((OPENFILENAME*)lParam)->lCustData);
			hideitem(parent,stc2);
			hideitem(parent,cmb13);
//			hideitem(parent,edt1);
			hideitem(parent,stc3);
			hideitem(parent,cmb1);
			setDItemText(parent,stc4,"Use This Folder:");
			centerwindow(parent,0);
			return (TRUE);
		case WM_NOTIFY:
			ofp = ((LPOFNOTIFY)lParam)->lpOFN;
			switch (((LPOFNOTIFY)lParam)->hdr.code)	{
				case CDN_FOLDERCHANGE:
					CommDlg_OpenSave_GetFolderPath(parent,strptr,MAX_PATH);
//					if (CommDlg_OpenSave_GetFolderPath(parent,strptr,MAX_PATH) >= 0)	{
//						setDItemText(hwnd,IDC_PREFS_SELECTDIR,PathFindFileName(strptr));
//					}
				default:
					;
			}
			break;
		case WM_HELP:
			return (dodialoghelp(hwnd,TEXT("base\\Prefs_gen.htm"),(HELPINFO *)lParam,wh_prefid));
	}
	return (FALSE);
}
/**********************************************************************************/
static void displayfont(HWND hwnd, int id, HFONT * fhp, int type)	/* shows font sample */

{
	LOGFONT lf;
	HWND hwst,hwsz;
	TCHAR fname[LF_FACESIZE], fsize[32];
	HDC dc;
	HFONT tfh;
	int size, index;

	hwsz = GetDlgItem(hwnd,id);
	if (type == CBN_SELENDOK)	{			/* selection change */
		index = SendMessage(hwsz,CB_GETCURSEL,0,0);
		SendMessage(hwsz,CB_GETLBTEXT,(WPARAM)index,(LPARAM)fsize);
	}
	else 		/* typed in */
		GetWindowText(hwsz,fsize,32);
	GetWindowText(GetDlgItem(hwnd,IDC_PREFS_PTEXTFONT),fname,FONTNAMELEN-1);
	hwst = GetDlgItem(hwnd,IDC_PREFS_PTEXTSAMPLE);
	memset(&lf,0,sizeof(lf));
	if (dc = GetDC(hwst))	{
		size = natoi(fsize);
		lf.lfHeight = -MulDiv(size, GetDeviceCaps (dc, LOGPIXELSY), 72);	/* scale font to right size */
		lf.lfOutPrecision = OUT_DEVICE_PRECIS;
		lf.lfPitchAndFamily = DEFAULT_PITCH;
		lf.lfCharSet = DEFAULT_CHARSET;
		nstrcpy(lf.lfFaceName,fname);
		if (tfh = CreateFontIndirect(&lf))	{
			SendMessage(hwst,WM_SETFONT,(WPARAM)tfh,MAKELPARAM(TRUE,0));
			if (*fhp)		/* if had previous font */
				DeleteFont(*fhp);	/* delete it */
			*fhp = tfh;		/* note new one */
		}
		ReleaseDC(hwst,dc);
	}
}
/*******************************************************************************/
void edit_savegroup(HWND hwnd)	/* makes permanent group from temp */

{
	DialogBoxParam(g_hinst,MAKEINTRESOURCE(IDD_SAVEGROUP),g_hwframe,gnameproc,(LPARAM)WX(hwnd,owner));
}
/******************************************************************************/
static INT_PTR CALLBACK gnameproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)

{
	INDEX * FF;
	char nstring[STSTRING];
	GROUPHANDLE gh;

	FF = getdata(hwnd);
	switch (msg)	{

		case WM_INITDIALOG:
			setdata(hwnd,(void *)lParam);/* set data */
			centerwindow(hwnd,1);
			SendMessage(GetDlgItem(hwnd,IDC_SAVEGROUP_NAME),EM_SETLIMITTEXT,GROUPLEN-1,0);	/* limits text */
			SetFocus(GetDlgItem(hwnd,IDC_SAVEGROUP_NAME));	/* set focus to text */
			return FALSE;
		case WM_COMMAND:
			switch (LOWORD(wParam))	{
				case IDOK:
					getDItemText(hwnd,IDC_SAVEGROUP_NAME,nstring,STSTRING);
					gh = FF->lastfile;
					if (grp_make(FF,gh, nstring, FALSE))	{	/* converts handle to permanent group */
						grp_dispose(gh);			/* get rid of this copy of group (will reopen officially) */
						grp_buildmenu(FF);
						view_showgroup(FF->vwind,nstring);	/* find from menu */
						FF->lastfile = NULL;	/* now have no last search */
					}
					else		/* some error */
						return (TRUE);
				case IDCANCEL:
					EndDialog(hwnd,LOWORD(wParam) == IDOK ? TRUE : FALSE);
					return TRUE;
			}
	}
	return FALSE;
}
/*******************************************************************************/
void edit_managegroups(HWND hwnd)	/* manages groups */

{
	DialogBoxParam(g_hinst,MAKEINTRESOURCE(IDD_MANAGEGROUPS),g_hwframe,gmanageproc,(LPARAM)WX(hwnd,owner));
}
/******************************************************************************/
static INT_PTR CALLBACK gmanageproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)

{
	INDEX * FF;
	GROUPHANDLE gh;
	HWND cbh;
	TCHAR itemtext[STSTRING];
	char *gname;
	int firstg, lastg;
	struct ginfo gi;

	FF = getdata(hwnd);
	switch (msg)	{
		case WM_INITDIALOG:
			if (FF = setdata(hwnd,(void *)lParam))	{	/* set data */
				CheckRadioButton(hwnd,IDC_MANAGEGROUPS_ALL,IDC_MANAGEGROUPS_GROUP, IDC_MANAGEGROUPS_GROUP);
				CheckRadioButton(hwnd,IDC_MANAGEGROUPS_REBUILD,IDC_MANAGEGROUPS_LINK, IDC_MANAGEGROUPS_REBUILD);
				cbh = GetDlgItem(hwnd,IDC_MANAGEGROUPS_LIST);
				grp_buildlist(FF,cbh);
				if (FF->viewtype == VIEW_GROUP)	/* if already viewing a group */
					ComboBox_SelectString(cbh,-1,toNative(FF->curfile->gname));	/* find item # */
				else
					ComboBox_SetCurSel(cbh,0);
				if (FF->head.sortpars.fieldorder[0] == PAGEINDEX)	/* if page sort */
					disableitem(hwnd,IDC_MANAGEGROUPS_LINK);		/* forbid linking */
				centerwindow(hwnd,1);
				SetFocus(cbh);	/* set focus to list */
			}
			return FALSE;
		case WM_COMMAND:
			cbh = GetDlgItem(hwnd,IDC_MANAGEGROUPS_LIST);
			switch (LOWORD(wParam))	{
				case IDOK:
					if (isitemchecked(hwnd,IDC_MANAGEGROUPS_ALL))	{		/* if doing all groups */
						firstg = 0;
						lastg = ComboBox_GetCount(cbh)-1;
					}
					else
						firstg = lastg = ComboBox_GetCurSel(cbh);
					while (firstg <= lastg)	{	/* for groups in range */
						ComboBox_GetLBText(cbh,firstg,itemtext);
						gname = fromNative(itemtext);
						if (gh = grp_open(FF,gname))	{		/* if can open group */
							if (isitemchecked(hwnd,IDC_MANAGEGROUPS_REBUILD))		/* revise */
								grp_revise(FF,&gh);		/* revise, sort & save */
							else if (isitemchecked(hwnd,IDC_MANAGEGROUPS_DELETE))	/* delete */
								grp_delete(FF,gname);
							else if (!grp_link(FF,&gh))	/* if can link */
								grp_make(FF,gh,gname,TRUE);
							grp_dispose(gh);			/* dispose of it after finishing */
								/* now check if action is on current group */
							if (FF->viewtype == VIEW_GROUP && !strcmp(FF->curfile->gname,gname))	{
								if (!isitemchecked(hwnd,IDC_MANAGEGROUPS_DELETE))		/* if not deleting our group */
									view_showgroup(FF->vwind,gname);	/* redisplay it */
								else
									view_allrecords(FF->vwind);	/* display all records */
							}
						}
						firstg++;
					}
					grp_buildmenu(FF);	/* rebuild menu */
				case IDCANCEL:
					EndDialog(hwnd,LOWORD(wParam) == IDOK ? TRUE : FALSE);
					return TRUE;
				case IDC_MANAGEGROUPS_ALL:
					disableitem(hwnd,IDC_MANAGEGROUPS_INFO);
					return (TRUE);
				case IDC_MANAGEGROUPS_GROUP:
					enableitem(hwnd,IDC_MANAGEGROUPS_INFO);
					return (TRUE);
				case IDC_MANAGEGROUPS_INFO:
					ComboBox_GetLBText(cbh,ComboBox_GetCurSel(cbh),itemtext);
					gname = fromNative(itemtext);
					if (gi.gh = grp_open(FF,gname))	{		/* if can open group */
						gi.FF = FF;
						DialogBoxParam(g_hinst,MAKEINTRESOURCE(IDD_GROUP_INFO),hwnd,showginfoproc,(LPARAM)&gi);
						grp_dispose(gi.gh);			/* dispose of it after finishing */
					}
					return (TRUE);
			}
			break;
		case WM_HELP:
			return (dodialoghelp(hwnd,TEXT("base\\Groups_manage.htm"),(HELPINFO *)lParam,wh_groupid));
	}
	return FALSE;
}
/*******************************************************************************/
static INT_PTR CALLBACK showginfoproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)

{
	struct ginfo * gi;
	GROUPHANDLE gh;
	char tstring[STSTRING], *tptr;
	struct tm * date;
	static char *fname[] = {"All","All Text","Last Text"};

	if (gi = getdata(hwnd))
		gh = gi->gh;
	switch (msg)	{
		case WM_INITDIALOG:
			if (gi = setdata(hwnd,(void *)lParam))	{	/* set data */
				gh = gi->gh;
				setDItemText(hwnd,IDC_GROUPINFO_NAME,gh->gname);
				setint(hwnd,IDC_GROUPINFO_RECNUM,gh->rectot);
				time_t tt = gh->tstamp;
				date = localtime(&tt);
				strftime(tstring, 100, "%b %d %Y %H:%M", date);
				setDItemText(hwnd,IDC_GROUPINFO_DATE,tstring);
				if (gh->gflags&GF_SEARCH)	{
					if (strlen(gh->lg.range0) || strlen(gh->lg.range1))	// if had range
						sprintf(tstring,"Search in range [%s] [%s]",gh->lg.range0,gh->lg.range1);
					else
						strcpy(tstring, "Search");
					if (gh->lg.lsarray[0].notflag)
						setDItemText(hwnd,IDC_GROUPINFO_NOT, "Not");
					setDItemText(hwnd,IDC_GROUPINFO_TEXT, gh->lg.lsarray[0].string);
					if (gh->lg.size > 1)
						setDItemText(hwnd,IDC_GROUPINFO_ANDOR, gh->lg.lsarray[0].andflag ? "and..." : "or...");
					if (gh->lg.lsarray[0].evalrefflag)
						checkitem(hwnd,IDC_GROUPINFO_EVAL, BST_INDETERMINATE);
					else
						disableitem(hwnd,IDC_GROUPINFO_EVAL);
					if (gh->lg.lsarray[0].wordflag)
						checkitem(hwnd,IDC_GROUPINFO_WORD, BST_INDETERMINATE);
					else
						disableitem(hwnd,IDC_GROUPINFO_WORD);
					if (gh->lg.lsarray[0].caseflag)
						checkitem(hwnd,IDC_GROUPINFO_CASE,BST_INDETERMINATE);
					else
						disableitem(hwnd,IDC_GROUPINFO_CASE);
					if (gh->lg.lsarray[0].patflag)
						checkitem(hwnd,IDC_GROUPINFO_PATTERN, BST_INDETERMINATE);
					else
						disableitem(hwnd,IDC_GROUPINFO_PATTERN);
					fs_showstyle(hwnd,IDC_GROUPINFO_ATTRIB,gh->lg.lsarray[0].style,gh->lg.lsarray[0].font, gh->lg.lsarray[0].forbiddenstyle, gh->lg.lsarray[0].forbiddenfont);
					if (gh->lg.lsarray[0].field < 0)
						tptr = fname[gh->lg.lsarray[0].field+(-ALLFIELDS)];
					else {
						if (gh->lg.lsarray[0].field == PAGEINDEX)
							tptr = gi->FF->head.indexpars.field[gi->FF->head.indexpars.maxfields-1].name;
						tptr = gi->FF->head.indexpars.field[gh->lg.lsarray[0].field].name;
					}
					setDItemText(hwnd,IDC_GROUPINFO_FIELD, tptr);			
				}
				else	{
					strcpy(tstring, "From selection");
					disableitem(hwnd,IDC_GROUPINFO_WORD);
					disableitem(hwnd,IDC_GROUPINFO_CASE);
					disableitem(hwnd,IDC_GROUPINFO_PATTERN);
					disableitem(hwnd,IDC_GROUPINFO_EVAL);
				}
				setDItemText(hwnd,IDC_GROUPINFO_METHOD,tstring);
				*tstring = '\0';
				if (gh->gflags&GF_REVISED)
					strcpy(tstring, "Revised ");
				if (gh->gflags&GF_COMBINE)
					strcat(tstring, "Combined ");
				if (gh->gflags&GF_LINKED)
					strcat(tstring,"Linked");
				setDItemText(hwnd,IDC_GROUPINFO_CHANGES,tstring);
				centerwindow(hwnd,1);
			}
			return FALSE;
		case WM_COMMAND:
			switch (LOWORD(wParam))	{
				case IDOK:
				case IDCANCEL:
					EndDialog(hwnd,TRUE);
					return TRUE;
			}
			break;
		case WM_HELP:
			return (dodialoghelp(hwnd,TEXT("base\\Groups_manage.htm"),(HELPINFO *)lParam,wh_ginfoid));
	}
	return FALSE;
}
/*******************************************************************************/
short env_tobase(int unit, float eval)	/* converts from expression to base */

{
	float tval;
	
	switch (unit)	{
		case U_INCH:
			tval = eval*PIXELSTOINCH;	/* inches to pixels */
			break;
		case U_MM:
			tval = eval*PIXELSTOMM;
			break;
		case U_POINT:
			tval = eval*PIXELSTOPOINT;
			break;
		case U_PICA:
			tval = eval*PIXELSTOPICA;
			break;
	}
	return ((short)tval);
}
/*******************************************************************************/
float env_toexpress(int unit, short bval)	/* converts from base to unit of espression */

{
	switch (unit)	{
		case U_INCH:
			return ((float)bval/PIXELSTOINCH);	/* pixels to inches */
		case U_MM:
			return ((float)bval/PIXELSTOMM);
		case U_POINT:
			return ((float)bval/PIXELSTOPOINT);
		case U_PICA:
			return ((float)bval/PIXELSTOPICA);
	}
	return ((float)0);
}
