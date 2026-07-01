#include "Server.hpp"
#include "cgi/Cgi.hpp"
#include "utils.hpp"
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <sys/poll.h>
#include <vector>
#include <sys/types.h>
#include <sys/wait.h>

bool Server::_running = true;

// ==== ~TORS ====
Server::Server(void) : _max_body_size(-1) {
  std::cout << BOLD_CYAN << "Server Default constructor called" << RESET
            << std::endl;
}

Server::Server(std::string name) : _name(name), _max_body_size(-1) {
  std::cout << BOLD_CYAN << "Server Name constructor called" << RESET
            << std::endl;
}

Server::~Server() {
  std::cout << BOLD_RED << "Server Destructor called" << RESET << std::endl;
}

// ==== GETTERS ====
std::vector<Location> &Server::getServerLocationsVector(void) {
  return (_locations);
}

std::vector<Location> &Server::getLocations(void) { return (_locations); }

std::vector<Socket> &Server::getSockets(void) { return (_sockets); }

MapIntStr &Server::getErrorPages(void) { return (_error_pages); }

// ==== OUTPUTS ====
void Server::printServer(void) {
  display("[ ==== GLOBAL INFOS ==== ]");
  display("SERVER NAME = " + _name);
  std::cout << "NBR OF SOCKETS = " << _sockets.size() << std::endl;
  for (size_t i = 0; i < _sockets.size(); i++) {
    std::cout << BOLD_CYAN << "LOCATION NUMBER " << i << endofline;
    _sockets[i].printSocket();
    std::cout << endofline;
  }
  std::cout << endofline;
  for (size_t i = 0; i < _locations.size(); i++) {
    std::cout << BOLD_CYAN << "LOCATION NUMBER " << i << endofline;
    _locations[i].printLocation();
    std::cout << endofline;
  }
}

void Server::handle_sigint(int) {
  Server::_running = false;
  std::cerr << "SIGNAL RECEIVED" << std::endl;
}

void Server::run(void) {
  Socket socket1("SocketTest");
  socket1.setSocket(PORT);
  _sockets.push_back(socket1);
  std::vector<std::string> ls;
  ls.push_back("python");
  ls.push_back("php");

  signal(SIGINT, handle_sigint);
  std::vector<pollfd> poll_fds;
	std::map<int, int> _pipe_to_client;

  std::vector<Socket>::iterator it = _sockets.begin();
  std::vector<Socket>::iterator ite = _sockets.end();
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
						if (poll_fds[j].fd == client_fd){
							poll_fds[j].events = POLLOUT;
							break;
						}
					}
				}
			}
			else if (_clients.count(fd) == 0) {
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
              // resp = buildResp(client._request);
              resp = "HTTP/1.1 200 OK\r\nContent-Type: text/html; "
                     "charset=utf-8\r\nContent-Length: 53\r\nConnection: "
                     "keep-alive\r\n\r\n<!DOCTYPE html><html><body>Hello "
                     "world</body></html>";
							client.setResponse(resp);
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
