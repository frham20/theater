#include "theater.h"

namespace Theater
{
	Dialog::Dialog( HINSTANCE _instance, int _resourceID )
	    : instance( _instance )
	    , resourceID( _resourceID )
	{
	}

	Dialog::Dialog( int _resourceID )
	    : Dialog( ::GetModuleHandle( nullptr ), _resourceID )
	{
	}

	Dialog::~Dialog()
	{
	}

	INT_PTR Dialog::ShowModal( HWND parentWnd )
	{
		return ::DialogBoxParamW( this->instance, MAKEINTRESOURCEW( this->resourceID ), parentWnd, DlgProc, reinterpret_cast<LONG_PTR>(this) );
	}

	INT_PTR Dialog::DlgProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
	{
		auto dialog = reinterpret_cast<Dialog*>(::GetWindowLongPtr( hWnd, DWLP_USER ));
		if(dialog != nullptr)
			return dialog->OnMessage( message, wParam, lParam );

		if (message == WM_INITDIALOG)
		{
			dialog = reinterpret_cast<Dialog*>( lParam );
			dialog->hwnd = hWnd;
			::SetWindowLongPtr( hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(dialog) );
			return dialog->OnMessage( message, wParam, lParam );
		}

		return FALSE;
	}
} // namespace Theater