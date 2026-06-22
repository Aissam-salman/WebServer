#ifndef SERVER_HPP
#define SERVER_HPP

#include <map>
#include <string>
#include <vector>
#include "client/Client.hpp"
#include "Socket.hpp"
#include "Location.hpp"

class Server {
private:
  std::string m_name;
  std::vector<Location> m_locations;
  std::vector<Socket> m_sockets;
  // size_t					m_max_body_size; // TODO :
  // Ajouter le cap body_size HOW TO STORE ERROR_PAGES (MAP ?)
  std::map<int, Client> _clients;

public:
	static bool g_running;
  Server(void);
  Server(std::string name);
  Server(const Server &src);
  Server &operator=(const Server &other);
  ~Server();
	void run(void);
	static void handle_sigint(int);
};

#endif
