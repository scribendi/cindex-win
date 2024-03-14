#pragma once

BOOL regex_validexpression(char * string, int flags);		// validates pattern
URegularExpression * regex_build(char * string, int flags);		// sets up regex from string
char * regex_find(URegularExpression *regex, char * text, int flags, short * length);		// find regex match
int regex_groupcount(URegularExpression *regex);		// returns number of capture groups
char * regex_textforgroup(URegularExpression *regex, int cgroup, char * source, int * matchlen);		// returns start & length of specified capture group
BOOL regex_replace(URegularExpression *regex, char * text, char * replacement);		// replaces all matches in source with replacement
