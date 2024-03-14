#pragma once
void edit_delrestore(HWND wptr);	/* dels/restores records */
void edit_switchtag(HWND wptr,int tagid);	/* tags/untags records records */
void edit_removemark(HWND hwnd);		// removes marks from records
short edit_duplicate(HWND wptr);	/* duplicates record(s) */
void edit_demote(HWND hwnd);	// demotes headings
void edit_preferences(void);	/* sets preferences */
void edit_savegroup(HWND wptr);	/* makes permanent group from temp */
void edit_managegroups(HWND hwnd);	/* manages groups */
short env_tobase(int unit, float eval);	/* converts from expression to base */
float env_toexpress(int unit, short bval);	/* converts from base to unit of espression */
