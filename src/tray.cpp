#include "theater.h"
#include "tray.h"
#include "resource.h"

namespace Theater
{
	namespace
	{
		constexpr wchar_t TRAY_WINDOWCLASS_NAME[] = L"TheaterTrayWindow";
		constexpr wchar_t TRAY_WINDOW_NAME[]      = L"TheaterTrayWindow";
		constexpr UINT    TRAY_WM_NOTIFICATION    = WM_USER + 1;

		// We won't be using a GUID for the tray icon as suggested by MSFT for Windows 7 and +
		// Whenever the user changes the install directory this will break!
		// This also means that we need 2 different GUIDs, one for debug exec and one for release exec
		// WTH Windows...
		// Source: https://stackoverflow.com/questions/7432319/notifyicondata-guid-problem
		// Source: https://docs.microsoft.com/en-us/windows/win32/api/shellapi/ns-shellapi-notifyicondataa
		// constexpr GUID TRAY_GUID                  = { 0x59cdfa40, 0xdb33, 0x44fd, 0x99, 0x24, 0x95, 0x13, 0x6c, 0x3e,
		// 0xef, 0x2b };
		constexpr UINT TRAY_ICON_ID = 1;
	} // namespace

	Tray::~Tray()
	{
		Close();
	}

	bool Tray::Init()
	{
		if ( !MessageWindowCreate() )
			return false;

		if ( !NotificationsRegister() )
			return false;

		return true;
	}

	void Tray::Close()
	{
		NotificationsUnregister();
		MessageWindowDestroy();
	}

	bool Tray::MessageWindowCreate()
	{
		const HINSTANCE hInstance = ::GetModuleHandleW( nullptr );

		WNDCLASSEXW wcex   = {};
		wcex.cbSize        = sizeof( WNDCLASSEX );
		wcex.lpfnWndProc   = Tray::WndProc;
		wcex.hInstance     = hInstance;
		wcex.lpszClassName = TRAY_WINDOWCLASS_NAME;

		if ( !RegisterClassExW( &wcex ) )
			return false;

		this->messageWindow = ::CreateWindowExW( 0, TRAY_WINDOWCLASS_NAME, TRAY_WINDOW_NAME, 0, 0, 0, 0, 0,
		                                         HWND_MESSAGE, nullptr, nullptr, nullptr );
		if ( this->messageWindow == nullptr )
			return false;

		::SetWindowLongPtrW( this->messageWindow, GWLP_USERDATA, reinterpret_cast<LONG_PTR>( this ) );
		return true;
	}

	void Tray::MessageWindowDestroy()
	{
		// destroy context menu
		if ( this->menu != nullptr )
		{
			::DestroyMenu( this->menu );
			this->menu        = nullptr;
			this->contextMenu = nullptr;
		}

		// destroy message window
		if ( this->messageWindow != nullptr )
		{
			::DestroyWindow( this->messageWindow );
			this->messageWindow = nullptr;
		}
	}

	bool Tray::NotificationsUnregister()
	{
		NOTIFYICONDATAW nid = {};
		nid.cbSize          = sizeof( NOTIFYICONDATAW );
		nid.uFlags          = 0;
		nid.hWnd            = this->messageWindow;
		nid.uID             = TRAY_ICON_ID;
		nid.hIcon           = this->icon;

		return Shell_NotifyIconW( NIM_DELETE, &nid );
	}

	bool Tray::NotificationsRegister()
	{
		const HINSTANCE hInstance = ::GetModuleHandleW( nullptr );

		this->icon = ::LoadIcon( hInstance, MAKEINTRESOURCE( IDI_THEATER ) );

		NOTIFYICONDATAW nid  = {};
		nid.cbSize           = sizeof( NOTIFYICONDATAW );
		nid.uFlags           = NIF_ICON | NIF_MESSAGE;
		nid.hWnd             = this->messageWindow;
		nid.uID              = TRAY_ICON_ID;
		nid.hIcon            = this->icon;
		nid.uCallbackMessage = TRAY_WM_NOTIFICATION;
		nid.uVersion         = NOTIFYICON_VERSION_4;

		if ( !Shell_NotifyIconW( NIM_ADD, &nid ) )
		{
			// didn't work? try to delete a previously stuck instance
			if ( !NotificationsUnregister() )
				return false;

			// try to register again
			if ( !Shell_NotifyIconW( NIM_ADD, &nid ) )
				return false;
		}

		if ( !Shell_NotifyIconW( NIM_SETVERSION, &nid ) )
			return false;

		return true;
	}

	LRESULT Tray::OnMessage( UINT message, WPARAM wParam, LPARAM lParam )
	{
		switch ( message )
		{
		case WM_COMMAND: {
			switch ( LOWORD( wParam ) )
			{
			case ID_TRAY_CONTEXT_OPACITY_100:
			case ID_TRAY_CONTEXT_OPACITY_90:
			case ID_TRAY_CONTEXT_OPACITY_80:
			case ID_TRAY_CONTEXT_OPACITY_70:
			case ID_TRAY_CONTEXT_OPACITY_60:
			case ID_TRAY_CONTEXT_OPACITY_50:
			case ID_TRAY_CONTEXT_OPACITY_40:
			case ID_TRAY_CONTEXT_OPACITY_30:
			case ID_TRAY_CONTEXT_OPACITY_20:
			case ID_TRAY_CONTEXT_OPACITY_10: {
				BYTE alpha = 255;
				switch ( LOWORD( wParam ) )
				{
				case ID_TRAY_CONTEXT_OPACITY_100:
					alpha = 255;
					break;
				case ID_TRAY_CONTEXT_OPACITY_90:
					alpha = 230;
					break;
				case ID_TRAY_CONTEXT_OPACITY_80:
					alpha = 204;
					break;
				case ID_TRAY_CONTEXT_OPACITY_70:
					alpha = 179;
					break;
				case ID_TRAY_CONTEXT_OPACITY_60:
					alpha = 153;
					break;
				case ID_TRAY_CONTEXT_OPACITY_50:
					alpha = 128;
					break;
				case ID_TRAY_CONTEXT_OPACITY_40:
					alpha = 102;
					break;
				case ID_TRAY_CONTEXT_OPACITY_30:
					alpha = 77;
					break;
				case ID_TRAY_CONTEXT_OPACITY_20:
					alpha = 51;
					break;
				case ID_TRAY_CONTEXT_OPACITY_10:
					alpha = 26;
					break;
				}

				App::Current().GetSettings().SetAlpha( alpha );
				App::Current().GetSettings().NotifyChanges();
				return 0;
			}

			case ID_TRAY_CONTEXT_COLOR: {
				static COLORREF customColors[16] = {};

				CHOOSECOLORW cc = {};
				cc.lStructSize  = sizeof( cc );
				cc.Flags        = CC_RGBINIT | CC_ANYCOLOR | CC_FULLOPEN;
				cc.lpCustColors = customColors;
				cc.rgbResult    = App::Current().GetSettings().GetColor();

				if ( ::ChooseColorW( &cc ) == TRUE )
					App::Current().GetSettings().SetColor( cc.rgbResult );

				App::Current().GetSettings().NotifyChanges();
				return 0;
			}
			case ID_TRAY_CONTEXT_EXIT: {
				::PostQuitMessage( 0 );
				return 0;
			}
			}
			break;
		}
		case TRAY_WM_NOTIFICATION: {
			switch ( LOWORD( lParam ) )
			{
			case WM_CONTEXTMENU: {
				if ( this->menu == nullptr )
				{
					this->menu = ::LoadMenuW( ::GetModuleHandleW( nullptr ), MAKEINTRESOURCEW( IDM_TRAY_CONTEXT ) );
					this->contextMenu = ::GetSubMenu( this->menu, 0 );
				}

				// needed to ensure context menu gets closed if a use clicks elsewhere when it is open
				::SetForegroundWindow( this->messageWindow );

				UINT flags = TPM_BOTTOMALIGN;
				flags |= ::GetSystemMetrics( SM_MENUDROPALIGNMENT ) != 0 ? TPM_RIGHTALIGN : TPM_LEFTALIGN;

				const int mouseX = GET_X_LPARAM( wParam );
				const int mouseY = GET_Y_LPARAM( wParam );
				::TrackPopupMenuEx( this->contextMenu, flags, mouseX, mouseY, this->messageWindow, nullptr );
				break;
			}
			}
			break;
		}
		}

		return DefWindowProc( this->messageWindow, message, wParam, lParam );
	}

	LRESULT CALLBACK Tray::WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
	{
		auto tray = reinterpret_cast<Tray*>( ::GetWindowLongPtrW( hWnd, GWLP_USERDATA ) );
		if ( tray == nullptr )
			return DefWindowProc( hWnd, message, wParam, lParam );

		return tray->OnMessage( message, wParam, lParam );
	}
} // namespace Theater
