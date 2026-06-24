#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>
#include <vector>
#include <map>
#include "Location.hpp"
#include "Socket.hpp"
#include "utils.hpp"

class Server {
  private:
	std::string						_name;
	std::vector<Socket>				_sockets;
	std::vector<Location>			_locations;
	MapIntStr						_error_pages; // LINKS ERROR CODES TO ACCORDING PAGES
	long							_max_body_size; // TODO : Ajouter le cap body_size
	// HOW TO STORE ERROR_PAGES (MAP ?)

	Server(const Server &src);
	Server &operator=(const Server &other);

  public:
	~Server();
	Server(void);
	Server(std::string name);

	std::vector<Location>&	getServerLocationsVector(void);
	void	printServer(void);

	std::vector<Location>&	getLocations(void) ;
	std::vector<Socket>&	getSockets(void) ;
	MapIntStr&				getErrorPages(void) ;
	
};

#endif
