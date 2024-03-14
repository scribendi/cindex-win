//
//  utime.h
//  Cindex
//
//  Created by Peter Lennie on 4/23/18.
//

#ifndef utime_h
#define utime_h

UDate time_dateValue(const char * dateString);
char * time_stringFromDate(UDate date);
char * time_stringFromTime(time_c time, BOOL local);	// returns time string

#endif /* utime_h */
