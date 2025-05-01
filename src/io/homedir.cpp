/*
 *  homedir.cpp
 */

#include "config.h"

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

homedir g_homedir; // our main homedir class (statically allocated to minimize
                   // risk of memory leak)

// no arguments because this is statically allocated
homedir::homedir()
{
    m_appdir  = "."; // our current directory must be our app directory, a '.'
    m_homedir = "."; // using curdir is a sensible default for the constructor
    m_romdir  = "";
    m_ramdir  = "";
}

string homedir::get_homedir() { return m_homedir; }
string homedir::get_romdir() { return m_romdir; }
string homedir::get_ramdir() { return m_ramdir; }

// helper function
void homedir::make_dir(const string &dir)
{
#ifdef WIN32
    CreateDirectory(dir.c_str(), NULL); // create directory if it does not
                                        // already exist
#else
    unsigned int oldumask = umask(022);
    mkdir(dir.c_str(), 0777); // create directory if it does not already exist
    umask(oldumask);
#endif
}

void homedir::set_homedir(const string &s)
{
    m_homedir = s;

    // create writable directories if they don't exist
    make_dir(m_homedir);
    make_dir(m_homedir + "/logs");
    make_dir(m_homedir + "/midi");
    make_dir(m_homedir + "/fonts");
    make_dir(m_homedir + "/bezels");
    make_dir(m_homedir + "/screenshots");

    if (m_ramdir.empty())
        make_dir(m_homedir + "/ram");

    if (m_romdir.empty())
        make_dir(m_homedir + "/roms");
}

void homedir::set_romdir(const string &s)
{
    m_romdir = s;

    while (!m_romdir.empty() && m_romdir.back() == '/') {
        m_romdir.pop_back();
    }

    make_dir(m_romdir);
}

void homedir::set_ramdir(const string &s)
{
    m_ramdir = s;

    while (!m_ramdir.empty() && m_ramdir.back() == '/') {
        m_ramdir.pop_back();
    }

    make_dir(m_ramdir);
}

string homedir::get_romfile(const string &s)
{
    string romDir = "roms/";
    bool fallback = true;

    if (!m_romdir.empty()) {
        romDir = m_romdir + "/";
        fallback = false;
    }

    return find_romfile(romDir + s, fallback);
}

string homedir::get_ramfile(const string &s)
{
    string ramDir = "ram/";

    if (!m_ramdir.empty())
        ramDir = m_ramdir + "/";

    return m_ramdir.empty() ? m_homedir + "/" + ramDir + s : ramDir + s;
}

string homedir::get_framefile(const string &s)
{
    // Framefiles may be passed as a fully-qualified path.  If so, see if it
    // exists first before trying the directories.
    if (mpo_file_exists(s.c_str())) {
        return s;
    } else {
        return find_file("framefile/" + s, true);
    }
}

string homedir::find_file(string fileName, bool bFallback)
{
    string strFile = fileName;
    string result  = "";

    result = m_homedir + "/" + strFile;

    if (bFallback && !mpo_file_exists(result.c_str())) {
        result = m_appdir + "/" + strFile;
    }

    return result;
}

string homedir::find_romfile(string fileName, bool bFallback)
{
    string strFile = fileName;
    string result  = "";

    result = m_romdir.empty() ? m_homedir + "/" + strFile : strFile;

    if (bFallback && !mpo_file_exists(result.c_str())) {
        result = m_appdir + "/" + strFile;
    }

    return result;
}

void homedir::create_dirs(const string &s) {

    size_t dir = s.find_last_of("/");
    string p = s.substr(0, ++dir);

    for (size_t i = 0; i < p.length(); i++) {
        if (p[i] == '/') {
            string sub = p.substr(0, i);
            if (!mpo_file_exists(sub.c_str())) {
                if (m_ramdir.empty()) {
                    make_dir(m_homedir + "/" + sub);
                } else {
                    make_dir(sub);
                }
            }
        }
    }
}
