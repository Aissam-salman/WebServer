#include <iostream>

#include "Location.hpp"
#include "utils.hpp"


// ==== ~TORS ====
Location::Location(std::string name): _name(name), _max_body_size(-1), _autoindex(true), _methods_flag(0) {
    std::cout << "Location standard constructor called" << std::endl;
}

Location::Location(const Location &src):
    _name(src._name),
    _max_body_size(src._max_body_size),
    _root_path(src._root_path),
    _index_path(src._index_path),
    // _allowed_methods(src._allowed_methods),
    _autoindex(src._autoindex), 
	_methods_flag(src._methods_flag) {} 

Location& Location::operator=(const Location &other) {
    if (this != &other) {
        _name = other._name;
        _root_path = other._root_path;
        _index_path = other._index_path;
        // _allowed_methods = other._allowed_methods;
        _max_body_size = other._max_body_size;
        _autoindex = other._autoindex;
    }
    return *this;
}

Location::~Location() { }

void	Location::setMethods(e_methods method) {
    _methods_flag |= method;
}

// ==== OUTPUTS ====
void	Location::printLocation(void) {
    std::cout << BOLD_WHITE << "======== LOCATION " << _name << " ======== " << endofline;
    std::cout << BOLD_YELLOW << "ROOT PATH = " << _root_path << endofline;
    std::cout << BOLD_YELLOW << "INDEX PATH = " << _index_path << endofline;
    std::cout << BOLD_YELLOW << "MAX BODY SIZE = " << _max_body_size << endofline;
    std::cout << BOLD_YELLOW << "AUTOINDEX = " << _autoindex << endofline;
    printSetMethods(_methods_flag);;
    std::cout << BOLD_WHITE << "===============================" << endofline;
}