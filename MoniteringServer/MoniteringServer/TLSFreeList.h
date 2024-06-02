#ifndef __TLS_FREE_LIST_VER2__
#define __TLS_FREE_LIST_VER2__

/*
* TLSFreeList Class
- ������
rootPool, bucketPool(�� ��Ŷ) �����Ҵ�
TLSAlloc() -> TLS_OUT_OF_INDEXES ���� Ȯ��

- �Ҹ���
TLSIndex�� ��Ŷ ������ Ȯ��
��Ŷ ������ ������ ���, �ش� ��Ŷ ���� �����ִ� ��� Ȯ�� (remain)
�� ��Ŷ�� ���, bucketPool�� ��ȯ


- Alloc


*/

#include "LockFreeStack.h"

#define BUCKETMAX				100			// ��Ŷ ���� (pTop)
#define NODEMAX					10000		// ��Ŷ �� ����ִ� ��� ����

struct Node;
class Bucket;

template <typename DATA>
class TLSObjectPool
{
	struct Node
	{
		DATA object;
		Node* next = nullptr;
	};

	class Bucket
	{
	public:
		Node* pTop;								// ��Ŷ �� top Node
		long size;								// ��Ŷ �� �����ϴ� ��� ����

	private:
		long maxCount;							// ��Ŷ �� ������ �� �ִ� �ִ� ��� ����
		long allocCount;						// ��Ŷ �� ��� �Ҵ� ����
		long freeCount;							// ��Ŷ �� ��� ��ȯ ����
		bool m_bPlacementNew;
		LockFreeStack<Node*>* _rootPool;

	public:

		// �ش� ������ŭ�� ��Ŷ �� ��带 Ǯ���� �����
		Bucket(int iNodeNum, bool _placementNew, LockFreeStack<Node*>* rootPool) :
			maxCount(iNodeNum), size(iNodeNum), allocCount(0), freeCount(0), pTop(nullptr),
			m_bPlacementNew(_placementNew), _rootPool(rootPool)
		{
			if (size != 0)
			{
				_rootPool->Pop(&pTop);
			}
		}

		inline DATA* Alloc()
		{
			// top ��带 ����
			Node* node = pTop;
			pTop = pTop->next;

			//allocCount++;
			size--;

			return &node->object;
		}

		inline bool Free(DATA* pData)
		{
			size++;
			//freeCount++;

			Node* node = (Node*)pData;

			// ��Ŷ�� ��ȯ�� ��� �������
			node->next = pTop;
			pTop = node;

			return true;
		}

		// �� ��Ŷ�� �ƴµ� rootPool�� ��尡 ���� ���� �Ҵ��� �Ϸ� �� ���
		inline void newAlloc()
		{
			size = maxCount;
			//allocCount = 0;
			//freeCount = 0;

			// true�� ������ ����ڰ� Alloc�Լ� ȣ���� ������ ������ ȣ������� �ϹǷ� ������ X
		/*	if (m_bPlacementNew)
			{*/
				for (int i = 0; i < maxCount; i++)
				{
					Node* node = (Node*)_aligned_malloc(sizeof(Node), alignof(Node));

					node->next = pTop;
					pTop = node;
				}
			/*}
			else
			{
				for (int i = 0; i < maxCount; i++)
				{
					Node* node = new Node;

					node->next = pTop;
					pTop = node;
				}
			}*/
		}

		//inline void Setting(Node* node, long iNodeCount)
		//{
		//	pTop = node;
		//	size = iNodeCount;
		//}

		//inline long GetSize() { return size; }

		inline void Clear()
		{
			pTop = nullptr;
			size = 0;
			//allocCount = 0;
			//freeCount = 0;
		}

		~Bucket()
		{
			if (pTop != nullptr)
			{
				Node* node;
				while (pTop)
				{
					node = pTop;
					pTop = pTop->next;

					// �Ҹ��� ȣ���ؾ��ϳ� Ȯ���ؾ���
					if (m_bPlacementNew)
						//free(node);
						_aligned_free(node);
					else
						delete node;
				}
			}
		}
		inline Node* GetTop() { return pTop; }
	};

	// TLS pointer (�Ҵ� & ������ ����ϰ� �ϸ鼭 �߻��ϴ� ������� ����)
	struct TLS_Bucket
	{
		Bucket* bucket1;			// ù��° ��Ŷ
		Bucket* bucket2;			// �ι�° ��Ŷ
	};

private:
	alignas(64) __int64 objectAllocCount;				// ����ڿ��� �Ҵ����� ��� ����
	alignas(64) __int64 objectFreeCount;				// ����ڰ� ��ȯ�� ��� ����
	alignas(64) __int64 objectUseCount;					// ���� ����ϰ� �ִ� ��� ����
	alignas(64) __int64 m_iCapacity;					// TLS Ǯ ���� ��ü �뷮
	
	DWORD tlsIndex;										// tls Index

	int defaultNodeCount;								// ó�� ������ ��� ����
	int defaultBucketCount;								// ó�� ������ ��Ŷ ����
	bool m_bPlacementNew;								// placement new �۵� ����

	LockFreeStack<Node*>* rootPool;						// root Pool
public:
	TLSObjectPool(int iNodeNum = NODEMAX, bool bPlacementNew = false, int iBucketNum = BUCKETMAX)
		: m_bPlacementNew(bPlacementNew), m_iCapacity(0),
		objectAllocCount(0), objectFreeCount(0), defaultNodeCount(iNodeNum), defaultBucketCount(iBucketNum)
	{
		// ��Ŷ ������ŭ ������������Ʈ ��� ����
		rootPool = new LockFreeStack<Node*>(defaultBucketCount, bPlacementNew);
		Init();

		tlsIndex = TlsAlloc();

		// Exception
		if (tlsIndex == TLS_OUT_OF_INDEXES)
			CRASH();
	}

	~TLSObjectPool()
	{
		// �ش� �����常 ����Ǿ� �ش� TLS ��Ŷ�� �����ִ� ���ҽ� ����
		// placement new flag�� ���� �Ҹ��� ȣ��
		// tls index ����
		// ������ �޸� Ǯ ����

		TLS_Bucket* buckets = (TLS_Bucket*)TlsGetValue(tlsIndex);

		if (buckets != nullptr)
		{
			delete buckets->bucket1;
			delete buckets->bucket2;
		}

		Node* top;
		while (rootPool->Pop(&top))
		{
			Node* cur;
			while (top)
			{
				cur = top;
				top = cur->next;
				if (!m_bPlacementNew)
					cur->object.~DATA();

				free(cur);
			}

			/*if (m_bPlacementNew)
			{

			}
			else
			{
				while (top)
				{
					cur = top;
					top = cur->next;
					delete cur;
				}
			}*/
		}

		TlsFree(tlsIndex);

		delete rootPool;
	}


	inline void Init()
	{
		if (m_bPlacementNew)
		{
			for (int i = 0; i < defaultBucketCount; i++)
			{
				Node* pTop = nullptr;
				// rootPool�� pTop ������ stack���� ���� (�� pTop �����ʹ� ��Ŷ ���� stack)
				for (int j = 0; j < defaultNodeCount; j++)
				{
					Node* node = (Node*)_aligned_malloc(sizeof(Node), alignof(Node));

					node->next = pTop;
					pTop = node;
				}

				rootPool->Push(pTop);
			}
		}
		else
		{
			for (int i = 0; i < defaultBucketCount; i++)
			{
				Node* pTop = nullptr;

				// rootPool�� pTop ������ stack���� ���� (�� pTop �����ʹ� ��Ŷ ���� stack)
				for (int j = 0; j < defaultNodeCount; j++)
				{
					Node* node = new Node;

					node->next = pTop;
					pTop = node;
				}

				rootPool->Push(pTop);
			}
		}

		m_iCapacity = defaultBucketCount * defaultNodeCount;
	}

	__int64 GetCapacity() { return m_iCapacity; }
	__int64 GetObjectAllocCount() { return objectAllocCount; }
	__int64 GetObjectFreeCount() { return objectFreeCount; }
	__int64 GetObjectUseCount() { return objectUseCount; }

	DATA* Alloc()
	{
		TLS_Bucket* buckets = (TLS_Bucket*)TlsGetValue(tlsIndex);

		// tlsIndex�� ����ִٸ� ��� ��Ŷ�� �Ҵ����� ���� ����
		// ���� �޸� Ǯ���� ��� ��Ŷ ������
		if (buckets == nullptr)
		{
			buckets = new TLS_Bucket;

			buckets->bucket1 = new Bucket(defaultNodeCount, m_bPlacementNew, rootPool);
			buckets->bucket2 = new Bucket(0, m_bPlacementNew, rootPool);

			TlsSetValue(tlsIndex, buckets);

			//// �� ������ �� ��Ŷ 2�� ���� �����Ƿ� �Ҵ� �뷮�� 2��
			//InterlockedAdd64(&m_iCapacity, defaultNodeCount * 2);
		}

		// �ι�° ��Ŷ ���� Ȯ��
		Bucket* bucket = buckets->bucket2;

		// 1. �ι�° ��Ŷ�� ������� ���
		if (bucket->size == 0)
		{
			// ù��° ��Ŷ Ȯ��
			bucket = buckets->bucket1;

			// 2. ù��° ��Ŷ�� ������� ���
			if (bucket->size == 0)
			{
				Node* pTop = nullptr;

				// rootPool���� ��� ������
				if (rootPool->Pop(&pTop))
				{
					// rootPool���� ������ ��带 ��Ŷ�� �Ҵ�
					//bucket->Setting(pTop, defaultNodeCount);
						bucket->pTop = pTop;
						bucket->size = defaultNodeCount;
						//size = iNodeCount;
				}
				// rootPool���� ���� ���
				else
				{
					// ��Ŷ�� ��� ���� �Ҵ� (��Ŷ�� ��带 ��� ä��)
					bucket->newAlloc();
				}
			}
		}

		DATA* object = bucket->Alloc();

		// �Ҵ��� ������ ������ ȣ��
		if (m_bPlacementNew)
			new(object)DATA;

		InterlockedIncrement64(&objectAllocCount);
		InterlockedIncrement64(&objectUseCount);

		return object;
	}

	bool Free(DATA* pData)
	{
		TLS_Bucket* buckets = (TLS_Bucket*)TlsGetValue(tlsIndex);

		// bucket�� ���� ��� (ó�� free)
		if (buckets == nullptr)
		{
			buckets = new TLS_Bucket;

			buckets->bucket1 = new Bucket(defaultNodeCount, m_bPlacementNew, rootPool);
			buckets->bucket2 = new Bucket(0, m_bPlacementNew, rootPool);

			TlsSetValue(tlsIndex, buckets);

			//// �� ������ �� ��Ŷ 2�� ���� �����Ƿ� �Ҵ� �뷮�� 2��
			//InterlockedAdd64(&m_iCapacity, defaultNodeCount * 2);

		}

		//ù��° ��Ŷ ���� Ȯ��
		Bucket* bucket = buckets->bucket1;

		// ù��° ��Ŷ�� ���� á�� ���
		if (bucket->size == defaultNodeCount)
		{
			bucket = buckets->bucket2;

			// �ι�° ��Ŷ�� ���� á�� ���
			if (bucket->size == defaultNodeCount)
			{
				// �ι�° ��Ŷ�� ��带 ��� root pool�� ��ȯ -> top ������ �ѱ�
				rootPool->Push(bucket->GetTop());

				// �ι�° ��Ŷ ����
				bucket->Clear();
			}
		}

		bucket->Free(pData);

		// ��ȯ�� ������ �Ҹ��� ȣ��
		if (m_bPlacementNew)
			pData->~DATA();

		InterlockedIncrement64(&objectFreeCount);
		InterlockedDecrement64(&objectUseCount);

		return true;
	}
};


#endif
