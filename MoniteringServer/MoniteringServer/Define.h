#ifndef __SERVER_DEFINE__
#define __SERVER_DEFINE__

#define ID_MAX_LEN			20
#define NICKNAME_MAX_LEN	20
#define MSG_MAX_LEN			64

#define TIMEOUT				5000

#define MAX_PACKET_LEN		348

#define PACKET_CODE			0x56
#define KEY					0xa9

#pragma pack(push, 1)
// LAN Header
struct LANHeader
{
	// Payload Len
	unsigned short len;
};

// Net Header
struct NetHeader
{
	unsigned char code;
	short len;
	unsigned char randKey;
	unsigned char checkSum;
};

#pragma pack(pop)

#endif // !__SERVER_DEFINE__
