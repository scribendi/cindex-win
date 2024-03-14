#pragma once

#define MAPMARGIN 500

#if 0
typedef struct {
	TCHAR operation[50];	// name of operation
	HEAD header;
	char * image;
	char * imagesize;
} INDEXCOPY;
#endif
void index_cleartags(INDEX * FF);	// clears tags, etc
int index_checkintegrity(INDEX * FF, RECN rtot);	// checks integrity of index
int index_repair(INDEX * FF);	// repairs index
INDEX * index_insert(INDEX * FF);		/* inserts empty (but linked) index struct */
BOOL index_setworkingsize(INDEX * FF, RECN extrarecords);	// sets working size with margin
BOOL index_setsize(INDEX * FF, RECN total,int recsize, RECN margin);	// sets specified size with margin
void index_flushall(void);		/* flushes all indexes */
short index_close(INDEX * FF);		/* flushes, closes, releases index and structures */
short index_cut(INDEX * FF);		/* saves, frees index, restores links */
INDEX * index_byname(TCHAR *name);		/* returns pointer to index with name 'name' */
INDEX * index_byfspec(TCHAR * path);	/* returns pointer to index from filespec */
INDEX * index_byindex(short index);	/* returns pointer to index at index posn in list */
short index_findindex(INDEX * target);	/* index position of target index */
BOOL index_install(INDEX * FF);	// miscellaneous setup
void index_setdefaults(INDEX * FF);		/* fills default structure for index  */
BOOL index_flush(INDEX * FF);		/* flushes all records & header */
BOOL index_readhead(INDEX * FF);		// reads header
void index_markdirty(INDEX * FF);		// marks index as dirty
BOOL index_writehead(INDEX * FF);		// writes header
short index_closefile(INDEX * FF);		/* closes file */
void index_getmodtime(INDEX * FF);		/* get time of last change on file */
void index_setmodtime(INDEX * FF);		/* set time of last change on file */
TCHAR * index_displayname(INDEX * FF);	// returns display name
INDEX * index_front(void);			/* gets index with frontmost window (not necess active) */
BOOL index_sizeforgroups(INDEX * FF, unsigned long gsize);	// sets specified size with margin

