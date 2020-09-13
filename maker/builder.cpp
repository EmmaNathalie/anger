#include "builder.h"

#include <Windows.h>
#include <Richedit.h>
#include <Shlwapi.h>


#pragma comment(lib, "shlwapi.lib")

CBuilder::CBuilder()
{
	hWndAgentName = NULL;

	GetCurrentDirectory(ARRAYSIZE(g_szCurDir), g_szCurDir);

	TCHAR szOutDir[MAX_PATH];

	PathCombine(szOutDir, g_szCurDir, _T("out"));

}


CBuilder::~CBuilder()
{
}


// Initialize GUI application
VOID CBuilder::Show(HINSTANCE hInstance)
{
	INT_PTR iRet = DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, (DLGPROC)DialogProc, (LPARAM)this);
}


INT_PTR CALLBACK CBuilder::DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CBuilder	*_this;
	
	switch (uMsg)
	{
	case WM_INITDIALOG:
		_this = (CBuilder *)lParam;
		_this->InitDialog(hwndDlg);

		SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR)_this);

		break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDCANCEL:
			//INT_PTR nResult = 0;
			EndDialog(hwndDlg, 0);
			break;
		case IDOK:
			_this = (CBuilder *)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
			if (_this->GenerateAgent())
			{
				MessageBox(NULL, _T("Good"), _T("Information"), MB_OK);

				//_this->Cleanup();
			}
			break;
		default:
			break;
		}
		break;
	case WM_DESTROY:
		EndDialog(hwndDlg, FALSE);
		return 1;
		break;
	default:
		DefWindowProc(hwndDlg, uMsg, wParam, lParam);
		break;
	}

	return 0;
}


BOOL CBuilder::GenerateAgent()
{
	BOOL	bRet = FALSE;
	PTSTR pszAgentName = NULL;
	PTSTR pszAgentVersion = NULL;
	PTSTR pszAgentTimeout = NULL;
	PTSTR pszServers = NULL;

	if (!GetAgentName(&pszAgentName))
	{
		MessageBox(NULL, _T("Agent name empty or error"), _T("Error"), MB_OK);
		goto Cleanup;
	}

	if (!GetAgentVersion(&pszAgentVersion))
	{
		MessageBox(NULL, _T("Agent version empty or error"), _T("Error"), MB_OK);
		goto Cleanup;
	}

	if (!GetAgentTimeout(&pszAgentTimeout))
	{
		MessageBox(NULL, _T("Agent timeout empty or error"), _T("Error"), MB_OK);
		goto Cleanup;
	}

	if (!GetServers(&pszServers))
	{
		MessageBox(NULL, _T("Servers empty or error"), _T("Error"), MB_OK);
		goto Cleanup;
	}

	INT iGenerationType = SendMessage(hWndComboBox, CB_GETCURSEL, (WPARAM)0, (LPARAM)0);

	switch (iGenerationType)
	{
	case 0:
		bRet = GenerateSchedulerInstall(pszAgentName, pszAgentTimeout, pszServers);
		break;
	case 1:
		// bRet = serviceinstall
		break;
	case 2:
		// bRet = svchostinstall
		break;
	}

Cleanup:
	if (pszAgentName != NULL)
	{
		free(pszAgentName);
		pszAgentName = NULL;
	}

	if (pszAgentVersion != NULL)
	{
		free(pszAgentVersion);
		pszAgentVersion = NULL;
	}

	if (pszAgentTimeout != NULL)
	{
		free(pszAgentTimeout);
		pszAgentTimeout = NULL;
	}

	if (pszServers != NULL)
	{
		free(pszServers);
		pszServers = NULL;
	}

	return bRet;
}


BOOL CBuilder::InitDialog(HWND hWndDialog)
{
	PTSTR	arrComboItems[] =
	{
		_T("Scheduler"),
		_T("Service"),
		_T("Svchost")
	};

	hWndComboBox = GetDlgItem(hWndDialog, IDC_COMBO1);
	hWndAgentName = GetDlgItem(hWndDialog, IDC_AGENT_NAME);
	hWndAgentVersion = GetDlgItem(hWndDialog, IDC_AGENT_VERSION);
	hWndServers = GetDlgItem(hWndDialog, IDC_SERVERS);
	hWndTimeout = GetDlgItem(hWndDialog, IDC_AGENT_TIMEOUT);

	//SendMessage(hwndDlg, );
	for (INT i = 0; i < ARRAYSIZE(arrComboItems); i++)
	{
		SendMessage(hWndComboBox, CB_ADDSTRING, 0, (LPARAM)arrComboItems[i]);
	}

	SendMessage(hWndComboBox, CB_SETCURSEL, (WPARAM)0, (LPARAM)0);
	return 0;
}


VOID CBuilder::StringToStringTable(PTSTR pszFilename, int nIDDString, PTSTR pszString)
{
	HMODULE hFile = LoadLibrary(pszFilename);
	if (hFile == NULL)
	{
		return;
	}

	HRSRC hRes = FindResource(hFile, MAKEINTRESOURCE(nIDDString / 16 + 1), RT_STRING);
	if (hRes == NULL)
	{
		return;
	}

	HGLOBAL hResLoad = LoadResource(hFile, hRes);
	if (hResLoad == NULL)
	{
		return;
	}

	PTSTR lpResLock = (PTSTR)LockResource(hResLoad);
	if (lpResLock == NULL)
	{
		return;
	}

	DWORD dwOldSize = SizeofResource(hFile, hRes);

	DWORD dwNewSize = dwOldSize + (lstrlen(pszString) + 1) * sizeof(TCHAR);
	PTSTR	pszNewResource = (PTSTR)malloc(dwNewSize);
	ZeroMemory(pszNewResource, dwNewSize);
	PTSTR   pszNewCursor = pszNewResource;

	PTSTR pszCursor = lpResLock;
	for (INT i = 0; i < 16; i++)
	{
		if (*pszCursor)
		{
			int iStrLength = *pszCursor;

			if (i == nIDDString % 16)
			{
				*pszNewCursor = lstrlen(pszString);
				CopyMemory(++pszNewCursor, pszString, lstrlen(pszString) * sizeof(TCHAR));
				pszCursor += iStrLength + 1;
				pszNewCursor += lstrlen(pszString);
			}
			else
			{
				CopyMemory(pszNewCursor, pszCursor, (iStrLength + 1) * sizeof(TCHAR));
				pszCursor += iStrLength + 1;
				//*pszNewCursor = *pszCursor;
				
				pszNewCursor += iStrLength + 1;
			}
		}
		else
		{
			pszCursor++;
			pszNewCursor++;
		}
	}

	//DWORD dwCheck = (PBYTE)pszCursor - (PBYTE)lpResLock;

	dwNewSize = (PBYTE)pszNewCursor - (PBYTE)pszNewResource;


	HANDLE hUpdateRes = BeginUpdateResource(pszFilename, FALSE);
	if (hUpdateRes == NULL) {

		return;
	}

	BOOL result = UpdateResource(hUpdateRes,
		RT_STRING,
		MAKEINTRESOURCE(nIDDString / 16 + 1),
		MAKELANGID(LANG_RUSSIAN, SUBLANG_DEFAULT),
		(void*)pszNewResource,
		dwNewSize);
	if (!result) {
		return;
	}
	BOOL commit = EndUpdateResource(hUpdateRes, FALSE);
}


BOOL CBuilder::GetServers(LPTSTR *ppszServers)
{
	BOOL bRet = FALSE;

	GETTEXTLENGTHEX	ServersLength;
	ServersLength.flags = GTL_DEFAULT | GTL_USECRLF;
	ServersLength.codepage = 1200;

	LRESULT iLength = SendMessage(hWndServers, EM_GETTEXTLENGTHEX, (WPARAM)&ServersLength, 0);
	if (iLength == 0)
	{
		return bRet;
	}

	iLength += 1 * sizeof(TCHAR);

	GETTEXTEX Servers;

	Servers.codepage = 1200;
	Servers.flags = GT_DEFAULT;
	Servers.lpDefaultChar = NULL;
	Servers.lpUsedDefChar = NULL;
	Servers.cb = iLength * sizeof(TCHAR);

	*ppszServers = (PTSTR)malloc(iLength);
	if (*ppszServers == NULL)
	{
		return bRet;
	}

	SendMessage(hWndServers, EM_GETTEXTEX, (WPARAM)&Servers, (LPARAM)*ppszServers);

	PTSTR pszCursor;
	while ((pszCursor = StrStr(*ppszServers, L"\r")) != NULL)
	{
		*pszCursor = ';';
	}

	return TRUE;
}


BOOL CBuilder::GetEditString(HWND hWndEdit, PTSTR* ppzString)
{
	INT cbStringLength = GetWindowTextLength(hWndEdit);
	if (cbStringLength == 0)
	{
		return FALSE;
	}

	*ppzString = (PTSTR)malloc((cbStringLength + 1) * sizeof(TCHAR));
	if (*ppzString == NULL)
	{
		return FALSE;
	}

	GetWindowText(hWndEdit, *ppzString, (cbStringLength + 1) * sizeof(TCHAR));

	return TRUE;
}


BOOL CBuilder::GetAgentName(PTSTR* ppszAgentName)
{
	return GetEditString(hWndAgentName, ppszAgentName);
}


BOOL CBuilder::GetAgentVersion(PTSTR* ppszAgentVersion)
{
	return GetEditString(hWndAgentVersion, ppszAgentVersion);
}


BOOL CBuilder::GetAgentTimeout(PTSTR* ppszAgentTimeout)
{
	return GetEditString(hWndTimeout, ppszAgentTimeout);
}


BOOL CBuilder::BinToBinaryData(PTSTR pszInFile, PTSTR pszOutFile, INT nIDDString)
{
	HANDLE	hFile = CreateFile(pszInFile, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		return FALSE;
	}

	DWORD dwFileSize = GetFileSize(hFile, NULL);
	PVOID pData = malloc(dwFileSize);
	if (!pData)
	{
		CloseHandle(hFile);

		return FALSE;
	}

	PDWORD pdwRW = 0;
	ReadFile(hFile, pData, dwFileSize, pdwRW, NULL);

	CloseHandle(hFile);

	HANDLE hUpdateRes = BeginUpdateResource(pszOutFile, FALSE);
	if (hUpdateRes == NULL) {
		return FALSE;
	}

	// Add our new resource
	BOOL result = UpdateResource(hUpdateRes,
		_T("RT_RCDATA"),
		MAKEINTRESOURCE(nIDDString),
		MAKELANGID(LANG_RUSSIAN, SUBLANG_DEFAULT),
		(void*)pData,
		dwFileSize);
	if (!result) {
		return FALSE;
	}
	BOOL commit = EndUpdateResource(hUpdateRes, FALSE);

	free(pData);

	return TRUE;
}

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


BOOL DumpBinResource(INT ResourceType, LPTSTR pszFileName)
{
	BOOL bRet = FALSE;
	HANDLE	hFile = INVALID_HANDLE_VALUE;
	PVOID OldValue = NULL;


	if (IsWow64())
	{
		if (Wow64DisableWow64FsRedirection(&OldValue))
		{
			hFile = CreateFile(pszFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

			if (FALSE == Wow64RevertWow64FsRedirection(OldValue))
			{
				return FALSE;
			}
		}
	}
	else
	{
		hFile = CreateFile(pszFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	}
	
	if (hFile == INVALID_HANDLE_VALUE)
	{
		return FALSE;
	}

	PBYTE	pbData = NULL;
	DWORD	dwDataSize = 0;

	pbData = MapResource(_T("RT_RCDATA"), ResourceType, &dwDataSize);
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

			DeleteFile(pszFileName);
		}
	}

	return bRet;
}

#define IDS_INSTALLER_CFG_PATH	102
#define IDS_INSTALLER_NAME		103
#define IDS_INSTALLER_TIMEOUT	104
#define	IDS_INSTALLER_SERVERS	105

BOOL CBuilder::GenerateSchedulerInstall(PTSTR pszAgentName, PTSTR pszAgentTimeout, PTSTR pszServers)
{
	TCHAR	szCurDir[MAX_PATH];
	TCHAR	szLoaderPath86[MAX_PATH];
	TCHAR	szLoaderPath64[MAX_PATH];
	TCHAR	szInstallerPath[MAX_PATH];

	GetCurrentDirectory(ARRAYSIZE(szCurDir), szCurDir);

	PathCombine(szLoaderPath86, szCurDir, _T("loader_86.exe"));
	PathCombine(szLoaderPath64, szCurDir, _T("loader_64.exe"));
	PathCombine(szInstallerPath, szCurDir, _T("installer.exe"));

	



	// Dump installer
	DumpBinResource(IDR_INSTALLER_SCHEDULER_86, szInstallerPath);
	// update agent (C#) resource
	// dump loader (all)
	DumpBinResource(IDR_LOADER_SCHEDULER_86, szLoaderPath86);
	DumpBinResource(IDR_LOADER_SCHEDULER_64, szLoaderPath64);
	// update loader_x86 (cfg path)
	//update loader_x64 (cfg path)
	StringToStringTable(szLoaderPath86, IDS_INSTALLER_CFG_PATH, pszAgentName);
	StringToStringTable(szLoaderPath64, IDS_INSTALLER_CFG_PATH, pszAgentName);

	StringToStringTable(szLoaderPath86, IDS_INSTALLER_TIMEOUT, pszAgentTimeout);
	StringToStringTable(szLoaderPath64, IDS_INSTALLER_TIMEOUT, pszAgentTimeout);

	StringToStringTable(szLoaderPath86, IDS_INSTALLER_SERVERS, pszServers);
	StringToStringTable(szLoaderPath64, IDS_INSTALLER_SERVERS, pszServers);


	// update all loaders in installer
	BinToBinaryData(szLoaderPath86, szInstallerPath, 103);
	BinToBinaryData(szLoaderPath64, szInstallerPath, 104);


	return TRUE;
}







//VOID UpdateFileResource()
//{
//	HGLOBAL hResLoad;   // handle to loaded resource
//	HMODULE hExe;       // handle to existing .EXE file
//	HRSRC hRes;         // handle/ptr. to res. info. in hExe
//	HANDLE hUpdateRes;  // update resource handle
//	LPVOID lpResLock;   // pointer to resource data
//	BOOL result;
//#define IDD_HAND_ABOUTBOX   103
//#define IDD_FOOT_ABOUTBOX   110
//
//	// Load the .EXE file that contains the dialog box you want to copy.
//	hExe = LoadLibrary(TEXT("hand.exe"));
//	if (hExe == NULL)
//	{
//		ErrorHandler(TEXT("Could not load exe."));
//		return;
//	}
//
//	// Locate the dialog box resource in the .EXE file.
//	hRes = FindResource(hExe, MAKEINTRESOURCE(IDD_HAND_ABOUTBOX), RT_DIALOG);
//	if (hRes == NULL)
//	{
//		ErrorHandler(TEXT("Could not locate dialog box."));
//		return;
//	}
//
//	// Load the dialog box into global memory.
//	hResLoad = LoadResource(hExe, hRes);
//	if (hResLoad == NULL)
//	{
//		ErrorHandler(TEXT("Could not load dialog box."));
//		return;
//	}
//
//	// Lock the dialog box into global memory.
//	lpResLock = LockResource(hResLoad);
//	if (lpResLock == NULL)
//	{
//		ErrorHandler(TEXT("Could not lock dialog box."));
//		return;
//	}
//
//	// Open the file to which you want to add the dialog box resource.
//	hUpdateRes = BeginUpdateResource(TEXT("foot.exe"), FALSE);
//	if (hUpdateRes == NULL)
//	{
//		ErrorHandler(TEXT("Could not open file for writing."));
//		return;
//	}
//
//	// Add the dialog box resource to the update list.
//	result = UpdateResource(hUpdateRes,    // update resource handle
//		RT_DIALOG,                         // change dialog box resource
//		MAKEINTRESOURCE(IDD_FOOT_ABOUTBOX),         // dialog box id
//		MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),  // neutral language
//		lpResLock,                         // ptr to resource info
//		SizeofResource(hExe, hRes));       // size of resource info
//
//	if (result == FALSE)
//	{
//		ErrorHandler(TEXT("Could not add resource."));
//		return;
//	}
//
//	// Write changes to FOOT.EXE and then close it.
//	if (!EndUpdateResource(hUpdateRes, FALSE))
//	{
//		ErrorHandler(TEXT("Could not write changes to file."));
//		return;
//	}
//
//	// Clean up.
//	if (!FreeLibrary(hExe))
//	{
//		ErrorHandler(TEXT("Could not free executable."));
//		return;
//	}
//}


VOID UpdateStringTableResource()
{

}
