#ifndef __LOGGING_H__
#define __LOGGING_H__

#define LOG_ERROR 1
#define LOG_WARNING 2
#define LOG_INFO 3
#define LOG_DEBUG 4

/* Initializes logging to the given absolute fileName. The special values "stdout" and "stderr" are allowed. */
void logInit(const char *fileName);

/* Logs a message with the given severity level. Takes a printf-style format string and variable number of arguments. */
void log(int level, const char *format, ...);

/* Logs an error message and terminates the program. */
void die(const char *format, ...);

#endif
