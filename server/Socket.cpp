#include <cstring>
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

void Socket::setSocket(int port) {
    int ret = 0;
    int yes = 1;
    struct addrinfo hints;
    struct addrinfo *res = NULL;

    _port = port;

    // RESOLVING host + port INTO A sockaddr WE CAN bind()
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;                       // IPv4 only → res->ai_addr is a sockaddr_in
    hints.ai_socktype = SOCK_STREAM;                 // TCP
    hints.ai_flags = AI_PASSIVE | AI_NUMERICHOST;    // for bind(); host must be a numeric IP (no DNS)

    std::ostringstream oss;
    oss << port;
    std::string port_str = oss.str();

    const char *node = _host.empty() ? NULL : _host.c_str();

    ret = getaddrinfo(node, port_str.c_str(), &hints, &res);
    if (ret != 0)
        throw std::runtime_error(std::string("getaddrinfo: ") + gai_strerror(ret));

    // ALLOCATING THE SOCKET FOR THE LISTENING FD (using what getaddrinfo resolved)
    _listen_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (_listen_fd < 0) {
        freeaddrinfo(res);
        throw std::runtime_error("Listening socket didn't initialize properly");
    }
    std::cout << "LISTENING SOCKET " << _listen_fd << " IS OPERATIONNAL" << endofline;

    // SETTING OPTIONS TO LISTENING SOCKET
    ret = setsockopt(_listen_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    if (ret != 0) {
        close(_listen_fd);
        freeaddrinfo(res);
        throw std::runtime_error("Listening socket didn't set properly");
    }

    // BINDING THE LISTENING SOCKET TO THE RESOLVED ADDRESS
    ret = bind(_listen_fd, res->ai_addr, res->ai_addrlen);
    if (ret != 0) {
        close(_listen_fd);
        freeaddrinfo(res);
        std::cout << "BINDING FAILURE\n\n" << std::endl;
        throw std::runtime_error("Binding failed.");
    }
    std::cout << "BINDING SUCCESS\n\n" << std::endl;

    // KEEP A COPY OF THE BOUND ADDRESS FOR printSocket()
    std::memcpy(&_addr, res->ai_addr, res->ai_addrlen);
    freeaddrinfo(res);

    // LISTENING TO THE SOCKET STREAM
    ret = listen(_listen_fd, BACK_LOG);
    if (ret != 0) {
        close(_listen_fd);
        throw std::runtime_error("Listening on socket failed");
    }
    std::cout << "LISTENING SUCCESS on port : " << port << std::endl;
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
