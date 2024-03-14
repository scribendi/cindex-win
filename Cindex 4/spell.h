#pragma once
#include "records.h"
#include "hspell.h"

typedef struct {		/* spell-checking struct */
	RECN firstr;		/* first record to check */
	RECN lastr;			/* record on which to stop */
	short newflag;		/* among new records */
	short modflag;		/* among modified records */
	short checkpage;	/* check page refs */
	short field;		/* fields to check */
	short doubleflag;	/* TRUE when double word found */
	struct langprefs lp;	/* language preferences */
	HUNSPELL * speller;	// handle to hunspell
} SPELL;

RECORD * sp_findfirst(INDEX * FF, SPELL * sp, short restart, char **sptr, short *mlptr);	/* finds first rec after rptr that contains err */
char * sp_checktext(INDEX * FF, RECORD * recptr, char * startptr, SPELL * sp, short * mlptr);	/* marks spell error */
char * sp_reptext(INDEX * FF, RECORD * recptr, char * startptr, short matchlen, char * corrtext);	 /* replaces text in record */
