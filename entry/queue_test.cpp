#include <MultiThreadQueue.hpp>
#include <iostream>
#include <thread>
#include <unistd.h>

using namespace std;
static MultiThreadQueue<int> q(false);
static int result[10];
void pull_print(int i) {
    usleep(100);
    // cout << i << endl;
    int x = *(q.pull());
    result[i] = x;
}
int main() {
    int a = 0;
    int b = 1;
    int c = 2;
    q.push(a);
    q.push(b);

    cout << *q.pull() << endl;
    return 0;
}