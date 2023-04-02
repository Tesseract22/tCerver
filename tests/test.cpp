#include <TCPServer.hpp>
#include <cerrno>
#include <cstdio>
#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>
#define CHILD_NUM 2

int main() {
    pid_t children[CHILD_NUM];
    // pid_t server = fork();
    // if (server == 0) {
    //     execl("./server", "./server", NULL);
    // }
    char path[100];
    getcwd(path, 100);
    puts(path);
    for (int i = 0; i < CHILD_NUM; ++i) {
        children[i] = fork();
        if (children[i] == 0) {
            int succes = execl("/usr/bin/python3", "/usr/bin/python3",
                               "../tests/tests.py", NULL);
            if (succes < 0)
                printf("err: %i\n", errno);
            exit(1);
        }
    }
    int status;
    // waitpid(server, &status, 0);
    for (int i = 0; i < CHILD_NUM; ++i) {
        waitpid(children[i], &status, 0);
    }
}