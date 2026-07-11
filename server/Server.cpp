#include "Server.hpp"
#include "Cgi.hpp"
#include "Client.hpp"
#include "Location.hpp"
#include "Request.hpp"
#include "Response.hpp"
#include "StaticHandler.hpp"
#include "config/configutils.hpp"
#include "utils.hpp"
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

// ==== LISTENER ====
Listener::Listener(Socket socket) : _socket(socket) {}

Socket &Listener::getSocket(void) { return (_socket); }
std::vector<Server *> &Listener::getLinkedServers(void) {
    return (_linked_servers_vector);
}

// Looks for an existing listener already bound to this host:port.
Listener *findListener(std::vector<Listener> &listeners_vector,
                       std::string host, int port) {
    for (size_t i = 0; i < listeners_vector.size(); i++) {
        if (listeners_vector[i].getSocket().getHost() == host &&
            listeners_vector[i].getSocket().getPort() == port)
            return (&listeners_vector[i]);
    }
    return (NULL);
}

// Walks every server and its sockets, and builds one listener per unique
// host:port, linking every server that shares it (virtual hosts).
std::vector<Listener> gatherListeners(std::vector<Server> &servers_vector) {
    std::vector<Listener> listener_vectors;
    std::string host;
    int port;

    for (size_t i = 0; i < servers_vector.size(); i++) {
        // reference, not a copy: we only read host/port here
        const std::vector<Socket> &sockets_vector =
            servers_vector[i].getSockets();

        for (size_t j = 0; j < sockets_vector.size(); j++) {
            host = sockets_vector[j].getHost();
            port = sockets_vector[j].getPort();
            Listener *listener_ptr = findListener(listener_vectors, host, port);

            // this host:port already has a listener -> just link this server to
            // it
            if (listener_ptr != NULL)
                listener_ptr->getLinkedServers().push_back(&servers_vector[i]);
            // otherwise create a new listener for it
            else {
                Listener new_listener(sockets_vector[j]);
                new_listener.getLinkedServers().push_back(&servers_vector[i]);
                listener_vectors.push_back(new_listener);
            }
        }
    }
    return (listener_vectors);
}

void printListeners(std::vector<Listener> &listeners) {
    for (size_t i = 0; i < listeners.size(); i++) {
        std::cout << BOLD_CYAN << "LISTENER NBR " << i << endofline;
        std::cout << BOLD_RED << "HOST = " << listeners[i].getSocket().getHost()
                  << " || PORT = " << listeners[i].getSocket().getPort()
                  << endofline;
        std::cout << BOLD_GREEN << "LINKED SERVERS = ";
        std::vector<Server *> servers = listeners[i].getLinkedServers();
        for (size_t j = 0; j < servers.size(); j++)
            std::cout << BOLD_GREEN << servers[j]->getServerName() << " | ";
        std::cout << endofline;
    }
}

// Binds and starts listening on every gathered listener's socket.
// Free function because it works across all servers, not a single one.
void setupListeners(std::vector<Listener> &listeners) {
    for (size_t i = 0; i < listeners.size(); i++) {
        Socket &socket = listeners[i].getSocket();
        socket.setSocket(socket.getPort());
    }
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
    char buf[STD_BUFFER];
    int n = read(fd, buf, sizeof(buf));
    if (n < 0) {
        throw std::runtime_error("500");
    }
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

long Server::findMaxBodySizeInLocation(void){
  std::vector<Location>::const_iterator it = _locations_vector.begin();
  std::vector<Location>::const_iterator ite = _locations_vector.end();
  
  for (;it != ite; ++it) {
    it->printLocation();
  }
  return 0;
}

// create client class, and poll_fd for client and add to pollfds
void Server::acceptNewClient(int listen_fd) {
    long cur_max_size = _max_body_size;
    findMaxBodySizeInLocation();
      
    _clients[listen_fd] = Client(listen_fd, Request(), cur_max_size);
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
    Cgi cgi(_languages_supported, client);
    cgi.run();
    pollfd pfd;
    pfd.fd = client.getCgiPipefd();
    pfd.events = POLLIN;
    _poll_fds.push_back(pfd);
    _pipe_to_client[client.getCgiPipefd()] = fd;
}

void Server::handleReq(Client &client, int i) {
    StaticHandler handler(client._request, _locations_vector);
    Response response = handler.handle();
    std::string bu = response.build();
    client.setResponse(bu);
    _poll_fds[i].events = POLLOUT;
}

// if catch std::runtime_error in clientRead catch
void Server::responseError(std::runtime_error &e, int i, Client &client) {
    int code = std::atoi(e.what());
    if (code == 0)
        code = 500;
    Response response = buildErrorResponse(code, getErrorPages());
    std::string bu = response.build();
    client.setResponse(bu);
    _poll_fds[i].events = POLLOUT;
}

void Server::clientRead(size_t &i, int fd) {
    Client &client = _clients[fd];
    bool isHere;

    try {
        isHere = client.handleRecv();
    } catch (std::runtime_error &e) {
        responseError(e, i, client);
        return;
    }
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

// Called once after every poll() wake-up. poll() has just filled in the
// .revents field of each pollfd; our job here is to walk the whole set and
// react to every fd that became ready. We handle THREE kinds of fd, all living
// in the same _poll_fds array:
//   1. CGI pipe fds     -> present in _pipe_to_client (pipe read-end <- CGI
//   stdout)
//   2. listening fds    -> not a client and not a pipe (a bound server socket)
//   3. client conn fds  -> present in _clients (an accepted TCP connection)
void Server::loopPollFds(void) {

    // Indexed loop (not an iterator) on purpose: closeClient() erases entries
    // from _poll_fds and does `i--`, so the container is mutated mid-iteration.
    // We also re-check .size() each turn because acceptNewClient()/handleCgi()
    // push_back new fds while we loop.
    for (size_t i = 0; i < _poll_fds.size(); i++) {
        int fd = _poll_fds[i].fd;

        // .revents == 0 -> poll() reported nothing happened on this fd this
        // round. Skip it; only fds with a pending event are worth handling.
        if (!_poll_fds[i].revents)
            continue;

        // --- Case 1: this fd is a CGI output pipe
        // ------------------------------- The key exists in _pipe_to_client,
        // i.e. it maps this pipe's read-end back to the client that launched
        // the CGI. Data (or EOF) is ready from the script's stdout;
        // readCgiPipe() accumulates it and, on EOF, builds the HTTP response
        // and flips the client to POLLOUT.
        if (_pipe_to_client.count(fd)) {
            readCgiPipe(i, fd);

            // --- Case 2: this fd is a listening socket
            // ------------------------------ Not a client (count == 0) and
            // (from the else-if order) not a pipe either, so it must be one of
            // our bound listeners. A POLLIN here means a brand-new connection
            // is waiting: accept() it and register the new client fd. NOTE (CP3
            // TODO): we don't yet know WHICH Listener this fd belongs to, so
            // the accepted client can't carry its candidate servers yet.
        } else if (_clients.count(fd) == 0) {
            if (_poll_fds[i].revents & POLLIN) {
                int client_fd = accept(fd, NULL, NULL);
                if (client_fd <
                    0) // accept failed (e.g. client vanished): ignore
                    continue;
                acceptNewClient(
                    client_fd); // create Client + add its fd to _poll_fds
            }

            // --- Case 3: this fd is an established client connection
            // -----------------
        } else {
            // POLLIN: the client sent bytes -> read/parse the request (may
            // dispatch to CGI or build a response, and switch the fd to POLLOUT
            // when ready).
            if (_poll_fds[i].revents & POLLIN)
                clientRead(i, fd);
            // POLLOUT: the socket is writable -> flush (part of) the response;
            // closeClient() once everything has been sent.
            else if (_poll_fds[i].revents & POLLOUT) {
                clientWrite(i, fd);
                // POLLHUP/POLLERR: peer hung up or the socket errored -> tear
                // it down.
            } else if (_poll_fds[i].revents & (POLLHUP | POLLERR)) {
                closeClient(i, fd);
            }
        }
    }
}

// RUNS THE SERVER
// TODO : Scale from one server to multiple server -> Make run in a
// "super-class" such as WebServer that can run through all the fds of all
// servers
void Server::run(std::vector<Listener> &listeners) {
    setupListeners(listeners);

    for (size_t i = 0; i < listeners.size(); i++) {
        pollfd pfd;
        pfd.fd = listeners[i].getSocket().getSocketFd();
        pfd.events = POLLIN;
        pfd.revents = 0;
        _poll_fds.push_back(pfd);
    }

    while (_running) {
        int ret = poll(&_poll_fds[0], _poll_fds.size(), TIMEOUT);
        if (ret == -1 && _running)
            throw std::runtime_error("Poll failed miserably");
        loopPollFds();
    }
}
