#include "Server.hpp"
#include "Response.hpp"
#include "Cgi.hpp"
#include "config/configutils.hpp"
#include "utils.hpp"
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <sys/poll.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

bool Server::_running = true;

// ==== ~TORS ====
Server::Server(void)
    : _name(""), _error_pages(initErrorPages()), _max_body_size(0) {}

Server::Server(std::string name)
    : _name(name), _error_pages(initErrorPages()), _max_body_size(-1) {}
Server::Server(const Server &src)
    : _name(src._name), _sockets_vector(src._sockets_vector),
      _locations_vector(src._locations_vector), _error_pages(src._error_pages),
      _max_body_size(src._max_body_size), _clients(src._clients) {}

Server::~Server() {}

// ==== GETTERS ====
void Server::setMaxBodySize(long size) { _max_body_size = size; }
void Server::setServerName(std::string name) { _name = name; }
std::vector<Location> &Server::getServerLocationsVector(void) {
  return (_locations_vector);
}
std::string Server::getServerName(void) { return (_name); }

std::vector<Location> &Server::getLocations(void) {
  return (_locations_vector);
}

long Server::getMaxBodySize(void) const { return (_max_body_size); }
Location &Server::getCurrentLocation(void) {
  return (_locations_vector.back());
}
std::vector<Socket> &Server::getSockets(void) { return (_sockets_vector); }

MapIntStr &Server::getErrorPages(void) { return (_error_pages); }
std::map<int, Client> &Server::getClients(void) { return (_clients); }

void Server::addLocation(Location &new_location) {
  _locations_vector.push_back(new_location);
}

void Server::addSocket(Socket &new_socket) {
  _sockets_vector.push_back(new_socket);
}

void Server::addErrorPage(int code, std::string path) {
  _error_pages[code] = path;
}

// ==== OUTPUTS ====
void Server::printErrorPages(void) {
  std::cout << BOLD_CYAN << "ERROR PAGES" << endofline;
  for (MapIntStr::const_iterator it = _error_pages.begin();
       it != _error_pages.end(); ++it) {
    std::cout << "[" << it->first << "] -> " << it->second << endofline;
  }
}

void Server::printServer(void) {
  display("\n[ ==== GLOBAL INFOS ==== ]");
  display("SERVER NAME = " + _name);
  printErrorPages();
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
  ls.push_back("php");

  signal(SIGINT, handle_sigint);
  std::vector<pollfd> poll_fds;
  std::map<int, int> _pipe_to_client;

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
      if (_pipe_to_client.count(fd)) {
        // from pipe_cgi non bloquant read
        int client_fd = _pipe_to_client[fd];
        Client &client = _clients[client_fd];
        char buf[4096];
        int n = read(fd, buf, sizeof(buf));
        if (n > 0)
          client.appendToBufferCgi(buf, n);
        else if (n == 0) {
          waitpid(client.getPid(), NULL, WNOHANG);
          std::string resp = buildHttpResponse(client.getBufferCgi());
          client.setResponse(resp);
          client.setStatus(WRITTING);
          close(fd);
          _pipe_to_client.erase(fd);
          poll_fds.erase(poll_fds.begin() + i--);
          for (size_t j = 0; j < poll_fds.size(); j++) {
            if (poll_fds[j].fd == client_fd) {
              poll_fds[j].events = POLLOUT;
              break;
            }
          }
        }
      } else if (_clients.count(fd) == 0) {
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
              Cgi cgi(ls, &client);
              cgi.run();
              pollfd pfd;
              pfd.fd = client.getCgiPipefd();
              pfd.events = POLLIN;
              poll_fds.push_back(pfd);
              _pipe_to_client[client.getCgiPipefd()] = fd;
            } else {
              try {
                std::cerr << "[DEBUG]" << std::endl;
                int r = client._request.parseRequest(client.getBufferRead());
                Response response(200);
                std::string bu = response.build();
                client.setResponse(bu);
              } catch (std::runtime_error &e) {
                int code = std::atoi(e.what());
                if (code == 0)
                  code = 500; // si e.what() n'est pas un code HTTP
                Response response(code);
                std::string bu = response.build();
                client.setResponse(bu);
              } catch (...) {
                std::cerr << "process client" << std::endl;
              }
              poll_fds[i].events = POLLOUT;
            }
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
