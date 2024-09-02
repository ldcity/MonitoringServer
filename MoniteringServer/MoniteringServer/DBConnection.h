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
	// MySQL 데이터 타입 추론
	template<typename T>
	enum enum_field_types GetMysqlType();

	// MySQL 데이터 타입 추론 특수화
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


	// 필요시, 기타 특수화 추가...

private:
	void BindParam(std::vector<MYSQL_BIND>& bindParams, std::wstring& value)
	{
		MYSQL_BIND bind;
		memset(&bind, 0, sizeof(MYSQL_BIND));

		// std::wstring을 UTF-8로 변환하기 위한 충분한 버퍼 크기 계산
		int utf8Length = WideCharToMultiByte(CP_UTF8, 0, value.c_str(), -1, NULL, 0, NULL, NULL);
		std::string utf8Value(utf8Length, 0);

		// 변환 수행
		WideCharToMultiByte(CP_UTF8, 0, value.c_str(), -1, &utf8Value[0], utf8Length, NULL, NULL);

		bind.buffer_type = MYSQL_TYPE_STRING;
		bind.buffer = (char*)utf8Value.c_str();
		bind.buffer_length = utf8Value.length();

		bindParams.push_back(bind);
	}


	// std::string에 대한 명시적 오버로딩
	void BindParam(std::vector<MYSQL_BIND>& bindParams, std::string& value)
	{
		MYSQL_BIND bind;
		memset(&bind, 0, sizeof(MYSQL_BIND));

		bind.buffer_type = MYSQL_TYPE_STRING;
		bind.buffer = (char*)value.c_str();
		bind.buffer_length = value.length();

		bindParams.push_back(bind);
	}

	// `char*`와 배열 모두를 처리할 수 있는 바인딩 함수
	void BindParam(std::vector<MYSQL_BIND>& bindParams, const char* value)
	{
		MYSQL_BIND bind;
		memset(&bind, 0, sizeof(MYSQL_BIND));

		bind.buffer_type = MYSQL_TYPE_STRING;
		bind.buffer = (char*)value;
		bind.buffer_length = strlen(value);  // 문자열 길이 계산

		bindParams.push_back(bind);
	}

	// 매개변수 바인딩을 처리하는 함수
	template<typename T>
	void BindParams(std::vector<MYSQL_BIND>& bindParams, T& value) {
		BindParam(bindParams, value);
	}

	// 템플릿 함수로 매개변수 바인딩
	template<typename T, typename... Args>
	void BindParams(std::vector<MYSQL_BIND>& bindParams, T& first, Args... rest)
	{
		BindParam(bindParams, first);
		BindParams(bindParams, rest...);
	}

	void BindParams(std::vector<MYSQL_BIND>& bindParams) {} // 재귀호출 종료 조건

	// 매개변수 하나를 바인딩하는 함수 (특수화 필요)
	template<typename T>
	void BindParam(std::vector<MYSQL_BIND>& bindParams, T& value)
	{
		MYSQL_BIND bind;
		memset(&bind, 0, sizeof(MYSQL_BIND));

		bind.buffer_type = GetMysqlType<T>();
		bind.buffer = (char*)&value;

		bindParams.push_back(bind);
	}

	// 결과 패치 함수
	template<typename T, typename... Args>
	int FetchResultImpl(MYSQL_STMT* stmt, T& firstArg, Args&... restArgs)
	{
		MYSQL_BIND resultBind[sizeof...(Args) + 1];
		memset(resultBind, 0, sizeof(resultBind));

		// 기본값 설정
		size_t i = 0;
		auto setupBind = [&](auto& arg, MYSQL_BIND& bind)
		{
			bind.buffer_type = GetMysqlType<decltype(arg)>();
			bind.buffer = reinterpret_cast<char*>(&arg);
			bind.is_null = nullptr;
			bind.length = nullptr;

			if (bind.buffer_type == MYSQL_TYPE_STRING)
			{
				// 문자열 버퍼의 크기 설정
				bind.buffer_length = sizeof(arg);

				// 길이 포인터 설정
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
	// DB 연결
	bool Open();

	bool ReOpen();

	// DB 연결 끊기
	void Close();

	// Transaction Start
	bool StartTransaction();

	// Commit
	bool Commit();

	// 매개변수화된 쿼리 실행 - 벡터 버전
	bool ExecuteQuery(const std::wstring& query, std::vector<MYSQL_BIND>& bindParams, std::function<bool(MYSQL_STMT*, Log* dbLog)> resultHandler);

	// 템플릿 함수로 매개변수를 바인딩 - 가변 인자 받음
	template<typename... Args>
	bool ExecuteQuery(const std::wstring& query, std::function<bool(MYSQL_STMT*, Log* dbLog)> resultHandler, Args... args)
	{
		std::vector<MYSQL_BIND> bindParams;
		BindParams(bindParams, args...);
		return ExecuteQuery(query, bindParams, resultHandler);
	}

	// Make Query - 쿼리 날리고 결과 저장 X
	bool Query(const wchar_t* strFormat, ...);

	// Make Query - 쿼리 날리고 결과 임시 보관
	// [return]
	// -1 : 일반적인 실패
	// 0 : 쿼리 성공 후 반환된 row 값이 0 (table 내에 없는 값 select
	// 1 이상 : 쿼리 성공 후 반환된 row 값
	int QuerySave(const wchar_t* strFormat, ...);

	// Transaction ~ Query ~ Commit
	bool AllQuery(std::string query);

	// 쿼리 날린 것에 대한 결과 뽑아오기
	MYSQL_ROW FetchRow()
	{
		return mysql_fetch_row(myResult);
	}

	// 한 쿼리에 대한 결과 모두 사용 후 정리
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
	// JSON 데이터 생성 -> 프로시저 매개변수로 전달을 위해 필요
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