#include "systeminfo.h"
#include "log.h"

#include <Shlwapi.h>
#include <Shlobj.h>
#include <atlcomcli.h>
#include <atlbase.h>
#include <atlcom.h>
#include <atlbase.h>
#include <atlcom.h>
#include <atlwin.h>
#include <atlsimpcoll.h>
#include <windows.h>
#include <objbase.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <shellapi.h>
#include <atlsnap.h>

#pragma comment(lib, "shlwapi.lib")
#pragma warning(disable : 4996)


typedef BOOL(WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);

BOOL IsWow64()
{
	BOOL bIsWow64 = FALSE;
	LPFN_ISWOW64PROCESS fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(GetModuleHandle(_T("kernel32")), "IsWow64Process");

	if (NULL != fnIsWow64Process)
	{
		if (!fnIsWow64Process(GetCurrentProcess(), &bIsWow64))
		{
			//handle error
		}
	}
	return bIsWow64;
}

BOOL IsAdmin()
{
	return IsUserAnAdmin();
}

BOOL IsBuiltInAdmin()
{
	HANDLE procToken;
	DWORD size;

	if (!IsAdmin() || !OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &procToken))
		return FALSE;

	if (GetTokenInformation(procToken, TokenUser, NULL, 0, &size) || GetLastError() != ERROR_INSUFFICIENT_BUFFER)
	{
		CloseHandle(procToken);

		return FALSE;
	}

	TOKEN_USER *tokenUser = (TOKEN_USER *)malloc(size);
	if (!tokenUser)
	{
		CloseHandle(procToken);
		return FALSE;
	}

	if (!GetTokenInformation(procToken, TokenUser, tokenUser, size, &size))
	{
		CloseHandle(procToken);
		free(tokenUser);
		return FALSE;
	}

	BOOL bRet = IsWellKnownSid(tokenUser->User.Sid, WinAccountAdministratorSid);

	CloseHandle(procToken);
	free(tokenUser);

	return bRet;
}

BOOL IsUacSupported()
{
	HKEY hkey;
	DWORD value = 1, size = sizeof(DWORD);

	OSVERSIONINFO osvi;

	ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

	GetVersionEx(&osvi);

	if (osvi.dwMajorVersion < 6)
	{
		return FALSE;
	}

	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\System"), 0, KEY_READ, &hkey) == ERROR_SUCCESS)
	{
		if (RegQueryValueEx(hkey, _T("EnableLUA"), 0, 0, (LPBYTE)&value, &size) != ERROR_SUCCESS)
			value = 1;

		RegCloseKey(hkey);
	}

	return value != 0;
}


VOID SystemInfo()
{
	OSVERSIONINFO osvi;

	ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

	GetVersionEx(&osvi);

	LogInfo(_T("Major version: %d\r\n"), osvi.dwMajorVersion);
	LogInfo(_T("Minor version: %d\r\n"), osvi.dwMinorVersion);
	LogInfo(_T("BuildNumber: %d\r\n"), osvi.dwBuildNumber);
	LogInfo(_T("PlatformId: %d\r\n"), osvi.dwPlatformId);
	LogInfo(_T("Version: %s\r\n"), osvi.szCSDVersion);
	LogInfo(_T("IsWOW64: %s\r\n"), IsWow64() ? _T("TRUE") : _T("FALSE"));
	LogInfo(_T("Admin: %s\r\n"), IsAdmin() ? _T("TRUE") : _T("FALSE"));
	LogInfo(_T("UAC supported: %s\r\n"), IsUacSupported() ? _T("TRUE") : _T("FALSE"));
	LogInfo(_T("Local admin: %s\r\n"), IsBuiltInAdmin() ? _T("TRUE") : _T("FALSE"));
}
