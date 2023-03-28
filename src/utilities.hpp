#include <cstring>
#include <map>
#include <string>
#include <sys/types.h>
#include <vector>
namespace utility {
int incrementParse(char const *str, char const *sep);

std::map<std::string, std::string> parseHeader(const std::string &str);

std::vector<std::string> splitString(const std::string &str,
                                     const std::string &sep, ssize_t time = -1);

const char *getFileExt(const char *filename);
}; // namespace utility