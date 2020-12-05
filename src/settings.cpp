#include "theater.h"
#include "settings.h"
#include <rapidjson/rapidjson.h>
#include <rapidjson/document.h>
#include <rapidjson/filewritestream.h>
#include <rapidjson/filereadstream.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/reader.h>

namespace Theater
{
	namespace
	{
		static constexpr int SETTINGS_VERSION = 1;

		typedef rapidjson::GenericDocument<rapidjson::UTF16<>> JSONDocument;
		typedef rapidjson::GenericValue<rapidjson::UTF16<>>    JSONValue;

		wchar_t s_settingsFilename[MAX_PATH]  = L"";
		wchar_t s_settingsDirectory[MAX_PATH] = L"";
		char    s_jsonReadWriteBuffer[1024 * 8];

		const wchar_t* GetSettingsDirectory()
		{
			if ( s_settingsDirectory[0] == 0 )
			{
				PWSTR pLocalAppDataPath = nullptr;
				if ( ::SHGetKnownFolderPath( FOLDERID_LocalAppData, KF_FLAG_DEFAULT, nullptr, &pLocalAppDataPath ) !=
				     S_OK )
				{
					::CoTaskMemFree( pLocalAppDataPath );
					return s_settingsDirectory;
				}

				_snwprintf_s( s_settingsDirectory, MAX_PATH, L"%s\\Theater", pLocalAppDataPath );
				::CoTaskMemFree( pLocalAppDataPath );
			}

			return s_settingsDirectory;
		}

		const wchar_t* GetSettingsFilename()
		{
			if ( s_settingsFilename[0] == 0 )
			{
				const auto dir = GetSettingsDirectory();
				_snwprintf_s( s_settingsFilename, MAX_PATH, L"%s\\settings.json", dir );
			}

			return s_settingsFilename;
		}
	} // namespace

	bool Settings::Load()
	{
		const auto filename = GetSettingsFilename();
		if ( ::GetFileAttributesW( filename ) == INVALID_FILE_ATTRIBUTES )
			return false;

		FILE* fp = nullptr;
		_wfopen_s( &fp, filename, L"rb" );
		if ( fp == nullptr )
			return false;

		rapidjson::FileReadStream inStream( fp, s_jsonReadWriteBuffer, sizeof( s_jsonReadWriteBuffer ) );

		JSONDocument doc;
		doc.ParseStream<rapidjson::kParseDefaultFlags, rapidjson::UTF8<>, rapidjson::FileReadStream>( inStream );
		fclose( fp );

		if ( doc.HasParseError() )
			return false;

		int         version    = 0;
		const auto& versionVal = doc[L"version"];
		if ( versionVal.IsInt() )
			version = versionVal.GetInt();

		switch ( version )
		{
		case SETTINGS_VERSION: {
			const auto& enabledVal = doc[L"enabled"];
			if ( enabledVal.IsBool() )
				this->enabled = enabledVal.GetBool();

			const auto& alphaVal = doc[L"alpha"];
			if ( alphaVal.IsInt() )
				this->alpha = static_cast<BYTE>( std::max( 0, std::min( 255, alphaVal.GetInt() ) ) );

			const auto& colorVal = doc[L"color"];
			if ( colorVal.IsArray() && colorVal.Size() == 3 )
			{
				BYTE r = 0;
				BYTE g = 0;
				BYTE b = 0;

				if ( colorVal[0].IsInt() )
					r = static_cast<BYTE>( std::max( 0, std::min( 255, colorVal[0].GetInt() ) ) );
				if ( colorVal[1].IsInt() )
					g = static_cast<BYTE>( std::max( 0, std::min( 255, colorVal[1].GetInt() ) ) );
				if ( colorVal[2].IsInt() )
					b = static_cast<BYTE>( std::max( 0, std::min( 255, colorVal[2].GetInt() ) ) );

				this->color = RGB( r, g, b );
			}

			const auto& processes = doc[L"processes"];
			if ( processes.IsArray() )
			{
				this->processNames.clear();

				for ( const auto& name : processes.GetArray() )
					this->processNames.emplace_back( std::wstring( name.GetString() ) );
			}

			break;
		}
		default: {
			// unsupported version, bail out
			return false;
		}
		}

		this->dirty = false;

		return true;
	}

	bool Settings::Save() const
	{
		const auto filename = GetSettingsFilename();
		if ( ::GetFileAttributesW( filename ) != INVALID_FILE_ATTRIBUTES && !this->dirty )
			return true;

		JSONDocument doc;
		doc.SetObject();

		auto& docAllocator = doc.GetAllocator();

		doc.AddMember( L"version", JSONValue( SETTINGS_VERSION ), docAllocator );
		doc.AddMember( L"enabled", JSONValue( this->enabled ), docAllocator );
		doc.AddMember( L"alpha", JSONValue( static_cast<int>( this->alpha ) ), docAllocator );

		JSONValue colorVal( rapidjson::kArrayType );
		colorVal.Reserve( 3, docAllocator );
		colorVal.PushBack( JSONValue( GetRValue( this->color ) ), docAllocator );
		colorVal.PushBack( JSONValue( GetGValue( this->color ) ), docAllocator );
		colorVal.PushBack( JSONValue( GetBValue( this->color ) ), docAllocator );
		doc.AddMember( L"color", colorVal, docAllocator );

		JSONValue processNamesVal( rapidjson::kArrayType );
		processNamesVal.Reserve( static_cast<rapidjson::SizeType>( this->processNames.size() ), docAllocator );
		for ( const auto& name : this->processNames )
			processNamesVal.PushBack( JSONValue( rapidjson::StringRef( name.c_str() ) ), docAllocator );
		doc.AddMember( L"processes", processNamesVal, docAllocator );

		// make sure the directory exists
		::SHCreateDirectoryExW( nullptr, GetSettingsDirectory(), nullptr );

		FILE* fp = nullptr;
		_wfopen_s( &fp, filename, L"wb" );
		if ( fp == nullptr )
			return false;

		rapidjson::FileWriteStream outStream( fp, s_jsonReadWriteBuffer, sizeof( s_jsonReadWriteBuffer ) );
		rapidjson::PrettyWriter<rapidjson::FileWriteStream, rapidjson::UTF16<>, rapidjson::UTF8<>> writer( outStream );
		const bool success = doc.Accept( writer );
		fclose( fp );

		this->dirty = false;

		return success;
	}

	bool Settings::IsTheaterEnabled() const
	{
		return this->enabled;
	}

	void Settings::EnableTheater( bool state )
	{
		this->enabled = state;
		this->dirty   = true;
	}

	size_t Settings::GetProcessNamesCount() const
	{
		return this->processNames.size();
	}

	size_t Settings::GetProcessNames( const wchar_t* processes[], size_t processNamesCount ) const
	{
		if ( processes == nullptr )
			return 0u;

		const size_t maxElements = std::min( processNamesCount, this->processNames.size() );
		for ( size_t i = 0; i < maxElements; i++ )
			processes[i] = this->processNames[i].c_str();

		return maxElements;
	}

	void Settings::AddProcessName( const wchar_t* processName )
	{
		this->processNames.emplace_back( std::wstring( processName ) );
		this->dirty = true;
	}

	void Settings::RemoveProcessName( const wchar_t* processName )
	{
		auto iter = std::find_if(
		    this->processNames.begin(), this->processNames.end(),
		    [processName]( const std::wstring& name ) { return _wcsicmp( name.c_str(), processName ) == 0; } );
		if ( iter == this->processNames.end() )
			return;

		this->processNames.erase( iter );
		this->dirty = true;
	}

	BYTE Settings::GetAlpha() const
	{
		return this->alpha;
	}

	void Settings::SetAlpha( BYTE value )
	{
		this->alpha      = value;
		this->dirty = true;
	}

	COLORREF Settings::GetColor() const
	{
		return this->color;
	}

	void Settings::SetColor( COLORREF value )
	{
		this->color      = value;
		this->dirty = true;
	}

	void Settings::RegisterChangedCallback( SETTINGSCHANGEDCALLBACK callback )
	{
		this->notifyCallbacks.emplace_back( callback );
	}

	void Settings::UnregisterChangedCallback( SETTINGSCHANGEDCALLBACK callback )
	{
		auto iter = std::find_if( this->notifyCallbacks.begin(), this->notifyCallbacks.end(),
		                          [callback]( SETTINGSCHANGEDCALLBACK clb ) { return clb == callback; } );
		if ( iter == this->notifyCallbacks.end() )
			return;

		this->notifyCallbacks.erase( iter );
	}

	void Settings::NotifyChanges() const
	{
		for ( auto callback : this->notifyCallbacks )
			callback();
	}
} // namespace Theater
