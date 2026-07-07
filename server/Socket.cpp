#include <cstring>
#include <cerrno>
#include <iostream>
#include <sstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <stdexcept>

#include "Socket.hpp"
#include "utils.hpp"

Socket::Socket(void): _host("0.0.0.0"), _port(0) { std::memset(&_addr, 0, sizeof(sockaddr_in)); }

Socket::Socket(std::string name): _name(name), _host("0.0.0.0"), _port(0) {
    std::memset(&_addr, 0, sizeof(sockaddr_in));
}

Socket::Socket(const Socket &src) {
  *this = src;
}

Socket &Socket::operator=(const Socket &other) {
  if (this != &other) {
    _name = other._name;
    _host = other._host;
    _port = other._port;
    _listen_fd = other._listen_fd;
    _addr = other._addr;
  }
  return (*this);
}

Socket::~Socket() {}

// Turns this Socket into a passive (listening) endpoint on _host:port.
// It runs the four classic server syscalls in order:
//     socket()  ->  setsockopt()  ->  bind()  ->  listen()
// After this returns, _listen_fd is an fd that accept() can pull connections
// from. On any failure it closes what it opened and throws, so a half-built
// socket is never left behind.
void Socket::setSocket(int port) {
    int ret = 0;
    int yes = 1;                    // "true" value for setsockopt below
    struct addrinfo hints;          // what KIND of address we want getaddrinfo to build
    struct addrinfo *res = NULL;    // OUT: linked list of matching addresses (we use the 1st)

    _port = port;

    // --- STEP 0: describe the address we want, then let getaddrinfo build it -
    // Rather than hand-filling a sockaddr_in, we ask getaddrinfo() to produce a
    // ready-to-bind sockaddr from (_host, port). hints filters what it returns.
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;                       // IPv4 only → res->ai_addr is a sockaddr_in
    hints.ai_socktype = SOCK_STREAM;                 // TCP (stream), not UDP
    hints.ai_flags = AI_PASSIVE | AI_NUMERICHOST;    // PASSIVE: address is for bind() (a server);
                                                     // NUMERICHOST: _host must already be a numeric
                                                     // IP like "0.0.0.0" — no DNS lookup is done.

    // getaddrinfo wants the port as a STRING ("8090"), so convert the int.
    std::ostringstream oss;
    oss << port;
    std::string port_str = oss.str();

    // NULL node = "any local interface" (INADDR_ANY); otherwise bind to _host.
    const char *node = _host.empty() ? NULL : _host.c_str();

    ret = getaddrinfo(node, port_str.c_str(), &hints, &res);
    if (ret != 0)   // note: getaddrinfo has its own error codes, hence gai_strerror (not errno)
        throw std::runtime_error(std::string("getaddrinfo: ") + gai_strerror(ret));

    // --- STEP 1: socket() — create the endpoint --------------------------
    // Uses the family/type/protocol getaddrinfo resolved. Returns an fd (int).
    _listen_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (_listen_fd < 0) {
        freeaddrinfo(res);          // free the list getaddrinfo malloc'd before bailing
        throw std::runtime_error("Listening socket didn't initialize properly");
    }
#if DEBUG == 1
    std::cout << "LISTENING SOCKET " << _listen_fd << " IS OPERATIONNAL" << endofline;
#endif

    // --- STEP 2: setsockopt(SO_REUSEADDR) — allow quick restart ----------
    // Without this, restarting the server right after it closed often fails
    // with "Address already in use" because the port sits in TIME_WAIT.
    // SO_REUSEADDR lets us re-bind that address immediately.
    ret = setsockopt(_listen_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    if (ret != 0) {
        close(_listen_fd);          // clean up the fd we just opened
        freeaddrinfo(res);
        throw std::runtime_error("Listening socket didn't set properly");
    }

    // --- STEP 3: bind() — claim the address:port -------------------------
    // Attaches the fd to the concrete host:port from getaddrinfo. This is the
    // step that fails with EACCES on privileged ports (<1024) without root,
    // or EADDRINUSE if something else already holds the port.
    ret = bind(_listen_fd, res->ai_addr, res->ai_addrlen);
    if (ret != 0) {
        close(_listen_fd);
        freeaddrinfo(res);
        std::cout << "BINDING FAILURE\n\n" << std::endl;
        // strerror(errno) turns the failure into a readable reason
        // (e.g. "Permission denied", "Address already in use").
        throw std::runtime_error(std::string("Binding failed.") + strerror(errno));
    }
#if DEBUG == 1
    std::cout << "BINDING SUCCESS\n\n" << std::endl;
#endif

    // Keep a copy of the bound sockaddr for printSocket(), THEN free the list.
    // After this point res is gone, so nothing below may touch it.
    std::memcpy(&_addr, res->ai_addr, res->ai_addrlen);
    freeaddrinfo(res);

    // --- STEP 4: listen() — mark the socket as passive -------------------
    // Flips the socket from "can connect out" to "accepts incoming". BACK_LOG
    // is the max number of not-yet-accepted connections the kernel will queue.
    ret = listen(_listen_fd, BACK_LOG);
    if (ret != 0) {
        close(_listen_fd);
        throw std::runtime_error("Listening on socket failed");
    }
#if DEBUG == 1
    std::cout << "LISTENING SUCCESS on port : " << port << std::endl;
#endif
}

void            Socket::setHost(std::string host) { _host = host; }
void            Socket::setPort(int port) { _port = port; }
std::string     Socket::getHost(void) const { return (_host); }
int             Socket::getPort(void) const { return (_port); }

int     Socket::getSocketFd(void) {
    return (_listen_fd);
}

void    Socket::printSocket(void) {
    std::cout << BOLD_GREEN << "{==== SOCKET " << _name << " ====} " << endofline;
    std::cout << "Config host: " << _host << std::endl;                // parsed from listen
    std::cout << "Config port: " << _port << std::endl;                // parsed from listen
    std::cout << "Family: " << _addr.sin_family << std::endl;          // 2 = AF_INET
    std::cout << "Port:   " << ntohs(_addr.sin_port) << std::endl;     // ntohs! → 8080
    std::cout << "IP:     " << inet_ntoa(_addr.sin_addr) << std::endl; // "0.0.0.0"
}
