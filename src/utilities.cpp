#include "utilities.hpp"
#include <cstddef>
#include <iostream>
#include <iterator>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
using namespace std;
int utility::incrementParse(char const *str, char const *sep) {
    char const *i = str;
    char const *sep_i = sep;
    int start = 0;
    int curr = 0;
    while (*i) {
        if (*i == *sep_i) {
            if (sep_i == sep) {
                start = curr;
            }
            sep_i++;

        } else if (*i == *sep) {
            sep_i = sep + 1;
            start = curr;
        } else {
            sep_i = sep;
        }
        if (*sep_i == '\0') {
            return start;
        }
        i++;
        curr++;
    }
    return -1;
}

const char *utility::getFileExt(const char *filename) {
    const char *dot = strrchr(filename, '.');
    if (!dot || dot == filename)
        return "";
    return dot;
}

void utility::sigintHandler(int dummy) { std::cerr << "test sign" << endl; }

string utility::readSocket(int fd, char *buffer, size_t buffer_size) {
    ssize_t bytes = 0;
    string result;

    while (read(fd, buffer, 1023) >= 0) {
        result += buffer;
        buffer[bytes] = '\0';
    }
    return result;
}

ssize_t utility::writeSocket(int fd, char *buffer, size_t size) {
    ssize_t bytes = 0;
    ssize_t total_bytes = 0;
    while (total_bytes < (ssize_t)size) {
        if ((bytes = send(fd, buffer, size, 0)) <= 0)
            break;
        total_bytes += bytes;
    }
    return total_bytes;
}

ssize_t utility::writeFileSocket(int fd, int other_fd) {
    off_t offset = 0;
    ssize_t bytes = 0;
    ssize_t total = 0;
    while ((bytes = (sendfile(fd, other_fd, &offset, 10240))) > 0) {
        total += bytes;
    }
    return total;
}

bool inline utility::startWith(const std::string &longer,
                               const std::string &prefix) {
    return equal(prefix.begin(), prefix.end(), longer.begin());
}