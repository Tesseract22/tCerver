#include <map>
#include <string>
#include <sys/types.h>
#include <vector>
namespace utility {
int incrementParse(char *str, char const *sep);

std::map<std::string, std::string> parseHeader(const std::string &str);

std::vector<std::string> splitByChar(const std::string &str,
                                     const std::string &sep, ssize_t time = -1);
}; // namespace utility