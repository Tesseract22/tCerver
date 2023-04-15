#include "HTTPUnit.hpp"
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
using namespace std;
int main() {
    string s = "form-data; name=\"this is a test\"";
    regex reg("(.*?)(?:$|;)\\s?(?:name=\"(.*?)\"(?:$|;))?\\s?(?:filename=\"(.*?"
              ")\"(?:$|;))?");
    cmatch cm;
    regex_match(s.data(), cm, reg);
    cout << cm[1] << endl;
    cout << cm[2] << endl;
    // cout << cm[3] << endl;

    return 0;
}