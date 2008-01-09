/*
 *  homedir.h
 */

#ifndef HOMEDIR_H
#define HOMEDIR_H

#include <string>	// STL strings, useful to prevent buffer overrun
#include "homedir.h"
#include "mpo_fileio.h"

using namespace std;	// for STL string to compile without problems ...

class homedir
{
public:
	//Constructor/destructors
	homedir();

	//Properties
	string get_homedir();
	void set_homedir(const string &s);
	string get_romfile(const string &s);
	string get_ramfile(const string &s);
	string get_framefile(const string &s);

	// Searches homedir for a filename indicated by 'fileName' and if it doesn't find it,
	//  searches the application directory for the same filename (if 'bFallback' is true).
	// Returns the found path,
	//  or an empty string if neither location yielded the file.
	string find_file(string fileName, bool bFallback = true);

private:
	void make_dir(const string &dir);
	
	//Private members	
	string m_appdir;				//Directory the app was launched from
	string m_homedir;				//"Home" directory to search first (defaults to appdir)
};

extern homedir g_homedir;	// our global game class.  Instead of having every .cpp file define this, we put it here.

#endif
