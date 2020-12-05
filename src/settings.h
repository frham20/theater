#pragma once

namespace Theater
{
	class Settings
	{
	public:
		Settings()  = default;
		~Settings() = default;

		bool Load();
		bool Save() const;

		bool IsTheaterEnabled() const;
		void EnableTheater( bool state );

		size_t GetProcessNamesCount() const;
		size_t GetProcessNames( const wchar_t* processNames[], size_t processNamesCount ) const;
		void   AddProcessName( const wchar_t* processName );
		void   RemoveProcessName( const wchar_t* processName );

		BYTE     GetAlpha() const;
		void     SetAlpha( BYTE alpha );
		COLORREF GetColor() const;
		void     SetColor( COLORREF color );

		typedef void ( *SETTINGSCHANGEDCALLBACK )();
		void RegisterChangedCallback( SETTINGSCHANGEDCALLBACK callback );
		void UnregisterChangedCallback( SETTINGSCHANGEDCALLBACK callback );
		void NotifyChanges() const;

	private:
		mutable bool dirty   = false;
		bool         enabled = true;
		BYTE         alpha   = 200;
		COLORREF     color   = RGB( 0, 0, 0 );

		std::vector<std::wstring> processNames;

		std::vector<SETTINGSCHANGEDCALLBACK> notifyCallbacks;
	};

} // namespace Theater