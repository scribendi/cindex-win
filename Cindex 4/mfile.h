#pragma once

typedef struct {
	HANDLE fref;	// file handle
	HANDLE mref;	// mapping handle
	char * base;	// view base
	long size;		// size of mapped section
	BOOL readonly;
} MFILE;

BOOL mfile_open(MFILE *mf, TCHAR * path, DWORD access, DWORD share, DWORD cflags,DWORD flags,long extend);	// opens file with mapped memory
BOOL mfile_resize(MFILE *mf, long newsize);	// resizes mapped file
BOOL mfile_flush(MFILE *mf);		// flushes map to file
BOOL mfile_close(MFILE *mf);		// closes mapped file
BOOL movemappeddata(void * to, void * from, size_t size);		/* moves mapped data */
BOOL copymappeddata(void * to, void * from, size_t size);		/* moves mapped data */
