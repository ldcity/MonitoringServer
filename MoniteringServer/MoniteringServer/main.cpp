#include "MonitoringLanServer.h"
#include "CrashDump.h"

lib::CrashDump crashDump;

MonitoringLanServer server;

int main()
{
	server.MonitoringLanServerStart();



	return 0;
}