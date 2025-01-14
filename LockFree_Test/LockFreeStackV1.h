#pragma once

#include <Windows.h>
#include <vector>
#include <list>
#include <string>

template<typename T>
class CLockFreeStackV1
{
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
	Node*		_topNode;
	LONGLONG	_size = 0;
};

template<typename T>
inline bool CLockFreeStackV1<T>::Push(const T& data)
{
	Node* newNode = _nodePool->Alloc();
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
inline bool CLockFreeStackV1<T>::Pop(T& data)
{
	for (;;)
	{
		Node* oldNode = _topNode;
		if (oldNode == nullptr) return false;

		Node* newTopNode = oldNode->_next;
		if (InterlockedCompareExchangePointer(reinterpret_cast<PVOID*>(&_topNode), newTopNode, oldNode) == oldNode)
		{
			data = oldNode->_data;
			delete(oldNode);
			return true;
		}
	}
}
