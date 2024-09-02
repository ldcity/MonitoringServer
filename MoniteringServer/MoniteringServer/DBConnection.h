#ifndef __DBCONNECTION_CLASS__
#define __DBCONNECTION_CLASS__

#pragma comment(lib, "mysql/libmysql.lib")

#include "mysql/include/mysql.h"
#include "mysql/include/errmsg.h"

#include <Windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <functional>

#include "LOG.h"
#include "include/json/json.h"

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

public:
	// MySQL ������ Ÿ�� �߷�
	template<typename T>
	enum enum_field_types GetMysqlType();

	// MySQL ������ Ÿ�� �߷� Ư��ȭ
	template<>
	enum enum_field_types GetMysqlType<int>()
	{
		return MYSQL_TYPE_LONG;
	}

	template<>
	enum enum_field_types GetMysqlType<int&>()
	{
		return MYSQL_TYPE_LONG;
	}

	template<>
	enum enum_field_types GetMysqlType<int64_t*>()
	{
		return MYSQL_TYPE_LONGLONG;
	}

	template<>
	enum enum_field_types GetMysqlType<int64_t>()
	{
		return MYSQL_TYPE_LONGLONG;
	}

	template<>
	enum enum_field_types GetMysqlType<std::string>()
	{
		return MYSQL_TYPE_STRING;
	}

	template<>
	enum enum_field_types GetMysqlType<std::wstring>()
	{
		return MYSQL_TYPE_STRING;
	}

	template<>
	enum enum_field_types GetMysqlType<const char*>()
	{
		return MYSQL_TYPE_STRING;
	}

	template<>
	enum enum_field_types GetMysqlType<char*>()
	{
		return MYSQL_TYPE_STRING;
	}


	// �ʿ��, ��Ÿ Ư��ȭ �߰�...

private:
	void BindParam(std::vector<MYSQL_BIND>& bindParams, std::wstring& value)
	{
		MYSQL_BIND bind;
		memset(&bind, 0, sizeof(MYSQL_BIND));

		// std::wstring�� UTF-8�� ��ȯ�ϱ� ���� ����� ���� ũ�� ���
		int utf8Length = WideCharToMultiByte(CP_UTF8, 0, value.c_str(), -1, NULL, 0, NULL, NULL);
		std::string utf8Value(utf8Length, 0);

		// ��ȯ ����
		WideCharToMultiByte(CP_UTF8, 0, value.c_str(), -1, &utf8Value[0], utf8Length, NULL, NULL);

		bind.buffer_type = MYSQL_TYPE_STRING;
		bind.buffer = (char*)utf8Value.c_str();
		bind.buffer_length = utf8Value.length();

		bindParams.push_back(bind);
	}


	// std::string�� ���� ����� �����ε�
	void BindParam(std::vector<MYSQL_BIND>& bindParams, std::string& value)
	{
		MYSQL_BIND bind;
		memset(&bind, 0, sizeof(MYSQL_BIND));

		bind.buffer_type = MYSQL_TYPE_STRING;
		bind.buffer = (char*)value.c_str();
		bind.buffer_length = value.length();

		bindParams.push_back(bind);
	}

	// `char*`�� �迭 ��θ� ó���� �� �ִ� ���ε� �Լ�
	void BindParam(std::vector<MYSQL_BIND>& bindParams, const char* value)
	{
		MYSQL_BIND bind;
		memset(&bind, 0, sizeof(MYSQL_BIND));

		bind.buffer_type = MYSQL_TYPE_STRING;
		bind.buffer = (char*)value;
		bind.buffer_length = strlen(value);  // ���ڿ� ���� ���

		bindParams.push_back(bind);
	}

	// �Ű����� ���ε��� ó���ϴ� �Լ�
	template<typename T>
	void BindParams(std::vector<MYSQL_BIND>& bindParams, T& value) {
		BindParam(bindParams, value);
	}

	// ���ø� �Լ��� �Ű����� ���ε�
	template<typename T, typename... Args>
	void BindParams(std::vector<MYSQL_BIND>& bindParams, T& first, Args... rest)
	{
		BindParam(bindParams, first);
		BindParams(bindParams, rest...);
	}

	void BindParams(std::vector<MYSQL_BIND>& bindParams) {} // ���ȣ�� ���� ����

	// �Ű����� �ϳ��� ���ε��ϴ� �Լ� (Ư��ȭ �ʿ�)
	template<typename T>
	void BindParam(std::vector<MYSQL_BIND>& bindParams, T& value)
	{
		MYSQL_BIND bind;
		memset(&bind, 0, sizeof(MYSQL_BIND));

		bind.buffer_type = GetMysqlType<T>();
		bind.buffer = (char*)&value;

		bindParams.push_back(bind);
	}

	// ��� ��ġ �Լ�
	template<typename T, typename... Args>
	int FetchResultImpl(MYSQL_STMT* stmt, T& firstArg, Args&... restArgs)
	{
		MYSQL_BIND resultBind[sizeof...(Args) + 1];
		memset(resultBind, 0, sizeof(resultBind));

		// �⺻�� ����
		size_t i = 0;
		auto setupBind = [&](auto& arg, MYSQL_BIND& bind)
		{
			bind.buffer_type = GetMysqlType<decltype(arg)>();
			bind.buffer = reinterpret_cast<char*>(&arg);
			bind.is_null = nullptr;
			bind.length = nullptr;

			if (bind.buffer_type == MYSQL_TYPE_STRING)
			{
				// ���ڿ� ������ ũ�� ����
				bind.buffer_length = sizeof(arg);

				// ���� ������ ����
				bind.length = &bind.buffer_length;
			}
		};

		setupBind(firstArg, resultBind[i++]);
		(..., setupBind(restArgs, resultBind[i++]));

		if (mysql_stmt_bind_result(stmt, resultBind))
		{
			return -1;
		}

		return mysql_stmt_fetch(stmt);
	}

public:
	// DB ����
	bool Open();

	bool ReOpen();

	// DB ���� ����
	void Close();

	// Transaction Start
	bool StartTransaction();

	// Commit
	bool Commit();

	// �Ű�����ȭ�� ���� ���� - ���� ����
	bool ExecuteQuery(const std::wstring& query, std::vector<MYSQL_BIND>& bindParams, std::function<bool(MYSQL_STMT*, Log* dbLog)> resultHandler);

	// ���ø� �Լ��� �Ű������� ���ε� - ���� ���� ����
	template<typename... Args>
	bool ExecuteQuery(const std::wstring& query, std::function<bool(MYSQL_STMT*, Log* dbLog)> resultHandler, Args... args)
	{
		std::vector<MYSQL_BIND> bindParams;
		BindParams(bindParams, args...);
		return ExecuteQuery(query, bindParams, resultHandler);
	}

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
	MYSQL_ROW FetchRow()
	{
		return mysql_fetch_row(myResult);
	}

	// �� ������ ���� ��� ��� ��� �� ����
	void FreeResult()
	{
		mysql_free_result(myResult);
	}

	void* GetConnectionStatus()
	{
		return connection;
	}

	void onError();

public:
	bool CallStoreProcedure(const std::wstring& procedureQuery, const std::string& tableName, int logCode, const std::vector<std::pair<int, int>>& typeData);

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

	private:
		Log* dbLog;
};


#endif