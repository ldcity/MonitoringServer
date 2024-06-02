#ifndef  __SERIALIZING_PACKET__
#define  __SERIALIZING_PACKET__

#include "Exception.h"
#include "TLSFreeList.h"
#include "Define.h"

#define _CRT_SECURE_NO_WARNINGS

class CPacket
{
public:
	/*---------------------------------------------------------------
	Packet Enum.
	----------------------------------------------------------------*/
	enum en_PACKET
	{
		eBUFFER_DEFAULT = 256,		// ��Ŷ�� �⺻ ���� ������.
		NET_HEADER_SIZE = 5,
		LAN_HEADER_SIZE = 2
	};

	//////////////////////////////////////////////////////////////////////////
	// ������, �ı���.
	//
	// Return:
	//////////////////////////////////////////////////////////////////////////
	CPacket();
	CPacket(int iBufferSize);
	CPacket(CPacket& clSrcPacket);

	virtual	~CPacket();

	//////////////////////////////////////////////////////////////////////////
// ���� �ø���.
//
// Parameters: ����.
// Return: ����.
//////////////////////////////////////////////////////////////////////////
	void	Resize(const char* methodName, int size);

	//////////////////////////////////////////////////////////////////////////
	// ��Ŷ û��.
	//
	// Parameters: ����.
	// Return: ����.
	//////////////////////////////////////////////////////////////////////////
	inline void	Clear(void)
	{
		m_iDataSize = 0;

		readPos = writePos = m_chpBuffer + NET_HEADER_SIZE;
	}


	//////////////////////////////////////////////////////////////////////////
	// ���� ������ ���.
	//
	// Parameters: ����.
	// Return: (int)��Ŷ ���� ������ ���.
	//////////////////////////////////////////////////////////////////////////
	inline int	GetBufferSize(void)
	{
		return m_iBufferSize;
	}

	//////////////////////////////////////////////////////////////////////////
	// ���� ������ ���.
	//
	// Parameters: ����.
	// Return: (char *)���� ������.
	//////////////////////////////////////////////////////////////////////////
	inline char* GetBufferPtr(void)
	{
		return m_chpBuffer;
	}

	//////////////////////////////////////////////////////////////////////////
	// ���� Pos �̵�. (�����̵��� �ȵ�)
	// GetBufferPtr �Լ��� �̿��Ͽ� �ܺο��� ������ ���� ������ ������ ��� ���. 
	//
	// Parameters: (int) �̵� ������.
	// Return: (int) �̵��� ������.
	//////////////////////////////////////////////////////////////////////////
	inline int		MoveWritePos(int iSize)
	{
		if (iSize < 0)
			return -1;
		if (m_iBufferSize - m_iDataSize - NET_HEADER_SIZE < iSize)
			return 0;

		writePos += iSize;
		m_iDataSize += iSize;

		return iSize;
	}

	inline int		MoveReadPos(int iSize)
	{
		if (iSize < 0)
			return -1;
		if (m_iDataSize < iSize)
			return 0;

		readPos += iSize;
		m_iDataSize -= iSize;

		if (readPos == writePos) Clear();

		return iSize;
	}

	/* ============================================================================= */
	// ������ �����ε�
	/* ============================================================================= */
	CPacket& operator = (CPacket& clSrcPacket);

	//////////////////////////////////////////////////////////////////////////
	// �ֱ�.	�� ���� Ÿ�Ը��� ��� ����.
	//////////////////////////////////////////////////////////////////////////
	inline CPacket& operator << (unsigned char byValue)
	{
		// ���� ������� ���� ���� ũ�Ⱑ �� ũ�� �ٷ� return
		if (m_iBufferSize - NET_HEADER_SIZE - m_iDataSize < sizeof(unsigned char))
			Resize("operator << (unsigned char) ", sizeof(unsigned char));

		memcpy_s(writePos, m_iBufferSize - NET_HEADER_SIZE - m_iDataSize, &byValue, sizeof(unsigned char));
		MoveWritePos(sizeof(unsigned char));

		return *this;
	}

	inline CPacket& operator << (char chValue)
	{
		// ���� ������� ���� ���� ũ�Ⱑ �� ũ�� �ٷ� return
		if (m_iBufferSize - NET_HEADER_SIZE - m_iDataSize < sizeof(char))
			Resize("operator << (char) ", sizeof(char));

		memcpy_s(writePos, m_iBufferSize - NET_HEADER_SIZE - m_iDataSize, &chValue, sizeof(char));
		MoveWritePos(sizeof(char));

		return *this;
	}

	inline CPacket& operator << (short shValue)
	{
		// ���� ������� ���� ���� ũ�Ⱑ �� ũ�� �ٷ� return
		if (m_iBufferSize - NET_HEADER_SIZE - m_iDataSize < sizeof(short))
			Resize("operator << (short) ", sizeof(short));

		memcpy_s(writePos, m_iBufferSize - NET_HEADER_SIZE - m_iDataSize, &shValue, sizeof(short));
		MoveWritePos(sizeof(short));

		return *this;
	}

	inline CPacket& operator << (unsigned short wValue)
	{
		// ���� ������� ���� ���� ũ�Ⱑ �� ũ�� �ٷ� return
		if (m_iBufferSize - NET_HEADER_SIZE - m_iDataSize < sizeof(unsigned short))
			Resize("operator << (short) ", sizeof(unsigned short));

		memcpy_s(writePos, m_iBufferSize - NET_HEADER_SIZE - m_iDataSize, &wValue, sizeof(unsigned short));
		MoveWritePos(sizeof(unsigned short));

		return *this;
	}

	inline CPacket& operator << (int iValue)
	{
		// ���� ������� ���� ���� ũ�Ⱑ �� ũ�� �ٷ� return
		if (m_iBufferSize - NET_HEADER_SIZE - m_iDataSize < sizeof(int))
			Resize("operator << (int) ", sizeof(int));

		memcpy_s(writePos, m_iBufferSize - NET_HEADER_SIZE - m_iDataSize, &iValue, sizeof(int));
		MoveWritePos(sizeof(int));

		return *this;
	}

	inline CPacket& operator << (unsigned long lValue)
	{
		// ���� ������� ���� ���� ũ�Ⱑ �� ũ�� �ٷ� return
		if (m_iBufferSize - NET_HEADER_SIZE - m_iDataSize < sizeof(unsigned long))
			Resize("operator << (long) ", sizeof(long));

		memcpy_s(writePos, m_iBufferSize - NET_HEADER_SIZE - m_iDataSize, &lValue, sizeof(unsigned long));
		MoveWritePos(sizeof(unsigned long));

		return *this;
	}

	inline CPacket& operator << (float fValue)
	{
		// ���� ������� ���� ���� ũ�Ⱑ �� ũ�� �ٷ� return
		if (m_iBufferSize - NET_HEADER_SIZE - m_iDataSize < sizeof(float))
			Resize("operator << (float) ", sizeof(float));

		memcpy_s(writePos, m_iBufferSize - NET_HEADER_SIZE - m_iDataSize, &fValue, sizeof(float));
		MoveWritePos(sizeof(float));

		return *this;
	}

	inline CPacket& operator << (__int64 iValue)
	{
		// ���� ������� ���� ���� ũ�Ⱑ �� ũ�� �ٷ� return
		if (m_iBufferSize - NET_HEADER_SIZE - m_iDataSize < sizeof(__int64))
			Resize("operator << (__int64) ", sizeof(__int64));

		memcpy_s(writePos, m_iBufferSize - NET_HEADER_SIZE - m_iDataSize, &iValue, sizeof(__int64));
		MoveWritePos(sizeof(__int64));

		return *this;
	}

	inline CPacket& operator << (double dValue)
	{
		// ���� ������� ���� ���� ũ�Ⱑ �� ũ�� �ٷ� return
		if (m_iBufferSize - NET_HEADER_SIZE - m_iDataSize < sizeof(double))
			Resize("operator << (double) ", sizeof(double));

		memcpy_s(writePos, m_iBufferSize - NET_HEADER_SIZE - m_iDataSize, &dValue, sizeof(double));
		MoveWritePos(sizeof(double));

		return *this;
	}

	//////////////////////////////////////////////////////////////////////////
	// ����.	�� ���� Ÿ�Ը��� ��� ����.
	//////////////////////////////////////////////////////////////////////////
	inline CPacket& operator >> (unsigned char& byValue)
	{
		if (m_iDataSize < sizeof(unsigned char))
			throw SerializingBufferException("Failed to read float", "operator >> (unsigned char)", __LINE__, this->GetBufferPtr());


		memcpy_s(&byValue, sizeof(unsigned char), readPos, sizeof(unsigned char));

		// ���� ������ ������ �̵�
		MoveReadPos(sizeof(unsigned char));

		return *this;
	}

	inline CPacket& operator >> (char& chValue)
	{
		if (m_iDataSize < sizeof(char))
			throw SerializingBufferException("Failed to read float", "operator >> (char)", __LINE__, this->GetBufferPtr());


		memcpy_s(&chValue, sizeof(char), readPos, sizeof(char));

		// ���� ������ ������ �̵�
		MoveReadPos(sizeof(char));

		return *this;
	}

	inline CPacket& operator >> (short& shValue)
	{
		if (m_iDataSize < sizeof(short))
			throw SerializingBufferException("Failed to read float", "operator >> (short)", __LINE__, this->GetBufferPtr());

		memcpy_s(&shValue, sizeof(short), readPos, sizeof(short));

		// ���� ������ ������ �̵�
		MoveReadPos(sizeof(short));

		return *this;
	}

	inline CPacket& operator >> (unsigned short& wValue)
	{
		if (m_iDataSize < sizeof(unsigned short))
			throw SerializingBufferException("Failed to read float", "operator >> (unsigned short)", __LINE__, this->GetBufferPtr());


		memcpy_s(&wValue, sizeof(unsigned short), readPos, sizeof(unsigned short));

		// ���� ������ ������ �̵�
		MoveReadPos(sizeof(unsigned short));

		return *this;
	}

	inline CPacket& operator >> (int& iValue)
	{
		if (m_iDataSize < sizeof(int))
			throw SerializingBufferException("Failed to read float", "operator >> (int)", __LINE__, this->GetBufferPtr());

		memcpy_s(&iValue, sizeof(int), readPos, sizeof(int));

		// ���� ������ ������ �̵�
		MoveReadPos(sizeof(int));

		return *this;
	}

	inline CPacket& operator >> (unsigned long& dwValue)
	{
		if (m_iDataSize < sizeof(unsigned long))
			throw SerializingBufferException("Failed to read float", "operator >> (unsigned long)", __LINE__, this->GetBufferPtr());


		memcpy_s(&dwValue, sizeof(unsigned long), readPos, sizeof(unsigned long));

		// ���� ������ ������ �̵�
		MoveReadPos(sizeof(unsigned long));

		return *this;
	}

	inline CPacket& operator >> (float& fValue)
	{
		if (m_iDataSize < sizeof(float))
			throw SerializingBufferException("Failed to read float", "operator >> (float)", __LINE__, this->GetBufferPtr());

		memcpy_s(&fValue, sizeof(float), readPos, sizeof(float));

		// ���� ������ ������ �̵�
		MoveReadPos(sizeof(float));

		return *this;
	}

	inline CPacket& operator >> (__int64& iValue)
	{
		if (m_iDataSize < sizeof(__int64))
			throw SerializingBufferException("Failed to read float", "operator >> (__int64)", __LINE__, this->GetBufferPtr());

		memcpy_s(&iValue, sizeof(__int64), readPos, sizeof(__int64));

		// ���� ������ ������ �̵�
		MoveReadPos(sizeof(__int64));

		return *this;
	}

	inline CPacket& operator >> (double& dValue)
	{
		if (m_iDataSize < sizeof(double))
			throw SerializingBufferException("Failed to read float", "operator >> (double)", __LINE__, this->GetBufferPtr());

		memcpy_s(&dValue, sizeof(double), readPos, sizeof(double));

		// ���� ������ ������ �̵�
		MoveReadPos(sizeof(double));

		return *this;
	}


	//////////////////////////////////////////////////////////////////////////
	// ����Ÿ peek
	//
	// Parameters: (char *)Dest ������. (int)Size.
	// Return: (int)������ ������.
	//////////////////////////////////////////////////////////////////////////
	inline int		Peek(char* chpDest, int iSize)
	{
		if (m_iDataSize < iSize)
			throw SerializingBufferException("Failed to read float", "GetData", __LINE__, this->GetBufferPtr());

		memcpy_s(chpDest, iSize, m_chpBuffer, iSize);

		return iSize;
	}

	inline int		LanPeek(char* chpDest, int iSize)
	{
		if (m_iDataSize < iSize)
			throw SerializingBufferException("Failed to read float", "GetData", __LINE__, this->GetBufferPtr());

		memcpy_s(chpDest, iSize, lanHeaderPtr, iSize);

		return iSize;
	}

	//////////////////////////////////////////////////////////////////////////
	// ����Ÿ ���.
	//
	// Parameters: (char *)Dest ������. (int)Size.
	// Return: (int)������ ������.
	//////////////////////////////////////////////////////////////////////////
	inline int		GetData(char* chpDest, int iSize)
	{
		if (m_iDataSize < iSize)
			throw SerializingBufferException("Failed to read float", "GetData", __LINE__, this->GetBufferPtr());

		memcpy_s(chpDest, iSize, readPos, iSize);
		MoveReadPos(iSize);

		return iSize;
	}

	//////////////////////////////////////////////////////////////////////////
	// ����Ÿ ����.
	//
	// Parameters: (char *)Src ������. (int)SrcSize.
	// Return: (int)������ ������.
	//////////////////////////////////////////////////////////////////////////
	inline int		PutData(char* chpSrc, int iSrcSize)
	{
		if (m_iBufferSize - NET_HEADER_SIZE - m_iDataSize < iSrcSize)
			Resize("PutData", iSrcSize);

		memcpy_s(writePos, m_iBufferSize - NET_HEADER_SIZE - m_iDataSize, chpSrc, iSrcSize);
		MoveWritePos(iSrcSize);

		return iSrcSize;
	}

	// ������ Front ������ ����.
	inline char* GetReadBufferPtr(void)
	{
		return readPos;
	}

	// ������ RearPos ������ ����.
	inline char* GetWriteBufferPtr(void)
	{
		return writePos;
	}

	// Net ��� ������
	inline char* GetNetBufferPtr(void)
	{
		return m_chpBuffer;
	}

	// Lan ��� ������
	inline char* GetLanBufferPtr(void)
	{
		return lanHeaderPtr;
	}

	// ���� ���̷ε� ������ ũ��
	inline int GetDataSize()
	{
		return m_iDataSize;
	}

	// ��� ���� ���̷ε� ������ ũ�� (Net)
	inline int GetNetDataSize()
	{
		return m_iDataSize + NET_HEADER_SIZE;
	}

	// ��� ���� ���̷ε� ������ ũ�� (Lan)
	inline int GetLanDataSize()
	{
		return m_iDataSize + LAN_HEADER_SIZE;
	}

	// default ��� ������
	inline int GetDefaultHeaderSize()
	{
		return NET_HEADER_SIZE;
	}

	// ��� ���� (Lan)
	inline void SetLanHeader()
	{
		LANHeader lanHeader;
		lanHeader.len = GetDataSize();

		memcpy_s(lanHeaderPtr, sizeof(LANHeader), &lanHeader, sizeof(LANHeader));
	}

	// ��� ���� & ���ڵ� (Net)
	void Encoding();

	// ���ڵ� (Net)
	bool Decoding();


	// ��������Ʈ���� �Ҵ�
	inline static CPacket* Alloc()
	{
		CPacket* packet = PacketPool.Alloc();

		packet->addRefCnt();

		InterlockedExchange8((char*)&packet->isEncoded, false);

		packet->Clear();

		return packet;
	}

	// ��������Ʈ�� �ݳ�
	inline static void Free(CPacket* packet)
	{
		// ��Ŷ�� �����ϴ� ���� ���ٸ� �׶����� Ǯ�� ��ȯ
		if (0 == packet->subRefCnt())
		{
			packet->Clear();
			PacketPool.Free(packet);
		}

		if (packet->ref_cnt < 0)
			CRASH();
	}

	inline static void SetCode(unsigned char code)
	{
		m_code = code;
	}

	inline static unsigned char GetCode()
	{
		return m_code;
	}

	inline static void SetKey(unsigned char key)
	{
		m_key = key;
	}

	inline void addRefCnt()
	{
		InterlockedIncrement((LONG*)&ref_cnt);
	}

	inline LONG subRefCnt()
	{
		return InterlockedDecrement((LONG*)&ref_cnt);
	}

	inline static __int64 GetPoolCapacity()
	{
		return PacketPool.GetCapacity();
	}

	inline static __int64 GetPoolTotalAllocCnt()
	{
		return PacketPool.GetObjectAllocCount();
	}

	inline static __int64 GetPoolTotalFreeCnt()
	{
		return PacketPool.GetObjectFreeCount();
	}

	inline static __int64 GetPoolUseCnt()
	{
		return PacketPool.GetObjectUseCount();
	}

	// Packet Memory Pool
	inline static TLSObjectPool<CPacket> PacketPool = TLSObjectPool<CPacket>(1000);

private:

	// ���� ī��Ʈ
	int alignas(64) ref_cnt;

	// ����ȭ ���� ũ��
	int	m_iBufferSize;

	// ���� ���ۿ� ������� ������ (payload)
	int	m_iDataSize;

	// ����ȭ ���� �Ҵ� ù ������ (netPtr)
	char* m_chpBuffer;

	// lan header ptr
	char* lanHeaderPtr;

	// �б� ��ġ
	char* readPos;

	// ���� ��ġ
	char* writePos;

	// ���ڵ� ���� Ȯ�� flag
	alignas(64) bool isEncoded;

	inline static unsigned char m_code;

	inline static unsigned char m_key;
};

#endif