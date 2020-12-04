#include "theater.h"
#include "app.h"
#include "dimmer.h"
#include "resource.h"

static constexpr wchar_t APP_WINDOWCLASS_NAME[] = L"TheaterWindow";
static constexpr wchar_t APP_WINDOW_NAME[] = L"TheaterWindow";

static HWND s_appWindowHandle = nullptr;
static bool s_theaterShown = false;
static UINT_PTR s_timerID = 0;
static std::chrono::high_resolution_clock::time_point s_timerStart;
static std::vector<HWND> s_topLevelWindows;
static HWINEVENTHOOK s_winEventHook = nullptr;
static std::unordered_set<std::wstring> s_processNameSet = {L"chrome", L"notepad"}; //tests


static BOOL CALLBACK App_EnumWindowsProc(_In_ HWND hwnd, _In_ LPARAM lParam)
{
	auto topLevelWindows = reinterpret_cast<std::vector<HWND>*>(lParam);
	if (::IsWindowVisible(hwnd))
		topLevelWindows->emplace_back(hwnd);
	return TRUE;
}

static void App_TheaterStart(HWND hwnd)
{
	const bool wasTheaterShown = s_theaterShown;
	s_theaterShown = true;

	if (!wasTheaterShown)
	{
		s_timerID = ::SetTimer(s_appWindowHandle, s_timerID, 16, nullptr);
		s_timerStart = std::chrono::high_resolution_clock::now();
		Dimmer_SetAlpha(0);
		Dimmer_Show(true);
	}

	s_topLevelWindows.clear();
	s_topLevelWindows.reserve(256);
	::EnumWindows(App_EnumWindowsProc, reinterpret_cast<LPARAM>(&s_topLevelWindows));

	HDWP dwp = ::BeginDeferWindowPos(static_cast<int>(s_topLevelWindows.size()));
	if (dwp == nullptr)
		return;

	for (auto topLevelWnd : s_topLevelWindows)
	{
		if (topLevelWnd == hwnd || Dimmer_IsDimmerWindow(topLevelWnd))
			continue;
		::DeferWindowPos(dwp, topLevelWnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
	}

	::EndDeferWindowPos(dwp);
}

static void App_TheaterStop()
{
	if (!s_theaterShown)
		return;

	Dimmer_Show(false);
	s_theaterShown = false;
}


static LRESULT CALLBACK App_MessageWindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_TIMER:
	{
		const auto currentTime = std::chrono::high_resolution_clock::now();
		const auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - s_timerStart).count();
		if (elapsedMs >= 500)
		{
			::KillTimer(hWnd, s_timerID);
			s_timerID = 0;
		}

		const float alpha = 0.8f * std::min(1.0f, elapsedMs / 500.0f);
		Dimmer_SetAlpha(alpha);
		break;
	}
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}

static bool App_MessageWindowCreate()
{
	const HINSTANCE hInstance = ::GetModuleHandleW(nullptr);

	WNDCLASSEXW wcex = {};
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.lpfnWndProc = App_MessageWindowProc;
	wcex.hInstance = hInstance;
	wcex.lpszClassName = APP_WINDOWCLASS_NAME;

	if (!RegisterClassExW(&wcex))
		return false;

	s_appWindowHandle = ::CreateWindowExW(0, APP_WINDOWCLASS_NAME, APP_WINDOW_NAME, 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, hInstance, NULL);
	if (s_appWindowHandle == nullptr)
		return false;

	return true;
}

static void App_MessageWindowDestroy()
{
	if(s_appWindowHandle != nullptr)
	{
		::DestroyWindow(s_appWindowHandle);
		s_appWindowHandle = nullptr;
	}
}

static void App_WinEventProc(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD idEventThread, DWORD dwmsEventTime)
{
	switch (event)
	{
	case EVENT_SYSTEM_FOREGROUND:
	{
		if (hwnd == nullptr)
			return;

		if (idObject != OBJID_WINDOW || idChild != CHILDID_SELF)
			return;

		DWORD wndProcessId = 0;
		::GetWindowThreadProcessId(hwnd, &wndProcessId);

		HANDLE processHandle = ::OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, wndProcessId);
		if (processHandle == nullptr)
			return;

		wchar_t processPath[_MAX_PATH];
		DWORD processPathLen = _MAX_PATH;
		auto queryResult = ::QueryFullProcessImageNameW(processHandle, 0, processPath, &processPathLen);
		processPath[_MAX_PATH - 1] = 0;
		::CloseHandle(processHandle);
		if (queryResult == 0)
			return;

		wchar_t filename[_MAX_FNAME];
		_wsplitpath_s(processPath, nullptr, 0, nullptr, 0, filename, _MAX_FNAME, nullptr, 0);
		filename[_MAX_FNAME - 1] = 0;
		std::transform(std::begin(filename), std::end(filename), std::begin(filename), [](wchar_t c) -> wchar_t { return std::tolower(c); });

		if (s_processNameSet.find(filename) != s_processNameSet.cend())
		{
			App_TheaterStart(hwnd);
		}
		else
		{
			App_TheaterStop();
		}

		break;
	}
	}
}

static bool App_HookRegister()
{
	auto result = ::CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	if (result != S_OK && result != S_FALSE)
		return false;

	s_winEventHook = ::SetWinEventHook(EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_FOREGROUND, nullptr, App_WinEventProc, 0, 0, WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS);
	return s_winEventHook != nullptr;
}

static void App_HookUnregister()
{
	if (s_winEventHook != nullptr)
	{
		::UnhookWinEvent(s_winEventHook);
		s_winEventHook = nullptr;
	}
	::CoUninitialize();
}

bool App_Init()
{
	if (!Tray_Init())
		return false;
	
	if (!App_MessageWindowCreate())
		return false;

	if (!Dimmer_Init())
		return false;

	if (!App_HookRegister())
		return false;

	return true;
}

int App_Run()
{
	MSG msg;
	while (::GetMessageW(&msg, nullptr, 0, 0))
	{
		::TranslateMessage(&msg);
		::DispatchMessageW(&msg);
	}

	return static_cast<int>(msg.wParam);
}

void App_Close()
{
	App_HookUnregister();
	App_MessageWindowDestroy();
	Dimmer_Close();
	Tray_Close();
}