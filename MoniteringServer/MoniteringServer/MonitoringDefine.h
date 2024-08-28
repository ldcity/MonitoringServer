#ifndef __MONITOR_DEFINE__
#define __MONITOR_DEFINE__

#include <limits.h>

#define MONITORARRCNF			20


enum ERRORCODE
{
	EMPTYMAPDATA = 100,			// map�� ���� ������ find
	LOGINEXIST,					// �α��� error (�̹� ������)
	TYPEERROR					// type error
};

enum SERVERTYPE
{
	LOGIN_SERVER_TYPE,
	GAME_SERVER_TYPE,
	CHAT_SERVER_TYPE,
	TOTAL,
	MONITOR_SERVER_TYPE
};

// dataType
enum LOGINMONITORTYPE
{
	MONITOR_DATA_TYPE_LOGIN_SERVER_RUN = 0,		// �α��μ��� ���࿩�� ON / OFF
	MONITOR_DATA_TYPE_LOGIN_SERVER_CPU = 1,		// �α��μ��� CPU ����
	MONITOR_DATA_TYPE_LOGIN_SERVER_MEM = 2,		// �α��μ��� �޸� ��� MByte
	MONITOR_DATA_TYPE_LOGIN_SESSION = 3,			// �α��μ��� ���� �� (���ؼ� ��)
	MONITOR_DATA_TYPE_LOGIN_AUTH_TPS = 4,			// �α��μ��� ���� ó�� �ʴ� Ƚ��
	MONITOR_DATA_TYPE_LOGIN_PACKET_POOL = 5,		// �α��μ��� ��ŶǮ ��뷮
};

enum GAMEMONITORTYPE
{

	MONITOR_DATA_TYPE_GAME_SERVER_RUN = 0,		// GameServer ���� ���� ON / OFF
	MONITOR_DATA_TYPE_GAME_SERVER_CPU = 1,		// GameServer CPU ����
	MONITOR_DATA_TYPE_GAME_SERVER_MEM = 2,		// GameServer �޸� ��� MByte
	MONITOR_DATA_TYPE_GAME_SESSION = 3,			// ���Ӽ��� ���� �� (���ؼ� ��)
	MONITOR_DATA_TYPE_GAME_AUTH_PLAYER = 4,		// ���Ӽ��� AUTH MODE �÷��̾� ��
	MONITOR_DATA_TYPE_GAME_GAME_PLAYER = 5,		// ���Ӽ��� GAME MODE �÷��̾� ��
	MONITOR_DATA_TYPE_GAME_ACCEPT_TPS = 6,		// ���Ӽ��� Accept ó�� �ʴ� Ƚ��
	MONITOR_DATA_TYPE_GAME_PACKET_RECV_TPS = 7,	// ���Ӽ��� ��Ŷó�� �ʴ� Ƚ��
	MONITOR_DATA_TYPE_GAME_PACKET_SEND_TPS = 8,	// ���Ӽ��� ��Ŷ ������ �ʴ� �Ϸ� Ƚ��
	MONITOR_DATA_TYPE_GAME_DB_WRITE_TPS = 9,		// ���Ӽ��� DB ���� �޽��� �ʴ� ó�� Ƚ��
	MONITOR_DATA_TYPE_GAME_DB_WRITE_MSG = 10,		// ���Ӽ��� DB ���� �޽��� ť ���� (���� ��)
	MONITOR_DATA_TYPE_GAME_AUTH_THREAD_FPS = 11,	// ���Ӽ��� AUTH ������ �ʴ� ������ �� (���� ��)
	MONITOR_DATA_TYPE_GAME_GAME_THREAD_FPS = 12,	// ���Ӽ��� GAME ������ �ʴ� ������ �� (���� ��)
	MONITOR_DATA_TYPE_GAME_PACKET_POOL = 13,		// ���Ӽ��� ��ŶǮ ��뷮
};

enum CHATMONITORTYPE
{
	MONITOR_DATA_TYPE_CHAT_SERVER_RUN = 0,		// ä�ü��� ChatServer ���� ���� ON / OFF
	MONITOR_DATA_TYPE_CHAT_SERVER_CPU = 1,		// ä�ü��� ChatServer CPU ����
	MONITOR_DATA_TYPE_CHAT_SERVER_MEM = 2,		// ä�ü��� ChatServer �޸� ��� MByte
	MONITOR_DATA_TYPE_CHAT_SESSION = 3,			// ä�ü��� ���� �� (���ؼ� ��)
	MONITOR_DATA_TYPE_CHAT_PLAYER = 4,			// ä�ü��� �������� ����� �� (���� ������)
	MONITOR_DATA_TYPE_CHAT_UPDATE_TPS = 5,		// ä�ü��� UPDATE ������ �ʴ� �ʸ� Ƚ��
	MONITOR_DATA_TYPE_CHAT_PACKET_POOL = 6,		// ä�ü��� ��ŶǮ ��뷮
	MONITOR_DATA_TYPE_CHAT_UPDATEMSG_POOL = 7,	// ä�ü��� UPDATE MSG Ǯ ��뷮
};

struct ClientInfo
{
	uint64_t sessionID;
	BYTE serverNo;
	wchar_t ip[16];
	unsigned short port;
};

struct MonitoringInfo
{
	BYTE iServerNo;
	int type;
	int iSum;			// avr ��� �뵵
	int iCount;			// avr ��� �뵵
	int iMin;			// �ּ�
	int iMax;			// �ִ�

	MonitoringInfo() : iServerNo(-1), type(-1), iSum(0), iCount(0), iMin(INT_MAX), iMax(INT_MIN) {}
};

#endif