#include "savemng.h"
#include <cstdarg>
#include <memory>
#include <stdexcept>
#include <string>

using namespace std;

bool replace(string &str, const string &from, const string &to);
string decodeXMLEscapeLine(string xmlString);

template<typename... Args>
auto string_format(const string &format, Args... args) -> string {
    int size_s = snprintf(nullptr, 0, format.c_str(), args...) + 1; // Extra space for '\0'
    if (size_s <= 0) { throw runtime_error("Error during formatting."); }
    auto size = static_cast<size_t>(size_s);
    unique_ptr<char[]> buf(new char[size]);
    snprintf(buf.get(), size, format.c_str(), args...);
    return string(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
}
