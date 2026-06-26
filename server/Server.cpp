#include "Server.hpp"
#include "cgi/Cgi.hpp"
#include "utils.hpp"
#include <csignal>
#include <iostream>
#include <stdexcept>
#include <sys/poll.h>
#include <unistd.h>
#include <vector>

bool Server::_running = true;

// ==== ~TORS ====
Server::Server(void) : _name(""), _max_body_size(0) {
  std::cout << BOLD_CYAN << "Server Default constructor called" << RESET
            << std::endl;
}

Server::Server(std::string name) : _name(name), _max_body_size(-1) {
  std::cout << BOLD_CYAN << "Server Name constructor called" << RESET
            << std::endl;
}
Server::Server(const Server &src)
    : _name(src._name), _sockets_vector(src._sockets_vector), _locations_vector(src._locations_vector),
      _error_pages(src._error_pages), _max_body_size(src._max_body_size),
      _clients(src._clients) {
  std::cout << BOLD_CYAN << "Server Copy constructor called" << RESET
            << std::endl;
}

Server::~Server() {
  std::cout << BOLD_RED << "Server Destructor called" << RESET << std::endl;
}

// ==== GETTERS ====
void                  Server::setMaxBodySize(long size) { _max_body_size = size; }
void						      Server::setServerName(std::string name) { _name = name;}
std::vector<Location> &Server::getServerLocationsVector(void) { return (_locations_vector); }
std::string					Server::getServerName(void) {return (_name);}

std::vector<Location> &Server::getLocations(void) { return (_locations_vector); }

long                   Server::getMaxBodySize(void) const { return (_max_body_size); }
Location&             Server::getCurrentLocation(void) { return (_locations_vector.back());}
std::vector<Socket>   &Server::getSockets(void) { return (_sockets_vector); }

MapIntStr &Server::getErrorPages(void) { return (_error_pages); }
std::map<int, Client> &Server::getClients(void) { return (_clients); }



void  Server::addLocation(Location& new_location) {
  _locations_vector.push_back(new_location);
}

void  Server::addSocket(Socket& new_socket) {
  _sockets_vector.push_back(new_socket);
}

void  Server::addErrorPage(int code, std::string path) {
  _error_pages[code] = path;
}


// ==== OUTPUTS ====
void Server::printServer(void) {
  display("\n[ ==== GLOBAL INFOS ==== ]");
  display("SERVER NAME = " + _name);
  std::cout << "NBR OF SOCKETS = " << _sockets_vector.size() << std::endl;
  for (size_t i = 0; i < _sockets_vector.size(); i++) {
    std::cout << BOLD_CYAN << "LOCATION NUMBER " << i << endofline;
    _sockets_vector[i].printSocket();
    std::cout << endofline;
  }
  std::cout << endofline;
  for (size_t i = 0; i < _locations_vector.size(); i++) {
    std::cout << BOLD_CYAN << "LOCATION NUMBER " << i << endofline;
    _locations_vector[i].printLocation();
    std::cout << endofline;
  }

  std::cout << BOLD_MAGENTA << "[===========-------===========]" << endofline;
}

void Server::handle_sigint(int) {
  Server::_running = false;
  std::cerr << "SIGNAL RECEIVED" << std::endl;
}

void Server::run(void) {
  Socket socket1("SocketTest");
  socket1.setSocket(PORT);
  _sockets_vector.push_back(socket1);
  std::vector<std::string> ls;
  ls.push_back("python");

  signal(SIGINT, handle_sigint);
  std::vector<pollfd> poll_fds;

  std::vector<Socket>::iterator it = _sockets_vector.begin();
  std::vector<Socket>::iterator ite = _sockets_vector.end();
  for (; it != ite; it++) {
    pollfd pfd;
    pfd.fd = (*it).getSocketFd();
    pfd.events = POLLIN;
    pfd.revents = 0;
    poll_fds.push_back(pfd);
  }

  while (_running) {
    int ret = poll(&poll_fds[0], poll_fds.size(), TIMEOUT);
    if (ret == -1 && _running)
      throw std::runtime_error("Poll failed miserably");
    for (size_t i = 0; i < poll_fds.size(); i++) {
      int fd = poll_fds[i].fd;
      if (!poll_fds[i].revents)
        continue;
      if (_clients.count(fd) == 0) {
        // new client
        if (poll_fds[i].revents & POLLIN) {
          int client_fd = accept(fd, NULL, NULL);
          _clients[client_fd] =
              Client(client_fd, Request("8080", "0.0.0.0", "0000", "www"));
          poll_fds.push_back(_clients[client_fd].getPollfd());
        }
      } else {
        // already client
        Client &client = _clients[fd];
        if (poll_fds[i].revents & POLLIN) {
          bool isHere = client.handleRecv();
          if (!isHere) {
            close(fd);
            _clients.erase(fd);
            poll_fds.erase(poll_fds.begin() + i--);
          } else if (client.getStatus() == WRITTING) {
            client._request.parseRequest(client.getBufferRead());
            std::string resp;
            if (client._request.isCGI()) {
              Cgi cgi(ls, client._request);
              resp = cgi.run();
            } else {
              // resp = buildResp(client._request);
              resp = "HTTP/1.1 200 OK\r\nContent-Type: text/html; "
                     "charset=utf-8\r\nContent-Length: 53\r\nConnection: "
                     "keep-alive\r\n\r\n<!DOCTYPE html><html><body>Hello "
                     "world</body></html>";
            }
            client.setResponse(resp);
            poll_fds[i].events = POLLOUT;
          }
        } else if (poll_fds[i].revents & POLLOUT) {
          bool isDone = client.handleSend();
          if (!isDone) {
            close(fd);
            _clients.erase(fd);
            poll_fds.erase(poll_fds.begin() + i--);
          }
        } else if (poll_fds[i].revents & (POLLHUP | POLLERR)) {
          close(fd);
          _clients.erase(fd);
          poll_fds.erase(poll_fds.begin() + i--);
        }
      }
    }
  }
}
