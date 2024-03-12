#pragma once

#define SORTVERSION 9		// mismatch with index value forces resort on opening
#define LOCALESORTVERSION 6	// first version to use localeID to specify collator

typedef struct {		/* struct for sort pars */
	char type;			/* sort type */
	unsigned char sversion;		// version
	char language[4];	// language ID
	short fieldorder[FIELDLIM+1];		/* order of fields */
	short charpri[CHARPRI+1];	/* character priority */
	char ignorepunct;	/* ignore l-by-l punctuation */
	char ignoreslash;	/* ignore /- in word sort */
	char evalnums;		/* evaluate numbers */
	char ignoreparenphrase;	// ignore parenthetical phrases
	char ignoreparen;	// ignore parenthetical endings
	char ignore[STSTRING];	/* ignored leading words in subheads */
	char substitutes[STSTRING];	// substitution word pairs
	char skiplast;			/* skip last heading field */
	char ordered;		/* references ordered */
	char ascendingorder;	/* refs in ascending order */
	char ison;			/* sort is on */
	char nativescriptfirst;	// language script sorts first
	short refpri[REFTYPES+1];		/* reference priority */
	char reftab[REFTYPES];	/* priority table for ref types */
	short partorder[COMPMAX+1];	/* component comparison order */
	short styleorder[STYLETYPES+1];	/* priority table for ref style types */
	char styletab[STYLETYPES*2];	// ref style precedence order
#if 1
	unichar symcode;
	unichar numcode;
#else
	uint32_t spareLong;			/* !!spare */
#endif
	char localeID[16];
	char forceleftrightorder;
	char s1;
	char s2;
	char s3;
	uint32_t spare[59];			/* !!spare */
} SORTPARAMS;
