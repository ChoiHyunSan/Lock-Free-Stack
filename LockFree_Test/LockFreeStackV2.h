#pragma once

#include <Windows.h>
#include <vector>
#include <list>
#include <string>
#include "ObjectPool.h"

template<typename T>
class CLockFreeStackV2
{
public:
	CLockFreeStackV2();
	~CLockFreeStackV2();

private:
	struct Node
	{
		Node() : _next(nullptr) {}

		T     _data;
		Node* _next;
	};

public:
	bool Push(const T& data);
	bool Pop(T& data);

private:
	Node* _topNode;
	LONGLONG	_size = 0;

	MemoryPool<Node>* _nodePool;
};

template<typename T>
inline CLockFreeStackV2<T>::CLockFreeStackV2()
	: _topNode(nullptr)
{
	_nodePool = new MemoryPool<Node>(10000, true);
}

template<typename T>
inline CLockFreeStackV2<T>::~CLockFreeStackV2()
{
	delete _nodePool;
}

template<typename T>
inline bool CLockFreeStackV2<T>::Push(const T& data)
{
	Node* newNode = _nodePool->Alloc();		// 노드 풀에서 할당
	newNode->_data = data;

	for (;;)
	{
		Node* topNode = _topNode;
		newNode->_next = _topNode;
		if (InterlockedCompareExchangePointer(reinterpret_cast<PVOID*>(&_topNode), newNode, topNode) == topNode)
		{
			return true;
		}
	}
}

template<typename T>
inline bool CLockFreeStackV2<T>::Pop(T& data)
{
	for (;;)
	{
		Node* oldNode = _topNode;
		if (oldNode == nullptr) return false;

		Node* newTopNode = oldNode->_next;
		if (InterlockedCompareExchangePointer(reinterpret_cast<PVOID*>(&_topNode), newTopNode, oldNode) == oldNode)
		{
			data = oldNode->_data;
			_nodePool->Free(oldNode);	// 노드 풀에 노드 반환
			return true;
		}
	}
}
