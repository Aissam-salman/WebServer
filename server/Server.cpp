#include "Client.hpp"
#include "Request.hpp"
#include "Server.hpp"
#include "Cgi.hpp"
#include "Response.hpp"
#include "config/configutils.hpp"
#include "../utils/utils.hpp"
#include "StaticHandler.hpp"
#include <csignal>
#include <cstddef>
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
    : _name(name), _error_pages(initErrorPages()), _max_body_size(-1) {
  signal(SIGINT, handle_sigint);
  _languages_supported.push_back("python");
  _languages_supported.push_back("php");
}

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

/*
 * Init Sockets, add to pollfd
 */
void Server::setupListeners(void) {
  Socket socket1("SocketTest");
  socket1.setSocket(PORT);
  _sockets_vector.push_back(socket1);

  // std::vector<Socket>::iterator it = _sockets_vector.begin();
  // std::vector<Socket>::iterator ite = _sockets_vector.end();
  // for (; it != ite; it++) {
  //   pollfd pfd;
  //   pfd.fd = s.getSocketFd();
  //   pfd.events = POLLIN;
  //   pfd.revents = 0;
  //   poll_fds.push_back(pfd);
  // }
  //
  pollfd pol;
  pol.fd = _sockets_vector.back().getSocketFd();
  pol.events = POLLIN;
  pol.revents = 0;
  _poll_fds.push_back(pol);
}

void Server::switchFdsToPollout(int client_fd) {
  for (size_t j = 0; j < _poll_fds.size(); j++) {
    if (_poll_fds[j].fd == client_fd) {
      _poll_fds[j].events = POLLOUT;
      break;
    }
  }
}

/*
 * read pipe from execve stdout, and switch to POLLOUT to send response http
 */
void Server::readCgiPipe(size_t &i, int fd) {
  int client_fd = _pipe_to_client[fd];
  Client &client = _clients[client_fd];
  char buf[4096];
  int n = read(fd, buf, sizeof(buf));
  //WARN:
  // que se passe-t-il si read() retourne -1 (erreur) ?
  // Le code ne traite que n > 0 et n == 0 : dans le cas d'erreur,
  // le fd ne serait jamais fermé ni retiré de _poll_fds. Ça te semble un cas possible en pratique ?
  if (n > 0) {
    client.appendToBufferCgi(buf, n);
  } else if (n == 0) {
    waitpid(client.getPid(), NULL, WNOHANG);
    std::string resp = buildHttpResponse(client.getBufferCgi());
    client.setResponse(resp);
    client.setStatus(WRITTING);
    closeClient(i, fd);
    switchFdsToPollout(client_fd);
  }
}

// create client class, and poll_fd for client and add to pollfds
void Server::acceptNewClient(int listen_fd) {
  // FIX: params request need dynamic value
  _clients[listen_fd] =
      Client(listen_fd, Request("8080", "0.0.0.0", "0000", "www"));
  _poll_fds.push_back(_clients[listen_fd].getPollfd());
}

void Server::closeClient(size_t &i, int fd) {
  close(fd);
  _clients.erase(fd);
  _poll_fds.erase(_poll_fds.begin() + i--);
}

// send client to cgi execve, with request already parsed
// and add to pollfds,
// i add to pipe_to_client to keep the stdout pipe of execve, for the response
// from cgi script (non bloquant)
// associate the pipe with client
void Server::handleCgi(Client &client, int fd) {
  Cgi cgi(_languages_supported, &client);
  cgi.run();
  pollfd pfd;
  pfd.fd = client.getCgiPipefd();
  pfd.events = POLLIN;
  _poll_fds.push_back(pfd);
  _pipe_to_client[client.getCgiPipefd()] = fd;
}

void Server::handleReq(Client &client, int i) {
  Response response(200);
  std::string bu = response.build();
  client.setResponse(bu);
  _poll_fds[i].events = POLLOUT;
}

// if catch std::runtime_error in clientRead catch
void Server::responseError(std::runtime_error &e, int i, Client &client) {
  int code = std::atoi(e.what());
  if (code == 0)
    code = 500;
  Response response(code);
  std::string bu = response.build();
  client.setResponse(bu);
  _poll_fds[i].events = POLLOUT;
}

void Server::clientRead(size_t &i, int fd) {
  Client &client = _clients[fd];

  bool isHere = client.handleRecv();
  if (!isHere)
    closeClient(i, fd);
  else if (client.getStatus() == WRITTING) {
    try {
      client._request.parseRequest(client.getBufferRead());
      if (client._request.isCGI())
        handleCgi(client, fd);
      else
        handleReq(client, i);
    } catch (std::runtime_error &e) {
      responseError(e, i, client);
    }
  }
}

void Server::clientWrite(size_t &i, int fd) {
  Client &client = _clients[fd];
  bool isDone = client.handleSend();
  if (!isDone) {
    closeClient(i, fd);
  }
}

void Server::loopPollFds(void) {
  for (size_t i = 0; i < _poll_fds.size(); i++) {
    int fd = _poll_fds[i].fd;

    if (!_poll_fds[i].revents)
      continue;

    if (_pipe_to_client.count(fd)) {
      readCgiPipe(i, fd);
    } else if (_clients.count(fd) == 0) {
      if (_poll_fds[i].revents & POLLIN) {
        // add new client to poll
        int client_fd = accept(fd, NULL, NULL);
        if (client_fd < 0)
          continue;
        acceptNewClient(client_fd);
      }
    } else {
      if (_poll_fds[i].revents & POLLIN)
        clientRead(i, fd);
      else if (_poll_fds[i].revents & POLLOUT) {
        clientWrite(i, fd);
      } else if (_poll_fds[i].revents & (POLLHUP | POLLERR)) {
        closeClient(i, fd);
      }
    }
  }
}

void Server::run(void) {
  setupListeners();
  while (_running) {
    int ret = poll(&_poll_fds[0], _poll_fds.size(), TIMEOUT);
    if (ret == -1 && _running)
      throw std::runtime_error("Poll failed miserably");
    loopPollFds();
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
            std::string resp;
            if (client._request.isCGI()) {
              client._request.parseRequest(client.getBufferRead());
              Cgi cgi(ls, &client);
              cgi.run();
              pollfd pfd;
              pfd.fd = client.getCgiPipefd();
              pfd.events = POLLIN;
              poll_fds.push_back(pfd);
              _pipe_to_client[client.getCgiPipefd()] = fd;
            } else {
              try {
                client._request.parseRequest(client.getBufferRead());
                StaticHandler handler(client._request, _locations_vector);
                Response response = handler.handle();
                std::string bu = response.build();
                client.setResponse(bu);
              } catch (std::runtime_error &e) {
                int code = std::atoi(e.what());
                if (code == 0)
                  code = 500; // si e.what() n'est pas un code HTTP
                Response response = buildErrorResponse(code, getErrorPages());
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



