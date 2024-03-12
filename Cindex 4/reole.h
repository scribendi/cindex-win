#pragma once

#include <ole2.h>
#include <richole.h>


typedef struct tagRECALLBACK	{	/* callback for window */
	IRichEditOleCallbackVtbl * rectable;
	LRESULT (*droptest)(struct tagRECALLBACK * reptr, int mode, TCHAR * string);	/* drop/paste check procedure */
	LRESULT (*dropsite)(struct tagRECALLBACK * reptr);	/* dropsite check procedure */
	LRESULT (*contextmenu)(struct tagRECALLBACK * reptr);	/* context menu procedure */
	HWND hwnd;						/* parent window */
	int length;						/* text length */
	int bcount;						/* number of line breaks */
	int pastemode;		// pasting mode
}RECALLBACK;

extern RECALLBACK rw_callback;	/* callback for rich edit window */
