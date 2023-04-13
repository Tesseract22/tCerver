#include "HTTPResponse.hpp"
#include "HTTPUnit.hpp"
#include "Scheduler.hpp"
#include <TCPServer.hpp>
#include <coroutine>
#include <csignal>
#include <fstream>
#include <ios>
#include <string>
#define TRUE 1
#define FALSE 0
#define PORT 80

HTTP::HTTPResponse *test(HTTP::HTTPRequest *request) {
    HTTP::HTTPResponseText *response = new HTTP::HTTPResponseText;
    response->body = "This is header";
    response->headers["Content-Length"] =
        std::to_string(response->body.length());
    return response;
}

using namespace std;

int main(int argc, char *argv[]) {

    // Scheduler::start(10);
    auto f = fstream("../logs/logs.txt", ios_base::app);
    f << "sever start" << endl;
    HTTPUnit http;
    http.bindUrl("/test", test);
    TCPServer server(std::move(http), 2, f);
    server.serverStart();
}