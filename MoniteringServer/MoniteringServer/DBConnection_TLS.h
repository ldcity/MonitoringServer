#ifndef __DBCONNECTION_TLS_CLASS__
#define __DBCONNECTION_TLS_CLASS__

#include "DBConnection.h"
#include "LockFreeStack.h"

using namespace std;


class DBConnector_TLS
{
public:
	DBConnector_TLS(const wchar_t* host, const wchar_t* user, const wchar_t* password, const wchar_t* db, unsigned short port, bool sslOff);
	~DBConnector_TLS();

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

	// Make Query - ���� ������ ��� �ӽ� ����
	// [return]
	// -1 : �Ϲ����� ����
	// 0 : ���� ���� �� ��ȯ�� row ���� 0 (table ���� ���� �� select
	// 1 �̻� : ���� ���� �� ��ȯ�� row ��
	int Query(const wchar_t* strFormat, ...);

	// Make Query - ���� ������ ��� ���� X
	bool QuerySave(const wchar_t* strFormat, ...);

	// ���� ���� �Ϳ� ���� ��� �̾ƿ���
	MYSQL_ROW FetchRow();

	// �� ������ ���� ��� ��� ��� �� ����
	void FreeResult();

	void* GetConnectionStatus();

	void onError();

private:
	DBConnector* GetDBConnector();

private:
	wchar_t mHost[SHORT_LEN];
	wchar_t mUser[MIDDLE_LEN];
	wchar_t mPassword[MIDDLE_LEN];
	wchar_t mDB[MIDDLE_LEN];

	unsigned short mPort;
	bool mFlag;

	// TLS
	DWORD tlsIndex;

	// ���ҽ� ���� �۾� : DB ������ ���μ��� ���� �� �������� �ʱ� ������ ���� Session���� ������ ��������� ��
	LockFreeStack<DBConnector*> DBStack;
};

#endif