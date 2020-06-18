#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "configfile.h"
#include "stringutil.h"

int cfgEgtbChunks;
string cfgEgtbPath;
string cfgLogFile;
int cfgLogLevel;
int cfgQueryServerPort;
string cfgBookFile;
int cfgSaveEvery;

void loadConfigFile(const char *fileName) {
  char path[1000];
  assert(getcwd(path, 1000));

  while (*path && !endsWith(path, "/colibri")) {
    char *lastSlash = strrchr(path, '/');
    if (lastSlash) {
      *lastSlash = '\0';
    }
  }
  strcat(path, "/");
  strcat(path, fileName);

  FILE *f = fopen(path, "r");
  assert(f);
  char line[1000];
  while (fgets(line, 1000, f)) {
    char *key = trim(line);
    if (*key && *key != ';') {
      // Split at the equal sign
      char *value = split(key, '=');
      assert(value);
      trim(key); // Should only be a right trim
      value = trim(value);
      value = unquote(value);
      if (!strcmp(key, "egtbChunks")) {
        cfgEgtbChunks = atoi(value);
      } else if (!strcmp(key, "egtbPath")) {
        cfgEgtbPath = string(value);
      } else if (!strcmp(key, "logFile")) {
        cfgLogFile = string(value);
      } else if (!strcmp(key, "logLevel")) {
        cfgLogLevel = atoi(value);
      } else if (!strcmp(key, "queryServerPort")) {
        cfgQueryServerPort = atoi(value);
      } else if (!strcmp(key, "bookFile")) {
        cfgBookFile = string(value);
      } else if (!strcmp(key, "saveEvery")) {
        cfgSaveEvery = atoi(value);
      }
    }
  }

  fclose(f);
}
