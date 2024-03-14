#pragma once
#define getaddress(FF,NN) ((RECORD *)(FF->mf.base+HEADSIZE+(NN-1)*FF->wholerec))

RECN rec_number(RECORD * recptr);
RECORD *rec_getrec(INDEX * FF, RECN n);	 /* returns pointer to record */
int rec_writerec(INDEX * FF, RECORD * p);		/* writes record */
RECORD * rec_writenew(INDEX * FF, char *rtext);   /* forms & writes new rec to index */
void rec_stamp(INDEX * FF, RECORD * recptr);   /* stamps record with date, initials, mod */
RECORD * rec_makenew(INDEX * FF, char * rtext, RECN num);   /* forms new record */
unsigned short rec_propagate (INDEX * FF, RECORD * recptr, char * origptr, struct numstruct * nptr);		/* propagates changes to identical lower records */
char * rec_uniquelevel(INDEX * FF, RECORD * recptr, short *hlevel, short * sprlevel, short *hidelevel, short * clevel);   /* finds level at which heading is unique */
void rec_getprevnext(INDEX * FF, RECORD * recptr, RECN * prevptr, RECN * nextptr, RECORD * (*skip)(INDEX *, RECORD *, short));   /* finds next & prev records */
short rec_strip(INDEX * FF, char * pos);	  /* strips empty strings from record; to min */
void rec_pad(INDEX *FF, char *string);	  /* expands xstring to min fields */
short rec_compress(INDEX * FF, RECORD * curptr, char jchar);	  /* compresses excess fields to maxfields, by combining from lowest */
int rec_checkfields(INDEX * FF, RECORD * recptr);	// checks syntax of fields
