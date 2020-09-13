#include "clr.h"
#include "..\\general\\log.h"

#define ARRAY

#pragma region Includes and Imports
#include <metahost.h>
#pragma comment(lib, "mscoree.lib")

// Import mscorlib.tlb (Microsoft Common Language Runtime Class Library).
#import "mscorlib.tlb" raw_interfaces_only			\
    	high_property_prefixes("_get","_put","_putref")		\
    	rename("ReportEvent", "InteropServices_ReportEvent")	\
	rename("or", "InteropServices_or")
using namespace mscorlib;
#pragma endregion



HRESULT LoadData(SAFEARRAY **psa, PBYTE pData, DWORD dwDataSize)
{
	HRESULT			hr;
	PVOID			pArrayData;
	SAFEARRAYBOUND rgsabound[1];

	rgsabound[0].lLbound = 0;
	rgsabound[0].cElements = dwDataSize;

	*psa = SafeArrayCreate(VT_UI1, 1, rgsabound);
	if (*psa == NULL)
	{
		return E_OUTOFMEMORY;
	}

	CHKHR(SafeArrayAccessData(*psa, (LPVOID *)&pArrayData), _T("Failed SafeArrayAccessData:"));

	CopyMemory(pArrayData, pData, dwDataSize);

	CHKHR(SafeArrayUnaccessData(*psa), _T("Failed: SafeArrayUnaccessData:"));

Cleanup:

	return hr;
}

HRESULT LoadCLR(PTSTR pszVersion, PBYTE pData, DWORD dwDataSize, PTSTR pszClassName)
{
	HRESULT hr;
	ICLRMetaHost       *pMetaHost = NULL;
	ICLRRuntimeInfo         *pCLRRuntimeInfo = NULL;
	ICorRuntimeHost         *pCorRuntimeHost = NULL;
	_TypePtr pType = NULL;
	_AssemblyPtr	pAssembly = NULL;
	_AppDomainPtr spDefaultAppDomain = NULL;
	SAFEARRAY *psa = NULL;
	BSTR bstrMethodName = NULL;
	SAFEARRAY *psaStaticMethodArgs = NULL;
	BSTR bstrClassName = NULL;

	CHKHR(CLRCreateInstance(CLSID_CLRMetaHost, IID_ICLRMetaHost, (LPVOID*)&pMetaHost), _T("CLRCreateInstance failed:"));

	CHKHR(pMetaHost->GetRuntime((LPCWSTR)pszVersion, IID_ICLRRuntimeInfo, (LPVOID *)&pCLRRuntimeInfo), _T("ICLRMetaHost::GetRuntime failed:"));



	BOOL bCLRIsLoadable;

	CHKHR(pCLRRuntimeInfo->IsLoadable(&bCLRIsLoadable), _T("ICLRRuntimeInfo::IsLoadable failed:"));
	
	if (!bCLRIsLoadable)
	{
		LogFatal(_T(".NET runtime %s cannot be loaded\r\n"), pszVersion);

		goto Cleanup;
	}

	CHKHR(pCLRRuntimeInfo->GetInterface(CLSID_CorRuntimeHost, IID_ICorRuntimeHost, (LPVOID*)&pCorRuntimeHost), _T("ICLRRuntimeInfo::GetInterface failed:"));

	CHKHR(pCorRuntimeHost->Start(), _T("CLR failed to start:"));

	IUnknown *pAppDomainThunk = NULL;

	CHKHR(pCorRuntimeHost->GetDefaultDomain(&pAppDomainThunk), _T("ICorRuntimeHost::GetDefaultDomain failed:"));

	

	CHKHR(pAppDomainThunk->QueryInterface(IID_PPV_ARGS(&spDefaultAppDomain)), _T("Failed to get default AppDomain:"));

	

	CHKHR(LoadData(&psa, pData, dwDataSize), _T("Error load payload:"));



	CHKHR(spDefaultAppDomain->Load_3(psa, &pAssembly), _T("Failed to load the assembly:"));

	

	

	bstrClassName = SysAllocString((const OLECHAR *)pszClassName);
	if (bstrClassName == NULL)
	{
		LogFatal(_T("Error SysAllocString()\r\n"));

		goto Cleanup;
	}

	
	CHKHR(pAssembly->GetType_2(bstrClassName, &pType), _T("Failed to get the Type interface:"));


	psaStaticMethodArgs = SafeArrayCreateVector(VT_VARIANT, 0, 1);
	LONG	index = 0;
	VARIANT	vtEmpty;
	VariantInit(&vtEmpty);
	VARIANT	vtArg;

#ifdef ARRAY
	
	

	SAFEARRAYBOUND rgsabound[1];

	rgsabound[0].lLbound = 0;
	rgsabound[0].cElements = 1;

	SAFEARRAY *strArray = SafeArrayCreate(VT_BSTR, 1, rgsabound);

	BSTR* dwArray = NULL;
	SafeArrayAccessData(strArray, (void**)&dwArray);

	dwArray[0] = SysAllocString(L"HKEY_LOCAL_MACHINE\\SYSTEM\\Test\\Service");

	SafeArrayUnaccessData(strArray);

	V_VT(&vtArg) = VT_BSTR | VT_ARRAY;
	V_ARRAY(&vtArg) = strArray;

	CHKHR(SafeArrayPutElement(psaStaticMethodArgs, &index, &vtArg), _T("SafeArrayPutElement failed:"));
	
#else
	V_VT(&vtArg) = VT_BSTR;
	V_BSTR(&vtArg) = SysAllocString(L"HKEY_LOCAL_MACHINE\\SYSTEM\\Test\\Service");
#endif

	CHKHR(SafeArrayPutElement(psaStaticMethodArgs, &index, &vtArg), _T("SafeArrayPutElement failed:"));

	bstrMethodName = SysAllocString(L"Main");
	if (bstrMethodName == NULL)
	{
		LogFatal(_T("Error SysAllocString()\r\n"));

		goto Cleanup;
	}

	 VARIANT	vtRet;
	

	CHKHR(pType->InvokeMember_3(bstrMethodName, (BindingFlags)(BindingFlags_InvokeMethod | BindingFlags_Static | BindingFlags_Public), NULL, vtEmpty,
		psaStaticMethodArgs, &vtRet), _T("Failed to invoke GetStringLength:"));

	VARIANT vtObject;

	CHKHR(pAssembly->CreateInstance(bstrClassName, &vtObject), _T("Assembly::CreateInstance failed:"));

Cleanup:
	SafeArrayDestroy(psa);

	if (bstrClassName)
	{
		SysFreeString(bstrClassName);
	}

	if (bstrMethodName)
	{
		SysFreeString(bstrMethodName);
	}

	if (pCorRuntimeHost)
	{
		pCorRuntimeHost->Release();
		pCorRuntimeHost = NULL;
	}

	if (pCLRRuntimeInfo) 
	{
		pCLRRuntimeInfo->Release(); 
		pCLRRuntimeInfo = NULL; 
	}

	if (pMetaHost) 
	{ 
		pMetaHost->Release();
		pMetaHost = NULL; 
	}

	if (psaStaticMethodArgs)
	{
		SafeArrayDestroy(psaStaticMethodArgs);
		psaStaticMethodArgs = NULL;
	}

	return hr;
}

HRESULT RunAgent(PBYTE pData, DWORD dwDataSize, PTSTR pszClassName)
{
	ICLRMetaHost *pMetaHost = NULL;
	IEnumUnknown *pEnum = NULL;

	HRESULT hr = CLRCreateInstance(CLSID_CLRMetaHost, IID_ICLRMetaHost, (LPVOID*)&pMetaHost);
	if (FAILED(hr))
	{
		LogFatal(_T("Error CLRCreateInstance() with error 0x%.08X\r\n"), hr);
		return hr;
	}

	hr = pMetaHost->EnumerateInstalledRuntimes(&pEnum);
	if (SUCCEEDED(hr))
	{
		while (true)
		{
			ULONG fetched = 0;
			IUnknown *pUnk = NULL;

			hr = pEnum->Next(1, &pUnk, &fetched);
			if (hr == S_OK)
			{
				ICLRRuntimeInfo *pClrRuntimeInfo = (ICLRRuntimeInfo *)pUnk;

				DWORD size = 60;
				TCHAR	szVersion[30];

				hr = pClrRuntimeInfo->GetVersionString((LPWSTR)szVersion, &size);

				LogInfo(_T("Version: %s\r\n"), szVersion);

				hr = LoadCLR(szVersion, pData, dwDataSize, pszClassName);

				LogTrace(_T("With version %s exit code 0x%.08X\r\n"), szVersion, hr);
			}
			else
			{
				break;
			}
		};

		pEnum->Release();
	}

	pMetaHost->Release();

	return hr;
}