#pragma once
#include <iostream>
#include <Windows.h>
using namespace std;

// 주의 : DEBUG모드를 키게되면 x64에서만 작동된다. 

// #define OBJECTPOOL_DEBUG

template<class T>
class ObjectPool
{
public:
	ObjectPool(int iBlockNum, bool bPlacementNew = false);
	virtual	~ObjectPool();

	//////////////////////////////////////////////////////////////////////////
	// 블럭 하나를 할당받는다.  
	//
	// Parameters: 없음.
	// Return: (DATA *) 데이타 블럭 포인터.
	//////////////////////////////////////////////////////////////////////////
	T*	Alloc(void);

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

private:
	// 안전장치 기능
	

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

	// 스택 방식으로 반환된 (미사용) 오브젝트 블럭을 관리.
	st_BLOCK_NODE* _pFreeNode;

private:
	int _useCount;
	int _capacity;

	const bool _placementNew;
};

template<class T>
inline ObjectPool<T>::ObjectPool(int iBlockNum /* 초기 생성 개수 */, bool bPlacementNew /*꺼낼 때 생성자 호출 여부*/)
	: _placementNew(bPlacementNew)
{
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
	// 오브젝트 풀의 메모리인지 체크
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
		// 다른 오브젝트 풀 or 잘못된 할당 해제
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
