#pragma once
#ifndef ATSS_REQUESTS_H
#define ATSS_REQUESTS_H 1
#ifndef UNICODE
#define UNICODE
#endif

#include <windows.h>
#include <wininet.h>
#include <stdlib.h>
#include <string.h>

#pragma comment(lib, "wininet.lib")
#define DEFAULT_HTTP_TIMEOUT 4000
typedef struct {
	LPSTR response;      /* pointer to heap-allocated response (caller must free) */
	SIZE_T response_len; /* length in bytes of response */
	BOOL errored;        /* TRUE if request failed or HTTP error code >= 400 */
	BOOL serverok;       /* TRUE if server indicated OK via leading '+' */
} http_response_t;

/* Sends an HTTP GET. Caller must free(resp.response) when resp.response != NULL.
   The server's response body MUST start with either '+' or '-' which indicates serverok.
   The returned response excludes that first byte. */
static http_response_t send_http_get(LPCSTR url, DWORD timeoutMS)
{
	http_response_t resp = {0};
	HINTERNET hInet = NULL, hUrl = NULL;
	DWORD status = 0, statusLen = sizeof(status);
	CHAR buffer[4096];
	SIZE_T capacity = 0, used = 0;

	hInet = InternetOpenA("MyAgent", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
	if (!hInet) {
		resp.errored = TRUE;
		return resp;
	}

	if (!InternetSetOption(hInet, INTERNET_OPTION_CONNECT_TIMEOUT, &timeoutMS, sizeof(timeoutMS))) {
		/* not fatal, continue */
	}

	hUrl = InternetOpenUrlA(hInet, url, NULL, 0, INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE, 0);
	if (!hUrl) {
		resp.errored = TRUE;
		InternetCloseHandle(hInet);
		return resp;
	}

	/* Query HTTP status code if available */
	if (HttpQueryInfoA(hUrl, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &status, &statusLen, NULL)) {
		if (status >= 400) {
			resp.errored = TRUE;
			InternetCloseHandle(hUrl);
			InternetCloseHandle(hInet);
			return resp;
		}
	}

	/* Read response into dynamically growing buffer */
	for (;;) {
		DWORD read = 0;
		if (!InternetReadFile(hUrl, buffer, (DWORD)sizeof(buffer), &read)) {
			resp.errored = TRUE;
			break;
		}
		if (read == 0)
			break;

		if (used + read + 1 > capacity) {
			SIZE_T newCap = capacity ? capacity * 2 : 8192;
			while (newCap < used + read + 1) newCap *= 2;
			LPSTR tmp = (LPSTR)realloc(resp.response, newCap);
			if (!tmp) { resp.errored = TRUE; break; }
			resp.response = tmp;
			capacity = newCap;
		}

		memcpy(resp.response + used, buffer, read);
		used += read;
	}

	if (!resp.errored) {
		/* Ensure null termination for convenience */
		if (resp.response) resp.response[used] = '\0';
		resp.response_len = used;
		if (resp.response_len == 0) {
			resp.errored = TRUE;
		} else {
			/* first byte indicates server status */
			if (resp.response[0] == '+') resp.serverok = TRUE;
			else resp.serverok = FALSE;

			/* remove first byte from buffer by shifting left one and resizing */
			if (resp.response_len > 0) {
				SIZE_T newLen = resp.response_len - 1;
				if (newLen > 0) memmove(resp.response, resp.response + 1, newLen);
				resp.response[newLen] = '\0';
				resp.response_len = newLen;
				/* shrink to fit (best-effort) */
				{
				LPSTR s = (LPSTR)realloc(resp.response, resp.response_len + 1);
					if (s) resp.response = s;
				}
			}
		}
	} else {
		/* free any partially allocated buffer on error */
		if (resp.response) { free(resp.response); resp.response = NULL; resp.response_len = 0; }
	}

	InternetCloseHandle(hUrl);
	InternetCloseHandle(hInet);
	return resp;
}

/* Sends an HTTP POST. Caller must free(resp.response) when resp.response != NULL.
   contentType may be NULL to use "application/x-www-form-urlencoded". */
static http_response_t send_http_post(LPCSTR url, LPCSTR body, SIZE_T body_len, LPCSTR contentType, DWORD timeoutMS)
{
	http_response_t resp = {0};
	HINTERNET hInet = NULL, hConnect = NULL, hRequest = NULL;
	URL_COMPONENTSA components;
	CHAR host[256] = {0};
	CHAR path[2048] = {0};
	INTERNET_PORT port = 80;
	BOOL useHttps = FALSE;
	CHAR buffer[4096];
	SIZE_T capacity = 0, used = 0;

	if (!url) { resp.errored = TRUE; return resp; }

	ZeroMemory(&components, sizeof(components));
	components.dwStructSize = sizeof(components);
	components.lpszHostName = host;
	components.dwHostNameLength = sizeof(host);
	components.lpszUrlPath = path;
	components.dwUrlPathLength = sizeof(path);

	if (!InternetCrackUrlA(url, 0, 0, &components)) {
		resp.errored = TRUE; return resp;
	}

	if (components.nScheme == INTERNET_SCHEME_HTTPS) { useHttps = TRUE; port = components.nPort; }
	else { useHttps = FALSE; port = components.nPort ? components.nPort : INTERNET_DEFAULT_HTTP_PORT; }

	hInet = InternetOpenA("MyAgent", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
	if (!hInet) { resp.errored = TRUE; return resp; }

	hConnect = InternetConnectA(hInet, host, port, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
	if (!hConnect) { resp.errored = TRUE; InternetCloseHandle(hInet); return resp; }

	LPCSTR acceptTypes[] = { "*/*", NULL };
	DWORD flags = useHttps ? INTERNET_FLAG_SECURE : 0;

	hRequest = HttpOpenRequestA(hConnect, "POST", path, NULL, NULL, acceptTypes, flags | INTERNET_FLAG_NO_CACHE_WRITE, 0);
	if (!hRequest) { resp.errored = TRUE; InternetCloseHandle(hConnect); InternetCloseHandle(hInet); return resp; }

	if (!InternetSetOption(hRequest, INTERNET_OPTION_CONNECT_TIMEOUT, &timeoutMS, sizeof(timeoutMS))) {
		/* continue */
	}

	CHAR hdr[256];
	if (!contentType) contentType = "application/x-www-form-urlencoded";
	_snprintf_s(hdr, sizeof(hdr), _TRUNCATE, "Content-Type: %s", contentType);

	BOOL sent = HttpSendRequestA(hRequest, hdr, (DWORD)strlen(hdr), (LPVOID)body, (DWORD)body_len);
	if (!sent) {
		resp.errored = TRUE;
		InternetCloseHandle(hRequest);
		InternetCloseHandle(hConnect);
		InternetCloseHandle(hInet);
		return resp;
	}

	/* Check HTTP status code */
	DWORD status = 0, statusLen = sizeof(status);
	if (HttpQueryInfoA(hRequest, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &status, &statusLen, NULL)) {
		if (status >= 400) {
			resp.errored = TRUE;
			InternetCloseHandle(hRequest);
			InternetCloseHandle(hConnect);
			InternetCloseHandle(hInet);
			return resp;
		}
	}

	/* Read response */
	for (;;) {
		DWORD read = 0;
		if (!InternetReadFile(hRequest, buffer, (DWORD)sizeof(buffer), &read)) { resp.errored = TRUE; break; }
		if (read == 0) break;

		if (used + read + 1 > capacity) {
			SIZE_T newCap = capacity ? capacity * 2 : 8192;
			while (newCap < used + read + 1) newCap *= 2;
			LPSTR tmp = (LPSTR)realloc(resp.response, newCap);
			if (!tmp) { resp.errored = TRUE; break; }
			resp.response = tmp;
			capacity = newCap;
		}

		memcpy(resp.response + used, buffer, read);
		used += read;
	}

	if (!resp.errored) {
		if (resp.response) resp.response[used] = '\0';
		resp.response_len = used;
		if (resp.response_len == 0) {
			resp.errored = TRUE;
		} else {
			if (resp.response[0] == '+') resp.serverok = TRUE;
			else resp.serverok = FALSE;

			if (resp.response_len > 0) {
				SIZE_T newLen = resp.response_len - 1;
				if (newLen > 0) memmove(resp.response, resp.response + 1, newLen);
				resp.response[newLen] = '\0';
				resp.response_len = newLen;
				{
				LPSTR s = (LPSTR)realloc(resp.response, resp.response_len + 1);
					if (s) resp.response = s;
				}
			}
		}
	} else {
		if (resp.response) { free(resp.response); resp.response = NULL; resp.response_len = 0; }
	}

	InternetCloseHandle(hRequest);
	InternetCloseHandle(hConnect);
	InternetCloseHandle(hInet);
	return resp;
}
#endif //!defined(ATSS_REQUESTS_H)