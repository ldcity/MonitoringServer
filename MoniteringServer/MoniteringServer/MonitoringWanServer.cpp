#include "MonitoringWanServer.h"
#include "MonitorProtocol.h"


MonitoringWanServer::MonitoringWanServer() : dbThread(INVALID_HANDLE_VALUE), mTimeout(0)
{
	wprintf(L"Monitoring Server Init...\n");

	InitializeSRWLock(&sessionMapLock);
}

MonitoringWanServer::~MonitoringWanServer()
{
	MonitoringWanServerStop();
}

bool MonitoringWanServer::MonitoringWanServerStart(const wchar_t* IP, unsigned short PORT, int createWorkerThreadCnt, int runningWorkerThreadCnt, bool nagelOff, int maxAcceptCnt, unsigned char packet_code, unsigned char packet_key)
{
	if (!Start(IP, PORT, createWorkerThreadCnt, runningWorkerThreadCnt, nagelOff, maxAcceptCnt, packet_code, packet_key))
	{
		MonitoringWanServerStop();
		return false;
	}
}

bool MonitoringWanServer::MonitoringWanServerStop()
{
	Stop();

	return true;
}

bool MonitoringWanServer::SendMonitorClients(BYTE serverNo, BYTE dataType, int dataValue, int timeStamp)
{
	CPacket* packet = CPacket::Alloc();
	mpDataUpdateToMonitorClient(serverNo, dataType, dataValue, timeStamp, packet);

	AcquireSRWLockExclusive(&sessionMapLock);

	// 외부 클라이언트들에게 Packet 전송 (broadcast)
	auto iter = sessionMap.begin();
	for (; iter != sessionMap.end(); ++iter)
	{
		SendPacket(*iter, packet);
	}

	ReleaseSRWLockExclusive(&sessionMapLock);

	CPacket::Free(packet);

	return true;
}

// 외부 Monitoring Client로 모니터링 정보 전송
bool MonitoringWanServer::SendMonitorClients(CPacket** packet, int packetCnt)
{
	AcquireSRWLockExclusive(&sessionMapLock);

	// 외부 클라이언트들에게 Packet 전송 (broadcast)
	auto iter = sessionMap.begin();
	for (; iter != sessionMap.end(); ++iter)
	{
		for (int i = 0; i < packetCnt; i++)
		{
			SendPacket(*iter, packet[i]);
		}
	}

	ReleaseSRWLockExclusive(&sessionMapLock);

	return true;
}


// 외부 로그인 요청
void MonitoringWanServer::PacketProc_Login(uint64_t sessionID, CPacket* packet)
{
	char netLoginSessionKey[256];
	char loginSessionKey[256];
	BYTE resLoginStatusType;

	int size = packet->GetDataSize();

	packet->GetData(loginSessionKey, size);

	loginSessionKey[size] = '\0';

	// wchar_t -> char
	WideCharToMultiByte(CP_UTF8, 0, mLoginSessionKey, -1, netLoginSessionKey, sizeof(netLoginSessionKey), NULL, NULL);

	// 외부 클라이언트의 로그인 세션 키와 서버 로그인 세션 키가 일치
	if (strcmp(loginSessionKey, netLoginSessionKey) == 0)
	{
		resLoginStatusType = dfMONITOR_TOOL_LOGIN_OK;

		AcquireSRWLockExclusive(&sessionMapLock);

		// 해당 클라이언트를 세션 관리 맵에 추가
		sessionMap.insert(sessionID);

		ReleaseSRWLockExclusive(&sessionMapLock);
	}
	else
		resLoginStatusType = dfMONITOR_TOOL_LOGIN_ERR_SESSIONKEY;

	CPacket::Free(packet);

	// 클라이언트 응답용 패킷
	CPacket* resPacket = CPacket::Alloc();
	mpResToMonitorClient(resLoginStatusType, resPacket);

	SendPacket(sessionID, resPacket);

	CPacket::Free(resPacket);

	// 로그인 세션키가 다르다면 disconnect
	if (resLoginStatusType == dfMONITOR_TOOL_LOGIN_ERR_SESSIONKEY)
		DisconnectSession(sessionID);
}

void MonitoringWanServer::mpDataUpdateToMonitorClient(BYTE serverNo, BYTE dataType, int dataValue, int timeStamp, CPacket* packet)
{
	WORD type = en_PACKET_CS_MONITOR_TOOL_DATA_UPDATE;
	*packet << type << serverNo << dataType << dataValue << timeStamp;
}

void MonitoringWanServer::mpResToMonitorClient(BYTE status, CPacket* packet)
{
	WORD type = en_PACKET_CS_MONITOR_TOOL_RES_LOGIN;
	*packet << type << status;
}


bool MonitoringWanServer::OnConnectionRequest(const wchar_t* IP, unsigned short PORT)
{
	return true;
}

void MonitoringWanServer::OnClientJoin(uint64_t sessionID)
{

}

void MonitoringWanServer::OnClientLeave(uint64_t sessionID)
{

}

void MonitoringWanServer::OnRecv(uint64_t sessionID, CPacket* packet)
{
	WORD type;
	*packet >> type;

	switch (type)
	{
	case en_PACKET_CS_MONITOR_TOOL_REQ_LOGIN:
		PacketProc_Login(sessionID, packet);
		break;

	default:
		break;
	}
}

void MonitoringWanServer::OnError(int errorCode, const wchar_t* msg)
{

}
