#ifndef SERVER_HPP
#define SERVER_HPP

#include "Location.hpp"
#include "Socket.hpp"
#include "utils.hpp"
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

class Listener;

// A single virtual host's configuration: its name, listening sockets, routes
// (locations), error pages and body-size cap. Pure data — the poll() event loop
// and per-connection state live in WebServ, which owns a vector of these.
class Server {
  private:
    std::string _name;
    std::vector<Socket> _sockets_vector;
    std::vector<Location> _locations_vector;
    MapIntStr _error_pages; // LINKS ERROR CODES TO ACCORDING PAGES
    long _max_body_size;    // TODO : Ajouter le cap body_size

    Server &operator=(const Server &other);

  public:
    ~Server();
    Server(const Server &src);
    Server(void);
    Server(std::string name);

    std::vector<Location> &getServerLocationsVector(void);
    void printServer(void);
    void printErrorPages(void);

    long getMaxBodySize(void) const;
    void setMaxBodySize(long size);
    void setServerName(std::string name);
    std::string getServerName(void);
    std::vector<Location> &getLocations(void);
    Location &getCurrentLocation(void);
    std::vector<Socket> &getSockets(void);
    MapIntStr &getErrorPages(void);

    void addLocation(Location &new_location);
    void addSocket(Socket &new_socket);
    void addErrorPage(int code, std::string path);
};

// One listening socket shared by every server that answers on its host:port
// (virtual hosts). Kept outside Server so servers don't own their listeners.
class Listener {
  private:
    Socket _socket;
    std::vector<Server *> _linked_servers_vector;

  public:
    Listener(Socket socket);

    Socket &getSocket(void);
    std::vector<Server *> &getLinkedServers(void);
};

// Free functions: operate across ALL servers, so they live outside Server.
std::vector<Listener> gatherListeners(std::vector<Server> &servers_vector);
Listener *findListener(std::vector<Listener> &listeners_vector,
                       std::string host, int port);
Listener *findListenerByFd(std::vector<Listener> &listeners_vector, int fd);
void printListeners(std::vector<Listener> &listeners);
void setupListeners(std::vector<Listener> &listeners);

#endif
