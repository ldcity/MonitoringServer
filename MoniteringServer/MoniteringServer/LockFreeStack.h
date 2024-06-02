#pragma once
#ifndef __LOCKFREESTACK__HEADER__
#define __LOCKFREESTACK__HEADER__

#include "LockFreeFreeList.h"

#define CRASH() do { int* ptr = nullptr; *ptr = 100;} while(0);

template <typename DATA>
class LockFreeStack
{

private:
	struct Node
	{
		Node() :data(NULL), next(nullptr)
		{
		}

		DATA data;
		Node* next;
	};

	alignas(64) Node* pTop;							// Stack Top
	alignas(64) __int64 _size;

	ObjectLockFreeStackFreeList<Node>* LockFreeStackPool;

public:
	LockFreeStack(int size = STACKMAX, int bPlacement = false) : _size(0), pTop(nullptr)
	{
		LockFreeStackPool = new ObjectLockFreeStackFreeList<Node>(size, bPlacement);
	}

	// unique pointer (64bit �𵨿��� ���� ������ �ʴ� ���� 17bit�� ����Ͽ� ������ ������ �� ����
	inline Node* GetUniqu64ePointer(Node* pointer, uint64_t iCount)
	{
		return (Node*)((uint64_t)pointer | (iCount << MAX_COUNT_VALUE));
	}

	// real pointer
	inline Node* Get64Pointer(Node* uniquePointer)
	{
		return (Node*)((uint64_t)uniquePointer & PTRMASK);
	}

	bool Push(DATA data)
	{
		Node* newRealNode = LockFreeStackPool->Alloc();
		if (newRealNode == nullptr)
			return false;

		newRealNode->data = data;

		Node* oldTop = nullptr;
		Node* realOldTop = nullptr;
		Node* newNode = nullptr;
		uint64_t cnt;

		while (true)
		{
			oldTop = pTop;

			cnt = ((uint64_t)oldTop >> MAX_COUNT_VALUE) + 1;

			realOldTop = Get64Pointer(oldTop);
			newRealNode->next = realOldTop;
			newNode = GetUniqu64ePointer(newRealNode, cnt);

			if ((PVOID)oldTop == InterlockedCompareExchangePointer((PVOID*)&pTop, newNode, oldTop))
			{
				break;
			}
		}

		InterlockedIncrement64(&_size);
		return true;
	}

	bool Pop(DATA* _value)
	{
		Node* oldTop = nullptr;
		Node* realOldTop = nullptr;

		Node* oldTopNext = nullptr;
		Node* realOldTopNext = nullptr;

		uint64_t cnt;

		while (true)
		{
			oldTop = pTop;
			realOldTop = Get64Pointer(oldTop);

			if (realOldTop == nullptr)
				return false;

			cnt = ((uint64_t)oldTop >> MAX_COUNT_VALUE) + 1;

			realOldTopNext = realOldTop->next;
			oldTopNext = GetUniqu64ePointer(realOldTopNext, cnt);

			if ((PVOID)oldTop == InterlockedCompareExchangePointer((PVOID*)&pTop, oldTopNext, oldTop))
			{
				*_value = realOldTop->data;
				LockFreeStackPool->Free(realOldTop);

				break;
			}
		}

		InterlockedDecrement64(&_size);

		return true;
	}

	__int64 GetSize() { return _size; }

	~LockFreeStack()
	{
		delete LockFreeStackPool;
	}
};

#endif // !1
