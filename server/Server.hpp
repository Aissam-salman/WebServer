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
  std::vector<Socket> _sockets_vector;
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

	std::vector<Location> 		&getServerLocationsVector(void);
  	void printServer(void);
  	void printErrorPages(void);

	long						getMaxBodySize(void) const;
	void						setMaxBodySize(long size);
	void						setServerName(std::string name);
	std::string					getServerName(void);
	std::vector<Location>		&getLocations(void);
	Location 					&getCurrentLocation(void);
	std::vector<Socket>			&getSockets(void);
	MapIntStr&					getErrorPages(void);
	std::map<int, Client>		&getClients(void);

	void 						addLocation(Location &new_location);
	void 						addSocket(Socket &new_socket);
	void 						addErrorPage(int code, std::string path);

	static bool					_running;
	void						run(void);
	static void					handle_sigint(int);
};

struct Listener {
    Socket _socket;
    std::vector<Server*> _pointers_to_server;
};


#endif
