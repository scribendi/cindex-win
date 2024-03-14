#pragma once

typedef struct {	/* contains data/functions for window */
	WFLIST wstuff;	/* basic set of window functions */
	HWND rbwind;		// rebar window
	LRESULT (CALLBACK *rbproc)(HWND, UINT, WPARAM, LPARAM );	/* saved riched procedure */
} CFLIST;

HWND container_setwindow(INDEX * FF, short visflag);	/*  sets up view window */
LRESULT CALLBACK container_proc (HWND, UINT, WPARAM, LPARAM);
void container_installrecordwindow(HWND hwnd);	/*  sets up record window */
//void container_setrecordtitle(HWND hwnd,char * title);	// sets record title
void container_removerecordwindow(HWND hwnd);	/*  removes record window */
