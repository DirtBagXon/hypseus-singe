#include "test_framework.h"

list<entry_s> TestFrameWork::m_lPassed;
list<entry_s> TestFrameWork::m_lFailed;

// the name of the current test case that's being run (so logging purposes)
string g_strTestCaseName;

#include <sstream>
#include <iostream>
using std::ostringstream;

int TestFrameWork::DoSummary()
{
	int iResult = 1;

	// only print failures so the list doesn't get too long
	if (!m_lFailed.empty())
	{
		for (list<entry_s>::const_iterator li = m_lFailed.begin();
			li != m_lFailed.end(); ++li)
		{
			cout << li->strFile << "(" << li->uLine << "): error" << 
				": test " << li->strDesc << " failed in '" << li->strTestCase << "'" << endl;
		}
	}
	// else all tests pass, return 0
	else
	{
		iResult = 0;
	}

	return iResult;
}

void TestFrameWork::DoTest(const string &strDescription, const void *pResult, unsigned int uLine,
		const string &strSourceFile)
{
	entry_s entry;

	entry.strDesc = strDescription;
	entry.strFile = strSourceFile;
	entry.uLine = uLine;
	entry.strTestCase = g_strTestCaseName;

	if (pResult != 0)
	{
		m_lPassed.push_back(entry);
	}
	else
	{
		m_lFailed.push_back(entry);
	}
}

void TestFrameWork::DoTest(const string &strDescription, bool bResult, unsigned int uLine,
		const string &strSourceFile)
{
	DoTest(strDescription, (const void *) bResult, uLine, strSourceFile);
}

template <class T1, class T2> void TestFrameWork::DoTestEqual(T1 val1, T2 val2, unsigned int uLine,
		const string &strSourceFile)
{
	ostringstream outstream;

	entry_s entry;
	entry.strFile = strSourceFile;
	entry.uLine = uLine;
	entry.strTestCase = g_strTestCaseName;

	outstream << val1 << "==" << val2;
	entry.strDesc = outstream.str();

	if (val1 == val2)
	{
		m_lPassed.push_back(entry);
	}
	else
	{
		m_lFailed.push_back(entry);
	}
}

template <class T1, class T2> void TestFrameWork::DoTestNotEqual(T1 val1, T2 val2, unsigned int uLine,
		const string &strSourceFile)
{
	ostringstream outstream;

	entry_s entry;
	entry.strFile = strSourceFile;
	entry.uLine = uLine;
	entry.strTestCase = g_strTestCaseName;

	outstream << val1 << "!=" << val2;
	entry.strDesc = outstream.str();

	if (val1 != val2)
	{
		m_lPassed.push_back(entry);
	}
	else
	{
		m_lFailed.push_back(entry);
	}
}

// Force template instantiation
// Anytime you need more templates instantated, add a dummy line here
void instantiator()
{
	unsigned int u = 0;
	unsigned char u8 = 0;
	TestFrameWork::DoTestEqual(1, 2, 0, "blah");
	TestFrameWork::DoTestEqual(u, 0, 0, "blah");
	TestFrameWork::DoTestEqual(true, true, 0, "blah");	// two bools
	TestFrameWork::DoTestEqual(u8, 0, 0, "blah");	// unsigned char, and int

	TestFrameWork::DoTestNotEqual(1, 2, 0, "blah");
	
}
