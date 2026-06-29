#include <cstring>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
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

    // ALLOCATING THE SOCKET FOR THE LISTENING FD
    _listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (_listen_fd < 0)	
        throw std::runtime_error("Listening socket didn't initialize properly");
    else
        std::cout << "LISTENING SOCKET " << _listen_fd << " IS OPERATIONNAL";

    // SETTING OPTIONS TO LISTENING SOCKET
    ret = setsockopt(_listen_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    if (ret != 0)
        throw std::runtime_error("Listening socket didn't set properly");

    // SETTING UP ADDRESS STRUCTURE
    memset(&_addr, 0, sizeof(sockaddr_in));
    _addr.sin_family = AF_INET;
    _addr.sin_port = htons(port);
    _addr.sin_addr.s_addr = INADDR_ANY;

    // BINDING THE LISTENING SOCKET TO ADDRESS STRUCT
    ret = bind(_listen_fd, reinterpret_cast<sockaddr *>(&_addr), sizeof(sockaddr));
    if (ret != 0) {
        std::cout << "BINDING FAILURE\n\n" << std::endl;
        throw std::runtime_error("Binding failed.");
    } else {
        std::cout << "BINDING SUCCESS\n\n" << std::endl;
    }

    // LISTENING TO THE SOCKET STREAM
    ret = listen(_listen_fd, BACK_LOG);
    if (ret != 0)
        std::cout << "Listening on FD " << _listen_fd << " failed miserably" << std::endl;
    else
        std::cout << "LISTENING SUCCESS on port : " << PORT << std::endl;
	
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
