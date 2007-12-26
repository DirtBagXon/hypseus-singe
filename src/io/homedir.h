/*
 *  homedir.h
 *  daphne
 *
 *  Created by Derek Stutsman on 2/19/05.
 *  Copyright 2005 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef HOMEDIR_H
#define HOMEDIR_H

#include <string>	// STL strings, useful to prevent buffer overrun
#include "homedir.h"

using namespace std;	// for STL string to compile without problems ...

enum private_dir
{
	pdRoms,
	pdRams,
	pdFramefiles
};

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
private:
	void make_dir(const string &dir);
	
	//Private members	
	string m_appdir;				//Directory the app was launched from
	string m_homedir;				//"Home" directory to search first (defaults to appdir)
	
	//Internal-use methods
	string find_file(private_dir fileType, string fileName);
	
};

extern homedir g_homedir;	// our global game class.  Instead of having every .cpp file define this, we put it here.

#endif
