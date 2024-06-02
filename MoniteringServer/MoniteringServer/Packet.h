#ifndef __CHATTING_PACKET__
#define __CHATTING_PACKET__

#include "SerializingBuffer.h"

// ä�ü��� �α��� ���� ��Ŷ
void mpResLogin(CPacket* packet, BYTE status, INT64 accountNo);

// ä�ü��� ���� �̵� ��� ��Ŷ
void mpResSectorMove(CPacket* packet, INT64 accountNo, WORD sectorX, WORD sectorY);

// ä�ü��� ä�ú����� ���� ��Ŷ
void mpResChatMessage(CPacket* packet, INT64 accountNo, WCHAR* id, WCHAR* nickname, WORD msgLen, WCHAR* msg);


#endif
