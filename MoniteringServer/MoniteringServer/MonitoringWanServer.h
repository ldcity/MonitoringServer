#pragma once
#ifndef __MONITORING_NetServer__
#define __MONITORING_NetServer__

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "winmm.lib")

#include "NetServer.h"

#include <unordered_set>

class MonitoringWanServer : public NetServer
{
public:
	MonitoringWanServer();
	~MonitoringWanServer();

	bool MonitoringWanServerStart(const wchar_t* IP, unsigned short PORT, int createWorkerThreadCnt, int runningWorkerThreadCnt, bool nagelOff, int maxAcceptCnt, unsigned char packet_code, unsigned char packet_key);
	bool MonitoringWanServerStop();
	bool SendMonitorClients(BYTE serverNo, BYTE dataType, int dataValue, int timeStamp);
	bool SendMonitorClients(CPacket** packet, int packetCnt);

	//--------------------------------------------------------------------------------------
	// Packet Proc
	//--------------------------------------------------------------------------------------
	void PacketProc_Login(uint64_t sessionID, CPacket* packet);			// 로그인 요청

	//--------------------------------------------------------------------------------------
	// Make Packet
	//--------------------------------------------------------------------------------------
	void mpDataUpdateToMonitorClient(BYTE serverNo, BYTE dataType, int dataValue, int timeStamp, CPacket* packet);
	void mpResToMonitorClient(BYTE status, CPacket* packet);

	bool OnConnectionRequest(const wchar_t* IP, unsigned short PORT);
	void OnClientJoin(uint64_t sessionID);
	void OnClientLeave(uint64_t sessionID);
	void OnRecv(uint64_t sessionID, CPacket* packet);
	void OnError(int errorCode, const wchar_t* msg);

public:
	inline wchar_t* GetLoginSessionKey()
	{
		return mLoginSessionKey;
	}

private:
	HANDLE dbThread;

	int mTimeout;

	wchar_t mLoginSessionKey[32];

	SRWLOCK sessionMapLock;

	// 외부 세션들 map
	std::unordered_set<uint64_t> sessionMap;
};


#endif