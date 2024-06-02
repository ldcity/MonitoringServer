#include "Exception.h"

const char* SerializingBufferException::what() const noexcept
{
	// �α� ���� ���
	const wchar_t* logFolder = L"Exception";

	// �α� ������ ������ ����
	_wmkdir(logFolder);

	time_t now;
	struct tm timeinfo;
	time(&now);
	localtime_s(&timeinfo, &now);

	// �α� ���� ������ ���� �ð����� �����մϴ�.
	wchar_t logFileName[50];
	wcsftime(logFileName, sizeof(logFileName), L"Exception_%Y%m%d%H%M%S.txt", &timeinfo); // localTime�� &timeinfo�� �����մϴ�.

	// �α� ���� ���
	std::wstring logFilePath = std::wstring(logFolder) + L"/" + logFileName; // wstring���� �����մϴ�.

	// �α� ������ ���ϴ�.
	std::ofstream logFile(logFilePath, std::ios::app);

	if (logFile.is_open())
	{
		logFile << message << std::endl;
		logFile << "Serialize Method Name : " << GetMethodName() << std::endl;
		logFile << "Line : " << GetLine() << std::endl;
		logFile << "Serialize Buffer Address : " << std::showbase << std::hex << (uintptr_t)buffer << std::dec << std::endl;
		logFile.close();
	}
	else {
		std::cerr << "Failed to open log file." << std::endl;
	}

	return message;
}
