#include "Server.hpp"
#include "Location.hpp"
#include "configutils.hpp"
#include "utils.hpp"
#include <iostream>
#include <string>
#include <vector>
#include <map>

// ==== ~TORS ====
Server::Server(void)
    : _name(""), _error_pages(initErrorPages()), _max_body_size(0) {}

Server::Server(std::string name)
    : _name(name), _error_pages(initErrorPages()), _max_body_size(-1) {}

Server::Server(const Server &src)
    : _name(src._name), _sockets_vector(src._sockets_vector),
      _locations_vector(src._locations_vector), _error_pages(src._error_pages),
      _max_body_size(src._max_body_size) {}

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

// DEBUG-PRINT THE ERROR-PAGE MAP
void Server::printErrorPages(void) {
    std::cout << BOLD_CYAN << "ERROR PAGES" << endofline;
    for (MapIntStr::const_iterator it = _error_pages.begin();
         it != _error_pages.end(); ++it) {
        std::cout << "[" << it->first << "] -> " << it->second << endofline;
    }
}

// DEBUG-PRINT THE SERVER: NAME, ERROR PAGES, SOCKETS, LOCATIONS
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

// ==== LISTENER ====
Listener::Listener(Socket socket) : _socket(socket) {}

Socket &Listener::getSocket(void) { return (_socket); }
std::vector<Server *> &Listener::getLinkedServers(void) {
    return (_linked_servers_vector);
}

// FIND AN EXISTING LISTENER BOUND TO THIS HOST:PORT (NULL IF NONE)
Listener *findListener(std::vector<Listener> &listeners_vector,
                       std::string host, int port) {
    for (size_t i = 0; i < listeners_vector.size(); i++) {
        if (listeners_vector[i].getSocket().getHost() == host &&
            listeners_vector[i].getSocket().getPort() == port)
            return (&listeners_vector[i]);
    }
    return (NULL);
}

Listener *findListenerByFd(std::vector<Listener> &listeners_vector, int fd) {
    for (size_t i = 0; i < listeners_vector.size(); i++) {
        if (listeners_vector[i].getSocket().getSocketFd() == fd)
            return (&listeners_vector[i]);
    }
    return (NULL);
}

// PICKS THE CANDIDATE WHOSE NAME MATCHES HOST (PORT SUFFIX STRIPPED),
// OR THE FIRST CANDIDATE IF NONE MATCH (NGINX-STYLE DEFAULT SERVER)
Server *matchServerByHost(std::vector<Server *> &candidates, const std::string &host_in) {
    if (candidates.empty())
        return (NULL);

    std::string host = host_in;
    size_t colon = host.find(':');
    if (colon != std::string::npos)
        host = host.substr(0, colon);

    for (size_t i = 0; i < candidates.size(); i++) {
        if (candidates[i]->getServerName() == host)
            return (candidates[i]);
    }
    return (candidates[0]);
}

// BUILD ONE LISTENER PER UNIQUE HOST:PORT, LINKING SERVERS THAT SHARE IT
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

            // this host:port already has a listener -> link this server to it
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

// DEBUG-PRINT EVERY LISTENER AND ITS LINKED SERVERS
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

// BIND AND START LISTENING ON EVERY GATHERED LISTENER'S SOCKET
void setupListeners(std::vector<Listener> &listeners) {
    for (size_t i = 0; i < listeners.size(); i++) {
        Socket &socket = listeners[i].getSocket();
        socket.setSocket(socket.getPort());
    }
}
