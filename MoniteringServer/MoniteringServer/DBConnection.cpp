#include <strsafe.h>

#include "DBConnection.h"

DBConnector::DBConnector(const wchar_t* host, const wchar_t* user, const wchar_t* password, const wchar_t* db, unsigned short port, bool sslOff) : connection(NULL), mFlag(0), mQueryTime(0)
{
	wmemcpy_s(mHost, SHORT_LEN, host, wcslen(host));
	wmemcpy_s(mUser, MIDDLE_LEN, user, wcslen(user));
	wmemcpy_s(mPassword, MIDDLE_LEN, password, wcslen(password));
	wmemcpy_s(mDB, MIDDLE_LEN, db, wcslen(db));
	mPort = port;

	if (sslOff) mFlag = SSL_MODE_DISABLED;
}

DBConnector::~DBConnector()
{
	Close();
}

// Database�� ������ �õ�
bool DBConnector::Open()
{
	// �ʱ�ȭ
	mysql_init(&conn);

	// SSL ��带 ��Ȱ��ȭ
	// ���� ������ ������ �ش� �ɼ� ����
	// SSL�� �����͸� ��ȣȭ�ϰ� ��ȣȭ�ϴ� ������ �߰��ǹǷ� �ణ�� ���� ������尡 �߻�
	// -> �� ����� ��Ȱ��ȭ�Ͽ� ���� ����
	// => ���� ��Ʈ��ũ�� �ִ� ���� �� ����̳� ���� �� �׽�Ʈ ȯ�濡�� SSL�� ��Ȱ��ȭ (��Ʈ��ũ ȯ���� �����ϴٰ� Ȯ��) 
	if (mFlag) mysql_options(&conn, MYSQL_OPT_SSL_MODE, &mFlag);

	string host;
	string user;
	string pwd;
	string db;

	// ���� ���ڵ� ��ȯ (wchar_t -> char)
	WideCharToMultiByte(CP_UTF8, 0, mHost, -1, mHostUTF8, SHORT_LEN, NULL, NULL);
	WideCharToMultiByte(CP_UTF8, 0, mUser, -1, mUserUTF8, MIDDLE_LEN, NULL, NULL);
	WideCharToMultiByte(CP_UTF8, 0, mPassword, -1, mPasswordUTF8, MIDDLE_LEN, NULL, NULL);
	WideCharToMultiByte(CP_UTF8, 0, mDB, -1, mDBUTF8, MIDDLE_LEN, NULL, NULL);

	// DB ���� �õ� �� ��õ�
	connection = mysql_real_connect(&conn, mHostUTF8, mUserUTF8, mPasswordUTF8, mDBUTF8, mPort, (char*)NULL, 0);
	if (connection == NULL)
	{
		// ���� ���� ��, ���� Ƚ�� �翬�� �õ�
		int tryCnt = 0;

		while (NULL == connection && tryCnt <= DBCONNECT_TRY)
		{
			// �翬�� �õ�
			connection = mysql_real_connect(&conn, mHostUTF8, mUserUTF8, mPasswordUTF8, mDBUTF8, mPort, (char*)NULL, 0);
			
			tryCnt++;
			Sleep(500);
		}

		return false;
	}

	mysql_set_character_set(connection, "utf8");

	return true;
}

void DBConnector::Close()
{
	// DB ���� ����
	mysql_close(connection);
	//wprintf(L"DB Closed...\n");
}

bool DBConnector::StartTransaction()
{
	if (mysql_query(connection, "start transaction;"))
	{
		printf("Query Error : %s\n", mysql_error(connection));
		return false;
	}

	return true;
}

bool DBConnector::Commit()
{
	if (mysql_query(connection, "commit;"))
	{
		printf("Query Error : %s\n", mysql_error(connection));
		return false;
	}

	return true;
}

// ���� ����
// ���� ���ڸ� �޾� ������ �����ϰ� ����� ����
bool DBConnector::Query(const wchar_t* strFormat, ...)
{
	// ���� ���̿� ���� ���� ó��
	if (QUERY_MAX_LEN < wcslen(strFormat))
		return false;
	
	memset(mQuery, 0, sizeof(wchar_t) * QUERY_MAX_LEN);
	memset(mQueryUTF8, 0, sizeof(char) * QUERY_MAX_LEN);

	HRESULT hResult;
	va_list vList;
	int queryError;

	va_start(vList, strFormat);
	hResult = StringCchPrintf(mQuery, QUERY_MAX_LEN, strFormat, vList);
	va_end(vList);

	if (FAILED(hResult))
		return false;

	// ���ڿ� ��ȯ
	WideCharToMultiByte(CP_UTF8, 0, mQuery, -1, mQueryUTF8, QUERY_MAX_LEN, NULL, NULL);

	// ���� ���� �ð�
	mQueryTime = GetTickCount64();

	int queryResult = mysql_query(connection, mQueryUTF8);

	// ���� ���� ���� ��, �翬��
	if (queryResult != 0)
	{
		onError();

		Close();
		Open();

		return false;
	}

	ULONGLONG time = GetTickCount64();

	if (time - mQueryTime > QUERY_MAX_TIME)
	{
		wprintf(L"Too much Time has passed [%IId]\n", time);
	}
	mQueryTime = time;

	// ��� �� ����
	myResult = mysql_store_result(connection);

	return true;
}

// ���� ����
// ������ �����ϰ� ����� �������� ����
bool DBConnector::QuerySave(const wchar_t* strFormat, ...)
{
	// ���� ���̿� ���� ���� ó��
	if (QUERY_MAX_LEN < wcslen(strFormat))
		return false;

	memset(mQuery, 0, sizeof(wchar_t) * QUERY_MAX_LEN);
	memset(mQueryUTF8, 0, sizeof(char) * QUERY_MAX_LEN);

	HRESULT hResult;
	va_list vList;
	int queryError;

	va_start(vList, strFormat);
	hResult = StringCchPrintf(mQuery, QUERY_MAX_LEN, strFormat, vList);
	va_end(vList);

	if (FAILED(hResult))
		return false;

	// wchar_t -> char ��ȯ
	WideCharToMultiByte(CP_UTF8, 0, mQuery, -1, mQueryUTF8, QUERY_MAX_LEN, NULL, NULL);

	// ���� ���� �ð�
	mQueryTime = GetTickCount64();

	int queryResult = mysql_query(connection, mQueryUTF8);

	if (queryResult != 0)
	{
		onError();

		Close();
		Open();

		return false;
	}

	ULONGLONG time = GetTickCount64();

	if (time - mQueryTime > QUERY_MAX_TIME)
	{
		wprintf(L"Too much Time has passed [%IId]\n", time);
	}
	mQueryTime = time;

	// �� ������ ���� ��� ��� ��� �� ����
	FreeResult();

	return true;
}



bool DBConnector::AllQuery(string query)
{
	string requestSQL = "start transaction;";
	requestSQL += query;
	requestSQL += " commit;";
	
	if (mysql_real_query(connection, requestSQL.c_str(), requestSQL.length()) != 0)
	{
		printf("Query Error : %s\n", mysql_error(connection));
		return false;
	}


	do
	{
		MYSQL_RES* result = mysql_store_result(connection);
		if (result != NULL)
			FreeResult();
	} while (!mysql_next_result(connection));

	return true;
}





void DBConnector::onError()
{	
	strcpy_s(errorMsgUTF8, mysql_error(connection));

	// char -> wchar_t
	int result = MultiByteToWideChar(CP_UTF8, 0, errorMsgUTF8, strlen(errorMsgUTF8), errorMsg, sizeof(errorMsg));
	if (result < sizeof(errorMsg))
		errorMsg[result] = L'\0';

	//DBLog->logger(dfLOG_LEVEL_ERROR, __LINE__, errorMsg);
}