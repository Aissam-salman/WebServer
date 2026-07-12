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
#include <fcntl.h>

#include "Socket.hpp"
#include "utils.hpp"

// ==== ~TORS ====

// DEFAULT SOCKET: 0.0.0.0:0 WITH A ZEROED SOCKADDR
Socket::Socket(void): _host("0.0.0.0"), _port(0) { std::memset(&_addr, 0, sizeof(sockaddr_in)); }

// NAMED SOCKET: 0.0.0.0:0 WITH A ZEROED SOCKADDR
Socket::Socket(std::string name): _name(name), _host("0.0.0.0"), _port(0) {
    std::memset(&_addr, 0, sizeof(sockaddr_in));
}

// COPY CTOR
Socket::Socket(const Socket &src) {
  *this = src;
}

// COPY ASSIGNMENT
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

// DESTRUCTOR
Socket::~Socket() {}

// TURN THIS SOCKET INTO A LISTENER ON _HOST:PORT (SOCKET->SETSOCKOPT->BIND->LISTEN)
void Socket::setSocket(int port) {
    int ret = 0;
    int yes = 1;
    struct addrinfo hints;
    struct addrinfo *res = NULL;

    _port = port;

    // step 0: let getaddrinfo build a ready-to-bind IPv4/TCP address from _host
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE | AI_NUMERICHOST;

    // getaddrinfo wants the port as a string
    std::ostringstream oss;
    oss << port;
    std::string port_str = oss.str();

    // NULL node = any local interface (INADDR_ANY); otherwise bind to _host
    const char *node = _host.empty() ? NULL : _host.c_str();

    ret = getaddrinfo(node, port_str.c_str(), &hints, &res);
    if (ret != 0)
        throw std::runtime_error(std::string("getaddrinfo: ") + gai_strerror(ret));

    // step 1: create the socket endpoint
    _listen_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

    if (_listen_fd < 0) {
        freeaddrinfo(res);
        throw std::runtime_error("Listening socket didn't initialize properly");
    }

    // non-blocking + close-on-exec (one F_SETFL call per the subject rule)
    fcntl(_listen_fd, F_SETFL, O_NONBLOCK | FD_CLOEXEC);

#if DEBUG == 1
    std::cout << "LISTENING SOCKET " << _listen_fd << " IS OPERATIONNAL" << endofline;
#endif

    // step 2: SO_REUSEADDR so a quick restart avoids "address already in use"
    ret = setsockopt(_listen_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    if (ret != 0) {
        close(_listen_fd);
        freeaddrinfo(res);
        throw std::runtime_error("Listening socket didn't set properly");
    }

    // step 3: bind the fd to the concrete host:port
    ret = bind(_listen_fd, res->ai_addr, res->ai_addrlen);
    if (ret != 0) {
        close(_listen_fd);
        freeaddrinfo(res);
        std::cout << "BINDING FAILURE\n\n" << std::endl;
        throw std::runtime_error(std::string("Binding failed.") + strerror(errno));
    }
#if DEBUG == 1
    std::cout << "BINDING SUCCESS\n\n" << std::endl;
#endif

    // keep a copy of the bound sockaddr for printSocket(), then free the list
    std::memcpy(&_addr, res->ai_addr, res->ai_addrlen);
    freeaddrinfo(res);

    // step 4: mark the socket passive (BACK_LOG = max queued pending connections)
    ret = listen(_listen_fd, BACK_LOG);
    if (ret != 0) {
        close(_listen_fd);
        throw std::runtime_error("Listening on socket failed");
    }
#if DEBUG == 1
    std::cout << "LISTENING SUCCESS on port : " << port << std::endl;
#endif
}

// HOST/PORT ACCESSORS
void            Socket::setHost(std::string host) { _host = host; }
void            Socket::setPort(int port) { _port = port; }
std::string     Socket::getHost(void) const { return (_host); }
int             Socket::getPort(void) const { return (_port); }

// THE LISTENING FD
int     Socket::getSocketFd(void) {
    return (_listen_fd);
}

// DEBUG-PRINT THE RESOLVED SOCKET (CONFIG VALUES + BOUND ADDRESS)
void    Socket::printSocket(void) {
    std::cout << BOLD_GREEN << "{==== SOCKET " << _name << " ====} " << endofline;
    std::cout << "Config host: " << _host << std::endl;
    std::cout << "Config port: " << _port << std::endl;
    std::cout << "Family: " << _addr.sin_family << std::endl;
    std::cout << "Port:   " << ntohs(_addr.sin_port) << std::endl;
    std::cout << "IP:     " << inet_ntoa(_addr.sin_addr) << std::endl;
}
