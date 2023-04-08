#include <cstddef>
#include <cstring>
#include <map>
#include <string>
#include <sys/types.h>
#include <vector>
namespace utility {
int incrementParse(char const *str, char const *sep);

const char *getFileExt(const char *filename);

void sigintHandler(int dummy);

std::string readSocket(int fd, char *buffer, size_t buffer_size);

ssize_t writeSocket(int fd, char *buffer, size_t size);

ssize_t writeFileSocket(int fd, int other_fd);
} // namespace utility