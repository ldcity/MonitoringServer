#ifndef __TEXT_PARSER__
#define __TEXT_PARSER__

#include <stdio.h>

class TextParser
{
private:
	wchar_t* buffer;					// �ؽ�Ʈ ����
	wchar_t* parsingTxt;
	wchar_t* parsingStart;
	wchar_t* blocksStart;				// ���� ���� ��ġ
	wchar_t** tokens;				// . ��ū �и��� ���ڿ�
	wchar_t* blocksTxt;				// ���(���) ����
	int keyCnt;						// tokens ����
	unsigned long _size;			// �ؽ�Ʈ ��ü ũ��
	FILE* fp;
public:
	TextParser();

	bool SearchBlock(const wchar_t*  blockName);

	void split(const wchar_t* szName, wchar_t** tokens, int tokensLen, int keyCnt, const wchar_t delimiter);

	// ��, ����, ����, �ּ� ó��
	bool SkipNoneCommand();

	// . ��ū�� �����ϴ��� Ȯ��
	int isExistedToken(const wchar_t* chppBuffer);

	// ���� �׸��� �����͸� ����
	bool GetValue(const wchar_t* szName, int* ipValue);

	bool GetValue(const wchar_t* szName, wchar_t* wpValue);

	// ���� �ܾ� ���
	bool GetNextWord(wchar_t** chppBuffer, int* ipLength);

	// ���� ��ü �б�
	void LoadFile(const wchar_t* txtName);

	~TextParser();
};

#endif // !__TEXT_PARSER__