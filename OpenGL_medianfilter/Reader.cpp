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
			// 偏移到文件末尾
			fseek(_fp, 0, SEEK_END);
			// 计算文件的字节数
			_count = ftell(_fp);
			// 返回至文件开头位置
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
