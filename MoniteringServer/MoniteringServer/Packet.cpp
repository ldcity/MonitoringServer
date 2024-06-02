#include "Packet.h"
#include "Define.h"
#include "Protocol.h"

// ä�ü��� �α��� ���� ��Ŷ
void mpResLogin(CPacket* packet, BYTE status, INT64 accountNo)
{
	WORD type = en_PACKET_CS_CHAT_RES_LOGIN;
	*packet << type << status << accountNo;
}

// ä�ü��� ���� �̵� ��� ��Ŷ
void mpResSectorMove(CPacket* packet, INT64 accountNo, WORD sectorX, WORD sectorY)
{
	WORD type = en_PACKET_CS_CHAT_RES_SECTOR_MOVE;
	*packet << type << accountNo << sectorX << sectorY;
}

// ä�ü��� ä�ú����� ���� ��Ŷ
void mpResChatMessage(CPacket* packet, INT64 accountNo, WCHAR* id, WCHAR* nickname, WORD msgLen, WCHAR* msg)
{
	WORD type = en_PACKET_CS_CHAT_RES_MESSAGE;
	*packet << type << accountNo;
	packet->PutData((char*)id, ID_MAX_LEN * sizeof(wchar_t));
	packet->PutData((char*)nickname, NICKNAME_MAX_LEN * sizeof(wchar_t));
	*packet << msgLen;
	packet->PutData((char*)msg, msgLen);
}