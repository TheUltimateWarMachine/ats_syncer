#pragma once
#ifndef ATSS_SYNCHRONIZE_H
#define ATSS_SYNCHRONIZE_H 1
#include<assert.h>
#include<bcrypt.h>
#include<Shlwapi.h>

CONST NPSTR SECTOR_PART_ORDER[FILES_PER_SECTOR] = { "aux", "base", "data", "desc", "layer" };
#define MAX_SECTORPART_EXT_LENGTH 5 /*layer*/
#define NT_SUCCESS(Status)          (((NTSTATUS)(Status)) >= 0)
#define STATUS_UNSUCCESSFUL         ((NTSTATUS)0xC0000001L)
typedef near BYTE* NPBYTE;
typedef far CHAR* LPCHAR;
typedef struct conflict_mgr_meta {
	LPSTR* sectors; //array (length n_sectors) of sector names
	LPSTR* local_hashes; //array (length n_sectors) of local hashes where item 0 is the sector name
	LPSTR* server_hashes; //array (length n_sectors) of server's hashes. Set to NULL if N/A.
	UINT n_sectors;

	BOOL uploading_allowed; // Is the user allowed to upload a sector from the dialog to the server?
} conflict_mgr_meta;
VOID heapfree_recv_structure(INT n_lvl1, INT n_lvl2, LPSTR** structure);
BOOL file_to_sha1(LPCSTR path, LPSTR out) {
	HANDLE hFile = INVALID_HANDLE_VALUE;
	BCRYPT_ALG_HANDLE hAlg = NULL;
	BCRYPT_HASH_HANDLE hHash = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DWORD cbData = 0, cbHash = 0, cbHashObject = 0;
	NPBYTE pbHashObject = NULL;
	NPBYTE pbHash = NULL;
	hFile = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, (HANDLE)NULL);
	if (hFile == INVALID_HANDLE_VALUE) return FALSE;
	if (FAILED(status = BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_SHA1_ALGORITHM, (LPCWSTR)NULL, BCRYPT_HASH_REUSABLE_FLAG))) { CloseHandle(hFile); return FALSE; }

	// Query the required object and hash sizes
	if (FAILED(status = BCryptGetProperty(hAlg, BCRYPT_OBJECT_LENGTH, (PUCHAR)&cbHashObject, sizeof(cbHashObject), &cbData, 0))) goto FILE_TO_SHA1_CLEANUP;
	if (FAILED(status = BCryptGetProperty(hAlg, BCRYPT_HASH_LENGTH, (PUCHAR)&cbHash, sizeof(cbHash), &cbData, 0))) goto FILE_TO_SHA1_CLEANUP;

	// Allocate buffers
	pbHashObject = (NPBYTE)HeapAlloc(PROCESS_HEAP, 0x0, cbHashObject);
	if (!pbHashObject) { status = STATUS_UNSUCCESSFUL; goto FILE_TO_SHA1_CLEANUP; }
	pbHash = (NPBYTE)HeapAlloc(PROCESS_HEAP, 0x0, cbHash);
	if (!pbHash) { status = STATUS_UNSUCCESSFUL; goto FILE_TO_SHA1_CLEANUP; }

	// Create the hash
	if (FAILED(status = BCryptCreateHash(hAlg, &hHash, pbHashObject, cbHashObject, NULL, 0, 0))) goto FILE_TO_SHA1_CLEANUP;

	// Read file and feed into the hash
	{
		BYTE buffer[4096];
		DWORD bytesRead = 0;
		while (ReadFile(hFile, buffer, sizeof(buffer), &bytesRead, NULL) && bytesRead != FALSE) {
			if (FAILED(status = BCryptHashData(hHash, buffer, bytesRead, 0x0))) goto FILE_TO_SHA1_CLEANUP;
		}
	}
	// Finish the hash
	if (FAILED(status = BCryptFinishHash(hHash, pbHash, cbHash, 0x0))) goto FILE_TO_SHA1_CLEANUP;
	// Convert hash bytes to hexadecimal string (each byte -> two chars)
	{
		static CONST NPSTR hex = "0123456789abcdef";
		DWORD i;
		BYTE b;
		for (i = 0; i < cbHash; ++i) {
			b = pbHash[i];
			out[i * 2] = hex[(b >> 4) & 0xF];
			out[i * 2 + 1] = hex[b & 0xF];
		}
		out[cbHash * 2] = '\0';
	}
	// Success
	status = 0;
FILE_TO_SHA1_CLEANUP:
	if (hHash) BCryptDestroyHash(hHash);
	if (hAlg) BCryptCloseAlgorithmProvider(hAlg, 0x0);
	if (hFile != INVALID_HANDLE_VALUE) CloseHandle(hFile);
	if (pbHashObject) HeapFree(PROCESS_HEAP, 0x0, pbHashObject);
	if (pbHash) HeapFree(PROCESS_HEAP, 0x0, pbHash);
	return NT_SUCCESS(status);
}

INT_PTR CALLBACK ConflictsDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);

BOOL sync_with_server(VOID) {
	CHAR url_buf[MAX_HOSTNAME_SIZE + 64];
	sprintf_s(url_buf, ARRAYSIZE(url_buf), "%s/sync/?usr=%s&pwd=%s&ver=%s", host_base, usr, pwd, PROGRAM_VER);
	http_response_t resp = send_http_get((LPCSTR)url_buf, DEFAULT_HTTP_TIMEOUT);
	if (resp.errored) {
		MessageBoxA(main_wnd, "Client request to connect to server for sector syncing failed to reach the server's handler.", "Sync Failure", MB_ICONERROR | MB_OK);
		return -1;
	}
	if (!resp.serverok) {
		MessageBoxA(main_wnd, resp.response, "Server Rejected Sync Request", MB_ICONERROR | MB_OK);
		FREE_IF_NONNULL(resp.response);
		return -1;
	}
	UINT n_sectors;
	INT scanned = sscanf_s(resp.response, "%u\n", &n_sectors);
	if (!scanned || n_sectors > MAX_ACTIVE_SECTORS) {
	SYNCHRONIZE_SERVER_MISFORMAT_ERR:
		MessageBoxA(main_wnd, "The server's response to a sync request is irrecovably misformatted.\nAborting to maintain program safety.", "Fatal Sync Failure", MB_OK | MB_ICONERROR);
		exit(EXIT_FAILURE);
		return -1;
	}
	//[["sector group 1", "file hash 1", "file hash 2", "..."], ["sector group 2", "file hash 1", "file hash 2", "..."]]
	LPSTR** sectors = (LPSTR**)HeapAlloc(PROCESS_HEAP, 0x0, sizeof(LPSTR) * n_sectors);
	if (!sectors) {
		MessageBoxA(main_wnd, "Memory allocation failed during sync parsing.", "Sync Failure", MB_ICONERROR | MB_OK);
		return -1;
	}

	LPSTR line_context = NULL;
	LPSTR line = strtok_s(resp.response, "\n", &line_context);
	// Skip the first line (n_sectors count)
	if (line) line = strtok_s(NULL, "\n", &line_context);
	else goto SYNCHRONIZE_SERVER_MISFORMAT_ERR;
	// Parse each sector line: "sectorname:hash1/hash2/hash3/hash4/hash5/"
	for (UINT sector_idx = 0; sector_idx < n_sectors; ++sector_idx) {
		sectors[sector_idx] = (LPSTR*)HeapAlloc(PROCESS_HEAP, 0x0, sizeof(LPSTR) * (FILES_PER_SECTOR + 1));
		sectors[sector_idx][0] = (LPSTR)HeapAlloc(PROCESS_HEAP, 0x0, MAX_SECTORNAME_SIZE + 1);
		for (UINT i = 0; i < FILES_PER_SECTOR; i++) {
			sectors[sector_idx][1 + i] = (LPSTR)HeapAlloc(PROCESS_HEAP, 0x0, SHA1_HASH_LEN + 1);
		}
		INT rc = sscanf_s(line, "%[^:]:%[^/]/%[^/]/%[^/]/%[^/]/%[^/]",
			sectors[sector_idx][0], (unsigned)MAX_SECTORNAME_SIZE + 1,
			sectors[sector_idx][1], (unsigned)SHA1_HASH_LEN + 1,
			sectors[sector_idx][2], (unsigned)SHA1_HASH_LEN + 1,
			sectors[sector_idx][3], (unsigned)SHA1_HASH_LEN + 1,
			sectors[sector_idx][4], (unsigned)SHA1_HASH_LEN + 1,
			sectors[sector_idx][5], (unsigned)SHA1_HASH_LEN + 1
		);
		if (rc != 1 + FILES_PER_SECTOR) {
			goto SYNCHRONIZE_SERVER_MISFORMAT_ERR;
		};
		for (UINT i = 0; i < FILES_PER_SECTOR; i++) {
			if (strlen(sectors[sector_idx][1 + i]) != SHA1_HASH_LEN) goto SYNCHRONIZE_SERVER_MISFORMAT_ERR;
		}
		if (sector_idx < n_sectors - 1) {
			if (NULL == (line = strtok_s(NULL, "\n", &line_context))) goto SYNCHRONIZE_SERVER_MISFORMAT_ERR;
		}
	}
	CHAR active_sector_part[MAX_DIR_SIZE + 1 /*\*/ + MAX_SECTORNAME_SIZE + 1/*.*/ + MAX_SECTORPART_EXT_LENGTH + 1/*\0*/] = { 0 };
	LPSTR* missing_sectors = (LPSTR*)HeapAlloc(PROCESS_HEAP, 0x0, n_sectors * sizeof(LPSTR));
	LPSTR* diffed_sectors = (LPSTR*)HeapAlloc(PROCESS_HEAP, 0x0, n_sectors * sizeof(LPSTR));
	UINT n_missing_sectors = 0;
	UINT n_diffed_sectors = 0;
	CHAR out_hash[SHA1_HASH_LEN + 1];
	{
		//Scan for sectors that are missing parts
		for (UINT i = 0; i < n_sectors; i++) {
			for (UINT j = 0; j < ARRAYSIZE(SECTOR_PART_ORDER); j++) {
				sprintf_s(active_sector_part, ARRAYSIZE(active_sector_part), "%s\\%s.%s", dir, sectors[i][0], SECTOR_PART_ORDER[j]);
				if (!PathFileExistsA(active_sector_part)) {
					missing_sectors[n_missing_sectors] = (LPSTR)HeapAlloc(PROCESS_HEAP, HEAP_ZERO_MEMORY, MAX_SECTORNAME_SIZE + 1);
					if (!missing_sectors[n_missing_sectors]) break;
					strcpy_s(missing_sectors[n_missing_sectors], MAX_SECTORNAME_SIZE + 1, sectors[i][0]);
					++n_missing_sectors;
					break;
				}
				SERVER_SYNC_CHECK_SECTOR_DIFFS:
				if (!file_to_sha1(active_sector_part, (LPSTR)out_hash)) {
					INT res = MessageBoxA(main_wnd, active_sector_part, "Failed to hash following sector part", MB_ABORTRETRYIGNORE | MB_ICONERROR);
					if (res == IDABORT) exit(EXIT_FAILURE);
					else if (res == IDRETRY) goto SERVER_SYNC_CHECK_SECTOR_DIFFS;
					else if (res == IDIGNORE) continue;
				}
				if(strncmp(out_hash, sectors[i][j + 1], (SIZE_T)SHA1_HASH_LEN)) {
					diffed_sectors[n_diffed_sectors] = (LPSTR)HeapAlloc(PROCESS_HEAP, HEAP_ZERO_MEMORY, MAX_SECTORNAME_SIZE + 1);
					if (!diffed_sectors[n_diffed_sectors]) break;
					strcpy_s(diffed_sectors[n_diffed_sectors], MAX_SECTORNAME_SIZE + 1, sectors[i][0]);
					++n_diffed_sectors;
					break;
				}
			}
		}
		if (n_missing_sectors) {
			static CHAR informer_msg[] = "Following sectors are missing parts!\nWould you like to manage the conflicts?\n";
			UINT informer_size = 1 + ARRAYSIZE(informer_msg) + n_missing_sectors * (MAX_SECTORNAME_SIZE + 2);
			LPSTR missing_sectors_informer = (LPSTR)HeapAlloc(PROCESS_HEAP, HEAP_ZERO_MEMORY, informer_size * sizeof(CHAR));
			strcpy_s(missing_sectors_informer, informer_size, informer_msg);
			// Point to the end of the current string and compute remaining space from the buffer start
			LPCHAR ptr = missing_sectors_informer + strlen(missing_sectors_informer);
			for (UINT i = 0; i < n_missing_sectors; i++) {
				size_t remaining = informer_size - (ptr - missing_sectors_informer);
				ptr += sprintf_s(ptr, remaining, "%s\n", missing_sectors[i]);
			}
			CHAR caption[64];
			sprintf_s(caption, ARRAYSIZE(caption), "Detected %d sectors missing parts", n_missing_sectors);
			if (MessageBoxA(main_wnd, missing_sectors_informer, caption, MB_ICONWARNING | MB_YESNO) == IDYES) {
				conflict_mgr_meta meta = {
					.n_sectors = n_missing_sectors,
						.sectors = missing_sectors,
						.uploading_allowed = FALSE
				};
				DialogBoxParam(main_instance, MAKEINTRESOURCE(IDD_CONFLICT), main_wnd, &ConflictsDlgProc, (LPARAM) &meta);
			}
			for (UINT i = 0; i < n_missing_sectors; i++) HeapFree(PROCESS_HEAP, 0x0, (LPVOID)missing_sectors[i]);
			HeapFree(PROCESS_HEAP, 0x0, missing_sectors_informer);
		}
		if (n_diffed_sectors) {
			static CHAR diffed_informer_msg[] = "The server's map has the following different sectors to your map!\nWould you like to manage the conflicts?\n";
			UINT diffed_informer_size = 1 + ARRAYSIZE(diffed_informer_msg) + n_diffed_sectors * (MAX_SECTORNAME_SIZE + 2);
			LPSTR diffed_sectors_informer = (LPSTR)HeapAlloc(PROCESS_HEAP, HEAP_ZERO_MEMORY, diffed_informer_size);
			strcpy_s(diffed_sectors_informer, diffed_informer_size, diffed_informer_msg);
			// Point to the end of the current string and compute remaining space from the buffer start
			LPCHAR diffed_ptr = diffed_sectors_informer + strlen(diffed_sectors_informer);
			for (UINT i = 0; i < n_diffed_sectors; i++) {
				size_t remaining = diffed_informer_size - (diffed_ptr - diffed_sectors_informer);
				diffed_ptr += sprintf_s(diffed_ptr, remaining, "%s\n", diffed_sectors[i]);
			}
			CHAR diffed_caption[64];
			sprintf_s(diffed_caption, ARRAYSIZE(diffed_caption), "Detected %d unsynced sectors", n_diffed_sectors);
			if (MessageBoxA(main_wnd, diffed_sectors_informer, diffed_caption, MB_ICONWARNING | MB_YESNO) == IDYES) {
				conflict_mgr_meta meta = {
					.n_sectors = n_diffed_sectors,
						.sectors = diffed_sectors,
						.uploading_allowed = TRUE
				};
				DialogBoxParam(main_instance, MAKEINTRESOURCE(IDD_CONFLICT), main_wnd, &ConflictsDlgProc, (LPARAM)&meta);
			}
			for (UINT i = 0; i < n_diffed_sectors; i++) HeapFree(PROCESS_HEAP, 0x0, (LPVOID)diffed_sectors[i]);
			HeapFree(PROCESS_HEAP, 0x0, diffed_sectors_informer);
		}
		else if (!n_missing_sectors && !n_diffed_sectors) {
			MessageBoxA(main_wnd, "You're all up-to-date.", "Successful Sync", MB_OK | MB_ICONASTERISK);
		}
	}
	HeapFree(PROCESS_HEAP, 0x0, missing_sectors);
	HeapFree(PROCESS_HEAP, 0x0, diffed_sectors);
	heapfree_recv_structure(n_sectors, FILES_PER_SECTOR + 1, sectors);
	FREE_IF_NONNULL(resp.response);
	return TRUE;
}
#endif // !ATSS_SYNCHRONIZE_H