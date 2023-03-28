#include <iostream>
#include <utilities.hpp>
using namespace std;
int main() {
    char a[100] = "key: : val";
    char *aa = a;
    int i;

    cout << (i = utility::incrementParse(aa, ": ")) << '\n';
    aa[i] = '\0';
    cout << aa << endl;
    aa += i + 2;
    cout << aa << endl;

    return 0;
}