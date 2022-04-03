#include "string.hpp"

char *replace_str(char *str, char *orig, char *rep) {
    static char buffer[4096];
    char *p;

    if (!(p = strstr(str, orig))) // Is 'orig' even in 'str'?
        return str;

    strncpy(buffer, str, p - str); // Copy characters from 'str' start to 'orig' st$
    buffer[p - str] = '\0';

    sprintf(buffer + (p - str), "%s%s", rep, p + strlen(orig));

    return buffer;
}

bool StartsWith(const char *a, const char *b) {
    if (strncmp(a, b, strlen(b)) == 0) return 1;
    return 0;
}