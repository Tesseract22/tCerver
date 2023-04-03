#include "HTTPUnit.hpp"
#include <TCPServer.hpp>
#include <coroutine>
#include <csignal>
#include <fstream>
#include <ios>
#define TRUE 1
#define FALSE 0
#define PORT 80

using namespace std;
int main(int argc, char *argv[]) {

    auto f = fstream("../logs/logs.txt", ios_base::app);
    f << "sever start" << endl;
    HTTPUnit http;
    TCPServer server(std::move(http), 2, 2, f);
    server.serverStart();
    std::signal(SIGINT, &TCPServer::sigintHandler);
}