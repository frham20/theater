#pragma once

namespace Theater
{
	class Dialog
	{
	public:
		INT_PTR ShowModal( HWND parentWnd );
		HWND GetHandle() const;
		static Dialog* FromHandle( HWND hwnd );

	protected:
		explicit Dialog( int resourceID);
		Dialog( HINSTANCE instance, int resourceID );
		~Dialog();

		virtual INT_PTR OnMessage( UINT message, WPARAM wParam, LPARAM lParam ) = 0;

	private:
		static INT_PTR DlgProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );

	private:
		HINSTANCE instance;
		int  resourceID;
		HWND hwnd;
	};
}
