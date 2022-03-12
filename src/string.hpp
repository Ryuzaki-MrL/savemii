#include <string>
extern "C" {
	#include "common/fs_defs.h"
	#include "savemng.h"
}
using namespace std;

char *replace_str(char *str, char *orig, char *rep);
bool StartsWith(const char *a, const char *b);
string string_format(const string fmt, ...);