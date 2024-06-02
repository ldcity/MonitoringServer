#ifndef __LANSERVER_CLASS__
#define __LANSERVER_CLASS__

#include <vector>

#include "SerializingBuffer.h"
#include "Session.h"
#include "LockFreeStack.h"
#include "LOG.h"

#define MAX_SESSION 500


// ���� ������ ��� ��� Ŭ����
class LanServer
{
public:
	LanServer();

	virtual ~LanServer();

	// ���� ���� ����
	bool DisconnectSession(uint64_t sessionID);

	// ��Ŷ ����
	bool SendPacket(uint64_t sessionID, CPacket* packet);

	// Server Start
	bool Start(const wchar_t* IP, unsigned short PORT, int createWorkerThreadCnt, int runningWorkerThreadCnt, bool nagelOff, int maxAcceptCnt);

	// Server Stop
	void Stop();

protected:
	// ==========================================================
	// White IP Check 
	// [PARAM] const wchar_t* IP, unsigned short PORT
	// [RETURN] TRUE : ���� ��� / FALSE : ���� �ź�
	// ==========================================================
	virtual bool OnConnectionRequest(const wchar_t* IP, unsigned short PORT) = 0;

	// ==========================================================
	// ����ó�� �Ϸ� �� ȣ�� 
	// [PARAM] __int64 sessionID
	// [RETURN] X
	// ==========================================================
	virtual void OnClientJoin(uint64_t sessionID, wchar_t* ip, unsigned short port) = 0;

	// ==========================================================
	// �������� �� ȣ��, Player ���� ���ҽ� ����
	// [PARAM] __int64 sessionID
	// [RETURN] X 
	// ==========================================================
	virtual void OnClientLeave(uint64_t sessionID) = 0;

	// ==========================================================
	// ��Ŷ ���� �Ϸ� ��
	// [PARAM] __int64 sessionID, CPacket* packet
	// [RETURN] X 
	// ==========================================================
	virtual void OnRecv(uint64_t sessionID, CPacket* packet) = 0;

	// ==========================================================
	// Error Check
	// [PARAM] int errorCode, wchar_t* msg
	// [RETURN] X 
	// ==========================================================
	virtual void OnError(int errorCode, const wchar_t* msg) = 0;

private:
	friend unsigned __stdcall LanAcceptThread(void* param);
	friend unsigned __stdcall LanWorkerThread(void* param);

	bool LanAcceptThread_serv();
	bool LanWorkerThread_serv();

	// �Ϸ����� ��, �۾� ó��
	bool RecvProc(stSESSION* pSession, long cbTransferred);
	bool SendProc(stSESSION* pSession, long cbTransferred);

	// �ۼ��� ���� ��� ��, �ۼ��� �Լ� ȣ��
	bool RecvPost(stSESSION* pSession);
	bool SendPost(stSESSION* pSession);

	// ���� ���ҽ� ���� �� ����
	void ReleaseSession(stSESSION* pSession);

	// uniqueID�� index�� ������ SessionID ����
	inline uint64_t  CreateSessionID(uint64_t uniqueID, int index)
	{
		return (uint64_t)((uint64_t)index | (uniqueID << SESSION_ID_BITS));
	}

	inline void ReleasePQCS(stSESSION* pSession)
	{
		PostQueuedCompletionStatus(IOCPHandle, 0, (ULONG_PTR)pSession, (LPOVERLAPPED)PQCSTYPE::RELEASE);
	}

	inline int GetSessionIndex(uint64_t sessionID)
	{
		return (int)(sessionID & SESSION_INDEX_MASK);
	}

	inline uint64_t GetSessionID(uint64_t sessionID)
	{
		return (uint64_t)(sessionID >> SESSION_ID_BITS);
	}

private:
	// ������ ����
	SOCKET ListenSocket;								// Listen Socket

	wchar_t mIP[16];									// Server IP
	unsigned short mPORT;								// Server Port

	HANDLE IOCPHandle;									// IOCP Handle
	HANDLE mAcceptThread;								// Accept Thread
	std::vector<HANDLE> mWorkerThreads;						// Worker Threads Count

	long s_workerThreadCount;							// Worker Thread Count (Server)
	long s_runningThreadCount;							// Running Thread Count (Server)

	long s_maxAcceptCount;								// Max Accept Count

	stSESSION* SessionArray;							// Session Array			
	LockFreeStack<int> AvailableIndexStack;				// Available Session Array Index

	enum PQCSTYPE
	{
		SENDPOST = 100,
		SENDPOSTDICONN,
		RELEASE,
		STOP,
	};

protected:
	// logging
	Log* logger;

	// ����͸��� ���� (1�� ����)
	// ������ ���Ǹ� ���� TPS�� �� ������ 1�ʴ� �߻��ϴ� �Ǽ��� ���, �������� �� ���� �հ踦 ��Ÿ��

	__int64 acceptCount;							// Accept Session Count.
	__int64 acceptTPS;								// Accept TPS
	__int64 sessionCnt;								// Session Total Cnt
	__int64 releaseCount;							// Release Session Count
	__int64 releaseTPS;								// Release TPS
	__int64 recvMsgTPS;								// Recv Packet TPS
	__int64 sendMsgTPS;								// Send Packet TPS
	__int64 recvMsgCount;							// Total Recv Packet Count
	__int64 sendMsgCount;							// Total Send Packet Count
	__int64 recvCallTPS;							// Recv Call TPS
	__int64 sendCallTPS;							// Send Call TPS
	__int64 recvCallCount;							// Total Recv Call Count
	__int64 sendCallCount;							// Total Send Call Count
	__int64 recvPendingTPS;							// Recv Pending TPS
	__int64 sendPendingTPS;							// Send Pending TPS
	__int64 recvBytesTPS;							// Recv Bytes TPS
	__int64 sendBytesTPS;							// Send Bytes TPS
	__int64 recvBytes;								// Total Recv Bytes
	__int64 sendBytes;								// Total Send Bytes
	__int64 workerThreadCount;						// Worker Thread Count (Monitering)
	__int64 runningThreadCount;						// Running Thread Count (Monitering)
	bool startMonitering;
};

#endif // !__LANSERVER_CLASS__


