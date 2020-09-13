#include "service.h"
#include "log.h"


DWORD DoInstallSvc(LPCTSTR pszInstallPath)
{
	TCHAR szPath[MAX_PATH];
	SC_HANDLE schSCManager = NULL;
	SC_HANDLE schService = NULL;
	DWORD	dwError = ERROR_SUCCESS;

	dwError = GetModuleFileName(NULL, szPath, ARRAYSIZE(szPath));
	if (dwError == 0)
	{
		LogTrace(_T("GetModuleFileName failed w/err 0x%08lx\n"), GetLastError());
		goto Cleanup;
	}

	schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT | SC_MANAGER_CREATE_SERVICE);
	if (schSCManager == NULL)
	{
		dwError = GetLastError();
		LogTrace(_T("OpenSCManager failed w/err 0x%08lx\n"), dwError);
		goto Cleanup;
	}

	schService = CreateService(schSCManager, SERVICE_NAME, SERVICE_DISPLAY_NAME, SERVICE_QUERY_STATUS, SERVICE_WIN32_OWN_PROCESS, 
								SERVICE_START_TYPE, SERVICE_ERROR_NORMAL, pszInstallPath, NULL, NULL, SERVICE_DEPENDENCIES, SERVICE_ACCOUNT,
								SERVICE_PASSWORD);
	if (schService == NULL)
	{
		dwError = GetLastError();
		LogTrace(_T("CreateService failed w/err 0x%08lx\n"), dwError);
		goto Cleanup;
	}

	LogTrace(_T("%s is installed.\n"), SERVICE_NAME);

Cleanup:
	if (schSCManager)
	{
		CloseServiceHandle(schSCManager);
		schSCManager = NULL;
	}
	if (schService)
	{
		CloseServiceHandle(schService);
		schService = NULL;
	}

	return dwError;
}


DWORD DoUninstallSvc()
{
	SC_HANDLE schSCManager = NULL;
	SC_HANDLE schService = NULL;
	SERVICE_STATUS ssSvcStatus = {};
	DWORD	dwError = ERROR_SUCCESS;

	schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
	if (schSCManager == NULL)
	{
		dwError = GetLastError();
		LogTrace(L"OpenSCManager failed w/err 0x%08lx\n", dwError);
		goto Cleanup;
	}

	schService = OpenService(schSCManager, SERVICE_NAME, SERVICE_STOP | SERVICE_QUERY_STATUS | DELETE);
	if (schService == NULL)
	{
		dwError = GetLastError();
		LogTrace(L"OpenService failed w/err 0x%08lx\n", dwError);
		goto Cleanup;
	}

	if (ControlService(schService, SERVICE_CONTROL_STOP, &ssSvcStatus))
	{
		LogTrace(L"Stopping %s.", SERVICE_NAME);
		Sleep(1000);

		while (QueryServiceStatus(schService, &ssSvcStatus))
		{
			if (ssSvcStatus.dwCurrentState == SERVICE_STOP_PENDING)
			{
				LogTrace(L".");
				Sleep(1000);
			}
			else break;
		}

		if (ssSvcStatus.dwCurrentState == SERVICE_STOPPED)
		{
			LogTrace(L"\n%s is stopped.\n", SERVICE_NAME);
		}
		else
		{
			LogTrace(L"\n%s failed to stop.\n", SERVICE_NAME);
		}
	}

	if (!DeleteService(schService))
	{
		dwError = GetLastError();
		LogTrace(L"DeleteService failed w/err 0x%08lx\n", dwError);
		goto Cleanup;
	}

	LogTrace(L"%s is removed.\n", SERVICE_NAME);

Cleanup:
	if (schSCManager)
	{
		CloseServiceHandle(schSCManager);
		schSCManager = NULL;
	}
	if (schService)
	{
		CloseServiceHandle(schService);
		schService = NULL;
	}

	return dwError;
}


DWORD DoStartSvc()
{
	SERVICE_STATUS_PROCESS ssStatus;
	DWORD dwOldCheckPoint;
	DWORD dwStartTickCount;
	DWORD dwWaitTime;
	DWORD dwBytesNeeded;
	DWORD	dwError = ERROR_SUCCESS;
	SC_HANDLE schService = NULL;

	SC_HANDLE schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (NULL == schSCManager)
	{
		dwError = GetLastError();
		LogTrace(L"OpenSCManager failed (%d)\n", dwError);
		goto Cleanup;
	}

	
	
	schService = OpenService(schSCManager, SERVICE_NAME, SERVICE_ALL_ACCESS);
	if (schService == NULL)
	{
		dwError = GetLastError();
		LogTrace(L"OpenService failed (%d)\n", dwError);
		goto Cleanup;
	}

	if (!QueryServiceStatusEx(schService, SC_STATUS_PROCESS_INFO, (LPBYTE)&ssStatus, sizeof(SERVICE_STATUS_PROCESS), &dwBytesNeeded))
	{
		dwError = GetLastError();
		LogTrace(L"QueryServiceStatusEx failed (%d)\n", dwError);
		goto Cleanup;
	}

	if (ssStatus.dwCurrentState != SERVICE_STOPPED && ssStatus.dwCurrentState != SERVICE_STOP_PENDING)
	{
		LogTrace(L"Cannot start the service because it is already running\n");
		goto Cleanup;
	}

	dwStartTickCount = GetTickCount();
	dwOldCheckPoint = ssStatus.dwCheckPoint;

	while (ssStatus.dwCurrentState == SERVICE_STOP_PENDING)
	{

		dwWaitTime = ssStatus.dwWaitHint / 10;

		if (dwWaitTime < 1000)
			dwWaitTime = 1000;
		else if (dwWaitTime > 10000)
			dwWaitTime = 10000;

		Sleep(dwWaitTime);

		if (!QueryServiceStatusEx(schService, SC_STATUS_PROCESS_INFO, (LPBYTE)&ssStatus,
			sizeof(SERVICE_STATUS_PROCESS), &dwBytesNeeded))
		{
			dwError = GetLastError();
			LogTrace(L"QueryServiceStatusEx failed (%d)\n", dwError);
			goto Cleanup;
		}

		if (ssStatus.dwCheckPoint > dwOldCheckPoint)
		{
			dwStartTickCount = GetTickCount();
			dwOldCheckPoint = ssStatus.dwCheckPoint;
		}
		else
		{
			if (GetTickCount() - dwStartTickCount > ssStatus.dwWaitHint)
			{
				LogTrace(L"Timeout waiting for service to stop\n");
				goto Cleanup;
			}
		}
	}

	if (!StartService(schService, 0, NULL))
	{
		dwError = GetLastError();
		LogTrace(L"StartService failed (%d)\n", dwError);
		goto Cleanup;
	}
	else LogInfo(L"Service start pending...\n");

	if (!QueryServiceStatusEx(schService, SC_STATUS_PROCESS_INFO, (LPBYTE)&ssStatus, sizeof(SERVICE_STATUS_PROCESS), &dwBytesNeeded))
	{
		dwError = GetLastError();
		LogTrace(L"QueryServiceStatusEx failed (%d)\n", dwError);
		goto Cleanup;
	}

	dwStartTickCount = GetTickCount();
	dwOldCheckPoint = ssStatus.dwCheckPoint;

	while (ssStatus.dwCurrentState == SERVICE_START_PENDING)
	{
		dwWaitTime = ssStatus.dwWaitHint / 10;

		if (dwWaitTime < 1000)
			dwWaitTime = 1000;
		else if (dwWaitTime > 10000)
			dwWaitTime = 10000;

		Sleep(dwWaitTime);


		if (!QueryServiceStatusEx(schService, SC_STATUS_PROCESS_INFO, (LPBYTE)&ssStatus, sizeof(SERVICE_STATUS_PROCESS), &dwBytesNeeded))
		{
			LogTrace(L"QueryServiceStatusEx failed (%d)\n", GetLastError());
			break;
		}

		if (ssStatus.dwCheckPoint > dwOldCheckPoint)
		{
			dwStartTickCount = GetTickCount();
			dwOldCheckPoint = ssStatus.dwCheckPoint;
		}
		else
		{
			if (GetTickCount() - dwStartTickCount > ssStatus.dwWaitHint)
			{
				break;
			}
		}
	}

	if (ssStatus.dwCurrentState == SERVICE_RUNNING)
	{
		LogInfo(L"Service started successfully.\n");
	}
	else
	{
		LogInfo(L"Service not started. \n");
		LogInfo(L"  Current State: %d\n", ssStatus.dwCurrentState);
		LogInfo(L"  Exit Code: %d\n", ssStatus.dwWin32ExitCode);
		LogInfo(L"  Check Point: %d\n", ssStatus.dwCheckPoint);
		LogInfo(L"  Wait Hint: %d\n", ssStatus.dwWaitHint);
	}


Cleanup:
	if (schSCManager)
	{
		CloseServiceHandle(schSCManager);
		schSCManager = NULL;
	}
	if (schService)
	{
		CloseServiceHandle(schService);
		schService = NULL;
	}

	return dwError;
}