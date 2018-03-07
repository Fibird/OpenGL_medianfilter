#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

class CReader
{
public:
	CReader() { init(); };
	~CReader();
	/*
	* Read from a text file.
	* @param The text file name.
	* @return Content of the file.
	*/
	char *textFileRead(char *chFileName);
private:
	void init(void);
	FILE *_fp;
	char *_content;
	int _count;
};

