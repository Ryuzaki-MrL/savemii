#pragma once

#include "savemng.h"
#include <cstdarg>
#include <memory>
#include <stdexcept>
#include <string>

auto replace(std::string &str, const std::string &from, const std::string &to) -> bool;
auto decodeXMLEscapeLine(std::string xmlString) -> std::string;

template<typename... Args>
auto string_format(const std::string &format, Args... args) -> std::string {
    int size_s = std::snprintf(nullptr, 0, format.c_str(), args...) + 1; // Extra space for '\0'
    if (size_s <= 0) { throw std::runtime_error("Error during formatting."); }
    auto size = static_cast<size_t>(size_s);
    std::unique_ptr<char[]> buf(new char[size]);
    std::snprintf(buf.get(), size, format.c_str(), args...);
    return std::string(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
}
