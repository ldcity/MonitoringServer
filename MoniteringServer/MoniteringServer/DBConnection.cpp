#include <strsafe.h>

#include "DBConnection.h"

DBConnector::DBConnector(const wchar_t* host, const wchar_t* user, const wchar_t* password, const wchar_t* db, unsigned short port, bool sslOff) : connection(NULL), mFlag(0), mQueryTime(0)
{
	wmemcpy_s(mHost, SHORT_LEN, host, wcslen(host));
	wmemcpy_s(mUser, MIDDLE_LEN, user, wcslen(user));
	wmemcpy_s(mPassword, MIDDLE_LEN, password, wcslen(password));
	wmemcpy_s(mDB, MIDDLE_LEN, db, wcslen(db));
	mPort = port;

	if (sslOff)
		mFlag = SSL_MODE_DISABLED;

	dbLog = new Log(L"DBLog.txt");
}


DBConnector::~DBConnector()
{
	delete dbLog;
}

bool DBConnector::Open()
{
	// �ʱ�ȭ
	mysql_init(&conn);

	// SSL ��带 ��Ȱ��ȭ
	// ���� ������ ������ �ش� �ɼ� ����
	// SSL�� �����͸� ��ȣȭ�ϰ� ��ȣȭ�ϴ� ������ �߰��ǹǷ� �ణ�� ���� ������尡 �߻�
	// -> �� ����� ��Ȱ��ȭ�Ͽ� ���� ����
	// => ���� ��Ʈ��ũ�� �ִ� ���� �� ����̳� ���� �� �׽�Ʈ ȯ�濡�� SSL�� ��Ȱ��ȭ (��Ʈ��ũ ȯ���� �����ϴٰ� Ȯ��) 
	if (mFlag)
	{
		mysql_options(&conn, MYSQL_OPT_SSL_MODE, &mFlag);
	}

	std::string host;
	std::string user;
	std::string pwd;
	std::string db;

	WideCharToMultiByte(CP_UTF8, 0, mHost, -1, mHostUTF8, SHORT_LEN, NULL, NULL);
	WideCharToMultiByte(CP_UTF8, 0, mUser, -1, mUserUTF8, MIDDLE_LEN, NULL, NULL);
	WideCharToMultiByte(CP_UTF8, 0, mPassword, -1, mPasswordUTF8, MIDDLE_LEN, NULL, NULL);
	WideCharToMultiByte(CP_UTF8, 0, mDB, -1, mDBUTF8, MIDDLE_LEN, NULL, NULL);

	// DB ����
	connection = mysql_real_connect(&conn, mHostUTF8, mUserUTF8, mPasswordUTF8, mDBUTF8, mPort, (char*)NULL, 0);
	if (connection == NULL)
	{
		// ���� ���� ��, ���� Ƚ�� �翬�� �õ�
		int tryCnt = 0;

		while (NULL == connection)
		{
			if (tryCnt > DBCONNECT_TRY)
			{
				// ���� ���� ����!

				return false;
			}

			// �翬�� �õ�
			connection = mysql_real_connect(&conn, mHostUTF8, mUserUTF8, mPasswordUTF8, mDBUTF8, mPort, (char*)NULL, 0);

			tryCnt++;
			Sleep(500);
		}

		return false;
	}

	mysql_set_character_set(connection, "utf8mb4");

	return true;
}

bool DBConnector::ReOpen()
{
	Close();
	Open();

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

bool DBConnector::Query(const wchar_t* strFormat, ...)
{
	if (QUERY_MAX_LEN < wcslen(strFormat))
	{
		//DBLog->logger(dfLOG_LEVEL_ERROR, __LINE__, L"Query Length is Too Long!");
		wprintf(L"Query Length is Too Long!");
		return false;
	}

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

	// wchar_t -> char
	//WideCharToMultiByte(CP_UTF8, 0, mQuery, lstrlenW(mQuery), mQueryUTF8, sizeof(mQuery), NULL, NULL);
	WideCharToMultiByte(CP_UTF8, 0, mQuery, -1, mQueryUTF8, QUERY_MAX_LEN, NULL, NULL);

	// ���� ���� �ð�
	mQueryTime = GetTickCount64();

	int queryResult = mysql_query(connection, mQueryUTF8);

	// ���� ����
	if (queryResult != 0)
	{
		onError();

		// �翬��
		Close();
		Open();

		return false;
	}

	ULONGLONG time = GetTickCount64();

	if (time - mQueryTime > QUERY_MAX_TIME)
	{
		//DBLog->logger(dfLOG_LEVEL_ERROR, __LINE__, L"Too much Time has passed [%IId]", time);
		wprintf(L"Too much Time has passed [%IId]", time);
	}
	
	mQueryTime = time;

	// ��� �� ����
	myResult = mysql_store_result(connection);

	FreeResult();

	return true;
}

int DBConnector::QuerySave(const wchar_t* strFormat, ...)
{
	if (QUERY_MAX_LEN < wcslen(strFormat))
	{
		//DBLog->logger(dfLOG_LEVEL_ERROR, __LINE__, L"Query Length is Too Long!");
		wprintf(L"Query Length is Too Long!");
		return false;
	}

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

	// wchar_t -> char
	//WideCharToMultiByte(CP_UTF8, 0, mQuery, lstrlenW(mQuery), mQueryUTF8, sizeof(mQuery), NULL, NULL);
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
		//DBLog->logger(dfLOG_LEVEL_ERROR, __LINE__, L"Too much Time has passed [%IId]", time);
		wprintf(L"Too much Time has passed [%IId]", time);
	}

	mQueryTime = time;

	// ��� �� ����
	myResult = mysql_store_result(connection);

	// ���� ���� ��, ��� row ���� ��ȯ
	return mysql_num_rows(myResult);
}



bool DBConnector::AllQuery(std::string query)
{
	std::string requestSQL = "start transaction;";
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

// �Ű�����ȭ�� ���� ���� - ���� ����
bool DBConnector::ExecuteQuery(const std::wstring& query, std::vector<MYSQL_BIND>& bindParams, std::function<bool(MYSQL_STMT*, Log* dbLog)> resultHandler)
{
	MYSQL_STMT* stmt = mysql_stmt_init(connection);
	if (!stmt)
	{
		dbLog->logger(dfLOG_LEVEL_ERROR, __LINE__, L"[DB] Coult not initialize statement");
		return false;
	}

	std::string utf8Query(query.begin(), query.end());
	if (mysql_stmt_prepare(stmt, utf8Query.c_str(), utf8Query.size()))
	{
		std::cout << mysql_stmt_error(stmt) << std::endl;
		dbLog->logger(dfLOG_LEVEL_ERROR, __LINE__, L"[DB] Statement preparation failed : %s", mysql_stmt_error(stmt));
		mysql_stmt_close(stmt);
		return false;
	}

	if (!bindParams.empty() && mysql_stmt_bind_param(stmt, bindParams.data()))
	{
		dbLog->logger(dfLOG_LEVEL_ERROR, __LINE__, L"[DB] Parameter binding failed : %s", mysql_stmt_error(stmt));
		mysql_stmt_close(stmt);
		return false;
	}

	if (mysql_stmt_execute(stmt))
	{
		std::cout << mysql_stmt_error(stmt) << std::endl;
		dbLog->logger(dfLOG_LEVEL_ERROR, __LINE__, L"[DB] Query execution failed : %s", mysql_stmt_error(stmt));
		mysql_stmt_close(stmt);
		return false;
	}

	bool isSuccessful = true;

	if (resultHandler)
		isSuccessful = resultHandler(stmt, dbLog);

	mysql_stmt_close(stmt);

	return isSuccessful;
}

bool DBConnector::CallStoreProcedure(const std::wstring& procedureQuery, const std::string& tableName, int logCode, const std::vector<std::pair<int, int>>& typeData)
{
	std::string jsonData = GenerateJSONData(typeData);

	std::vector<MYSQL_BIND> bindParams;

	// �Ű����� ���ε�
	MYSQL_BIND bind;
	memset(&bind, 0, sizeof(bind));

	// ���̺� �̸� (���ڿ�)
	bind.buffer_type = MYSQL_TYPE_STRING;
	bind.buffer = (char*)tableName.c_str();
	bind.buffer_length = tableName.length();
	bindParams.push_back(bind);
	
	MYSQL_BIND bind2;
	memset(&bind2, 0, sizeof(bind2));

	// �α� �ڵ� (������)
	bind2.buffer_type = MYSQL_TYPE_LONG;
	bind2.buffer = &logCode;
	bindParams.push_back(bind2);

	MYSQL_BIND bind3;
	memset(&bind3, 0, sizeof(bind3));

	// JSON ������ (���ڿ�)
	bind3.buffer_type = MYSQL_TYPE_STRING;
	bind3.buffer = (char*)jsonData.c_str();
	bind3.buffer_length = jsonData.length();
	bindParams.push_back(bind3);

	// �Ű����� ���ε� �Ŀ� �Ű�����ȭ�� ���� ������ procedureQuery ȣ��
	if (!ExecuteQuery(procedureQuery.c_str(), bindParams, nullptr))
	{
		return false;
	}

	return true;
}

std::string DBConnector::GenerateJSONData(const std::vector<std::pair<int, int>>& typeData)
{
	Json::Value jsonData(Json::arrayValue);

	for (const auto& [type, data] : typeData)
	{
		Json::Value item;
		item["type"] = type;
		item["data"] = data;
		jsonData.append(item);
	}

	Json::StreamWriterBuilder writer;
	writer["indentation"] = "";
	std::string jsonString = Json::writeString(writer, jsonData);

	return jsonString;
}