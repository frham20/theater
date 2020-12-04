#include "theater.h"
#include "settings.h"
#include <rapidjson/rapidjson.h>
#include <rapidjson/document.h>
#include <rapidjson/filewritestream.h>
#include <rapidjson/filereadstream.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/reader.h>

static constexpr int SETTINGS_VERSION = 1;

struct Settings
{
	bool dirty = false;
	bool enabled = true;
	std::vector<std::wstring> processNames;
	BYTE alpha = 200;
	COLORREF color = RGB(0, 0, 0);
};

static Settings s_settings;
static wchar_t s_settingsFilename[MAX_PATH] = L"";
static wchar_t s_settingsDirectory[MAX_PATH] = L"";
static char s_jsonReadWriteBuffer[1024 * 8];

typedef rapidjson::GenericDocument<rapidjson::UTF16<> > JSONDocument;
typedef rapidjson::GenericValue<rapidjson::UTF16<> >    JSONValue;

static const wchar_t* Settings_GetSettingsDirectory()
{
	if (s_settingsDirectory[0] == 0)
	{
		PWSTR pLocalAppDataPath = nullptr;
		if (::SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_DEFAULT, nullptr, &pLocalAppDataPath) != S_OK)
		{
			::CoTaskMemFree(pLocalAppDataPath);
			return s_settingsDirectory;
		}

		_snwprintf_s(s_settingsDirectory, MAX_PATH, L"%s\\Theater", pLocalAppDataPath);
		::CoTaskMemFree(pLocalAppDataPath);
	}

	return s_settingsDirectory;
}

static const wchar_t* Settings_GetSettingsFilename()
{
	if (s_settingsFilename[0] == 0)
	{
		const auto dir = Settings_GetSettingsDirectory();
		_snwprintf_s(s_settingsFilename, MAX_PATH, L"%s\\settings.json", dir);
	}

	return s_settingsFilename;
}

bool Settings_Load()
{
	const auto filename = Settings_GetSettingsFilename();
	if (::GetFileAttributesW(filename) == INVALID_FILE_ATTRIBUTES)
		return false;

	FILE* fp = nullptr;
	_wfopen_s(&fp, filename, L"rb");
	if (fp == nullptr)
		return false;

	rapidjson::FileReadStream inStream(fp, s_jsonReadWriteBuffer, sizeof(s_jsonReadWriteBuffer));
	
	JSONDocument doc;
	doc.ParseStream<rapidjson::kParseDefaultFlags, rapidjson::UTF8<>, rapidjson::FileReadStream>(inStream);
	fclose(fp);

	if (doc.HasParseError())
		return false;

	int version = 0;
	const auto& versionVal = doc[L"version"];
	if (versionVal.IsInt())
		version = versionVal.GetInt();

	switch (version)
	{
	case SETTINGS_VERSION:
	{
		const auto& enabledVal = doc[L"enabled"];
		if (enabledVal.IsBool())
			s_settings.enabled = enabledVal.GetBool();

		const auto& alphaVal = doc[L"alpha"];
		if (alphaVal.IsInt())
			s_settings.alpha = static_cast<BYTE>(std::max(0, std::min(255, alphaVal.GetInt())));

		const auto& color = doc[L"color"];
		if (color.IsArray() && color.Size() == 3)
		{
			BYTE r = 0;
			BYTE g = 0;
			BYTE b = 0;

			if (color[0].IsInt())
				r = static_cast<BYTE>(std::max(0, std::min(255, color[0].GetInt())));
			if (color[1].IsInt())
				g = static_cast<BYTE>(std::max(0, std::min(255, color[1].GetInt())));
			if (color[2].IsInt())
				b = static_cast<BYTE>(std::max(0, std::min(255, color[2].GetInt())));
			
			s_settings.color = RGB(r, g, b);
		}

		const auto& processes = doc[L"processes"];
		if (processes.IsArray())
		{
			s_settings.processNames.clear();

			for (const auto& name : processes.GetArray())
				s_settings.processNames.emplace_back(std::wstring(name.GetString()));
		}

		break;
	}
	default:
	{
		//unsupported version, bail out
		return false;
	}
	}

	s_settings.dirty = false;

	return true;
}

bool Settings_Save()
{
	const auto filename = Settings_GetSettingsFilename();
	if (::GetFileAttributesW(filename) != INVALID_FILE_ATTRIBUTES && !s_settings.dirty)
		return true;

	JSONDocument doc;
	doc.SetObject();

	auto& docAllocator = doc.GetAllocator();

	doc.AddMember(L"version", JSONValue(SETTINGS_VERSION), docAllocator);
	doc.AddMember(L"enabled", JSONValue(s_settings.enabled), docAllocator);
	doc.AddMember(L"alpha", JSONValue(static_cast<int>(s_settings.alpha)), docAllocator);

	JSONValue color(rapidjson::kArrayType);
	color.Reserve(3, docAllocator);
	color.PushBack(JSONValue(GetRValue(s_settings.color)), docAllocator);
	color.PushBack(JSONValue(GetGValue(s_settings.color)), docAllocator);
	color.PushBack(JSONValue(GetBValue(s_settings.color)), docAllocator);
	doc.AddMember(L"color", color, docAllocator);

	JSONValue processNames(rapidjson::kArrayType);
	processNames.Reserve(static_cast<rapidjson::SizeType>(s_settings.processNames.size()), docAllocator);
	for (const auto& name : s_settings.processNames)
		processNames.PushBack(JSONValue(rapidjson::StringRef(name.c_str())), docAllocator);
	doc.AddMember(L"processes", processNames, docAllocator);

	//make sure the directory exists
	::SHCreateDirectoryExW(nullptr, Settings_GetSettingsDirectory(), nullptr);

	FILE* fp = nullptr;
	_wfopen_s(&fp, filename, L"wb");
	if (fp == nullptr)
		return false;

	rapidjson::FileWriteStream outStream(fp, s_jsonReadWriteBuffer, sizeof(s_jsonReadWriteBuffer));
	rapidjson::PrettyWriter<rapidjson::FileWriteStream, rapidjson::UTF16<>, rapidjson::UTF8<> > writer(outStream);
	const bool success = doc.Accept(writer);
	fclose(fp);

	return success;
}

bool Settings_IsTheaterEnabled()
{
	return true;
}

void Settings_EnableTheater(bool state)
{
	s_settings.enabled = state;
	s_settings.dirty = true;
}

size_t Settings_GetProcessNamesCount()
{
	return s_settings.processNames.size();
}

size_t Settings_GetProcessNames(const wchar_t* processNames[], size_t processNamesCount)
{
	if (processNames == nullptr)
		return 0u;

	const size_t maxElements = std::min(processNamesCount, s_settings.processNames.size());
	for (size_t i = 0; i < maxElements; i++)
		processNames[i] = s_settings.processNames[i].c_str();

	return maxElements;
}

void Settings_AddProcessName(const wchar_t* processName)
{
	s_settings.processNames.emplace_back(std::wstring(processName));
	s_settings.dirty = true;
}

void Settings_RemoveProcessName(const wchar_t* processName)
{
	auto iter = std::find_if(s_settings.processNames.begin(), s_settings.processNames.end(), [processName](const std::wstring& name) { return _wcsicmp(name.c_str(), processName) == 0; });
	if (iter == s_settings.processNames.end())
		return;

	s_settings.processNames.erase(iter);
	s_settings.dirty = true;
}

BYTE Settings_GetAlpha()
{
	return s_settings.alpha;
}

COLORREF Settings_GetColor()
{
	return s_settings.color;
}