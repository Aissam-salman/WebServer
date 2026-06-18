#include <string>
#include <fstream>
#include <iostream>
#include <vector>
#include <stdexcept>

#include "utils.hpp"
#include "Lexer.hpp"
#include "configutils.hpp"


// ===== ~TORS  =====
Lexer::Lexer(std::string conf_file_path): _state(GLOBAL), _raw_file_path(conf_file_path), _raw_conf_file(conf_file_path.c_str()) {
    std::cout << "Lexer name constructor called" << endofline;
    if (_raw_file_path.size() < 5 || _raw_file_path.substr(_raw_file_path.size() - 5) != ".conf")
        throw std::runtime_error ("Invalid file name : must end with .conf");
    if (_raw_conf_file.is_open() == false) 
        throw std::runtime_error ("Error opening the file : " + _raw_file_path);
}

Lexer::~Lexer() {
    std::cout << "Destructor called" << endofline;
}

// ===== UTILS FUNCTONS =====
// SPLITS WORDS AND SEPARATORS
static std::string addSpacesAroundSeparators(const std::string &line) {
    std::string result;
    for (size_t i = 0; i < line.size(); i++) {
        bool isSep = (line[i] == '{' || line[i] == '}' || line[i] == ';');
        if (isSep) result += ' ';
        result += line[i];
        if (isSep) result += ' ';
    }
    return result;
}

// ===== METHODS =====
// CREATES RAW TOKENS VECTORS
void    Lexer::initRawVector(void) {
    std::string line;

    // 
    while (std::getline(_raw_conf_file, line)) {

        size_t first = line.find_first_not_of(" \t");
        if (first == std::string::npos || line[first] == '#')
            continue;

        std::string token;
        std::istringstream clean_line(addSpacesAroundSeparators(line));
        while (clean_line >> token)
            _raw_tokens_vector.push_back(token);
    }
}

void    Lexer::initTokensVector(void) {

    for (size_t i = 0; i < _raw_tokens_vector.size(); i++) {
    }
}


// ===== TEST/OUTPUT =====
// PRINTS THE TOKEN VECTORS
void    Lexer::printTokens(void) const { 
    for (size_t i = 0; i < _raw_tokens_vector.size(); i++) {
        std::cout << "TOKEN NB ["<< i << "] = " <<  _raw_tokens_vector[i] << endofline;
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