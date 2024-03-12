#pragma once

HWND rcontainer_installrecordwindow(INDEX * FF);	/*  sets up view window */
//void rcontainer_removerecordwindow(INDEX * FF);	// removes record window and destroys
LRESULT CALLBACK rcontainer_proc (HWND, UINT, WPARAM, LPARAM);
