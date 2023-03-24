#include <MultiThreadQueue.hpp>
#include <iostream>
#include <thread>
#include <unistd.h>

using namespace std;
static MultiThreadQueue<int> q(true);
static int result[10];
void pull_print(int i) {
    sleep(1);
    // cout << i << endl;
    int x = *(q.pull());
    result[i] = x;
}
int main() {

    thread s[10];

    for (int i = 0; i < 20; ++i) {
        q.push(i);
    }

    for (int i = 0; i < 10; ++i) {
        thread t(pull_print, i);
        cout << i << endl;
        s[i] = std::move(t);
    }

    for (int i = 20; i < 40; ++i) {
        q.push(i);
    }
    // cout << "\n\n";
    // sleep(2);
    for (int i = 0; i < 10; ++i) {
        s[i].join();
        cout << result[i] << endl;
    }

    return 0;
}