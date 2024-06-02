#pragma warning(disable :4996)

#include <wchar.h>
#include <stdlib.h>
#include <stack>

#include "TextParser.h"

TextParser::TextParser() : buffer(NULL), parsingTxt(NULL), parsingStart(NULL), blocksTxt(NULL), _size(0) {}


// ��, ����, ����, �ּ� ó��
bool TextParser::SkipNoneCommand()
{
	wchar_t* initBuffer = buffer;					// ���Ͽ��� �о�� ����
	blocksStart = blocksTxt;
	parsingStart = parsingTxt;						// ���� �Ľ̵� ������
	

	// ���� ���� ������ ������ �ݺ�
	while (*buffer != L'\0')
	{
		// ����, �� ���� (���ʹ� �ܾ� ������ ���� �ʿ�)
		if (*buffer == L' ' || *buffer == L'\t')
			buffer++;

		// // �ּ� ����
		else if (*buffer == L'/' && *(buffer + 1) == L'/')
		{
			// ���� ���ö����� �ݺ� (// �ּ��� ���ٸ� ó���ϱ� ����)
			while (*buffer != L'\n' && *buffer != L'\r')
				buffer++;
		}

		// /* �ּ� ����
		else if (*buffer == L'/' && *(buffer + 1) == L'*')
		{
			buffer += 2;
			// */ �ּ� ���ö����� �ݺ�
			while (*buffer != L'*' || *(buffer + 1) != L'/')
				buffer++;

			buffer += 2;
		}
		else
		{
			*parsingTxt = *buffer;
			*blocksTxt = *buffer;

			buffer++;
			parsingTxt++;
			blocksTxt++;
		}
	}
	*parsingTxt = L'\0';
	*blocksTxt = L'\0';

	// �޸� ��ȯ(free)�� ���� �ʱ� ������ ��ġ�� �̵�
	buffer = initBuffer;
	parsingTxt = parsingStart;
	blocksTxt = blocksStart;

	return true;
}

void TextParser::split(const wchar_t* szName, wchar_t** tokens, int tokensLen, int keyCnt, const wchar_t delimiter)
{
	int i = 0, j = 0;
	wchar_t temp[128];
	int szNameSize = wcslen(szName);

	wmemcpy_s(temp, sizeof(temp) / sizeof(wchar_t), szName, szNameSize);

	temp[szNameSize] = L'\0';

	wchar_t* start = temp;
	int size = 0;

	while (temp[i])
	{
		
		if (temp[i] == delimiter)
		{
			temp[i] = L'\0';
			
			size = wcslen(start) + 1;

			wmemcpy_s(tokens[j], size, start, size);
			start = &temp[i + 1];
			j++;
		}
		i++;
	}

	size = wcslen(start) + 1;
	wmemcpy_s(tokens[j], size, start, size);
	
}



// ��� ���� ������ �Լ�
bool TextParser::SearchBlock(const wchar_t* block)
{
	// blocksTxt���� �ܾ��� ��� ( : { ~~~~~~ } ) �� ���̸� ����
	// blocksTxt���� �ܾ��� ��� ũ�⸸ŭ �ӽ� ���� ũ�⸸ŭ �����Ҵ� �Ŀ� ����
	// blocksTxt ���� & �ش� ��� ũ�⸸ŭ �����Ҵ�
	// blocksTxt�� �ӽ� ���� memcpy
	// blocksTxt ��ġ ������ġ�� �ʱ�ȭ

	std::stack<wchar_t> s;

	wchar_t* chpBuff;
	wchar_t chWord[256];

	int iLength;

	int i = 0;
	
	// {}, ��� �԰��� �´��� Ȯ�� (Ʋ���� false)
	while (blocksTxt[i])
	{
		if (blocksTxt[i] == L',') break;

		if (blocksTxt[i] == L'{')
		{
			// ��� ��ȣ ¦ ���߱� ( {, } )
			s.push(blocksTxt[i]);

		}

		if (blocksTxt[i] == L'}')
		{
			s.pop();
		}

		i++;
	}

	// ������ ������� ������ ��ȣ ¦�� �ȸ´� ���̹Ƿ� false
	if (!s.empty()) return false;

	// ��� ������������ �̵�
	while (*blocksTxt)
	{
		if (*blocksTxt == L'{') break;

		blocksTxt++;
	}

	// { ���� ��ġ
	blocksTxt++;

	// { �������� ù ���� ��ġ ã�� ( { �� ù���� ���̿� ���� ���� ���ڵ��� ���� �� ����)
	while (*blocksTxt)
	{
		if (*blocksTxt != L'\n' && *blocksTxt != L'\r' && *blocksTxt != L' ') break;

		blocksTxt++;
	}

	i = 0;

	// ����� �� ������ } �� ������ �������� ������ ũ�� ����
	int length = 0;
	while (blocksTxt[i] != L'}')
	{
		i++;
		length++;
	}

	// ��� ���� ������ �ӽ� ����
	wchar_t* tempTxt = (wchar_t*)malloc(sizeof(wchar_t) * (length + 1));

	i = 0;
	// ����� �� ������ } �� ������ ������ ����
	while (blocksTxt[i] != L'}')
	{
		tempTxt[i] = blocksTxt[i];
		i++;
	}

	tempTxt[i] = L'\0';

	// ���� ��� ���� �� �� ��� ũ�� �Ҵ�
	blocksTxt = blocksStart;
	free(blocksTxt);

	blocksTxt = (wchar_t*)malloc(sizeof(wchar_t) * (length + 1));

	// ��� �����ּ� �ٽ� ����
	blocksStart = blocksTxt;

	// �ӽù��ۿ� �ִ� ��� ����
	wmemcpy_s(blocksTxt, length + 1, tempTxt, length + 1);


	return true;
}

// .txt ���Ͽ��� �޾ƿ� ���ڿ����� �Ľ��ؼ� ���ϴ� �����͸� ����
/*
Example)
// ��������.txt
Game:
{
	nickname:"player"
	hp:200
	LV:1
}

// ����
int lv;
GetValue("Game.LV", &lv);  // lv�� ���� 1�� ��
*/
// ���� �׸��� �����͸� ����
bool TextParser::GetValue(const wchar_t* szName, int* ipValue)
{
	blocksTxt = parsingStart;

	wchar_t* chpBuff, chWord[256];
	int iLength;

	chpBuff = (wchar_t*)malloc(128 * sizeof(wchar_t));


	// . ��ū�� �����ϴ��� Ȯ�� -> . �� �������� key ���� ex) Network.Version -> 2��
	keyCnt = isExistedToken(szName) + 1;
	
	tokens = (wchar_t**)malloc(sizeof(wchar_t*) * keyCnt);

	for (int j = 0; j < keyCnt; j++)
	{
		tokens[j] = (wchar_t*)malloc(sizeof(wchar_t) * 16);
	}
	
	split(szName, tokens, sizeof(tokens), keyCnt, L'.');

	int i = 0;

	// ��ū���� �и��� �ܾ�� ��� value ã��
	while (i < keyCnt)
	{
		// key�� �ϳ��� �ٷ� value ã�� (������ ��ū�� block key�� �ƴ϶� �ܾ� �� ��ü�� key
		if (i == keyCnt - 1)
		{
			// ã���� �ϴ� �ܾ ���ö����� ��� ã�� ���̹Ƿ� while ������ �˻�
			while (GetNextWord(&chpBuff, &iLength))
			{
				// Word ���ۿ� ã�� �ܾ ����
				wmemset(chWord, 0, 256);
				wmemcpy(chWord, chpBuff, iLength);

				// ���ڷ� �Է� ���� �ܾ�� ������ �˻�
				if (0 == wcscmp(tokens[i], chpBuff))
				{
					// ������ �ٷ� �ڿ� :�� ã��
					if (GetNextWord(&chpBuff, &iLength))
					{
						wmemset(chWord, 0, 256);
						wmemcpy(chWord, chpBuff, iLength);

						if (0 == wcscmp(chpBuff, L":"))
						{
							// : ������ ������(�ܾ�) �κ��� ����
							if (GetNextWord(&chpBuff, &iLength))
							{
								wmemset(chWord, 0, 256);
								wmemcpy(chWord, chpBuff, iLength);

								// ã�� �ܾ ���������� ��ȯ �� ���� (���ڿ��� ��쵵 ����ؾ���)
								*ipValue = _wtoi(chWord);
								return true;
							}
							return false;
						}
					}
					return false;
				}
			}
		}
		// ��ū �и��ϸ鼭 ���� ��ū�� ��� ã�� ����
		else
		{
			while (GetNextWord(&chpBuff, &iLength))
			{
				// Word ���ۿ� ã�� �ܾ ����
				wmemset(chWord, 0, 256);
				wmemcpy(chWord, chpBuff, iLength);

				// ���ڷ� �Է� ���� �ܾ�� ������ �˻�
				if (0 == wcscmp(tokens[i], chpBuff))
				{
					// ������ �ٷ� �ڿ� :�� ã��
					if (GetNextWord(&chpBuff, &iLength))
					{
						wmemset(chWord, 0, 256);
						wmemcpy(chWord, chpBuff, iLength);

						if (0 == wcscmp(chpBuff, L":"))
						{
							// ��� ���ϱ�
							// ��� ã���� ���� ��ū���� �ѱ�� (true, blocksTxt ����� ����
							if (SearchBlock(tokens[i]))
							{
								i++;
								break;
							}
							else return false;
						}
						else return false;
					}
				}
			}
		}
	}

	free(chpBuff);

	return false;
}

bool TextParser::GetValue(const wchar_t* szName, wchar_t* wpValue)
{
	blocksTxt = parsingStart;

	wchar_t* chpBuff, chWord[256];
	int iLength;

	chpBuff = (wchar_t*)malloc(128 * sizeof(wchar_t));


	// . ��ū�� �����ϴ��� Ȯ�� -> . �� �������� key ���� ex) Network.Version -> 2��
	keyCnt = isExistedToken(szName) + 1;

	tokens = (wchar_t**)malloc(sizeof(wchar_t*) * keyCnt);

	for (int j = 0; j < keyCnt; j++)
	{
		tokens[j] = (wchar_t*)malloc(sizeof(wchar_t) * 16);
	}

	split(szName, tokens, sizeof(tokens), keyCnt, L'.');

	int i = 0;

	// ��ū���� �и��� �ܾ�� ��� value ã��
	while (i < keyCnt)
	{
		// key�� �ϳ��� �ٷ� value ã�� (������ ��ū�� block key�� �ƴ϶� �ܾ� �� ��ü�� key
		if (i == keyCnt - 1)
		{
			// ã���� �ϴ� �ܾ ���ö����� ��� ã�� ���̹Ƿ� while ������ �˻�
			while (GetNextWord(&chpBuff, &iLength))
			{
				// Word ���ۿ� ã�� �ܾ ����
				wmemset(chWord, 0, 256);
				wmemcpy(chWord, chpBuff, iLength);

				// ���ڷ� �Է� ���� �ܾ�� ������ �˻�
				if (0 == wcscmp(tokens[i], chpBuff))
				{
					// ������ �ٷ� �ڿ� :�� ã��
					if (GetNextWord(&chpBuff, &iLength))
					{
						wmemset(chWord, 0, 256);
						wmemcpy(chWord, chpBuff, iLength);

						if (0 == wcscmp(chpBuff, L":"))
						{
							// : ������ ������(�ܾ�) �κ��� ����
							if (GetNextWord(&chpBuff, &iLength))
							{
								wmemset(chWord, 0, 256);
								wmemcpy(chWord, chpBuff, iLength);

								// ã�� �ܾ ū����ǥ�� �ѷ��ο� �ִٸ�, ū����ǥ�� ����
								if (chWord[0] == L'"' && chWord[iLength - 1] == L'"')
								{
									chWord[iLength - 1] = L'\0';
									wcscpy_s(wpValue, wcslen(chWord), chWord + 1);
								}
								else
								{
									wcscpy_s(wpValue, wcslen(chWord) + 1, chWord);
								}

								return true;
							}
							return false;
						}
					}
					return false;
				}
			}
		}
		// ��ū �и��ϸ鼭 ���� ��ū�� ��� ã�� ����
		else
		{
			while (GetNextWord(&chpBuff, &iLength))
			{
				// Word ���ۿ� ã�� �ܾ ����
				wmemset(chWord, 0, 256);
				wmemcpy(chWord, chpBuff, iLength);

				// ���ڷ� �Է� ���� �ܾ�� ������ �˻�
				if (0 == wcscmp(tokens[i], chpBuff))
				{
					// ������ �ٷ� �ڿ� :�� ã��
					if (GetNextWord(&chpBuff, &iLength))
					{
						wmemset(chWord, 0, 256);
						wmemcpy(chWord, chpBuff, iLength);

						if (0 == wcscmp(chpBuff, L":"))
						{
							// ��� ���ϱ�
							// ��� ã���� ���� ��ū���� �ѱ�� (true, blocksTxt ����� ����
							if (SearchBlock(tokens[i]))
							{
								i++;
								break;
							}
							else return false;
						}
						else return false;
					}
				}
			}
		}
	}

	free(chpBuff);

	return false;
}

// . ��ū�� �����ϴ��� Ȯ��
int TextParser::isExistedToken(const wchar_t* chppBuffer)
{
	int cnt = 0;

	while (*chppBuffer)
	{
		if (*chppBuffer == L'.')
		{
			cnt++;
		}

		chppBuffer++;
	}

	
	return cnt;
}


// ���� �ܾ� ���
bool TextParser::GetNextWord(wchar_t** chppBuffer, int* ipLength)
{
	// szName�� . ��ū�� �������� �и� (���ڿ� �ȿ� . �� �־�� ����)
	// ��ū �и��� ���� �������� �ܾ� ���� ���� (Network.Version �� �ܾ� 2��)
	// �ܾ 1���� �ƴϸ� ��� ã�� �ݺ�
	// ù��° �ܾ���� �����ؼ� �ش� �ܾ �ؽ�Ʈ�� �ִ��� ã�� (GetNextWord) - blocksTxt
	// �ܾ ���� ���, ��� ���ϱ� (true, blocksTxt ��ġ�� ã�� �ܾ��� ��)
	// �ܾ ���� ���, ���ϱ� �ߴ� (false, break)
	// �ܾ� ������ :{ �� �ƴϰų� �ܾ� �������� },�� �ƴ� ���, �߸��� �ؽ�Ʈ (false, break)
	// blocksTxt���� �ܾ��� ��� ( : { ~~~~~~ } ) �� ���̸� ����
	// blocksTxt���� �ܾ��� ��� ũ�⸸ŭ �ӽ� ���� ũ�⸸ŭ �����Ҵ� �Ŀ� ����
	// blocksTxt ���� & �ش� ��� ũ�⸸ŭ �����Ҵ�
	// blocksTxt�� �ӽ� ���� memcpy
	// blocksTxt ��ġ ������ġ�� �ʱ�ȭ
	// �ܾ 1���� ��� �� ã�� �ٷ� key�� �����ϴ� value�� ã��
	// �ش� �� �ؽ�Ʈ�� �ִ��� ã�� (GetNextWord) - blocksTxt
	// �ܾ ���� ���, key�� �����ϴ� value�� ã�� (true, blocksTxt ��ġ�� ã�� �ܾ��� ��)
	// blocksTxt ���� �ٷ� ������ : �� �־�� �� (������ �߸��� �ؽ�Ʈ! false)
	// : ���� ��ġ���� GetNextWord! (true�� �� ã�ƿ� ��)
	// �� ��ã�ƿ��� ����

	*ipLength = 0;

	// ù ���� ���ö����� �ݺ�
	while (*blocksTxt == 0x0a || *blocksTxt == 0x0d || *blocksTxt == L' ')
	{
		blocksTxt++;
	}

	int i = 0;

	if (blocksTxt[i] == L':')
	{
		(*ipLength)++;
		i++;
	}
	else
	{
		// ���� �ܾ��� ���̸� ������� �ݺ���
		while (blocksTxt[i])
		{
			// : �̰ų� ������ ���, �ܾ� ����
			if (blocksTxt[i] == L':' || blocksTxt[i] == 0x0a || blocksTxt[i] == 0x0d || blocksTxt[i] == L' ')
			{
				break;
			}

			(*ipLength)++;
			i++;

		}
	}

	// NULL�̸� �ܾ� ��ã�� �� ���������� �� �Ŵϱ� false
	if (blocksTxt[i] == NULL) return false;

	*chppBuffer = (wchar_t*)malloc(sizeof(wchar_t) * (*ipLength + 1));

	int j = 0;

	if (blocksTxt[j] == L':')
	{
		(*chppBuffer)[j] = *blocksTxt;
		blocksTxt++;
		j++;
	}
	else
	{
		while (*blocksTxt)
		{
			// : �̰ų� ������ ���, �ܾ� ����
			if (*blocksTxt == L':' || *blocksTxt == 0x0a || *blocksTxt == 0x0d)
			{
				break;
			}

			(*chppBuffer)[j] = *blocksTxt;

			blocksTxt++;
			j++;
		}
	}
	(*chppBuffer)[j] = L'\0';

	return true;
}



// ���� ��ü �б�
void TextParser::LoadFile(const wchar_t* txtName)
{

	FILE* fp = NULL;
	errno_t err = _wfopen_s(&fp, txtName, L"rb");
	if (err != 0 || fp == NULL)
	{
		wprintf(L"�ؽ�Ʈ ���� �б⿡ �����Ͽ����ϴ�\n");
		return;
	}

	// ���� ��ü ũ��
	fseek(fp, 0, SEEK_END);
	_size = ftell(fp);

	rewind(fp);

	buffer = (wchar_t*)malloc(sizeof(wchar_t) * (_size + 1));
	parsingTxt = (wchar_t*)malloc(sizeof(wchar_t) * (_size + 1));
	blocksTxt = (wchar_t*)malloc(sizeof(wchar_t) * (_size + 1));


	// ���� ������ �о��
	fread(buffer, sizeof(wchar_t), _size, fp);
	buffer[_size] = L'\0';

	// ��, ����, ����, �ּ� ����
	SkipNoneCommand();
}


TextParser::~TextParser()
{
	if (fp != NULL)
		fclose(fp);
	
	if (buffer != NULL)
		free(buffer);
	
	if (parsingTxt != NULL)
		free(parsingTxt);

	if (blocksTxt != NULL)
		free(blocksTxt);

	for (int i = 0; i < keyCnt; i++)
	{
		if (tokens[i] != NULL)
		{
			free(tokens[i]);
		}
	}

	if (tokens != NULL) free(tokens);

}