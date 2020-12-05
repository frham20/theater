#pragma once

//Win32 API
#include <SDKDDKVer.h>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <windowsx.h>
#include <combaseapi.h>
#include <shellapi.h>
#include <shlobj.h>
#include <commdlg.h>

//STL
#include <algorithm>
#include <chrono>
#include <string>
#include <unordered_set>
#include <vector>

//App
#include "settings.h"
#include "tray.h"
#include "app.h"
