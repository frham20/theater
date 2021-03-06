#pragma once

namespace Theater
{
	class App
	{
	public:
		App()  = default;
		~App() = default;

		bool Init();
		int  Run();
		void Close();

		Settings&       GetSettings();
		const Settings& GetSettings() const;

		static App& Current();

	private:
		void TheaterStart( HWND hwnd );
		void TheaterStop();
		void TheaterEnable( bool state );

		bool                    MessageWindowCreate();
		void                    MessageWindowDestroy();
		LRESULT                 OnMessage( UINT message, WPARAM wParam, LPARAM lParam );
		static LRESULT CALLBACK MessageWndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );

		bool        HookRegister();
		void        HookUnregister();
		void        OnWinEvent( HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd, LONG idObject, LONG idChild,
		                        DWORD idEventThread, DWORD dwmsEventTime );
		static void WinEventHookProc( HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd, LONG idObject, LONG idChild,
		                              DWORD idEventThread, DWORD dwmsEventTime );

		void        OnSettingsChanged();
		static void SettingsChangedCallback();

	private:
		App( const App& ) = delete;
		App( App&& )      = delete;
		App& operator=( const App& ) = delete;
		App& operator=( App&& ) = delete;

	private:
		HWND messageWindow = nullptr;
		bool theaterShown  = false;

		UINT_PTR                                       timerID = 0;
		std::chrono::high_resolution_clock::time_point timerStart;

		HWINEVENTHOOK                    winEventHook = nullptr;
		std::vector<HWND>                topLevelWindows;
		std::unordered_set<std::wstring> processNameSet;

		Dimmer   dimmer;
		Tray     tray;
		Settings settings;
	};
} // namespace Theater