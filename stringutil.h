#ifndef __STRINGUTIL_H__
#define __STRINGUTIL_H__

/* Returns true iff "s" has "suffix" as a suffix */
bool endsWith(const char *s, const char *suffix);

/* Trims the whitespace ( \t\n) from both ends. Modifies the right end. Returns a pointer inside the given string. */
char* trim(char *s);

/* Removes at most one set of doublequotes from both ends. Modifies the right end. Returns a pointer inside the given string.
 * Does nothing and returns s if the quotes don't match. */
char* unquote(char *s);

/* Splits the string at the first occurrence of separator. Replaces the separator char with '\0'.
 * Returns a pointer to the character following the separator.
 * If there are no occurrences of the separator, returns null and leaves s unchanged. */
char* split(char *s, char separator);

#endif

