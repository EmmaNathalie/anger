#include "uninstall.h"

#include "scheduler.h"
#include "service.h"
#include "svchost.h"
#include "..\\general\\log.h"
#include "resource.h"



BOOL RegDelnodeRecurse(HKEY hKeyRoot, LPTSTR lpSubKey)
{
	LONG lResult;
	DWORD dwSize;
	TCHAR szName[MAX_PATH];
	HKEY hKey;
	FILETIME ftWrite;

	lResult = RegDeleteKey(hKeyRoot, lpSubKey);
	if (lResult == ERROR_SUCCESS)
	{
		LogTrace(_T("Delete cfg success\r\n"));
		return TRUE;
	}

	lResult = RegOpenKeyEx(hKeyRoot, lpSubKey, 0, KEY_READ, &hKey);
	if (lResult != ERROR_SUCCESS)
	{
		if (lResult == ERROR_FILE_NOT_FOUND) {
			LogTrace(_T("Key not found.\n"));
			return TRUE;
		}
		else {
			LogTrace(_T("Error opening key.\n"));
			return FALSE;
		}
	}

	dwSize = MAX_PATH;
	lResult = RegEnumKeyEx(hKey, 0, szName, &dwSize, NULL, NULL, NULL, &ftWrite);
	if (lResult == ERROR_SUCCESS)
	{
		do {

			RegDeleteValue(hKey, szName);

			dwSize = MAX_PATH;

			lResult = RegEnumKeyEx(hKey, 0, szName, &dwSize, NULL, NULL, NULL, &ftWrite);

		} while (lResult == ERROR_SUCCESS);
	}

	RegCloseKey(hKey);

	lResult = RegDeleteKey(hKeyRoot, lpSubKey);
	if (lResult == ERROR_SUCCESS)
	{
		LogTrace(_T("Success delete key\r\n"));

		return TRUE;
	}
		

	return FALSE;
}

VOID Uninstall(LPTSTR pszInstallPath)
{
	LogTrace(_T("\r\nUninstall processed\r\n\r\n"));

#pragma region Step 1
	LogTrace(_T("Step 1. Delete config\r\n"));

	TCHAR	szRegPath[MAX_PATH];

	if (LoadString(NULL, IDS_CFG_PATH, szRegPath, ARRAYSIZE(szRegPath)) > 0)
	{
		RegDelnodeRecurse(HKEY_CURRENT_USER, szRegPath);

		RegDelnodeRecurse(HKEY_LOCAL_MACHINE, szRegPath);
	}
	else
	{
		LogTrace(_T("Error load cfg path %d\r\n"), GetLastError());
	}

#pragma endregion

#pragma region Step 2
	LogTrace(_T("Step 2. Delete loader\r\n"));

	if (DeleteFile(pszInstallPath))
	{
		LogTrace(_T("Success delete!\r\n"));
	}
	else
	{
		LogTrace(_T("Error delete %d\r\n"), GetLastError());
	}
#pragma endregion

#pragma region Step 3
	LogTrace(_T("Step 3. Delete from startup\r\n"));

	//if (LoadString(NULL, IDS_TASK, szTaskName, ARRAYSIZE(szTaskName)) > 0)
	{
		HRESULT hr = DeleteTask(_T("Update"));
		if (SUCCEEDED(hr))
		{
			LogTrace(_T("Success delete\r\n"));
		}
		else
		{
			LogTrace(_T("Error delete 0x%.8X\r\n"), hr);
		}
	}
	//else
	{
	//	LogTrace(_T("Error load task name %d\r\n"), GetLastError());
	}
#pragma endregion

	LogTrace(_T("Finish uninstall\r\n"));
}