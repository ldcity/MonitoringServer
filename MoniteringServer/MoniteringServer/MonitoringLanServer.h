#ifndef __MONITORING_LANSERVER__
#define __MONITORING_LANSERVER__

#include <unordered_map>

#include "LanServer.h"
#include "MonitoringWanServer.h"
#include "DBConnection_TLS.h"
#include "PerformanceMonitor.h"
#include "MonitorProtocol.h"
#include "MonitoringDefine.h"


class MonitoringLanServer : public LanServer
{
public:
	MonitoringLanServer();
	~MonitoringLanServer();

	bool MonitoringLanServerStart();
	bool MonitoringLanServerStop();

	bool MonitorThread_serv();
	bool DBWriteThread_serv();

	// ServerNo + DataType ���� Ű ����
	inline uint64_t GetKey(BYTE serverNo, BYTE dataType)
	{
		return (uint64_t)(dataType | ((uint64_t)serverNo << SESSION_ID_BITS));
	}

	//--------------------------------------------------------------------------------------
	// Packet Proc
	//--------------------------------------------------------------------------------------
	void PacketProc_Login(uint64_t sessionID, CPacket* packet);			// �Ϲ� ������ ����͸� ������ �α��� ��û
	void PacketProc_Update(uint64_t sessionID, CPacket* packet);		// ����͸� ������ ������Ʈ

	//--------------------------------------------------------------------------------------
	// Contents Logic
	//--------------------------------------------------------------------------------------
	bool OnConnectionRequest(const wchar_t* IP, unsigned short PORT);
	void OnClientJoin(uint64_t sessionID, wchar_t* ip, unsigned short port);
	void OnClientLeave(uint64_t sessionID);
	void OnRecv(uint64_t sessionID, CPacket* packet);
	void OnError(int errorCode, const wchar_t* msg);

private:
	friend unsigned __stdcall MonitorThread(LPVOID param);
	friend unsigned __stdcall DBWriteThread(LPVOID param);
	friend class MonitoringWanServer;

private:
	Log* monitorLog;

	MonitoringWanServer* monitorNetServ;	// �ܺ� ��ſ� ����͸� ���� ������
	DBConnector* dbConn;					// db ������ ���� Connector ������

	HANDLE monitorThreadHandle;
	HANDLE dbThreadHandle;

	HANDLE monitorTickEvent;
	HANDLE dbTickEvent;

	int mTimeout;

private:

	PerformanceMonitor perfomanceMonitor;
	wstring m_tempIp;
	string m_ip;

	enum MONITORTYPE
	{
		df_MONITOR_MIN,
		df_MONITOR_MAX,
		df_MONITOR_AVG
	};
	
	// Server �� Monitoring Type
	// �� : ���� ��ȣ / �� : dataType ��ȣ
	BYTE monitoringInfoArr[SERVERTYPE::TOTAL][MONITORARRCNF] =
	{
			{
				dfMONITOR_DATA_TYPE_LOGIN_SERVER_RUN,
				dfMONITOR_DATA_TYPE_LOGIN_SERVER_CPU,
				dfMONITOR_DATA_TYPE_LOGIN_SERVER_MEM,
				dfMONITOR_DATA_TYPE_LOGIN_SESSION,
				dfMONITOR_DATA_TYPE_LOGIN_AUTH_TPS,
				dfMONITOR_DATA_TYPE_LOGIN_PACKET_POOL,
			},
			{
				dfMONITOR_DATA_TYPE_GAME_SERVER_RUN,
				dfMONITOR_DATA_TYPE_GAME_SERVER_CPU,
				dfMONITOR_DATA_TYPE_GAME_SERVER_MEM,
				dfMONITOR_DATA_TYPE_GAME_SESSION,
				dfMONITOR_DATA_TYPE_GAME_AUTH_PLAYER,
				dfMONITOR_DATA_TYPE_GAME_GAME_PLAYER,
				dfMONITOR_DATA_TYPE_GAME_ACCEPT_TPS,
				dfMONITOR_DATA_TYPE_GAME_PACKET_RECV_TPS,
				dfMONITOR_DATA_TYPE_GAME_PACKET_SEND_TPS,
				dfMONITOR_DATA_TYPE_GAME_DB_WRITE_TPS,
				dfMONITOR_DATA_TYPE_GAME_DB_WRITE_MSG,
				dfMONITOR_DATA_TYPE_GAME_AUTH_THREAD_FPS,
				dfMONITOR_DATA_TYPE_GAME_GAME_THREAD_FPS,
				dfMONITOR_DATA_TYPE_GAME_PACKET_POOL
			},
			{
				dfMONITOR_DATA_TYPE_CHAT_SERVER_RUN,
				dfMONITOR_DATA_TYPE_CHAT_SERVER_CPU,
				dfMONITOR_DATA_TYPE_CHAT_SERVER_MEM,
				dfMONITOR_DATA_TYPE_CHAT_SESSION,
				dfMONITOR_DATA_TYPE_CHAT_PLAYER,
				dfMONITOR_DATA_TYPE_CHAT_UPDATE_TPS,
				dfMONITOR_DATA_TYPE_CHAT_PACKET_POOL,
				dfMONITOR_DATA_TYPE_CHAT_UPDATEMSG_POOL
			}
	};

	SRWLOCK dbDataMapLock;
	SRWLOCK clientMapLock;

	// �� serverNo + dataType�� dbData ����뵵 
	// key : serverNo�� dataType ����
	unordered_map<uint64_t, MonitoringInfo*> dbDataMap;

	bool isTableTrue;

	// Client ������ ����ִ� map
	// key : sessionID
	unordered_map<uint64_t, ClientInfo*> clientMap;

	// ClientInfo ��ü 1000�� ���� (Ȯ�强 ����ص� ���� �ʿ�� ���� ��)
	TLSObjectPool<ClientInfo> clientPool = TLSObjectPool<ClientInfo>(10);

private:
	wchar_t mDBName[64];
	WCHAR startTime[64];
};



#endif