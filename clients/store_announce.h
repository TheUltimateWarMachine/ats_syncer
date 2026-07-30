#pragma once
#ifndef ATSS_STORE_ANNOUNCE_H
#define ATSS_STORE_ANNOUNCE_H 1
#include <Windows.h>
#include <shlobj.h>
#include <strsafe.h>
#include <stdlib.h> // for strtol
#include <string.h>


#define STORAGE_DIR "syncer"
#define STORAGE_FILE "data.txt"
#define STORAGE__CLIENT_ANNOUNCEMENT 1
#define STORAGE__HOSTNAME 2
#define STORAGE__USR 3
#define STORAGE__PWD 4
#define STORAGE__DIR 5
#define STORAGE__ACTIVES 6
#define DEF_HOST_BASE "_default_"
#define INTERNAL_DEF_HOST_BASE "http://URL/ats_syncer"
#define DEF_USR "unset"
#define DEF_PWD "unset"
#define DEF_DIR "%USERPROFILE%\\Documents\\American Truck Simulator\\mod\\user_map\\map\\PROJECT NAME"
#define MAX_HOSTNAME_SIZE 64
#define MAX_USRPWD_SIZE 16
#define MAX_DIR_SIZE 192
#define MAX_SENT_ANNOUNCEMENT_SIZE 64
#define MAX_SECTOR_LOCKS 24 //how many sectors can be locked at a time?
#define FIELDS_PER_LOCK_LINE 4
#define MAX_TIMESTAMP_LEN 48
#define SHA1_HASH_LEN 40
#define MAX_SECTORNAME_SIZE 24
#define MAX_STORED_ACTIVES 10
#define FILES_PER_SECTOR 5
#define MAX_ACTIVE_SECTORS 64
#define MAX_LOCK_REASON_LEN 48
#define MIN_LOCK_REASON_LEN 2
#define MAX_HISTORY_ENTRIES_PER_FILE 34
#define HISTORY_FIELDS_PER_ENTRY 4
#define HISTORY_MAX_SECTORLIST_SIZE 768
#define HISTORY_MAX_REASON_LEN 48
#define SECTOR_GUARD_MAX_SECTORS_PER_PROJECT 64
INT last_announcement = -1;
CHAR host_base[MAX_HOSTNAME_SIZE + 1] = { 0 };
CHAR usr[MAX_USRPWD_SIZE + 1] = { 0 };
CHAR pwd[MAX_USRPWD_SIZE + 1] = { 0 };
CHAR dir[MAX_DIR_SIZE + 1] = { 0 };
CHAR storage_file_path[MAX_PATH]; //Initialized upon calling initialize_persistent_settings()
CHAR actives[MAX_STORED_ACTIVES][MAX_SECTORNAME_SIZE + 1] = { {0} };
INT actives_count = 0;
BOOL actives_exist = FALSE;

//-1 Misformatted file
//0 STORAGE_FILE doesn't exist
//1 All is well
//Initializes host_base, usr, and pwd.
BOOL initialize_persistent_settings(VOID)
{
	HANDLE hFile = INVALID_HANDLE_VALUE;
	LARGE_INTEGER fileSize = { 0 };
	DWORD dwRead = 0;
	SIZE_T toRead = 0;

	/* Try to open existing file for reading */
	DWORD size = ExpandEnvironmentStringsA(STORAGE_DIR, (LPSTR)NULL, 0);
 	if (size == 0) return -1;
	LPSTR appdata = getenv("APPDATA");
	sprintf_s(storage_file_path, ARRAYSIZE(storage_file_path), "%s\\%s", appdata, STORAGE_DIR);
	if (!CreateDirectoryA((LPCSTR)storage_file_path, (LPSECURITY_ATTRIBUTES)NULL)) {
		DWORD err = GetLastError();
		if (err != ERROR_ALREADY_EXISTS) {
			MessageBoxA((HWND)NULL, "Failed to create directory " STORAGE_DIR " in AppData", "Failed to Initialize Persistent Settings", MB_OK | MB_ICONERROR);
			return -1;
		}
	}
	sprintf_s(storage_file_path, ARRAYSIZE(storage_file_path), "%s\\%s\\%s", appdata, STORAGE_DIR, STORAGE_FILE);
	hFile = CreateFileA(storage_file_path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		DWORD err = GetLastError();
		if (err == ERROR_FILE_NOT_FOUND) {
			/* Create file with defaults */
			HANDLE hCreate = CreateFileA(storage_file_path, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
			if (hCreate == INVALID_HANDLE_VALUE) {
				return -1; /* could not create file */
			}

			CHAR outBuf[1024];
			HRESULT hr = StringCchPrintfA(outBuf, ARRAYSIZE(outBuf), "%d\n%s\n%s\n%s\n%s\n\n", last_announcement, DEF_HOST_BASE, DEF_USR, DEF_PWD, DEF_DIR);
			if (FAILED(hr)) {
				CloseHandle(hCreate);
				return -1;
			}

			DWORD written = 0;
			if (!WriteFile(hCreate, outBuf, (DWORD)strlen(outBuf), &written, NULL)) {
				CloseHandle(hCreate);
				return -1;
			}

			CloseHandle(hCreate);

			/* Initialize globals with defaults */
			last_announcement = 0;
			StringCchCopyA(host_base, ARRAYSIZE(host_base), INTERNAL_DEF_HOST_BASE);
			StringCchCopyA(usr, ARRAYSIZE(usr), DEF_USR);
			StringCchCopyA(pwd, ARRAYSIZE(pwd), DEF_PWD);
			StringCchCopyA(dir, ARRAYSIZE(dir), DEF_DIR);
			/* Actives defaults: no actives */
			actives_count = 0;
			actives_exist = FALSE;
			for (INT _i = 0; _i < MAX_STORED_ACTIVES; ++_i) actives[_i][0] = '\0';
			CHAR expanded_dir_tmp_new[MAX_DIR_SIZE + 1] = { 0 };
			if (ExpandEnvironmentStringsA(dir, expanded_dir_tmp_new, ARRAYSIZE(expanded_dir_tmp_new)) > 0) {
				strcpy_s(dir, ARRAYSIZE(dir), expanded_dir_tmp_new);
			}
			return 0;
		}
		return -1; /* other error opening file */
	}

	/* Get file size */
	if (!GetFileSizeEx(hFile, &fileSize)) {
		CloseHandle(hFile);
		return -1;
	}

	if (fileSize.QuadPart == 0 || fileSize.QuadPart > 65536) {
		CloseHandle(hFile);
		return -1;
	}

	toRead = (SIZE_T)fileSize.QuadPart;
	/* Allocate buffer on the heap using Win32 HeapAlloc */
	NPSTR buffer = (NPSTR)HeapAlloc(PROCESS_HEAP, 0, toRead + 1);
	if (!buffer) { CloseHandle(hFile); return -1; }

	if (!ReadFile(hFile, buffer, (DWORD)toRead, &dwRead, NULL) || dwRead != (DWORD)toRead) {
		HeapFree(PROCESS_HEAP, 0, buffer);
		CloseHandle(hFile);
		return -1;
	}
	buffer[dwRead] = '\0';

	CloseHandle(hFile);

	/* Parse six lines: last_announcement, host_base, usr, pwd, dir, actives */
	NPSTR cursor = buffer;
	for (INT i = 0; i < 6; ++i) {
		NPSTR newline = (NPSTR)strchr(cursor, '\n');
		SIZE_T tokenLen = 0;
		if (newline) tokenLen = (SIZE_T)(newline - cursor);
		else tokenLen = strlen(cursor);

		/* Trim trailing CR */
		while (tokenLen > 0 && cursor[tokenLen - 1] == '\r') tokenLen--;

		if (i == 0) {
			/* parse integer */
			if (tokenLen == 0) { HeapFree(PROCESS_HEAP, 0, buffer); return -1; }
			CHAR tmp[64];
			if (tokenLen >= ARRAYSIZE(tmp)) { HeapFree(PROCESS_HEAP, 0, buffer); return -1; }
			memcpy(tmp, cursor, tokenLen);
			tmp[tokenLen] = '\0';

			NPSTR endptr = NULL;
			long val = strtol(tmp, &endptr, 10);
			if (endptr == tmp) { HeapFree(PROCESS_HEAP, 0, buffer); return -1; }
			/* ensure any remaining chars are whitespace */
			while (*endptr != '\0') {
				if (*endptr != ' ' && *endptr != '\t') { HeapFree(PROCESS_HEAP, 0, buffer); return -1; }
				++endptr;
			}
			last_announcement = (INT)val;
		}
		else if (i == 1) {
			/* host_base */
			if (tokenLen == 0 || tokenLen >= ARRAYSIZE(host_base)) { HeapFree(PROCESS_HEAP, 0, buffer); return -1; }
			/* copy and null-terminate */
			if (!strncmp(cursor, DEF_HOST_BASE, strlen(DEF_HOST_BASE))) {
				strcpy_s(host_base, (SIZE_T)ARRAYSIZE(host_base), INTERNAL_DEF_HOST_BASE);
			}
			else {
				memcpy(host_base, cursor, tokenLen);
				host_base[tokenLen] = '\0';
			}
		}
		else if (i == 2) {
			if (tokenLen == 0 || tokenLen >= ARRAYSIZE(usr)) { HeapFree(PROCESS_HEAP, 0, buffer); return -1; }
			memcpy(usr, cursor, tokenLen);
			usr[tokenLen] = '\0';
		}
		else if (i == 3) {
			if (tokenLen == 0 || tokenLen >= ARRAYSIZE(pwd)) { HeapFree(PROCESS_HEAP, 0, buffer); return -1; }
			memcpy(pwd, cursor, tokenLen);
			pwd[tokenLen] = '\0';
		}
		else if (i == 4) {
			if (tokenLen == 0 || tokenLen >= ARRAYSIZE(dir)) { HeapFree(PROCESS_HEAP, 0, buffer); return -1; }
			memcpy(dir, cursor, tokenLen);
			dir[tokenLen] = '\0';

		}
		else if (i == 5) {
			/* actives: comma-delimited list, empty means none */
			if (tokenLen == 0) {
				/* no actives */
				actives_count = 0;
				actives_exist = FALSE;
				/* clear array */
				for (INT _i = 0; _i < MAX_STORED_ACTIVES; ++_i) actives[_i][0] = '\0';
			}
			else {
				if (tokenLen >= 1024) { HeapFree(PROCESS_HEAP, 0, buffer); return -1; }
				CHAR tmpAct[1024];
				memcpy(tmpAct, cursor, tokenLen);
				tmpAct[tokenLen] = '\0';
				/* split by comma */
				CHAR* ctx = NULL;
				CHAR* tok = strtok_s(tmpAct, ",", &ctx);
				INT ac = 0;
				while (tok) {
					SIZE_T tlen = strlen(tok);
					if (tlen == 0 || tlen > MAX_SECTORNAME_SIZE) { HeapFree(PROCESS_HEAP, 0, buffer); return -1; }
					if (ac >= MAX_STORED_ACTIVES) { HeapFree(PROCESS_HEAP, 0, buffer); return -1; }
					/* copy token */
					StringCchCopyA(actives[ac], MAX_SECTORNAME_SIZE + 1, tok);
					ac++;
					tok = strtok_s(NULL, ",", &ctx);
				}
				actives_count = ac;
				actives_exist = (ac > 0) ? TRUE : FALSE;
			}
		}
		if (newline) cursor = newline + 1;
		else cursor += tokenLen; /* no newline, we're at EOF */
	}

	HeapFree(PROCESS_HEAP, 0, buffer);
	CHAR expanded_dir_tmp[MAX_DIR_SIZE + 1] = { 0 };
	if (ExpandEnvironmentStringsA(dir, expanded_dir_tmp, ARRAYSIZE(expanded_dir_tmp)) > 0) {
		strcpy_s(dir, ARRAYSIZE(dir), expanded_dir_tmp);
	}

	return 1;
}

/* Replace the given 1-based line number in storage_file with newText.
   Returns TRUE on success, FALSE on failure. newText should not contain '\n'. */
static BOOL replace_storage_line(INT line_number, LPCSTR newText)
{
	if (line_number < 1 || !newText) return FALSE;

	HANDLE hFile = CreateFileA(storage_file_path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) return FALSE;

	LARGE_INTEGER fileSize = { 0 };
	if (!GetFileSizeEx(hFile, &fileSize)) { CloseHandle(hFile); return FALSE; }
	if (fileSize.QuadPart < 0 || fileSize.QuadPart > 10 * 1024 * 1024) { CloseHandle(hFile); return FALSE; }

	DWORD toRead = (DWORD)fileSize.QuadPart;
	LPSTR buffer = (LPSTR)HeapAlloc(PROCESS_HEAP, 0, (SIZE_T)toRead + 1);
	if (!buffer) { CloseHandle(hFile); return FALSE; }

	DWORD read = 0;
	if (!ReadFile(hFile, buffer, toRead, &read, NULL)) { HeapFree(PROCESS_HEAP, 0, buffer); CloseHandle(hFile); return FALSE; }
	buffer[read] = '\0';
	CloseHandle(hFile);

	/* Find the requested line */
	LPSTR ptr = buffer;
	LPSTR lineStart = NULL;
	LPSTR lineEnd = NULL;
	INT current = 1;

	while (current <= line_number && *ptr != '\0') {
		lineStart = ptr;
		/* find end of line (newline or end) */
		LPSTR nl = (LPSTR)strchr(ptr, '\n');
		if (nl) {
			lineEnd = nl; /* points to '\n' */
			ptr = nl + 1;
		}
		else {
			lineEnd = ptr + strlen(ptr);
			ptr = lineEnd;
		}
		if (current == line_number) break;
		++current;
	}

	if (current != line_number || !lineStart || !lineEnd) { HeapFree(PROCESS_HEAP, 0, buffer); return FALSE; }

	/* compute old line length excluding trailing CR */
	SIZE_T oldLen = (SIZE_T)(lineEnd - lineStart);
	while (oldLen > 0 && lineStart[oldLen - 1] == '\r') --oldLen;

	/* prepare new text (strip any trailing CR/LF) */
	SIZE_T newLen = strlen(newText);
	while (newLen > 0 && (newText[newLen - 1] == '\n' || newText[newLen - 1] == '\r')) --newLen;

	/* compute prefix and suffix sizes */
	SIZE_T prefixSize = (SIZE_T)(lineStart - buffer);
	LPSTR suffixPtr = lineEnd;
	BOOL hadNewline = FALSE;
	BOOL hadCR = FALSE;
	if (suffixPtr < buffer + read && *suffixPtr == '\n') {
		hadNewline = TRUE;
		/* check if there was a CR before the LF */
		if (lineEnd > lineStart && lineEnd[-1] == '\r') hadCR = TRUE;
		suffixPtr = suffixPtr + 1; /* move past newline */
	}
	SIZE_T suffixSize = (SIZE_T)((buffer + read) - suffixPtr);

	/* build new buffer */
	SIZE_T extraSep = 0;
	if (hadNewline) extraSep = hadCR ? 2 : 1; /* preserve CRLF or LF */
	SIZE_T newFileSize = prefixSize + newLen + extraSep + suffixSize;
	LPSTR newBuf = (LPSTR)HeapAlloc(PROCESS_HEAP, 0, newFileSize + 1);
	if (!newBuf) { HeapFree(PROCESS_HEAP, 0, buffer); return FALSE; }

	/* copy prefix */
	if (prefixSize) memcpy(newBuf, buffer, prefixSize);
	/* copy new text */
	if (newLen) memcpy(newBuf + prefixSize, newText, newLen);
	/* insert original separator if present */
	if (extraSep) {
		if (hadCR) {
			newBuf[prefixSize + newLen] = '\r';
			newBuf[prefixSize + newLen + 1] = '\n';
		}
		else {
			newBuf[prefixSize + newLen] = '\n';
		}
	}
	/* copy suffix */
	if (suffixSize) memcpy(newBuf + prefixSize + newLen + extraSep, suffixPtr, suffixSize);
	newBuf[newFileSize] = '\0';

	/* write back to file (overwrite) */
	HANDLE hOut = CreateFileA(storage_file_path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hOut == INVALID_HANDLE_VALUE) { HeapFree(PROCESS_HEAP, 0, newBuf); HeapFree(PROCESS_HEAP, 0, buffer); return FALSE; }

	DWORD written = 0;
	BOOL ok = WriteFile(hOut, newBuf, (DWORD)newFileSize, &written, NULL) && written == (DWORD)newFileSize;
	CloseHandle(hOut);

	HeapFree(PROCESS_HEAP, 0, newBuf);
	HeapFree(PROCESS_HEAP, 0, buffer);

	return ok ? TRUE : FALSE;
}

/* helper: write actives into storage file (line 6) */
BOOL write_actives_to_storage(LPCSTR items[], INT count, BOOL no_actives)
{
	if (no_actives) {
		/* clear globals */
		actives_count = 0;
		actives_exist = FALSE;
		for (INT i = 0; i < MAX_STORED_ACTIVES; ++i) actives[i][0] = '\0';
		return replace_storage_line(STORAGE__ACTIVES, "");
	}

	if (!items || count < 0 || count > MAX_STORED_ACTIVES) return FALSE;

	CHAR outBuf[HISTORY_MAX_SECTORLIST_SIZE];
	outBuf[0] = '\0';

	for (INT i = 0; i < count; ++i) {
		if (!items[i]) return FALSE;
		SIZE_T len = strlen(items[i]);
		if (len == 0 || len > MAX_SECTORNAME_SIZE) return FALSE;
		if (strchr(items[i], ',')) return FALSE; /* disallow commas in items */
		if (i > 0) {
			if (FAILED(StringCchCatA(outBuf, ARRAYSIZE(outBuf), ","))) return FALSE;
		}
		if (FAILED(StringCchCatA(outBuf, ARRAYSIZE(outBuf), items[i]))) return FALSE;
	}

	BOOL ok = replace_storage_line(STORAGE__ACTIVES, outBuf);
	if (ok) {
		/* update globals */
		actives_count = count;
		actives_exist = (count > 0) ? TRUE : FALSE;
		for (INT i = 0; i < MAX_STORED_ACTIVES; ++i) {
			if (i < count) StringCchCopyA(actives[i], MAX_SECTORNAME_SIZE + 1, items[i]);
			else actives[i][0] = '\0';
		}
	}
	return ok;
}

BOOL check_for_announcements(BOOL notify_if_no_announcement) {

	CHAR buf[192];
	sprintf_s(buf, ARRAYSIZE(buf), "%s/announce/?usr=%s&pwd=%s&ver=%s&client_last=%d", host_base, usr, pwd, PROGRAM_VER, last_announcement);
	http_response_t response = send_http_get((LPCSTR)buf, DEFAULT_HTTP_TIMEOUT);
	if (response.errored) {
		if (notify_if_no_announcement)MessageBoxA(main_wnd, "Client request to check for announcements failed to connect to server's handler.", "Announcement Request Error", MB_OK);
		return -1;
	}
	if (response.serverok) { //Got an announcement
		CHAR n_announcements[9];
		CHAR time_buf[65];
		CHAR author[MAX_USRPWD_SIZE + 1];
		CHAR data[65];
		/* Parse four lines while preserving spaces: use scanset %[^
] with size args for scanf_s */
		INT rc = sscanf_s(response.response,
			"%8[^\n]\n%64[^\n]\n%16[^\n]\n%64[^\n]",
			n_announcements, (UINT)ARRAYSIZE(n_announcements),
			time_buf, (UINT)ARRAYSIZE(time_buf),
			author, (UINT)ARRAYSIZE(author),
			data, (UINT)ARRAYSIZE(data));
		if (rc != 4) {
			free(response.response);
			MessageBoxA(main_wnd, "Failed to scan all components of the received announcement.", "Announcement Request Error", MB_OK | MB_ICONWARNING);
			return -1;
		}
		PCHAR endptr;
		errno = 0;
		LONG id_received_announcement = strtol(n_announcements, &endptr, 10);
		if (endptr == n_announcements || *endptr != '\0' || errno == ERANGE) {
			MessageBoxA(main_wnd, "The received announcement's ID could not be validated to be an integer.", "Fatal Announcement Request Error", MB_ICONERROR | MB_OK);
			exit(EXIT_FAILURE);
		}
		CHAR msg_buf[256];
		CHAR caption_buf[64];
		sprintf_s(msg_buf, ARRAYSIZE(msg_buf), "\"%s\"\n\nID #%s\nPOSTED %s\nPOSTER \"%s\"", data, n_announcements, time_buf, author);
		sprintf_s(caption_buf, ARRAYSIZE(caption_buf), "Received a %zu-byte Announcement", response.response_len);
		MessageBoxA(main_wnd, msg_buf, caption_buf, MB_OK | MB_ICONINFORMATION);
		last_announcement = id_received_announcement;
		replace_storage_line(STORAGE__CLIENT_ANNOUNCEMENT, (LPCSTR)n_announcements);
		free(response.response);
		return 1;
	}
	else if (!response.response_len && notify_if_no_announcement) {
		MessageBoxA(main_wnd, "No new announcements at this time.", "You're Caught Up.", MB_OK | MB_ICONINFORMATION);
		FREE_IF_NONNULL(response.response);
		return 0;
	}
	else if (notify_if_no_announcement) { //Error?
		MessageBoxA(main_wnd, response.response, "Server-side Announcement Request Rejection", MB_OK | MB_ICONERROR);
		FREE_IF_NONNULL(response.response);
	}
	return -1;
}

//MUST BE IN THE SAME FORMAT AS WHAT IS RECEIVED FROM THE SERVER!
struct listed_sector_lock {
	LPSTR sector;
	LPSTR author;
	LPSTR timestamp;
	LPSTR reason;
};

BOOL update_sector_locks_listbox(BOOL notify, HWND listbox) {
	//Clear old items
	INT n_items = (INT)SendMessageA(listbox, LB_GETCOUNT, (WPARAM)NULL, (LPARAM)NULL);
	struct listed_sector_lock* data;
	LRESULT res = (LRESULT)LB_ERR;
	if (n_items && n_items != LB_ERR) while (LB_ERR != (res = SendMessage(listbox, LB_GETITEMDATA, (WPARAM)0, (LPARAM)NULL))) {
		data = (struct listed_sector_lock*)res;
		HeapFree(PROCESS_HEAP, 0x0, data->timestamp);
		HeapFree(PROCESS_HEAP, 0x0, data->reason);
		HeapFree(PROCESS_HEAP, 0x0, data->author);
		HeapFree(PROCESS_HEAP, 0x0, data->sector);
		HeapFree(PROCESS_HEAP, 0x0, data);
		SendMessage(listbox, LB_DELETESTRING, (WPARAM)0, (LPARAM)NULL);
	}
	CHAR url_buf[MAX_HOSTNAME_SIZE + 96];
	sprintf_s(url_buf, ARRAYSIZE(url_buf), "%s/locker/?usr=%s&pwd=%s&ver=%s", host_base, usr, pwd, PROGRAM_VER);
	http_response_t resp = send_http_get((LPCSTR)url_buf, DEFAULT_HTTP_TIMEOUT);
	if (resp.errored) {
		if (notify) MessageBoxA(main_wnd, "Failed to update sector locks:\nRequest for locks failed to reach the server's handler.", "Sector Lock Retrieval Failure", MB_OK | MB_ICONERROR);
		return -1;
	}
	if (!resp.serverok) {
		if (notify) MessageBoxA(main_wnd, resp.response, "Server Rejected Server Lock Retreival", MB_OK | MB_ICONERROR);
		return FALSE;
	}
	LPSTR line_context = NULL;
	LPSTR line = strtok_s(resp.response, "\n", &line_context);
	// Skip the first line (n_sectors count)
	UINT n_locks;
	if (line) {
		INT rc = sscanf_s(resp.response, "%u", &n_locks);
		if (!rc || n_locks > MAX_SECTOR_LOCKS) goto LOCK_RETRIEVAL_SERVER_MISFORMAT_ERR;
		line = strtok_s(NULL, "\n", &line_context);
	}
	else {
	LOCK_RETRIEVAL_SERVER_MISFORMAT_ERR:
		MessageBoxA(main_wnd, "Server response is misformatted! Aborting to maintain program safety.", "Sector Lock Retrieval Failure", MB_OK | MB_ICONERROR);
		exit(EXIT_FAILURE);
	};
	LPSTR** locks = (LPSTR**)HeapAlloc(PROCESS_HEAP, 0x0, n_locks * sizeof(LPSTR*)); //[ ["sector_name1", "usr1", "time1", "reason1"]
	// Parse each line: sector_name/author/time_str/reason
	UINT lock_idx = 0;
	//for (lock_idx = 0; lock_idx < n_locks; lock_idx++) {
	while (lock_idx < n_locks) {
		locks[lock_idx] = (LPSTR*)HeapAlloc(PROCESS_HEAP, 0x0, FIELDS_PER_LOCK_LINE * sizeof(LPSTR));
		locks[lock_idx][0] = (LPSTR)HeapAlloc(PROCESS_HEAP, 0x0, sizeof(CHAR) * (MAX_SECTORNAME_SIZE + 1));
		locks[lock_idx][1] = (LPSTR)HeapAlloc(PROCESS_HEAP, 0x0, sizeof(CHAR) * (MAX_USRPWD_SIZE + 1));
		locks[lock_idx][2] = (LPSTR)HeapAlloc(PROCESS_HEAP, 0x0, sizeof(CHAR) * (MAX_TIMESTAMP_LEN + 1));
		locks[lock_idx][3] = (LPSTR)HeapAlloc(PROCESS_HEAP, 0x0, sizeof(CHAR) * (MAX_LOCK_REASON_LEN + 1));
		INT rc = sscanf_s(line, "%[^/]/%[^/]/%[^/]/%[^/]",
			locks[lock_idx][0], (unsigned)(MAX_SECTORNAME_SIZE + 1),
			locks[lock_idx][1], (unsigned)(MAX_USRPWD_SIZE + 1),
			locks[lock_idx][2], (unsigned)(MAX_TIMESTAMP_LEN + 1),
			locks[lock_idx][3], (unsigned)(MAX_LOCK_REASON_LEN + 1)
		);
		if (rc != FIELDS_PER_LOCK_LINE) goto LOCK_RETRIEVAL_SERVER_MISFORMAT_ERR;
		if (lock_idx++ < n_locks - 1)
			if (NULL == (line = strtok_s(NULL, "\n", &line_context))) goto LOCK_RETRIEVAL_SERVER_MISFORMAT_ERR;
	}
	CHAR buf[MAX_SECTORNAME_SIZE + MAX_USRPWD_SIZE + 8];
	for (lock_idx = 0; lock_idx < n_locks; lock_idx++) {
		data = (struct listed_sector_lock*)HeapAlloc(PROCESS_HEAP, 0x0, sizeof(struct listed_sector_lock));
		data->sector = locks[lock_idx][0];
		data->author = locks[lock_idx][1];
		data->timestamp = locks[lock_idx][2];
		data->reason = locks[lock_idx][3];
		//*data = *(struct listed_sector_lock*)locks[lock_idx];
		sprintf_s(buf, ARRAYSIZE(buf), "%s: %s", locks[lock_idx][0], locks[lock_idx][1]);
		INT list_idx = (INT)SendMessageA(listbox, LB_ADDSTRING, (LPARAM)NULL, (WPARAM)buf);
		SendMessageA(listbox, LB_SETITEMDATA, (WPARAM)list_idx, (LPARAM)data);
		//each item is not freed bc they are placed into the listbox. They are freed on listbox update (see start of this fn) or on program close.
		HeapFree(PROCESS_HEAP, 0x0, (LPVOID)locks[lock_idx]);
	}
	HeapFree(PROCESS_HEAP, 0x0, (LPVOID)locks);
	FREE_IF_NONNULL(resp.response);
	if (notify) MessageBoxA(main_wnd, "Sector locks on the main window have been successfully updated.", "Sector Lock Retrieval Success", MB_OK | MB_ICONASTERISK);
	return TRUE;
}

enum lock_sector_actions { LOCK_SECTOR_ACTION_LOCK, LOCK_SECTOR_ACTION_UNLOCK };

//TRUE == success, FALSE == Rejection, FAILED == Unable to reach/argument failure
BOOL lock_sector(LPSTR reason /*Only read if action == LOCK_SECTOR_ACTION_LOCK*/, enum lock_sector_actions action, LPSTR sector_name) {
	CHAR url_buf[MAX_HOSTNAME_SIZE + MAX_LOCK_REASON_LEN + 2 * MAX_USRPWD_SIZE + 96];
	CHAR action_name[16];
	if (action == LOCK_SECTOR_ACTION_LOCK) strcpy_s(action_name, ARRAYSIZE(action_name), "lock");
	else if (action == LOCK_SECTOR_ACTION_UNLOCK) strcpy_s(action_name, ARRAYSIZE(action_name), "unlock");
	else {
		fprintf(stderr, "lock_sector(action = %d): Unknown action!", action);
		return -1;
	}
	sprintf_s(url_buf, ARRAYSIZE(url_buf), "%s/locker/?usr=%s&pwd=%s&ver=%s&sector=%s&action=%s", host_base, usr, pwd, PROGRAM_VER, sector_name, action_name);
	http_response_t resp;
	if (action == LOCK_SECTOR_ACTION_LOCK) resp = send_http_post((LPCSTR)url_buf, (LPCSTR)reason, strlen((LPCSTR)reason), (LPCSTR)NULL, DEFAULT_HTTP_TIMEOUT);
	else if (action == LOCK_SECTOR_ACTION_UNLOCK) resp = send_http_post((LPCSTR)url_buf, (LPCSTR)NULL, 0, (LPCSTR)NULL, DEFAULT_HTTP_TIMEOUT);
	if (resp.errored) {
		MessageBoxA(main_wnd, "Lock request failed to reach the server's handler.", "Sector Locker Error", MB_OK | MB_ICONERROR);
		return -1;
	}
	if (!resp.serverok) {
		MessageBoxA(main_wnd, (LPCSTR)resp.response, "Sector Locker's Request was Rejected", MB_OK | MB_ICONERROR);
		FREE_IF_NONNULL(resp.response);
		return FALSE;
	}
	FREE_IF_NONNULL(resp.response);
	return TRUE;
}
#endif //!defined(ATSS_STORE_ANNOUNCE_H)