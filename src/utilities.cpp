#include "utilities.hpp"
#include <cstddef>
#include <iostream>
#include <sys/types.h>
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

map<string, string> utility::parseHeader(const string &str) {
    map<string, string> res;
    return res;
}

vector<string> utility::splitString(const string &str, const string &sep,
                                    ssize_t time) {
    vector<string> res;
    for (size_t i = 0; i < str.length(); ++i) {
    }
    return res;
}

const char *utility::getFileExt(const char *filename) {
    const char *dot = strrchr(filename, '.');
    if (!dot || dot == filename)
        return "";
    return dot;
}

void utility::sigintHandler(int dummy) { std::cerr << "test sign" << endl; }