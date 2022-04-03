#include <memory>
#include <string>
#include <stdarg.h>
extern "C" {
#include "savemng.h"
}
using namespace std;

char *replace_str(char *str, char *orig, char *rep);
bool StartsWith(const char *a, const char *b);

std::string string_format(const std::string fmt_str, ...);
