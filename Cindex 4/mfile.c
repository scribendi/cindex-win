#include "stdafx.h"
#include "mfile.h"

/*****************************************************************************/
BOOL mfile_open(MFILE *mf, TCHAR * path, DWORD access, DWORD share, DWORD cflags,DWORD flags,long extend)	// opens file with mapped memory

{
	memset(mf, 0, sizeof(MFILE));
	mf->fref = CreateFile(path,access,share,NULL,cflags,FILE_ATTRIBUTE_NORMAL|flags,0);
	if (mf->fref != INVALID_HANDLE_VALUE)	{	// if have file
		DWORD cursize = GetFileSize(mf->fref,NULL);

		if (cursize != INVALID_FILE_SIZE)	{	// if has size
			mf->readonly = access&GENERIC_WRITE ? FALSE : TRUE;
			if (mf->readonly)
				extend = 0;
			if (extend && !mf->readonly)	{	// if want to extend
				// deal with errors here
				SetFilePointer(mf->fref,cursize+extend,NULL,FILE_BEGIN);
				SetEndOfFile(mf->fref);
			}
			if (cursize+extend)	{	// if file has any size
				mf->mref = CreateFileMapping(mf->fref,NULL,mf->readonly ? PAGE_READONLY : PAGE_READWRITE,0,cursize+extend,NULL);
				if (mf->mref)	{
					mf->base = MapViewOfFile(mf->mref,mf->readonly ? FILE_MAP_READ : FILE_MAP_WRITE,0,0,0);
					if (mf->base)	{
						mf->size = cursize+extend; 
						return TRUE;
					}
				}
			}
			else	// empty file, so no mapping currently
				return TRUE;
		}
		CloseHandle(mf->fref);	// invalid size
	}
	memset(mf, 0, sizeof(MFILE));	// clear on failure
	return FALSE;
}
/*****************************************************************************/
BOOL mfile_resize(MFILE *mf, long newsize)	// resizes mapped file

{
	if (!mf->readonly && newsize != mf->size)	{	// if not readonly and need size change
		if (mf->fref)	{/* if have mapped view */
			if (UnmapViewOfFile(mf->base))	{
				mf->base = NULL;
				CloseHandle(mf->mref);
				mf->mref = NULL;
			}
		}
		if (SetFilePointer(mf->fref,newsize,NULL,FILE_BEGIN) != INVALID_SET_FILE_POINTER)	{
			SetEndOfFile(mf->fref);
			mf->mref = CreateFileMapping(mf->fref,NULL,mf->readonly ? PAGE_READONLY : PAGE_READWRITE,0,newsize,NULL);
			if (mf->mref)	{
				mf->base = MapViewOfFile(mf->mref,mf->readonly ? FILE_MAP_READ : FILE_MAP_WRITE,0,0,0);
				if (mf->base)	{
					mf->size = newsize; 
					return TRUE;
				}
			}
			CloseHandle(mf->fref);
		}
		return FALSE;
	}
	return TRUE;
}
/*****************************************************************************/
BOOL mfile_flush(MFILE *mf)		// flushes map to file

{
	return FlushViewOfFile(mf->base,0);
}
/*****************************************************************************/
BOOL mfile_close(MFILE *mf)		// closes mapped file

{
	if (mf->fref)	{/* if have mapped view */
		if (UnmapViewOfFile(mf->base))
			CloseHandle(mf->mref);
	}
	return CloseHandle(mf->fref);
}
/*******************************************************************************/
BOOL movemappeddata(void * to, void * from, size_t size)		/* moves mapped data */

{
	__try {
		memmove(to,from,size);
		return (TRUE);
	}
	__except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION){
		return (FALSE);
	}
}
/*******************************************************************************/
BOOL copymappeddata(void * to, void * from, size_t size)		/* moves mapped data */

{
	__try {
		memcpy(to,from,size);
		return (TRUE);
	}
	__except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION){
		return (FALSE);
	}
}
