#pragma once
namespace Theater
{
	class Dimmer
	{
	public:
		Dimmer()  = default;
		~Dimmer() = default;

		bool Init();
		void Close();

		void Show( bool state );
		bool IsDimmerWindow( HWND hwnd ) const;

		void SetAlpha( float alpha );
		void SetColor( COLORREF rgb );
		void SetColor( float r, float g, float b );

	private:
		struct MonitorInstance
		{
			HMONITOR handle;
			RECT     rc;
			HWND     hwnd;
		};

		static BOOL EnumMonitorsProc( HMONITOR handle, HDC dc, LPRECT rc, LPARAM lParam );

		bool                    WindowsCreate();
		void                    WindowsDestroy();
		LRESULT                 OnMessage( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );
		static LRESULT CALLBACK WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );

	private:
		std::vector<MonitorInstance> monitors;
		COLORREF                     clearColor = RGB( 0, 0, 0 );
	};

} // namespace Theater