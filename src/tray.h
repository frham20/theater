#pragma once

namespace Theater
{
	class Tray
	{
	public:
		Tray() = default;
		~Tray();

		bool Init();
		void Close();

	private:
		Tray( Tray&& )      = delete;
		Tray( const Tray& ) = delete;
		Tray& operator=( const Tray& ) = delete;
		Tray& operator=( Tray&& ) = delete;

		bool MessageWindowCreate();
		void MessageWindowDestroy();
		bool NotificationsRegister();
		bool NotificationsUnregister();

		LRESULT                 OnMessage( UINT message, WPARAM wParam, LPARAM lParam );
		static LRESULT CALLBACK WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );

	private:
		HMENU menu          = nullptr;
		HMENU contextMenu   = nullptr;
		HWND  messageWindow = nullptr;
		HICON icon          = nullptr;
	};
} // namespace Theater