/*
 *  hspell.c
 *  Cindex
 *
 *  Created by Peter Lennie on 1/4/11.
 *  Copyright 2011 Peter Lennie. All rights reserved.
 *
 */

#include "stdafx.h"
#include "hspell.h"
#include "locales.h"
#include "util.h"
#include "files.h"

#define SNATIVE_SET "UTF-8"

static HUNSPELL speller;

DICTIONARYSET hs_dictionaries[20];
unsigned int hs_dictionaryCount;
char * hs_mdextn = ".dic";
char * hs_afextn = ".aff";

static void pathforobject(char *path, DICTIONARYSET * set, char * type);
static void closehlist(HLIST * hlptr);
static BOOL ismaindictionary(char * path);	// tests for genuine dic
static int wcompare(const void * r1, const void * r2);	// compares words

/******************************************************************************/
static void pathforobject(char *path, DICTIONARYSET * set, char * type)

{
#if defined CIN_MAC_OS
	sprintf(path, "%s/%s.%s", set->path,set->root,type);
#else
	sprintf(path, "%s\\%s%s", set->path,set->root,type);
#endif
}
/******************************************************************************/
static void closehlist(HLIST * hlptr)

{
	if (hlptr)	{
		Hunspell_free_list(speller.handle,&hlptr->list,hlptr->entrycount);
		memset(hlptr,0, sizeof(HLIST));
	}
}
/******************************************************************************/
static BOOL ismaindictionary(char * path)	// tests for genuine dic

{
    FILE *dic = fopen(path,"r");
    char buf[200];

	if (dic)	{
		if (fgets(buf,200,dic))	{
			unsigned int entries = atol(buf);	// first line should be entry count
			if (entries > 0)
				return YES;
		}
		fclose(dic);
	}
	return NO;
}
/******************************************************************************/
void hspell_init(void)

{
#if defined CIN_MAC_OS
	NSArray * libpaths;
	NSString * spelldir;
	
	spelldirectories = [[NSMutableArray alloc ] initWithCapacity:3];
	libpaths = NSSearchPathForDirectoriesInDomains(NSLibraryDirectory, NSUserDomainMask, YES);
	spelldir = [[libpaths objectAtIndex:0] stringByAppendingPathComponent:@"Spelling"];
	if (spelldir)
		[spelldirectories addObject:spelldir];
	libpaths = NSSearchPathForDirectoriesInDomains(NSLibraryDirectory, NSLocalDomainMask, YES);
	spelldir = [[libpaths objectAtIndex:0] stringByAppendingPathComponent:@"Spelling"];
	if (spelldir)
		[spelldirectories addObject:spelldir];
	libpaths = NSSearchPathForDirectoriesInDomains(NSLibraryDirectory, NSSystemDomainMask, YES);
	spelldir = [[libpaths objectAtIndex:0] stringByAppendingPathComponent:@"Spelling"];
	if (spelldir)
		[spelldirectories addObject:spelldir];
#endif	
	hspell_finddictionaries();
}
/******************************************************************************/
void hspell_finddictionaries(void)

{
	unsigned int index;
	
#if defined CIN_MAC_OS
	hspell_releasedictionaries();
	for (NSString * dirpath in spelldirectories)	{
		NSArray * files = [[NSFileManager defaultManager] contentsOfDirectoryAtPath:dirpath error:NULL];
//		NSLog([files description]);
		for (NSString * dicname in files) {
			NSRange rr = [dicname rangeOfString:@".dic" options:NSBackwardsSearch|NSAnchoredSearch];
			if (rr.location != NSNotFound) {
				if (ismaindictionary([[dirpath stringByAppendingPathComponent:dicname] UTF8String]))	{	// if legit dic
					NSString * rootname = [dicname substringToIndex:rr.location];
					char * root = (char *)[rootname UTF8String];
					if (!hspell_dictionarysetforlocale(root) && hs_dictionaryCount < DICMAX)	{	// if don't already have this dictionary and have room
						char * displayname = displayNameForLocale(root);
						NSString * dicname = [rootname stringByAppendingString:@".aff"];
						
						hs_dictionaries[hs_dictionaryCount].root = malloc(strlen(root)+1);
						strcpy(hs_dictionaries[hs_dictionaryCount].root,root);
						hs_dictionaries[hs_dictionaryCount].displayname = malloc(strlen(displayname)+1);
						strcpy(hs_dictionaries[hs_dictionaryCount].displayname,displayname);
//						if (!hs_dictionaries[hs_dictionaryCount].displayname)	// if don't have display name
//							hs_dictionaries[hs_dictionaryCount].displayname = hs_dictionaries[hs_dictionaryCount].root;	// show root
						hs_dictionaries[hs_dictionaryCount].path = malloc([dirpath length]+1);
						strcpy(hs_dictionaries[hs_dictionaryCount].path,[dirpath UTF8String]);
						if (![files containsObject:dicname])	{	// if don't have both aff and dic
							hs_dictionaries[hs_dictionaryCount].flags |= DIC_ISAUX;	// must be auxiliary dic
			//				NSLog(root);
						}
						hs_dictionaryCount++;
					}
				}
			}
		}
	}
#else
	WIN32_FIND_DATA data;
	HANDLE handle;
	TCHAR specpath[MAX_PATH];
	TCHAR basepath[MAX_PATH];
	int pindex;

	hspell_releasedictionaries();
	for (pindex = 0; pindex < 2; pindex++)	{	// for all potential dic folders
		if (!pindex)	{		// cindex directory
			file_makecinpath(basepath,TEXT(""));
			file_makecinpath(specpath,TEXT("*.dic"));
		}
		else {					// public documents folder
			file_makepublicpath(basepath,TEXT(""));
			file_makepublicpath(specpath,TEXT("*.dic"));
		}
		handle = FindFirstFile(specpath,&data);
		if (handle != INVALID_HANDLE_VALUE)	{
			do {
				TCHAR fullpath[MAX_PATH];

				nstrcpy(fullpath,basepath);
				PathAppend(fullpath,data.cFileName);
				if (ismaindictionary(fromNative(fullpath)))	{	// if legit dic
					char root[MAX_PATH];

					PathRemoveExtension(data.cFileName);
					strcpy(root,fromNative(data.cFileName)); 
					if (!hspell_dictionarysetforlocale(root) && hs_dictionaryCount < DICMAX)	{	// if don't already have this dictionary and have room
						char * displayname = displayNameForLocale(root);
						
						hs_dictionaries[hs_dictionaryCount].root = malloc(strlen(root)+1);
						strcpy(hs_dictionaries[hs_dictionaryCount].root,root);
						hs_dictionaries[hs_dictionaryCount].displayname = malloc(strlen(displayname)+1);
						strcpy(hs_dictionaries[hs_dictionaryCount].displayname,displayname);
						hs_dictionaries[hs_dictionaryCount].path = malloc(nstrlen(basepath)+1);
						strcpy(hs_dictionaries[hs_dictionaryCount].path,fromNative(basepath));
						PathRenameExtension(fullpath,toNative(hs_afextn));
						if (!PathFileExists(fullpath))	{	// if don't have both aff and dic
							hs_dictionaries[hs_dictionaryCount].flags |= DIC_ISAUX;	// must be auxiliary dic
			//				NSLog(root);
						}
						hs_dictionaryCount++;
					}
				}
			} while (FindNextFile(handle,&data));
			FindClose(handle);
		}
	}
#endif
	for (index = 0; index < hs_dictionaryCount; index++)	{	// find auxiliary sets for each language
		DICTIONARYSET * currentset = &hs_dictionaries[index];
		unsigned int sindex;
		
		currentset->sets = calloc(hs_dictionaryCount,sizeof(DICTIONARYSET *));
		for (sindex = 0; sindex < hs_dictionaryCount; sindex++)	{
			if (sindex != index && hs_dictionaries[sindex].flags&DIC_ISAUX && localeSameLanguage(currentset->root,hs_dictionaries[sindex].root))	{
				currentset->sets[currentset->setcount] = &hs_dictionaries[sindex];
				currentset->setcount++;
			}
		}
	}
}
/******************************************************************************/
void hspell_releasedictionaries(void)

{
	while (hs_dictionaryCount)	{
		hs_dictionaryCount--;
		free(hs_dictionaries[hs_dictionaryCount].root);
		free(hs_dictionaries[hs_dictionaryCount].path);
		free(hs_dictionaries[hs_dictionaryCount].sets);
	}
	memset(&hs_dictionaries,0,DICMAX *sizeof(DICTIONARYSET));
}
/******************************************************************************/
DICTIONARYSET * hspell_dictionarysetforlocale(char * locale)

{
	unsigned int index;
	
	if (locale)	{
		for (index = 0; index < hs_dictionaryCount; index++)	{
			if (!strcmp(locale, hs_dictionaries[index].root))	// if found
				return &hs_dictionaries[index];
		}
	}
	return NULL;
}
#if 0
/******************************************************************************/
char * hspell_localefordisplayname(char * name)

{
	unsigned int index;
	
	if (name)	{
		for (index = 0; index < hs_dictionaryCount; index++)	{
			if (!strcmp(name, hs_dictionaries[index].displayname))	// if found
				return hs_dictionaries[index].root;
		}
	}
	return NULL;
}
#endif
/******************************************************************************/
HUNSPELL * hspell_open(char * locale)

{
	char apath[512];
	char dpath[512];
	char * encoding;
	
	hspell_close();
	speller.dicset = hspell_dictionarysetforlocale(locale);
	if (speller.dicset)	{
		pathforobject(apath,speller.dicset,hs_afextn);
		pathforobject(dpath,speller.dicset,hs_mdextn);
		speller.handle = Hunspell_create(apath,dpath);
		encoding = Hunspell_get_dic_encoding(speller.handle);
		if (strcmp(encoding, SNATIVE_SET))	{	// if need a converter
			speller.forwardconverter = iconv_open(encoding, SNATIVE_SET);
			speller.backconverter = iconv_open(SNATIVE_SET, encoding);
			// test return from iconv_open. is -1 on error
		}
		return &speller;
	}
	return NULL;
}
/******************************************************************************/
void hspell_close(void)

{
	if (speller.handle)	{
		hspell_closepd();
		hspell_wlfree(&speller.ignoredwords);
		iconv_close(speller.forwardconverter);
		iconv_close(speller.backconverter);
		closehlist(&speller.suggestions);
		closehlist(&speller.analyses);
		closehlist(&speller.stems);
		Hunspell_destroy(speller.handle);
		memset(&speller, 0,sizeof(HUNSPELL));
	}
}
/******************************************************************************/
int hspell_openauxdic(DICTIONARYSET * dsp)	//opens aux dic and adds words from it

{
	char dpath[512];
	int err = TRUE;

	if (dsp)	{
		pathforobject(dpath,dsp,hs_mdextn);
		err = Hunspell_add_dic(speller.handle,dpath);
	}
	return !err;
}
/******************************************************************************/
int hspell_createpd(char * path)	// creates empty pd file

{
	return hspell_wlsavetofile(NULL,path,"w");
}
/******************************************************************************/
int hspell_openpd(char * path)

{
	int error;
	WORDLIST * wlp = hspell_wlfromfile(path,&error);
	WORDLIST * clp;
	
	for (clp = wlp; clp; clp = clp->next)
		hspell_addword(clp->word);
	hspell_wlfree(&wlp);
	strcpy(speller.userdicpath,path);
	return !error;
}
/******************************************************************************/
int hspell_savepd(void)	// saves pd

{
	if (hspell_wlsavetofile(speller.newwords,speller.userdicpath,"a"))	{	// if can append to file
		hspell_wlfree(&speller.newwords);
		return TRUE;
	}
	return FALSE;
}
/******************************************************************************/
int hspell_closepd(void)	// saves and closes pd; removes its words from list

{
	if (hspell_savepd())	{	// if can save
		WORDLIST * wlp = hspell_wlfromfile(speller.userdicpath,NULL);	// recover full word list
		WORDLIST * clp;
		
		for (clp = wlp; clp; clp = clp->next)
			hspell_removeword(clp->word);
		hspell_wlfree(&wlp);
		*speller.userdicpath = '\0';
		return TRUE;
	}
	return FALSE;
}
/******************************************************************************/
int hspell_addword(char * word)

{
	int err = TRUE;
//	char * tptr, *w;
	
	word = hspell_convertword(word, TODICTIONARY);
#if 1
//	for (tptr = word; (tptr = strchr(tptr,'/')); tptr += 2)	{	// while have slashes
//		memmove(tptr+1, tptr, strlen(tptr)+1);	// insert protection
//		*tptr = '\\';
//	}
	err = Hunspell_add(speller.handle, word);
#else
	if (((w = strstr(word + 1, "/")) == NULL)) {	// if no affix
		// !! how to handle this when user inputs ?
        if (*word == '*')
			err = Hunspell_remove(speller.handle, word + 1);
		else
			err = Hunspell_add(speller.handle, word);
	}
	else {		// deal with affix
		char c = *w;
		
		*w = '\0';
		err = Hunspell_add_with_affix(speller.handle,word, w + 1); // word/pattern
		*w = c;		// restore orig string
	}
#endif
	return !err;
}
/******************************************************************************/
void hspell_removeword(char * word)

{
	word = hspell_convertword(word, TODICTIONARY);
	Hunspell_remove(speller.handle,word);
}
/******************************************************************************/
int hspell_addpdword(char * word)	// adds word to pd

{
	hspell_wladdword(&speller.newwords,word);
	return hspell_addword(word);
}
/******************************************************************************/
void hspell_removeignoredwords(void)	// removes all words on ignored list
{
	WORDLIST * clp;
	
	for (clp = speller.ignoredwords; clp; clp = clp->next)
		hspell_removeword(clp->word);
	hspell_wlfree(&speller.ignoredwords);
}
/******************************************************************************/
int hspell_addignoredword(char * word)	// adds word to ignore list

{
	hspell_wladdword(&speller.ignoredwords,word);
	return hspell_addword(word);
}
/******************************************************************************/
int hspell_spellword(char * word)

{	
#if 1
	return Hunspell_spell(speller.handle,hspell_convertword(word, TODICTIONARY));
#else
	int info;
	char * root;
	int ok;
	
	ok = Hunspell_spell_extended(speller.handle,hspell_convertword(word, TODICTIONARY),&info,&root);
	if (ok)	{
		if (info & SPELL_COMPOUND)
			NSLog(@"compound");
		else if (root)	{
			NSLog (@"root: %s", root);
//			free(root);
		}
		return TRUE;
	}
	return FALSE;
#endif
}
/******************************************************************************/
int hspell_suggest(char * word)		// get suggestions

{	
	closehlist(&speller.suggestions);
	speller.suggestions.entrycount = Hunspell_suggest(speller.handle,&speller.suggestions.list,word);
	return speller.suggestions.entrycount;
}
/******************************************************************************/
int hspell_analyze(char * word)

{	
	closehlist(&speller.analyses);
	speller.analyses.entrycount = Hunspell_analyze(speller.handle,&speller.analyses.list,word);
	return speller.analyses.entrycount;
}
/******************************************************************************/
int hspell_stem(char * word)

{	
	closehlist(&speller.stems);
	speller.stems.entrycount = Hunspell_analyze(speller.handle,&speller.stems.list,word);
	return speller.stems.entrycount;
}
/******************************************************************************/
char * hspell_convertword(char * text, int direction)	// converts to/from dictionary character

{
	iconv_t converter = (direction == TODICTIONARY ? speller.forwardconverter : speller.backconverter);
	
	if (converter)	{	// if need conversion
		static char outbuf[256];
		char * source = text;
		char * dest = outbuf;
		size_t sourcecount = strlen(text)+1;
		size_t destcount = 256;
		size_t length;
		
		length  = iconv(converter,&source,&sourcecount,&dest,&destcount);
		if ((int)length < 0)	// if error
			NSLog("encoding: %s [%d]", text, sourcecount);
		return outbuf;
	}
	return text;
}
/******************************************************************************/
WORDLIST * hspell_wlfromfile(char * path, int *error)	// makes word list from file

{
    FILE *dic = fopen(path,"r");
	WORDLIST *wlp = NULL;
    char buf[200];
	
	if (error)
		*error = TRUE;
	if (dic)	{
		while (fgets(buf,200,dic)) {			
			if (*(buf + strlen(buf) - 1) == '\n')
				*(buf + strlen(buf) - 1) = '\0';
			hspell_wladdword(&wlp,buf);
	    }
		fclose(dic);
		if (error)
			*error = FALSE;
	}
	return wlp;
}
/******************************************************************************/
BOOL hspell_wlsavetofile(WORDLIST *wlp, char * path, char * mode)	// writes word list to file

{
    FILE * dic = fopen(path,mode);
	BOOL ok = FALSE;
	
	if (dic)	{		
		while (wlp) {	// while words in list
//			NSLog(@"Saving: %s",wlp->word);
			if (*wlp->word)
				fprintf(dic,"%s\n",wlp->word);
			wlp = wlp->next;
		}
		ok = !fclose(dic);
	}
	return ok;
}
/******************************************************************************/
void hspell_wladdword(WORDLIST ** wlpp,char * word)	// add word to linked list

{
	WORDLIST * clp = malloc(sizeof(WORDLIST));
	
	clp->word = strdup(word);
	clp->next = *wlpp;
	*wlpp = clp;
}
/******************************************************************************/
void hspell_wlfree(WORDLIST ** wlpp)	// free word list

{
	WORDLIST * wlp = *wlpp;
	while (wlp) {	// while words in list
		WORDLIST * clp = wlp;
		
		wlp = wlp->next;
		free(clp->word);
		free(clp);
	}
	*wlpp = wlp;
}
/******************************************************************************/
void hspell_wlsort(WORDLIST *wlp)	// sorts word list in place

{
	int count, wcount;
	WORDLIST * wlcp;
	
	for (count = 0, wlcp = wlp; wlcp; wlcp = wlcp->next)	// find how many words
		count++;
	if (count)	{
		char ** warray = malloc(count * sizeof(char *));	// get array for WORDLIST pointers
		
		for (wcount = 0, wlcp = wlp; wcount < count; wlcp = wlcp->next, wcount++)
			warray[wcount] = wlcp->word;
		qsort(warray, count,sizeof(char *),wcompare);
		for (count = 0, wlcp = wlp; wlcp; wlcp = wlcp->next, count++)	// replace word pointers with sorted ones from array
			wlcp->word = warray[count];
		free(warray);
	}
}
/******************************************************************************/
static int wcompare(const void * r1, const void * r2)	// compares words

{
	UChar us1[1024], us2[1024];
	UErrorCode error = U_ZERO_ERROR;
	int32_t olength;
	
	u_strFromUTF8(us1,sizeof(us1),&olength,*(char **)r1,-1,&error);
	error = U_ZERO_ERROR;
	u_strFromUTF8(us2,sizeof(us2),&olength,*(char **)r2,-1,&error);
	return u_strcasecmp(us1,us2,0);
}
/******************************************************************************/
UChar * hspell_wlToUtext(WORDLIST *wlp, int *wordcount)	// makes sorted unicode text list from word list

{
	WORDLIST * twp;
	unsigned int length;
	UChar * base, * sptr;

	hspell_wlsort(wlp);
	for (*wordcount = 0, length = 0, twp = wlp; twp; twp = twp->next)	{
		length += strlen(twp->word)+2;	// add 2 for cr/lf
		*wordcount += 1;
	}
	base = malloc(length*sizeof(UChar)+2);	// add for terminating NULL
	for (sptr = base, twp = wlp; twp; twp = twp->next)	{
		UErrorCode error = U_ZERO_ERROR;
		int32_t olength;
		
		u_strFromUTF8(sptr,length-(sptr-base),&olength,twp->word,-1,&error);
		sptr += olength;
		*sptr++ = '\r';
		*sptr++ = '\n';

	}
	*sptr = '\0';
	return base;
}
/******************************************************************************/
WORDLIST * hspell_wlFromUtext(UChar * base)	// makes wordlist from unicode text

{
	static UChar spstr[] = {' ','\r','\n'};
	WORDLIST * wlp = NULL;
	unsigned int length = u_strlen(base);
	UChar * sptr;
	
	for (sptr = base; sptr < base+length;)	{
		UChar * eptr = u_strpbrk(sptr,spstr);
		int slen;
		
		if (eptr)
			*eptr = '\0';
		slen = u_strlen(sptr);
		if (slen)	{	// if have something to add
			char u8str[512];
			UErrorCode error = U_ZERO_ERROR;
			int32_t olength;
			
			u_strToUTF8(u8str,sizeof(u8str),&olength,sptr,-1,&error);
			hspell_wladdword(&wlp,u8str);
		}
		sptr += slen+1;
	}
	return wlp;
}
