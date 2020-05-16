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

void vlog(int level, const char *format, va_list vl) {
  u64 millis = timerGet();
  if (level <= cfgLogLevel) {
    fprintf(logFile, "[%7llu.%03llu] [%s] ",
            millis / 1000, millis % 1000, LOG_LEVEL_NAMES[level]);
    vfprintf(logFile, format, vl);
    fprintf(logFile, "\n");
    fflush(logFile);
  }
}

void log(int level, const char *format, ...) {
  va_list vl;
  va_start(vl, format);
  vlog(level, format, vl);
  va_end(vl);
}

void die(const char *format, ...) {
  va_list vl;
  va_start(vl, format);
  vlog(LOG_ERROR, format, vl);
  va_end(vl);
  exit(1);
}
