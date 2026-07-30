#pragma once
#ifndef ATSS_GET_PLACE_H
#define ATSS_GET_PLACE_H 1
#define MAX_PLACEABLE_FILE_SIZE (8 * 1000 * 1000)
#define MAX_PLACEMENT_REASON_LEN 64
#define MAX_RETRIEVED_FILE_SIZE MAX_PLACEABLE_FILE_SIZE
#define SERVER_PLACEMENT_SEPARATOR "*" //MUST BE 1-LONG STR ONLY for pointer math in place_onto_server
//FALSE == failed, TRUE == success
//If no placement reason, set reason = NULL.
BOOL place_onto_server(LPSTR sector_name, LPSTR reason) {
	CONST MAX_REASON_LEN = 64;
	//b64-encode all files to get the message body
	CHAR active_sector_part[MAX_DIR_SIZE + 1 /*\*/ + MAX_SECTORNAME_SIZE + 1/*.*/ + MAX_SECTORPART_EXT_LENGTH + 1/*\0*/] = { 0 };
	HANDLE hPart;
	DWORD file_lengths[FILES_PER_SECTOR] = { 0 };
	LPSTR b64_files[FILES_PER_SECTOR];
	for (UINT i = 0; i < FILES_PER_SECTOR; i++) {
		sprintf_s(active_sector_part, ARRAYSIZE(active_sector_part), "%s\\%s.%s", dir, sector_name, SECTOR_PART_ORDER[i]);
		hPart = CreateFileA(active_sector_part, GENERIC_READ, 0x0, (LPSECURITY_ATTRIBUTES)NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, (HANDLE)NULL);
		if (hPart == INVALID_HANDLE_VALUE) {
			MessageBoxA(main_wnd, "Failed to open a sector part for reading.", "Client-side Server Placement Error", MB_OK | MB_ICONERROR);
			return FALSE;
		}
		LARGE_INTEGER size;
		GetFileSizeEx(hPart, &size);
		if (size.QuadPart > MAX_PLACEABLE_FILE_SIZE) {
			MessageBoxA(main_wnd, "A file is too big to place onto the server", "Client-side Server Placement Error", MB_OK | MB_ICONERROR);
			CloseHandle(hPart);
			return FALSE;
		}
		if (size.QuadPart == 0 || size.HighPart > 0) {
			MessageBoxA(main_wnd, "A file is either empty or too big to place onto the server.", "Client-side Server Placement Error", MB_OK | MB_ICONERROR);
			CloseHandle(hPart);
			return FALSE;
		}
		LPBYTE data = (LPBYTE)HeapAlloc(PROCESS_HEAP, 0x0, size.QuadPart);
		if (data == NULL) {
			MessageBoxA(main_wnd, "Failed to allocate memory!!", "Client-side Server Placement Error", MB_OK | MB_ICONERROR);
			CloseHandle(hPart);
			exit(EXIT_FAILURE);
		}
		DWORD read_bytes = 0;
		BOOL success = ReadFile(hPart, data, size.LowPart, &read_bytes, (LPOVERLAPPED)NULL);
		CloseHandle(hPart);
		if (!success || read_bytes != size.LowPart) {
			MessageBoxA(main_wnd, "file reading byte mismatch or error", "Client-side Server Placement Error", MB_OK | MB_ICONERROR);
			HeapFree(PROCESS_HEAP, 0x0, data);
			exit(EXIT_FAILURE);
		}
		if (!CryptBinaryToStringA((LPCBYTE)data, read_bytes, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, (LPSTR)NULL, &file_lengths[i])) {
		PLACEMENT_B64_ENCODING_ERR:
			MessageBoxA(main_wnd, "Failed to base64-encode a sector part.", "Client-side Server Placement Error", MB_OK | MB_ICONERROR);
			HeapFree(PROCESS_HEAP, 0x0, data);
			return -1;
		}
		b64_files[i] = (LPSTR)HeapAlloc(PROCESS_HEAP, 0x0, file_lengths[i]);
		if (!CryptBinaryToStringA((LPCBYTE)data, read_bytes, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, b64_files[i], &file_lengths[i])) {
			for (UINT j = 0; j <= i; j++) HeapFree(PROCESS_HEAP, 0x0, b64_files[j]);
			goto PLACEMENT_B64_ENCODING_ERR;
		}
		HeapFree(PROCESS_HEAP, 0x0, data);
	}
	DWORD len = (FILES_PER_SECTOR * sizeof(CHAR)); /*accounts for separators*/
	for (UINT i = 0; i < FILES_PER_SECTOR; i++) len += file_lengths[i];
	LPSTR body = HeapAlloc(PROCESS_HEAP, 0x0, len);
	LPCHAR ptr = (LPCHAR)body;
	for (UINT i = 0; i < FILES_PER_SECTOR; i++) {
		ptr += sprintf_s((LPSTR)ptr, (body + (len * sizeof(CHAR))) - ptr,
			(i == FILES_PER_SECTOR - 1) ? "%s" : "%s" SERVER_PLACEMENT_SEPARATOR,
			b64_files[i]);
		HeapFree(PROCESS_HEAP, 0x0, b64_files[i]);
	}
	CHAR url_buf[MAX_HOSTNAME_SIZE + MAX_PLACEMENT_REASON_LEN + 96];
	if (reason == (LPSTR)NULL) {
		sprintf_s(url_buf, ARRAYSIZE(url_buf), "%s/place/?usr=%s&pwd=%s&ver=%s&sector=%s", host_base, usr, pwd, PROGRAM_VER, sector_name);
	}
	else sprintf_s(url_buf, ARRAYSIZE(url_buf), "%s/place/?usr=%s&pwd=%s&ver=%s&sector=%s&reason=%s", host_base, usr, pwd, PROGRAM_VER, sector_name, reason);
	http_response_t resp = send_http_post(url_buf, body, --len /*\0*/, "text/plain", DEFAULT_HTTP_TIMEOUT);
	HeapFree(PROCESS_HEAP, 0x0, body);
	if (resp.errored) {
		MessageBoxA(main_wnd, "Client request failed to reach the server's placement handler", "Server Placement Error", MB_OK | MB_ICONERROR);
		return -1;
	}
	if (!resp.serverok) {
		MessageBoxA(main_wnd, resp.response, "Server Rejected Client's Request to Place Sector", MB_OK | MB_ICONERROR);
		FREE_IF_NONNULL(resp.response);
		return FALSE;
	}
	FREE_IF_NONNULL(resp.response);
	//CHAR buf[128];
	//sprintf_s(buf, ARRAYSIZE(buf), "The client's request to place sector %s onto the server succeeded.", sector_name);
	//MessageBoxA(main_wnd, buf, "Successful Server Placement", MB_OK | MB_ICONASTERISK);
	return TRUE;
}

BOOL retrieve_from_server(LPSTR sector_name) {
	CHAR url_buf[MAX_HOSTNAME_SIZE + 64];
	sprintf_s(url_buf, ARRAYSIZE(url_buf), "%s/get/?usr=%s&pwd=%s&ver=%s&sector=%s", host_base, usr, pwd, PROGRAM_VER, sector_name);
	http_response_t resp = send_http_get(url_buf, DEFAULT_HTTP_TIMEOUT);
	if (resp.errored) {
		MessageBoxA(main_wnd, "Client request to get sector failed to reach the server's placement handler", "Server Placement Error", MB_OK | MB_ICONERROR);
		return -1;
	}
	if (!resp.serverok) {
		MessageBoxA(main_wnd, resp.response, "Server Rejected Client's Request to Get Sector", MB_OK | MB_ICONERROR);
		FREE_IF_NONNULL(resp.response);
		return FALSE;
	}
	// resp.response should contain FILES_PER_SECTOR base64 chunks separated by SERVER_PLACEMENT_SEPARATOR
	LPSTR body = resp.response;
	if (body == NULL) {
		MessageBoxA(main_wnd, "Server returned an empty response for sector retrieval.", "Server Placement Error", MB_OK | MB_ICONERROR);
		FREE_IF_NONNULL(resp.response);
		return FALSE;
	}

	LPSTR parts[FILES_PER_SECTOR];
	SIZE_T part_lens[FILES_PER_SECTOR];
	for (UINT i = 0; i < FILES_PER_SECTOR; i++) { parts[i] = NULL; part_lens[i] = 0; }

	LPSTR cur = body;
	SIZE_T sep_len = strlen(SERVER_PLACEMENT_SEPARATOR);
	BOOL bad = FALSE;
	for (UINT i = 0; i < FILES_PER_SECTOR; i++) {
		if (i < FILES_PER_SECTOR - 1) {
			LPSTR sep = strstr(cur, SERVER_PLACEMENT_SEPARATOR);
			if (sep == NULL) { bad = TRUE; break; }
			SIZE_T len = (SIZE_T)(sep - cur);
			parts[i] = (LPSTR)HeapAlloc(PROCESS_HEAP, 0x0, len + 1);
			if (parts[i] == NULL) { bad = TRUE; break; }
			memcpy(parts[i], cur, len);
			parts[i][len] = '\0';
			part_lens[i] = len;
			cur = sep + sep_len;
		}
		else {
			// last part: rest of string
			SIZE_T len = strlen(cur);
			parts[i] = (LPSTR)HeapAlloc(PROCESS_HEAP, 0x0, len + 1);
			if (parts[i] == NULL) { bad = TRUE; break; }
			memcpy(parts[i], cur, len);
			parts[i][len] = '\0';
			part_lens[i] = len;
		}
	}
	if (bad) {
		MessageBoxA(main_wnd, "Server response did not contain the expected number of files for this sector.", "Server Placement Error", MB_OK | MB_ICONERROR);
		for (UINT i = 0; i < FILES_PER_SECTOR; i++) if (parts[i]) HeapFree(PROCESS_HEAP, 0x0, parts[i]);
		FREE_IF_NONNULL(resp.response);
		return FALSE;
	}

	// Ensure there are no extra separators embedded (sanity)
	if (strstr(parts[FILES_PER_SECTOR - 1], SERVER_PLACEMENT_SEPARATOR) != NULL) {
		MessageBoxA(main_wnd, "Server response contains too many file chunks.", "Server Placement Error", MB_OK | MB_ICONERROR);
		for (UINT i = 0; i < FILES_PER_SECTOR; i++) if (parts[i]) HeapFree(PROCESS_HEAP, 0x0, parts[i]);
		FREE_IF_NONNULL(resp.response);
		return FALSE;
	}

	// Decode all parts first
	LPBYTE decoded[FILES_PER_SECTOR];
	DWORD decoded_lens[FILES_PER_SECTOR];
	for (UINT i = 0; i < FILES_PER_SECTOR; i++) { decoded[i] = NULL; decoded_lens[i] = 0; }

	for (UINT i = 0; i < FILES_PER_SECTOR; i++) {
		DWORD need = 0;
		// Let the API compute the input string length by passing cchString = 0.
		// This avoids false 'invalid base64' errors when part_lens may not account for padding.
		if (!CryptStringToBinaryA((LPCSTR)parts[i], 0, CRYPT_STRING_BASE64, (BYTE*)NULL, &need, NULL, NULL)) {
			fprintf(stderr, "%s invalid\n", parts[i]);
			MessageBoxA(main_wnd, "One of the files received from the server was not valid base64.", "Server Placement Error", MB_OK | MB_ICONERROR);
			bad = TRUE;
			break;
		}
		if (need > MAX_RETRIEVED_FILE_SIZE) {
			MessageBoxA(main_wnd, "Server sent a file that exceeds the maximum allowed size for retrieval.", "Server Placement Error", MB_OK | MB_ICONERROR);
			bad = TRUE;
			break;
		}
		decoded[i] = (LPBYTE)HeapAlloc(PROCESS_HEAP, 0x0, need);
		if (decoded[i] == NULL) { MessageBoxA(main_wnd, "Failed to allocate memory to decode server file.", "Server Placement Error", MB_OK | MB_ICONERROR); bad = TRUE; break; }
		decoded_lens[i] = need;
		if (!CryptStringToBinaryA((LPCSTR)parts[i], 0, CRYPT_STRING_BASE64, decoded[i], &need, NULL, NULL)) {
			MessageBoxA(main_wnd, "Failed to decode a base64 file from the server.", "Server Placement Error", MB_OK | MB_ICONERROR);
			bad = TRUE;
			break;
		}
		// Adjust decoded length to actual bytes written
		decoded_lens[i] = need;
	}

	// free the base64 parts strings
	for (UINT i = 0; i < FILES_PER_SECTOR; i++) if (parts[i]) { HeapFree(PROCESS_HEAP, 0x0, parts[i]); parts[i] = NULL; }

	if (bad) {
		for (UINT i = 0; i < FILES_PER_SECTOR; i++) if (decoded[i]) HeapFree(PROCESS_HEAP, 0x0, decoded[i]);
		FREE_IF_NONNULL(resp.response);
		return FALSE;
	}

	// Write decoded data into sector part files. If any write fails, delete any written files and error out.
	CHAR active_sector_part[MAX_DIR_SIZE + 1 /*\*/ + MAX_SECTORNAME_SIZE + 1/*.*/ + MAX_SECTORPART_EXT_LENGTH + 1/*\0*/] = { 0 };
	HANDLE hPart = INVALID_HANDLE_VALUE;
	BOOL write_failed = FALSE;
	UINT wrote_count = 0;

	for (UINT i = 0; i < FILES_PER_SECTOR; i++) {
		sprintf_s(active_sector_part, ARRAYSIZE(active_sector_part), "%s\\%s.%s", dir, sector_name, SECTOR_PART_ORDER[i]);
		hPart = CreateFileA(active_sector_part, GENERIC_WRITE, 0x0, (LPSECURITY_ATTRIBUTES)NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, (HANDLE)NULL);
		if (hPart == INVALID_HANDLE_VALUE) { write_failed = TRUE; break; }
		DWORD written = 0;
		if (!WriteFile(hPart, decoded[i], decoded_lens[i], &written, (LPOVERLAPPED)NULL) || written != decoded_lens[i]) {
			write_failed = TRUE;
			CloseHandle(hPart);
			break;
		}
		CloseHandle(hPart);
		wrote_count++;
	}

	if (write_failed) {
		// delete any files that were successfully created
		for (UINT j = 0; j < wrote_count; j++) {
			sprintf_s(active_sector_part, ARRAYSIZE(active_sector_part), "%s\\%s.%s", dir, sector_name, SECTOR_PART_ORDER[j]);
			DeleteFileA(active_sector_part);
		}
		MessageBoxA(main_wnd, "Failed to write one or more sector part files to disk.", "Server Placement Error", MB_OK | MB_ICONERROR);
		for (UINT i = 0; i < FILES_PER_SECTOR; i++) if (decoded[i]) HeapFree(PROCESS_HEAP, 0x0, decoded[i]);
		FREE_IF_NONNULL(resp.response);
		return FALSE;
	}

	// cleanup decoded buffers
	for (UINT i = 0; i < FILES_PER_SECTOR; i++) if (decoded[i]) HeapFree(PROCESS_HEAP, 0x0, decoded[i]);

	FREE_IF_NONNULL(resp.response);
	//CHAR buf[128];
	//sprintf_s(buf, ARRAYSIZE(buf), "Successfully retrieved sector %s from server.", sector_name);
	//MessageBoxA(main_wnd, buf, "Successful Sector Retrieval", MB_OK | MB_ICONASTERISK);
	return TRUE;
}
#endif // !defined(ATSS_GET_PLACE_H)