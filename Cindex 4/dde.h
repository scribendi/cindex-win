#pragma once
extern BOOL d_ddeopen;
extern DWORD d_ddeinst;		/* ddeml instance */

void dde_setup(void);		/* initializes ddeml and sets up server */
void dde_close(void);		/*  cleans up ddeml */
BOOL dde_sendcheck(void);	/*  returns TRUE if this copy of CINDEX running */
