#ifndef LOGGER_FACTORY_H
#define LOGGER_FACTORY_H

#include "logger.h"

class LoggerFactory
{
public:
	typedef enum
	{
		NULLTYPE,
		CONSOLE,
	} LoggerType;

	static ILogger *GetInstance(LoggerType type);
};

#endif // LOGGER_FACTORY_H
