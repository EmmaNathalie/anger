#include <Windows.h>
#include <tchar.h>
#include <MSTask.h>
#include <MSTErr.h>
#include <CorError.h>
#include <taskschd.h>

#pragma comment(lib, "Mstask.lib")
#pragma comment(lib, "Taskschd.lib")

#pragma region TaskSchedulerV1

HRESULT ActivateTask(LPTSTR pszName)
{
	HRESULT			hr = S_OK;
	ITaskScheduler	*pTaskSched = NULL;

	if (SUCCEEDED(CoCreateInstance(CLSID_CTaskScheduler, 0, CLSCTX_INPROC_SERVER, IID_ITaskScheduler, (LPVOID *)&pTaskSched)))
	{
		ITask	*pTask = NULL;

		hr = pTaskSched->Activate(pszName, IID_ITask, (IUnknown **)&pTask) != COR_E_FILENOTFOUND;

		if (pTask)
		{
			pTask->Release();
		}
	}

	if (pTaskSched)
	{
		pTaskSched->Release();
	}

	return hr;
}

HRESULT StatusTask(LPTSTR pszName)
{
	HRESULT hr = S_OK;
	HRESULT hrStatus;
	ITaskScheduler	*pTaskSched = NULL;
	ITask			*pTask;

	hr = CoCreateInstance(CLSID_CTaskScheduler, 0, CLSCTX_SERVER, IID_ITaskScheduler, (LPVOID *)&pTaskSched);
	if (SUCCEEDED(hr))
	{
		pTask = NULL;

		hr = pTaskSched->Activate(pszName, IID_ITask, (IUnknown **)&pTask);
		if (SUCCEEDED(hr))
		{
			hrStatus = S_OK;

			hr = pTask->GetStatus(&hrStatus);
			if (SUCCEEDED(hr))
			{
				hr = hrStatus;
			}
		}

		if (pTask)
		{
			pTask->Release();
		}

	}

	if (pTaskSched)
	{
		pTaskSched->Release();
	}

	return hr;
}

HRESULT TerminateTask(LPTSTR pszName)
{
	HRESULT			hr;
	ITask			*pTask = NULL;
	ITaskScheduler	*pTaskSched = NULL;

	if (StatusTask(pszName) != 267009)
	{
		return 0;
	}

	hr = CoCreateInstance(CLSID_CTaskScheduler, 0, CLSCTX_SERVER, IID_ITaskScheduler, (LPVOID *)&pTaskSched);
	if (SUCCEEDED(hr))
	{
		hr = pTaskSched->Activate(pszName, IID_ITask, (IUnknown **)&pTask);
		if (SUCCEEDED(hr))
		{
			hr = pTask->Terminate();
		}

		if (pTask)
		{
			pTask->Release();
		}
	}

	if (pTaskSched)
	{
		pTaskSched->Release();
	}

	return hr;
}

HRESULT DeleteTask(LPTSTR pszName)
{
	HRESULT	hr;
	ITaskScheduler	*pTaskSched = NULL;

	hr = CoCreateInstance(CLSID_CTaskScheduler, 0, CLSCTX_SERVER, IID_ITaskScheduler, (LPVOID *)&pTaskSched);
	if (SUCCEEDED(hr))
	{
		TerminateTask(pszName);

		hr = pTaskSched->Delete(pszName);
		if (FAILED(hr) && (hr == COR_E_FILENOTFOUND))
		{
			hr = S_OK;
		}
	}

	if (pTaskSched)
	{
		pTaskSched->Release();
	}

	return hr;
}

HRESULT ConfigureTask(ITask *pTask, LPCTSTR pszInstallPath)
{
	HRESULT hr;

	hr = pTask->SetApplicationName(pszInstallPath);
	if (SUCCEEDED(hr))
	{
		hr = pTask->SetParameters(NULL);
		if (SUCCEEDED(hr))
		{
			hr = pTask->SetComment(NULL);
			if (SUCCEEDED(hr))
			{
				hr = pTask->SetAccountInformation(NULL, NULL);
			}
		}
	}

	return hr;
}

HRESULT NewTask(LPTSTR pszName, LPCTSTR pszInstallPath)
{
	HRESULT hr;
	ITaskScheduler	*pTaskSched = NULL;
	ITask			*pTask = NULL;

	if (ActivateTask(pszName))
	{
		DeleteTask(pszName);
	}

	hr = CoCreateInstance(CLSID_CTaskScheduler, 0, CLSCTX_SERVER, IID_ITaskScheduler, (LPVOID *)&pTaskSched);
	if (SUCCEEDED(hr))
	{
		hr = pTaskSched->NewWorkItem(pszName, CLSID_CTask, IID_ITask, (IUnknown **)&pTask);
		if (SUCCEEDED(hr))
		{
			ConfigureTask(pTask, pszInstallPath);
		}

		if (pTask)
		{
			pTask->Release();
		}
	}

	if (pTaskSched)
	{
		pTaskSched->Release();
	}

	return hr;
}

#pragma endregion

#pragma region TaskSchedulerV2

HRESULT InitTaskFolder(ITaskFolder **pRootFolder)
{
	HRESULT		hr;
	ITaskService	*pService = NULL;
	VARIANT	vtServerName,
		vtUser,
		vtDomain,
		vtPassword;

	hr = CoCreateInstance(CLSID_TaskScheduler, NULL, CLSCTX_INPROC_SERVER, IID_ITaskService, (LPVOID *)&pService);
	if (SUCCEEDED(hr))
	{
		VariantInit(&vtServerName);
		VariantInit(&vtUser);
		VariantInit(&vtDomain);
		VariantInit(&vtPassword);

		hr = pService->Connect(vtServerName, vtUser, vtDomain, vtPassword);

		VariantClear(&vtServerName);
		VariantClear(&vtUser);
		VariantClear(&vtDomain);
		VariantClear(&vtPassword);

		if (SUCCEEDED(hr))
		{
			BSTR bstrPath = SysAllocString(L"\\");

			hr = pService->GetFolder(bstrPath, pRootFolder);

			SysFreeString(bstrPath);
		}
	}

	if (pService)
	{
		pService->Release();
	}

	return hr;
}

HRESULT NewTask2(LPTSTR pszName, LPCTSTR pszInstallPath)
{
	PTSTR pszTask = _T("");


	HRESULT		hr;
	ITaskFolder	*pRootFolder = NULL;

	hr = InitTaskFolder(&pRootFolder);
	if (SUCCEEDED(hr))
	{
		BSTR	bstrApplicationPath = SysAllocString(pszInstallPath);

//		hr = pRootFolder->RegisterTask(bstrApplicationPath, pszTask, TASK_CREATE_OR_UPDATE, , , TASK_LOGON_S4U,);

		SysFreeString(bstrApplicationPath);
	}

	if (pRootFolder)
	{
		pRootFolder->Release();
	}

	return hr;
}

#pragma endregion

HRESULT RegisterTask(LPTSTR pszName, LPCTSTR pszInstallPath)
{
	HRESULT			hr;
	BOOL	bVersion2;
	ITaskService	*pService = NULL;

	if (SUCCEEDED(CoCreateInstance(CLSID_TaskScheduler, NULL, CLSCTX_INPROC_SERVER, IID_ITaskService, (LPVOID *)&pService)))
	{
		bVersion2 = TRUE;
	}

	if (pService)
	{
		pService->Release();
	}

	if (bVersion2)
	{
		hr = NewTask2(pszName, pszInstallPath);
	}
	else
	{
		hr = NewTask(pszName, pszInstallPath);
	}

	return hr;
}