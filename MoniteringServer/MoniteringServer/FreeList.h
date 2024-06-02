#pragma once
/*---------------------------------------------------------------

	�޸� Ǯ Ŭ���� (������Ʈ Ǯ / ��������Ʈ)
	Ư�� ����Ÿ(����ü,Ŭ����,����)�� ������ �Ҵ� �� ��������.

	- ����.

	procademy::CMemoryPool<DATA> MemPool(300, FALSE);
	DATA *pData = MemPool.Alloc();

	pData ���

	MemPool.Free(pData);

----------------------------------------------------------------*/
#ifndef  __PROCADEMY_MEMORY_POOL__
#define  __PROCADEMY_MEMORY_POOL__

//#include <new>
//#include <list>
//#include <map>
//#include <unordered_map>
//#include <iostream>
//#include <cmath>

#define POOLLIMIT 1000											// ������Ʈ Ǯ���� ������ �� �ִ� �ִ� �� ����
#define CRASH() do{ int* ptr = nullptr; *ptr = 100;} while(0)	// nullptr �����ؼ� ������ crash �߻�

namespace ldcity
{
	// ��ü �Ҵ翡 ���Ǵ� ���
	template <class DATA>
	struct st_MEMORY_BLOCK_NODE
	{
		int size;
		int underflow;
		DATA data;
		int overflow;
		st_MEMORY_BLOCK_NODE<DATA>* next;
		unsigned int type;
	};

	template <class DATA>
	class ObjectFreeListBase
	{
	public:
		virtual ~ObjectFreeListBase() {}

		virtual DATA* Alloc() = 0;
		virtual bool Free(DATA* ptr) = 0;
	};

	template <class DATA>
	class ObjectFreeList : ObjectFreeListBase<DATA>
	{
	private:
		// �� ��� ���� - bool�� false = �Ҹ��� ȣ�� ����, true = �Ҹ��� ȣ����
		std::map<st_MEMORY_BLOCK_NODE<DATA>*, bool> memoryBlocks;
		bool m_bPlacementNew;
		unsigned int m_type;							// ������Ʈ Ǯ Ÿ��
		long m_iCapacity;								// ������Ʈ Ǯ ���� ��ü ����
		long m_iUseCount;								// ������� �� ����.
		long m_iAllocCnt;
		long m_iFreeCnt;
		CRITICAL_SECTION poolLock;
	public:
		// ���� ������� ��ȯ�� (�̻��) ������Ʈ ���� ����.(Top)
		st_MEMORY_BLOCK_NODE<DATA>* _pFreeBlockNode;

		//////////////////////////////////////////////////////////////////////////
		// ������, �ı���.
		//
		// Parameters:	(int) �ʱ� �� ����. - 0�̸� FreeList, ���� ������ �ʱ�� �޸� Ǯ, ���߿� ��������Ʈ
		//				(bool) Alloc �� ������ / Free �� �ı��� ȣ�� ����
		// Return:
		//////////////////////////////////////////////////////////////////////////
		ObjectFreeList(int iBlockNum = 0, bool bPlacementNew = false);

		//// MemoryFreeList���� ��ü�� �ƴ� ���� �޸� �Ҵ��� ���� ���Ǵ� ������
		//ObjectFreeList(int size);

		virtual	~ObjectFreeList();


		//////////////////////////////////////////////////////////////////////////
		// �޸� Ǯ Ÿ�� ����
		//
		// Parameters: ����.
		// Return: ����
		//////////////////////////////////////////////////////////////////////////
		void PoolType(void);


		//////////////////////////////////////////////////////////////////////////
		// �� �ϳ��� �Ҵ�޴´�.  
		//
		// Parameters: ����.
		// Return: (DATA *) ����Ÿ �� ������.
		//////////////////////////////////////////////////////////////////////////
		DATA* Alloc(void);

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
		int		GetCapacityCount(void) { return m_iCapacity; }

		//////////////////////////////////////////////////////////////////////////
		// ���� ������� �� ������ ��´�.
		//
		// Parameters: ����.
		// Return: (int) ������� �� ����.
		//////////////////////////////////////////////////////////////////////////
		long		GetUseCount(void) { return m_iUseCount; }

		long		GetAllocCount(void) { return m_iAllocCnt; }

		long		GetFreeCount(void) { return m_iFreeCnt; }



		void	SetCapacityCount(long value)
		{
			InterlockedExchange(&m_iCapacity, value);
		}

		void	SetUseCount(long value)
		{
			InterlockedExchange(&m_iUseCount, value);
		}

		void	SetAllocCount(long value)
		{
			InterlockedExchange(&m_iAllocCnt, value);
		}

		void	SetFreeCount(long value)
		{
			InterlockedExchange(&m_iFreeCnt, value);
		}
	};

	template <class DATA>
	ObjectFreeList<DATA>::ObjectFreeList(int iBlockNum, bool bPlacementNew)
		: _pFreeBlockNode(nullptr), m_bPlacementNew(bPlacementNew), m_iCapacity(iBlockNum), m_iUseCount(0)
	{
		InitializeCriticalSection(&poolLock);

		// Ǯ Ÿ�� ����
		PoolType();

		// ���� ��������Ʈ�� �����ڿ��� �ƹ��͵� �� ��
		if (m_iCapacity == 0) return;

		// �ʱ⿡ �޸� Ǯó�� ���� �޸� ����� �Ҵ� 
		char* memoryBlock = (char*)malloc(sizeof(st_MEMORY_BLOCK_NODE<DATA>));

		if (memoryBlock == nullptr) throw std::bad_alloc();

		// ������ġ(0xfdfdfdfd)�� �յڷ� �ְ� data�� ��ü�� �ƴ� �޸� ��� ������ä�� ��
		// -> placement new�� �� ������ �ƴ����� ���� �޸� ����� ����, ������ ȣ���Ͽ� ��ü�� ���� ������
		_pFreeBlockNode = (st_MEMORY_BLOCK_NODE<DATA>*)(memoryBlock);
		_pFreeBlockNode->underflow = 0xfdfdfdfd;
		_pFreeBlockNode->overflow = 0xfdfdfdfd;
		_pFreeBlockNode->type = m_type;
		_pFreeBlockNode->next = nullptr;

		// placement new �� false �� ��ü ����(ó���� ��� �����Ҷ��� ������ ȣ��) - �� ���Ŀ��� ����!
		if (!m_bPlacementNew)
			new(&_pFreeBlockNode->data)DATA;

		// ����Ʈ ������ ���� ��� ������ ��� ���� - false = �Ҹ��� ȣ�� ���� �ּҰ�
		memoryBlocks.insert({ _pFreeBlockNode, false });

		st_MEMORY_BLOCK_NODE<DATA>* temp = _pFreeBlockNode;

		for (int i = 0; i < m_iCapacity - 1; i++)
		{
			// �� ��� ����
			char* memoryBlock = (char*)malloc(sizeof(st_MEMORY_BLOCK_NODE<DATA>));
			if (memoryBlock == nullptr) throw std::bad_alloc();

			st_MEMORY_BLOCK_NODE<DATA>* newNode = (st_MEMORY_BLOCK_NODE<DATA>*)(memoryBlock);
			newNode->underflow = 0xfdfdfdfd;
			newNode->overflow = 0xfdfdfdfd;
			newNode->type = m_type;
			newNode->next = _pFreeBlockNode;
			_pFreeBlockNode = newNode;

			// placement new �� false �� ��ü ����(ó���� ��� �����Ҷ��� ������ ȣ��) - �� ���Ŀ��� ����!
			if (!m_bPlacementNew)
				new(&newNode->data)DATA;

			// ����Ʈ ������ ���� ��� ������ ��� ����
			memoryBlocks.insert({ newNode, false });
		}
	}

	template <class DATA>
	ObjectFreeList<DATA>::~ObjectFreeList()
	{
		for (auto iter = memoryBlocks.begin(); iter != memoryBlocks.end(); ++iter)
		{
			st_MEMORY_BLOCK_NODE<DATA>* node = (st_MEMORY_BLOCK_NODE<DATA>*)iter->first;

			// placement new �� false �� ��ü �Ҹ� (�޸� Ǯ �Ҹ�� ���������� �Ҹ��� ȣ��)
			// placement new �� true �� �Ҹ��� ȣ��� �͵� �ְ� ���� �͵� �����Ƿ�
			// map���� �Ҹ��� ȣ�� ���� flag�� �����ؼ� �����ϰų�
			// �迭�̳� ����Ʈ��� �Ҹ��ڸ� ȣ���ߴ��� �ƴ��� Ȯ���ϸ鼭 ȣ��
			if (!m_bPlacementNew)
			{
				node->data.~DATA();
			}
			else
			{
				// second ���� false : �Ҹ��� ȣ�� �ȵ��� ���
				if (!iter->second)
					node->data.~DATA();
			}

			free(node);
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// �޸� Ǯ Ÿ�� ����
	//
	// Parameters: ����.
	// Return: ����
	//////////////////////////////////////////////////////////////////////////
	template <class DATA>
	void ObjectFreeList<DATA>::PoolType()
	{
		// �޸� Ǯ �ּҰ��� type���� ����
		m_type = (unsigned int)this;
	}

	//////////////////////////////////////////////////////////////////////////
	// �� �ϳ��� �Ҵ�޴´�.  
	//
	// Parameters: ����.
	// Return: (DATA *) ����Ÿ �� ������.
	//////////////////////////////////////////////////////////////////////////
	template <class DATA>
	DATA* ObjectFreeList<DATA>::Alloc()
	{
		EnterCriticalSection(&poolLock);

		m_iAllocCnt++;

		// �̻�� ��(free block)�� ���� ��
		if (_pFreeBlockNode != nullptr)
		{
			// �޸� ���� ������ ���(or ��ü) ������ �� _pFreeBlockNode ��ġ �ٲ�
			DATA* object = &_pFreeBlockNode->data;

			_pFreeBlockNode->size = sizeof(DATA);
			_pFreeBlockNode = _pFreeBlockNode->next;

			// ������ ȣ��
			if (m_bPlacementNew) new(object) DATA;

			// ������� �� ���� ����
			m_iUseCount++;

			LeaveCriticalSection(&poolLock);
			return object;
		}
		// �̻�� ��(free block)�� ���� ��
		// ���ο� �� ����
		char* memoryBlock = (char*)malloc(sizeof(st_MEMORY_BLOCK_NODE<DATA>));
		if (memoryBlock == nullptr) throw std::bad_alloc();

		st_MEMORY_BLOCK_NODE<DATA>* newNode = (st_MEMORY_BLOCK_NODE<DATA>*)(memoryBlock);
		newNode->size = sizeof(DATA);
		newNode->underflow = 0xfdfdfdfd;
		newNode->overflow = 0xfdfdfdfd;
		newNode->type = m_type;
		newNode->next = nullptr;

		DATA* object = &newNode->data;

		// �� �� �����̹Ƿ� ������ ȣ��
		new(object)DATA;

		// ��ü �� ���� ����
		m_iCapacity++;

		memoryBlocks.insert({ newNode, false });

		// ������� �� ���� ����
		m_iUseCount++;

		LeaveCriticalSection(&poolLock);

		return object;
	}

	//////////////////////////////////////////////////////////////////////////
	// ������̴� ���� �����Ѵ�.
	//
	// Parameters: (DATA *) �� ������.
	// Return: (BOOL) TRUE, FALSE.
	//////////////////////////////////////////////////////////////////////////
	template <class DATA>
	bool ObjectFreeList<DATA>::Free(DATA* pData)
	{
		EnterCriticalSection(&poolLock);

		// ������ġ(0xfdfdfdfd) �κк��� ���� (pData�� 4byte ��)
		st_MEMORY_BLOCK_NODE<DATA>* temp = (st_MEMORY_BLOCK_NODE<DATA>*)((char*)pData - sizeof(int) * 2);

		// �޸� Ǯ�� type�� �ٸ� ���
		if (temp->type != m_type)
		{
			wprintf(L"Pool Type Failed\n");
			LeaveCriticalSection(&poolLock);
			return false;
		}

		// ��� ���� ���� ��� ���̻� ������ ���� ���� ���
		if (m_iUseCount == 0)
		{
			wprintf(L"Free Failed.\n");
			LeaveCriticalSection(&poolLock);
			return false;
		}

		// �޸� ���� ����
		if (temp->underflow != 0xfdfdfdfd || temp->overflow != 0xfdfdfdfd)
			CRASH();

		// �Ҹ��� ȣ�� - placement new�� true�̸� ��ü ��ȯ�� �� �Ҹ��� ȣ����Ѿ� ��
		if (m_bPlacementNew)
		{
			auto it = memoryBlocks.find(temp);
			if (it != memoryBlocks.end())
			{
				it->second = true;
				temp->data.~DATA();
			}
			else CRASH();
		}

		m_iFreeCnt++;

		if (_pFreeBlockNode == nullptr)
		{
			// �ش� ��ü�� _pFreeBlockNode(top)���� ����
			_pFreeBlockNode = temp;
			_pFreeBlockNode->next = nullptr;

			//// buffer clear
			//memset(&_pFreeBlockNode->data, 0, sizeof(_pFreeBlockNode->data));

			// ������� �� ���� ����
			m_iUseCount--;

			LeaveCriticalSection(&poolLock);

			return true;
		}

		//// buffer clear
		//memset(&temp->data, 0, sizeof(temp->data));

		// �ش� ��ü�� _pFreeBlockNode(top)���� ����
		temp->next = _pFreeBlockNode;
		_pFreeBlockNode = temp;

		// ������� �� ���� ����
		m_iUseCount--;

		LeaveCriticalSection(&poolLock);

		return true;
	}
}




#endif

