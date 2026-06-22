#include <iostream>
#include <stdatomic.h>
#include "utils.hpp"
#include "Lexer.hpp"

Lexer::Lexer(void) {
    std::cout << "Default constructor called" << std::endl;
}

Lexer::Lexer(std::string name): _name(name) {
    std::cout << "Name constructor called" << std::endl;
}

Lexer::Lexer(const Lexer &src) {
    std::cout << "Copy constructor called" << std::endl;
    *this = src;
}

Lexer& Lexer::operator= (const Lexer &other) {
    std::cout << "Copy assignment operator called" << std::endl;
    if (this != &other)
        _name = other._name;
    return (*this);
}

Lexer::~Lexer() {
    std::cout << "Destructor called" << std::endl;
}

// TESTS / DEMOS

void    Lexer::printTokens(void) const { 
    for (size_t i = 0; i < _tokens_vector.size(); i++) {
        std::cout << "TOKEN NB ["<< i << "] = " <<  _tokens_vector[i].c_str() << "\n" << std::endl;
    }
}
