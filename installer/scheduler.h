#pragma once
#include <Windows.h>
#include <tchar.h>

HRESULT DeleteTask(LPTSTR pszName);

HRESULT RegisterTask(LPTSTR pszName, LPCTSTR pszInstallPath);

