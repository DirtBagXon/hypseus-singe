#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <string>
#include <list>

using namespace std;

#define TEST_CHECK(a) TestFrameWork::DoTest(#a, a, __LINE__, __FILE__)

#define TEST_REQUIRE(a) TestFrameWork::DoTest(#a, a, __LINE__, __FILE__); if (!a) return

#define TEST_CHECK_EQUAL(a,b) TestFrameWork::DoTestEqual(a, b, __LINE__, __FILE__)

#define TEST_CHECK_NOT_EQUAL(a,b) TestFrameWork::DoTestNotEqual(a, b, __LINE__, __FILE__)

struct entry_s
{
	string strDesc;	// description
	unsigned int uLine;	// line number
	string strFile;	// source file name
	string strTestCase;	// name of the test case
};

class TestFrameWork
{
public:

	static int DoSummary();

	static void DoTest(const string &strDescription, const void *pResult, unsigned int uLine,
		const string &strSourceFile);

	static void DoTest(const string &strDescription, bool bResult, unsigned int uLine,
		const string &strSourceFile);

	template <class T1, class T2> static void DoTestEqual(T1, T2, unsigned int uLine,
		const string &strSourceFile);

	template <class T1, class T2> static void DoTestNotEqual(T1, T2, unsigned int uLine,
		const string &strSourceFile);

private:

	static list <entry_s> m_lPassed;
	static list <entry_s> m_lFailed;

};

extern string g_strTestCaseName;

// this will automatically run a test case
#define TEST_CASE(name) \
class name; \
class name	\
{	\
public:	\
		name() { g_strTestCaseName = #name ; run_test(); }	\
	void run_test();	\
};	\
name name ; \
void name::run_test()

#endif // TEST_FRAMEWORK_H
