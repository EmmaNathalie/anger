#pragma once
#include <Windows.h>
#include <tchar.h>

enum LogLevel
{
	LOG_TRACE, 
	LOG_DEBUG, 
	LOG_INFO, 
	LOG_WARN, 
	LOG_ERROR, 
	LOG_FATAL
};

enum LogOutput
{
	LOG_CONSOLE,
	LOG_DEBUGOUTPUT,
	LOG_FILE
};

#ifdef DEBUG_LOG
#define Debug(x, ...)	DebugFile(x, __VA_ARGS__)
#elif DEBUG_FILE
#define Debug(x, ...)	DebugLog(L ## x, ...)
#endif // DEBUG_LOG


VOID LogInit(LogOutput OutputType);

#define LogTrace(...) Log(LOG_TRACE, __FILE__, __LINE__, __VA_ARGS__)
#define LogDebug(...) Log(LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define LogInfo(...)  Log(LOG_INFO,  __FILE__, __LINE__, __VA_ARGS__)
#define LogWarn(...)  Log(LOG_WARN,  __FILE__, __LINE__, __VA_ARGS__)
#define LogError(...) Log(LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define LogFatal(...) Log(LOG_FATAL, __FILE__, __LINE__, __VA_ARGS__)


VOID Log(LogLevel iLogLevel, PCSTR pszFile, INT iLine, LPTSTR lpszFmt, ...);