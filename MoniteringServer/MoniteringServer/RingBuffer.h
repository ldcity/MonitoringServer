#ifndef __RING_BUFFER__
#define __RING_BUFFER__

#define BUFSIZE 512

class RingBuffer
{
public:
	RingBuffer(void);
	RingBuffer(int iBufferSize);
	~RingBuffer();

	void Resize();							// 2��� ������ ũ�� �ø���
	int GetBufferSize(void);

	int GetUseSize(void);					// ���� ������� �뷮 ���.
	int GetFreeSize(void);					// ���� ���ۿ� ���� �뷮 ���.

	// ���� �����ͷ� �ܺο��� �ѹ濡 �а�, �� �� �ִ� ����.
	// ������ ���� ����
	int DirectEnqueueSize(void);
	int DirectDequeueSize(void);

	//	WritePos �� ����Ÿ ����.
	// Parameters: (char *)����Ÿ ������. (int)ũ��.
	// Return: (int)���� ũ��.
	int Enqueue(const char* chpData, int iSize);

	// ReadPos ���� ����Ÿ ������. ReadPos �̵�.
	// Parameters: (char *)����Ÿ ������. (int)ũ��.
	// Return: (int)������ ũ��.
	int Dequeue(char* chpDest, int iSize);

	// ReadPos ���� ����Ÿ �о��. ReadPos ����.
	// Parameters: (char *)����Ÿ ������. (int)ũ��.
	// Return: (int)������ ũ��.
	int Peek(char* chpDest, int iSize);

	// ���ϴ� ���̸�ŭ �б���ġ ���� ���� / ���� ��ġ �̵�
	// Return: (int)�̵�ũ��
	int MoveWritePtr(int iSize);
	int MoveReadPtr(int iSize);
	int MoveReadPtr(char* ptr);

	// ������ ��� ����Ÿ ����.
	void ClearBuffer(void);

	// ������ Front ������ ����.
	char* GetReadBufferPtr(void);

	// ������ RearPos ������ ����.
	char* GetWriteBufferPtr(void);

	// ������ ���� ������
	char* GetBufferPtr(void);


private:
	int ringBufferSize;
	char* readPos;
	char* writePos;
	char* begin;
	char* end;
};

#endif