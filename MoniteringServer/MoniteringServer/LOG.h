#pragma once
#ifndef __LOG __H__
#define __LOG __H__

#include <time.h>

#define dfLOG_LEVEL_DEBUG		0
#define dfLOG_LEVEL_ERROR		1
#define dfLOG_LEVEL_SYSTEM		2

// ���ϰ� �߻��ϴ� �����ڵ�
#define ERROR_10054		10054
#define ERROR_10058		10058
#define ERROR_10060		10060

class Log
{
private:
	FILE* fp;
	int g_iLogLevel;				// ��� ���� ����� �α� ����
	wchar_t g_szLogBuff[1024];		// �α� ����� �ʿ��� �ӽ� ����
	CRITICAL_SECTION cs;
public:
	Log() {};

	Log(const WCHAR* szFileName)
	{
		fp = nullptr;
		g_iLogLevel = -1;
		ZeroMemory(g_szLogBuff, sizeof(wchar_t));

		time_t now;
		struct tm timeinfo;
		time(&now);
		localtime_s(&timeinfo, &now);

		// �α� ���� ��� ����
		wchar_t logFolderPath[MAX_PATH];
		GetCurrentDirectoryW(MAX_PATH, logFolderPath);
		wcscat_s(logFolderPath, L"\\LOG");

		// �α� ������ ���ٸ� ����
		if (!CreateDirectoryW(logFolderPath, NULL) && GetLastError() != ERROR_ALREADY_EXISTS)
		{
			// ���� ������ �����ϸ� ���� ó��
			wprintf(L"Failed to create LOG folder. Error code: %d\n", GetLastError());
			return;
		}

		// �α� ���� ������ ���� �ð� + szFileName���� ����
		wchar_t logFileName[MAX_PATH];
		wcsftime(logFileName, sizeof(logFileName) / sizeof(wchar_t), L"%Y%m%d%H%M%S_", &timeinfo);
		wcscat_s(logFileName, sizeof(logFileName) / sizeof(wchar_t), szFileName);
		wcscat_s(logFileName, sizeof(logFileName) / sizeof(wchar_t), L".txt");

		// �α� ���� ��ü ��� ����
		wchar_t fullPath[MAX_PATH];
		wcscpy_s(fullPath, logFolderPath);
		wcscat_s(fullPath, L"\\");
		wcscat_s(fullPath, logFileName);

		_wfopen_s(&fp, fullPath, L"w");
		if (fp == nullptr)
		{
			// ���� ������ �����ϸ� ���� ó��
			wprintf(L"Failed to open LOG File. Error code: %d\n", GetLastError());
			return;
		}

		InitializeCriticalSection(&cs);
	}

	void logger(int iLogLevel, int lines, const wchar_t* format, ...)
	{
		EnterCriticalSection(&cs);

		if (g_iLogLevel <= iLogLevel)
		{

			// ���� �ð��� �����ɴϴ�.
			SYSTEMTIME st;
			GetLocalTime(&st);

			wchar_t buffer[32];
			swprintf_s(buffer, L"%02d%02d%02d_%02d%02d%02d", st.wYear % 100, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);



			// �α� �޽��� �ۼ�
			va_list args;
			va_start(args, format);
			vswprintf_s(g_szLogBuff, format, args);
			va_end(args);

			if (iLogLevel == dfLOG_LEVEL_DEBUG)
				fwprintf_s(fp, L"%s [DEBUG : %d] : %s\n", buffer, lines, g_szLogBuff);
			else if (iLogLevel == dfLOG_LEVEL_ERROR)
				fwprintf_s(fp, L"%s [ERROR : %d] : %s\n", buffer, lines, g_szLogBuff);
			else if (iLogLevel == dfLOG_LEVEL_SYSTEM)
				fwprintf_s(fp, L"%s [SYSTEM : %d] : %s\n", buffer, lines, g_szLogBuff);

			fflush(fp);
		}
		LeaveCriticalSection(&cs);
	}

	~Log()
	{
		if (fp != nullptr)
			fclose(fp);

		DeleteCriticalSection(&cs);
	}
};

#endif