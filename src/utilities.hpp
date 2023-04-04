#include <cstring>
#include <map>
#include <string>
#include <sys/types.h>
#include <vector>
namespace utility {
int incrementParse(char const *str, char const *sep);

const char *getFileExt(const char *filename);

void sigintHandler(int dummy);
} // namespace utility