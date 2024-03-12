#pragma once
#ifdef PUBLISH

#define CINAPI_VERSION 320
#define CINAPI_TOPICNAME "cindex_API_320"

extern BOOL api_active;	// TRUE when api command in process
extern BOOL api_quitondisconnect;	// TRUE forces quit on disconnect
extern TCHAR * api_printfile;		// path for printing to file

BOOL api_docommand(TCHAR *string);	/* executes dde command */
HDDEDATA api_senddata(TCHAR *command);	/* returns data */
HDDEDATA api_receivedata(TCHAR *command, void * data, unsigned long length);	/* receives data */
#else
#define api_active FALSE
#define api_printfile NULL
#endif //PUBLISH
