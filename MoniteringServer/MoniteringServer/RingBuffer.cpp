#include <memory.h>

#include "RingBuffer.h"

RingBuffer::RingBuffer() : ringBufferSize(BUFSIZE)
{
	begin = new char[ringBufferSize];
	end = begin + ringBufferSize;
	readPos = writePos = begin;
}

RingBuffer::RingBuffer(int iBufferSize) : ringBufferSize(iBufferSize)
{
	begin = new char[ringBufferSize];
	end = begin + ringBufferSize;
	readPos = writePos = begin;
}

RingBuffer::~RingBuffer()
{
	ClearBuffer();
	if (begin != nullptr)
	{
		delete[] begin;
		begin = nullptr;
	}
}

// �� ���ۿ� ������ �� á�� �� resize
void RingBuffer::Resize()
{
	// temp ���� �迭 �Ҵ� (��� ���� ũ�⸸ �ӽ� ���ۿ� copy�ϸ� ��)
	char* temp = new char[ringBufferSize];

	// ������ ����
	memcpy_s(temp, ringBufferSize * sizeof(char), begin, ringBufferSize * sizeof(char));

	// ���� ���� �迭 ����
	delete[] begin;

	begin = new char[ringBufferSize * 2];
	memcpy_s(begin, ringBufferSize * sizeof(char) * 2, temp, ringBufferSize * sizeof(char));

	// temp ���� �迭 ����
	delete[] temp;
}

int RingBuffer::GetBufferSize(void)
{
	return ringBufferSize - 1;
}

// ���� ������� �뷮 ���
int RingBuffer::GetUseSize(void)
{
	if (writePos >= readPos)
		return writePos - readPos;
	else
		return (end - readPos) + (writePos - begin);
}

// ���� ���ۿ� ���� �뷮 ���.
int RingBuffer::GetFreeSize(void)
{
	if (writePos >= readPos)
		return (end - writePos - 1) + (readPos - begin);
	else
		return readPos - writePos - 1;

}

// ���� �����ͷ� �ܺο��� �ѹ濡 �а�, �� �� �ִ� ����.
// ������ ���� ����
int RingBuffer::DirectEnqueueSize(void)
{
	if (readPos <= writePos)
	{
		if (readPos == begin)
			return end - writePos - 1;
		else
			return end - writePos;
	}
	else
		return readPos - writePos - 1;
}

int RingBuffer::DirectDequeueSize(void)
{
	if (readPos <= writePos)
		return writePos - readPos;
	else
		return end - readPos;
}

//	WritePos �� ����Ÿ ����.
	// Parameters: (char *)����Ÿ ������. (int)ũ��.
	// Return: (int)���� ũ��.
int RingBuffer::Enqueue(const char* chpData, int iSize)
{
	if (GetFreeSize() < iSize)
		return 0;

	int directEnqueueSize = DirectEnqueueSize();

	if (iSize <= directEnqueueSize)
	{
		memcpy_s(writePos, iSize, chpData, iSize);
		MoveWritePtr(iSize);

		return iSize;
	}

	// direct enqueue size���� �������� �����Ͱ� �� ũ�ٸ�
	// �ϴ� ���� �� �ִ¸�ŭ �ְ�(direct enqueue size) ���� size��ŭ �� �� �� enqueue
	memcpy_s(writePos, directEnqueueSize, chpData, directEnqueueSize);
	MoveWritePtr(directEnqueueSize);

	int remainSize = iSize - directEnqueueSize;
	memcpy_s(writePos, remainSize, chpData + directEnqueueSize, remainSize);
	MoveWritePtr(remainSize);

	return iSize;
}

// ReadPos ���� ����Ÿ ������. ReadPos �̵�.
// Parameters: (char *)����Ÿ ������. (int)ũ��.
// Return: (int)������ ũ��.
int RingBuffer::Dequeue(char* chpDest, int iSize)
{
	if (GetUseSize() < iSize)
		return 0;

	int directDequeueSize = DirectDequeueSize();

	if (directDequeueSize >= iSize)
	{
		memcpy_s(chpDest, iSize, readPos, iSize);
		MoveReadPtr(iSize);

		return iSize;
	}

	// direct dequeue size���� �������� �����Ͱ� �� ũ�ٸ�
	// �ϴ� �� �� �ִ¸�ŭ �ְ�(direct dequeue size) ���� size��ŭ �� �� �� dequeue
	memcpy_s(chpDest, directDequeueSize, readPos, directDequeueSize);
	MoveReadPtr(directDequeueSize);

	int remainSize = iSize - directDequeueSize;
	memcpy_s(chpDest + directDequeueSize, remainSize, readPos, remainSize);
	MoveReadPtr(remainSize);

	return iSize;
}

// ReadPos ���� ����Ÿ �о��. ReadPos ����.
// Parameters: (char *)����Ÿ ������. (int)ũ��.
// Return: (int)������ ũ��, ���� : 0
int RingBuffer::Peek(char* chpDest, int iSize)
{
	if (GetUseSize() < iSize) return 0;
	if (DirectDequeueSize() >= iSize)
	{
		memcpy_s(chpDest, iSize, readPos, iSize);
		return iSize;
	}

	// direct dequeue size���� peek�Ϸ��� �����Ͱ� �� ũ�ٸ� �����ۿ��� �� ���� �����Ͱ� �ɰ��� �ִ� ��
	// �ϴ� �� �� �ִ¸�ŭ ����(direct dequeue size), ���� size��ŭ �� �� �� peek
	int directDequeueSize = DirectDequeueSize();
	memcpy_s(chpDest, directDequeueSize, readPos, directDequeueSize);

	int remainSize = iSize - directDequeueSize;
	memcpy_s(chpDest + directDequeueSize, remainSize, begin, remainSize);

	return iSize;
}

// ���ϴ� ���̸�ŭ �б���ġ ���� ���� / ���� ��ġ �̵�
// Return: (int)�̵�ũ��
// writePos ��ġ �̵�
int RingBuffer::MoveWritePtr(int iSize)
{
	writePos += iSize;


	if (writePos >= end)
	{
		int overFlow = writePos - end;
		writePos = begin + overFlow;
	}

	return iSize;
}

// readPos ��ġ �̵�
int RingBuffer::MoveReadPtr(int iSize)
{
	readPos += iSize;

	if (readPos >= end)
	{
		int overFlow = readPos - end;
		readPos = begin + overFlow;
	}

	return iSize;
}

// readPos ��ġ �̵�
// Parameters:�̵��� ��ġ
int RingBuffer::MoveReadPtr(char* ptr)
{
	if (ptr == nullptr)
	{
		return 0;
	}
	readPos = ptr;

	return 1;
}

// ������ ��� ����Ÿ ����.
void RingBuffer::ClearBuffer(void)
{
	readPos = writePos = begin;
}

// ������ Front ������ ����.
char* RingBuffer::GetReadBufferPtr(void)
{
	return readPos;
}

// ������ RearPos ������ ����.
char* RingBuffer::GetWriteBufferPtr(void)
{
	return writePos;
}

// ������ ���� ������
char* RingBuffer::GetBufferPtr(void)
{
	return begin;
}