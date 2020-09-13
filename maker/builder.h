#pragma once
#include <Windows.h>
#include <tchar.h>

#include "resource.h"

class CBuilder
{
public:
	CBuilder();
	~CBuilder();
	// Initialize GUI application
	VOID Show(HINSTANCE hInstance);
private:
	static INT_PTR CALLBACK DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
protected:
	HWND hWndComboBox;
	HWND hWndTimeout;
public:
	HWND hWndAgentName;
protected:
	HWND hWndAgentVersion;
	HWND hWndServers;
	BOOL GenerateAgent();
public:
	BOOL InitDialog(HWND hWndDialog);
private:
	TCHAR g_szCurDir[MAX_PATH];
	// save config value to string table
	VOID StringToStringTable(PTSTR pszFilename, int nIDDString, PTSTR pszString);
	BOOL BinToBinaryData(PTSTR pszInFile, PTSTR pszOutFile, INT nIDDString);

	BOOL GetServers(LPTSTR *ppszServers);
	BOOL GetEditString(HWND hWndEdit, PTSTR* ppzString);
	BOOL GetAgentName(PTSTR* ppszAgentName);
	BOOL GetAgentVersion(PTSTR* ppszAgentVersion);
	BOOL GetAgentTimeout(PTSTR* ppszAgentTimeout);
	
	BOOL GenerateSchedulerInstall(PTSTR pszAgentName, PTSTR pszAgentTimeout, PTSTR pszServers);
};

