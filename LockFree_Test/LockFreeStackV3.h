#pragma once

#include <Windows.h>
#include <vector>
#include <list>
#include <string>
#include "ObjectPool.h"

class LogInfo
{
public:
	enum JOB
	{
		PUSH = 0,
		POP = 1
	};

	bool			_job;
	int				_ID;
	void* _unp;
	void* _tnp;

	LogInfo(){}
	LogInfo(int id, void* use, void* top, bool job)
		: _ID(id), _unp(use), _tnp(top), _job(job){}
};

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
	bool Push(const T& data, LogInfo& info);
	bool Pop(out T& data, LogInfo& info);

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
inline bool CLockFreeStackV3<T>::Push(const T& data, LogInfo& info)
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
inline bool CLockFreeStackV3<T>::Pop(T& data, LogInfo& info)
{
	for (;;)
	{
		Node* oldNode = _topNode;
		if (oldNode == nullptr) return false;

		Node* newTopNode = oldNode->_next;
		if (InterlockedCompareExchangePointer(reinterpret_cast<PVOID*>(&_topNode), newTopNode, oldNode) == oldNode)
		{
			data = oldNode->_data;
			_nodePool->Free(oldNode);
			return true;
		}
	}
}
