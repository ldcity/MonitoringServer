/*---------------------------------------------------------------

procademy MemoryPool.

�޸� Ǯ Ŭ���� (������Ʈ Ǯ / ��������Ʈ)
Ư�� ����Ÿ(����ü,Ŭ����,����)�� ������ �Ҵ� �� ��������.

- ����.

procademy::CMemoryPool<DATA> MemPool(300, FALSE);
DATA *pData = MemPool.Alloc();

pData ���

MemPool.Free(pData);


----------------------------------------------------------------*/
#ifndef  __LOCKFREESTACK_MEMORY_POOL__
#define  __LOCKFREESTACK_MEMORY_POOL__

#include <Windows.h>
#include <vector>
#include <iostream>

#define CRASH() do{ int* ptr = nullptr; *ptr = 100;} while(0)		// nullptr �����ؼ� ������ crash �߻�


#define MEMORYPOOLCNT 10000											// �޸� Ǯ ��� ����
#define QUEUEMAX 100
#define STACKMAX 200

#define MAXBITS 17
#define MAX_COUNT_VALUE 47
#define PTRMASK 0x00007FFFFFFFFFFF
#define MAXIMUMADDR 0x00007FFFFFFEFFFF

template <class DATA>
class ObjectFreeListBase
{
public:
	virtual ~ObjectFreeListBase() {}
	virtual DATA* Alloc() = 0;
	virtual bool Free(DATA* ptr) = 0;
};

template <class DATA>
class ObjectLockFreeStackFreeList : ObjectFreeListBase<DATA>
{
private:
	alignas(64) uint64_t m_iUseCount;								// ������� �� ����.
	alignas(64) uint64_t m_iAllocCnt;
	alignas(64) uint64_t m_iFreeCnt;
	alignas(64) uint64_t m_iCapacity;								// ������Ʈ Ǯ ���� ��ü ����

	// ��ü �Ҵ翡 ���Ǵ� ���
	template <class DATA>
	struct st_MEMORY_BLOCK_NODE
	{
		DATA data;
		st_MEMORY_BLOCK_NODE<DATA>* next;
	};

	// ���� ������� ��ȯ�� (�̻��) ������Ʈ ���� ����.(Top)
	st_MEMORY_BLOCK_NODE<DATA>* _pFreeBlockNode;

	bool m_bPlacementNew;

public:
	//////////////////////////////////////////////////////////////////////////
	// ������, �ı���.
	//
	// Parameters:	(int) �ʱ� �� ����. - 0�̸� FreeList, ���� ������ �ʱ�� �޸� Ǯ, ���߿� ��������Ʈ
	//				(bool) Alloc �� ������ / Free �� �ı��� ȣ�� ����
	// Return:
	//////////////////////////////////////////////////////////////////////////
	ObjectLockFreeStackFreeList(int iBlockNum = 0, bool bPlacementNew = false);

	virtual	~ObjectLockFreeStackFreeList();

	//////////////////////////////////////////////////////////////////////////
	// �� �ϳ��� �Ҵ�޴´�.  
	//
	// Parameters: ����.
	// Return: (DATA *) ����Ÿ �� ������.
	//////////////////////////////////////////////////////////////////////////
	DATA* Alloc();

	//////////////////////////////////////////////////////////////////////////
	// ������̴� ���� �����Ѵ�.
	//
	// Parameters: (DATA *) �� ������.
	// Return: (BOOL) TRUE, FALSE.
	//////////////////////////////////////////////////////////////////////////
	bool	Free(DATA* pData);

	//////////////////////////////////////////////////////////////////////////
	// ���� Ȯ�� �� �� ������ ��´�. (�޸�Ǯ ������ ��ü ����)
	//
	// Parameters: ����.
	// Return: (int) �޸� Ǯ ���� ��ü ����
	//////////////////////////////////////////////////////////////////////////
	uint64_t		GetCapacityCount(void) { return m_iCapacity; }

	//////////////////////////////////////////////////////////////////////////
	// ���� ������� �� ������ ��´�.
	//
	// Parameters: ����.
	// Return: (int) ������� �� ����.
	//////////////////////////////////////////////////////////////////////////
	uint64_t		GetUseCount(void) { return m_iUseCount; }

	uint64_t		GetAllocCount(void) { return m_iAllocCnt; }

	uint64_t		GetFreeCount(void) { return m_iFreeCnt; }

	// unique pointer
	inline st_MEMORY_BLOCK_NODE<DATA>* GetUniqu64ePointer(st_MEMORY_BLOCK_NODE<DATA>* pointer, uint64_t iCount)
	{
		return (st_MEMORY_BLOCK_NODE<DATA>*)((uint64_t)pointer | ((uint64_t)iCount << MAX_COUNT_VALUE));
	}

	// real pointer
	inline st_MEMORY_BLOCK_NODE<DATA>* Get64Pointer(st_MEMORY_BLOCK_NODE<DATA>* uniquePointer)
	{
		return (st_MEMORY_BLOCK_NODE<DATA>*)((uint64_t)uniquePointer & PTRMASK);
	}
};

template <class DATA>
ObjectLockFreeStackFreeList<DATA>::ObjectLockFreeStackFreeList(int iBlockNum, bool bPlacementNew)
	: _pFreeBlockNode(nullptr), m_bPlacementNew(bPlacementNew), m_iCapacity(iBlockNum), m_iUseCount(0), m_iAllocCnt(0), m_iFreeCnt(0)
{
	// x64 �𵨿��� ����� �� �ִ� �޸� �뿪�� ����ƴ��� Ȯ��
	SYSTEM_INFO si;
	GetSystemInfo(&si);

	if ((__int64)si.lpMaximumApplicationAddress != MAXIMUMADDR)
	{
		wprintf(L"X64 Maximum Address Used...\n");
		CRASH();
	}

	// ���� ��������Ʈ�� �����ڿ��� �ƹ��͵� �� ��
	if (m_iCapacity == 0) return;

	st_MEMORY_BLOCK_NODE<DATA>* pFreeBlockNode = nullptr;

	for (int i = 0; i < m_iCapacity; i++)
	{
		// �� ��� ����
		st_MEMORY_BLOCK_NODE<DATA>* memoryBlock = (st_MEMORY_BLOCK_NODE<DATA>*)_aligned_malloc(sizeof(st_MEMORY_BLOCK_NODE<DATA>), alignof(st_MEMORY_BLOCK_NODE<DATA>));
		if (memoryBlock == nullptr)
			CRASH();
		//memoryBlock->next = _pFreeBlockNode;
		//_pFreeBlockNode = newNode;
		memoryBlock->next = pFreeBlockNode;
		pFreeBlockNode = memoryBlock;

		// placement new �� false �� ��ü ����(ó���� ��� �����Ҷ��� ������ ȣ��) - �� ���Ŀ��� ����!
		if (!m_bPlacementNew)
			new(&memoryBlock->data)DATA;
	}

	_pFreeBlockNode = pFreeBlockNode;
}

template <class DATA>
ObjectLockFreeStackFreeList<DATA>::~ObjectLockFreeStackFreeList()
{
	while (_pFreeBlockNode != nullptr)
	{
		st_MEMORY_BLOCK_NODE<DATA>* curNode = Get64Pointer(_pFreeBlockNode);

		if (!m_bPlacementNew)
		{
			curNode->data.~DATA();
		}

		_pFreeBlockNode = curNode->next;

		_aligned_free(curNode);

	}

}


//////////////////////////////////////////////////////////////////////////
// �� �ϳ��� �Ҵ�޴´�.  - ������带 pop
//
// Parameters: ����.
// Return: (DATA *) ����Ÿ �� ������.
//////////////////////////////////////////////////////////////////////////
template <class DATA>
DATA* ObjectLockFreeStackFreeList<DATA>::Alloc()
{
	st_MEMORY_BLOCK_NODE<DATA>* pOldTopNode = nullptr;
	st_MEMORY_BLOCK_NODE<DATA>* pOldRealNode = nullptr;
	st_MEMORY_BLOCK_NODE<DATA>* pNewTopNode = nullptr;

	uint64_t cnt;

	while (true)
	{
		pOldTopNode = _pFreeBlockNode;

		pOldRealNode = Get64Pointer(pOldTopNode);

		// �̻�� ��(free block)�� ���� �� 
		if (pOldRealNode == nullptr)
		{
			pOldRealNode = (st_MEMORY_BLOCK_NODE<DATA>*)_aligned_malloc(sizeof(st_MEMORY_BLOCK_NODE<DATA>), alignof(st_MEMORY_BLOCK_NODE<DATA>));
			if (pOldRealNode == nullptr)
				CRASH();

			pOldRealNode->next = nullptr;

			// ��ü �� ���� ����
			InterlockedIncrement64((LONG64*)&m_iCapacity);

			break;
		}

		cnt = ((uint64_t)pOldTopNode >> MAX_COUNT_VALUE) + 1;

		//pNewTopNode = pOldRealNode->next;
		pNewTopNode = GetUniqu64ePointer(pOldRealNode->next, cnt);

		if (pOldTopNode == InterlockedCompareExchangePointer((PVOID*)&_pFreeBlockNode, pNewTopNode, pOldTopNode))
		{
			break;
		}
	}

	DATA* object = &pOldRealNode->data;

	// �� �� �����̹Ƿ� ������ ȣ��
	if (m_bPlacementNew)
	{
		new(object) DATA;
	}

	InterlockedIncrement64((LONG64*)&m_iAllocCnt);

	// ������� �� ���� ����
	InterlockedIncrement64((LONG64*)&m_iUseCount);

	return object;
}

//////////////////////////////////////////////////////////////////////////
// ������̴� ���� �����Ѵ�. - ������带 push
//
// Parameters: (DATA *) �� ������.
// Return: (BOOL) TRUE, FALSE.
//////////////////////////////////////////////////////////////////////////
template <class DATA>
bool ObjectLockFreeStackFreeList<DATA>::Free(DATA* pData)
{
	st_MEMORY_BLOCK_NODE<DATA>* realTemp = (st_MEMORY_BLOCK_NODE<DATA>*)((char*)pData);

	// ��� ���� ���� ��� ���̻� ������ ���� ���� ���
	if (m_iUseCount == 0)
	{
		wprintf(L"Free Failed.\n");
		CRASH();
		return false;
	}


	// backup�� ���� free top ���
	st_MEMORY_BLOCK_NODE<DATA>* oldTopNode = nullptr;
	st_MEMORY_BLOCK_NODE<DATA>* temp = nullptr;
	uint64_t cnt;

	while (true)
	{
		// free node top�� backup
		oldTopNode = _pFreeBlockNode;

		cnt = ((uint64_t)oldTopNode >> MAX_COUNT_VALUE) + 1;

		// �����Ϸ��� node�� ��������Ʈ�� top���� �ӽ� ����
		//realTemp->next = oldTopNode;
		//temp = GetUniqu64ePointer(realTemp, cnt);
		realTemp->next = Get64Pointer(oldTopNode);

		temp = GetUniqu64ePointer(realTemp, cnt);

		// CAS
		if (oldTopNode == InterlockedCompareExchangePointer((PVOID*)&_pFreeBlockNode, temp, oldTopNode))
		{
			break;
		}
	}

	// �Ҹ��� ȣ�� - placement new�� true�̸� ��ü ��ȯ�� �� �Ҹ��� ȣ����Ѿ� ��
	// CAS �۾��� �����Ͽ� free node�� push �Ϸ��ϰ��� �Ҹ��� ȣ���ؾ��� (oldNode�� �Ҹ��� ȣ��)
	if (m_bPlacementNew)
	{
		realTemp->data.~DATA();
	}

	InterlockedIncrement64((LONG64*)&m_iFreeCnt);
	InterlockedDecrement64((LONG64*)&m_iUseCount);

	return true;
}


#endif

