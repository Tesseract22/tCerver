#include "HTTPUnit.hpp"
#include <TCPServer.hpp>
#include <csignal>
#include <fstream>

#define TRUE 1
#define FALSE 0
#define PORT 80

using namespace std;
int main(int argc, char *argv[]) {

    auto f = fstream("../logs/logs.txt");
    f << "sever start" << endl;
    HTTPUnit http;
    TCPServer server(std::move(http), 1, 1, f);
    server.serverStart();
    std::signal(SIGINT, &TCPServer::sigintHandler);
}