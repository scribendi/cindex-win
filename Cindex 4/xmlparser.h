//
//  xmlparser.h
//  Cindex
//
//  Created by Peter Lennie on 2/2/11.
//
#pragma once

#include "import.h"

int xml_parserecords(INDEX * FF, IMPORTPARAMS * imp, char * datastring, int length, char ** errmessage);		// parses data string
