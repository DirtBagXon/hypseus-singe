#ifndef LOGGER_H
#define LOGGER_H

#include <string>
using namespace std;

class ILogger
{
public:
	virtual void DeleteInstance() = 0;

	virtual void Log(const string &strText) = 0;
protected:
	virtual ~ILogger();
};

// doesn't actually do anything, useful for testing
class NullLogger : public ILogger
{
	friend class LoggerFactory;

public:
	// we want the null logger to be creatable directly without using the factory, so that
	//  our tests don't need to rely on ConsoleLogger
	static ILogger *GetInstance();

	void DeleteInstance();

	void Log(const string &strText);
private:
	NullLogger();
};

#endif // LOGGER_H
