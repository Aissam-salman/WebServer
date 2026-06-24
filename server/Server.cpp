#include <iostream>
#include <cstring>

#include "Server.hpp"
#include "utils.hpp"


// ==== ~TORS ====
Server::Server(void) : _max_body_size(-1) {
	std::cout << BOLD_CYAN << "Server Default constructor called" << RESET << std::endl;
}

Server::Server(std::string name) : _name(name), _max_body_size(-1) {
	std::cout << BOLD_CYAN << "Server Name constructor called" << RESET << std::endl;
}

Server::~Server() {
	std::cout << BOLD_RED << "Server Destructor called" << RESET << std::endl;
}

// ==== GETTERS ====
std::vector<Location>&	Server::getServerLocationsVector(void) {
	return (_locations);
}

std::vector<Location>&	Server::getLocations(void) {
	return (_locations);
}

std::vector<Socket>&	Server::getSockets(void) {
	return (_sockets);
}

MapIntStr&	Server::getErrorPages(void) {
	return (_error_pages);
}

// ==== OUTPUTS ====
void	Server::printServer(void) {
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
