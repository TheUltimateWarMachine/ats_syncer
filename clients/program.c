#define _CRT_SECURE_NO_WARNINGS
#define PROGRAM_VER "0.1" //Keep < 8 characters.
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "bcrypt.lib")
#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(linker, "/SUBSYSTEM:WINDOWS")
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\" ")
#include<Windows.h>
#include<windowsx.h>
#include<stdlib.h>
#include<stdio.h>
#include<wingdi.h>
#include<CommCtrl.h>
#include<complex.h>
#include<math.h>
#include<windef.h>
//Default Window Dimensions
#define DEFAULT_WINW 405
#define DEFAULT_WINH 360
typedef near PCHAR NPCHAR;
HANDLE PROCESS_HEAP;
HWND main_wnd = NULL; //main window, initialized in WinMain
//HMENU main_menu = NULL;
HINSTANCE main_instance = (HINSTANCE)NULL;
CONST INITCOMMONCONTROLSEX DEFAULT_COMMON_CONTROLS = { //Use this in dlgs
		.dwICC = ICC_STANDARD_CLASSES,
		.dwSize = sizeof(INITCOMMONCONTROLSEX)
};
#define CWM_SUCCESSFUL_SECTOR_PROCESSING (WM_USER + 1) //Informs Conflict Dialog handler that a sector was successfully processed (WPARAM)NULL; (LPARAM)(LPSTR)sector_name;
//Meta for the PlacementReasonDlg dialog
typedef struct placersn_dlg_meta {
	USHORT n_sectors; //Amount of sectors to send
	LPSTR* sector_names; //List of LPSTRs indicating sectors' names
	HWND calling_dlg;//If calling_dlg is nonnull, PlacementReasonDlg will send a CWM_SUCCESSFUL_SECTOR_PROCESSING to the calling_dlg
} placersn_dlg_meta;

#define ANNOUNCEMENT_CHECKER_TIMER_INTERVAL 60000 //How often to check for announcements
#define _OFFSET(n) (ID_OFFSET + n)
#define ID_OFFSET 1000
#define IDC_ANNOUNCEMENT_BTN _OFFSET(1)
#define IDC_CREDENTIALS_BTN _OFFSET(2)
#define IDC_DIR_BTN _OFFSET(3)
#define IDC_NEWANNOUNCEMENT_BTN _OFFSET(4)
#define IDC_ABT_BTN _OFFSET(5)
#define IDT_ANNOUNCEMENT_CHECKER _OFFSET(6)
#define IDC_SYNC_BTN _OFFSET(7)
#define IDC_LOCKS_STATIC _OFFSET(8)
#define IDC_LOCKS_LIST _OFFSET(9)
#define IDC_LOCKS_LIST_TIME _OFFSET(10)
#define IDC_LOCKS_LIST_RSN _OFFSET(11)
#define IDC_LOCKS_LIST_UNLOCK_BTN _OFFSET(12)
#define IDC_UPDATE_LOCKS_LIST_BTN _OFFSET(13)
#define IDC_NEW_LOCK_BTN _OFFSET(14)
#define IDC_HISTORY_BTN _OFFSET(15)
#define IDC_ACTIVES_BTN _OFFSET(16)
#define IDC_GUARD_BTN _OFFSET(17)
#define FREE_IF_NONNULL(obj) if((LPVOID)obj != NULL) free(obj); //requests.h uses reg frees instead of the Windows Memory manager's
#include"Resource.h"
#include"requests.h"
#include"store_announce.h"
#include"synchronize.h"
#include"get_place.h"
#include"dlg.h"

//Responsible for setting the right font on all windows in WndProc WM_CREATE
BOOL CALLBACK SetFontsProc(HWND hwnd, LPARAM lphFont) {
	SendMessageW(hwnd, WM_SETFONT, (WPARAM)lphFont, (LPARAM)TRUE);
	return TRUE;
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	RECT rc;
	switch (msg) {
	case WM_MOUSEMOVE:
		SetCursor(LoadCursor(NULL, IDC_ARROW));
		break;
	case WM_PAINT:;
		PAINTSTRUCT ps;
		GetClientRect(hwnd, &rc);
		HDC hdc = BeginPaint(hwnd, &ps);
		FillRect(hdc, &(RECT){0, 0, DEFAULT_WINW, DEFAULT_WINH}, (HBRUSH)(COLOR_WINDOW + 1));
		EndPaint(hwnd, &ps);
		return FALSE;
	case WM_TIMER:
		if ((UINT)wParam == IDT_ANNOUNCEMENT_CHECKER) check_for_announcements(FALSE);
		break;
	case WM_CREATE:;
		switch (initialize_persistent_settings()) {
		case -1:
			MessageBoxA(hwnd, "Misformatted file or IO Error.", "Failed to Initialize Persistent Settings", MB_OK | MB_ICONERROR);
			exit(EXIT_FAILURE);
		case 0:
			SetTimer(hwnd, IDT_ANNOUNCEMENT_CHECKER, (UINT)ANNOUNCEMENT_CHECKER_TIMER_INTERVAL, (TIMERPROC)NULL);
			MessageBoxA(hwnd, "Welcome to the ATS Syncer V." PROGRAM_VER ". To complete setup, you must open the directory picker and press Ok. Then enter your credentials in the credential modifier.\nOptionally, you can then restart the program to get all listeners active instead of having them disabled. Word of warning:\n-----\nDo not connect to untrusted hosts under ANY circumstances.\nAlso, this program will not function if your username contains non UTF-8 characters.\nDO NOT DISTRIBUTE THIS PROGRAM UNLESS GRANTED PERMISSION!\n-----\nEnjoy =)", "First Run Warnings", MB_OK);
			
			break;
		case 1:
			SetTimer(hwnd, IDT_ANNOUNCEMENT_CHECKER, (UINT)ANNOUNCEMENT_CHECKER_TIMER_INTERVAL, (TIMERPROC)NULL);
			check_for_announcements(TRUE);
			break;
		default:
			break;
		}
		GetClientRect(hwnd, &rc);
		HWND announcements_btn = CreateWindow(TEXT("BUTTON"), TEXT("Check for Announcements"), WS_CHILDWINDOW | WS_VISIBLE | BS_PUSHBUTTON, 5, 5, 180, 24, hwnd, (HMENU)IDC_ANNOUNCEMENT_BTN, main_instance, (LPARAM)NULL);
		HWND credentials_btn = CreateWindow(TEXT("BUTTON"), TEXT("Credentials"), WS_CHILDWINDOW | WS_VISIBLE | BS_PUSHBUTTON, 185, 5, 100, 24, hwnd, (HMENU)IDC_CREDENTIALS_BTN, main_instance, (LPARAM)NULL);
		HWND dir_btn = CreateWindow(TEXT("BUTTON"), TEXT("Directory"), WS_CHILDWINDOW | WS_VISIBLE | BS_PUSHBUTTON, 285, 5, 100, 24, hwnd, (HMENU)IDC_DIR_BTN, main_instance, (LPARAM)NULL);
		HWND new_announcement_btn = CreateWindow(TEXT("BUTTON"), TEXT("Announce"), WS_CHILDWINDOW | WS_VISIBLE | BS_PUSHBUTTON, 5, 29, 100, 24, hwnd, (HMENU)IDC_NEWANNOUNCEMENT_BTN, main_instance, (LPARAM)NULL);
		HWND abt_btn = CreateWindow(TEXT("BUTTON"), TEXT("About"), WS_CHILDWINDOW | WS_VISIBLE | BS_PUSHBUTTON, 105, 29, 60, 24, hwnd, (HMENU)IDC_ABT_BTN, main_instance, (LPARAM)NULL);
		HWND sync_btn = CreateWindow(TEXT("BUTTON"), TEXT("Full Sync"), WS_CHILDWINDOW | WS_VISIBLE | BS_PUSHBUTTON, 165, 29, 60, 24, hwnd, (HMENU)IDC_SYNC_BTN, main_instance, (LPARAM)NULL);
		HWND locks_static = CreateWindow(TEXT("STATIC"), TEXT("Current Sector Locks"), WS_CHILDWINDOW | WS_VISIBLE | SS_LEFT, 5, 110, 140, 24, hwnd, (HMENU)IDC_LOCKS_STATIC, main_instance, (LPARAM)NULL);
		HWND locks_list = CreateWindowExW(WS_EX_LEFT | WS_EX_LTRREADING | WS_EX_RIGHTSCROLLBAR | WS_EX_DLGMODALFRAME, TEXT("LISTBOX"), (LP)NULL, WS_CHILDWINDOW | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY, 5, 132, 300, 150, hwnd, (HMENU)IDC_LOCKS_LIST, main_instance, (LPARAM)NULL);
		HWND locks_list_rsn_disp = CreateWindow(TEXT("STATIC"), TEXT(""), WS_CHILDWINDOW | WS_VISIBLE | SS_LEFT, 5, 280, DEFAULT_WINW - 5, 32, hwnd, (HMENU)IDC_LOCKS_LIST_RSN, main_instance, (LPARAM)NULL);
		HWND locks_list_time_disp = CreateWindow(TEXT("STATIC"), TEXT(""), WS_CHILDWINDOW | WS_VISIBLE | SS_LEFT, 5, 317, 300, 32, hwnd, (HMENU)IDC_LOCKS_LIST_TIME, main_instance, (LPARAM)NULL);
		HWND locks_list_unlock_btn = CreateWindow(TEXT("BUTTON"), TEXT("Unlock"), WS_CHILDWINDOW | BS_PUSHBUTTON, 310, 132, 70, 24, hwnd, (HMENU)IDC_LOCKS_LIST_UNLOCK_BTN, main_instance, (LPARAM)NULL);
		HWND update_locks_list_btn = CreateWindow(TEXT("BUTTON"), TEXT("Update Locks"), WS_CHILDWINDOW | WS_VISIBLE | BS_PUSHBUTTON, 225, 29, 90, 24, hwnd, (HMENU)IDC_UPDATE_LOCKS_LIST_BTN, main_instance, (LPARAM)NULL);
		HWND new_server_lock_btn = CreateWindow(TEXT("BUTTON"), TEXT("New Sector Lock"), WS_CHILDWINDOW | WS_VISIBLE | BS_PUSHBUTTON, 150, 106, 100, 24, hwnd, (HMENU)IDC_NEW_LOCK_BTN, main_instance, (LPARAM)NULL);
		HWND history_btn = CreateWindow(TEXT("BUTTON"), TEXT("History"), WS_CHILDWINDOW | WS_VISIBLE | BS_PUSHBUTTON, 315, 29, 70, 24, hwnd, (HMENU)IDC_HISTORY_BTN, main_instance, (LPARAM)NULL);
		HWND actives_btn = CreateWindow(TEXT("BUTTON"), TEXT("Quick Sectors"), WS_CHILDWINDOW | WS_VISIBLE | BS_PUSHBUTTON, 5, 53, 110, 24, hwnd, (HMENU)IDC_ACTIVES_BTN, main_instance, (LPARAM)NULL);
		HWND sector_guard_btn = CreateWindow(TEXT("BUTTON"), TEXT("Sector Guard (Administrators)"), WS_CHILDWINDOW | WS_VISIBLE | BS_PUSHBUTTON, 115, 53, 180, 24, hwnd, (HMENU)IDC_GUARD_BTN, main_instance, (LPARAM)NULL);
		update_sector_locks_listbox(FALSE, locks_list);
		NONCLIENTMETRICS ncm = { sizeof(ncm) };
		SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);
		EnumChildWindows(hwnd, &SetFontsProc, (LPARAM)CreateFontIndirect(&ncm.lfMessageFont));
		return FALSE;
	case WM_COMMAND:
		switch (HIWORD(wParam)) {
		case BN_CLICKED:
			switch (LOWORD(wParam)) {
			case IDC_DIR_BTN:
				DialogBox(main_instance, MAKEINTRESOURCE(IDD_FOLDERER), hwnd, &FoldererDlgProc);
				break;
			case IDC_ANNOUNCEMENT_BTN:
				check_for_announcements(TRUE);
				break;
			case IDC_CREDENTIALS_BTN:
				DialogBox(main_instance, MAKEINTRESOURCE(IDD_CREDENTIALS), hwnd, &CredentialsDlgProc);
				break;
			case IDC_NEWANNOUNCEMENT_BTN:
				DialogBox(main_instance, MAKEINTRESOURCE(IDD_NEW_ANNOUNCEMENT), hwnd, &NewAnnouncementDlgProc);
				break;
			case IDC_ABT_BTN:
				DialogBox(main_instance, MAKEINTRESOURCE(IDD_ABT), hwnd, &AbtDlgProc);
				break;
			case IDC_SYNC_BTN:
				sync_with_server();
				break;
			case IDC_UPDATE_LOCKS_LIST_BTN:
				update_sector_locks_listbox(TRUE, GetDlgItem(hwnd, IDC_LOCKS_LIST));
				break;
			case IDC_LOCKS_LIST_UNLOCK_BTN:;
				INT sel = (INT)SendDlgItemMessage(hwnd, IDC_LOCKS_LIST, LB_GETCURSEL, (WPARAM)NULL, (LPARAM)NULL);
				if (sel == LB_ERR) break;
				LPSTR sector_name = ((struct listed_sector_lock*)SendDlgItemMessage(hwnd, IDC_LOCKS_LIST, LB_GETITEMDATA, (WPARAM)sel, (LPARAM)NULL))->sector;
				CHAR buf[128];
				if (lock_sector((LPSTR)NULL, LOCK_SECTOR_ACTION_UNLOCK, sector_name) != TRUE) {
					sprintf_s(buf, ARRAYSIZE(buf), "Failed to unlock sector %s.", sector_name);
					MessageBoxA(hwnd, buf, "Detected Sector Unlocking Error", MB_OK | MB_ICONERROR);
					break;
				}
				sprintf_s(buf, ARRAYSIZE(buf), "Sector %s has been unlocked successfully.\nWill now refresh the sector locks list...", sector_name);
				MessageBoxA(hwnd, buf, "Sucessful Sector Unlocking", MB_OK | MB_ICONASTERISK);
				update_sector_locks_listbox(TRUE, GetDlgItem(hwnd, IDC_LOCKS_LIST));
				break;
			case IDC_NEW_LOCK_BTN:;
				DialogBox(main_instance, MAKEINTRESOURCE(IDD_LOCKRSN), hwnd, &LockRsnDlg);
				break;
			case IDC_HISTORY_BTN:
				DialogBox(main_instance, MAKEINTRESOURCE(IDD_HISTORY), hwnd, &HistoryDlg);
				break;
			case IDC_ACTIVES_BTN:
				DialogBox(main_instance, MAKEINTRESOURCE(IDD_ACTIVES), hwnd, &ActivesDlg);
				break;
			case IDC_GUARD_BTN:
				DialogBox(main_instance, MAKEINTRESOURCE(IDD_GUARD), hwnd, &GuardDlg);
				break;
			}
			break;
		case LBN_SELCHANGE:
			switch (LOWORD(wParam)) {
			case IDC_LOCKS_LIST:;
				INT sel = (INT)SendDlgItemMessage(hwnd, IDC_LOCKS_LIST, LB_GETCURSEL, (WPARAM)NULL, (LPARAM)NULL);
				if (sel == LB_ERR) break;
				struct listed_sector_lock* data = (struct listed_sector_lock*)SendDlgItemMessageW(hwnd, IDC_LOCKS_LIST, LB_GETITEMDATA, (WPARAM)sel, (LPARAM)NULL);
				CHAR time_buf[MAX_TIMESTAMP_LEN + 16];
				CHAR rsn_buf[MAX_LOCK_REASON_LEN + 16];
				sprintf_s(time_buf, ARRAYSIZE(time_buf), "Lock Created: %s", data->timestamp);
				sprintf_s(rsn_buf, ARRAYSIZE(rsn_buf), "Lock Reason: %s", data->reason);
;				SetDlgItemTextA(hwnd, IDC_LOCKS_LIST_TIME, (LPCSTR)time_buf);
				SetDlgItemTextA(hwnd, IDC_LOCKS_LIST_RSN, (LPCSTR)rsn_buf);
				ShowWindow(GetDlgItem(hwnd, IDC_LOCKS_LIST_UNLOCK_BTN), strcmp(data->author, usr) ? SW_HIDE : SW_SHOW);
				break;
			}
			break;
		}
		break;
	case WM_DESTROY:; //The window has been closed.
		PostQuitMessage(EXIT_SUCCESS);
		exit(EXIT_SUCCESS);
		break;
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

INT WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ INT nCmdShow) {
	PROCESS_HEAP = GetProcessHeap();
	UNREFERENCED_PARAMETER(lpCmdLine);
	UNREFERENCED_PARAMETER(hPrevInstance);
	main_instance = hInstance;
	//if (NULL == (main_menu = LoadMenuW(hInstance, MAKEINTRESOURCEW(IDM_MAIN)))) {
	//	fprintf(stderr, "Failed to register the main window menu with the kernel.");
	//	exit(EXIT_FAILURE);
	//}
	InitCommonControlsEx(&DEFAULT_COMMON_CONTROLS);
#ifdef _DEBUG
		AllocConsole();
		// Redirect stdout to the new console
		(VOID)freopen("CONOUT$", "w", stdout);
		(VOID)freopen("CONOUT$", "w", stderr);
		(VOID)freopen("CONIN$", "r", stdin);
#endif
	HICON PROGRAM_ICON = (HICON)LoadImageW(main_instance, MAKEINTRESOURCEW(IDI_ICO64), IMAGE_ICON, 0, 0, LR_COPYFROMRESOURCE);
	WNDCLASSEX wcex = {
		.cbSize = sizeof(WNDCLASSEX),
		.hInstance = main_instance,
		.style = CS_VREDRAW | CS_HREDRAW, //Trigger a full window redraw on size change
		.lpszClassName = TEXT("ATSSyncer"),
		.lpfnWndProc = &WndProc,
		.cbClsExtra = 0,
		.cbWndExtra = 0,
		.hCursor = LoadCursorW((HINSTANCE)NULL, IDC_ARROW),
		.hIcon = PROGRAM_ICON,
		.hIconSm = PROGRAM_ICON
	};
	if (!RegisterClassExW(&wcex)) {
		fprintf(stderr, "The Kernel failed to register the main window class.");
		exit(EXIT_FAILURE);
	}
	RECT window_bounds = { 0, 0, DEFAULT_WINW, DEFAULT_WINH };
	DWORD window_style = WS_OVERLAPPED | WS_MINIMIZEBOX | WS_SYSMENU;
	AdjustWindowRect(&window_bounds, window_style, TRUE); //the window bounds must be adjusted since a menu bar is present
	main_wnd = CreateWindowW(
		wcex.lpszClassName, TEXT("ATS Syncer"), window_style,
		CW_USEDEFAULT, CW_USEDEFAULT, window_bounds.right - window_bounds.left, window_bounds.bottom - window_bounds.top,
		(HWND)NULL, (HMENU)NULL, hInstance, (LPARAM)NULL
	);
	ShowWindow(main_wnd, nCmdShow);
	SetForegroundWindow(main_wnd);
	MSG msg;
	BOOL ret;
	//Msg loop
	while (ret = GetMessageW(&msg, NULL, (UINT)0, (UINT)0)) {
		if (FAILED(ret)) {
			fprintf(stderr, "Failed to process message %u in main message loop (got status %d)", msg.message, ret);
			exit(EXIT_FAILURE);
		}
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}
