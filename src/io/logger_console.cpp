#include "logger_console.h"

void ConsoleLogger::DeleteInstance()
{
	delete this;
}

void ConsoleLogger::Log(const string &strText)
{
	printline(strText.c_str());
}

ConsoleLogger::ConsoleLogger()
{
}

ILogger *ConsoleLogger::GetInstance()
{
	ConsoleLogger *pInstance = new ConsoleLogger();
	return pInstance;
}
