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

	HWND Dialog::GetHandle() const
	{
		return this->hwnd;
	}

	Dialog* Dialog::FromHandle( HWND hwnd )
	{
		return reinterpret_cast<Dialog*>( ::GetWindowLongPtr( hwnd, DWLP_USER ) );
	}

	INT_PTR Dialog::ShowModal( HWND parentWnd )
	{
		this->isModal = true;
		return ::DialogBoxParamW( this->instance, MAKEINTRESOURCEW( this->resourceID ), parentWnd, DlgProc, reinterpret_cast<LONG_PTR>(this) );
	}

	BOOL Dialog::OnInitDialog()
	{
		return this->isModal ? TRUE : FALSE;
	}

	INT_PTR Dialog::OnMessage( UINT message, WPARAM wParam, LPARAM lParam )
	{
		UNREFERENCED_PARAMETER( wParam );
		UNREFERENCED_PARAMETER( lParam );

		switch (message)
		{
		case WM_INITDIALOG: {
			return OnInitDialog();
		}
		}

		return FALSE;
	}

	INT_PTR Dialog::DlgProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
	{
		auto dialog = Dialog::FromHandle( hWnd );
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