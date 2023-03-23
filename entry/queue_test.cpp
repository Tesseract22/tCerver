#include <MultiThreadQueue.hpp>
#include <iostream>
#include <thread>
#include <unistd.h>

using namespace std;
MultiThreadQueue<int> q;

void pull_print() {
    sleep(1);
    cout << std::this_thread::get_id() << ": " << q.pull() << endl;
}
int main() {

    thread s[10];
    for (int i = 0; i < 20; ++i) {
        q.push(i);
    }

    for (int i = 0; i < 10; ++i) {
        thread t(pull_print);
        s[i] = std::move(t);
    }

    for (int i = 20; i < 40; ++i) {
        q.push(i);
    }

    for (int i = 0; i < 10; ++i) {
        s[i].join();
    }
    return 0;
}