#include "SerializingBuffer.h"
#include "Define.h"

CPacket::CPacket() : m_iDataSize(0), m_iBufferSize(eBUFFER_DEFAULT), m_chpBuffer(nullptr), isEncoded(false), ref_cnt(0)
{
	m_chpBuffer = new char[m_iBufferSize + 1];

	lanHeaderPtr = m_chpBuffer + NET_HEADER_SIZE - LAN_HEADER_SIZE;
	readPos = writePos = m_chpBuffer + NET_HEADER_SIZE;
}

CPacket::CPacket(int iBufferSize) : m_iDataSize(0), m_iBufferSize(iBufferSize), m_chpBuffer(nullptr), isEncoded(false), ref_cnt(0)
{
	m_chpBuffer = new char[m_iBufferSize + 1];

	lanHeaderPtr = m_chpBuffer + NET_HEADER_SIZE - LAN_HEADER_SIZE;
	readPos = writePos = m_chpBuffer + NET_HEADER_SIZE;
}

CPacket::CPacket(CPacket& clSrcPacket) : m_iDataSize(0), m_iBufferSize(eBUFFER_DEFAULT), m_chpBuffer(nullptr), isEncoded(false)
{
	if (this != &clSrcPacket)
	{
		// ���� ������ clear
		Clear();

		int copyBufferSize;

		// source ���� ũ�Ⱑ dest ���� ũ�⺸�� Ŭ ���, ������ ũ��� dest ���� ũ��
		if (m_iBufferSize < clSrcPacket.m_iBufferSize)
			copyBufferSize = m_iBufferSize;
		else
			copyBufferSize = clSrcPacket.m_iBufferSize;

		memcpy_s(m_chpBuffer, m_iBufferSize, clSrcPacket.GetBufferPtr(), copyBufferSize);

		m_iDataSize = clSrcPacket.m_iDataSize;
		m_iBufferSize = clSrcPacket.m_iBufferSize;

		lanHeaderPtr = clSrcPacket.lanHeaderPtr;

		readPos = clSrcPacket.readPos;
		writePos = clSrcPacket.writePos;
	}
}

CPacket::~CPacket()
{
	Clear();
	if (m_chpBuffer != nullptr)
	{
		delete[] m_chpBuffer;
		m_chpBuffer = nullptr;
	}
}

void CPacket::Resize(const char* methodName, int size)
{
	// ���� ����ȭ ���ۿ� �����ִ� ������ �ӽ� ���ۿ� ����
	char* temp = new char[m_iDataSize];

	memcpy_s(temp, m_iDataSize, m_chpBuffer, m_iDataSize);

	// ���� ����ȭ ���� delete
	delete[] m_chpBuffer;

	int oldSize = m_iBufferSize;

	// default ����ȭ ���� ũ��(�����ִ� ������ ũ�⿡�� �ʿ���ϴ� ������ ũ�⸸ŭ ���ϰ� 2��� �ø�
	m_iBufferSize = (oldSize + size) * 2;

	// ���ο� ����ȭ ���� �Ҵ� & �ӽ� ���ۿ� �ִ� ������ ����
	m_chpBuffer = new char[m_iBufferSize];
	memcpy_s(m_chpBuffer, m_iBufferSize, temp, m_iDataSize);

	lanHeaderPtr = m_chpBuffer + NET_HEADER_SIZE - LAN_HEADER_SIZE;
	readPos = writePos = m_chpBuffer + NET_HEADER_SIZE;

	// �ӽ� ���� delete
	delete[] temp;
}


CPacket& CPacket::operator = (CPacket& clSrcPacket)
{
	if (this == &clSrcPacket) return *this;

	// ���� ������ clear
	Clear();

	int copyBufferSize;

	// source ���� ũ�Ⱑ dest ���� ũ�⺸�� Ŭ ���, ������ ũ��� dest ���� ũ��
	if (m_iBufferSize < clSrcPacket.m_iBufferSize)
		copyBufferSize = m_iBufferSize;
	else
		copyBufferSize = clSrcPacket.m_iBufferSize;

	memcpy_s(m_chpBuffer, m_iBufferSize, clSrcPacket.GetBufferPtr(), copyBufferSize);

	m_iDataSize = clSrcPacket.m_iDataSize;
	m_iBufferSize = clSrcPacket.m_iBufferSize;

	lanHeaderPtr = m_chpBuffer + NET_HEADER_SIZE - LAN_HEADER_SIZE;

	readPos = clSrcPacket.readPos;
	writePos = clSrcPacket.writePos;

	return *this;
}

// ���ڵ�
void CPacket::Encoding()
{
	if (true == InterlockedExchange8((char*)&isEncoded, true))
	{
		return;
	}

	NetHeader netHeader;
	netHeader.code = m_code;
	netHeader.len = m_iDataSize;

	uint64_t checksum = 0;

	// üũ�� ���
	for (unsigned char i = 0; i < m_iDataSize; i++)
	{
		checksum += *((unsigned char*)readPos + i);
	}

	checksum %= 256;
	netHeader.checkSum = checksum;
	srand(checksum);
	netHeader.randKey = rand();

	memcpy_s(m_chpBuffer, sizeof(NetHeader), &netHeader, sizeof(NetHeader));

	unsigned char randKey = *(unsigned char*)(m_chpBuffer + 3);
	unsigned char* d = (unsigned char*)(m_chpBuffer + 4);
	unsigned char p = 0;
	unsigned char e = 0;
	unsigned char size = m_iDataSize + 1;			// üũ�� ���� ���̷ε�

	for (unsigned char i = 0; i < size; i++)
	{
		p = *(d + i) ^ (p + randKey + i + 1);
		e = p ^ (e + m_key + i + 1);
		*(d + i) = e;
	}
}

// ���ڵ�
bool CPacket::Decoding()
{
	unsigned char randKey = *((unsigned char*)m_chpBuffer + 3);
	unsigned char* d = (unsigned char*)m_chpBuffer + 4;
	unsigned char size = *((unsigned short*)(m_chpBuffer + 1)) + 1;			// üũ�� ���� ���̷ε�

	for (unsigned char i = size - 1; i >= 1; i--)
	{
		*(d + i) ^= (*(d + (i - 1)) + m_key + (i + 1));
	}

	*d ^= (m_key + 1);

	for (unsigned char i = size - 1; i >= 1; i--)
	{
		*(d + i) ^= (*(d + (i - 1)) + randKey + (i + 1));
	}

	*d ^= (randKey + 1);

	uint64_t checksum = 0;

	char* payloadPtr = m_chpBuffer + sizeof(NetHeader);

	// üũ�� ���
	for (unsigned char i = 0; i < m_iDataSize; i++)
	{
		checksum += *((unsigned char*)payloadPtr + i);
	}

	checksum %= 256;

	// ���� üũ��� ��ȣȭ�� üũ���� �ٸ� ���
	if (checksum != *d)
		return false;

	return true;
}
