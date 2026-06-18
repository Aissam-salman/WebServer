#include <iostream>
#include <fstream>
#include <stdexcept>
#include <string>
// 
#include "utils.hpp"
#include "configutils.hpp"
#include "Lexer.hpp"


// ===== ~TORS  =====
Lexer::Lexer(void) {
    std::cout << "Default constructor called" << endofline;
}

Lexer::Lexer(std::string conf_file_path): _raw_file_path(conf_file_path), _raw_conf_file(conf_file_path.c_str()) {
    std::cout << "Lexer name constructor called" << endofline;
    if (_raw_file_path.size() < 5 || _raw_file_path.substr(_raw_file_path.size() - 5) != ".conf")
        throw std::runtime_error ("Invalid file name : must end with .conf");
    if (_raw_conf_file.is_open() == false) 
        throw std::runtime_error ("Error opening the file : " + _raw_file_path);
}

Lexer::Lexer(const Lexer &src) {
    std::cout << "Copy constructor called" << endofline;
    *this = src;
}

// TODO : Changer les *this 
Lexer& Lexer::operator= (const Lexer &other) {
    std::cout << "Copy assignment operator called" << endofline;
    if (this != &other)
        return (*this);
    return (*this);
}

Lexer::~Lexer() {
    std::cout << "Destructor called" << endofline;
}

// ===== METHODS =====
// CREER LA LISTE DE TOKENS
void    Lexer::initTokensVector(void) {
    std::string line;

    while(std::getline(_raw_conf_file, line))
        display(line);
}

// ===== TEST/OUTPUT =====
// PRINTS THE TOKEN VECTORS
void    Lexer::printTokens(void) const { 
    for (size_t i = 0; i < _raw_tokens_vector.size(); i++) {
        std::cout << "TOKEN NB ["<< i << "] = " <<  _raw_tokens_vector[i].c_str() << "\n" << endofline;
    }
}


// READS THROUGH THE _RAW_CONF_FILE
void    Lexer::printRawConfFile(void) {
    std::string line;

    while(std::getline(_raw_conf_file, line))
        display(line);

    _raw_conf_file.clear(); // RETURN TO THE BEGININ OF THE IFSTREAM
    _raw_conf_file.seekg(0); // RETURN TO THE BEGININ OF THE IFSTREAM
}