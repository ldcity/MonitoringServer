#include <process.h>
#include <ws2tcpip.h>

#include "LanServer.h"
#include "Define.h"


// ========================================================================
// Thread Call
// ========================================================================
// Accept Thread Call
unsigned __stdcall LanAcceptThread(void* param)
{
	LanServer* lanServ = (LanServer*)param;

	lanServ->LanAcceptThread_serv();

	return 0;
}

// Worker Thread Call
unsigned __stdcall LanWorkerThread(void* param)
{
	LanServer* lanServ = (LanServer*)param;

	lanServ->LanWorkerThread_serv();

	return 0;
}


LanServer::LanServer() : ListenSocket(INVALID_SOCKET), mIP{ 0 }, mPORT(0), IOCPHandle(0), mAcceptThread(0), mWorkerThreads{ 0 },
	SessionArray{ nullptr }, acceptCount(0), releaseCount(0), recvMsgTPS(0), sendMsgTPS(0),
	recvMsgCount(0), sendMsgCount(0), recvCallTPS(0), sendCallTPS(0), recvCallCount(0), sendCallCount(0), recvPendingTPS(0), sendPendingTPS(0),
	recvBytesTPS(0), sendBytesTPS(0), recvBytes(0), sendBytes(0), s_workerThreadCount(0), s_runningThreadCount(0), s_maxAcceptCount(0), startMonitering(false)
{
	// ========================================================================
	// Initialize
	// ========================================================================
	logger = new Log(L"LanServer");

	wprintf(L"LanServer Initializing...\n");

	WSADATA  wsaData;

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		int initError = WSAGetLastError();

		return;
	}
}

LanServer::~LanServer()
{
	Stop();
}

bool LanServer::Start(const wchar_t* IP, unsigned short PORT, int createWorkerThreadCnt, int runningWorkerThreadCnt, bool nagelOff, int maxAcceptCnt)
{
	s_maxAcceptCount = maxAcceptCnt;

	wmemcpy_s(mIP, wcslen(IP) + 1, IP, wcslen(IP) + 1);
	mPORT = PORT;

	SessionArray = new stSESSION[s_maxAcceptCount];

	// ���� �ε����� ������������ ������ ���� max index���� push
	for (int i = s_maxAcceptCount - 1; i >= 0; i--)
	{
		// ��� ������ �ε��� push
		AvailableIndexStack.Push(i);
	}

	// Create Listen Socket
	ListenSocket = socket(AF_INET, SOCK_STREAM, 0);

	if (INVALID_SOCKET == ListenSocket)
	{
		int sockError = WSAGetLastError();

		return false;
	}

	SOCKADDR_IN serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(PORT);
	InetPtonW(AF_INET, IP, &serverAddr.sin_addr);

	// bind
	if (bind(ListenSocket, (SOCKADDR*)&serverAddr, sizeof(SOCKADDR_IN)) == SOCKET_ERROR)
	{
		int bindError = WSAGetLastError();

		return false;
	}

	// TCP Send Buffer Remove - zero copy
	int sendVal = 0;
	if (setsockopt(ListenSocket, SOL_SOCKET, SO_SNDBUF, (const char*)&sendVal, sizeof(sendVal)) == SOCKET_ERROR)
	{
		int setsockoptError = WSAGetLastError();

		return false;
	}

	if (nagelOff)
	{
		// Nagle off
		if (setsockopt(ListenSocket, IPPROTO_TCP, TCP_NODELAY, (const char*)&nagelOff, sizeof(nagelOff)) == SOCKET_ERROR)
		{
			int setsockoptError = WSAGetLastError();

			return false;
		}
	}

	// TIME_WAIT off
	struct linger ling;
	ling.l_onoff = 1;
	ling.l_linger = 0;
	if (setsockopt(ListenSocket, SOL_SOCKET, SO_LINGER, (const char*)&ling, sizeof(ling)) == SOCKET_ERROR)
	{
		int setsockoptError = WSAGetLastError();

		return false;
	}

	// listen
	if (listen(ListenSocket, SOMAXCONN) == SOCKET_ERROR)
	{
		int listenError = WSAGetLastError();

		return false;
	}

	SYSTEM_INFO si;
	GetSystemInfo(&si);

	// CPU Core Counting
	// Worker Thread ������ 0 ���϶��, �ھ� ���� * 2 �� �缳��
	if (createWorkerThreadCnt <= 0)
		s_workerThreadCount = si.dwNumberOfProcessors * 2;
	else
		s_workerThreadCount = createWorkerThreadCnt;

	// Running Thread�� CPU Core ������ �ʰ��Ѵٸ� CPU Core ������ �缳��
	if (runningWorkerThreadCnt > si.dwNumberOfProcessors)
		s_runningThreadCount = si.dwNumberOfProcessors;
	else
		s_runningThreadCount = runningWorkerThreadCnt;

	// Create I/O Completion Port
	IOCPHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, s_runningThreadCount);
	if (IOCPHandle == NULL)
	{
		int iocpError = WSAGetLastError();

		return false;
	}

	// ========================================================================
	// Create Thread
	// ========================================================================
	// 
	// Accept Thread
	mAcceptThread = (HANDLE)_beginthreadex(NULL, 0, LanAcceptThread, this, 0, NULL);
	if (LanAcceptThread == NULL)
	{
		int threadError = GetLastError();

		return false;
	}

	// Worker Thread
	mWorkerThreads.resize(s_workerThreadCount);
	for (int i = 0; i < mWorkerThreads.size(); i++)
	{
		mWorkerThreads[i] = (HANDLE)_beginthreadex(NULL, 0, LanWorkerThread, this, 0, NULL);
		if (mWorkerThreads[i] == NULL)
		{
			int threadError = GetLastError();

			return false;
		}
	}

	return true;
}

// Accept Thread
bool LanServer::LanAcceptThread_serv()
{
	SOCKADDR_IN clientAddr;
	int addrLen = sizeof(SOCKADDR_IN);
	SOCKET clientSocket;
	DWORD recvBytes;

	// ������ Ŭ�� ���� ���
	wchar_t szClientIP[16] = { 0 };

	unsigned long long s_sessionUniqueID = 0;				// Unique Value

	while (true)
	{
		// accept
		clientSocket = accept(ListenSocket, (SOCKADDR*)&clientAddr, &addrLen);

		if (clientSocket == INVALID_SOCKET)
		{
			int acceptError = GetLastError();

			break;
		}

		InetNtopW(AF_INET, &clientAddr.sin_addr, szClientIP, 16);

		if (!OnConnectionRequest(szClientIP, ntohs(clientAddr.sin_port)))
		{
			// ���� �ź�
			// ...
			closesocket(clientSocket);
			break;
		}

		// ��� ������ index ����
		int index;

		// ����ִ� �迭 ã��
		// index stack�� ��������� �迭�� �� ����� (full!)
		if (!AvailableIndexStack.Pop(&index))
		{
			// ��� accept �ߴ� ���� ����
			closesocket(clientSocket);

			continue;
		}

		// ���� ��
		InterlockedIncrement64(&acceptTPS);
		InterlockedIncrement64(&acceptCount);

		// �ش� index�� ���� ����
		stSESSION* session = &SessionArray[index];

		// ���� ����ī��Ʈ ���� -> accept �Լ� �������� ���� 
		// -> accept �ܰ迡�� ���� ���� ��, ���� ����� �� ��, refcount�� 0�� �Ǵ� �� ����
		// increment�� �ƴ϶� exchange�ϴ� ���� 
		// -> ������ ����Ͽ� ��ȯ�� �����̸� release�ϸ鼭 �����ִ� release flag ���� ����� ����

		// * �Ͼ �� �ִ� ��Ȳ
		// recvpost�� �����Ͽ� refcount(1) ���� ��, WSARecv���� �ɾ��ְ� IO_PENDING ������ ���µ�,
		// �Ϸ� ������ ���� ���� �� ��ġ���� rst or ���������� disconnect�Ǹ� refcount�� 0�� �Ǿ� �� ������ relese��
		// �׸��� ���Ҵ�Ǿ� �ٸ� ������ �Ǿ����� �� �־� �Ϸ� ������ ���Ҵ�� �������� �� �� ����
		InterlockedExchange64(&session->ioRefCount, 1);

		session->sessionID = CreateSessionID(++s_sessionUniqueID, index);

		__int64 id = session->sessionID;

		// ������ ���ο� �����ִ� �� ó��
		session->recvRingBuffer.ClearBuffer();
		ZeroMemory(session->IP_str, sizeof(session->IP_str));
		ZeroMemory(&session->m_stSendOverlapped, sizeof(OVERLAPPED));
		ZeroMemory(&session->m_stRecvOverlapped, sizeof(OVERLAPPED));

		session->IP_num = 0;
		session->PORT = 0;
		//session->LastRecvTime = 0;

		session->m_socketClient = clientSocket;
		wcscpy_s(session->IP_str, _countof(session->IP_str), szClientIP);
		session->PORT = ntohs(clientAddr.sin_port);

		// ����flag ����
		InterlockedExchange8((char*)&session->isDisconnected, false);

		// IOCP�� ���� ����
		// ���� �ּҰ��� Ű ��
		if (CreateIoCompletionPort((HANDLE)clientSocket, IOCPHandle, (ULONG_PTR)session, 0) == NULL)
		{
			int ciocpError = WSAGetLastError();

			// accept�� �ٷ� ��ȯ�ϴ� ���̹Ƿ� iocount ���� �� io �Ϸ� ���� Ȯ��
			if (0 == InterlockedDecrement64((LONG64*)&session->ioRefCount))
			{
				ReleaseSession(session);
				continue;
			}
		}

		// ���� ó��
		// ������ �޾Ƴ��� id�� �Ű������� ���� -> ������ id ��������� ���� ������ �����ϰ� �Ǹ�
		// �� ���̿� ���Ҵ�Ǿ� �ٸ� ������ �� ��, �ش� id�� ���� player�� ������ �� ����
		// ex) ���� Ŭ���̾�Ʈ ���� ��, ������ �� �����ƴµ� player�� �����ִ� ���� �߻�
		OnClientJoin(id, szClientIP, ntohs(clientAddr.sin_port));

		InterlockedIncrement64(&sessionCnt);

		// �񵿱� recv I/O ��û
		RecvPost(session);

		// accept���� �ø� ����ī��Ʈ ����
		if (0 == InterlockedDecrement64(&session->ioRefCount))
		{
			ReleaseSession(session);
		}
	}

	return true;
}

// Worker Thread
bool LanServer::LanWorkerThread_serv()
{
	DWORD threadID = GetCurrentThreadId();

	stSESSION* pSession = nullptr;
	BOOL bSuccess = true;
	long cbTransferred = 0;
	LPOVERLAPPED pOverlapped = nullptr;

	bool completionOK;

	while (true)
	{
		// �ʱ�ȭ
		cbTransferred = 0;
		pSession = nullptr;
		pOverlapped = nullptr;
		completionOK = false;

		// GQCS call
			// client�� send���� �����ʰ� �ٷ� disconnect �� ��� -> WorkerThread���� recv 0�� ���� GQCS�� ���
		bSuccess = GetQueuedCompletionStatus(IOCPHandle, (LPDWORD)&cbTransferred, (PULONG_PTR)&pSession,
			&pOverlapped, INFINITE);

		// IOCP Error or TIMEOUT or PQCS�� ���� NULL ����
		// ���� ����������� error counting�� �Ͽ� 5ȸ �̻� �߻����� ��, ���� �۾� ���� ���̵� �ֱ� ��
		// �Ϸ������� �ȿ��� ���, �̹� ������ skip
		if (pOverlapped == NULL)
		{
			int iocpError = WSAGetLastError();

			break;
		}

		// sendpost
		if (cbTransferred == 0 && (unsigned char)pOverlapped == PQCSTYPE::SENDPOST)
		{
			InterlockedExchange8((char*)&pSession->sendFlag, false);

			// �ٸ� IOCP worker thread�� ���ؼ� dequeue�Ǿ� sendQ�� ������� ���� ������
			// �׷� �� ���� �Լ� call�� �Ͽ� ���ο��� ���� �˻縦 �� �ʿ� ���� (���ɿ� ������)
			if (pSession->sendQ.GetSize() > 0)
				SendPost(pSession);
		}
		// rlease
		else if (cbTransferred == 0 && (unsigned char)pOverlapped == PQCSTYPE::RELEASE)
		{
			// ioRefCount�� 0�� ��쿡 PQCS�� IOCP ��Ʈ�� enqueue�� ���̹Ƿ�
			// �̹� �� ������ ioRefCount�� 0�� -> continue�� �Ѱܾ� �ϴܿ� ioRefCount�� �����ִ� ���� skip ����
			ReleaseSession(pSession);
			continue;
		}
		// Send / Recv Proc
		else if (pOverlapped == &pSession->m_stRecvOverlapped && cbTransferred > 0)
		{
			completionOK = RecvProc(pSession, cbTransferred);
		}
		else if (pOverlapped == &pSession->m_stSendOverlapped && cbTransferred > 0)
		{
			completionOK = SendProc(pSession, cbTransferred);
		}

		// I/O �Ϸ� ������ ���̻� ���ٸ� ���� ���� �۾�
		if (0 == InterlockedDecrement64(&pSession->ioRefCount))
		{
			ReleaseSession(pSession);
		}

	}

	return true;
}


bool LanServer::RecvProc(stSESSION* pSession, long cbTransferred)
{
	pSession->recvRingBuffer.MoveWritePtr(cbTransferred);

	int useSize = pSession->recvRingBuffer.GetUseSize();


	// Recv Message Process
	while (useSize > 0)
	{
		LANHeader header;

		// Header ũ�⸸ŭ �ִ��� Ȯ��
		if (useSize <= sizeof(LANHeader))
			break;

		// Header Peek
		if (pSession->recvRingBuffer.Peek((char*)&header, sizeof(LANHeader)) == 0)
		{
			CRASH();
		}


		// Len Ȯ�� (�����ų� ���� �� �ִ� ��Ŷ ũ�⺸�� Ŭ ��
		if (header.len < 0 || header.len > CPacket::en_PACKET::eBUFFER_DEFAULT)
		{
			DisconnectSession(pSession->sessionID);

			return false;
		}

		// Packet ũ�⸸ŭ �ִ��� Ȯ��
		if (useSize < sizeof(LANHeader) + header.len)
			break;

		// packet alloc
		CPacket* packet = CPacket::Alloc();

		// payload ũ�⸸ŭ ������ Dequeue
		pSession->recvRingBuffer.Dequeue(packet->GetLanBufferPtr(), header.len + CPacket::en_PACKET::LAN_HEADER_SIZE);

		// payload ũ�⸸ŭ packet write pos �̵�
		packet->MoveWritePos(header.len);

		// Total Recv Message Count
		InterlockedIncrement64((LONG64*)&recvMsgCount);

		// Recv Message TPS
		InterlockedIncrement64((LONG64*)&recvMsgTPS);

		// Total Recv Bytes
		InterlockedAdd64((LONG64*)&recvBytes, header.len);

		// Recv Bytes TPS
		InterlockedAdd64((LONG64*)&recvBytesTPS, header.len);

		// ������ �� recv ó��
		OnRecv(pSession->sessionID, packet);

		useSize = pSession->recvRingBuffer.GetUseSize();
	}

	// Recv ����
	RecvPost(pSession);

	return true;
}

bool LanServer::SendProc(stSESSION* pSession, long cbTransferred)
{
	// send �����ۿ� ���ٴ� �� recv �����ۿ��� ����� �޾ƿ��� ���߰ų� recv�� ���� ��
	if (pSession->sendPacketCount == 0)
		return false;

	int totalSendBytes = 0;
	int iSendCount;

	// send �Ϸ� ������ ��Ŷ ����
	for (iSendCount = 0; iSendCount < pSession->sendPacketCount; iSendCount++)
	{
		totalSendBytes += pSession->SendPackets[iSendCount]->GetDataSize();
		CPacket::Free(pSession->SendPackets[iSendCount]);
	}
	// Total Send Bytes
	InterlockedAdd64((long long*)&sendBytes, totalSendBytes);

	// Send Bytes TPS
	InterlockedAdd64((long long*)&sendBytesTPS, totalSendBytes);

	// Total Send Message Count
	InterlockedAdd64((long long*)&sendMsgCount, pSession->sendPacketCount);

	// Send Message TPS
	InterlockedAdd64((long long*)&sendMsgTPS, pSession->sendPacketCount);

	pSession->sendPacketCount = 0;

	// ���� �� flag�� �ٽ� ������ ���·� �ǵ�����
	InterlockedExchange8((char*)&pSession->sendFlag, false);

	// 1ȸ send ��, sendQ�� �׿��ִ� ������ ������ ��� send
	if (pSession->sendQ.GetSize() > 0)
	{
		// sendFlag�� false�ΰ� �ѹ� Ȯ���� ������ ���Ͷ� �� (������� �� ���̿� true�� ��찡 �ɷ����� ���Ͷ� call ����)
		if (pSession->sendFlag == false)
		{
			if (false == InterlockedExchange8((char*)&pSession->sendFlag, true))
			{
				InterlockedIncrement64(&pSession->ioRefCount);
				PostQueuedCompletionStatus(IOCPHandle, 0, (ULONG_PTR)pSession, (LPOVERLAPPED)PQCSTYPE::SENDPOST);
			}
		}
	}

	return true;
}

bool LanServer::RecvPost(stSESSION* pSession)
{
	// recv �ɱ� ���� �ܺο��� disconnect ȣ��� �� ����
	// -> recv �� �ɷ��� �� io ����ص� �ǹ� �����ϱ� ������ recvpost ���ƹ���
	if (pSession->isDisconnected)
		return false;

	// ������ ���
	WSABUF wsa[2] = { 0 };
	int wsaCnt = 1;
	DWORD flags = 0;

	int freeSize = pSession->recvRingBuffer.GetFreeSize();
	int directEequeueSize = pSession->recvRingBuffer.DirectEnqueueSize();

	if (freeSize == 0)
		return false;

	wsa[0].buf = pSession->recvRingBuffer.GetWriteBufferPtr();
	wsa[0].len = directEequeueSize;

	// ������ ���ο��� �� ������ �� �������� ���� ���
	if (freeSize > directEequeueSize)
	{
		wsa[1].buf = pSession->recvRingBuffer.GetBufferPtr();
		wsa[1].len = freeSize - directEequeueSize;

		++wsaCnt;
 	}

	// recv overlapped I/O ����ü reset
	ZeroMemory(&pSession->m_stRecvOverlapped, sizeof(OVERLAPPED));

	// recv
	// ioCount : WSARecv �Ϸ� ������ ���Ϻ��� ���� ������ �� �����Ƿ� WSARecv ȣ�� ���� �������Ѿ� ��
	InterlockedIncrement64(&pSession->ioRefCount);
	int recvRet = WSARecv(pSession->m_socketClient, wsa, wsaCnt, NULL, &flags, &pSession->m_stRecvOverlapped, NULL);
	InterlockedIncrement64(&recvCallCount);
	InterlockedIncrement64(&recvCallTPS);

	// ����ó��
	if (recvRet == SOCKET_ERROR)
	{
		int recvError = WSAGetLastError();

		if (recvError != WSA_IO_PENDING)
		{
			if (recvError != ERROR_10054 && recvError != ERROR_10058 && recvError != ERROR_10060)
			{
				// ����				
				OnError(recvError, L"RecvPost # WSARecv Error\n");
			}

			// Pending�� �ƴ� ���, �Ϸ� ���� ����
			if (0 == InterlockedDecrement64(&pSession->ioRefCount))
			{
				ReleaseSession(pSession);
			}
			return false;
		}
		// Pending�� ���
		else
		{
			InterlockedIncrement64(&recvPendingTPS);

			// Pending �ɷȴµ�, �� ������ disconnect�Ǹ� �� �� �����ִ� �񵿱� io �����������
			if (pSession->isDisconnected)
			{
				CancelIoEx((HANDLE)pSession->m_socketClient, &pSession->m_stRecvOverlapped);
			}

		}
	}

	return true;
}

bool LanServer::SendPost(stSESSION* pSession)
{
	// 1ȸ �۽� ������ ���� flag Ȯ�� (send call Ƚ�� ���̷��� -> send call ��ü�� ����
	// true�� ���� ��� �ƴ�
	// false -> true �� ���� ���
	// true -> true �� ���� ����� �ƴ�
	if ((pSession->sendFlag == true) || true == InterlockedExchange8((char*)&pSession->sendFlag, true))
		return false;

	// SendQ�� ������� �� ����
	// -> �ٸ� �����忡�� Dequeue �������� ���
	if (pSession->sendQ.GetSize() <= 0)
	{
		// * �Ͼ �� �ִ� ��Ȳ
		// �ٸ� �����忡�� dequeue�� ���� �ؼ� size�� 0�� �ż� �� ���ǹ��� �����Ѱǵ�
		// �� ��ġ���� �Ǵٸ� �����忡�� ��Ŷ�� enqueue�ǰ� sendpost�� �Ͼ�� �Ǹ�
		// ���� sendFlag�� false�� ������� ���� �����̱� ������ sendpost �Լ� ��� ���ǿ� �ɷ� ���������� ��
		// �� ��, �� ������� �ٽ� ���ƿ��� �� ���, 
		// sendQ�� ��Ŷ�� �ִ� �����̹Ƿ� sendFlas�� false�� �ٲ��ֱ⸸ �ϰ� �����ϴ°� �ƴ϶�
		// �ѹ� �� sendQ�� size Ȯ�� �� sendpost PQCS ���� �� ����

		InterlockedExchange8((char*)&pSession->sendFlag, false);

		// �� ���̿� SendQ�� Enqueue �ƴٸ� �ٽ� SendPost Call 
		if (pSession->sendQ.GetSize() > 0)
		{
			// sendpost �Լ� ������ send call�� 1ȸ ������
			// sendpost �Լ��� ȣ���ϱ� ���� PQCS�� 1ȸ ������ �־� ���� ������
			// -> �׷��� ���� ���, sendpacket ���´�� ��� PQCS ȣ���ϰ� �Ǿ� ������ �����Ѵ�� �ȳ��� �� ���� 
			if (pSession->sendFlag == false)
			{
				if (false == InterlockedExchange8((char*)&pSession->sendFlag, true))
				{
					InterlockedIncrement64(&pSession->ioRefCount);
					PostQueuedCompletionStatus(IOCPHandle, 0, (ULONG_PTR)pSession, (LPOVERLAPPED)PQCSTYPE::SENDPOST);
				}
			}
		}
		return false;
	}

	int deqIdx = 0;

	// ������ ���
	WSABUF wsa[MAX_WSA_BUF] = { 0 };

	while (pSession->sendQ.Dequeue(pSession->SendPackets[deqIdx]))
	{
		// ��Ŷ ���� ������ (����� ������)
		wsa[deqIdx].buf = pSession->SendPackets[deqIdx]->GetLanBufferPtr();

		// ��Ŷ ũ�� (��� ����)
		wsa[deqIdx].len = pSession->SendPackets[deqIdx]->GetLanDataSize();

		deqIdx++;

		if (deqIdx >= MAX_WSA_BUF)
			break;
	}

	pSession->sendPacketCount = deqIdx;

	// send overlapped I/O ����ü reset
	ZeroMemory(&pSession->m_stSendOverlapped, sizeof(OVERLAPPED));

	// send
	// ioCount : WSASend �Ϸ� ������ ���Ϻ��� ���� ������ �� �����Ƿ� WSASend ȣ�� ���� �������Ѿ� ��
	InterlockedIncrement64(&pSession->ioRefCount);
	int sendRet = WSASend(pSession->m_socketClient, wsa, deqIdx, NULL, 0, &pSession->m_stSendOverlapped, NULL);
	InterlockedIncrement64(&sendCallCount);
	InterlockedIncrement64(&sendCallTPS);

	// ����ó��
	if (sendRet == SOCKET_ERROR)
	{
		int sendError = WSAGetLastError();

		// default error�� ����
		if (sendError != WSA_IO_PENDING)
		{
			if (sendError != ERROR_10054 && sendError != ERROR_10058 && sendError != ERROR_10060)
			{
				OnError(sendError, L"SendPost # WSASend Error\n");
			}

			// Pending�� �ƴ� ���, �Ϸ� ���� ���� -> IOCount�� ����
			if (0 == InterlockedDecrement64(&pSession->ioRefCount))
			{
				ReleaseSession(pSession);
			}

			return false;
		}
		else
			InterlockedIncrement64(&sendPendingTPS);
	}

	return true;
}

bool LanServer::SendPacket(uint64_t sessionID, CPacket* packet)
{
	// find session�� ���� io ����ī��Ʈ �������Ѽ� ����޾ƾ���	
		// index ã�� -> out of indexing ���� ó��
	int index = GetSessionIndex(sessionID);
	if (index < 0 || index >= s_maxAcceptCount)
	{
		return false;
	}

	stSESSION* pSession = &SessionArray[index];

	if (pSession == nullptr)
	{
		return false;
	}

	// ���� ��� ����ī��Ʈ ���� & Release ������ ���� Ȯ��
	// Release ��Ʈ���� 1�̸� ReleaseSession �Լ����� ioCount = 0, releaseFlag = 1 �� ����
	// ��ó�� �ٽ� release���� ������ �����̹Ƿ� ioRefCount ���ҽ�Ű�� �ʾƵ� ��
	if (InterlockedIncrement64(&pSession->ioRefCount) & RELEASEMASKING)
	{
		return false;
	}

	// ------------------------------------------------------------------------------------
	// Release ���� ���� �̰������� ���� ����Ϸ��� ����

	// �� ������ �´��� �ٽ� Ȯ�� (�ٸ� ������ ���� ���� & �Ҵ�Ǿ�, �ٸ� ������ ���� ���� ����)
	// �� ������ �ƴ� ���, ������ �����ߴ� ioCount�� �ǵ����� �� (�ǵ����� ������ ���Ҵ� ������ io�� 0�� �� ���� ����)
	if (sessionID != pSession->sessionID)
	{
		if (0 == InterlockedDecrement64(&pSession->ioRefCount))
		{
			ReleasePQCS(pSession);
		}

		return false;
	}

	// �ܺο��� disconnect �ϴ� ����
	if (pSession->isDisconnected)
	{
		if (0 == InterlockedDecrement64(&pSession->ioRefCount))
		{
			ReleasePQCS(pSession);
		}

		return false;
	}

	// ��� ����
	packet->SetLanHeader();

	// Enqueue�� ��Ŷ�� �ٸ� ������ ����ϹǷ� ��Ŷ ����ī��Ʈ ���� -> Dequeue�� �� ����
	packet->addRefCnt();

	// packet ������ enqueue
	pSession->sendQ.Enqueue(packet);

	// sendpost �Լ� ������ send call�� 1ȸ ������
	// sendpost �Լ��� ȣ���ϱ� ���� PQCS�� 1ȸ ������ �־� ���� ������
	// -> �׷��� ���� ���, sendpacket ���´�� ��� PQCS ȣ���ϰ� �Ǿ� ������ �����Ѵ�� �ȳ��� �� ���� 
	if (pSession->sendFlag == false)
	{
		if (false == InterlockedExchange8((char*)&pSession->sendFlag, true))
		{
			InterlockedIncrement64(&pSession->ioRefCount);
			PostQueuedCompletionStatus(IOCPHandle, 0, (ULONG_PTR)pSession, (LPOVERLAPPED)PQCSTYPE::SENDPOST);
		}
	}

	// sendPacket �Լ����� ������Ų ���� ���� ī��Ʈ ����
	if (0 == InterlockedDecrement64(&pSession->ioRefCount))
	{
		ReleasePQCS(pSession);

		return false;
	}
}

void LanServer::ReleaseSession(stSESSION* pSession)
{
	// ���� �۽� �� ��ġ�� �����ؾ��� 
	// ���� �޸� ����
	// ���� ����
	// ���� ����
	// ioCount == 0 && releaseFlag == 0 => release = 1 (���Ͷ� �Լ��� �ذ�)
	// �ٸ� ������ �ش� ������ ���(sendpacket or disconnect)�ϴ��� Ȯ��
	if (InterlockedCompareExchange64(&pSession->ioRefCount, RELEASEMASKING, 0) != 0)
	{
		return;
	}

	//-----------------------------------------------------------------------------------
	// Release ���� ���Ժ�
	//-----------------------------------------------------------------------------------
	//ioCount = 0, releaseFlag = 1 �� ����

	uint64_t sessionID = pSession->sessionID;

	pSession->sessionID = -1;

	SOCKET sock = pSession->m_socketClient;

	// ���� Invalid ó���Ͽ� ���̻� �ش� �������� I/O ���ް� ��
	pSession->m_socketClient = INVALID_SOCKET;

	// recv�� ���̻� ������ �ȵǹǷ� ���� close
	closesocket(sock);

	InterlockedExchange8((char*)&pSession->sendFlag, false);

	// Send Packet ���� ���ҽ� ����
	// SendQ���� Dqeueue�Ͽ� SendPacket �迭�� �־����� ���� WSASend ���ؼ� �����ִ� ��Ŷ ����
	for (int iSendCount = 0; iSendCount < pSession->sendPacketCount; iSendCount++)
	{
		CPacket::Free(pSession->SendPackets[iSendCount]);
	}

	pSession->sendPacketCount = 0;

	// SendQ�� �����ִٴ� �� WSABUF�� �ȾƳ����� ���� ���� 
	if (pSession->sendQ.GetSize() > 0)
	{
		CPacket* packet = nullptr;
		while (pSession->sendQ.Dequeue(packet))
		{
			CPacket::Free(packet);
		}
	}

	// �̻�� index�� stack�� push
	AvailableIndexStack.Push(GetSessionIndex(sessionID));

	// �����(Player) ���� ���ҽ� ���� (ȣ�� �Ŀ� �ش� ������ ���Ǹ� �ȵ�)
	OnClientLeave(sessionID);

	// �������� client �� ����
	InterlockedDecrement64(&sessionCnt);

	// ���� ������ clinet �� ����
	InterlockedIncrement64(&releaseCount);
	InterlockedIncrement64(&releaseTPS);
}

bool LanServer::DisconnectSession(uint64_t sessionID)
{

	// ���� �˻�
	// index ã��
	int index = GetSessionIndex(sessionID);
	if (index < 0 || index >= s_maxAcceptCount)
	{
		return false;
	}

	stSESSION* pSession = &SessionArray[index];

	if (pSession == nullptr)
	{
		return false;
	}

	// ���� ��� ����ī��Ʈ ���� & Release ������ ���� Ȯ��
	// Release ��Ʈ���� 1�̸� ReleaseSession �Լ����� ioCount = 0, releaseFlag = 1 �� ����
	// ��ó�� �ٽ� release���� ������ �����̹Ƿ� ioRefCount ���ҽ�Ű�� �ʾƵ� ��
	if (InterlockedIncrement64(&pSession->ioRefCount) & RELEASEMASKING)
	{
		return false;
	}

	// Release ���� ���� �̰������� ���� ����Ϸ��� ����

	// �� ������ �´��� �ٽ� Ȯ�� (�ٸ� ������ ���� ����&�Ҵ�Ǿ�, �ٸ� ������ ���� ���� ����)
	// -> �̹� �ܺο��� disconnect �� ���� accept�Ǿ� ���� session�� �Ҵ�Ǿ��� ���
	// ������ �����ߴ� ioCount�� �ǵ����� �� (0�̸� Release ����)
	if (sessionID != pSession->sessionID)
	{
		if (0 == InterlockedDecrement64(&pSession->ioRefCount))
		{
			ReleasePQCS(pSession);
		}

		return false;
	}

	// �ܺο��� disconnect �ϴ� ����
	if (pSession->isDisconnected)
	{
		if (0 == InterlockedDecrement64(&pSession->ioRefCount))
		{
			ReleasePQCS(pSession);
		}

		return false;
	}

	// ------------------------ Disconnect Ȯ�� ------------------------
	// �׳� closesocket�� �ϰ� �Ǹ� closesocket �Լ��� CancelIoEx �Լ� ���̿��� ������ ������ 
	// ���Ҵ�Ǿ� �ٸ� ������ �� �� ����
	// �׶� ���Ҵ�� ������ IO �۾����� CancelIoEx�� ���� ���ŵǴ� ���� �߻�
	// disconnected flag�� true�� �����ϸ� sendPacket �� recvPost �Լ� ������ ����
	InterlockedExchange8((char*)&pSession->isDisconnected, true);

	// ���� IO �۾� ��� ���
	CancelIoEx((HANDLE)pSession->m_socketClient, NULL);

	// Disconnect �Լ����� ������Ų ���� ���� ī��Ʈ ����
	if (0 == InterlockedDecrement64(&pSession->ioRefCount))
	{
		ReleasePQCS(pSession);

		return false;
	}

	return true;
}

void LanServer::Stop()
{
	// stop �Լ� ���� ���� �Ϸ�

	// worker thread�� ���� PQCS ����
	for (int i = 0; i < s_workerThreadCount; i++)
	{
		PostQueuedCompletionStatus(IOCPHandle, 0, 0, 0);
	}


	WaitForMultipleObjects(s_workerThreadCount, &mWorkerThreads[0], TRUE, INFINITE);
	closesocket(ListenSocket);
	WaitForSingleObject(mAcceptThread, INFINITE);

	CloseHandle(IOCPHandle);
	CloseHandle(mAcceptThread);

	for (int i = 0; i < s_workerThreadCount; i++)
		CloseHandle(mWorkerThreads[i]);

	if (SessionArray != nullptr)
		delete[] SessionArray;

	WSACleanup();
}
