#pragma once
#ifndef __LOCKFREEQUEUE__HEADER__
#define __LOCKFREEQUEUE__HEADER__

#include "LockFreeFreeList.h"

#define CRASH() do { int* ptr = nullptr; *ptr = 100;} while(0);

template <typename DATA>
class LockFreeQueue
{
private:
	struct Node
	{
		DATA data;
		Node* next;
	};

	alignas(64)Node* _head;
	alignas(64)	Node* _tail;
	
	alignas(64) __int64 _size;

	ObjectLockFreeStackFreeList<Node>* LockFreeQueuePool;

public:
	LockFreeQueue(int size = QUEUEMAX, int bPlacement = false) : _size(0)
	{
		LockFreeQueuePool = new ObjectLockFreeStackFreeList<Node>(size);

		// dummy node
		Node* node = LockFreeQueuePool->Alloc();
		node->next = nullptr;

		_head = node;
		_tail = _head;
	}

	~LockFreeQueue()
	{
		Node* node = Get64Pointer(_head);

		while (node != nullptr)
		{
			Node* temp = node;
			node = node->next;
			LockFreeQueuePool->Free(temp);
		}

		delete LockFreeQueuePool;
	}


	inline __int64 GetSize() { return _size; }

	// unique pointer
	inline Node* GetUniqu64ePointer(Node* pointer, uint64_t iCount)
	{
		return (Node*)((uint64_t)pointer | (iCount << MAX_COUNT_VALUE));
	}

	// real pointer
	inline Node* Get64Pointer(Node* uniquePointer)
	{
		return (Node*)((uint64_t)uniquePointer & PTRMASK);
	}

	void Enqueue(DATA _data)
	{

		uint64_t cnt;

		Node* realNewNode = LockFreeQueuePool->Alloc();
		if (realNewNode == nullptr)
			return;

		realNewNode->data = _data;
		realNewNode->next = nullptr;

		Node* oldTail = nullptr;
		Node* realOldTail = nullptr;
		Node* oldTailNext = nullptr;
		Node* newNode = nullptr;

		Node* oldHead = nullptr;
		Node* realOldHead = nullptr;
		Node* oldHeadNext = nullptr;

		while (true)
		{
			oldTail = _tail;

			realOldTail = Get64Pointer(oldTail);

			// unique value
			cnt = ((uint64_t)oldTail >> MAX_COUNT_VALUE) + 1;

			// ���� tail�� next ���
			oldTailNext = realOldTail->next;

			oldHead = _head;
			realOldHead = Get64Pointer(oldHead);
			oldHeadNext = realOldHead->next;

			if (oldTailNext == nullptr)
			{

				// CAS 1 ����
				if (oldTailNext == InterlockedCompareExchangePointer((PVOID*)&realOldTail->next, realNewNode, oldTailNext))
				{
					newNode = GetUniqu64ePointer(realNewNode, cnt);
					InterlockedCompareExchangePointer((PVOID*)&_tail, newNode, oldTail);

					break;
				}
				else
				{
					oldTailNext = realOldTail->next;
					if (oldTailNext != nullptr)
					{
						newNode = GetUniqu64ePointer(oldTailNext, cnt);
						InterlockedCompareExchangePointer((PVOID*)&_tail, newNode, oldTail);
					}
				}

			}
			else
			{
				// oldTail�� next�� null�� �ƴ�
				// �� �κ� �ذ� �ȵǸ� ���ѷ���
				// CAS 2 ���� ���� _tail�� ������ ���� ������ �ȵǾ� �߻��ϴ� ����
				// -> _tail�� ������ ���� ����
				// -> ù��° CAS�� �����ϸ� enqueue�� ������ ��

				if (_tail == oldTail)
				{
					newNode = GetUniqu64ePointer(oldTailNext, cnt);
					InterlockedCompareExchangePointer((PVOID*)&_tail, newNode, oldTail);
				}
			}
		}

		InterlockedIncrement64(&_size);
	}

	bool Dequeue(DATA& _data)
	{
		if (_size <= 0)
		{
			return false;
		}

		InterlockedDecrement64(&_size);

		uint64_t cnt;

		Node* oldHead = nullptr;
		Node* raalOldHead = nullptr;
		Node* oldHeadNext = nullptr;

		Node* oldTail = nullptr;
		Node* realOldTail = nullptr;
		Node* newTail = nullptr;

		while (true)
		{
			// head, next ���
			oldHead = _head;
			raalOldHead = Get64Pointer(oldHead);

			cnt = ((uint64_t)oldHead >> MAX_COUNT_VALUE) + 1;

			oldHeadNext = raalOldHead->next;

			// ��� head�� next�� �� ���� ���� -> �׽�Ʈ �ڵ忡�� ������ ���� ��ť, ��ť�� �ϱ� ����
			// �׷����� �߻��� �� �ִ� ����?
			// head�� ����� ��, ���ؽ�Ʈ ����Ī�� �Ͼ�� �ٸ� �����尡 �ش� ��带 ��ť, ��ť�Ͽ�
			// ���� �ּ��� ���� ���Ҵ�� ��, head�� next�� nullptr�� ������
			// �ٽ� ���ؽ�Ʈ ����Ī�Ǿ� �Ʊ��� ������� ���ƿ��� ��� head�� next�� nullptr�� ����Ǿ� �ִ� ����
			// -> ������ ť�� ����־�� ����
			//if (oldHeadNext == nullptr && oldSize <= 0)
			if (oldHeadNext == nullptr)
			{
				if (_size >= 0)
					continue;

				InterlockedIncrement64(&_size);

				return false;
			}
			else
			{
				_data = oldHeadNext->data;
				Node* newHead = GetUniqu64ePointer(oldHeadNext, cnt);

				if (oldHead == InterlockedCompareExchangePointer((PVOID*)&_head, newHead, oldHead))
				{
					oldTail = _tail;
					realOldTail = Get64Pointer(oldTail);

					if (realOldTail->next == oldHeadNext)
					{
						uint64_t rearCnt = ((uint64_t)oldTail >> MAX_COUNT_VALUE) + 1;
						newTail = GetUniqu64ePointer(oldHeadNext, rearCnt);
						InterlockedCompareExchangePointer((PVOID*)&_tail, newTail, oldTail);
					}

					LockFreeQueuePool->Free(raalOldHead);
					break;
				}
			}
		}

		return true;
	}
};

#endif