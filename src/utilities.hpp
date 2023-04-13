#include <cstddef>
#include <cstring>
#include <map>
#include <string>
#include <sys/types.h>
#include <vector>
namespace utility {
/**
 * @brief Incrementally parse the str once with the sep. This would find the
 * first match of `sep` in `str`, and return the start of the index. For
 * example, with `str`="key: value" and `sep`=": " will return 3
 *
 * @param str the str to be parsed
 * @param sep the targe to be matched in str
 * @return int. The index of the start of the first match. On not found, return
 * -1
 */
int incrementParse(char const *str, char const *sep);

const char *getFileExt(const char *filename);

void sigintHandler(int dummy);

std::string readSocket(int fd, char *buffer, size_t buffer_size);

ssize_t writeSocket(int fd, char *buffer, size_t size);

ssize_t writeFileSocket(int fd, int other_fd);

bool inline startWith(const std::string &longer, const std::string &prefix);
} // namespace utility