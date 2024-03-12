#pragma once

enum {				/* group flags */
	GF_SEARCH = 1,		/* formed from search */
	GF_RANGE = 2,		/* formed from numerical range */
	GF_SELECT = 8,		/* formed from selection */
	GF_COMBINE = 4, 	/* formed from some combination of groups */
	GF_LINKED = 16,		/* linked */
	GF_REVISED = 32,	/* revised */
};

#define GROUPMAXSIZE 10000		// unit in which group size is incremented

#define groupsize(gp) (sizeof(GROUP)+(gp)->rectot*sizeof(RECN))
#define nextgroup(gp) ((GROUP *)((char *)gp+groupsize(gp)))
#define MAXGSIZE (sizeof(GROUP)+GROUPMAXSIZE*sizeof(RECN))

void grp_checkparams(INDEX * FF);	// checks/fixes group sort parameters
int grp_buildmenu(INDEX * FF);	/* builds and checks group menu */
int grp_buildlist(INDEX * FF, HWND lh);	/* builds a list of groups */
BOOL grp_checkintegrity(INDEX * FF);	// checks integrity of groups
BOOL grp_repair(INDEX * FF);	// repairs groups as far as possible
GROUPHANDLE grp_startgroup(INDEX * FF);		/* initializes group */
void grp_installtemp(INDEX * FF, GROUPHANDLE gh);	/* installs temporary group */
BOOL grp_make(INDEX * FF, GROUPHANDLE gh, char *name, short oflag);	// adds group to file
BOOL grp_install(INDEX * FF, char *name);	/* opens & checks & installs group  */
void grp_closecurrent(INDEX * FF);		/* closes current group */
void grp_dispose(GROUPHANDLE gh);		/* discards group  */
RECN grp_getstats(INDEX * FF, GROUPHANDLE gh, COUNTPARAMS * csptr);		/* gets stats on group */
RECN grp_buildfromcheck(INDEX * FF, GROUPHANDLE * gh);	// builds group for syntax errors
RECN grp_buildfromsearch(INDEX * FF, GROUPHANDLE *gh);	/* adds search hits to group file  */
RECN grp_buildfromrange(INDEX * FF, GROUPHANDLE *gh, RECN first, RECN last, short stype);	/* makes group from selection or numerical range */
GROUPHANDLE grp_open(INDEX * FF, char *gname);	/* opens group  */
short grp_link(INDEX * FF, GROUPHANDLE *gh);	/* add cross-refs to group */
void grp_revise(INDEX * FF, GROUPHANDLE *gh);	/* rebuilds group  */
void grp_delete(INDEX * FF, char * gname);	/* delete group by name */
RECORD * grp_lookup(INDEX * FF,GROUPHANDLE gh, char * string, int subflag);	/* does bsearch for group entry */
