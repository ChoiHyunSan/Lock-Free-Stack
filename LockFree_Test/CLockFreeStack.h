#pragma once
#include <Windows.h>
#include <vector>
#include <list>
#include <string>
#include "ObjectPool.h"
#include "LockFreeStackTest.h"

/************************

	 LockFree Stack

************************/

#define TEST_LOG
#define out

using namespace std;

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
	void*			_unp;
	void*			_tnp;

	LogInfo()
	{}

	LogInfo(int id, void* use, void* top, bool job)
		: _ID(id), _unp(use), _tnp(top), _job(job)
	{

	}
};

extern LogInfo* infoArray;
extern LONGLONG infoIndex;

#define KEY_BIT 47

template<typename T>
class CLockFreeStack
{
public:
	CLockFreeStack();
	~CLockFreeStack();
	
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
	Node*				_topNode;
	LONGLONG			_size;

	MemoryPool<Node>*	_nodePool;

private:
	LONGLONG			_key = 0;
	LONGLONG			_addressMask = 0x7FFFFFFFFFFF;
};

template<typename T>
inline CLockFreeStack<T>::CLockFreeStack()
	: _topNode(nullptr), _size(0)
{
	_nodePool = new MemoryPool<Node>(10000, true);
}

template<typename T>
inline CLockFreeStack<T>::~CLockFreeStack()
{
	delete _nodePool;
}

template<typename T>
inline bool CLockFreeStack<T>::Push(const T& data, LogInfo& info)
{
	// Node* newNode = new Node;
	Node* newNode = _nodePool->Alloc();
	newNode->_data = data;
	// info._unp = newNode;

	LONGLONG uniqueBit = (InterlockedIncrement64(&_key) << KEY_BIT);
	Node* newTopNode = reinterpret_cast<Node*>((LONGLONG)newNode | uniqueBit);

	for (;;)
	{
		Node* topNode = _topNode;
		newNode->_next = reinterpret_cast<Node*>((LONGLONG)topNode & _addressMask);

		//info._tnp = topNode;
		if (InterlockedCompareExchangePointer(reinterpret_cast<PVOID*>(&_topNode), newTopNode, topNode) == topNode)
		{
			// InterlockedIncrement64(&_size);
			// LONGLONG index = InterlockedIncrement64(&infoIndex);
			// infoArray[index] = info;

			return true;
		}
	}
}

template<typename T>
inline bool CLockFreeStack<T>::Pop(out T& data, LogInfo& info)
{
	for (;;)
	{
		Node* oldTopNode = _topNode;
		Node* oldNode = reinterpret_cast<Node*>((LONGLONG)_topNode & _addressMask);
		if (oldNode == nullptr) return false;

		LONGLONG uniqueBit = (InterlockedIncrement64(&_key) << KEY_BIT);
		Node* newTopNode = reinterpret_cast<Node*>((LONGLONG)(oldNode->_next) | uniqueBit);

		//info._unp = oldNode;
		//info._tnp = newTopNode;

		if (InterlockedCompareExchangePointer(reinterpret_cast<PVOID*>(&_topNode), newTopNode, oldTopNode) == oldTopNode)
		{
			// InterlockedDecrement64(&_size);

			// LONGLONG index = InterlockedIncrement64(&infoIndex);
			// infoArray[index] = info;

			data = oldNode->_data;
			_nodePool->Free(oldNode);
			// delete oldNode;
			return true;
		}
	}
}
