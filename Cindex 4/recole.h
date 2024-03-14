#pragma once
#include <ole2.h>

typedef struct tagDSRCOBJ	{	/* source target window */
	IDropSourceVtbl * dsvt;
	int refcount;
	HWND hwnd;					/* parent window */
} DSRCOBJ;

typedef struct tagDTRGOBJ	{	/* drop target object */
	IDropTargetVtbl * dtvt;
	int refcount;
	HWND hwnd;					/* parent window */
	int accept;					/* set by drag enter to indicate acceptable data */
} DTRGOBJ;

typedef struct tagRECORDOBJ	{	/* record data object */
	IDataObjectVtbl * obvt;
	int refcount;
	HWND hwnd;					/* parent window */
	HGLOBAL gh;					/* handle to original data */
} RECORDOBJ;

extern DSRCOBJ dsrc_call;		/* interface for drop source */
extern DTRGOBJ dtrg_call;		/* interface for drop target */
extern RECORDOBJ rec_call;		/* interface for data object */

BOOL recole_capture(HWND hwnd, RECORDOBJ * dp);		/* captures and holds selection */
BOOL recole_ispastable(IDataObject * dp);		/* checks if pastable records */
int recole_paste(HWND hwnd, IDataObject * dp);	/* pastes records */
