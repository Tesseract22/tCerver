
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

    // EPoll(NULL, s, NULL, NULL, NULL);

    return 0;
}