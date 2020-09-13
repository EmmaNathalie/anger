#include "log.h"
#include <Shlwapi.h>
#include <stdio.h>

#pragma comment(lib, "shlwapi.lib")

VOID DebugFile(LPCTSTR Format, DWORD dwFormatLength)
{
	HANDLE	hFile;
	TCHAR	szFile[MAX_PATH];
	DWORD	dwRW;

	GetModuleFileName(NULL, szFile, ARRAYSIZE(szFile));

	PathRenameExtension(szFile, _T(".log"));

	hFile = CreateFile(szFile, FILE_APPEND_DATA, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		return;
	}

	WriteFile(hFile, Format, dwFormatLength, &dwRW, NULL);

	CloseHandle(hFile);
}

VOID DebugLog(LPCTSTR Format, ...)
{
	TCHAR	szBuf[0x1000];
	va_list args;

	va_start(args, Format);

	DWORD dwBufSize = wvnsprintf(szBuf, ARRAYSIZE(szBuf), Format, args);
	if (dwBufSize > -1)
	{
		OutputDebugString(szBuf);
	}
	else
	{
		OutputDebugString(_T("Overflow string\r\n"));
	}

	va_end(args);
}

PCTSTR LevelColors[] = {
	_T("\x1b[94m"), 
	_T("\x1b[36m"), 
	_T("\x1b[32m"), 
	_T("\x1b[33m"),
	_T("\x1b[31m"), 
	_T("\x1b[35m")
};

PCTSTR LevelNames[] = {
	_T("TRACE"), 
	_T("DEBUG"), 
	_T("INFO"), 
	_T("WARN"), 
	_T("ERROR"), 
	_T("FATAL")
};

typedef struct _LOGGER
{
	INT		iLevel;
	INT		Output;
} LOGGER, *PLOGGER;

LOGGER Logger;

VOID LogInit(LogOutput OutputType)
{
	Logger.Output = OutputType;
	Logger.iLevel = 0;
}

VOID Log(LogLevel iLogLevel, PCSTR pszFile, INT iLine, LPTSTR lpszFmt, ...)
{
	if (Logger.iLevel > iLogLevel)
	{
		return;
	}

	SYSTEMTIME stTime;
	TCHAR		szBuf[32];
	TCHAR		szBuf1[1000];
	TCHAR		szOutput[0x1000];

	GetLocalTime(&stTime);

	wnsprintf(szBuf, ARRAYSIZE(szBuf), _T("%02d.%02d.%04d %02d:%02d:%02d"), stTime.wDay, stTime.wMonth, stTime.wYear, stTime.wHour, stTime.wMinute, stTime.wSecond);
	wnsprintf(szBuf1, ARRAYSIZE(szBuf1), _T("%s %-5s %S(%d): %s"), szBuf, LevelNames[iLogLevel], PathFindFileNameA(pszFile), iLine, lpszFmt);
	va_list args;
	va_start(args, lpszFmt);
	DWORD dwOutputLength = wvnsprintf(szOutput, ARRAYSIZE(szOutput), szBuf1, args);
	va_end(args);

	switch (Logger.Output)
	{
	case LOG_CONSOLE:
	{
		AllocConsole();
		HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
		AttachConsole(ATTACH_PARENT_PROCESS);
		DWORD dwWritten;
		WriteConsole(hConsole, szOutput, dwOutputLength, &dwWritten, NULL);
	}
		break;
	case LOG_DEBUGOUTPUT:
		OutputDebugString(szOutput);
		break;
	case LOG_FILE:
		DebugFile(szOutput, dwOutputLength * sizeof(TCHAR));
		break;
	default:
		break;
	}
}

