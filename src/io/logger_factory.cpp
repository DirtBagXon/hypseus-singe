#include "logger_factory.h"
#include "logger_console.h"

ILogger *LoggerFactory::GetInstance(LoggerType type)
{
	ILogger *pInstance = 0;

	switch (type)
	{
	default:
		pInstance = NullLogger::GetInstance();
		break;
	case CONSOLE:
		pInstance = ConsoleLogger::GetInstance();
		break;
	}

	return pInstance;
}
