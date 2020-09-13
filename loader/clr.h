#pragma once
#include <Windows.h>
#include <tchar.h>

#include <metahost.h>  
#pragma comment(lib, "mscoree.lib") 

#define CHKHR(stmt, message)    do { hr = (stmt); if (FAILED(hr)){ LogFatal(message); LogFatal(_T(" HRESULT - 0x%8X\r\n"), hr); goto Cleanup;} } while (0)

HRESULT LoadData(SAFEARRAY **psa, PBYTE pData, DWORD dwDataSize);

HRESULT LoadCLR(PTSTR pszVersion, PBYTE pData, DWORD dwDataSize, PTSTR pszClassName);

HRESULT RunAgent(PBYTE pData, DWORD dwDataSize, PTSTR pszClassName);