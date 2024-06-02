#include <strsafe.h>

#include "DBConnection_TLS.h"

DBConnector_TLS::DBConnector_TLS(const wchar_t* host, const wchar_t* user, const wchar_t* password, const wchar_t* db, unsigned short port, bool sslOff) : mFlag(0)
{
	tlsIndex = TlsAlloc();
	if (tlsIndex == TLS_OUT_OF_INDEXES)
	{
		wprintf(L"DBConnector_TLS : TLS_OUT_OF_INDEXES\n");
		CRASH();
	}

	wmemcpy_s(mHost, SHORT_LEN, host, wcslen(host));
	wmemcpy_s(mUser, MIDDLE_LEN, user, wcslen(user));
	wmemcpy_s(mPassword, MIDDLE_LEN, password, wcslen(password));
	wmemcpy_s(mDB, MIDDLE_LEN, db, wcslen(db));
	mPort = port;

	if (sslOff)
		mFlag = SSL_MODE_DISABLED;
}


DBConnector_TLS::~DBConnector_TLS()
{
	DBConnector* dbConn;

	while (DBStack.Pop(&dbConn))
	{
		delete dbConn;
	}

	TlsFree(tlsIndex);

}

DBConnector* DBConnector_TLS::GetDBConnector()
{
	DBConnector* dbConn = (DBConnector*)TlsGetValue(tlsIndex);
	if (dbConn == nullptr)
	{
		dbConn = new DBConnector(mHost, mUser, mPassword, mDB, mPort, mFlag);

		DBStack.Push(dbConn);
		TlsSetValue(tlsIndex, dbConn);

		while (!dbConn->Open())
		{
			Sleep(1000);
		}
	}

	return dbConn;
}

void DBConnector_TLS::Close()
{
	DBConnector* dbConn = GetDBConnector();

	return dbConn->Close();
}

bool DBConnector_TLS::Query(const wchar_t* strFormat, ...)
{
	if (DBConnector::QUERY_MAX_LEN < wcslen(strFormat))
	{
		wprintf( L"Query Length is Too Long!");
		return false;
	}

	DBConnector* dbConn = GetDBConnector();

	// �������� ���ø��� �Ű������� �ѱ��...
	// �׳� ���⼭ �� �����ؼ� �Ѱܹ����߰ڴ�
	wchar_t queryMsg[DBConnector::QUERY_MAX_LEN] = { 0 };

	HRESULT hResult;
	va_list vList;
	int queryError;

	va_start(vList, strFormat);
	hResult = StringCchPrintf(queryMsg, DBConnector::QUERY_MAX_LEN, strFormat, vList);
	va_end(vList);

	if (FAILED(hResult))
		return false;

	return dbConn->Query(queryMsg);
}

bool DBConnector_TLS::QuerySave(const wchar_t* strFormat, ...)
{
	if (DBConnector::QUERY_MAX_LEN < wcslen(strFormat))
	{
		wprintf(L"Query Length is Too Long!");
		return false;
	}

	DBConnector* dbConn = GetDBConnector();

	// �������� ���ø��� �Ű������� �ѱ��...
	// �׳� ���⼭ �� �����ؼ� �Ѱܹ����߰ڴ�
	wchar_t queryMsg[DBConnector::QUERY_MAX_LEN] = { 0 };

	HRESULT hResult;
	va_list vList;
	int queryError;

	va_start(vList, strFormat);
	hResult = StringCchPrintf(queryMsg, DBConnector::QUERY_MAX_LEN, strFormat, vList);
	va_end(vList);

	if (FAILED(hResult))
		return false;

	return dbConn->QuerySave(queryMsg);
}

MYSQL_ROW DBConnector_TLS::FetchRow()
{
	DBConnector* dbConn = GetDBConnector();

	return dbConn->FetchRow();
}

// �� ������ ���� ��� ��� ��� �� ����
void DBConnector_TLS::FreeResult()
{
	DBConnector* dbConn = GetDBConnector();

	dbConn->FreeResult();
}

void* DBConnector_TLS::GetConnectionStatus()
{
	DBConnector* dbConn = GetDBConnector();

	return dbConn->GetConnectionStatus();
}


void DBConnector_TLS::onError()
{
	DBConnector* dbConn = GetDBConnector();

	dbConn->onError();
}