#pragma once
BOOL mcr_isplaying(void);	/* returns TRUE if playing */
void mcr_save(void);		/* saves macros */
void mcr_load(void);		/* loads macros */
void mcr_record(int index);		/* starts/stops recording */
void mcr_play(int index);		/* stops recording */
BOOL mcr_cancel(void);		/* cancels journalling */
void mcr_setmenu(int disable);		/* sets macro menu items */
void mcr_setname(HWND hwnd);		/* assigns name to current macro */
