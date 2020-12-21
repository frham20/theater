#include "theater.h"
#include "app.h"
#include "dimmer.h"
#include "resource.h"

namespace Theater
{
	namespace
	{
		constexpr wchar_t APP_WINDOWCLASS_NAME[] = L"TheaterWindow";
		constexpr wchar_t APP_WINDOW_NAME[]      = L"TheaterWindow";

		BOOL CALLBACK EnumWindowsProc( _In_ HWND hwnd, _In_ LPARAM lParam )
		{
			auto topLevelWindows = reinterpret_cast<std::vector<HWND>*>( lParam );
			if ( ::IsWindowVisible( hwnd ) )
				topLevelWindows->emplace_back( hwnd );
			return TRUE;
		}

		App s_app;
	} // namespace

	App& App::Current()
	{
		return s_app;
	}

	Settings& App::GetSettings()
	{
		return this->settings;
	}

	const Settings& App::GetSettings() const
	{
		return this->settings;
	}

	void App::TheaterStart( HWND hwnd )
	{
		const bool wasTheaterShown = this->theaterShown;
		this->theaterShown         = true;

		if ( !wasTheaterShown )
		{
			this->timerID    = ::SetTimer( this->messageWindow, this->timerID, 16, nullptr );
			this->timerStart = std::chrono::high_resolution_clock::now();
			this->dimmer.SetAlpha( 0 );
			this->dimmer.Show( true );
		}

		this->topLevelWindows.clear();
		this->topLevelWindows.reserve( 256 );
		::EnumWindows( EnumWindowsProc, reinterpret_cast<LPARAM>( &this->topLevelWindows ) );

		HDWP dwp = ::BeginDeferWindowPos( static_cast<int>( this->topLevelWindows.size() ) );
		if ( dwp == nullptr )
			return;

		for ( auto topLevelWnd : this->topLevelWindows )
		{
			if ( topLevelWnd == hwnd || this->dimmer.IsDimmerWindow( topLevelWnd ) )
				continue;
			::DeferWindowPos( dwp, topLevelWnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE );
		}

		::EndDeferWindowPos( dwp );
	}

	void App::TheaterStop()
	{
		if ( !this->theaterShown )
			return;

		this->dimmer.Show( false );
		this->theaterShown = false;
	}

	LRESULT App::OnMessage( UINT message, WPARAM wParam, LPARAM lParam )
	{
		switch ( message )
		{
		case WM_TIMER: {
			const auto currentTime = std::chrono::high_resolution_clock::now();
			const auto elapsedMs =
			    std::chrono::duration_cast<std::chrono::milliseconds>( currentTime - this->timerStart ).count();
			if ( elapsedMs >= 500 )
			{
				::KillTimer( this->messageWindow, this->timerID );
				this->timerID = 0;
			}

			const float alpha = ( this->settings.GetAlpha() / 255.0f ) * std::min( 1.0f, elapsedMs / 500.0f );
			this->dimmer.SetAlpha( alpha );
			break;
		}
		}

		return ::DefWindowProc( this->messageWindow, message, wParam, lParam );
	}

	LRESULT CALLBACK App::MessageWndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
	{
		auto app = reinterpret_cast<App*>( ::GetWindowLongPtrW( hWnd, GWLP_USERDATA ) );
		if ( app == nullptr )
			return ::DefWindowProc( hWnd, message, wParam, lParam );

		return app->OnMessage( message, wParam, lParam );
	}

	bool App::MessageWindowCreate()
	{
		const HINSTANCE hInstance = ::GetModuleHandleW( nullptr );

		WNDCLASSEXW wcex   = {};
		wcex.cbSize        = sizeof( WNDCLASSEX );
		wcex.lpfnWndProc   = App::MessageWndProc;
		wcex.hInstance     = hInstance;
		wcex.lpszClassName = APP_WINDOWCLASS_NAME;

		if ( !RegisterClassExW( &wcex ) )
			return false;

		this->messageWindow = ::CreateWindowExW( 0, APP_WINDOWCLASS_NAME, APP_WINDOW_NAME, 0, 0, 0, 0, 0, HWND_MESSAGE,
		                                         nullptr, hInstance, NULL );
		if ( this->messageWindow == nullptr )
			return false;

		::SetWindowLongPtrW( this->messageWindow, GWLP_USERDATA, reinterpret_cast<LONG_PTR>( this ) );
		return true;
	}

	void App::MessageWindowDestroy()
	{
		if ( this->messageWindow != nullptr )
		{
			::DestroyWindow( this->messageWindow );
			this->messageWindow = nullptr;
		}
	}

	void App::OnWinEvent( HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd, LONG idObject, LONG idChild,
	                      DWORD idEventThread, DWORD dwmsEventTime )
	{
		UNREFERENCED_PARAMETER( hWinEventHook );
		UNREFERENCED_PARAMETER( dwmsEventTime );
		UNREFERENCED_PARAMETER( idEventThread );

		switch ( event )
		{
		case EVENT_SYSTEM_FOREGROUND: {
			if ( hwnd == nullptr )
				return;

			if ( idObject != OBJID_WINDOW || idChild != CHILDID_SELF )
				return;

			DWORD wndProcessId = 0;
			::GetWindowThreadProcessId( hwnd, &wndProcessId );

			HANDLE processHandle = ::OpenProcess( PROCESS_QUERY_LIMITED_INFORMATION, FALSE, wndProcessId );
			if ( processHandle == nullptr )
				return;

			wchar_t processPath[_MAX_PATH];
			DWORD   processPathLen     = _MAX_PATH;
			auto    queryResult        = ::QueryFullProcessImageNameW( processHandle, 0, processPath, &processPathLen );
			processPath[_MAX_PATH - 1] = 0;
			::CloseHandle( processHandle );
			if ( queryResult == 0 )
				return;

			wchar_t filename[_MAX_FNAME];
			_wsplitpath_s( processPath, nullptr, 0, nullptr, 0, filename, _MAX_FNAME, nullptr, 0 );
			filename[_MAX_FNAME - 1] = 0;
			std::transform( std::begin( filename ), std::end( filename ), std::begin( filename ),
			                []( wchar_t c ) -> wchar_t { return static_cast<wchar_t>( std::tolower( c ) ); } );

			if ( this->processNameSet.find( filename ) != this->processNameSet.cend() )
			{
				TheaterStart( hwnd );
			}
			else
			{
				TheaterStop();
			}

			break;
		}
		}
	}

	void App::TheaterEnable( bool state )
	{
		if ( state )
		{
			HookRegister();
		}
		else
		{
			HookUnregister();
			TheaterStop();
		}
	}

	void App::WinEventHookProc( HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd, LONG idObject, LONG idChild,
	                            DWORD idEventThread, DWORD dwmsEventTime )
	{
		App::Current().OnWinEvent( hWinEventHook, event, hwnd, idObject, idChild, idEventThread, dwmsEventTime );
	}

	bool App::HookRegister()
	{
		if ( this->winEventHook != nullptr )
			return true;

		this->winEventHook =
		    ::SetWinEventHook( EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_FOREGROUND, nullptr, App::WinEventHookProc, 0, 0,
		                       WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS );

		return this->winEventHook != nullptr;
	}

	void App::HookUnregister()
	{
		if ( this->winEventHook != nullptr )
		{
			::UnhookWinEvent( this->winEventHook );
			this->winEventHook = nullptr;
		}
	}

	void App::OnSettingsChanged()
	{
		// Apply settings
		const wchar_t* processNames[256] = {};
		const auto     processNameCount  = this->settings.GetProcessNames( processNames, 256 );

		this->processNameSet.clear();
		for ( size_t i = 0; i < processNameCount; i++ )
		{
			std::wstring name( processNames[i] );
			std::transform( name.begin(), name.end(), name.begin(),
			                []( wchar_t c ) { return static_cast<wchar_t>( std::tolower( c ) ); } );
			this->processNameSet.insert( std::move( name ) );
		}

		this->dimmer.SetAlpha( this->settings.GetAlpha() );
		this->dimmer.SetColor( this->settings.GetColor() );

		TheaterEnable( this->settings.IsTheaterEnabled() );
	}

	void App::SettingsChangedCallback()
	{
		App::Current().OnSettingsChanged();
	}

	bool App::Init()
	{
		auto result = ::CoInitializeEx( nullptr, COINIT_MULTITHREADED );
		if ( result != S_OK && result != S_FALSE )
			return false;

		this->settings.Load();

		if ( !this->tray.Init() )
			return false;

		if ( !MessageWindowCreate() )
			return false;

		if ( !this->dimmer.Init() )
			return false;

		if ( !HookRegister() )
			return false;

		this->settings.RegisterChangedCallback( App::SettingsChangedCallback );
		this->settings.NotifyChanges();

		return true;
	}

	int App::Run()
	{
		MSG msg;
		while ( ::GetMessageW( &msg, nullptr, 0, 0 ) )
		{
			::TranslateMessage( &msg );
			::DispatchMessageW( &msg );
		}

		return static_cast<int>( msg.wParam );
	}

	void App::Close()
	{
		this->settings.UnregisterChangedCallback( App::SettingsChangedCallback );
		this->settings.Save();
		HookUnregister();
		MessageWindowDestroy();
		this->dimmer.Close();
		this->tray.Close();

		::CoUninitialize();
	}
} // namespace Theater
