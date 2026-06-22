#include "Server.hpp"
#include "cgi/Cgi.hpp"
#include "utils.hpp"
#include <csignal>
#include <iostream>
#include <stdexcept>
#include <vector>

bool Server::g_running = false;

Server::Server(void) {
  std::cout << BOLD_CYAN << "Server Default constructor called" << RESET
            << std::endl;
}

Server::Server(std::string name) : m_name(name) {
  std::cout << BOLD_CYAN << "Server Name constructor called" << RESET
            << std::endl;
  g_running = true;
}

Server::Server(const Server &src) {
  std::cout << BOLD_BLUE << "Server Copy constructor called" << RESET
            << std::endl;
  *this = src;
}

Server &Server::operator=(const Server &other) {
  std::cout << BOLD_BLUE << "Server Copy assignment operator called" << RESET
            << std::endl;
  if (this != &other)
    m_name = other.m_name;
  return (*this);
}

Server::~Server() {
  std::cout << BOLD_RED << "Server Destructor called" << RESET << std::endl;
}

void Server::handle_sigint(int) {
  Server::g_running = false;
  std::cerr << "SIGNAL RECEIVED" << std::endl;
}

void Server::run(void) {
  Socket socket1("SocketTest");
  socket1.setSocket(PORT);
  m_sockets.push_back(socket1);
  std::vector<std::string> ls;
  ls.push_back("python");

  signal(SIGINT, handle_sigint);
  std::vector<pollfd> poll_fds;

  std::vector<Socket>::iterator it = m_sockets.begin();
  std::vector<Socket>::iterator ite = m_sockets.end();

  for (; it != ite; it++) {
    pollfd pfd;
    pfd.fd = (*it).getSocketFd();
    std::cout << (*it).getSocketFd() << std::endl;
    pfd.events = POLLIN;
    pfd.revents = 0;
    poll_fds.push_back(pfd);
  }

  while (g_running) {
    int ret = poll(&poll_fds[0], poll_fds.size(), TIMEOUT);
    if (ret == -1 && g_running)
      throw std::runtime_error("Poll failed miserably");
    for (size_t i = 0; i < poll_fds.size(); i++) {
      int fd = poll_fds[i].fd;
      if (!poll_fds[i].revents)
        continue;
      if (_clients.count(fd) == 0) {
        // new client
        if (poll_fds[i].revents & POLLIN) {
          int client_fd = accept(fd, NULL, NULL);
          _clients[client_fd] = Client(client_fd, Request("8080", "0.0.0.0", "0000", "www"));
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
              resp = "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\nContent-Length: 53\r\nConnection: keep-alive\r\n\r\n<!DOCTYPE html><html><body>Hello world</body></html>";
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
