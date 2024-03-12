#pragma once

typedef struct {		/* reference parameters */
	char crosstart[STSTRING];	/* words that can begin cross-refs */
	char crossexclude[FTSTRING];	/* word that begin general cross ref */
	char maxvalue[FTSTRING];	/* highest valued page reference */
	char csep;			/* cross-ref separator */
	char psep;			/* page ref separator */
	char rsep;			/* page ref connector */
	unsigned char clocatoronly;	// TRUE when recognizing cross-refs in subheadings
	long maxspan;		/* maximum span allowed in range */
	long spare[32];		/* !!spare */
} REFPARAMS;
