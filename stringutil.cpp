#include <assert.h>
#include <ctype.h>
#include <regex.h>
#include <string.h>
#include "stringutil.h"

bool endsWith(const char *s, const char *suffix) {
  if (!s || !suffix) {
    return false;
  }
  int len = strlen(s);
  int lens = strlen(suffix);
  if (lens >  lens) {
    return false;
  }
  return !strncmp(s + len - lens, suffix, lens);
}

char* trim(char *s) {
  while (isspace(*s)) {
    s++;
  }
  char *t = s + strlen(s) - 1;
  while ((t > s) && isspace(*t)) {
    t--;
  }
  *(t + 1) = '\0';
  return s;
}

char* unquote(char *s) {
  int len = strlen(s);
  if (len >= 2) {
    char *last = s + strlen(s) - 1;
    if (*s == '"' && *last == '"') {
      s++;
      *last = '\0';
    }
  }
  return s;
}

char* split(char *s, char separator) {
  char *t = strchr(s, separator);
  if (!t) {
    return NULL;
  }
  *t = '\0';
  return t + 1;
}

bool isFen(const char *s) {
  regex_t r;

  assert(!regcomp(&r, "([a-z1-8]+/){7}[a-z1-8]+ [bw] [-kq]+ [-a-h1-8]+ [0-9]+ [0-9]+", REG_EXTENDED | REG_ICASE | REG_NOSUB));
  return !regexec(&r, s, 0, NULL, 0);
}
