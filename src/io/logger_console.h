#ifndef FILE_LOGGER_H
#define FILE_LOGGER_H

#include "logger.h"
#include "conout.h"

// logs to the console
class ConsoleLogger : public ILogger
{
	friend class LoggerFactory;
public:
	static ILogger *GetInstance();
	void DeleteInstance();

	void Log(const string &strText);
private:
	ConsoleLogger();
};


#endif // FILE_LOGGER_H
