#pragma once

enum {			/* error level ids */
	WARN = 0,	/* 1 sound, box; 1 sound, box; 2 sounds, box; 3 sounds, box */
	WARNNB,		/* 1 sound, no box; 1 sound, box; 1 sound, box; 1 sound, box */
	WARNNS,		/* no sound, box; 1 sound, box; 1 sound, box; 1 sound, box */
	WNEVER		/* box but never sound */
};

extern short err_eflag;	/* global error flag -- TRUE after any error */

short sendinfooption(TCHAR * title, const int warnno, ...);		/*  Yes,no */
void sendinfo(const int warnno, ...);		/*  O.K. */
short sendwarning(const int warnno, ...);	/* cancel, ok */
short savewarning(const int warnno, ...);		/* discard, cancel, o.k. */
short senderr(const int errnum, const int level, ...);
short senderronstatusline(const int errnum, const int level, ...);
void senditemerr(HWND hwnd,int item);	/* flags item error and sets focus */
