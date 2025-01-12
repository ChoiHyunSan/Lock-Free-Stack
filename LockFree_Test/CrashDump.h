#pragma once

#pragma comment(lib, "DbgHelp.Lib")
#include <Windows.h>
#include <stdio.h>
#include <dbghelp.h>
#include <crtdbg.h>
#include "CLockFreeStack.h"

/************************

	   Crash Dump

************************/
// - 프로그램에서 에러가 발생할 때, 자동적으로 덤프파일을 뽑아내기 위한 클래스
//   전역으로 선언하여 사용한다.

#define DEBUG_BREAK DebugBreak();
#define DUMP

extern LogInfo* infoArray;
extern LONGLONG infoIndex;

class CrashDump
{
public:
	CrashDump()
	{
		_DumpCount = 0;

		_invalid_parameter_handler oldHandler, newHandler;
		newHandler = myInvalidParameterHandler;

		oldHandler = _set_invalid_parameter_handler(newHandler);		// crt 함수에 null포인터 등을 넣었을 때..
		_CrtSetReportMode(_CRT_WARN, 0);
		_CrtSetReportMode(_CRT_ASSERT, 0);
		_CrtSetReportMode(_CRT_ERROR, 0);

		_CrtSetReportHook(_custom_Report_hook);

		//--------------------------------------------------------------------------------
		// pure virtual function called 에러 핸들러를 사용자 정의 함수로 우회시킨다.
		//--------------------------------------------------------------------------------
		_set_purecall_handler(myPurecallHandler);

		SetHandlerDump();
	}

	static void Crash(void)
	{
		int* p = nullptr;
		*p = 0;
	}

	static LONG WINAPI MyExceptionFilter(__in PEXCEPTION_POINTERS pExceptionPointer)
	{
		int iWorkingMemory = 0;
		SYSTEMTIME	stNowTime;
		long DumpCount = InterlockedIncrement(&_DumpCount);

		//--------------------------------------------------------------------------------
		// 현재 날짜와 시간을 알아온다.
		//--------------------------------------------------------------------------------
		WCHAR filename[MAX_PATH];

		GetLocalTime(&stNowTime);
		wsprintf(filename, L"Dump_%d%02d%02d_%02d.%02d.%02d._%d.dmp",
			stNowTime.wYear, stNowTime.wMonth, stNowTime.wDay, stNowTime.wHour, stNowTime.wMinute, stNowTime.wSecond, DumpCount);

		wprintf(L"\n\n\n!!! Crash Error !!! %d.%d.%d / %d:%d:%d \n",
			stNowTime.wYear, stNowTime.wMonth, stNowTime.wDay, stNowTime.wHour, stNowTime.wMinute, stNowTime.wSecond);
		wprintf(L"Now Save dump file..  \n");

		HANDLE hDumpFile = ::CreateFile(filename, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hDumpFile != INVALID_HANDLE_VALUE)
		{
			_MINIDUMP_EXCEPTION_INFORMATION MinidumpExceptionInfomation;

			MinidumpExceptionInfomation.ThreadId = ::GetCurrentThreadId();
			MinidumpExceptionInfomation.ExceptionPointers = pExceptionPointer;
			MinidumpExceptionInfomation.ClientPointers = TRUE;

			MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hDumpFile, MiniDumpWithFullMemory, &MinidumpExceptionInfomation, NULL, NULL);
			CloseHandle(hDumpFile);

			wprintf(L"CrashDump Save Finish!");
		}

		wprintf(L"Now Save Log File .. Count : %lld \n", infoIndex);
		FILE* pFile = nullptr;
		WCHAR LogfileName[1000];
		wsprintf(LogfileName, L"LockFreeLog%d.txt", DumpCount);
		_wfopen_s(&pFile, LogfileName, L"w");
		if (pFile)
		{
			LONGLONG iIndex = infoIndex;
			int i = 0;
			if (iIndex > 1000000)
			{
				i = iIndex - 1000000;
			}

			for (;i <= iIndex; i++)
			{
				if (infoArray[i]._job)
				{
					fprintf(pFile, "(%d) PUSH ID : %d, NewNode : %lld, OldTop : %lld \n", i, infoArray[i]._ID, (long long)infoArray[i]._unp, (long long)infoArray[i]._tnp);
				}
				else
				{
					fprintf(pFile, "(%d) POP  ID : %d, OldNode : %lld, NewTop : %lld \n", i , infoArray[i]._ID, (long long)infoArray[i]._unp, (long long)infoArray[i]._tnp);
				}
			}
			fclose(pFile);
		}

		return EXCEPTION_EXECUTE_HANDLER;
	}

	static void SetHandlerDump()
	{
		SetUnhandledExceptionFilter(MyExceptionFilter);
	}

	// Invalid Parameter handler
	static void myInvalidParameterHandler(const wchar_t* expression, const wchar_t* funcion, const wchar_t* file, unsigned int line, uintptr_t pReserverd)
	{
		Crash();
	}

	static int _custom_Report_hook(int ireposttype, char* message, int* returnvalue)
	{
		Crash();
		return true;
	}

	static void myPurecallHandler(void)
	{
		Crash();
	}

	static long _DumpCount;
};


#ifdef DUMP

extern CrashDump g_crashDump;

#endif