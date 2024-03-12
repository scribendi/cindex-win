#pragma once
TCHAR * draft_build(INDEX * FF, RECORD * recptr, short * hlevel);	/* returns text of formed entry */
RECORD * draft_skip(INDEX * FF, RECORD * recptr, short dir);	/* skips in draft display mode */
short draft_disp(HWND wptr, HDC dc, short line, short lcount);	/* displays lines of record */
short draft_measurelines(HWND wptr, HDC dc, RECORD * recptr, LSET *arrayptr, struct selstruct * slptr);	/* makes single dstring */	
