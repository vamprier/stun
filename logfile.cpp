#include "logfile.h"


Logger::Logger()
{

}

Logger::~Logger()
{

}

void Logger::CreateFile(const char* f)
{
	memcpy(this->filename,f,strlen(f));
	out = fopen(this->filename,"wb");
	if(out != NULL)
	{
		fclose(out);
	}
}

void Logger::Log(const char* contant)
{
	bool isWriteLog = true;
	out = fopen(this->filename,"ab");
	if(out == NULL)
	{
		isWriteLog = false;
	}
	if( isWriteLog)
	{
		fwrite(contant,1,strlen(contant),out);
		fclose(out);
	}
}


