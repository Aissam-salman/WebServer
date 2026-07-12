#include <iostream>

#include "Location.hpp"
#include "utils.hpp"


// ==== ~TORS ====
Location::Location(std::string name, long max_body_size): _name(name), _max_body_size(max_body_size), _root_path(""),  _autoindex(true), _methods_flag(0), _return(false), _return_error_code(0), _return_path("") {
}

Location::Location(const Location &src):
    _name(src._name),
    _max_body_size(src._max_body_size),
    _root_path(src._root_path),
    _index_path(src._index_path),
    _autoindex(src._autoindex),
	_methods_flag(src._methods_flag),
	_return(src._return),
	_return_error_code(src._return_error_code),
	_return_path(src._return_path),
	_cgi_map(src._cgi_map) {}

Location& Location::operator=(const Location &other) {
    if (this != &other) {
        _name = other._name;
        _root_path = other._root_path;
        _index_path = other._index_path;
        _max_body_size = other._max_body_size;
        _autoindex = other._autoindex;
        _methods_flag = other._methods_flag;
        _return = other._return;
        _return_error_code = other._return_error_code;
        _return_path = other._return_path;
        _cgi_map = other._cgi_map;
    }
    return *this;
}

Location::~Location() { }

// ==== SETTERS ====
void	Location::setName(std::string name) { _name = name; }
void	Location::setMaxBodySize(long size) { _max_body_size = size; }
void	Location::setRootPath(std::string path) { _root_path = path; }
void	Location::setIndexPath(std::string path) { _index_path = path; }
void	Location::setAutoIndex(bool state) { _autoindex = state; }
void	Location::setMethods(e_methods method) { _methods_flag |= method; }
void	Location::setReturn(bool state) { _return = state; }
void	Location::setReturnPath(std::string path) { _return_path = path; }
void	Location::setReturnErrorCode(int code) { _return_error_code = code; }
void	Location::setCgi(std::string extension, std::string interpreter) { _cgi_map[extension] = interpreter; }

// ==== GETTERS ====
std::string&	Location::getName(void) { return (_name); }
long			Location::getMaxBodySize(void) const { return (_max_body_size); }
std::string&	Location::getRootPath(void) { return (_root_path); }
std::string&	Location::getIndexPath(void) { return (_index_path); }
bool			Location::getAutoIndex(void) const { return (_autoindex); }
int&			Location::getMethodFlag(void) { return (_methods_flag); }
bool			Location::getReturn(void) const { return (_return); }
std::string&	Location::getReturnPath(void) { return (_return_path); }
int				Location::getReturnErrorCode(void) const { return (_return_error_code); }
std::map<std::string, std::string>&	Location::getCgiMap(void) { return (_cgi_map); }

// ==== OUTPUTS ====

// DEBUG-PRINT THIS LOCATION BLOCK (ROOT, INDEX, METHODS, CGI, RETURN...)
void	Location::printLocation(void) const {
    std::cout << BOLD_WHITE << "======== LOCATION " << _name << " ======== " << endofline;
    std::cout << BOLD_YELLOW << "ROOT PATH = " << _root_path << endofline;
    std::cout << BOLD_YELLOW << "INDEX PATH = " << _index_path << endofline;
    std::cout << BOLD_YELLOW << "MAX BODY SIZE = " << _max_body_size << endofline;
    std::cout << BOLD_YELLOW << "AUTOINDEX = " << _autoindex << endofline;
    std::cout << BOLD_YELLOW << "RETURN = " << _return << endofline;
    std::cout << BOLD_YELLOW << "RETURN CODE = " << _return_error_code << endofline;
    std::cout << BOLD_YELLOW << "RETURN PATH = " << _return_path << endofline;
    printSetMethods(_methods_flag);;
    for (std::map<std::string, std::string>::const_iterator it = _cgi_map.begin(); it != _cgi_map.end(); ++it)
        std::cout << BOLD_YELLOW << "CGI = " << it->first << " -> " << it->second << endofline;

    std::cout << BOLD_WHITE << "===============================" << endofline;
}