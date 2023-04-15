#include <condition_variable>
#include <coroutine>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <fcntl.h>
#include <fstream>
#include <functional>
#include <ios>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <netinet/in.h>
#include <new>
#include <optional>
#include <ostream>
#include <stdarg.h>
#include <stdexcept>
#include <string>
#include <string_view>
#include <sys/epoll.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>
#define INIT_SIZE 4
#define DEBUG_PRINT(info)                                                      \
    std::cerr << "[" << std::this_thread::get_id() << "] " << info << std::endl;
#define DEFAULT_PORT 80
#define LOG(...) __debug__(std::this_thread::get_id(), __LINE__, __VA_ARGS__);

template <typename T> class MultiThreadQueue {
  public:
    MultiThreadQueue();
    ~MultiThreadQueue();
    T &pull();
    void push(const T &x);
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

template <typename T> MultiThreadQueue<T>::~MultiThreadQueue() {
    delete[] data_;
}

template <typename T> MultiThreadQueue<T>::MultiThreadQueue() {
    size_ = INIT_SIZE;
    data_ = new T[INIT_SIZE]; // array of ptr
    curr_size_ = 0;
    top_ = 0;
    tail_ = 0;
}

template <typename T> size_t MultiThreadQueue<T>::resize() {
    T *old_data = data_;
    data_ = new T[size_ * 2];
    size_t new_i = 0;
    size_t old_i = top_;
    while (new_i < curr_size_) {
        data_[new_i++] = old_data[old_i++];
        old_i %= size_;
    }

    delete[] old_data; // element ptrs should remain intact

    top_ = 0;
    tail_ = new_i;
    size_ *= 2;
    return size_;
}

template <typename T> void MultiThreadQueue<T>::push(const T &x) {
    std::unique_lock<std::mutex> lock(m_);
    if (size_ == curr_size_) {
        // reshape
        // alloc more space
        resize();
    }
    data_[tail_++] = T(x);
    tail_ %= size_;
    curr_size_++;
    cond_.notify_all();
}

template <typename T> T &MultiThreadQueue<T>::pull() {
    std::unique_lock<std::mutex> lock(m_);

    cond_.wait(lock, [this] { return curr_size_ != 0; });
    T &ret = data_[top_++];
    top_ %= size_;
    curr_size_--;
    return ret;
}
class Scheduler {
  public:
    // friend class Task;

    Scheduler(size_t count)

        : queue_(
              std::make_shared<MultiThreadQueue<std::coroutine_handle<>>>()) {
        for (size_t i = 0; i < count; ++i) {
            threads_.emplace_back(&Scheduler::thread_start, this);
        }
        list_.push_back(this);
    }

    ~Scheduler() {
        for (auto &t : threads_) {
            t.join();
        }
    }

    Scheduler(const Scheduler &other) {
        threads_ = std::vector<std::thread>(0);
        queue_ = other.queue_;
        running_ = other.running_;
    }
    void enqueue(const std::coroutine_handle<> &handle) {
        queue_->push(handle);
    }

    bool await_ready() const noexcept { return false; }
    void await_suspend(std::coroutine_handle<> handle) { queue_->push(handle); }
    void await_resume() const noexcept {}

    void static SIGINT_HANDLER(int dummy) {
        for (auto s : list_) {
            s->running_ = false;
            for (auto &t : s->threads_) {
                s->enqueue(std::coroutine_handle<>());
            }
        }
    }

  private:
    // Scheduler() = default;
    static std::vector<Scheduler *> list_;
    void thread_start() {
        while (running_) {
            auto handle = queue_->pull();
            if (!running_)
                break;
            handle.resume();
            // DEBUG_PRINT(handle.done());
        }
    }
    volatile bool running_ = true;
    std::vector<std::thread> threads_;
    std::shared_ptr<MultiThreadQueue<std::coroutine_handle<>>> queue_;
    // static std::vector<Scheduler*>
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
namespace utility {
/**
 * @brief Incrementally parse the str once with the sep. This would find the
 * first match of `sep` in `str`, and return the start of the index. For
 * example, with `str`="key: value" and `sep`=": " will return 3
 *
 * @param str the str to be parsed
 * @param sep the targe to be matched in str
 * @return int. The index of the start of the first match. On not found, return
 * -1
 */
int incrementParse(char const *str, char const *sep);

const char *getFileExt(const char *filename);

void sigintHandler(int dummy);

std::string readSocket(int fd, char *buffer, size_t buffer_size);

ssize_t writeSocket(int fd, char *buffer, size_t size);

ssize_t writeFileSocket(int fd, int other_fd);

bool inline startWith(const std::string &longer, const std::string &prefix);
} // namespace utility
typedef std::map<std::string, std::string> s_map_t;
namespace HTTP {
struct HTTPBody {
    std::map<std::string, std::string> args;
    struct HTTPFile {
        std::string file_name;
        std::string name;
        std::string disposition;
        std::string_view bytes;
    };
    std::vector<HTTPFile> bytes;
};
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
    virtual std::variant<int, std::string> getBody() = 0;
    virtual ~HTTPResponse() {}
};
class HTTPRequest {
  public:
    HTTPRequest() = default;
    HTTPRequest(char *path_arg,
                std::map<std::string_view, std::string_view> header_arg,
                std::map<std::string, std::string> args_arg, char *body_arg)
        : path(path_arg), headers(header_arg), args(args_arg) {}
    std::string path;
    std::string method;
    std::string protocol;
    std::map<std::string_view, std::string_view> headers;
    std::map<std::string, std::string> args;
    HTTPBody body;
    std::string_view raw_body;
};

class HTTPResponseText : public HTTPResponse {
  public:
    HTTPResponseText() : HTTPResponse() {
        type = text;
        headers.insert({"Content-Type", "text/plain"});
        headers.insert({"Content-Length", "0"});
    }
    std::string body;
    std::variant<int, std::string> getBody();
};

class HTTPResponseFile : public HTTPResponse {

  public:
    HTTPResponseFile() : HTTPResponse() { type = file; }
    std::string path_to_file;
    int fd = -1;
    size_t file_size;
    std::variant<int, std::string> getBody();
    // ~HTTPResponseFile() { close(fd); }
};

HTTPResponse *defaultPage(HTTPRequest *request);
HTTPResponse *defaultFileFounder(HTTPRequest *request);
HTTPResponse *notFoundHandler(HTTPRequest *request);

extern const std::unordered_map<std::string, std::string> g_mime_type_map;
} // namespace HTTP
class HTTPUnit {
  public:
    HTTPUnit();

    std::pair<HTTP::HTTPRequest *, HTTP::HTTPResponse *>
    parseRequest(std::string &raw_request);
    std::string getResponseHeaders(HTTP::HTTPResponse *response);

    void bindUrl(
        const std::string &url,
        const std::function<HTTP::HTTPResponse *(HTTP::HTTPRequest *)> &func);

  private:
    void handleMethod(HTTP::HTTPRequest *request);
    char *dispatchHTTP(std::string &raw_request, HTTP::HTTPRequest *request);
    char *dispatchHeader(char *c_str,
                         std::map<std::string_view, std::string_view> &headers);
    /**
     * @brief parse the given url and return the key:val pairs contained.
     * Example: /index.html?aaa=bbb&xxx=yyy
      would return {"aaa":"bbb, "xxx":"yyy"}
     * @param url std::string&. Incorret formatted url would cause undefined
     behavior, though it would not throw
     * @return std::map<std::string, std::string> containing the args paris
     */
    std::map<std::string, std::string> parseUrl(std::string &url) noexcept;
    /**
     * @brief A helper function to parse url and
     * application/x-www-form-urlencoded.
     * See https://developer.mozilla.org/en-US/docs/Web/HTTP/Methods/POST
     *
     * @param c_str char* c_str.
     * @return std::map<std::string, std::string> containing the args paris
     */
    std::map<std::string, std::string> parseArgs(const char *c_str) noexcept;
    std::map<std::string,
             std::function<HTTP::HTTPResponse *(HTTP::HTTPRequest *)>>
        url_map_;
    std::vector<std::string>
    parseSemiColSeperated(const std::string &str) noexcept;
    HTTP::HTTPBody::HTTPFile parseFormDataInfo(const char *str) noexcept;
};

struct SameThreadResumer {
    std::vector<std::coroutine_handle<>> resumers;
    bool resuming;
    explicit SameThreadResumer(std::vector<std::coroutine_handle<>> &&r)
        : resumers(r), resuming(true) {}
    explicit SameThreadResumer() : resumers(), resuming(false) {}

    bool await_ready() const {
        for (auto &h : resumers)
            h.resume();
        return resuming;
    }
    void await_suspend(std::coroutine_handle<> handle) const noexcept {}
    void await_resume() const {}
};

template <typename T> class Task {
  public:
    // ~Task<T>() {}
    // Task<T>(std::cor) {}
    Task<T>(Task<T> &&other) noexcept
        : handle_(std::exchange(other.handle_, {})),
          result_(std::exchange(other.result_, {})),
          finished_(std::exchange(other.finished_, {})),
          m_(std::exchange(other.m_, {})) {}

    Task<T>(Task<T> &) = default;
    Task<T> &operator=(Task<T> &) = default;
    // https://www.bennyhuo.com/2022/03/11/cpp-coroutines-02-generator/

    struct promise_type {
        std::shared_ptr<std::mutex> m = std::make_shared<std::mutex>();
        std::shared_ptr<T> result = std::make_shared<T>();
        std::shared_ptr<bool> finished = std::make_shared<bool>(false);
        std::vector<std::coroutine_handle<>> others;
        Scheduler *scheduler = nullptr;
        std::suspend_never initial_suspend() noexcept {
            // DEBUG_PRINT("await schedule")
            return {};
        }
        std::suspend_never final_suspend() noexcept {
            m->lock();
            *finished = true;
            if (scheduler) {
                for (auto &o : others)
                    scheduler->enqueue(o);
            } else {
                for (auto &o : others)
                    o.resume();
            }
            // DEBUG_PRINT("enqueue");
            m->unlock();
            return {};
        }
        void unhandled_exception() {
            try {
                std::rethrow_exception(std::current_exception());
            } catch (const SocketException &e) {
                DEBUG_PRINT(e.what());
            }
        }

        Task get_return_object() {
            // std::cout << test << std::endl;
            return {std::coroutine_handle<promise_type>::from_promise(*this),
                    result, finished, m};
        }
        // void return_void() {}
        void return_value(T value) { *result = std::move(value); }
        std::suspend_always yield_value(T value) {
            m->lock();
            // resume "other" coroutine that are waiting us
            if (scheduler) {
                for (auto &o : others)
                    scheduler->enqueue(o);
            } else {
                for (auto &o : others)
                    o.resume();
            }
            others.clear();
            *result = std::move(value);
            m->unlock();
            return {};
        }
        template <typename T_>
        constexpr Task<T_> &await_transform(Task<T_> &task) const {
            return task;
        }
        Scheduler &await_transform(Scheduler &s) {
            scheduler = &s;
            return s;
        }
    };

    Task<T>(const std::coroutine_handle<promise_type> &h,
            const std::shared_ptr<T> &result,
            const std::shared_ptr<bool> &finished,
            const std::shared_ptr<std::mutex> &m) {
        handle_ = h;
        result_ = result;
        finished_ = finished;
        m_ = m;
        // std::cerr << m << std::endl;
    }

    bool constexpr await_ready() const { return false; }
    bool await_suspend(std::coroutine_handle<> handle) const noexcept {
        // std::cerr << m_ << std::endl;
        m_->lock();

        // DEBUG_PRINT("awaited ");
        if (*finished_) {
            m_->unlock();
            return false; // resumes current coroutine
        } else {
            handle_.promise().others.push_back(handle);
            m_->unlock();
            return true;
        }
    }
    T await_resume() const { return std::move(*result_); }

    std::coroutine_handle<promise_type> handle_;
    std::shared_ptr<T> result_ = nullptr;
    std::shared_ptr<bool> finished_ = nullptr;
    std::shared_ptr<std::mutex> m_ = nullptr;
};

template <> class Task<void> {
  public:
    // ~Task<T>() {}
    // Task<T>(std::cor) {}
    Task<void>() = default;
    Task<void>(Task<void> &&other) noexcept
        : handle_(std::exchange(other.handle_, {})),
          finished_(std::exchange(other.finished_, {})),
          m_(std::exchange(other.m_, {})) {}
    Task<void>(Task<void> &) = default;
    Task<void> &operator=(Task<void> &) = default;
    // https://www.bennyhuo.com/2022/03/11/cpp-coroutines-02-generator/

    struct promise_type {
        std::shared_ptr<std::mutex> m = std::make_shared<std::mutex>();
        std::shared_ptr<bool> finished = std::make_shared<bool>(false);
        std::vector<std::coroutine_handle<>> others;
        Scheduler *scheduler = nullptr;
        std::suspend_never initial_suspend() noexcept {
            // DEBUG_PRINT("await schedule")
            return {};
        }
        std::suspend_never final_suspend() noexcept {
            m->lock();
            *finished = true;
            for (auto &o : others) {
                scheduler->enqueue(o);
            }
            // DEBUG_PRINT("enqueue");
            m->unlock();
            return {};
        }
        void unhandled_exception() {
            try {
                std::rethrow_exception(std::current_exception());
            } catch (const SocketException &e) {
                std::cerr << e.what() << std::endl;
            }
        }
        Task<void> get_return_object() {
            // std::cout << test << std::endl;
            return {std::coroutine_handle<promise_type>::from_promise(*this),
                    finished, m};
        }
        // void return_void() {}
        void return_void() {}
        std::suspend_always yield_value() {
            // if (result.has_value())
            //     throw std::runtime_error("yield value not consumed");
            m->lock();
            for (auto &o : others)
                scheduler->enqueue(o);
            others.clear();
            m->unlock();
            return {};
        }

        Scheduler &await_transform(Scheduler &s) {
            scheduler = &s;
            return s;
        }

        template <typename T_>
        constexpr Task<T_> await_transform(Task<T_> task) const {
            return task;
        }
    };
    Task<void>(const std::coroutine_handle<promise_type> &h,
               const std::shared_ptr<bool> &finished,
               const std::shared_ptr<std::mutex> &m) {
        handle_ = h;
        finished_ = finished;
        m_ = m;
    }

    bool constexpr await_ready() const { return false; }
    bool await_suspend(std::coroutine_handle<> handle) const {

        // DEBUG_PRINT("awaited ");
        m_->lock();
        if (*finished_) {
            m_->unlock();
            return false; // resumes current coroutine
        } else {
            handle_.promise().others.push_back(handle);
            m_->unlock();
            return true;
        }
    }
    void await_resume() const {}

    std::coroutine_handle<promise_type> handle_;
    std::shared_ptr<bool> finished_;
    std::shared_ptr<std::mutex> m_;
};

// TCP server implementation

class TCPServer {
  public:
    TCPServer(HTTPUnit &&http, size_t listen_threads = 2,
              std::ostream &log_io = std::cout,
              std::ostream &err_io = std::cerr);
    ~TCPServer();
    void serverStart();
    void serverStop();

    void logRequest(HTTP::HTTPRequest *request, HTTP::HTTPResponse *response);
    void static SIGINT_HANDLER(int dummy);
    typedef int FileFd;
    struct Response {
        std::string str = "";
        FileFd fd = -1;
    };

  private:
    enum Status {
        Open = 0b1,
        PendingRead = 0b10,
        PendingClose = 0b100,
        Closed = 0,
    };
    class EPoll {
      public:
        EPoll(std::mutex &m_, int master_socket_fd, std::vector<int> &sockt_vec,
              std::vector<std::mutex *> &mutex_vec, Scheduler &s,
              const std::function<Task<Response>(std::string &)> &callback);
        EPoll(EPoll &&other) = default;
        EPoll(const EPoll &X) = delete;

        Task<void> wait(int dummy_fd);
        void stop();
        int epoll_fd_;

      private:
        void addSocket(int socket_fd_);
        void delSocket(int socket_fd_);
        // void modSocket(int socket_fd_, int act);
        // int getSocket(int socket_fd_);

        void lockSocket(int socket_fd);
        void unlockSocket(int socket_fd);

        std::mutex &m_;
        int master_socket_fd_;
        std::vector<std::mutex *> &mutex_vec_;
        std::vector<int> &socket_vec_;
        std::function<Task<Response>(std::string &)> handleReqeust_;
        epoll_event temp_event_;
        bool running_ = false;
        Scheduler &s_;
        // The socket file descriptor is directly used as indexes
        // The value in the vector represent the status of the socket
    };
    // std::function<Task<std::string>(TCPServer *, const std::string &)>
    //     protocol_;
    static std::vector<TCPServer *> servers_;
    int stop_pipe_[2];

    void waitParse(size_t id);
    void waitListen(size_t id);

    Task<TCPServer::Response> handleRequest(std::string &str);
    bool running_;

    std::ostream &log_io_;
    std::ostream &err_io_;

    /**
      @new
    */
    int socket_fd_;
    sockaddr_in addr_;
    int port_;

    // An array of threads listening to requests, we are not responsible for
    // allocating and deallocating this
    size_t listen_threads_;

    std::vector<int> sockets_;
    std::vector<std::mutex *> mutexes_;
    std::mutex epoll_m_;
    // Any array of EPoll object. Each one correspond to one listen_threads_;
    std::vector<EPoll> epolls_;

    HTTPUnit http_;

    char resource_path_[100];
    Scheduler s_;
};

static Scheduler __debug_s__(1);
template <class... Args>
Task<void> __debug__(std::thread::id id, size_t line, Args... args) {
    // std::cerr << __func__ << std::endl;
    co_await __debug_s__;
    std::cerr << "[" << std::hex << id << std::dec << "] ";
    std::cerr << "at line: " << line << ": ";
    (std::cerr << ... << args) << std::endl;
}