#include <MultiThreadQueue.hpp>
#include <iostream>

using namespace std;
int main() {
    MultiThreadQueue<int> q;
    q.push(0);
    q.push(1);
    q.push(2);
    q.push(3);
    q.push(4);
    cout << q.pull() << endl;
    cout << q.pull() << endl;
    cout << q.pull() << endl;
    cout << q.pull() << endl;
    cout << q.pull() << endl;

    q.push(2);
    q.push(3);
    q.push(4);
    q.push(2);
    q.push(3);
    q.push(4);
    q.push(2);
    q.push(3);
    q.push(4);
    q.push(2);
    q.push(3);
    q.push(4);

    cout << q.pull() << endl;
    cout << q.pull() << endl;
    cout << q.pull() << endl;
    cout << q.pull() << endl;
    cout << q.pull() << endl;

    return 0;
}