#include "logger.h"

ILogger::~ILogger()
{
}

void NullLogger::DeleteInstance()
{
	delete this;
}

void NullLogger::Log(const string &strText)
{
	// do nothing, since this is a null logger
}

NullLogger::NullLogger()
{
}

ILogger *NullLogger::GetInstance()
{
	return new NullLogger();
}
