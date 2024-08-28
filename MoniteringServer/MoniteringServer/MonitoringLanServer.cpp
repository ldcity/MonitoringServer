#include <process.h>

#include "MonitoringLanServer.h"
#include "TextParser.h"
#include "MonitorProtocol.h"

unsigned __stdcall MonitorThread(LPVOID param)
{
	MonitoringLanServer* lanServ = (MonitoringLanServer*)param;

	lanServ->MonitorThread_serv();

	return 0;
}

unsigned __stdcall DBWriteThread(LPVOID param)
{
	MonitoringLanServer* lanServ = (MonitoringLanServer*)param;

	lanServ->DBWriteThread_serv();

	return 0;
}

MonitoringLanServer::MonitoringLanServer() : monitorLog(nullptr), monitorNetServ(nullptr), dbConn(nullptr), monitorThreadHandle(INVALID_HANDLE_VALUE), mTimeout(0)
{
	wprintf(L"Monitoring Lan Server Init...\n");

	InitializeSRWLock(&dbDataMapLock);
	InitializeSRWLock(&clientMapLock);
}

MonitoringLanServer::~MonitoringLanServer()
{
	MonitoringLanServerStop();
}

bool MonitoringLanServer::MonitoringLanServerStart()
{
	SYSTEMTIME stNowTime;
	GetLocalTime(&stNowTime);
	wsprintfW(startTime, L"%d%02d%02d_%02d.%02d.%02d",
		stNowTime.wYear, stNowTime.wMonth, stNowTime.wDay, stNowTime.wHour, stNowTime.wMinute, stNowTime.wSecond);

	monitorLog = new Log(L"MonitorLog.txt");

	TextParser monitorCnfFile;
	const wchar_t* cnfName = L"MonitoringCnf.txt";
	monitorCnfFile.LoadFile(cnfName);

	// DB ���� ���
	wchar_t host[16];
	wchar_t user[64];
	wchar_t password[64];
	wchar_t dbName[64];
	int port;

	monitorCnfFile.GetValue(L"DB.HOST", host);
	monitorCnfFile.GetValue(L"DB.USER", user);
	monitorCnfFile.GetValue(L"DB.PASSWORD", password);
	monitorCnfFile.GetValue(L"DB.DBNAME", dbName);
	monitorCnfFile.GetValue(L"DB.PORT", &port);

	wmemcpy_s(mDBName, 64, dbName, 64);

	//dbConn_TLS = new DBConnector_TLS(host, user, password, dbName, port, true);
	dbConn = new DBConnector(host, user, password, dbName, port, true);
	

	monitorThreadHandle = (HANDLE)_beginthreadex(NULL, 0, MonitorThread, this, 0, NULL);
	if (monitorThreadHandle == NULL)
	{
		int threadError = GetLastError();

		return false;
	}

	dbThreadHandle = (HANDLE)_beginthreadex(NULL, 0, DBWriteThread, this, 0, NULL);
	if (dbThreadHandle == NULL)
	{
		int threadError = GetLastError();

		return false;
	}

	// Create Auto Event
	monitorTickEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (monitorTickEvent == NULL)
	{
		int eventError = WSAGetLastError();

		return false;
	}

	// Create Auto Event
	dbTickEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (dbTickEvent == NULL)
	{
		int eventError = WSAGetLastError();

		return false;
	}

	// Lan Server ���� ���
	wchar_t lanIP[16];
	monitorCnfFile.GetValue(L"LANSERVER.BIND_IP", lanIP);

	m_tempIp = lanIP;
	int len = WideCharToMultiByte(CP_UTF8, 0, m_tempIp.c_str(), -1, NULL, 0, NULL, NULL);
	string result(len - 1, '\0');
	WideCharToMultiByte(CP_UTF8, 0, m_tempIp.c_str(), -1, &result[0], len, NULL, NULL);
	m_ip = result;

	perfomanceMonitor.AddInterface(m_ip);

	int lanPort;
	monitorCnfFile.GetValue(L"LANSERVER.BIND_PORT", &lanPort);

	int lanWorkerThread;
	monitorCnfFile.GetValue(L"LANSERVER.IOCP_WORKER_THREAD", &lanWorkerThread);

	int lanRunningThread;
	monitorCnfFile.GetValue(L"LANSERVER.IOCP_ACTIVE_THREAD", &lanRunningThread);

	int lanNagleOff;
	monitorCnfFile.GetValue(L"LANSERVER.NAGLE_OFF", &lanNagleOff);

	int lanSessionMAXCnt;
	monitorCnfFile.GetValue(L"LANSERVER.SESSION_MAX", &lanSessionMAXCnt);

	int timeout;
	monitorCnfFile.GetValue(L"LANSERVER.TIMEOUT", &timeout);

	// Net Server ���� ���
	wchar_t netIP[16];
	monitorCnfFile.GetValue(L"NetServer.BIND_IP", netIP);

	int netPort;
	monitorCnfFile.GetValue(L"NetServer.BIND_PORT", &netPort);

	int netWorkerThread;
	monitorCnfFile.GetValue(L"NetServer.IOCP_WORKER_THREAD", &netWorkerThread);

	int netRunningThread;
	monitorCnfFile.GetValue(L"NetServer.IOCP_ACTIVE_THREAD", &netRunningThread);

	int netNagleOff;
	monitorCnfFile.GetValue(L"NetServer.NAGLE_OFF", &netNagleOff);

	int netSessionMAXCnt;
	monitorCnfFile.GetValue(L"NetServer.SESSION_MAX", &netSessionMAXCnt);

	int packet_code;
	monitorCnfFile.GetValue(L"NetServer.PACKET_CODE", &packet_code);

	int packet_key;
	monitorCnfFile.GetValue(L"NetServer.PACKET_KEY", &packet_key);

	// Net Server Start
	monitorNetServ = new MonitoringWanServer();

	if (!monitorNetServ->MonitoringWanServerStart(netIP, netPort, netWorkerThread, netRunningThread, netNagleOff, netSessionMAXCnt, packet_code, packet_key))
	{
		MonitoringLanServerStop();
		return false;
	}

	monitorCnfFile.GetValue(L"NetServer.LOGIN_SESSION_KEY", monitorNetServ->GetLoginSessionKey());


	// Lan Server Start
	if (!Start(lanIP, lanPort, lanWorkerThread, lanRunningThread, lanNagleOff, lanSessionMAXCnt))
	{
		MonitoringLanServerStop();
		return false;
	}

	WaitForSingleObject(monitorThreadHandle, INFINITE);
	WaitForSingleObject(dbThreadHandle, INFINITE);

	return true;
}

bool MonitoringLanServer::MonitoringLanServerStop()
{
	Stop();
	monitorNetServ->MonitoringWanServerStop();

	CloseHandle(monitorTickEvent);
	CloseHandle(dbTickEvent);
	CloseHandle(monitorThreadHandle);
	CloseHandle(dbThreadHandle);

	auto iter = dbDataMap.begin();

	for (; iter != dbDataMap.end(); ++iter)
	{
		delete (*iter).second;
	}

	delete monitorLog;
	//delete dbConn_TLS;
	delete dbConn;
	delete monitorNetServ;

	return true;
}

// 1�ʸ��� ����͸� Ŭ���̾�Ʈ�� ���μ��� �� �ϵ���� ���� ����
bool MonitoringLanServer::MonitorThread_serv()
{
	CPacket* packets[5];

	while (1)
	{
		// 1�ʸ��� ����͸� ������ ����
		DWORD ret = WaitForSingleObject(monitorTickEvent, 1000);

		if (ret == WAIT_TIMEOUT)
		{
			wprintf(L"%s\n", startTime);
			wprintf(L"------------------------[LAN]----------------------------\n");
			wprintf(L"[Session              ] Total    : %10I64d\n", sessionCnt);
			wprintf(L"[Accept               ] Total    : %10I64d   TPS : %10I64d\n", acceptCount, InterlockedExchange64(&acceptTPS, 0));
			wprintf(L"[Release              ] Total    : %10I64d   TPS : %10I64d\n", releaseCount, InterlockedExchange64(&releaseTPS, 0));
			wprintf(L"[Recv Call            ] Total    : %10I64d   TPS : %10I64d\n", recvCallCount, InterlockedExchange64(&recvCallTPS, 0));
			wprintf(L"[Send Call            ] Total    : %10I64d   TPS : %10I64d\n", sendCallCount, InterlockedExchange64(&sendCallTPS, 0));
			wprintf(L"[Recv Bytes           ] Total    : %10I64d   TPS : %10I64d\n", recvBytes, InterlockedExchange64(&recvBytesTPS, 0));
			wprintf(L"[Send Bytes           ] Total    : %10I64d   TPS : %10I64d\n", sendBytes, InterlockedExchange64(&sendBytesTPS, 0));
			wprintf(L"[Recv  Packet         ] Total    : %10I64d   TPS : %10I64d\n", recvMsgCount, InterlockedExchange64(&recvMsgTPS, 0));
			wprintf(L"[Send  Packet         ] Total    : %10I64d   TPS : %10I64d\n", sendMsgCount, InterlockedExchange64(&sendMsgTPS, 0));
			wprintf(L"[Pending TPS          ] Recv     : %10I64d   Send: %10I64d\n", InterlockedExchange64(&recvPendingTPS, 0), InterlockedExchange64(&sendPendingTPS, 0));
			wprintf(L"==============================================================\n\n");
			wprintf(L"------------------------[WAN]----------------------------\n");
			wprintf(L"[Session              ] Total    : %10I64d\n", monitorNetServ->sessionCnt);
			wprintf(L"[Accept               ] Total    : %10I64d   TPS : %10I64d\n", monitorNetServ->acceptCount, InterlockedExchange64(&monitorNetServ->acceptTPS, 0));
			wprintf(L"[Release              ] Total    : %10I64d   TPS : %10I64d\n", monitorNetServ->releaseCount, InterlockedExchange64(&monitorNetServ->releaseTPS, 0));
			wprintf(L"[Recv Call            ] Total    : %10I64d   TPS : %10I64d\n", monitorNetServ->recvCallCount, InterlockedExchange64(&monitorNetServ->recvCallTPS, 0));
			wprintf(L"[Send Call            ] Total    : %10I64d   TPS : %10I64d\n", monitorNetServ->sendCallCount, InterlockedExchange64(&monitorNetServ->sendCallTPS, 0));
			wprintf(L"[Recv Bytes           ] Total    : %10I64d   TPS : %10I64d\n", monitorNetServ->recvBytes, InterlockedExchange64(&monitorNetServ->recvBytesTPS, 0));
			wprintf(L"[Send Bytes           ] Total    : %10I64d   TPS : %10I64d\n", monitorNetServ->sendBytes, InterlockedExchange64(&monitorNetServ->sendBytesTPS, 0));
			wprintf(L"[Recv  Packet         ] Total    : %10I64d   TPS : %10I64d\n", monitorNetServ->recvMsgCount, InterlockedExchange64(&monitorNetServ->recvMsgTPS, 0));
			wprintf(L"[Send  Packet         ] Total    : %10I64d   TPS : %10I64d\n", monitorNetServ->sendMsgCount, InterlockedExchange64(&monitorNetServ->sendMsgTPS, 0));
			wprintf(L"[Pending TPS          ] Recv     : %10I64d   Send: %10I64d\n", InterlockedExchange64(&monitorNetServ->recvPendingTPS, 0), InterlockedExchange64(&monitorNetServ->sendPendingTPS, 0));
			wprintf(L"==============================================================\n\n");

			// Process �� Hardware ����͸� ���� ������Ʈ
			perfomanceMonitor.UpdateMonitorData();

			int timeStamp = (int)time(NULL);

			for (int i = 0; i < 5; i++)
				packets[i] = CPacket::Alloc();

			// ������ǻ�� CPU ��ü ����
			monitorNetServ->mpDataUpdateToMonitorClient(3, dfMONITOR_DATA_TYPE_MONITOR_CPU_TOTAL, (int)perfomanceMonitor.GetSystemCpuTotal(), timeStamp, packets[0]);

			// ������ǻ�� �������� �޸� MByte
			monitorNetServ->mpDataUpdateToMonitorClient(3, dfMONITOR_DATA_TYPE_MONITOR_NONPAGED_MEMORY, (int)perfomanceMonitor.GetSystemNonPagedByMB(), timeStamp, packets[1]);

			// ������ǻ�� ��Ʈ��ũ ���ŷ� KByte
			monitorNetServ->mpDataUpdateToMonitorClient(3, dfMONITOR_DATA_TYPE_MONITOR_NETWORK_RECV, (int)perfomanceMonitor.GetInDataSizeByKB(), timeStamp, packets[2]);
			
			// ������ǻ�� ��Ʈ��ũ �۽ŷ� KByte
			monitorNetServ->mpDataUpdateToMonitorClient(3, dfMONITOR_DATA_TYPE_MONITOR_NETWORK_SEND, (int)perfomanceMonitor.GetOutDataSizeByKB(), timeStamp, packets[3]);
			
			// ������ǻ�� ��밡�� �޸�
			monitorNetServ->mpDataUpdateToMonitorClient(3, dfMONITOR_DATA_TYPE_MONITOR_AVAILABLE_MEMORY, (int)perfomanceMonitor.GetSystemAvailMemoryByMB(), timeStamp, packets[4]);

			monitorNetServ->SendMonitorClients(packets, 5);		// �ܺ� Monitoring Client�� ����͸� ���� ����

			for (int i = 0; i < 5; i++)
				CPacket::Free(packets[i]);

			// DB Map Update
			// ������ǻ�� CPU ����
			BYTE monitorNo = SERVERTYPE::MONITOR_SERVER_TYPE;
			BYTE monitorType = en_PACKET_SS_MONITOR_DATA_UPDATE::dfMONITOR_DATA_TYPE_MONITOR_CPU_TOTAL;
			int dataValue = (int)perfomanceMonitor.GetSystemCpuTotal();

			// ������ Monitoring Type���� �ο��� ���� key
			uint64_t key = GetKey(monitorNo, monitorType);

			AcquireSRWLockExclusive(&dbDataMapLock);

			// ������ �˻�
			auto iter = dbDataMap.find(key);

			MonitoringInfo* monitorInfo = nullptr;

			// Ư�� ������ �����͹�ȣ�� ���� ����͸� ������ �ʿ� ���� ���, �߰�
			if (iter == dbDataMap.end())
			{
				monitorInfo = new MonitoringInfo;
				monitorInfo->iServerNo = monitorNo;
				monitorInfo->type = monitorType;

				dbDataMap.insert({ key, monitorInfo });
			}
			// ������ ���� ���, �ش� ���� �����
			else monitorInfo = (*iter).second;

			// ����͸� ���� ������Ʈ
			monitorInfo->iCount++;					// ��� �� ����� ���� counting
			monitorInfo->iSum += dataValue;			// ��� �� ����� ���� total

			if (monitorInfo->iMin > dataValue)		// ��� �� ��꿡�� �ּڰ� ����
				monitorInfo->iMin = dataValue;

			if (monitorInfo->iMax < dataValue)		// ��� �� ��꿡�� �ִ� ����
				monitorInfo->iMax = dataValue;

			ReleaseSRWLockExclusive(&dbDataMapLock);
		
			// ������ǻ�� �������� �޸� MByte
			monitorType = en_PACKET_SS_MONITOR_DATA_UPDATE::dfMONITOR_DATA_TYPE_MONITOR_NONPAGED_MEMORY;
			dataValue = (int)perfomanceMonitor.GetSystemNonPagedByMB();

			key = GetKey(monitorNo, monitorType);

			AcquireSRWLockExclusive(&dbDataMapLock);

			iter = dbDataMap.find(key);

			monitorInfo = nullptr;

			// Ư�� ������ �����͹�ȣ�� ���� ����͸� ������ �ʿ� ���� ���, �߰�
			if (iter == dbDataMap.end())
			{
				monitorInfo = new MonitoringInfo;
				monitorInfo->iServerNo = monitorNo;
				monitorInfo->type = monitorType;

				dbDataMap.insert({ key, monitorInfo });
			}
			// ������ ���� ���, �ش� ���� �����
			else
			{
				monitorInfo = (*iter).second;
			}

			// ����͸� ���� ������Ʈ
			monitorInfo->iCount++;
			monitorInfo->iSum += dataValue;

			if (monitorInfo->iMin > dataValue)
				monitorInfo->iMin = dataValue;

			if (monitorInfo->iMax < dataValue)
				monitorInfo->iMax = dataValue;

			ReleaseSRWLockExclusive(&dbDataMapLock);

			// ������ǻ�� ��Ʈ��ũ ���ŷ� KByte
			monitorType = en_PACKET_SS_MONITOR_DATA_UPDATE::dfMONITOR_DATA_TYPE_MONITOR_NETWORK_RECV;
			dataValue = (int)perfomanceMonitor.GetInDataSizeByKB();

			key = GetKey(monitorNo, monitorType);

			AcquireSRWLockExclusive(&dbDataMapLock);

			iter = dbDataMap.find(key);

			monitorInfo = nullptr;

			// Ư�� ������ �����͹�ȣ�� ���� ����͸� ������ �ʿ� ���� ���, �߰�
			if (iter == dbDataMap.end())
			{
				monitorInfo = new MonitoringInfo;
				monitorInfo->iServerNo = monitorNo;
				monitorInfo->type = monitorType;

				dbDataMap.insert({ key, monitorInfo });
			}
			// ������ ���� ���, �ش� ���� �����
			else
			{
				monitorInfo = (*iter).second;
			}

			// ����͸� ���� ������Ʈ
			monitorInfo->iCount++;
			monitorInfo->iSum += dataValue;

			if (monitorInfo->iMin > dataValue)
				monitorInfo->iMin = dataValue;

			if (monitorInfo->iMax < dataValue)
				monitorInfo->iMax = dataValue;

			ReleaseSRWLockExclusive(&dbDataMapLock);

			// ������ǻ�� ��Ʈ��ũ �۽ŷ� KByte
			monitorType = en_PACKET_SS_MONITOR_DATA_UPDATE::dfMONITOR_DATA_TYPE_MONITOR_NETWORK_SEND;
			dataValue = (int)perfomanceMonitor.GetOutDataSizeByKB();

			key = GetKey(monitorNo, monitorType);

			AcquireSRWLockExclusive(&dbDataMapLock);

			iter = dbDataMap.find(key);

			monitorInfo = nullptr;

			// Ư�� ������ �����͹�ȣ�� ���� ����͸� ������ �ʿ� ���� ���, �߰�
			if (iter == dbDataMap.end())
			{
				monitorInfo = new MonitoringInfo;
				monitorInfo->iServerNo = monitorNo;
				monitorInfo->type = monitorType;

				dbDataMap.insert({ key, monitorInfo });
			}
			// ������ ���� ���, �ش� ���� �����
			else
			{
				monitorInfo = (*iter).second;
			}

			// ����͸� ���� ������Ʈ
			monitorInfo->iCount++;
			monitorInfo->iSum += dataValue;

			if (monitorInfo->iMin > dataValue)
				monitorInfo->iMin = dataValue;

			if (monitorInfo->iMax < dataValue)
				monitorInfo->iMax = dataValue;

			ReleaseSRWLockExclusive(&dbDataMapLock);

			// ������ǻ�� ��밡�� �޸�
			monitorType = en_PACKET_SS_MONITOR_DATA_UPDATE::dfMONITOR_DATA_TYPE_MONITOR_AVAILABLE_MEMORY;
			dataValue = (int)perfomanceMonitor.GetSystemAvailMemoryByMB();

			key = GetKey(monitorNo, monitorType);

			AcquireSRWLockExclusive(&dbDataMapLock);

			iter = dbDataMap.find(key);

			monitorInfo = nullptr;

			// Ư�� ������ �����͹�ȣ�� ���� ����͸� ������ �ʿ� ���� ���, �߰�
			if (iter == dbDataMap.end())
			{
				monitorInfo = new MonitoringInfo;
				monitorInfo->iServerNo = monitorNo;
				monitorInfo->type = monitorType;

				dbDataMap.insert({ key, monitorInfo });
			}
			// ������ ���� ���, �ش� ���� �����
			else
			{
				monitorInfo = (*iter).second;
			}

			// ����͸� ���� ������Ʈ
			monitorInfo->iCount++;
			monitorInfo->iSum += dataValue;

			if (monitorInfo->iMin > dataValue)
				monitorInfo->iMin = dataValue;

			if (monitorInfo->iMax < dataValue)
				monitorInfo->iMax = dataValue;

			ReleaseSRWLockExclusive(&dbDataMapLock);
		}
	}

	return true;
}

// 1�п� �ѹ��� Monitoring ������ DB�� Write
bool MonitoringLanServer::DBWriteThread_serv()
{
	while (!dbConn->Open())
	{
		Sleep(1000);
	}

	SYSTEMTIME st;
	GetLocalTime(&st);

	// �ش� ��¥�� ���̺� (monitorlog_yymmdd) �̸� ����
	std::wstringstream tableNameStream;
	tableNameStream << mDBName << L".monitorlog_"
		<< std::setw(2) << std::setfill(L'0') << (st.wYear % 100)
		<< std::setw(2) << std::setfill(L'0') << st.wMonth
		<< std::setw(2) << std::setfill(L'0') << st.wDay;

	std::wstring tmpTableName = tableNameStream.str();

	// �ش� ��¥�� ���̺� select ���� ����
	std::wstringstream queryStream;
	queryStream << L"SELECT * FROM `" << tmpTableName << L"`;";
	std::wstring createTableQuery = queryStream.str();

	// ���̺��� �����ϴ��� Ȯ��
	int ret = dbConn->Query(createTableQuery.c_str());

	// ���̺��� �����Ǿ� ���� ���� ���
	if (ret <= 0)
		tmpTableName.clear();

	while (1)
	{
		DWORD ret = WaitForSingleObject(dbTickEvent, 60000);

		if (ret == WAIT_TIMEOUT)
		{
			SYSTEMTIME st;
			GetLocalTime(&st);

			// �ش� ��¥�� ���̺� �̸� ����
			tableNameStream.str(L""); // ��Ʈ�� �ʱ�ȭ
			tableNameStream << mDBName << L".monitorlog_"
				<< std::setw(2) << std::setfill(L'0') << (st.wYear % 100)
				<< std::setw(2) << std::setfill(L'0') << st.wMonth
				<< std::setw(2) << std::setfill(L'0') << st.wDay;
			std::wstring newTableName = tableNameStream.str();


			// table �̸��� �Ϸ簡 ���� �޶����ų� ���� ���̺��� ���ٸ� ���̺� �����ؾ���
			// ��¥�� ���� table�� ���, skip
			if (newTableName != tmpTableName)
			{
				tmpTableName = newTableName;

				// ���̺� ���� ���� ����
				queryStream.str(L""); // ��Ʈ�� �ʱ�ȭ
				queryStream << L"CREATE TABLE IF NOT EXISTS `" << tmpTableName << L"` ("
					<< L"`no` BIGINT NOT NULL AUTO_INCREMENT, "
					<< L"`logtime` DATETIME, "
					<< L"`logcode` INT NOT NULL, "
					<< L"`type` INT NOT NULL, "
					<< L"`data` INT NOT NULL, "
					<< L"PRIMARY KEY(`no`));";

				std::wstring createTableQuery = queryStream.str();

				// ���̺� ���� ���� ����
				dbConn->Query(createTableQuery.c_str());
			}

			AcquireSRWLockExclusive(&dbDataMapLock);

			auto iter = dbDataMap.begin();

			for (; iter != dbDataMap.end(); ++iter)
			{
				// �ش� ��¥�� ���̺� ������ insert
				MonitoringInfo* monitorData = (*iter).second;

				int avr;

				// count ���� 0�̸� ��հ� ���� �� exception �߻��ϹǷ� skip
				if (monitorData->iCount == 0)
					continue;

				avr = monitorData->iSum / monitorData->iCount;

				//wchar_t insertDataQuery[256];
				//swprintf_s(insertDataQuery, L"insert into `%s` %s values (now(), %d, %d, %d, %d, %d)",
				//	tmpTableName,
				//	L"(`logtime`, `serverno`, `type`, `avr`, `min`, `max`)",
				//	monitorData->iServerNo, monitorData->type, avr, monitorData->iMin, monitorData->iMax);

				// min ������
				wchar_t insertDataQuery[256] = { 0 };
				swprintf_s(insertDataQuery, L"INSERT INTO `%s` %s values (now(), %d, %d, %d)",
					tmpTableName,
					L"(`logtime`, `logcode`, `type`, `data`)",
					monitorData->type, MONITORTYPE::df_MONITOR_MIN, monitorData->iMin);

				// insert ���� ����
				dbConn->Query(insertDataQuery);

				// max ������
				memset(insertDataQuery, 0, 255 * sizeof(wchar_t));
				swprintf_s(insertDataQuery, L"INSERT INTO `%s` %s values (now(), %d, %d, %d)",
					tmpTableName,
					L"(`logtime`, `logcode`, `type`, `data`)",
					monitorData->type, MONITORTYPE::df_MONITOR_MAX, monitorData->iMax);

				// insert ���� ����
				dbConn->Query(insertDataQuery);

				// avg ������
				memset(insertDataQuery, 0, 255 * sizeof(wchar_t));
				swprintf_s(insertDataQuery, L"INSERT INTO `%s` %s values (now(), %d, %d, %d)",
					tmpTableName,
					L"(`logtime`, `logcode`, `type`, `data`)",
					monitorData->type, MONITORTYPE::df_MONITOR_AVG, avr);

				// insert ���� ����
				dbConn->Query(insertDataQuery);

				// ������ ���� reset
				monitorData->iSum = 0;
				monitorData->iCount = 0;
				monitorData->iMin = INT_MAX;
				monitorData->iMax = INT_MIN;
			}


			ReleaseSRWLockExclusive(&dbDataMapLock);
		}
	}

	return true;
}


// ���� �α��� ��û
void MonitoringLanServer::PacketProc_Login(uint64_t sessionID, CPacket* packet)
{
	BYTE serverNo;

	*packet >> serverNo;			// Server ���� ��ȣ

	AcquireSRWLockShared(&clientMapLock);
	auto iter = clientMap.find(sessionID);

	// �α��� ��û ���� �̹� �α��εǾ� �ִٸ� ����
	if (iter == clientMap.end())
	{
		ReleaseSRWLockShared(&clientMapLock);
		OnError(LOGINEXIST, L"Server is already logined...");
		return;
	}

	ReleaseSRWLockShared(&clientMapLock);

	ClientInfo* client = (*iter).second;
	client->serverNo = serverNo;

	CPacket::Free(packet);
}

// ������ ����͸������� ������ ���� (Lan)
void MonitoringLanServer::PacketProc_Update(uint64_t sessionID, CPacket* packet)
{
	BYTE serverNo;			// Server ���� ��ȣ
	BYTE dataType;			// Monitoring ������ Type
	int dataValue;			// Monitoring ������ ��ġ
	int timeStamp;			// Monitoring �����͸� ���� �ð�

	*packet >> serverNo >> dataType >> dataValue >> timeStamp;

	// dataType�� �ش� ����(serverNo)���� ���ǵ� type define ��
	// serverNo���� �ʿ��� Monitoring Type�� ���߹迭�� ���ǵǾ� ����
	BYTE monitorDataType = monitoringInfoArr[serverNo][dataType];

	// ������ key �� ����
	uint64_t key = GetKey(serverNo, monitorDataType);

	AcquireSRWLockExclusive(&dbDataMapLock);

	auto iter = dbDataMap.find(key);

	MonitoringInfo* monitorInfo = nullptr;

	// Ư�� ������ �����͹�ȣ�� ���� ����͸� ������ �ʿ� ���� ���, �߰�
	if (iter == dbDataMap.end())
	{
		monitorInfo = new MonitoringInfo;
		monitorInfo->iServerNo = serverNo;
		monitorInfo->type = monitorDataType;

		dbDataMap.insert({ key, monitorInfo });
	}
	// ������ ���� ���, �ش� ���� �����
	else monitorInfo = (*iter).second;

	// ����͸� ���� ������Ʈ
	monitorInfo->iCount++;				// ��� ���� ���ϱ� ���� count
	monitorInfo->iSum += dataValue;		// ��� ���� ���ϱ� ���� total ��

	if (monitorInfo->iMin > dataValue)	// �ּ� ���� ��� �� �������� ����
		monitorInfo->iMin = dataValue;	

	if (monitorInfo->iMax < dataValue)	// �ִ� ���� ��� �� �������� ����
		monitorInfo->iMax = dataValue;

	ReleaseSRWLockExclusive(&dbDataMapLock);

	CPacket::Free(packet);

	// ���Ĵ� ����͸� NetServer�� ����
	// ��Ŷ ���� �� �ܺ� Client�鿡�� ������ ����
	monitorNetServ->SendMonitorClients(serverNo, monitorDataType, dataValue, timeStamp);
}

bool MonitoringLanServer::OnConnectionRequest(const wchar_t* IP, unsigned short PORT)
{
	return true;
}

void MonitoringLanServer::OnClientJoin(uint64_t sessionID, wchar_t* ip, unsigned short port)
{
	// client ��ü ����
	ClientInfo* client = clientPool.Alloc();
	client->sessionID = sessionID;
	wcscpy_s(client->ip, _countof(client->ip), ip);
	client->port = port;

	// clientMap�� ��ü insert
	AcquireSRWLockExclusive(&clientMapLock);
	clientMap.insert({ sessionID, client });
	ReleaseSRWLockExclusive(&clientMapLock);
}

void MonitoringLanServer::OnClientLeave(uint64_t sessionID)
{
	// client ��ü ����
	AcquireSRWLockExclusive(&clientMapLock);

	
	auto iter = clientMap.find(sessionID);
		
	// map�� ���� ��ü�� �����Ϸ��Ѵ�?? error!
	if (iter == clientMap.end())
	{
		OnError(ERRORCODE::EMPTYMAPDATA, L"ClientMap find error!");
		CRASH();
	}

	ClientInfo* client = (*iter).second;

	// map���� �ش� ��ü ����
	clientMap.erase(iter);

	ReleaseSRWLockExclusive(&clientMapLock);

	// pool�� ��ȯ
	clientPool.Free(client);
}

void MonitoringLanServer::OnRecv(uint64_t sessionID, CPacket* packet)
{
	WORD type;
	*packet >> type;

	switch (type)
	{
		// Server �� ����͸� ������ �α��� ��
	case en_PACKET_SS_MONITOR_LOGIN:
		PacketProc_Login(sessionID, packet);
		break;

		// Server�� ����͸������� ������ ���� (Lan)
	case en_PACKET_SS_MONITOR_DATA_UPDATE:
		PacketProc_Update(sessionID, packet);
		break;

	default:
	{
		OnError(ERRORCODE::TYPEERROR, L"Tyoe Error");
		CRASH();
		break;
	}
	}
}

void MonitoringLanServer::OnError(int errorCode, const wchar_t* msg)
{
	wstring logMsg = L"[errorCode : " + errorCode;
	logMsg += L"] ";
	logMsg += msg;

	monitorLog->logger(dfLOG_LEVEL_ERROR, __LINE__, logMsg.c_str());
}
