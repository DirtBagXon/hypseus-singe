/*
 *  homedir.cpp
 */

#include "homedir.h"
#include "mpo_fileio.h"
#include "conout.h"

#ifdef WIN32
// for CreateDirectory
#include <windows.h>
#endif

#ifdef UNIX
// for mkdir
#include <sys/stat.h>
#include <sys/types.h>
#endif

using namespace std;

homedir g_homedir;	// our main homedir class (statically allocated to minimize risk of memory leak)

// no arguments because this is statically allocated
homedir::homedir()
{
	m_appdir = ".";	// our current directory must be our app directory, so a '.' here is sufficient
	m_homedir = ".";	// using curdir is a sensible default for the constructor
}

string homedir::get_homedir()
{
	return m_homedir;
}

// helper function
void homedir::make_dir(const string &dir)
{
#ifdef WIN32
	CreateDirectory(dir.c_str(), NULL);	// create directory if it does not already exist
#else
	unsigned int oldumask = umask(022);
	mkdir(dir.c_str(), 0777);	// create directory if it does not already exist
	umask(oldumask);
#endif
}

void homedir::set_homedir(const string &s)
{
	m_homedir = s;
	
	// create writable directories if they don't exist
	make_dir(m_homedir);
	make_dir(m_homedir + "/ram");
	make_dir(m_homedir + "/roms");
	make_dir(m_homedir + "/framefile");
}

string homedir::get_romfile(const string &s)
{
	return find_file("roms/" + s, true);
}

string homedir::get_ramfile(const string &s)
{
	return find_file("ram/" + s, false);
}

string homedir::get_framefile(const string &s)
{
	//Framefiles may be passed as a fully-qualified path.  If so, see if it exists first before trying the directories.
	if (mpo_file_exists(s.c_str()))
	{
		return s;
	}
	else
	{
		return find_file("framefile/" + s, true);
	}
}

string homedir::find_file(string fileName, bool bFallback)
{
	string strFile = fileName;
	string result = "";

	// try homedir first
	result = m_homedir + "/" + strFile;
	
	// if file does not exist in home directory and we are allowed to fallback to app dir
	if (bFallback && !mpo_file_exists(result.c_str()))
	{
		result = m_appdir + "/" + strFile;
	}
	// else file either exists or we cannot fall back
	
	return result;
}
