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
	std::string				_name;
	std::vector<Location>	_locations;
	std::vector<Socket>		_sockets;
	std::map<e_codes, std::string> _error_pages; // LINKS ERROR CODES TO ACCORDING PAGES
	// size_t					m_max_body_size; // TODO : Ajouter le cap body_size
	// HOW TO STORE ERROR_PAGES (MAP ?)

  public:
	Server(void);
	Server(std::string name);
	Server(const Server &src);
	Server &operator=(const Server &other);
	~Server();
	
};

#endif
