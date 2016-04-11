#ifndef LOGFILE_H
#define LOGFILE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <iostream>

using namespace std;

class Logger
{
public:
	Logger();
	~Logger();
private:
	char filename[256];
	FILE* out;
public:
	void CreateFile(const char* filename);
	void Log(const char* contant);
private://屏蔽函数
	Logger(const Logger&);
	Logger&operator = (const Logger&);

};

#endif
