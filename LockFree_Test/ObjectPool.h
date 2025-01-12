#pragma once
#include <iostream>
#include <Windows.h>
using namespace std;

// ���� : DEBUG��带 Ű�ԵǸ� x64������ �۵��ȴ�. 

// #define OBJECTPOOL_DEBUG

template<class T>
class ObjectPool
{
public:
	ObjectPool(int iBlockNum, bool bPlacementNew = false);
	virtual	~ObjectPool();

	//////////////////////////////////////////////////////////////////////////
	// �� �ϳ��� �Ҵ�޴´�.  
	//
	// Parameters: ����.
	// Return: (DATA *) ����Ÿ �� ������.
	//////////////////////////////////////////////////////////////////////////
	T*	Alloc(void);

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

private:
	// ������ġ ���
	

private:
	struct st_BLOCK_NODE
	{
		st_BLOCK_NODE()
			: _next(nullptr)
		{

		}

#ifdef OBJECTPOOL_DEBUG
		void* front;
#endif

		T data;
		st_BLOCK_NODE* _next;

#ifdef OBJECTPOOL_DEBUG
		void* back;
#endif
	};

	// ���� ������� ��ȯ�� (�̻��) ������Ʈ ���� ����.
	st_BLOCK_NODE* _pFreeNode;

private:
	int _useCount;
	int _capacity;

	const bool _placementNew;
};

template<class T>
inline ObjectPool<T>::ObjectPool(int iBlockNum /* �ʱ� ���� ���� */, bool bPlacementNew /*���� �� ������ ȣ�� ����*/)
	: _placementNew(bPlacementNew)
{
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
	}
}

template<class T>
inline ObjectPool<T>::~ObjectPool()
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
inline T* ObjectPool<T>::Alloc(void)
{	
	for (;;)
	{
		if (_pFreeNode == nullptr)
		{
			st_BLOCK_NODE* newNode = new st_BLOCK_NODE;

#ifdef OBJECTPOOL_DEBUG
			newNode->front = newNode;
			newNode->back = newNode;
#endif
			return (T*)&(newNode->data);
		}

		st_BLOCK_NODE* oldNode = _pFreeNode;
		st_BLOCK_NODE* newTopNode = _pFreeNode->_next;
		
		if (InterlockedCompareExchangePointer(reinterpret_cast<PVOID*>(&_pFreeNode), newTopNode, oldNode) == oldNode)
		{
#ifdef OBJECTPOOL_DEBUG
			topNode->front = topNode;
			topNode->back = topNode;
#endif
			if (_placementNew)
			{
				st_BLOCK_NODE* placementNode = new(oldNode) st_BLOCK_NODE;
				return (T*)&(placementNode->data);
			}
			return (T*)&(oldNode->data);
		}
	}
}

template<class T>
inline bool ObjectPool<T>::Free(T* pData)
{
	// ������Ʈ Ǯ�� �޸����� üũ
#ifdef OBJECTPOOL_DEBUG
	st_BLOCK_NODE* freeNode = (st_BLOCK_NODE*)((long long)pData - sizeof(void*));
#endif 
#ifndef OBJECTPOOL_DEBUG
	st_BLOCK_NODE* freeNode = (st_BLOCK_NODE*)pData;
#endif
	if (_placementNew)
	{
		(freeNode)->~st_BLOCK_NODE();
	}

#ifdef OBJECTPOOL_DEBUG
	if (freeNode != freeNode->front || freeNode != freeNode->back)
	{
		// �ٸ� ������Ʈ Ǯ or �߸��� �Ҵ� ����
		return false;
	}

	freeNode->front = (void*)~(int)freeNode;
	freeNode->back = (void*)~(int)freeNode;
#endif

	for (;;)
	{
		st_BLOCK_NODE* topNode = _pFreeNode;
		freeNode->_next = topNode;

#ifdef OBJECTPOOL_DEBUG
		_pFreeNode = (st_BLOCK_NODE*)((long long)pData - sizeof(void*));
		reinterpret_cast<st_BLOCK_NODE*>((long long)pData - sizeof(void*))->_next = topNode;
#endif

#ifndef OBJECTPOOL_DEBUG
		if (InterlockedCompareExchangePointer(reinterpret_cast<PVOID*>(&_pFreeNode), freeNode, topNode) == topNode)
		{
			break;
		}
#endif
	}

	return true;
}
