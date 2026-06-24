#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <stdexcept>
#include <sys/socket.h>

#include "Socket.hpp"
#include "utils.hpp"

Socket::Socket(void) { std::cout << "Default constructor called" << std::endl; }

Socket::Socket(std::string name): _name(name) {
    std::cout << "Name constructor called" << std::endl;
}

Socket::Socket(const Socket &src) {
  std::cout << "Copy constructor called" << std::endl;
  *this = src;
}

Socket &Socket::operator=(const Socket &other) {
  std::cout << "Copy assignment operator called" << std::endl;
  if (this != &other) {
    _name = other._name;
    _listen_fd = other._listen_fd;
    _addr = other._addr;
  }
  return (*this);
}

Socket::~Socket() { std::cout << "Destructor called" << std::endl; }

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

int     Socket::getSocketFd(void) {
    return (_listen_fd);
}

void    Socket::printSocket(void) {
    std::cout << BOLD_GREEN << "{==== SOCKET " << _name << " ====} " << endofline;
    std::cout << "Family: " << addr.sin_family << std::endl;          // 2 = AF_INET
    std::cout << "Port:   " << ntohs(addr.sin_port) << std::endl;     // ntohs! → 8080
    std::cout << "IP:     " << inet_ntoa(addr.sin_addr) << std::endl; // "0.0.0.0"
}
