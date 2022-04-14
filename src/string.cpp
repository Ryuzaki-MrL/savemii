#include "string.hpp"

bool replace(string &str, const string &from, const string &to) {
    size_t start_pos = str.find(from);
    if (start_pos == string::npos)
        return false;
    str.replace(start_pos, from.length(), to);
    return true;
}

string decodeXMLEscapeLine(string xmlString) {
    replace(xmlString, "&quot;", "\"");
    replace(xmlString, "&apos;", "'");
    replace(xmlString, "&lt;", "<");
    replace(xmlString, "&gt;", ">");
    replace(xmlString, "&amp;", "&");
    return xmlString;
}