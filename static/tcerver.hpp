#include <condition_variable>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <mutex>
#include <netinet/in.h>
#include <ostream>
#include <sstream>
#include <string>
#include <sys/epoll.h>
#include <thread>
#include <unordered_map>
#include <vector>
// HTTPResponse.hpp
typedef std::map<std::string, std::string> s_map_t;

namespace HTTP {
enum ResponseType {
    fail = -1,
    text = 0,
    file = 1,
};
class HTTPResponse {
  public:
    s_map_t headers;
    std::string status = "200 OK";
    std::string http_ver = "HTTP/1.1";
    ResponseType type = text;
    virtual int sendData(int socket_fd) = 0;
    virtual ~HTTPResponse() {}
};
class HTTPRequest {
  public:
    HTTPRequest() = default;
    HTTPRequest(char *path_arg,
                std::map<std::string_view, std::string_view> header_arg,
                std::map<std::string_view, std::string_view> args_arg,
                char *body_arg)
        : path(path_arg), headers(header_arg), args(args_arg), body(body_arg) {}
    std::string path;
    std::map<std::string_view, std::string_view> headers;
    std::map<std::string_view, std::string_view> args;
    const char *body;
};

class HTTPResponseText : public HTTPResponse {
  public:
    HTTPResponseText() : HTTPResponse() {
        type = text;
        headers.insert({"Content-Type", "text/plain"});
    }
    std::string body;
    int sendData(int socket_fd);
};

class HTTPResponseFile : public HTTPResponse {

  public:
    HTTPResponseFile() : HTTPResponse() { type = file; }
    std::string path_to_file;
    int fd = -1;
    size_t file_size;
    int sendData(int socket_fd);
    // ~HTTPResponseFile() { close(fd); }
};

HTTPResponse *defaultPage(HTTPRequest *request);
HTTPResponse *defaultFileFounder(HTTPRequest *request);
HTTPResponse *notFoundHandler(HTTPRequest *request);

extern const std::unordered_map<std::string, std::string> g_mime_type_map;
} // namespace HTTP

// HTTPUnit.hpp
class HTTPUnit {
  public:
    HTTPUnit();

    std::pair<HTTP::HTTPRequest *, HTTP::HTTPResponse *>
    parseRequest(std::string &raw_request);

    std::string dispatchResponseHeaders(HTTP::HTTPResponse *response);

  private:
    char *parseHeader(std::string &raw_request,
                      std::map<std::string_view, std::string_view> &headers);

    std::map<std::string,
             std::function<HTTP::HTTPResponse *(HTTP::HTTPRequest *)>>
        url_map_;
};
// MultiThreadQueue.hpp
template <typename T> class MultiThreadQueue {
  public:
    MultiThreadQueue();
    ~MultiThreadQueue();
    T &pull();
    void push(T &x);
    // void debug() {

    // }

  private:
    size_t resize();
    size_t size_;
    size_t curr_size_;
    std::mutex m_;
    std::condition_variable cond_;
    T *data_;
    size_t top_;  // represent the front element in the queue
    size_t tail_; // one past the last element in the queue
};

struct Task {
    std::string task;
    int socket_fd;
};

class SocketException : std::exception {
  public:
    explicit SocketException();
    SocketException(const char *msg) : msg_(msg) {}
    SocketException(const std::string &msg) : msg_(msg) {}
    virtual ~SocketException() noexcept {}
    virtual const char *what() const noexcept { return msg_.data(); }

  protected:
    std::string msg_;
};
// Epoll.hpp
enum Action {
    Open = 0b1000,
    PendingRead = 0b001,
    PendingClose = 0b010,
    PendingWrite = 0b100,
};

class EPoll {
  public:
    EPoll(std::mutex *m_, int master_socket_fd, std::vector<int> *sockt_vec,
          std::vector<std::mutex *> *mutex_vec,
          MultiThreadQueue<Task *> *task_q_);
    EPoll(EPoll &&other);
    EPoll(const EPoll &X) = default;

    void wait();

  private:
    void addSocket(int socket_fd_);
    void delSocket(int socket_fd_);
    void modSocket(int socket_fd_, int act);
    int getSocket(int socket_fd_);

    void lockSocket(int socket_fd);
    void unlockSocket(int socket_fd);

    std::mutex *m_;
    int master_socket_fd_;
    std::vector<std::mutex *> *mutex_vec_;
    int epoll_fd_;
    epoll_event temp_event_;
    // The socket file descriptor is directly used as indexes
    // The value in the vector represent the status of the socket
    std::vector<int> *socket_vec_;

    MultiThreadQueue<Task *> *task_q_;
};

// TCPServer.hpp
class TCPServer {
  public:
    // TCPServer(std::ostream &log = std::cout, std::ostream &err = std::cerr);
    TCPServer(HTTPUnit &&http, size_t listen_threads = 2,
              size_t parse_threads = 2, std::ostream &log_io = std::cout,
              std::ostream &err_io = std::cerr);
    // TCPServer();
    ~TCPServer();
    void serverStart();
    void serverStop();

    void logRequest(HTTP::HTTPRequest *request, HTTP::HTTPResponse *response);

    // void startPoll

  private:
    void waitParse(size_t id);
    void waitListen(size_t id);
    bool running_;

    std::ostream &log_io_;
    std::ostream &err_io_;

    /**
      @new
    */
    int socket_fd_;
    sockaddr_in addr_;
    int port_;

    MultiThreadQueue<Task *> task_q_;

    // An array of threads listening to requests, we are not responsible for
    // allocating and deallocating this
    std::vector<std::thread> listen_threads_;
    std::vector<std::thread> parse_threads_;
    // Any array of EPoll object. Each one correspond to one listen_threads_;

    std::vector<EPoll> epolls_;

    std::vector<int> sockets_;
    std::vector<std::mutex *> mutexes_;

    std::mutex epoll_m_;

    HTTPUnit http_;

    char resource_path_[100];
};
