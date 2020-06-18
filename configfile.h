#ifndef __CONFIGFILE_H__
#define __CONFIGFILE_H__
#include <string>
using namespace std;

extern int cfgEgtbChunks;
extern string cfgEgtbPath;
extern string cfgLogFile;
extern int cfgLogLevel;
extern int cfgQueryServerPort;
extern string cfgBookFile;
extern int cfgSaveEvery;

/* Loads options from an INI file. Exits on errors. */
void loadConfigFile(const char *fileName);

#endif
