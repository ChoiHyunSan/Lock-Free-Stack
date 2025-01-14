#pragma once
#include <iostream>
#include <Windows.h>
using namespace std;

// ���� : DEBUG��带 Ű�ԵǸ� x64������ �۵��ȴ�. 

// #define OBJECTPOOL_DEBUG
#define KEY_BIT 47

template<class T>
class MemoryPool
{
public:
	MemoryPool(int iBlockNum, bool bPlacementNew = false);
	virtual	~MemoryPool();

	//////////////////////////////////////////////////////////////////////////
	// �� �ϳ��� �Ҵ�޴´�.  
	//
	// Parameters: ����.
	// Return: (DATA *) ����Ÿ �� ������.
	//////////////////////////////////////////////////////////////////////////
	T* Alloc(void);

	//////////////////////////////////////////////////////////////////////////
	// ������̴� ���� �����Ѵ�.
	//
	// Parameters: (DATA *) �� ������.
	// Return: (BOOL) TRUE, FALSE.
	//////////////////////////////////////////////////////////////////////////
	bool	Free(T* pData);

	//////////////////////////////////////////////////////////////////////////
	// ���� Ȯ�� �� �� ������ ��´�. (�޸�Ǯ ������ ��ü ����)
	//
	// Parameters: ����.
	// Return: (int) �޸� Ǯ ���� ��ü ����
	//////////////////////////////////////////////////////////////////////////
	int		GetCapacityCount(void) { return _capacity; }

	//////////////////////////////////////////////////////////////////////////
	// ���� ������� �� ������ ��´�.
	//
	// Parameters: ����.
	// Return: (int) ������� �� ����.
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

	// ���� ������� ��ȯ�� (�̻��) ������Ʈ ���� ����.
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
inline MemoryPool<T>::MemoryPool(int iBlockNum /* �ʱ� ���� ���� */, bool bPlacementNew /*���� �� ������ ȣ�� ����*/)
	: _placementNew(bPlacementNew)
{
	InitializeSRWLock(&lock);

	// �ʱ� ���� ������Ʈ�� ���� ��, ������ŭ �����.
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
