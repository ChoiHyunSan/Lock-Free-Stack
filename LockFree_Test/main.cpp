#include "LockFreeStackTest.h"
#include  "CrashDump.h"
#include <queue>
#include "ObjectPool.h"
#include <list>

#include "LockFreeStackV1.h"
#include "LockFreeStackV2.h"
#include "LockFreeStackV3.h"

#define LOG_COUNT 500000000

using namespace std;

LogInfo* infoArray = new LogInfo[10000];
LONGLONG infoIndex = -1;

namespace LockFreeStackTest
{
	CLockFreeStackV3<int> stack;

	unsigned int TestThreadFunc(void* arg)
	{
		//LogInfo info;
		//info._ID = GetCurrentThreadId();

		for (;;)
		{
			int callCnt = 5;
			int value = 10;
			//info._job = 1;
			for (int cnt = 0; cnt < callCnt; cnt++)
			{
				stack.Push(value);
			}
			//info._job = 0;
			for (int cnt = 0; cnt < callCnt; cnt++)
			{
				if (stack.Pop(value) == false)
				{
					DebugBreak();
				}
			}
		}

		return 0;
	}

	void Test()
	{
		::wcout << L"Test Start" << endl;

		// 테스트에 사용할 쓰레드
		const int testThreadCount = 2;
		HANDLE hThread[testThreadCount];
		
		__int64 index = 0;
		for (; index < testThreadCount; index++)
		{
			hThread[index] = (HANDLE)_beginthreadex(nullptr, 0, TestThreadFunc, (void*)index, 0, nullptr);
			if (hThread[index] == nullptr)
			{
				::wcout << L"Thread handle null error" << endl;
				DebugBreak();
			}
		}

		int result = WaitForMultipleObjects(testThreadCount, hThread, TRUE, INFINITE);
		if (result == WAIT_ABANDONED_0 || result == WAIT_FAILED)
		{
			::wcout << L"Thread handle null error" << endl;
			DebugBreak();
		}

		for (int index = 0; index < testThreadCount; index++)
			CloseHandle(hThread[index]);

		::wcout << L"Test Complete" << endl;
		return;
	}
}

namespace ObjectPoolTest
{
	MemoryPool<__int32>* testPool;
	__int64				 numCnt = -1;
	int*				 Log;

	unsigned int TestThreadFunc(void* arg)
	{
		for (;;)
		{
			const int callCnt = 1500;
			int* ptr[callCnt];
			for (int i = 0; i < callCnt; i++)
			{
				ptr[i] = testPool->Alloc();
				if (ptr[i] == nullptr)
				{
					return false;
				}
				Log[InterlockedIncrement64(&numCnt) % 300000000] = (*ptr[i]);
			}

			for (int i = 0; i < callCnt; i++)
			{
				Log[InterlockedIncrement64(&numCnt) % 300000000] = (*ptr[i] * -1);
				if (testPool->Free(ptr[i]) == false)
				{
					return false;
				}
			}
		}
	}

	__int32* ptr[3000];
	void Test()
	{
		// TODO : Ready To Logging
		Log = new int[300000000];

		testPool = new MemoryPool<__int32>(3000, false);
		for (int cnt = 0; cnt < 3000; cnt++)
		{
			ptr[cnt] = testPool->Alloc();
			(*ptr[cnt]) = cnt;
		}
		for (int cnt = 0; cnt < 3000; cnt++)
		{
			testPool->Free(ptr[cnt]);
		}

		std::wcout << L"Test Start" << endl;

		const int testThreadCnt = 2;
		HANDLE hThread[testThreadCnt];

		for (int index = 0; index < testThreadCnt; index++)
		{
			hThread[index] = (HANDLE)_beginthreadex(nullptr, 0, ObjectPoolTest::TestThreadFunc, nullptr, 0, nullptr);
			if (hThread[index] == INVALID_HANDLE_VALUE)
			{
				std::wcout << L"Thread handle null error" << endl;
				DebugBreak();
			}
		}
		
		int result = WaitForMultipleObjects(testThreadCnt, hThread, TRUE, INFINITE);
		if (result == WAIT_ABANDONED_0 || result == WAIT_FAILED)
		{
			::wcout << L"Thread handle null error" << endl;
			DebugBreak();
		}

		for (int index = 0; index < testThreadCnt; index++)
			CloseHandle(hThread[index]);

		std::wcout << L"Test Complete" << endl;
		return;
	}
}

int main()
{
	LockFreeStackTest::Test();
}
