#include "Reader.h"

CReader::~CReader()
{
}

char * CReader::textFileRead(char * chFileName)
{
	if (chFileName != NULL)
	{
		_fp = fopen(chFileName, "rt");
		if (_fp != NULL)
		{
			// ƫ�Ƶ��ļ�ĩβ
			fseek(_fp, 0, SEEK_END);
			// �����ļ����ֽ���
			_count = ftell(_fp);
			// �������ļ���ͷλ��
			rewind(_fp);
			if (_count > 0)
			{
				_content = (char*)malloc(sizeof(char) * (_count + 1));
				_count = fread(_content, sizeof(char), _count, _fp);
				_content[_count] = '\0';
			}
			fclose(_fp);
		}
	}
	return _content;
}

void CReader::init(void)
{
	_content = NULL;
	_count = 0;
}
