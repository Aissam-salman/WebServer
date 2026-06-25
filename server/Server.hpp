#ifndef SERVER_HPP
#define SERVER_HPP

#include "Location.hpp"
#include "Socket.hpp"
#include "client/Client.hpp"
#include "utils.hpp"
#include <map>
#include <string>
#include <vector>

class Server {
private:
  std::string _name;
  std::vector<Socket> _sockets;
  std::vector<Location> _locations_vector;
  MapIntStr _error_pages; // LINKS ERROR CODES TO ACCORDING PAGES
  long _max_body_size;    // TODO : Ajouter le cap body_size
  // HOW TO STORE ERROR_PAGES (MAP ?)
  std::map<int, Client> _clients;

  Server &operator=(const Server &other);

public:
  ~Server();
  Server(const Server &src);
  Server(void);
  Server(std::string name);

  std::vector<Location> &getServerLocationsVector(void);
  void printServer(void);

  std::vector<Location> &getLocations(void);
  std::vector<Socket> &getSockets(void);
  MapIntStr &getErrorPages(void);

  void  addLocation(Location& new_location);

  static bool _running;
  void run(void);
  static void handle_sigint(int);
};

#endif
