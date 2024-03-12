/*
 *  hspell.h
 *  Cindex
 *
 *  Created by Peter Lennie on 1/4/11.
 *  Copyright 2011 Peter Lennie. All rights reserved.
 *
 */

#pragma once

#include "hunspell.h"
#include "iconv.h"
//#include "hspelladditions.h"

#define DICMAX 20

enum {		// dictionary flags
	DIC_ISAUX = 1
};

enum {		// charset converion type
	TODICTIONARY = 0,
	TONATIVE = 1
};

typedef struct dset {
	char * root;
	char * displayname;
	char * path;
	int flags;
	struct dset ** sets;
	int setcount;
} DICTIONARYSET;

typedef struct {
	char ** list;
	int entrycount;
}HLIST;

typedef struct wl{
	char * word;
	struct wl * next;
}WORDLIST;

typedef struct {
	Hunhandle * handle;
	DICTIONARYSET * dicset;		// active dicset
	iconv_t forwardconverter;	// to dictionary charset
	iconv_t backconverter;		// to native charset
	char userdicpath[512];
	WORDLIST * newwords;		// user words added to dictionary
	WORDLIST * ignoredwords;	// words ignored always
	HLIST suggestions;
	HLIST analyses;
	HLIST stems;
} HUNSPELL;

extern DICTIONARYSET hs_dictionaries[DICMAX];
extern unsigned int hs_dictionaryCount;
extern char * hs_mdextn;
extern char * hs_afextn;

void hspell_init(void);
void hspell_finddictionaries(void);
void hspell_releasedictionaries(void);
DICTIONARYSET * hspell_dictionarysetforlocale(char * locale);
//char * hspell_localefordisplayname(char * name);
HUNSPELL* hspell_open(char * dictionary);
void hspell_close(void);
int hspell_openauxdic(DICTIONARYSET * dsp);	//opens aux dic and adds words from it
int hspell_createpd(char * path);	// creates empty pd file
int hspell_openpd(char * path);
int hspell_savepd(void);	// saves pd; removes its words from dic if close == TRUE
int hspell_closepd(void);	// saves and closes pd
int hspell_addword(char * word);
void hspell_removeword(char * word);
int hspell_addpdword(char * word);
int hspell_addignoredword(char * word);
void hspell_removeignoredwords(void);	// removes all words on ignored list
int hspell_spellword(char * word);
int hspell_suggest(char * word);
int hspell_analyze(char * word);
int hspell_stem(char * word);
WORDLIST * hspell_wlfromfile(char * path,int *error);	// makes list from file
BOOL hspell_wlsavetofile(WORDLIST *wlp, char * path, char * mode);	// writes list to file
void hspell_wladdword(WORDLIST ** wlp,char * word);	// add word to linked list
void hspell_wlfree(WORDLIST ** wlp);	// free word list
char * hspell_convertword(char * text, int direction);	// converts to dictionary character set and vv
void hspell_wlsort(WORDLIST *wlp);	// sorts word list in place
UChar * hspell_wlToUtext(WORDLIST *wlp, int *wordcount);	// makes sorted unicode text list from word list
WORDLIST * hspell_wlFromUtext(UChar * base);	// makes wordlist from unicode text
