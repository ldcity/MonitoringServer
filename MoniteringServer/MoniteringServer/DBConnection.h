#ifndef __DBCONNECTION_CLASS__
#define __DBCONNECTION_CLASS__

#pragma comment(lib, "mysql/libmysql.lib")

#include "mysql/include/mysql.h"
#include "mysql/include/errmsg.h"

#include <Windows.h>
#include <string>
#include <vector>

#include "LOG.h"

using namespace std;

class DBConnector
{
public:
	DBConnector(const wchar_t* host, const wchar_t* user, const wchar_t* password, const wchar_t* db, unsigned short port, bool sslOff);
	~DBConnector();

	enum myDB
	{
		QUERY_MAX_LEN = 1024,
		QUERY_MAX_TIME = 1000,
		DBCONNECT_TRY = 5,
		SHORT_LEN = 16,
		MIDDLE_LEN = 64,
		LONG_LEN = 128
	};

	// DB ����
	bool Open();

	// DB ���� ����
	void Close();

	// Transaction Start
	bool StartTransaction();

	// Commit
	bool Commit();

	// Make Query - ���� ������ ��� ���� X
	bool Query(const wchar_t* strFormat, ...);

	// Make Query - ���� ������ ��� �ӽ� ����
	// [return]
	// -1 : �Ϲ����� ����
	// 0 : ���� ���� �� ��ȯ�� row ���� 0 (table ���� ���� �� select
	// 1 �̻� : ���� ���� �� ��ȯ�� row ��
	int QuerySave(const wchar_t* strFormat, ...);

	// Transaction ~ Query ~ Commit
	bool AllQuery(std::string query);

	// ���� ���� �Ϳ� ���� ��� �̾ƿ���
	inline MYSQL_ROW FetchRow()
	{
		return mysql_fetch_row(myResult);
	}

	// �� ������ ���� ��� ��� ��� �� ����
	inline void FreeResult()
	{
		mysql_free_result(myResult);
	}

	inline void* GetConnectionStatus()
	{
		return connection;
	}

	void onError();

public:
	bool CallStoreProc(const std::wstring& procName, const std::wstring& tableName, const std::vector<std::pair<int, int>>& typeData);

private:
	// JSON ������ ���� -> ���ν��� �Ű������� ������ ���� �ʿ�
	std::string GenerateJSONData(const std::vector<std::pair<int, int>>& typeData);

private:
	MYSQL conn;
	MYSQL* connection;

	MYSQL_RES* myResult;

	wchar_t mHost[SHORT_LEN];
	wchar_t mUser[MIDDLE_LEN];
	wchar_t mPassword[MIDDLE_LEN];
	wchar_t mDB[MIDDLE_LEN];

	char mHostUTF8[SHORT_LEN];
	char mUserUTF8[MIDDLE_LEN];
	char mPasswordUTF8[MIDDLE_LEN];
	char mDBUTF8[MIDDLE_LEN];

	unsigned short mPort;
	bool mFlag;

	wchar_t mQuery[QUERY_MAX_LEN];
	char mQueryUTF8[QUERY_MAX_LEN];

	wchar_t errorMsg[LONG_LEN];
	char errorMsgUTF8[LONG_LEN];

	ULONGLONG mQueryTime;

	uint64_t connectCnt;
	uint64_t connectFailCnt;
	uint64_t disconnectCnt;
};


#endif