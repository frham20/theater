#include "theater.h"
#include "dimmer.h"
#include "resource.h"

namespace Theater
{
	namespace
	{
		constexpr wchar_t DIMMER_WINDOWCLASS_NAME[] = L"TheaterDimmerWindow";
		constexpr wchar_t DIMMER_WINDOW_NAME[]      = L"TheaterDimmerWindow";
	} // namespace

	BOOL Dimmer::EnumMonitorsProc( HMONITOR handle, HDC dc, LPRECT rc, LPARAM lParam )
	{
		UNREFERENCED_PARAMETER( dc );

		auto dimmer = reinterpret_cast<Dimmer*>( lParam );

		MonitorInstance monitor = {};
		monitor.handle          = handle;
		monitor.rc              = *rc;
		dimmer->monitors.emplace_back( std::move( monitor ) );
		return TRUE;
	}

	LRESULT Dimmer::OnMessage( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
	{
		switch ( message )
		{
		case WM_PAINT: {
			PAINTSTRUCT ps;
			HDC         dc = ::BeginPaint( hWnd, &ps );

			const COLORREF oldDCBrushColor = ::SetDCBrushColor( dc, this->clearColor );
			::FillRect( dc, &ps.rcPaint, static_cast<HBRUSH>( ::GetStockObject( DC_BRUSH ) ) );
			::SetDCBrushColor( dc, oldDCBrushColor );

			::EndPaint( hWnd, &ps );

			return 0;
		}
		case WM_ERASEBKGND: {
			return TRUE;
		}
		}

		return ::DefWindowProc( hWnd, message, wParam, lParam );
	}

	LRESULT CALLBACK Dimmer::WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
	{
		auto dimmer = reinterpret_cast<Dimmer*>( ::GetWindowLongPtr( hWnd, GWLP_USERDATA ) );
		if ( dimmer != nullptr )
			return dimmer->OnMessage( hWnd, message, wParam, lParam );

		if ( message == WM_NCCREATE )
		{
			auto cs = reinterpret_cast<LPCREATESTRUCTW>( lParam );
			dimmer  = reinterpret_cast<Dimmer*>( cs->lpCreateParams );
			::SetWindowLongPtr( hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>( dimmer ) );
			return dimmer->OnMessage( hWnd, message, wParam, lParam );
		}

		return ::DefWindowProc( hWnd, message, wParam, lParam );
	}

	bool Dimmer::WindowsCreate()
	{
		// enum all monitors
		if ( !::EnumDisplayMonitors( nullptr, nullptr, EnumMonitorsProc, reinterpret_cast<LPARAM>(this) ))
			return false;

		const HINSTANCE hInstance = ::GetModuleHandleW( nullptr );

		// register our window class
		WNDCLASSEXW wcex;
		wcex.cbSize        = sizeof( WNDCLASSEX );
		wcex.style         = CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc   = WndProc;
		wcex.cbClsExtra    = 0;
		wcex.cbWndExtra    = 0;
		wcex.hInstance     = hInstance;
		wcex.hIcon         = ::LoadIcon( hInstance, MAKEINTRESOURCE( IDI_THEATER ) );
		wcex.hCursor       = ::LoadCursor( nullptr, IDC_ARROW );
		wcex.hbrBackground = nullptr; // static_cast<HBRUSH>(::GetStockObject(BLACK_BRUSH));
		wcex.lpszMenuName  = NULL;
		wcex.lpszClassName = DIMMER_WINDOWCLASS_NAME;
		wcex.hIconSm       = nullptr;

		if ( !RegisterClassExW( &wcex ) )
			return false;

		// for each monitor, create a window overlapping the entire region
		for ( auto& monitor : this->monitors )
		{
			const DWORD exStyle = WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE;
			const DWORD style   = WS_POPUP;
			monitor.hwnd =
			    ::CreateWindowExW( exStyle, DIMMER_WINDOWCLASS_NAME, DIMMER_WINDOW_NAME, style, monitor.rc.left,
			                       monitor.rc.top, monitor.rc.right - monitor.rc.left,
			                       monitor.rc.bottom - monitor.rc.top, nullptr, nullptr, hInstance, this );
			::SetLayeredWindowAttributes( monitor.hwnd, 0, 0, LWA_ALPHA );
		}

		return true;
	}

	void Dimmer::WindowsDestroy()
	{
		for ( auto& monitor : this->monitors )
			::DestroyWindow( monitor.hwnd );

		this->monitors.clear();
	}

	bool Dimmer::Init()
	{
		return WindowsCreate();
	}

	void Dimmer::Show( bool state )
	{
		for ( const auto& monitor : this->monitors )
			::ShowWindow( monitor.hwnd, state ? SW_SHOWNOACTIVATE : SW_HIDE );
	}

	void Dimmer::SetAlpha( float alpha )
	{
		const BYTE alpha256 =
		    static_cast<BYTE>( std::min( static_cast<DWORD>( 255 ), static_cast<DWORD>( alpha * 255.0f + 0.5f ) ) );
		for ( const auto& monitor : this->monitors )
			::SetLayeredWindowAttributes( monitor.hwnd, 0, alpha256, LWA_ALPHA );
	}

	void Dimmer::SetColor( COLORREF rgb )
	{
		this->clearColor = rgb;

		for ( const auto& monitor : this->monitors )
		{
			if ( !::IsWindowVisible( monitor.hwnd ) )
				continue;

			::InvalidateRect( monitor.hwnd, nullptr, FALSE );
		}
	}

	void Dimmer::SetColor( float r, float g, float b )
	{
		const BYTE r256 =
		    static_cast<BYTE>( std::min( static_cast<DWORD>( 255 ), static_cast<DWORD>( r * 255.0f + 0.5f ) ) );
		const BYTE g256 =
		    static_cast<BYTE>( std::min( static_cast<DWORD>( 255 ), static_cast<DWORD>( g * 255.0f + 0.5f ) ) );
		const BYTE b256 =
		    static_cast<BYTE>( std::min( static_cast<DWORD>( 255 ), static_cast<DWORD>( b * 255.0f + 0.5f ) ) );
		SetColor( RGB( r256, g256, b256 ) );
	}

	void Dimmer::Close()
	{
		WindowsDestroy();
	}

	bool Dimmer::IsDimmerWindow( HWND hwnd ) const
	{
		for ( const auto& monitor : this->monitors )
		{
			if ( hwnd == monitor.hwnd )
				return true;
		}

		return false;
	}

} // namespace Theater
