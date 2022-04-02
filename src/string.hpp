#ifndef _STRING_HPP
#define _STRING_HPP
#include <cstring>
#include <string>
#include <memory>
#include "savemng.hpp"
using namespace std;

char *replace_str(char *str, char *orig, char *rep);
bool StartsWith(const char *a, const char *b);
void decodeXMLEscapeLine(std::string& xmlString);

template<typename... Args>
std::string string_format(const std::string &format, Args... args) {
    int size_s = std::snprintf(nullptr, 0, format.c_str(), args...) + 1; // Extra space for '\0'
    auto size  = static_cast<size_t>(size_s);
    auto buf   = std::make_unique<char[]>(size);
    std::snprintf(buf.get(), size, format.c_str(), args...);
    return (buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
}

static inline void ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](char8_t ch) {
        return !std::isspace(ch);
    }));
}

static inline void rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](char8_t ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

static inline void trim(std::string &s) {
    ltrim(s);
    rtrim(s);
}

static inline void replaceAll(std::string& str, const std::string& oldSubstr, const std::string& newSubstr) {
    size_t pos = 0;
    while ((pos = str.find(oldSubstr, pos)) != std::string::npos) {
        str.replace(pos, oldSubstr.length(), newSubstr);
        pos += newSubstr.length();
    }
}

#endif