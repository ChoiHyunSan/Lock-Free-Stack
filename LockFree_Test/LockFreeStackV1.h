#pragma once

#include <Windows.h>
#include <vector>
#include <list>
#include <string>
#include "CLockFreeStack.h"

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
	bool Push(const T& data, LogInfo info);
	bool Pop(T& data, LogInfo info);

private:
	Node*		_topNode;
	LONGLONG	_size = 0;
};

template<typename T>
inline bool CLockFreeStackV1<T>::Push(const T& data, LogInfo info)
{
	Node* newNode = new Node();
	newNode->_data = data;
	info._unp = newNode;

	for (;;)
	{
		Node* topNode = _topNode;
		newNode->_next = topNode;

		info._tnp = topNode;
		if (InterlockedCompareExchangePointer(reinterpret_cast<PVOID*>(&_topNode), newNode, topNode) == topNode)
		{
			InterlockedIncrement64(&_size);
			LONGLONG index = InterlockedIncrement64(&infoIndex);
			infoArray[index % 10000] = info;
			
			return true;
		}
	}
}

template<typename T>
inline bool CLockFreeStackV1<T>::Pop(T& data, LogInfo info)
{
	for (;;)
	{
		Node* oldNode = _topNode;
		if (oldNode == nullptr) return false;

		Node* newTopNode = oldNode->_next;
		
		info._unp = oldNode;
		info._tnp = newTopNode;
		
		if (InterlockedCompareExchangePointer(reinterpret_cast<PVOID*>(&_topNode), newTopNode, oldNode) == oldNode)
		{
			InterlockedDecrement64(&_size);
			LONGLONG index = InterlockedIncrement64(&infoIndex);
			infoArray[index % 10000] = info;

			data = oldNode->_data;
			delete(oldNode);
			return true;
		}
	}
}
