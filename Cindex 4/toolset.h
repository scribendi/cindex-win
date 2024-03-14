#pragma once
void ts_reconcile(HWND hwnd);	/* reconciles headings */
void ts_managecrossrefs(HWND hwnd);	// manages cross-refs
void ts_autogen(HWND hwnd);	/* generates auto cross-references */
void ts_verify(HWND hwnd);	/* verifies cross-refs */
void ts_adjustrefs(HWND hwnd);	/* adjusts references */
void ts_checkindex(HWND hwnd);	// checks index
void ts_splitheadings(HWND hwnd);	// splits headings
void ts_count(HWND hwnd);	/* counts records */
void ts_statistics(HWND hwnd);	/* gets statistics */
BOOL ts_fontsub(INDEX * FF, FONTMAP * fm);	/* organizes font mapping */
