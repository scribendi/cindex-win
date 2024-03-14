#pragma once


extern FCONTROLX * typesetters[];	// indexed to efilters

BOOL formexport_write(INDEX * FF, struct expfile * ef,FCONTROLX * xptr);	/* opens & writes formatted export file */
void formexport_buildentry(INDEX * FF, RECORD * recptr);	/* builds entry of right type; appends to data */
BOOL formexport_writepastable(INDEX * FF);	// selection formatted to clipboard
void formexport_writeembedded(INDEX * FF);	/* formats output string for embedding */
void formexport_makefield(FCONTROLX * xptr, INDEX * FF, char * source);     /* formats field to output */
void formexport_gettypeindents(INDEX * FF, int level, int tabmode, int scale, int * firstindent, int * baseindent, char * tabcontrol, char * tabstops, char * tabset);	/* gets typesetting indents, appropriately scaled */
void formexport_stylenames(FCONTROLX * xptr, INDEX * FF);	// installs names to use for styles
