#pragma once

#include <Windows.h>
#include <vector>
#include <list>
#include <string>
#include "ObjectPool.h"
#include "CLockFreeStack.h"

template<typename T>
class CLockFreeStackV3
{
	CLockFreeStackV3();
	~CLockFreeStackV3();

private:
	struct Node
	{
		Node() : _next(nullptr) {}

		T     _data;
		Node* _next;
	};

public:
	bool Push(const T& data);
	bool Pop(out T& data);

private:
	Node* _topNode;
	LONGLONG	_size = 0;
};

template<typename T>
inline CLockFreeStackV3<T>::CLockFreeStackV3()
	: _topNode(nullptr), _size(0)
{
	_nodePool = new MemoryPool<Node>(10000, true);
}

template<typename T>
inline CLockFreeStackV3<T>::~CLockFreeStackV3()
{
	delete _nodePool;
}

template<typename T>
inline bool CLockFreeStackV3<T>::Push(const T& data)
{
	Node* newNode = _nodePool->Alloc();
	newNode->_data = data;

	LONGLONG uniqueBit = (InterlockedIncrement64(&_key) << KEY_BIT);
	Node* newTopNode = reinterpret_cast<Node*>((LONGLONG)newNode | uniqueBit);

	for (;;)
	{
		Node* topNode = _topNode;
		newNode->_next = reinterpret_cast<Node*>((LONGLONG)topNode & _addressMask);
		if (InterlockedCompareExchangePointer(reinterpret_cast<PVOID*>(&_topNode), newTopNode, topNode) == topNode)
		{
			return true;
		}
	}
}

template<typename T>
inline bool CLockFreeStackV3<T>::Pop(T& data)
{
	for (;;)
	{
		Node* oldTopNode = _topNode;
		Node* oldNode = reinterpret_cast<Node*>((LONGLONG)_topNode & _addressMask);
		if (oldNode == nullptr) return false;

		LONGLONG uniqueBit = (InterlockedIncrement64(&_key) << KEY_BIT);
		Node* newTopNode = reinterpret_cast<Node*>((LONGLONG)(oldNode->_next) | uniqueBit);
		if (InterlockedCompareExchangePointer(reinterpret_cast<PVOID*>(&_topNode), newTopNode, oldTopNode) == oldTopNode)
		{
			data = oldNode->_data;
			_nodePool->Free(oldNode);
			return true;
		}
	}
}
