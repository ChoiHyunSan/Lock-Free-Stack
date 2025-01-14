#pragma once
#include <iostream>
#include <Windows.h>
using namespace std;

// 주의 : DEBUG모드를 키게되면 x64에서만 작동된다. 

// #define OBJECTPOOL_DEBUG
#define KEY_BIT 47

template<class T>
class MemoryPool
{
public:
	MemoryPool(int iBlockNum, bool bPlacementNew = false);
	virtual	~MemoryPool();

	//////////////////////////////////////////////////////////////////////////
	// 블럭 하나를 할당받는다.  
	//
	// Parameters: 없음.
	// Return: (DATA *) 데이타 블럭 포인터.
	//////////////////////////////////////////////////////////////////////////
	T* Alloc(void);

	//////////////////////////////////////////////////////////////////////////
	// 사용중이던 블럭을 해제한다.
	//
	// Parameters: (DATA *) 블럭 포인터.
	// Return: (BOOL) TRUE, FALSE.
	//////////////////////////////////////////////////////////////////////////
	bool	Free(T* pData);

	//////////////////////////////////////////////////////////////////////////
	// 현재 확보 된 블럭 개수를 얻는다. (메모리풀 내부의 전체 개수)
	//
	// Parameters: 없음.
	// Return: (int) 메모리 풀 내부 전체 개수
	//////////////////////////////////////////////////////////////////////////
	int		GetCapacityCount(void) { return _capacity; }

	//////////////////////////////////////////////////////////////////////////
	// 현재 사용중인 블럭 개수를 얻는다.
	//
	// Parameters: 없음.
	// Return: (int) 사용중인 블럭 개수.
	//////////////////////////////////////////////////////////////////////////
	int		GetUseCount(void) { return _useCount; }
	int		GetAllocCount(void) { return _allocCount; }

private:
	struct st_BLOCK_NODE
	{
		st_BLOCK_NODE()
			: _next(nullptr)
		{

		}
		T data;
		st_BLOCK_NODE* _next;
	};

	// 스택 방식으로 반환된 (미사용) 오브젝트 블럭을 관리.
	st_BLOCK_NODE* _pFreeNode = nullptr;

private:
	LONG _useCount = 0;
	int _capacity;
	LONG _allocCount = 0;

	const bool _placementNew;

	LONGLONG			_key = 0;
	LONGLONG			_addressMask = 0x7FFFFFFFFFFF;

	SRWLOCK lock;
};

template<class T>
inline MemoryPool<T>::MemoryPool(int iBlockNum /* 초기 생성 개수 */, bool bPlacementNew /*꺼낼 때 생성자 호출 여부*/)
	: _placementNew(bPlacementNew)
{
	InitializeSRWLock(&lock);

	// 초기 생성 오브젝트가 있을 시, 개수만큼 만든다.
	if (iBlockNum > 0)
	{
		for (int cnt = 0; cnt < iBlockNum; cnt++)
		{
			st_BLOCK_NODE* newNode;
			if (_placementNew)
			{
				newNode = (st_BLOCK_NODE*)malloc(sizeof(st_BLOCK_NODE));
				newNode->_next = nullptr;
			}
			else
			{
				newNode = new st_BLOCK_NODE;
			}

			if (_pFreeNode == nullptr)
			{
				_pFreeNode = newNode;
				continue;
			}

			newNode->_next = _pFreeNode->_next;
			_pFreeNode->_next = newNode;
		}

		InterlockedAdd(&_allocCount, iBlockNum);
	}
}

template<class T>
inline MemoryPool<T>::~MemoryPool()
{
	while (_pFreeNode != nullptr)
	{
		st_BLOCK_NODE* oldNode = _pFreeNode;
		_pFreeNode = oldNode->_next;

		if (_placementNew)
		{
			free(oldNode);
		}
		else
		{
			delete oldNode;
		}
	}

	delete _pFreeNode;
}

template<class T>
inline T* MemoryPool<T>::Alloc(void)
{
	for (;;)
	{
		st_BLOCK_NODE* oldNode = _pFreeNode;
		st_BLOCK_NODE* oldMaskNode = reinterpret_cast<st_BLOCK_NODE*>((LONGLONG)oldNode & _addressMask);
		if (oldMaskNode == nullptr)
		{
			st_BLOCK_NODE* newNode = new st_BLOCK_NODE;
			InterlockedIncrement(&_allocCount);
			return (T*)&(newNode->data);
		}
		st_BLOCK_NODE* newTopNode = oldMaskNode->_next;

		LONGLONG uniqueBit = (InterlockedIncrement64(&_key) << KEY_BIT);
		st_BLOCK_NODE* newMaskTopNode = reinterpret_cast<st_BLOCK_NODE*>((LONGLONG)newTopNode | uniqueBit);

		if (InterlockedCompareExchangePointer(reinterpret_cast<PVOID*>(&_pFreeNode), newMaskTopNode, oldNode) == oldNode)
		{
			InterlockedDecrement(&_useCount);
			if (_placementNew)
			{
				st_BLOCK_NODE* placementNode = new(oldMaskNode) st_BLOCK_NODE;
				return (T*)&(placementNode->data);
			}
			return (T*)&(oldMaskNode->data);
		}
	}
}

template<class T>
inline bool MemoryPool<T>::Free(T* pData)
{
	st_BLOCK_NODE* freeNode = (st_BLOCK_NODE*)pData;
	if (_placementNew)
	{
		(freeNode)->~st_BLOCK_NODE();
	}

	LONGLONG uniqueBit = (InterlockedIncrement64(&_key) << KEY_BIT);
	st_BLOCK_NODE* freeMaskNode = reinterpret_cast<st_BLOCK_NODE*>((LONGLONG)freeNode | uniqueBit);

	for (;;)
	{
		st_BLOCK_NODE* topNode = _pFreeNode;
		st_BLOCK_NODE* topMaskNode = reinterpret_cast<st_BLOCK_NODE*>((LONGLONG)topNode & _addressMask);

		freeNode->_next = topMaskNode;
		if (InterlockedCompareExchangePointer(reinterpret_cast<PVOID*>(&_pFreeNode), freeMaskNode, topNode) == topNode)
		{
			InterlockedIncrement(&_useCount);
			break;
		}
	}
	return true;
}
