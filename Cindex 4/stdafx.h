// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once
#define UNICODE

#define STRICT
#define NOCOMM

#define YES TRUE
#define NO FALSE

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <shlwapi.h>
#include <richedit.h>

#include <stdlib.h>
#include <limits.h>
#include <stdio.h>
#include <time.h>
#include <malloc.h>
#include <memory.h>

#include <CRTDBG.H>
#include "resource.h"
#include "resource.hm"

#include "unicode/ustring.h"
#include "unicode/uchar.h"
#include "unicode/ucol.h"
#include "unicode/ustdio.h"

#if defined UNICODE
#define nstrcpy(A,B) wcscpy(A,B)
#define nstrncpy(A,B,C) wcsncpy(A,B,C)
#define nstrcmp(A,B) wcscmp(A,B)
#define nstrncmp(A,B,C) wcsncmp(A,B,C)
#define nstrlen(A) wcslen(A)
#define nstrcat(A,B) wcscat(A,B)
#define nstricmp(A,B) u_strcasecmp(A,B,0)
#define nfopen(A,B) _wfopen(A,B)
#define nstrchr(A,B) wcschr(A,B)
#define nstrrchr(A,B) wcsrchr(A,B)
#define nstrstr(A,B) wcsstr(A,B)
#define nstrtol(A,B,C) wcstol(A,B,C)
#define natoi(A) _wtoi(A)
#define nstrpbrk(A,B) wcspbrk(A,B)
#define nstrlwr(A) _wcslwr(A)
#else
#define nstrcpy(A,B) strcpy(A,B)
#define nstrncpy(A,B,C) strncpy(A,B,C)
#define nstrcmp(A,B) strcmp(A,B)
#define nstrncmp(A,B,C) strncmp(A,B,C)
#define nstrlen(A) strlen(A)
#define nstrcat(A,B) strcat(A,B)
#define nstricmp(A,B) _stricmp(A,B)
#define nfopen(A,B) fopen(A,B)
#define nstrchr(A,B) strchr(A,B)
#define nstrrchr(A,B) strrchr(A,B)
#define nstrstr(A,B) strstr(A,B)
#define nstrtol(A,B,C) strtol(A,B,C)
#define natoi(A) atoi(A)
#define nstrpbrk(A,B) strpbrk(A,B)
#endif

#include "osdependencies.h"
#include "cintypes.h"
#include "stringtypes.h"
#include "recordparams.h"
#include "fontmap.h"
#include "referenceparams.h"
#include "formatparams.h"
#include "indexparams.h"
#include "sortparams.h"
#include "recordparams.h"
#include "headparams.h"

#include "searchparams.h"
#include "groupparams.h"

#include "printformat.h"
#include "indexdocument.h"

#include "cglobal.h"

#pragma warning(error : 4002)	/* to many params to macro */
#pragma warning(error : 4003)	/* too few params to macro */
#pragma warning(error : 4021)	/* too many params */
#pragma warning(error : 4020)	/* too few actual params */
#pragma warning(error : 4133)	/* incompatible objects */
#pragma warning(error : 4047)	/* different levels of indirection */
