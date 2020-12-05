#pragma once
bool Settings_Load();
bool Settings_Save();
bool Settings_IsTheaterEnabled();
void Settings_EnableTheater(bool state);
size_t Settings_GetProcessNamesCount();
size_t Settings_GetProcessNames(const wchar_t* processNames[], size_t processNamesCount );
void Settings_AddProcessName(const wchar_t* processName);
void Settings_RemoveProcessName(const wchar_t* processName);
BYTE Settings_GetAlpha();
void Settings_SetAlpha(BYTE alpha);
COLORREF Settings_GetColor();
void Settings_SetColor(COLORREF color);

typedef void (*SETTINGSCHANGEDCALLBACK)();
void Settings_RegisterChangedCallback(SETTINGSCHANGEDCALLBACK callback);
void Settings_UnregisterChangedCallback(SETTINGSCHANGEDCALLBACK callback);
void Settings_NotifyChanges();
