#pragma once
#ifndef ATSS_DLG_H
#define ATSS_DLG_H 1
#include <Windows.h>
#include<ShlObj.h>
#include <wctype.h>
#define END_CURRENT_DLG EndDialog(hDlg, IDCANCEL); return (INT_PTR)
#define HISTORY_DLG_DEFAULT_RSN_TXT "Select an entry to retrieve its reason."
#define HISTORY_DLG_DEFAULT_AFFECTED_TXT "Select an entry to retrieve its affected sectors."
HWND credentials_dlg = NULL;
HWND folderer_dlg = NULL;
HWND abt_dlg = NULL;
HWND new_announcement_dlg = NULL;
HWND conflicts_dlg = NULL;
HWND placersn_dlg = NULL;
HWND lockrsn_dlg = NULL;
HWND history_dlg = NULL;
HWND actives_dlg = NULL;
HWND guard_dlg = NULL;
INT_PTR CALLBACK CredentialsDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK FoldererDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK AbtDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK NewAnnouncementDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK ConflictsDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK PlacementReasonDlg(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK NewLockDlg(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK HistoryDlg(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK ActivesDlg(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK GuardDlg(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
//Is a string alphanumeric-, whitespace-, and/or periods-only?
BOOL isalnum_wsp_dot(LPSTR str) {
	INT i = 0;
	CHAR letter;
	while (TRUE) {
		letter = str[i++];
		if (letter == '\0') return TRUE;
		if (!isalnum((INT)letter) && !isspace((INT)letter) && letter != '.') return FALSE;
	}
}

//Is a string alphanumeric-, plus-, and/or hyphen-only?
BOOL isalnum_plus_hyphen(LPSTR str) {
	INT i = 0;
	CHAR letter;
	while (TRUE) {
		letter = str[i++];
		if (letter == '\0') return TRUE;
		if (!isalnum((INT)letter) && letter != '-' && letter != '+') return FALSE;
	}
}

INT_PTR CALLBACK CredentialsDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
	case WM_INITDIALOG:;
		credentials_dlg = hDlg;
		InitCommonControlsEx(&DEFAULT_COMMON_CONTROLS);
		HWND usr_edit = GetDlgItem(hDlg, IDC_CREDENTIALS_USR);
		HWND pwd_edit = GetDlgItem(hDlg, IDC_CREDENTIALS_PWD);
		HWND host_edit = GetDlgItem(hDlg, IDC_CREDENTIALS_HOST);
		SetWindowTextA(usr_edit, (LPCSTR)usr);
		SetWindowTextA(pwd_edit, (LPCSTR)pwd);
		if (!strcmp(host_base, INTERNAL_DEF_HOST_BASE)) {
			SetWindowTextA(host_edit, DEF_HOST_BASE);
		}
		else {
			SetWindowTextA(host_edit, (LPCSTR)host_base);
		}
		Edit_LimitText(usr_edit, MAX_USRPWD_SIZE);
		Edit_LimitText(pwd_edit, MAX_USRPWD_SIZE);
		Edit_LimitText(host_edit, MAX_HOSTNAME_SIZE);
		return (INT_PTR)TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_CREDENTIALS_CANCEL:
			goto END_CREDENTIALS_DLG;
			break;
		case IDC_CREDENTIALS_OK:;
			WCHAR usr_buf[MAX_USRPWD_SIZE + 1] = { 0 };
			WCHAR pwd_buf[MAX_USRPWD_SIZE + 1] = { 0 };
			WCHAR host_buf[MAX_HOSTNAME_SIZE + 1] = { 0 };
			UINT copied_usr, copied_pwd, copied_host;
			copied_usr = GetDlgItemTextW(hDlg, IDC_CREDENTIALS_USR, usr_buf, ARRAYSIZE(usr_buf));
			copied_pwd = GetDlgItemTextW(hDlg, IDC_CREDENTIALS_PWD, pwd_buf, ARRAYSIZE(pwd_buf));
			copied_host = GetDlgItemTextW(hDlg, IDC_CREDENTIALS_HOST, host_buf, ARRAYSIZE(host_buf));
			if (!copied_usr || !copied_host || !copied_pwd) {
				MessageBoxA(hDlg, "You may not leave a field blank!", "Invalid input", MB_ICONEXCLAMATION | MB_OK);
				return (INT_PTR)TRUE;
			}
			for (USHORT i = 0; i < MAX_USRPWD_SIZE; i++) {
				if (usr_buf[i] == L'\0') break;
				if (!iswalpha((INT)usr_buf[i])) {
					MessageBoxA(hDlg, "The username field has invalid character; only a-Z allowed.", "Invalid Input", MB_ICONWARNING | MB_OK);
					return (INT_PTR)TRUE;
				}
			}
			for (USHORT i = 0; i < MAX_USRPWD_SIZE; i++) {
				if (pwd_buf[i] == L'\0') break;
				if (!iswalnum((INT)pwd_buf[i])) {
					MessageBoxA(hDlg, "The password field has invalid character; only a-Z/0-9 allowed.", "Invalid Input", MB_ICONWARNING | MB_OK);
					return (INT_PTR)TRUE;
				}
			}
			INT usr_utf8_len = WideCharToMultiByte(CP_UTF8, 0, usr_buf, -1, NULL, 0, NULL, NULL);
			INT pwd_utf8_len = WideCharToMultiByte(CP_UTF8, 0, pwd_buf, -1, NULL, 0, NULL, NULL);
			INT host_utf8_len = WideCharToMultiByte(CP_UTF8, 0, host_buf, -1, NULL, 0, NULL, NULL);
			if (usr_utf8_len == 0 || pwd_utf8_len == 0 || host_utf8_len == 0) {
			CREDENTIALS_FIELD_CONVERSION_ERR:
				MessageBoxA(hDlg, "Failed to convert input fields to UTF-8. Make sure all characters you entered are compatible.", "Input Error", MB_ICONERROR);
				return (INT_PTR)TRUE;
			};
			if (usr_utf8_len > (INT)ARRAYSIZE(usr) || pwd_utf8_len > (INT)ARRAYSIZE(pwd) || host_utf8_len > (INT)ARRAYSIZE(host_base)) {
				MessageBoxA(hDlg, "One field you input has too many characters.", "Input Error", MB_ICONERROR);
				return (INT_PTR)TRUE;
			}
			LPSTR usr_utf8_buf = (LPSTR)HeapAlloc(GetProcessHeap(), 0, (SIZE_T)usr_utf8_len);
			LPSTR pwd_utf8_buf = (LPSTR)HeapAlloc(GetProcessHeap(), 0, (SIZE_T)pwd_utf8_len);
			LPSTR host_utf8_buf = (LPSTR)HeapAlloc(GetProcessHeap(), 0, (SIZE_T)host_utf8_len);
			if (!usr_utf8_buf || !pwd_utf8_buf || !host_utf8_buf) {
				if (usr_utf8_buf) HeapFree(GetProcessHeap(), 0, usr_utf8_buf);
				if (pwd_utf8_buf) HeapFree(GetProcessHeap(), 0, pwd_utf8_buf);
				if (host_utf8_buf) HeapFree(GetProcessHeap(), 0, host_utf8_buf);
				goto CREDENTIALS_FIELD_CONVERSION_ERR;
			}
			INT wrote1 = WideCharToMultiByte(CP_UTF8, 0, usr_buf, -1, usr_utf8_buf, usr_utf8_len, NULL, NULL);
			INT wrote2 = WideCharToMultiByte(CP_UTF8, 0, pwd_buf, -1, pwd_utf8_buf, pwd_utf8_len, NULL, NULL);
			INT wrote3 = WideCharToMultiByte(CP_UTF8, 0, host_buf, -1, host_utf8_buf, host_utf8_len, NULL, NULL);
			if (wrote1 == 0 || wrote2 == 0 || wrote3 == 0) {
				if (usr_utf8_buf) HeapFree(GetProcessHeap(), 0, usr_utf8_buf);
				if (pwd_utf8_buf) HeapFree(GetProcessHeap(), 0, pwd_utf8_buf);
				if (host_utf8_buf) HeapFree(GetProcessHeap(), 0, host_utf8_buf);
				goto CREDENTIALS_FIELD_CONVERSION_ERR;
			}
			replace_storage_line(STORAGE__USR, usr_utf8_buf);
			replace_storage_line(STORAGE__PWD, pwd_utf8_buf);
			replace_storage_line(STORAGE__HOSTNAME, host_utf8_buf);
			ZeroMemory(usr, sizeof(usr));
			ZeroMemory(pwd, sizeof(pwd));
			ZeroMemory(host_base, sizeof(host_base));
			strcpy_s(usr, ARRAYSIZE(usr), usr_utf8_buf);
			strcpy_s(pwd, ARRAYSIZE(pwd), pwd_utf8_buf);
			strcpy_s(host_base, ARRAYSIZE(host_base), strcmp(DEF_HOST_BASE, host_utf8_buf) ? host_utf8_buf : INTERNAL_DEF_HOST_BASE);
			HeapFree(GetProcessHeap(), 0, usr_utf8_buf);
			HeapFree(GetProcessHeap(), 0, pwd_utf8_buf);
			HeapFree(GetProcessHeap(), 0, host_utf8_buf);
			MessageBoxA(hDlg, "All input fields have been validated and credentials have been saved to disk.\nWill now try to check for announcements...", "Credentials Modified", MB_OK | MB_ICONASTERISK);
			check_for_announcements(TRUE);

			goto END_CREDENTIALS_DLG;
			break;
		}
		break;
	case WM_DESTROY:
	case WM_CLOSE:
	END_CREDENTIALS_DLG:
		END_CURRENT_DLG IDOK;
		credentials_dlg = NULL;
		return (INT_PTR)TRUE;
	}
	return (INT_PTR)FALSE;
}

INT_PTR CALLBACK FoldererDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
	case WM_INITDIALOG:
		folderer_dlg = hDlg;
		InitCommonControlsEx(&DEFAULT_COMMON_CONTROLS);
		SendDlgItemMessageW(hDlg, IDC_FOLDERER_PATHDISP, EM_LIMITTEXT, (WPARAM)MAX_DIR_SIZE, (LPARAM)NULL);
		/* dir is stored as UTF-8; convert to wide for the Unicode edit control */
		WCHAR wideDir[MAX_DIR_SIZE + 1] = { 0 };
		if (dir[0] != '\0') {
			INT need = MultiByteToWideChar(CP_UTF8, 0, dir, -1, NULL, 0);
			if (need > 0 && need <= ARRAYSIZE(wideDir)) {
				MultiByteToWideChar(CP_UTF8, 0, dir, -1, wideDir, ARRAYSIZE(wideDir));
			}
		}
		SetDlgItemTextW(hDlg, IDC_FOLDERER_PATHDISP, wideDir);
		return (INT_PTR)TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_FOLDERER_CHOOSE: {
			BROWSEINFOW bi = { 0 };
			bi.hwndOwner = hDlg;
			bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE | BIF_VALIDATE;
			bi.lpszTitle = L"Select Target Directory";
			LPITEMIDLIST pidl = SHBrowseForFolderW(&bi);
			if (!pidl) {
				MessageBoxA(hDlg, "Directory selection was canceled or failed.", "Notice", MB_OK | MB_ICONWARNING);
				break;
			}
			WCHAR selPathW[MAX_PATH] = { 0 };
			if (!SHGetPathFromIDListW(pidl, selPathW)) {
				CoTaskMemFree(pidl);
				MessageBoxA(hDlg, "Failed to obtain the selected path.", "Error", MB_ICONERROR | MB_OK);
				break;
			}
			CoTaskMemFree(pidl);
			/* Set the edit control to the selected path (wide) */
			SetDlgItemTextW(hDlg, IDC_FOLDERER_PATHDISP, selPathW);
			break;
		}
		case IDC_FOLDERER_OK: {
			/* Read wide text from edit control, convert to UTF-8, save */
			WCHAR selBufW[MAX_DIR_SIZE + 1] = { 0 };
			GetDlgItemTextW(hDlg, IDC_FOLDERER_PATHDISP, selBufW, ARRAYSIZE(selBufW));
			/* Convert to UTF-8 */
			INT need = WideCharToMultiByte(CP_UTF8, 0, selBufW, -1, NULL, 0, NULL, NULL);
			if (need == 0 || need > (INT)ARRAYSIZE(dir)) {
				MessageBoxA(hDlg, "Selected path is too long or invalid.", "Error", MB_ICONERROR | MB_OK);
				break;
			}
			LPSTR utf8buf = (LPSTR)HeapAlloc(PROCESS_HEAP, 0, (SIZE_T)need);
			if (!WideCharToMultiByte(CP_UTF8, 0, selBufW, -1, utf8buf, need, NULL, NULL)) {
				HeapFree(PROCESS_HEAP, 0, utf8buf);
				MessageBoxA(hDlg, "Failed to convert selected path.", "Error", MB_ICONERROR | MB_OK);
				break;
			}
			/* Save to persistent storage and update global */
			replace_storage_line(STORAGE__DIR, utf8buf);
			StringCchCopyA(dir, ARRAYSIZE(dir), utf8buf);
			HeapFree(PROCESS_HEAP, 0, utf8buf);
			MessageBoxA(hDlg, "Folder preference set and saved to persistent storage.", "Saved", MB_ICONASTERISK | MB_OK);
			goto END_FOLDERER_DLG;
		}
		case IDC_FOLDERER_CANCEL:
			goto END_FOLDERER_DLG;
		}
		return (INT_PTR)TRUE;
	case WM_DESTROY:
	case WM_CLOSE:
	END_FOLDERER_DLG:
		END_CURRENT_DLG IDOK;
		folderer_dlg = NULL;
		return (INT_PTR)TRUE;
	}
	return (INT_PTR)FALSE;
}

INT_PTR CALLBACK AbtDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
	case WM_INITDIALOG:
		InitCommonControlsEx(&DEFAULT_COMMON_CONTROLS);
		SetDlgItemTextA(hDlg, IDC_ABTDLG_VER, "PROGRAM:\nv" PROGRAM_VER);
		SetDlgItemTextA(hDlg, IDC_ABTDLG_COMPILATION, "COMPILED:\n" __TIMESTAMP__);
		abt_dlg = hDlg;
		break;
	case WM_COMMAND:
		if (LOWORD(wParam) == IDC_ABTDLG_OK) goto END_ABT_DLG;
	case WM_DESTROY:
	case WM_CLOSE:
	END_ABT_DLG:
		END_CURRENT_DLG IDOK;
		abt_dlg = NULL;
		return (INT_PTR)TRUE;
	}
	return (INT_PTR)FALSE;
}

INT_PTR CALLBACK NewAnnouncementDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
	case WM_INITDIALOG:
		InitCommonControlsEx(&DEFAULT_COMMON_CONTROLS);
		new_announcement_dlg = hDlg;
		SendDlgItemMessageA(hDlg, IDC_NEW_ANNOUNCEMENT_EDITOR, EM_LIMITTEXT, (WPARAM)MAX_SENT_ANNOUNCEMENT_SIZE, (LPARAM)NULL);
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_NEW_ANNOUNCEMENT_OK:;
			CHAR announcement[MAX_SENT_ANNOUNCEMENT_SIZE + 1];
			GetDlgItemTextA(hDlg, IDC_NEW_ANNOUNCEMENT_EDITOR, (LPSTR)announcement, ARRAYSIZE(announcement));
			UINT i = 0;
			CHAR letter;
			while (i < ARRAYSIZE(announcement)) {
				letter = announcement[i++];
				if (letter == '\0') break;
				if (!isalnum((INT)letter) && !isspace((INT)letter) && letter != '.') {
					MessageBoxA(hDlg, "Your announcement contains an invalid character", "Failed to Send Announcement", MB_ICONWARNING | MB_OK);
					return (INT_PTR)TRUE;
				}
			}
			//Decrementing i so that when the request is sent, the announcement length will reflect the true string length and will not send the null-terminator, as i is currently 1 higher than the string's length.
			if (i-- == 1) {
				MessageBoxA(hDlg, "Your announcement is empty.\nCannot send an empty announcement.", "Failed to Send Announcement", MB_ICONWARNING | MB_OK);
				return (INT_PTR)TRUE;
			}
			CHAR url_buf[MAX_SENT_ANNOUNCEMENT_SIZE + MAX_HOSTNAME_SIZE + 64];
			sprintf_s(url_buf, ARRAYSIZE(url_buf), "%s/announce/?usr=%s&pwd=%s&ver=%s", host_base, usr, pwd, PROGRAM_VER);
			http_response_t resp = send_http_post((LPCSTR)url_buf, announcement, i, "text/plain", DEFAULT_HTTP_TIMEOUT);
			if (resp.errored) {
				MessageBoxA(hDlg, "Client request to announce failed to connect to server's handler.", "Failed to Send Announcement", MB_ICONWARNING | MB_OK);
				return (INT_PTR)TRUE;
			}
			if (!resp.serverok) {
				MessageBoxA(hDlg, resp.response, "Server-side Announcement Request Rejection", MB_OK | MB_ICONERROR);
				FREE_IF_NONNULL(resp.response);
				return (INT_PTR)TRUE;
			}
			MessageBoxA(hDlg, "Your announcement has been successfully sent and queued.\nActive mappers will receive it within the next minute.", "Announcement Successful", MB_OK);
			FREE_IF_NONNULL(resp.response);
			goto END_NEW_ANNOUNCEMENT_DLG;
		case IDC_NEW_ANNOUNCEMENT_CANCEL:
			goto END_NEW_ANNOUNCEMENT_DLG;
		}
		return (INT_PTR)TRUE;
	case WM_DESTROY:
	case WM_CLOSE:
	END_NEW_ANNOUNCEMENT_DLG:
		END_CURRENT_DLG IDOK;
		new_announcement_dlg = NULL;
		return (INT_PTR)TRUE;
	}
	return (INT_PTR)FALSE;
}

INT_PTR CALLBACK ConflictsDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
	case WM_INITDIALOG:;
		InitCommonControlsEx(&DEFAULT_COMMON_CONTROLS);
		conflicts_dlg = hDlg;
		SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)lParam);
		conflict_mgr_meta* meta = (conflict_mgr_meta*)lParam;
		if (meta == NULL) {
			END_CURRENT_DLG IDCANCEL;
		}
		SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)meta);
		EnableWindow(GetDlgItem(hDlg, IDC_CONFLICT_UPDATE), meta->uploading_allowed);
		EnableWindow(GetDlgItem(hDlg, IDC_CONFLICT_UPDATEALL), meta->uploading_allowed);
		INT pos;
		for (UINT i = 0; i < meta->n_sectors; i++) {
			if (strlen(meta->sectors[i]) > MAX_SECTORNAME_SIZE) {
				MessageBoxA(hDlg, "A passed in sector name was too long.\nAborting to maintain program safety.", "Error while creating Conflicts Dialog", MB_OK | MB_ICONERROR);
				exit(EXIT_FAILURE);
			}
			pos = (INT)SendDlgItemMessageA(hDlg, IDC_CONFLICT_LIST, LB_ADDSTRING, (WPARAM)NULL, (LPARAM)meta->sectors[i]);
			SendDlgItemMessage(hDlg, IDC_CONFLICT_LIST, LB_SETITEMDATA, (WPARAM)pos, (LPARAM)i);
		}
		SetFocus(GetDlgItem(hDlg, IDC_CONFLICT_LIST));
		return (INT_PTR)TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_CONFLICT_OK:
			goto END_CONFLICT_DLG;
			//TODO
		case IDC_CONFLICT_IGNORE:;
			INT selected = (INT)SendDlgItemMessage(hDlg, IDC_CONFLICT_LIST, LB_GETCURSEL, (WPARAM)NULL, (LPARAM)NULL);
			if (selected == LB_ERR) break;
			SendDlgItemMessage(hDlg, IDC_CONFLICT_LIST, LB_DELETESTRING, (WPARAM)selected, (LPARAM)NULL);
		CHECK_FOR_EMPTY_LIST_CONFLICT_DLG:;
			INT remaining = (INT)SendDlgItemMessage(hDlg, IDC_CONFLICT_LIST, LB_GETCOUNT, (WPARAM)NULL, (LPARAM)NULL);
			if (!remaining) {
				EnableWindow(GetDlgItem(hDlg, IDC_CONFLICT_GET), FALSE);
				EnableWindow(GetDlgItem(hDlg, IDC_CONFLICT_GETALL), FALSE);
				EnableWindow(GetDlgItem(hDlg, IDC_CONFLICT_UPDATE), FALSE);
				EnableWindow(GetDlgItem(hDlg, IDC_CONFLICT_IGNORE), FALSE);
				EnableWindow(GetDlgItem(hDlg, IDC_CONFLICT_UPDATEALL), FALSE);
			}
			break;
		case IDC_CONFLICT_GET:;
			INT idx2 = (INT)SendDlgItemMessageA(hDlg, IDC_CONFLICT_LIST, LB_GETCURSEL, 0, 0);
			if (idx2 == LB_ERR) return (INT_PTR)FALSE;
			LPSTR sector_buf2 = (LPSTR)HeapAlloc(PROCESS_HEAP, 0x0, sizeof(CHAR) * (MAX_SECTORNAME_SIZE + 1));
			SendDlgItemMessageA(hDlg, IDC_CONFLICT_LIST, LB_GETTEXT, (WPARAM)idx2, (LPARAM)sector_buf2);
			if (retrieve_from_server(sector_buf2)) SendMessageA(hDlg, CWM_SUCCESSFUL_SECTOR_PROCESSING, (WPARAM)NULL, (LPARAM)sector_buf2);
			HeapFree(PROCESS_HEAP, 0x0, sector_buf2);
			break;
		case IDC_CONFLICT_GETALL:;
			UINT n_items2 = (unsigned)SendDlgItemMessageA(hDlg, IDC_CONFLICT_LIST, LB_GETCOUNT, (WPARAM)NULL, (LPARAM)NULL);
			if (n_items2 == LB_ERR || !n_items2) return (INT_PTR)TRUE;
			LPSTR getall_sector_buf = (LPSTR)HeapAlloc(PROCESS_HEAP, 0x0, sizeof(CHAR) * (MAX_SECTORNAME_SIZE + 1));
			for (UINT i = 0; i < n_items2; i++) {
				SendDlgItemMessageA(hDlg, IDC_CONFLICT_LIST, LB_GETTEXT, (WPARAM)i, (LPARAM)getall_sector_buf);
				if (retrieve_from_server(getall_sector_buf)) SendMessageA(hDlg, CWM_SUCCESSFUL_SECTOR_PROCESSING, (WPARAM)NULL, (LPARAM)getall_sector_buf);
			}
			HeapFree(PROCESS_HEAP, 0x0, getall_sector_buf);
			break;
		case IDC_CONFLICT_UPDATE:;
			INT idx = (INT)SendDlgItemMessageA(hDlg, IDC_CONFLICT_LIST, LB_GETCURSEL, 0, 0);
			if (idx == LB_ERR) return (INT_PTR)FALSE;
			LPSTR sector_buf = (LPSTR)HeapAlloc(PROCESS_HEAP, 0x0, sizeof(CHAR) * (MAX_SECTORNAME_SIZE + 1));
			SendDlgItemMessageA(hDlg, IDC_CONFLICT_LIST, LB_GETTEXT, (WPARAM)idx, (LPARAM)sector_buf);
			placersn_dlg_meta meta = {
				.n_sectors = 1,
				.sector_names = &sector_buf,
				.calling_dlg = hDlg
			};
			DialogBoxParam(main_instance, MAKEINTRESOURCE(IDD_PLACERSN), hDlg, &PlacementReasonDlg, (LPARAM)&meta);
			HeapFree(PROCESS_HEAP, 0x0, (LPVOID)sector_buf);
			break;
		case IDC_CONFLICT_UPDATEALL:;
			UINT n_items = (unsigned)SendDlgItemMessageA(hDlg, IDC_CONFLICT_LIST, LB_GETCOUNT, (WPARAM)NULL, (LPARAM)NULL);
			if (n_items == LB_ERR || !n_items) return (INT_PTR)TRUE;
			LPSTR* sector_names = (LPSTR*)HeapAlloc(PROCESS_HEAP, 0x0, n_items * sizeof(LPSTR));
			placersn_dlg_meta meta2 = {
				.n_sectors = n_items,
				.sector_names = sector_names,
				.calling_dlg = hDlg
			};
			for (UINT i = 0; i < n_items; i++) {
				sector_names[i] = (LPSTR)HeapAlloc(PROCESS_HEAP, 0x0, sizeof(CHAR) * (MAX_SECTORNAME_SIZE + 1));
				SendDlgItemMessageA(hDlg, IDC_CONFLICT_LIST, LB_GETTEXT, (WPARAM)i, (LPARAM)sector_names[i]);
			}
			DialogBoxParam(main_instance, MAKEINTRESOURCE(IDD_PLACERSN), hDlg, &PlacementReasonDlg, (LPARAM)&meta2);
			for (UINT i = 0; i < n_items; i++) HeapFree(PROCESS_HEAP, 0x0, (LPVOID)sector_names[i]);
			HeapFree(PROCESS_HEAP, 0x0, (LPVOID)sector_names);
			break;
		}
		return (INT_PTR)TRUE;
	case WM_CLOSE:
	case WM_DESTROY:
	END_CONFLICT_DLG:
		END_CURRENT_DLG IDOK;
		conflicts_dlg = NULL;
		return (INT_PTR)TRUE;
	case CWM_SUCCESSFUL_SECTOR_PROCESSING:; //PlacementReasonDlg informed the handler that a sector was successfully updated on the server.
		INT idx = (INT)SendDlgItemMessageA(hDlg, IDC_CONFLICT_LIST, LB_FINDSTRINGEXACT, (WPARAM)-1, lParam);
		if (idx != LB_ERR) {
			SendDlgItemMessageA(hDlg, IDC_CONFLICT_LIST, LB_DELETESTRING, (WPARAM)idx, (LPARAM)NULL);
			goto CHECK_FOR_EMPTY_LIST_CONFLICT_DLG;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

VOID _placersn_dlg_check_place_onto_server(HWND hDlg, placersn_dlg_meta* meta, LPSTR reason) {
	for (USHORT i = 0; i < meta->n_sectors; i++) {
		if (place_onto_server(meta->sector_names[i], reason) != TRUE) {
			CHAR buf[128];
			sprintf_s(buf, ARRAYSIZE(buf), "Failed to place a sector %s (%hu/%hu) onto the server.\nCancel subsequent placements?", meta->sector_names[i], i + 1, meta->n_sectors);
			if (IDYES == MessageBoxA(hDlg, buf, "Detected Server Placement Error", MB_ICONERROR | MB_YESNO)) break;
		}
		else if (meta->calling_dlg != (HWND)NULL) SendMessageA(meta->calling_dlg, CWM_SUCCESSFUL_SECTOR_PROCESSING, (WPARAM)NULL, (LPARAM)meta->sector_names[i]);
	}
}

INT_PTR CALLBACK PlacementReasonDlg(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
	case WM_INITDIALOG:
		InitCommonControlsEx(&DEFAULT_COMMON_CONTROLS);
		if ((LPVOID)lParam == NULL) {
			END_CURRENT_DLG IDCANCEL;
		}
		SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)lParam);
		SendDlgItemMessageA(hDlg, IDC_PLACERSN_RSN, EM_LIMITTEXT, (WPARAM)MAX_PLACEMENT_REASON_LEN, (LPARAM)NULL);
		placersn_dlg_meta* meta_buf = (placersn_dlg_meta*)lParam;
		CHAR sector_msg_base[] = "(Updating Sectors: ";
		SIZE_T sector_msg_size = ARRAYSIZE(sector_msg_base) + (SIZE_T)meta_buf->n_sectors * (MAX_SECTORNAME_SIZE + 2) + 3;
		LPSTR sector_msg = (LPSTR)HeapAlloc(PROCESS_HEAP, 0x0, sector_msg_size);
		strcpy_s(sector_msg, sector_msg_size, sector_msg_base);
		LPCHAR ptr = (LPCHAR)sector_msg + sizeof(CHAR) * (ARRAYSIZE(sector_msg_base) - 1) /*overwrite \0*/;
		for (USHORT i = 0; i < meta_buf->n_sectors; i++) ptr += sprintf_s((LPSTR)ptr, (SIZE_T)((sector_msg + sizeof(CHAR) * sector_msg_size) - ptr), "%s, ", meta_buf->sector_names[i]);
		//sectorname, 0(END)
		//           ^
		//sectorname)00(END)
		ptr -= 2 * sizeof(CHAR);
		strcpy_s((LPSTR)ptr, (sector_msg + sizeof(CHAR) * sector_msg_size) - ptr, ")");
		SetDlgItemTextA(hDlg, IDC_PLACERSN_SECTOR, (LPCSTR)sector_msg);
		HeapFree(PROCESS_HEAP, 0x0, (LPVOID)sector_msg);
		placersn_dlg = hDlg;
		return (INT_PTR)TRUE;

	case WM_COMMAND:;
		placersn_dlg_meta* meta = (placersn_dlg_meta*)GetWindowLongPtr(hDlg, GWLP_USERDATA);
		switch (LOWORD(wParam)) {
		case IDC_PLACERSN_NORSN:;
			_placersn_dlg_check_place_onto_server(hDlg, meta, (LPSTR)NULL);
			goto END_PLACEMENT_DLG;
		case IDC_PLACERSN_PLACE:;
			CHAR reason[MAX_PLACEMENT_REASON_LEN + 1] = { 0 };
			GetDlgItemTextA(hDlg, IDC_PLACERSN_RSN, (LPSTR)reason, MAX_PLACEMENT_REASON_LEN + 1);
			if (reason[0] == '\0') {
				MessageBoxA(hDlg, "You cannot input a blank reason!\nIf you have no sector update reason, use the button in the bottom right.", "Client-Side Server Placement Error", MB_OK);
				return (INT_PTR)TRUE;
			}
			if (!isalnum_wsp_dot(reason)) {
				MessageBoxA(hDlg, "Your reason must be alphanumeric(spaces and periods are also OK.)\nIt currently is not.", "Client-Side Server Placement Error", MB_OK);
				return (INT_PTR)TRUE;
			}
			_placersn_dlg_check_place_onto_server(hDlg, meta, (LPSTR)reason);
			goto END_PLACEMENT_DLG;
			break;
		case IDC_PLACERSN_CANCEL:
			goto END_PLACEMENT_DLG;
		}
		return (INT_PTR)TRUE;
	case WM_CLOSE:
	case WM_DESTROY:
	END_PLACEMENT_DLG:
		END_CURRENT_DLG IDOK;
		placersn_dlg = NULL;
		return (INT_PTR)TRUE;
	}
	return (INT_PTR)FALSE;
}
INT_PTR CALLBACK LockRsnDlg(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
	case WM_INITDIALOG:
		lockrsn_dlg = hDlg;
		InitCommonControlsEx(&DEFAULT_COMMON_CONTROLS);
		SendDlgItemMessageA(hDlg, IDC_LOCKRSN_RSN, EM_LIMITTEXT, (WPARAM)MAX_LOCK_REASON_LEN, (LPARAM)NULL);
		SendDlgItemMessageA(hDlg, IDC_LOCKRSN_SECTOR, EM_LIMITTEXT, (WPARAM)MAX_SECTORNAME_SIZE, (LPARAM)NULL);
		return (INT_PTR)TRUE;
	case WM_COMMAND:
		if (HIWORD(wParam) != BN_CLICKED) break;
		if (LOWORD(wParam) == IDC_LOCKRSN_SUBMIT) {
			//LPSTR buf = HeapAlloc(PROCESS_HEAP, 0x0, sizeof(CHAR) * (MAX_LOCK_REASON_LEN + 1));
			//LPSTR sector_buf = HeapAlloc(PROCESS_HEAP, 0x0, sizeof(CHAR) * (MAX_SECTORNAME_SIZE + 1));
			CHAR buf[MAX_LOCK_REASON_LEN + 1];
			CHAR sector_buf[MAX_SECTORNAME_SIZE + 1];
			GetDlgItemTextA(hDlg, IDC_LOCKRSN_RSN, (LPSTR)buf, ARRAYSIZE(buf));
			GetDlgItemTextA(hDlg, IDC_LOCKRSN_SECTOR, (LPSTR)sector_buf, ARRAYSIZE(sector_buf));
			SIZE_T len = strlen((LPCSTR)buf);
			if (len < MIN_LOCK_REASON_LEN) {
				MessageBoxA(hDlg, "Your reason must be at least 2 characters long.", "Sector Locker Error", MB_OK | MB_ICONERROR);
				return (INT_PTR)TRUE;
			}
			if (!isalnum_wsp_dot((LPSTR)buf)) {
				MessageBoxA(hDlg, "Your reason must be alphanumeric (spaces, periods allowed).", "Sector Locker Error", MB_OK | MB_ICONERROR);
				return (INT_PTR)TRUE;
			}
			if (lock_sector(buf, LOCK_SECTOR_ACTION_LOCK, sector_buf) == TRUE) {
				MessageBoxA(main_wnd, "Successfully locked specified sector. Will now refresh sector locks list...", "Sector Locker Success", MB_OK | MB_ICONASTERISK);
				update_sector_locks_listbox(TRUE, GetDlgItem(main_wnd, IDC_LOCKS_LIST));
				goto END_LOCKRSN_DLG;
			}
			break;
		}
		else if (LOWORD(wParam) == IDC_LOCKRSN_CANCEL) goto END_LOCKRSN_DLG;
		return (INT_PTR)TRUE;
	case WM_CLOSE:
	case WM_DESTROY:
	END_LOCKRSN_DLG:
		END_CURRENT_DLG IDOK;
		lockrsn_dlg = NULL;
		return (INT_PTR)TRUE;
	}
	return (INT_PTR)FALSE;
}

struct listed_history_item
{
	LPSTR reason;
	LPSTR sectors;
};

struct history_file_out {
	BOOL success;
	UINT n;
	LPSTR** data;
};

struct history_dlg_memory {
	UINT max_file;
	UINT curr_file;
};

struct history_file_out request_history_file(UINT file_n, LPSTR url_buf, SIZE_T size_url_buf) {
	UINT n_entries;
	sprintf_s(url_buf, size_url_buf, "%s/place/?usr=%s&pwd=%s&ver=%s&action=retrieve&file=%u", host_base, usr, pwd, PROGRAM_VER, file_n);
	http_response_t resp = send_http_get((LPCSTR)url_buf, DEFAULT_HTTP_TIMEOUT);
	if (resp.errored) {
		MessageBoxA(main_wnd, "Client request to retrieve history failed to reach server's handler.", "History Retrieval Error", MB_OK | MB_ICONERROR);
		return (struct history_file_out) {
			FALSE
		};
	}
	if (!resp.serverok) {
		MessageBoxA(main_wnd, resp.response, "Server Rejected History Retrieval Request", MB_OK | MB_ICONERROR);
		FREE_IF_NONNULL(resp.response);
		return (struct history_file_out) {
			FALSE
		};
	}
	LPSTR line_context = NULL;
	LPSTR line = strtok_s(resp.response, "\n", &line_context);
	// Skip the first line
	if (line) {
		if (1 != sscanf_s(line, "%u", &n_entries) || n_entries > MAX_HISTORY_ENTRIES_PER_FILE) goto HISTORY_RETRIEVAL_MISFORMAT_ERR;
		line = strtok_s(NULL, "\n", &line_context);
	}
	else {
	HISTORY_RETRIEVAL_MISFORMAT_ERR:
		FREE_IF_NONNULL(resp.response);
		MessageBoxA(main_wnd, "Server's response to client request to retrieve a history file is misformatted! Aborting to maintain program safety.", "History Enumeration Error", MB_OK | MB_ICONERROR);
		exit(EXIT_FAILURE);
	}
	//[ ["sectors", "author", "rsn", "timestamp"], [...]]
	LPSTR** entries = (LPSTR**)HeapAlloc(PROCESS_HEAP, 0x0, sizeof(LPSTR*) * n_entries);
	for (UINT i = 0; i < n_entries; i++) {
		entries[i] = (LPSTR*)HeapAlloc(PROCESS_HEAP, 0x0, sizeof(LPSTR) * HISTORY_FIELDS_PER_ENTRY);
		entries[i][0] = (LPSTR)HeapAlloc(PROCESS_HEAP, 0x0, sizeof(CHAR) * (HISTORY_MAX_SECTORLIST_SIZE + 1));
		entries[i][1] = (LPSTR)HeapAlloc(PROCESS_HEAP, 0x0, sizeof(CHAR) * (MAX_USRPWD_SIZE + 1));
		entries[i][2] = (LPSTR)HeapAlloc(PROCESS_HEAP, 0x0, sizeof(CHAR) * (HISTORY_MAX_REASON_LEN + 1));
		entries[i][3] = (LPSTR)HeapAlloc(PROCESS_HEAP, 0x0, sizeof(CHAR) * (MAX_TIMESTAMP_LEN + 1));
		INT rc = sscanf_s(line, "%[^/]/%[^/]/%[^/]/%[^/]/",
			entries[i][0], (unsigned)(HISTORY_MAX_SECTORLIST_SIZE + 1),
			entries[i][1], (unsigned)(MAX_USRPWD_SIZE + 1),
			entries[i][2], (unsigned)(HISTORY_MAX_REASON_LEN + 1),
			entries[i][3], (unsigned)(MAX_TIMESTAMP_LEN + 1)
		);
		if (rc != 4) goto HISTORY_RETRIEVAL_MISFORMAT_ERR;
		if (i < n_entries - 1) {
			if (NULL == (line = strtok_s(NULL, "\n", &line_context))) goto HISTORY_RETRIEVAL_MISFORMAT_ERR;
		}
	}
	return (struct history_file_out) {
		TRUE, n_entries, entries
	};
}

VOID list_out_history_file(HWND parent, INT target_list, struct history_file_out* file) {
	INT idx;
	CHAR buf[MAX_USRPWD_SIZE + MAX_TIMESTAMP_LEN + 8];
	for (UINT i = 0; i < file->n; i++) {
		sprintf_s(buf, ARRAYSIZE(buf), "%s: %s", file->data[i][1], file->data[i][3]);
		idx = (INT)SendDlgItemMessageA(parent, target_list, LB_ADDSTRING, (WPARAM)NULL, (LPARAM)buf);
		struct listed_history_item* data = (struct listed_history_item*)HeapAlloc(PROCESS_HEAP, 0x0, sizeof(struct listed_history_item));
		data->reason = (LPSTR)HeapAlloc(PROCESS_HEAP, 0x0, sizeof(CHAR) * (HISTORY_MAX_REASON_LEN + 1));
		data->sectors = (LPSTR)HeapAlloc(PROCESS_HEAP, 0x0, sizeof(CHAR) * (HISTORY_MAX_SECTORLIST_SIZE + 1));
		strcpy_s(data->reason, HISTORY_MAX_REASON_LEN + 1, file->data[i][2]);
		strcpy_s(data->sectors, HISTORY_MAX_SECTORLIST_SIZE + 1, file->data[i][0]);
		SendDlgItemMessage(parent, target_list, LB_SETITEMDATA, (WPARAM)idx, (LPARAM)data);
	}
}

VOID clear_out_history(HWND parent, INT target_list) {
	LRESULT res;
	INT n_items = (INT)SendDlgItemMessage(parent, target_list, LB_GETCOUNT, (WPARAM)NULL, (LPARAM)NULL);
	struct listed_history_item* data;
	if (n_items && n_items != LB_ERR) while (LB_ERR != (res = SendDlgItemMessage(parent, target_list, LB_GETITEMDATA, (WPARAM)0, (LPARAM)NULL))) {
		data = (struct listed_history_item*)res;
		HeapFree(PROCESS_HEAP, 0x0, data->reason);
		HeapFree(PROCESS_HEAP, 0x0, data->sectors);
		HeapFree(PROCESS_HEAP, 0x0, data);
		SendDlgItemMessage(parent, target_list, LB_DELETESTRING, (WPARAM)0, (LPARAM)NULL);
	}
}

VOID heapfree_recv_structure(_In_ INT n_lvl1, _In_ INT n_lvl2, _In_ LPSTR** structure) {
	for (INT i = 0; i < n_lvl1; i++) {
		for (INT j = 0; j < n_lvl2; j++)
			HeapFree(PROCESS_HEAP, 0x0, (LPVOID)structure[i][j]);
		HeapFree(PROCESS_HEAP, 0x0, structure[i]);
	}
	HeapFree(PROCESS_HEAP, 0x0, structure);
}

#define HISTORY_DLG_RESET_DISPLAYS() sprintf_s(num_buf, ARRAYSIZE(num_buf), "%u/%u", data->curr_file + 1, data->max_file); \
SetDlgItemTextA(hDlg, IDC_HISTORY_NUMBERS, (LPCSTR)num_buf); \
SetDlgItemTextA(hDlg, IDC_HISTORY_RSN, HISTORY_DLG_DEFAULT_RSN_TXT); \
SetDlgItemTextA(hDlg, IDC_HISTORY_AFFECTED, HISTORY_DLG_DEFAULT_AFFECTED_TXT)

INT_PTR CALLBACK HistoryDlg(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
	static CHAR url_buf[MAX_HOSTNAME_SIZE + MAX_USRPWD_SIZE * 2 + 96];
	static CHAR num_buf[24];
	switch (msg) {
	case WM_INITDIALOG:
		history_dlg = hDlg;
		InitCommonControlsEx(&DEFAULT_COMMON_CONTROLS);
		sprintf_s(url_buf, ARRAYSIZE(url_buf), "%s/place/?usr=%s&pwd=%s&ver=%s&action=enumerate", host_base, usr, pwd, PROGRAM_VER);
		http_response_t resp = send_http_get((LPCSTR)url_buf, DEFAULT_HTTP_TIMEOUT);
		if (resp.errored) {
		HISTORY_FAILED_ENUMERATION_ERR:
			MessageBoxA(main_wnd, "Client request to enumerate history failed to reach server's handler.", "History Enumeration Error", MB_OK | MB_ICONERROR);
			goto END_HISTORY_DLG;
		}
		if (!resp.serverok) {
			MessageBoxA(main_wnd, resp.response, "Server Rejected History Enumeration Request", MB_OK | MB_ICONERROR);
			FREE_IF_NONNULL(resp.response);
			goto END_HISTORY_DLG;
		}
		UINT n_hist;
		if (1 != sscanf_s(resp.response, "%u", &n_hist)) {
			MessageBoxA(main_wnd, "Server's response to client request to enumerate history files is misformatted!", "History Enumeration Error", MB_OK | MB_ICONERROR);
			FREE_IF_NONNULL(resp.response);
			goto END_HISTORY_DLG;
		};
		if (!n_hist) {
			MessageBoxA(main_wnd, "Server has no saved edit history!", "History Enumeration Error", MB_OK | MB_ICONERROR);
			FREE_IF_NONNULL(resp.response);
			goto END_HISTORY_DLG;
		}
		sprintf_s(num_buf, ARRAYSIZE(num_buf), "%u/%u", n_hist, n_hist);
		SetDlgItemTextA(hDlg, IDC_HISTORY_NUMBERS, num_buf);
		struct history_dlg_memory* data = (struct history_dlg_memory*)HeapAlloc(PROCESS_HEAP, 0x0, sizeof(struct history_dlg_memory));
		*data = (struct history_dlg_memory){
			.curr_file = --n_hist, //-1-indexed
			.max_file = n_hist
		};
		SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)data);
		FREE_IF_NONNULL(resp.response);
		struct history_file_out out = request_history_file(n_hist, url_buf, ARRAYSIZE(url_buf));
		if (!out.success) {
			return (INT_PTR)TRUE;
		}
		list_out_history_file(hDlg, IDC_HISTORY_LIST, &out);
		heapfree_recv_structure(out.n, HISTORY_FIELDS_PER_ENTRY, out.data);
		return (INT_PTR)TRUE;
		break;
	case WM_COMMAND:;
		if (HIWORD(wParam) == BN_CLICKED) switch (LOWORD(wParam)) {
		case IDC_HISTORY_OK:
			goto END_HISTORY_DLG;
		case IDC_HISTORY_CHECK_NEW:
			sprintf_s(url_buf, ARRAYSIZE(url_buf), "%s/place/?usr=%s&pwd=%s&ver=%s&action=enumerate", host_base, usr, pwd, PROGRAM_VER);
			http_response_t resp = send_http_get((LPCSTR)url_buf, DEFAULT_HTTP_TIMEOUT);
			if (resp.errored) goto HISTORY_FAILED_ENUMERATION_ERR;
			if (!resp.serverok) {
				MessageBoxA(main_wnd, resp.response, "Server Rejected History Enumeration Request", MB_OK | MB_ICONERROR);
				FREE_IF_NONNULL(resp.response);
				goto END_HISTORY_DLG;
			}
			UINT n_buf;
			data = (struct history_dlg_memory*)GetWindowLongPtr(hDlg, GWLP_USERDATA);
			if (1 == sscanf_s(resp.response, "%u", &n_buf)) {
				if (data->max_file == n_buf) {
					MessageBoxA(hDlg, "No new history files were found upon enumeration.", "Successful History Enumeration", MB_OK | MB_ICONASTERISK);
					return (INT_PTR)TRUE;
				}
				puts("");
				data->max_file = n_buf;
				data->curr_file = min(data->curr_file, data->max_file - 1);
				clear_out_history(hDlg, IDC_HISTORY_LIST);
				struct history_file_out out = request_history_file(data->curr_file, url_buf, ARRAYSIZE(url_buf));
				if (!out.success) goto HISTORY_DLG_FAILED_ENUMERATION_ERR;
				list_out_history_file(hDlg, IDC_HISTORY_LIST, &out);
				heapfree_recv_structure(out.n, HISTORY_FIELDS_PER_ENTRY, out.data);
				HISTORY_DLG_RESET_DISPLAYS();
				MessageBoxA(hDlg, "New history file found. List has been updated.", "Successful History Enumeration", MB_OK | MB_ICONASTERISK);
				break;
			}
			else {
			HISTORY_DLG_FAILED_ENUMERATION_ERR:
				MessageBoxA(hDlg, "Enumeration of history files failed.", "Unsuccessful History Enumeration", MB_OK | MB_ICONERROR);
				return (INT_PTR)TRUE;
			}
		case IDC_HISTORY_NEXT_BTN:
		case IDC_HISTORY_PREV_BTN:;
			struct history_dlg_memory* data = (struct history_dlg_memory*)GetWindowLongPtr(hDlg, GWLP_USERDATA);
			if (LOWORD(wParam) == IDC_HISTORY_PREV_BTN && data->curr_file == 0) {
				MessageBoxA(hDlg, "Already at earliest file", "Cannot switch page", MB_ICONEXCLAMATION | MB_OK);
				return (INT_PTR)TRUE;
			}
			else if (LOWORD(wParam) == IDC_HISTORY_NEXT_BTN && data->curr_file == data->max_file - 1) {
				MessageBoxA(hDlg, "Already at latest file", "Cannot switch page", MB_ICONEXCLAMATION | MB_OK);
				return (INT_PTR)TRUE;
			}
			if (LOWORD(wParam) == IDC_HISTORY_PREV_BTN) data->curr_file--;
			else data->curr_file++;
			struct history_file_out out = request_history_file(data->curr_file, url_buf, ARRAYSIZE(url_buf));
			if (!out.success) return (INT_PTR)TRUE;
			clear_out_history(hDlg, IDC_HISTORY_LIST);
			list_out_history_file(hDlg, IDC_HISTORY_LIST, &out);
			HISTORY_DLG_RESET_DISPLAYS();
			break;
		}
		else if (HIWORD(wParam) == LBN_SELCHANGE) {
			INT sel = (INT)SendDlgItemMessage(hDlg, IDC_HISTORY_LIST, LB_GETCURSEL, (WPARAM)NULL, (LPARAM)NULL);
			if (sel == LB_ERR) break;
			struct listed_history_item* item = (struct listed_history_item*)SendDlgItemMessage(hDlg, IDC_HISTORY_LIST, LB_GETITEMDATA, (WPARAM)sel, (LPARAM)NULL);
			CHAR sectors_buf[HISTORY_MAX_SECTORLIST_SIZE + 32];
			CHAR rsn_buf[HISTORY_MAX_REASON_LEN + 16];
			sprintf_s(sectors_buf, ARRAYSIZE(sectors_buf), "Affected sectors: %s", item->sectors);
			sprintf_s(rsn_buf, ARRAYSIZE(rsn_buf), "Reason: %s", item->reason);
			SetDlgItemTextA(hDlg, IDC_HISTORY_AFFECTED, sectors_buf);
			SetDlgItemTextA(hDlg, IDC_HISTORY_RSN, rsn_buf);
		}
		return (INT_PTR)TRUE;
	case WM_CLOSE:
	case WM_DESTROY:
	END_HISTORY_DLG:;
		LONG_PTR val;
		clear_out_history(hDlg, IDC_HISTORY_LIST);
		if ((LONG_PTR)NULL != (val = GetWindowLongPtr(hDlg, GWLP_USERDATA))) {
			HeapFree(PROCESS_HEAP, 0x0, (LPVOID)(struct history_dlg_memory*)val);
			SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)NULL);
		}
		history_dlg = NULL;
		END_CURRENT_DLG IDOK;
		return (INT_PTR)TRUE;
	}
	return (INT_PTR)FALSE;
}

INT_PTR CALLBACK ActivesDlg(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
	static CHAR name_buf[MAX_SECTORNAME_SIZE + 1];
	switch (msg) {
	case WM_INITDIALOG:
		actives_dlg = hDlg;
		SendDlgItemMessage(hDlg, IDC_ACTIVES_NEW, EM_LIMITTEXT, (WPARAM)MAX_SECTORNAME_SIZE, (LPARAM)NULL);
		if (actives && actives_count > 0)
			for (INT i = 0; i < actives_count; i++)
				SendDlgItemMessageA(hDlg, IDC_ACTIVES_LIST, LB_ADDSTRING, (WPARAM)NULL, (LPARAM)actives[i]);
		break;
	case WM_COMMAND:;
		INT sel = (INT)SendDlgItemMessage(hDlg, IDC_ACTIVES_LIST, LB_GETCURSEL, (WPARAM)NULL, (LPARAM)NULL);
		INT count = (INT)SendDlgItemMessage(hDlg, IDC_ACTIVES_LIST, LB_GETCOUNT, (WPARAM)NULL, (LPARAM)NULL);
		if (HIWORD(wParam) == BN_CLICKED) switch (LOWORD(wParam)) {
		case IDC_ACTIVES_ACTIVATE:
			GetDlgItemTextA(hDlg, IDC_ACTIVES_NEW, name_buf, ARRAYSIZE(name_buf));
			if (count == MAX_STORED_ACTIVES) {
				MessageBoxA(hDlg, "You have reached the maximum amount of actives allowed.", "Error Adding Quick Sector", MB_OK | MB_ICONEXCLAMATION);
				break;
			}
			if (!isalnum_plus_hyphen(name_buf)) {
				MessageBoxA(hDlg, "Your quick sector's name must be alphanumeric with +'s and -'s.", "Error Adding Quick Sector", MB_OK | MB_ICONEXCLAMATION);
				break;
			}
			SendDlgItemMessageA(hDlg, IDC_ACTIVES_LIST, LB_ADDSTRING, (WPARAM)NULL, (LPARAM)name_buf);
			break;
		case IDC_ACTIVES_DEACTIVATE:
			if (sel == LB_ERR) break;
			SendDlgItemMessage(hDlg, IDC_ACTIVES_LIST, LB_DELETESTRING, (WPARAM)sel, (LPARAM)NULL);
			break;
		case IDC_ACTIVES_UPLOAD:;
			if (sel == LB_ERR) break;
			LPSTR heap_buf = (LPSTR)HeapAlloc(PROCESS_HEAP, 0x0, (MAX_SECTORNAME_SIZE + 1) * sizeof(CHAR));
			SendDlgItemMessageA(hDlg, IDC_ACTIVES_LIST, LB_GETTEXT, (WPARAM)sel, (LPARAM)heap_buf);
			placersn_dlg_meta meta = {
				.calling_dlg = (HWND)NULL,
				.n_sectors = 1,
				.sector_names = &heap_buf
			};
			DialogBoxParam(main_instance, MAKEINTRESOURCE(IDD_PLACERSN), hDlg, &PlacementReasonDlg, (LPARAM)&meta);
			HeapFree(PROCESS_HEAP, 0x0, (LPVOID)heap_buf);
			break;
		case IDC_ACTIVES_GET:
			if (sel == LB_ERR) break;
			SendDlgItemMessageA(hDlg, IDC_ACTIVES_LIST, LB_GETTEXT, (WPARAM)sel, (LPARAM)name_buf);
			retrieve_from_server(name_buf);
			break;
		case IDC_ACTIVES_UPLOADALL:;
			if (count == 0) break;
			LPSTR* all_buf = (LPSTR*)HeapAlloc(PROCESS_HEAP, 0x0, sizeof(LPSTR) * count);
			for (INT i = 0; i < count; i++) {
				all_buf[i] = (LPSTR)HeapAlloc(PROCESS_HEAP, 0x0, sizeof(CHAR) * (MAX_SECTORNAME_SIZE + 1));
				SendDlgItemMessageA(hDlg, IDC_ACTIVES_LIST, LB_GETTEXT, (WPARAM)i, (LPARAM)all_buf[i]);
			}
			placersn_dlg_meta meta_all = {
				.calling_dlg = (HWND)NULL,
				.n_sectors = count,
				.sector_names = all_buf
			};
			DialogBoxParam(main_instance, MAKEINTRESOURCE(IDD_PLACERSN), hDlg, &PlacementReasonDlg, (LPARAM)&meta_all);
			for (INT i = 0; i < count; i++) HeapFree(PROCESS_HEAP, 0x0, (LPVOID)all_buf[i]);
			HeapFree(PROCESS_HEAP, 0x0, (LPVOID)all_buf);
			break;
		case IDC_ACTIVES_GETALL:
			if (count == 0) break;
			for (INT i = 0; i < count; i++) {
				SendDlgItemMessageA(hDlg, IDC_ACTIVES_LIST, LB_GETTEXT, (WPARAM)i, (LPARAM)name_buf);
				retrieve_from_server(name_buf);
			}
			break;
		case IDC_ACTIVES_OK:
			if (count == 0) {
				MessageBoxA(hDlg, "Did not save any quick sectors to persistent storage.", "Successful Actives Write", MB_OK | MB_ICONASTERISK);
				write_actives_to_storage((LPCSTR*)NULL, (INT)-1, TRUE);
			}
			else {
				LPSTR* save_buf = (LPSTR*)HeapAlloc(PROCESS_HEAP, 0x0, sizeof(LPSTR) * count);
				for (INT i = 0; i < count; i++) {
					save_buf[i] = (LPSTR)HeapAlloc(PROCESS_HEAP, 0x0, sizeof(CHAR) * (MAX_SECTORNAME_SIZE + 1));
					SendDlgItemMessageA(hDlg, IDC_ACTIVES_LIST, LB_GETTEXT, (WPARAM)i, (LPARAM)save_buf[i]);
				}
				write_actives_to_storage((LPCSTR*)save_buf, count, FALSE);
				for (INT i = 0; i < count; i++) HeapFree(PROCESS_HEAP, 0x0, (LPVOID)save_buf[i]);
				HeapFree(PROCESS_HEAP, 0x0, (LPVOID)save_buf);
				MessageBoxA(hDlg, "Quick sectors saved to persistent storage.", "Successful Actives Write", MB_OK | MB_ICONASTERISK);
			}
			goto END_ACTIVES_DLG;
		}
		return (INT_PTR)TRUE;
	case WM_CLOSE:
	case WM_DESTROY:
	END_ACTIVES_DLG:
		END_CURRENT_DLG IDOK;
		actives_dlg = NULL;
		return (INT_PTR)TRUE;
	}
	return (INT_PTR)FALSE;
};

//FALSE == end dialog; TRUE == good
BOOL _guard_dlg_update_in_use_sectors(HWND hDlg) {
	static CHAR url_buf[MAX_HOSTNAME_SIZE + 2 * MAX_USRPWD_SIZE + 96];
	sprintf_s(url_buf, ARRAYSIZE(url_buf), "%s/sector_guard/?usr=%s&pwd=%s&ver=%s", host_base, usr, pwd, PROGRAM_VER);
	http_response_t resp = send_http_get((LPCSTR)url_buf, DEFAULT_HTTP_TIMEOUT);
	if (resp.errored) {
		MessageBoxA(hDlg, "Client's request to request sector guard data failed to reach the server's handler", "Sector Guard Retrieval Error", MB_OK | MB_ICONERROR);
		return FALSE;
	}
	if (!resp.serverok) {
		MessageBoxA(hDlg, resp.response, "Server Rejected Request for Sector Guard Data Retrieval", MB_OK | MB_ICONERROR);
		FREE_IF_NONNULL(resp.response);
		return FALSE;
	}
	UINT n_sectors;
	if (1 != sscanf_s(resp.response, "%u", &n_sectors) || n_sectors > SECTOR_GUARD_MAX_SECTORS_PER_PROJECT) {
	GUARD_DLG_RETRIEVAL_MISFORMAT_ERR:
		MessageBoxA(hDlg, "Server Response to Sector Guard Retrieval is misformatted! Aborting to maintain program safety.", "Fatal Sector Guard Retrieval Error", MB_OK | MB_ICONERROR);
		exit(EXIT_FAILURE);
	}
	if (n_sectors) {
		LPSTR* sectors = (LPSTR*)HeapAlloc(PROCESS_HEAP, 0x0, sizeof(LPSTR) * n_sectors);
		LPSTR line, line_context = (LPSTR)NULL;
		if ((LPSTR)NULL == strtok_s(resp.response, "\n", &line_context)) goto GUARD_DLG_RETRIEVAL_MISFORMAT_ERR;
		//skip 1st line (n_sectors)
		if ((LPSTR)NULL == (line = strtok_s((LPSTR)NULL, "\n", &line_context))) goto GUARD_DLG_RETRIEVAL_MISFORMAT_ERR;
		for (UINT i = 0; i < n_sectors; i++) {
			sectors[i] = (LPSTR)HeapAlloc(PROCESS_HEAP, 0x0, sizeof(CHAR) * (MAX_SECTORNAME_SIZE + 1));
			if (1 != sscanf_s(line, "%s", sectors[i], (unsigned)(MAX_SECTORNAME_SIZE + 1))) goto GUARD_DLG_RETRIEVAL_MISFORMAT_ERR;
			if (i < n_sectors - 1) {
				if ((LPSTR)NULL == (line = strtok_s(NULL, "\n", &line_context))) goto GUARD_DLG_RETRIEVAL_MISFORMAT_ERR;
			}
		}
		UINT out_len = sizeof(CHAR) * (5 + n_sectors * (MAX_SECTORNAME_SIZE + 5));
		LPSTR out_str = (LPSTR)HeapAlloc(PROCESS_HEAP, 0x0, out_len);
		LPCHAR ptr = (LPCHAR)out_str;
		for (UINT i = 0; i < n_sectors; i++) {
			ptr += sprintf_s(ptr, out_str + out_len - ptr, "%s, ", sectors[i]);
			HeapFree(PROCESS_HEAP, 0x0, sectors[i]);
		}
		HeapFree(PROCESS_HEAP, 0x0, sectors);
		*(ptr - 2) = '\0';
		SetDlgItemTextA(hDlg, IDC_GUARD_SECTORS_DISP, (LPCSTR)out_str);
		HeapFree(PROCESS_HEAP, 0x0, out_str);
		FREE_IF_NONNULL(resp.response);
		return TRUE;
	}
	else {
		MessageBoxA(hDlg, "There are currently no sectors in use on the map.", "Server Sent 0 Sectors", MB_OK | MB_ICONASTERISK);
		FREE_IF_NONNULL(resp.response);
		return TRUE;
	}
}

#define GUARDDLG_VALIDATE_SECTOR_INPUT_INTO(buffer)                                                                                                                                          \
	CHAR buffer[MAX_SECTORNAME_SIZE + 1];                                                                                                                                                    \
	GetDlgItemTextA(hDlg, IDC_GUARD_ENTRY, (LPSTR)buffer, ARRAYSIZE(buffer));                                                                                                                \
	if (!isalnum_plus_hyphen((LPSTR)buffer) || strlen((LPCSTR)buffer) < 2) {                                                                                                                 \
		MessageBoxA(hDlg, "Your inputted sector must be alphanumeric (pluses and hyphens allowed) and 2+ characters.", "In-Use Sector List Modification Error", MB_OK | MB_ICONEXCLAMATION); \
		return (INT_PTR)TRUE;                                                                                                                                                                \
	}

INT_PTR CALLBACK GuardDlg(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
	static CHAR url_buf[MAX_HOSTNAME_SIZE + 2 * MAX_USRPWD_SIZE + 96];
	switch (msg) {
	case WM_INITDIALOG:
		SendDlgItemMessage(hDlg, IDC_GUARD_ENTRY, EM_LIMITTEXT, (WPARAM)MAX_SECTORNAME_SIZE, (LPARAM)NULL);
		if (!_guard_dlg_update_in_use_sectors(hDlg)) goto END_GUARD_DLG;
		break;
	case WM_COMMAND:
		if (HIWORD(wParam) == BN_CLICKED) switch (LOWORD(wParam)) {
		case IDC_GUARD_FORCE_UNLOCK:;
			GUARDDLG_VALIDATE_SECTOR_INPUT_INTO(buffer);
			if (IDNO == MessageBoxA(hDlg, "Doing this action consistently may be administrator misuse.\nIt bypasses normal procedure and can be a breach of trust.\nContinue?", "Continue with Forcible Unlocking?", MB_YESNO | MB_ICONEXCLAMATION)) break;
			if (TRUE == lock_sector((LPSTR)NULL, LOCK_SECTOR_ACTION_UNLOCK, (LPSTR)buffer)) {
				MessageBoxA(hDlg, "Sector was successfully forcibly unlocked.\nWill now refresh main window list...", "Successful Forcible Sector Unlocking", MB_OK | MB_ICONASTERISK);
				update_sector_locks_listbox(TRUE, GetDlgItem(main_wnd, IDC_LOCKS_LIST));
			};
			break;
		case IDC_GUARD_CANCEL:
			goto END_GUARD_DLG;
		case IDC_GUARD_ADD:
		case IDC_GUARD_DEL:
			if (LOWORD(wParam) == IDC_GUARD_ADD) sprintf_s(url_buf, ARRAYSIZE(url_buf), "%s/sector_guard/?usr=%s&pwd=%s&ver=%s&action=add", host_base, usr, pwd, PROGRAM_VER);
			else sprintf_s(url_buf, ARRAYSIZE(url_buf), "%s/sector_guard/?usr=%s&pwd=%s&ver=%s&action=del", host_base, usr, pwd, PROGRAM_VER);
			GUARDDLG_VALIDATE_SECTOR_INPUT_INTO(buf);
			http_response_t resp = send_http_post((LPCSTR)url_buf, (LPCSTR)buf, strlen((LPCSTR)buf), (LPCSTR)NULL, DEFAULT_HTTP_TIMEOUT);
			if (resp.errored) {
				MessageBoxA(hDlg, "Client request to modify the In-Use sector list failed to reach the server's handler.", "In-Use Sector List Modification Error", MB_OK | MB_ICONERROR);
				return (INT_PTR)TRUE;
			}
			if (!resp.serverok) {
				MessageBoxA(hDlg, resp.response, "Server Rejected In-Use Sector List Modification Request", MB_OK | MB_ICONERROR);
				FREE_IF_NONNULL(resp.response);
				return (INT_PTR)TRUE;
			}
			MessageBoxA(hDlg, "Successfully modified in-use sector list.\nWill now attempt to update the sector list on the main window...", "In-Use Sector List Modification Success", MB_OK | MB_ICONASTERISK);
			_guard_dlg_update_in_use_sectors(hDlg);
			return TRUE;
		}
		break;
	case WM_CLOSE:
	case WM_DESTROY:
	END_GUARD_DLG:
		END_CURRENT_DLG IDOK;
		actives_dlg = NULL;
		return (INT_PTR)TRUE;
		break;
	}
	return (INT_PTR)FALSE;
}
#endif //!defined(ATSS_DLG_H)
