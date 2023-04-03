#include <EPoll.hpp>
#include <iostream>
#include <sys/epoll.h>
#include <sys/socket.h>
using namespace std;
#include <thread>

class test {
  public:
    test() { cout << epoll_create(1) << endl; }
};
int main() {
    int s = socket(AF_INET, SOCK_STREAM, 0);
    // EPoll(NULL, s, NULL, NULL, NULL);
    vector<test> v(2, test());

    return 0;
}