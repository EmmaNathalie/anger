#pragma once
#include <Windows.h>
#include <tchar.h>

#define SERVICE_NAME             _T("Anger")

#define SERVICE_DISPLAY_NAME     _T("CppWindowsService Sample Service")

#define SERVICE_START_TYPE       SERVICE_AUTO_START

#define SERVICE_DEPENDENCIES     _T("")

#define SERVICE_ACCOUNT          _T("NT AUTHORITY\\LocalService")

#define SERVICE_PASSWORD         NULL

DWORD DoInstallSvc(LPCTSTR pszInstallPath);

DWORD DoUninstallSvc();

DWORD DoStartSvc();
