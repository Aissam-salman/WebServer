#include <iostream>
#include "Location.hpp"

Location::Location(std::string name): _name(name), _max_body_size(-1), _autoindex(true) {
    std::cout << "Location standard constructor called" << std::endl;
}

Location::Location(const Location &src):
    _name(src._name),
    _root_path(src._root_path),
    _index_path(src._index_path),
    _allowed_methods(src._allowed_methods),
    _max_body_size(src._max_body_size),
    _autoindex(src._autoindex) {}

Location& Location::operator=(const Location &other) {
    if (this != &other) {
        _name = other._name;
        _root_path = other._root_path;
        _index_path = other._index_path;
        _allowed_methods = other._allowed_methods;
        _max_body_size = other._max_body_size;
        _autoindex = other._autoindex;
    }
    return *this;
}

Location::~Location() { }
