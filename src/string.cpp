#include "string.hpp"

string string_format(const string fmt, ...) {
    int size=100;
    string str;
    va_list ap;

    while (1) {
        str.resize(size);
        va_start(ap, fmt);
        int n = vsnprintf(&str[0], size, fmt.c_str(), ap);
        va_end(ap);
   
        if (n > -1 && n < size) {
            str.resize(n); // Make sure there are no trailing zero char
            return str;
        }
        if (n > -1)
            size = n + 1;
        else
            size *= 2;
    }
}