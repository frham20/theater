#include "theater.h"
#include "tray.h"
#include "resource.h"

static constexpr wchar_t TRAY_WINDOWCLASS_NAME[] = L"TheaterTrayWindow";
static constexpr wchar_t TRAY_WINDOW_NAME[]      = L"TheaterTrayWindow";
static constexpr UINT TRAY_WM_NOTIFICATION       = WM_USER + 1;

// We won't be using a GUID for the tray icon as suggested by MSFT for Windows 7 and +
// Whenever the user changes the install directory this will break!
// This also means that we need 2 different GUIDs, one for debug exec and one for release exec
// WTH Windows...
// Source: https://stackoverflow.com/questions/7432319/notifyicondata-guid-problem
// Source: https://docs.microsoft.com/en-us/windows/win32/api/shellapi/ns-shellapi-notifyicondataa
//static constexpr GUID TRAY_GUID                  = { 0x59cdfa40, 0xdb33, 0x44fd, 0x99, 0x24, 0x95, 0x13, 0x6c, 0x3e, 0xef, 0x2b };
static constexpr UINT TRAY_ICON_ID = 1;

static HMENU s_trayMenu         = nullptr;
static HMENU s_trayContextMenu  = nullptr;
static HWND  s_trayWindowHandle = nullptr;
static HICON s_trayIcon = nullptr;

static LRESULT CALLBACK Tray_WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_COMMAND:
	{
		switch (LOWORD(wParam))
		{
		case ID_TRAY_CONTEXT_ABOUT:
		{
			//DialogBoxW(::GetModuleHandleW(nullptr), MAKEINTRESOURCE(IDD_ABOUTBOX), HWND_DESKTOP, About);
			return 0;
		}
		case ID_TRAY_CONTEXT_EXIT:
		{
			::PostQuitMessage(0);
			return 0;
		}
		}
		break;
	}
	case TRAY_WM_NOTIFICATION:
	{
		switch (LOWORD(lParam))
		{
		case WM_CONTEXTMENU:
		{
			if (s_trayMenu == nullptr)
			{
				s_trayMenu = ::LoadMenuW(::GetModuleHandleW(nullptr), MAKEINTRESOURCEW(IDM_TRAY_CONTEXT));
				s_trayContextMenu = ::GetSubMenu(s_trayMenu, 0);
			}

			//needed to ensure context menu gets closed if a use clicks elsewhere when it is open
			::SetForegroundWindow(s_trayWindowHandle);

			UINT flags = TPM_BOTTOMALIGN;
			flags |= ::GetSystemMetrics(SM_MENUDROPALIGNMENT) != 0 ? TPM_RIGHTALIGN : TPM_LEFTALIGN;

			const int mouseX = GET_X_LPARAM(wParam);
			const int mouseY = GET_Y_LPARAM(wParam);
			::TrackPopupMenuEx(s_trayContextMenu, flags, mouseX, mouseY, s_trayWindowHandle, nullptr);
			break;
		}
		}
		break;
	}
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}

static bool Tray_MessageWindowCreate()
{
	const HINSTANCE hInstance = ::GetModuleHandleW(nullptr);

	WNDCLASSEXW wcex = {};
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.lpfnWndProc = Tray_WndProc;
	wcex.hInstance = hInstance;
	wcex.lpszClassName = TRAY_WINDOWCLASS_NAME;

	if (!RegisterClassExW(&wcex))
		return false;

	s_trayWindowHandle = ::CreateWindowExW(0, TRAY_WINDOWCLASS_NAME, TRAY_WINDOW_NAME, 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, nullptr, nullptr);
	if (s_trayWindowHandle == nullptr)
		return false;

	return true;
}

static void Tray_MessageWindowDestroy()
{
	//destroy context menu
	if (s_trayMenu != nullptr)
	{
		::DestroyMenu(s_trayMenu);
		s_trayMenu = nullptr;
		s_trayContextMenu = nullptr;
	}

	//destroy message window
	if (s_trayWindowHandle != nullptr)
	{
		::DestroyWindow(s_trayWindowHandle);
		s_trayWindowHandle = nullptr;
	}
}

static bool Tray_NotificationsUnregister()
{
	NOTIFYICONDATAW nid = {};
	nid.cbSize = sizeof(NOTIFYICONDATAW);
	nid.uFlags = 0;
	nid.hWnd = s_trayWindowHandle;
	nid.uID = TRAY_ICON_ID;
	nid.hIcon = s_trayIcon;

	return Shell_NotifyIconW(NIM_DELETE, &nid);
}

static bool Tray_NotificationsRegister()
{
	const HINSTANCE hInstance = ::GetModuleHandleW(nullptr);

	s_trayIcon = ::LoadIcon(hInstance, MAKEINTRESOURCE(IDI_THEATER));

	NOTIFYICONDATAW nid = {};
	nid.cbSize = sizeof(NOTIFYICONDATAW);
	nid.uFlags = NIF_ICON | NIF_MESSAGE;
	nid.hWnd = s_trayWindowHandle;
	nid.uID = TRAY_ICON_ID;
	nid.hIcon = s_trayIcon;
	nid.uCallbackMessage = TRAY_WM_NOTIFICATION;
	nid.uVersion = NOTIFYICON_VERSION_4;

	if (!Shell_NotifyIconW(NIM_ADD, &nid))
	{
		//didn't work? try to delete a previously stuck instance
		if (!Tray_NotificationsUnregister())
			return false;

		//try to register again
		if (!Shell_NotifyIconW(NIM_ADD, &nid))
			return false;
	}

	if (!Shell_NotifyIconW(NIM_SETVERSION, &nid))
		return false;

	return true;
}

bool Tray_Init()
{
	if (!Tray_MessageWindowCreate())
		return false;

	if (!Tray_NotificationsRegister())
		return false;

	return true;
}

void Tray_Close()
{
	Tray_NotificationsUnregister();
	Tray_MessageWindowDestroy();
}

#pragma optimize( "", on )