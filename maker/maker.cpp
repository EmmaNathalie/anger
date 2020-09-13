#include <Windows.h>
#include <tchar.h>

#include "builder.h"

#include <commctrl.h> 
#pragma comment(lib, "comctl32.lib") 

#if defined _M_IX86 
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"") 
#elif defined _M_IA64 
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='ia64' publicKeyToken='6595b64144ccf1df' language='*'\"") 
#elif defined _M_X64 
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"") 
#else 
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"") 
#endif 
#pragma endregion 

int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
	CBuilder	*Builder = new CBuilder();

	INITCOMMONCONTROLSEX cc = { sizeof(INITCOMMONCONTROLSEX), ICC_WIN95_CLASSES };
	if (!InitCommonControlsEx(&cc))
	{
		MessageBox(NULL, _T("Error initialize program"), _T("Error"), MB_OK);

		return 0;
	}

	LoadLibrary(_T("RichEd20.dll"));

	try
	{
		Builder->Show(hInstance);

		Builder->~CBuilder();
	}
	catch (...)
	{
		Builder->~CBuilder();
	}
	
	return 0;
}

