#include "theater.h"
#include "dimmer.h"
#include "resource.h"

static constexpr wchar_t DIMMER_WINDOWCLASS_NAME[] = L"TheaterDimmerWindow";
static constexpr wchar_t DIMMER_WINDOW_NAME[] = L"TheaterDimmerWindow";

struct MonitorInstance
{
	HMONITOR handle;
	RECT rc;
	HWND hwnd;
};

static std::vector<MonitorInstance> s_monitors;
static COLORREF s_clearColor = RGB(0, 0, 50);

static BOOL Dimmer_EnumMonitorsProc(HMONITOR handle, HDC dc, LPRECT rc, LPARAM lParam)
{
	MonitorInstance monitor = {};
	monitor.handle = handle;
	monitor.rc = *rc;
	s_monitors.emplace_back(std::move(monitor));
	return TRUE;
}

static LRESULT CALLBACK Dimmer_WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC dc = ::BeginPaint(hWnd, &ps);

		const COLORREF oldDCBrushColor = ::SetDCBrushColor(dc, s_clearColor);
		::FillRect(dc, &ps.rcPaint, static_cast<HBRUSH>(::GetStockObject(DC_BRUSH)));
		::SetDCBrushColor(dc, oldDCBrushColor);

		::EndPaint(hWnd, &ps);

		return 0;
	}
	case WM_ERASEBKGND:
	{
		return TRUE;
	}
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}

static bool Dimmer_WindowsCreate()
{
	//enum all monitors
	if (!::EnumDisplayMonitors(nullptr, nullptr, Dimmer_EnumMonitorsProc, NULL))
		return false;

	const HINSTANCE hInstance = ::GetModuleHandleW(nullptr);

	//register our window class
	WNDCLASSEXW wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = Dimmer_WindowProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = ::LoadIcon(hInstance, MAKEINTRESOURCE(IDI_THEATER));
	wcex.hCursor = ::LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = nullptr; // static_cast<HBRUSH>(::GetStockObject(BLACK_BRUSH));
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = DIMMER_WINDOWCLASS_NAME;
	wcex.hIconSm = nullptr;

	if (!RegisterClassExW(&wcex))
		return false;

	//for each monitor, create a window overlapping the entire region
	for (auto& monitor : s_monitors)
	{
		const DWORD exStyle = WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE;
		const DWORD style = WS_POPUP;
		monitor.hwnd = ::CreateWindowExW(exStyle, DIMMER_WINDOWCLASS_NAME, DIMMER_WINDOW_NAME, style,
			monitor.rc.left, monitor.rc.top, monitor.rc.right - monitor.rc.left, monitor.rc.bottom - monitor.rc.top,
			nullptr, nullptr, hInstance, nullptr);
		::SetLayeredWindowAttributes(monitor.hwnd, 0, 0, LWA_ALPHA);
	}

	return true;
}

static void Dimmer_WindowsDestroy()
{
	for (auto& monitor : s_monitors)
		::DestroyWindow(monitor.hwnd);

	s_monitors.clear();
}


bool Dimmer_Init()
{
	return Dimmer_WindowsCreate();
}

void Dimmer_Show(bool state)
{
	for (const auto& monitor : s_monitors)
		::ShowWindow(monitor.hwnd, state ? SW_SHOWNOACTIVATE : SW_HIDE);
}

void Dimmer_SetAlpha(float alpha)
{
	const BYTE alpha256 = static_cast<BYTE>(std::min(static_cast<DWORD>(255), static_cast<DWORD>(alpha * 255.0f + 0.5f)));
	for (const auto& monitor : s_monitors)
		::SetLayeredWindowAttributes(monitor.hwnd, 0, alpha256, LWA_ALPHA);
}

void Dimmer_SetColor(float r, float g, float b)
{
	const BYTE r256 = static_cast<BYTE>(std::min(static_cast<DWORD>(255), static_cast<DWORD>(r * 255.0f + 0.5f)));
	const BYTE g256 = static_cast<BYTE>(std::min(static_cast<DWORD>(255), static_cast<DWORD>(g * 255.0f + 0.5f)));
	const BYTE b256 = static_cast<BYTE>(std::min(static_cast<DWORD>(255), static_cast<DWORD>(b * 255.0f + 0.5f)));

	s_clearColor = RGB(r256, g256, b256);

	for (const auto& monitor : s_monitors)
	{
		if (!::IsWindowVisible(monitor.hwnd))
			continue;
		
		::InvalidateRect(monitor.hwnd, nullptr, FALSE);
	}
}

void Dimmer_Close()
{
	Dimmer_WindowsDestroy();
}

bool Dimmer_IsDimmerWindow(HWND hwnd)
{
	for (const auto& monitor : s_monitors)
	{
		if (hwnd == monitor.hwnd)
			return true;
	}

	return false;
}