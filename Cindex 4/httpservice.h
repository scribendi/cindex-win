#pragma once

enum {
	U_MAC = 1,
	U_WIN = 2,
	U_WIN_PUB = 4
};

BOOL http_connect(BOOL silent);		// checks data on indexres.com
BOOL http_runupdate(TCHAR * URL);	// opense browser to load and run update
