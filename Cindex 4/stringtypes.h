#pragma once

typedef struct {	/* xstring management struct */
	char *str;		/* string */
	short ln;		/* length */
} CSTR;

struct codeseq	{			/* code sequence for font & style */
	char style;
	char font;
	char color;
};
typedef struct {	// for capturing code state
	char code;	// code
	char font;	// font
} CSTATE;

