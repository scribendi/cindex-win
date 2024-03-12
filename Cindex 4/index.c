#include "stdafx.h"
#include <string.h>
#include <fcntl.h>
#include <io.h>
#include "errors.h"
#include "index.h"
#include "sort.h"
#include "collate.h"
#include "records.h"
#include "group.h"
#include "strings.h"
#include "typestuff.h"
#include "util.h"
#include "files.h"
#include "mfile.h"

INDEX ** index_getpaddr(INDEX * target);		/* returns address of real pointer in linked list */

/*******************************************************************************/
void index_cleartags(INDEX * FF)	// clears tags, etc
{
	RECN n;

	for (n = 1; n <= FF->head.rtot; n++) 	{	/* for all records, fetch in turn */
		RECORD * p = getaddress(FF,n);

		p->isttag = FALSE;	// clear tag
//		p->ismark = FALSE;	// clear mark
	}
}
/*******************************************************************************/
int index_checkintegrity(INDEX * FF, RECN rtot)	// checks integrity of index

{
	RECN n, total;

	total = FF->head.rtot > rtot ? rtot : FF->head.rtot;	// # of records expected in file
	if (FF->head.indexpars.minfields < 2 || FF->head.indexpars.minfields > FF->head.indexpars.maxfields
		|| FF->head.indexpars.maxfields < 2 || FF->head.indexpars.maxfields > FIELDLIM
		|| FF->head.indexpars.recsize > MAXREC)
		return -1;	// fatal error
	for (n = 1; n <= total; n++) 	{	/* for all records, fetch in turn */
		RECORD * p = getaddress(FF,n);
		int index, fcount;
		__try {
			if (p->num != n)
				return 1;	// error
			for (index = 0, fcount = 0; index < FF->head.indexpars.recsize && fcount <= FF->head.indexpars.maxfields; index++)	{
				if (iscodechar(p->rtext[index]))	{
					if (index < FF->head.indexpars.recsize - 1) {
						if (!p->rtext[index + 1] || p->rtext[index + 1] == EOCS || (p->rtext[index] == FONTCHR && p->rtext[index + 1] & FX_COLOR))	// if bad code or font
							return 1;
						index++;		// skip code
					}
					continue;
				}
				if (p->rtext[index] == '\0')	{
					fcount++;
					continue;
				}
				if (p->rtext[index] == EOCS && index && p->rtext[index-1] == '\0')	// if preceded by field end
					break;
			}
			if (index == FF->head.indexpars.recsize || fcount > FF->head.indexpars.maxfields || fcount < FF->head.indexpars.minfields)
				return 1;
		}
		__except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION){
			return -1;	// fatal error
		}
	}
	return 0; // OK
}
/*******************************************************************************/
int index_repair(INDEX * FF)	// repairs index

{
	RECN n, markcount;

	markcount = 0;
	for (n = 1; n <= FF->head.rtot; n++) 	{	/* for all records, fetch in turn */
		RECORD * p = getaddress(FF,n);
		int index, fcount, change;
		int strindex[FIELDLIM];

		change = 0;
		strindex[0] = 0;
		__try {
			if (p->num != n)	{
				p->num = n;	// repair number
				change = TRUE;
			}
			for (index = 0, fcount = 0; index < FF->head.indexpars.recsize && fcount <= FF->head.indexpars.maxfields; index++)	{
				if (iscodechar(p->rtext[index]))	{
					if (index < FF->head.indexpars.recsize - 1) {
						if (!p->rtext[index + 1] || p->rtext[index + 1] == EOCS || (p->rtext[index] == FONTCHR && p->rtext[index + 1] & FX_COLOR)) {// if bad code or font
							p->rtext[index + 1] = FX_FONT;	// font 0 if font; off if style
							change = TRUE;
						}
						index++;		// skip code
					}
					continue;
				}
				if (p->rtext[index] == EOCS)	{	// end
					if (index >= FF->head.indexpars.minfields) {	// if at or above min fields
						if (p->rtext[index-1] != '\0')	{	// if locator is badly terminated
							p->rtext[index-1] = '\0';		// terminate locator
							fcount++;					// count the field
							strindex[fcount] = index;	// note EOCS as base of string
							change = TRUE;
						}
						break;
					}
					else
						p->rtext[index] = '@';		// replace bad EOCS with token
					change = TRUE;
					continue;
				}
				if (p->rtext[index] == '\0')	{
					fcount++;
					strindex[fcount] = index+1;	// save base of string for field at index fcount
					continue;
				}
				if ((unsigned char)p->rtext[index] < SPACE)	{	// if bad char
					p->rtext[index] = '@';
					change = TRUE;
				}
			}
			if (index == FF->head.indexpars.recsize || fcount > FF->head.indexpars.maxfields)	{	// if run out to max record size
				if (fcount > FF->head.indexpars.maxfields)		// if too many fields
					index = strindex[FF->head.indexpars.maxfields];	// index is char beyond end of last legal field
				else if (!p->rtext[--index])	// else  run off end -- if last char is field end
					fcount--;		// will lose a field
				p->rtext[index] = EOCS;	// force EOCS
				if (p->rtext[--index])	{	// if preceding character isn't null
					p->rtext[index] = '\0';	// force it
					fcount++;
					while (fcount > FF->head.indexpars.maxfields && index--)	{	// work towards start of record
						if (p->rtext[index])	{// if not a field break
							p->rtext[index] = '\0';		// make one
							fcount--;
						}
					}
				}
				while (fcount < FF->head.indexpars.minfields && index > 0)	{	// while don't have enough fields
					if (p->rtext[--index] != '\0')		{// if not end of field
						p->rtext[index] = '\0';		// make one
						fcount++;
					}
				}
				change = TRUE;
			}
			else if (fcount < FF->head.indexpars.minfields) {	// too few fields
				int need = FF->head.indexpars.minfields-fcount;
				if (need+index < FF->head.indexpars.recsize) {	// if room to add fields
					while (need--)		// append fields
						p->rtext[index++] = '\0';
					p->rtext[index]	= EOCS;
				}
				else {		// no room; must create new fields from within current space
					while (index-- && need)	{// work towards start of record text
						if (p->rtext[index])	{	// if not a field break
							p->rtext[index] = '\0';		// make one
							need--;
						}
					}
				}
				change = TRUE;
			}
			if (change)	{
				rec_strip(FF,p->rtext);
				p->ismark = TRUE;
				markcount++;
			}
		}
		__except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION){
			return 0;	// error
		}
	}
	return markcount;	// OK
}
#if 0
/*******************************************************************************/
BOOL index_makeundocopy(INDEX * FF, TCHAR * operation)	// makes restorable image of index

{
	// use VitrualAlloc and VirtualFree
	if (FF->indeximage.image)	// if have existing image
		free(FF->indeximage.image);	// release it;
	memset(&FF->image, 0 sizeof(INDEXCOPY));
	if (FF->indeximage.image = malloc(FF->mf.size))	{	// if can get memory for copy
		FF->indeximage.size = FF->mf.size;
		nstrcpy(FF->indeximage.operation,operation);
		memcpy(FF->indeximage.header, &FF->head);
		return TRUE;
	}
	return FALSE;
}
#endif
/*******************************************************************************/
INDEX * index_insert(INDEX * FF)		/* inserts empty (but linked) index struct */

{
	register INDEX ** indexaddr, *linkptr;
	
	indexaddr = index_getpaddr(FF);	/* get address */
	linkptr = *indexaddr;		/* get link to next index */
	if (*indexaddr = getmem(sizeof(INDEX)))		/* if can get memory */
		(*indexaddr)->inext = linkptr;
	return (*indexaddr);
}
/*******************************************************************************/
BOOL index_setworkingsize(INDEX * FF, RECN extrarecords)	// sets working size with margin

{
	return index_setsize(FF,FF->head.rtot,FF->head.indexpars.recsize,extrarecords);
}
/******************************************************************************/
BOOL index_setsize(INDEX * FF, RECN total,int recsize, RECN margin)	// sets specified size with margin

{
	unsigned long oldmapsize = FF->mf.size-FF->head.groupsize;
	unsigned long mapsize = HEADSIZE+((total+margin)*(RECSIZE+recsize));

	if (oldmapsize > mapsize)	// if shrinking record map
		movemappeddata(FF->mf.base+mapsize,FF->mf.base+oldmapsize,FF->head.groupsize);	// shift groups down
	if (mfile_resize(&FF->mf,mapsize+FF->head.groupsize))	{
		FF->recordlimit = total+margin;
		if (mapsize > oldmapsize)		// if enlarging record map
			movemappeddata(FF->mf.base+mapsize,FF->mf.base+oldmapsize,FF->head.groupsize);	// shift groups up
		FF->gbase = (GROUP *)(FF->mf.base+mapsize);
		return TRUE;
	}
	return FALSE;
}
/******************************************************************************/
BOOL index_sizeforgroups(INDEX * FF, unsigned long groupsize)	// extends group collection

{
	unsigned long mapsize = HEADSIZE+(FF->recordlimit*(RECSIZE+FF->head.indexpars.recsize));

	if (mfile_resize(&FF->mf,mapsize+groupsize))	{	// extend current size
		FF->gbase = (GROUP *)(FF->mf.base+mapsize);	// base is file size minus new groupsize
		FF->head.groupsize = groupsize;
		return TRUE;
	}
	return FALSE;
}
/*******************************************************************************/
BOOL index_flush(INDEX * FF)		/* flushes all records & header */

{
	if (!FF->mf.readonly)	{		/* if modified */
		time_t curtime;
		time(&curtime);		/* get current time */
		FF->head.elapsed += curtime-FF->lastflush;	 /* add to total time */
		FF->lastflush = curtime;
		if (FF->head.dirty)		{	/* if was dirty */
			FILETIME wt;

			GetSystemTimeAsFileTime(&wt);	// get current time as write time
			SetFileTime(FF->mf.fref,NULL,NULL,&wt);	// set last write
			FF->head.dirty = FALSE;
			FF->wasdirty = TRUE;
		}
		return index_writehead(FF);	// write header and flush
	}
	return (TRUE);
}
/*******************************************************************************/
void index_flushall(void)		/* flushes all indexes */

{
	register INDEX * FF;
	
	for (FF = g_indexbase; FF; FF = FF->inext)	/* for all indexes */
		index_flush(FF);
}
/*******************************************************************************/
short index_close(INDEX * FF)		/* flushes, closes, releases index and structures */

{
	if (index_flush(FF))	{	/* flush records, write header */
		if (index_closefile(FF))		{	/* close file */
			ucol_close(FF->collator.ucol);
			return (index_cut(FF));
		}
	}
	return (TRUE);
}
/*******************************************************************************/
short index_cut(INDEX * FF)		/* closes file, releases index and restores links */

{
	register INDEX ** indexaddr, *linkptr;
	
	indexaddr = index_getpaddr(FF);	/* get address */
	grp_closecurrent(FF);		/* close any active group */
	if (FF->lastfile)
		grp_dispose(FF->lastfile);
	linkptr = FF->inext;	/* get link to next index */
	freemem(FF);				/* free its space */
	*indexaddr = linkptr;
	return (FALSE);
}
/*******************************************************************************/
INDEX * index_byname(TCHAR *name)		/* returns pointer to index with name 'name' */

{
	register INDEX * FF;
	
	for (FF = g_indexbase; FF; FF = FF->inext)	{	/* for all indexes */
		if (!nstricmp(FF->iname, name))	/* if match */
			break;
	}
	return (FF);
}
/*******************************************************************************/
INDEX * index_byfspec(TCHAR * path)	/* returns pointer to index from filespec */

{
	INDEX * FF;
	
	for (FF = g_indexbase; FF; FF = FF->inext)	{		/* check all */
		if (!nstricmp(FF->pfspec,path))
			break;
	}
	return (FF);
}
/*******************************************************************************/
INDEX * index_byindex(short index)	/* returns pointer to index at index posn in list */

{
	register INDEX * FF;
	short count;
	
	for (count = 0, FF = g_indexbase; FF; FF = FF->inext, count++)	{	/* for all progs */
		if (count == index)	/* if match */
			break;
	}
	return (FF);
}
/*******************************************************************************/
short index_findindex(INDEX * target)	/* find position of target index */

{
	register INDEX * FF;
	short count;
	
	for (count = 0, FF = g_indexbase; FF; FF = FF->inext, count++)	{	/* for all indexes */
		if (FF == target)	/* if match */
			return (count);
	}
	return (-1);
}
/*******************************************************************************/
BOOL index_install(INDEX * FF)	// miscellaneous setup

{
	FF->wholerec = FF->head.indexpars.recsize+RECSIZE;	/* set complete record size */
	col_init(&FF->head.sortpars, FF);		// initialize collator
	return index_setworkingsize(FF,MAPMARGIN);
}
/*******************************************************************************/
void index_setdefaults(INDEX * FF)		/* fills default structure for new index  */

{
	FF->head.endian = 1;
	FF->head.headsize = HEADSIZE;	/* know our header size */
	FF->head.version = CINVERSION; 		/* version under which file created */
	FF->head.createtime = time(NULL);
	FF->head.indexpars = g_prefs.indexpars;
	FF->head.sortpars = g_prefs.sortpars;
	FF->head.sortpars.sversion = SORTVERSION;
	FF->head.refpars = g_prefs.refpars;
	FF->head.privpars = g_prefs.privpars;
	FF->head.formpars = g_prefs.formpars;
	str_xcpy(FF->head.stylestrings,g_prefs.stylestrings);	/* copy style strings */
	strcpy(FF->head.flipwords,g_prefs.flipwords);	/* copy style strings */
	memcpy(FF->head.fm,g_prefs.gen.fm,sizeof(g_prefs.gen.fm));	/* copy local font set */
}
/*******************************************************************************/
void index_markdirty(INDEX * FF)		// marks index as dirty

{
	if (!FF->head.dirty && !FF->mf.readonly)	{	/* if haven't marked index as dirty */
		FF->head.dirty = TRUE;	/* index is dirty */
		index_writehead(FF);	/* write header marked as dirty */
	}
}
/*******************************************************************************/
BOOL index_readhead(INDEX * FF)		// reads header

{
	return copymappeddata(&FF->head,FF->mf.base,HEADSIZE);
}
/*******************************************************************************/
BOOL index_writehead(INDEX * FF)		// writes header

{
	BOOL ok = FALSE;

	if (copymappeddata(FF->mf.base,&FF->head,HEADSIZE))
		ok = mfile_flush(&FF->mf);
	if (FF->cwind)
		SetWindowText(FF->cwind,index_displayname(FF));
	return (ok);
}
/**********************************************************************************/	
void index_getmodtime(INDEX * FF)		/* get time of last change on file */

{
	BY_HANDLE_FILE_INFORMATION finfo;

	if (GetFileInformationByHandle(FF->mf.fref,&finfo))
		FF->modtime = finfo.ftLastWriteTime;
}
/**********************************************************************************/	
void index_setmodtime(INDEX * FF)		/* set time of last change on file */

{
	SetFileTime(FF->mf.fref,NULL,NULL,&FF->modtime);
}
/**********************************************************************************/	
TCHAR * index_displayname(INDEX * FF)	// returns display name

{
	static TCHAR dname[MAX_PATH];

	nstrcpy(dname,FF->iname);
	PathRemoveExtension(dname);
	if (FF->head.dirty)
		nstrcat(dname, TEXT("*"));
	if (FF->mf.readonly)
		nstrcat(dname, TEXT(" [Read Only]"));
	return dname;
}
/*******************************************************************************/
short index_closefile(INDEX * FF)		/* closes file */

{
	index_setworkingsize(FF,0);
	if (!FF->head.dirty && !FF->wasdirty)	/* if not dirty and never has been */
		index_setmodtime(FF);	/* restore last mod time */
	file_deleteprivatebackup(FF);
	return mfile_close(&FF->mf);
}
/*******************************************************************************/
INDEX * index_front(void)		/* gets index with frontmost window (not necess active) */

{
	HWND hwnd;

	hwnd = FORWARD_WM_MDIGETACTIVE(g_hwclient, SendMessage);	/* get active window */
	do	{
		if (getmmstate(hwnd,NULL) != SW_SHOWMINIMIZED && getdata(hwnd) && WX(hwnd,owner))
			return (WX(hwnd, owner));
	} while (hwnd = GetWindow(hwnd, GW_HWNDPREV));
	return (NULL);
}
/*******************************************************************************/
static INDEX ** index_getpaddr(INDEX * FF)		/* returns address of real pointer in linked list */

	/* if target = NULL, returns address for appending next program */
{
	register INDEX ** ptraddr, *pptr;
	
		
	for (pptr = g_indexbase, ptraddr = &g_indexbase; pptr;)	{	/* for all blocks */
		if (pptr == FF)		/* if this is our prog */
			break;
		ptraddr = &pptr->inext;		/* set up address of next */
		pptr = pptr->inext;
	}
	return (ptraddr);
}
