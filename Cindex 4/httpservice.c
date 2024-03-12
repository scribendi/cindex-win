#include "stdafx.h"
#include <winhttp.h>
#include "commands.h"
#include "errors.h"
#include "httpservice.h"
#include "util.h"
#include "json.h"
#include "registry.h"

static time_t get_time(char * tstring);

// https://github.com/udp/json-parser

/****************************************************************/
BOOL http_connect(BOOL silent)		// checks data on indexres.com

{
	BOOL result = FALSE;
#if TOPREC == RECLIMIT && !READER
	DWORD dwSize = 0;
	DWORD dwDownloaded = 0;
	LPSTR pszOutBuffer;
	BOOL  bResults = FALSE;
	HINTERNET  hSession = NULL, hConnect = NULL, hRequest = NULL;

	// Use WinHttpOpen to obtain a session handle.
	hSession = WinHttpOpen(L"Cindex/4.0",  
		WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,WINHTTP_NO_PROXY_NAME,WINHTTP_NO_PROXY_BYPASS, 0);

	// Specify an HTTP server.
	if ( hSession )
		hConnect = WinHttpConnect(hSession, L"storage.googleapis.com", INTERNET_DEFAULT_HTTPS_PORT, 0);

	// Create an HTTP request handle.
	if (hConnect)
		hRequest = WinHttpOpenRequest(hConnect, L"GET", L"/indexres-d3231811-9b04-47b0-8756-5da84afef700/downloads/versionupdate.json",
			NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);

	// Send a request.
	if (hRequest)
		bResults = WinHttpSendRequest(hRequest,WINHTTP_NO_ADDITIONAL_HEADERS, 0,WINHTTP_NO_REQUEST_DATA, 0, 0, 0);

	// End the request.
	if (bResults)
		bResults = WinHttpReceiveResponse(hRequest, NULL);

	if (bResults){
		do {		// until data done
			dwSize = 0;
			if (WinHttpQueryDataAvailable(hRequest, &dwSize)) {	// Check for available data.
				pszOutBuffer = (LPSTR)calloc(dwSize+1,1);
				if (WinHttpReadData(hRequest, (LPVOID)pszOutBuffer, dwSize, &dwDownloaded))	{
					json_value * value = json_parse((json_char *)pszOutBuffer, dwSize);
					if (value)	{	// if full version info
						char * startDate, *endDate, *message, *messageTitle, *url;
						int minVersion, maxVersion, messageType, versionType;
						time_t start, end, now = time(NULL);
						int x;
						for (x = 0; x < value->u.object.length; x++) {
							char * name = value->u.object.values[x].name;
							json_value * vv = value->u.object.values[x].value;
							if (!strcmp(name, "startDate")) {
								startDate = vv->u.string.ptr;
								start = get_time(startDate);
							}
							else if (!strcmp(name, "endDate")) {
								endDate = vv->u.string.ptr;
								end = get_time(endDate);
							}
							else if (!strcmp(name, "message"))
								message = vv->u.string.ptr;
							else if (!strcmp(name, "messageTitle"))
								messageTitle = vv->u.string.ptr;
							else if (!strcmp(name, "url"))
								url = vv->u.string.ptr;
							else if (!strcmp(name, "minVersion"))
								minVersion = vv->u.integer;
							else if (!strcmp(name, "maxVersion"))
								maxVersion = vv->u.integer;
							else if (!strcmp(name, "messageType"))
								messageType = vv->u.integer;
							else if (!strcmp(name, "versionType"))
								versionType = vv->u.integer;
						}
#ifdef PUBLISH
						if (CINVERSION >= minVersion && CINVERSION <= maxVersion && versionType&U_WIN_PUB) {	// if potentially eligible for message
#else
						if (CINVERSION >= minVersion && CINVERSION <= maxVersion && versionType&U_WIN) {	// if potentially eligible for message
#endif
						if (start <= now && end >= now) {	// if within date range
								time_t lastcheck = 0;
								DWORD size = sizeof(time_t);
								reg_getkeyvalue(K_GENERAL, MESSAGECHECK, &lastcheck, &size);		// setup if prior save
								if (!lastcheck || now > lastcheck + 86400 * 14) {	// show message every 14 days
									if (sendinfooption(toNative(messageTitle), INFO_UPDATEAVAILABLE, message))
										result = http_runupdate(toNative(url));
									reg_setkeyvalue(K_GENERAL, MESSAGECHECK, REG_BINARY, &now, sizeof(now));	// save
								}
							}
						}
						json_value_free(value);
					}
				}
				free(pszOutBuffer);
			}
		} while(dwSize > 0);
	}
	if (!bResults && !silent)	// Report any errors.
		senderr(ERR_NOCONNECTION,WARN,"www.example.com");

	// Close any open handles.
	if (hRequest )
		WinHttpCloseHandle(hRequest);
	if (hConnect )
		WinHttpCloseHandle(hConnect);
	if (hSession )
		WinHttpCloseHandle(hSession);
#endif
	return(result);
}
/****************************************************************/
BOOL http_runupdate(TCHAR * URL)	// opens browser to load and run update

{
	HINSTANCE result;

	result = ShellExecute(NULL, TEXT("open"), URL, NULL, NULL, SW_SHOWNORMAL);
	if ((int)result > 32)	{
		SendMessage(g_hwframe,WM_CLOSE,0,0);
		return TRUE;
	}
	return FALSE;
}
/****************************************************************/
static time_t get_time(char * tstring)

{
	struct tm rt;
	memset(&rt,0,sizeof(rt));
	int scount = sscanf(tstring, "%d-%d-%d", &rt.tm_year, &rt.tm_mon, &rt.tm_mday);
	if (scount == 3) {
		rt.tm_year -= 1900;
		rt.tm_mon -= 1;
		return timegm(&rt);
	}
	return 0;
}
