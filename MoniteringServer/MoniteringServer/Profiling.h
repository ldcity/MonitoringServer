#ifndef __PROFILING__
#define __PROFILING__


//---------------------------------------------------------------------------
// Profiling Program 
//---------------------------------------------------------------------------
#include <Windows.h>
#include <list>
#include <locale.h>

using namespace std;

#define PROFILE_CHECK
#define PROFILE_NUMMAX 30
#define PROFILE_SAMPLE_MAX 50
#define PROFILE_THREAD_MAX 500

class CSystemLog;

struct PROFILE_DATA
{
	long lFlag;						// ���������� ��� ����. (�迭�ÿ���)

	WCHAR szName[64];				// �������� ���� �̸�

	LARGE_INTEGER IStartTime;		// �������� ���� ���� �ð�
	__int64 iTotalTime;				// ��ü ���ð� ī���� Time. (��½� ȣ��Ƚ���� ������ ��� ����)
	__int64 iMin[2];				// �ּ� ���ð� ī���� Time. (�ʴ����� ����Ͽ� ���� / [0] �����ּ� [1] ���� �ּ� [2])
	__int64 iMax[2];				// �ִ� ���ð� ī���� Time. (�ʴ����� ����Ͽ� ���� / [0] �����ִ� [1] �����ֵ� [2])

	__int64 iCall;					// ���� ȣ�� Ƚ��
};

//////////////////////////////////////////////////////////////////////
// �� �����尡 ������ �� �������ϸ� �ʼ� TLS �ڷ��
//////////////////////////////////////////////////////////////////////
class CProfileThread
{
public:
	DWORD dwThreadID;
	PROFILE_DATA Data[PROFILE_SAMPLE_MAX];

	CProfileThread();
};

class CProfile
{
private:
	CProfile();
	~CProfile();
	DWORD tlsIndex;
	LARGE_INTEGER lFreqency;
	double fMicroFreqeuncy;

	list<CProfileThread*> ProfileList;

	CRITICAL_SECTION WriteSection;

public:
	static CProfile* GetInstance()
	{
		static CProfile Profile;
		return &Profile;
	}

	// �����带  ã�Ƴ���, �̸��� ã�Ƴ���
	void ProfileBegin(const WCHAR* _fName);
	void ProfileEnd(const WCHAR* _fName);

	// ��� �������� �����͸� ����Ѵ�.
	void ProfileOutText(const WCHAR* szFileName);
	void ProfileReset();

private:
	void ListExistAddCounter(CProfileThread* pThread, const WCHAR* _fName);
};

#ifdef PROFILE_CHECK
#define PRO_BEGIN(x)  CProfile::GetInstance()->ProfileBegin(x)
#define PRO_END(x)  CProfile::GetInstance()->ProfileEnd(x)
#define PRO_TEXT(x)	CProfile::GetInstance()->ProfileOutText(x)
#define PRO_RESET() CProfile::GetInstance()->ProfileReset()
#else
#define PRO_BEGIN(x)
#define PRO_END(x)
#endif

#endif // !__PROFILING__