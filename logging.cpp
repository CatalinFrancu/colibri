#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "configfile.h"
#include "logging.h"
#include "timer.h"

static const char *LOG_LEVEL_NAMES[] = { "", "ERROR", "WARNING", "INFO", "DEBUG" };
static FILE *logFile;

void logInit(const char *fileName) {
  if (!strcmp(fileName, "stdout")) {
    logFile = stdout;
  } else if (!strcmp(fileName, "stderr")) {
    logFile = stderr;
  } else {
    logFile = fopen(fileName, "at");
  }
  assert(logFile);
  timerReset();
}

void log(int level, const char *format, ...) {
  if (level <= cfgLogLevel) {
    va_list vl;
    fprintf(logFile, "[%10llu] [%s] ", timerGet(), LOG_LEVEL_NAMES[level]);
    va_start(vl, format);
    vfprintf(logFile, format, vl);
    va_end(vl);
    fprintf(logFile, "\n");
    fflush(logFile);
  }
}
