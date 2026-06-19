#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <stdexcept>

#include "utils.hpp"
#include "Token.hpp"
#include "Lexer.hpp"
#include "configutils.hpp"

Lexer::Lexer(std::string conf_file_path): _raw_file_path(conf_file_path), _raw_conf_file(conf_file_path.c_str()) {
    std::cout << "Lexer name constructor called" << endofline;
    if (_raw_file_path.size() < 5 || _raw_file_path.substr(_raw_file_path.size() - 5) != ".conf")
        throw std::runtime_error ("Invalid file name : must end with .conf");
    if (_raw_conf_file.is_open() == false) 
        throw std::runtime_error ("Error opening the file : " + _raw_file_path);
}

Lexer::~Lexer() { }

// ===== UTILS FUNCTONS =====
// SPLITS WORDS AND SEPARATORS
static std::string formatConfFile(const std::string &line) {
    std::string result;
    for (size_t i = 0; i < line.size(); i++) {
        bool isSep = (line[i] == '{' || line[i] == '}' || line[i] == ';');
        if (isSep) result += ' ';
        result += line[i];
        if (isSep) result += ' ';
    }
    return result;
}

std::vector<Token> Lexer::getTokenVector(void) {
    return (_tokens_vector);
}

// ===== METHODS =====
// CREATES STD::STRING RAW_TOKENS_VECTORS
void    Lexer::initRawVector(void) {
    std::string line;

    // 
    while (std::getline(_raw_conf_file, line)) {

        size_t first = line.find_first_not_of(" \t");
        if (first == std::string::npos || line[first] == '#')
            continue;

        std::string token;
        std::istringstream clean_line(formatConfFile(line));
        while (clean_line >> token)
            _raw_tokens_vector.push_back(token);
    }
}

// TAKES STD::STRING RAW_TOKENS_VECTOR -> TURNS INTO PRE-TYPES TOKENS
void    Lexer::initTokensVector(void) {
    for (size_t i = 0; i < _raw_tokens_vector.size(); i++) {
        Token token(_raw_tokens_vector[i]);
        if (isValidKey(token._value, SEPARATORS, SEPARATORS_SIZE) == true)
            token.setSeparatorType();
        else if (isValidKey(token._value, GLOBAL_DIRECTIVES, GLOBAL_DIRECTIVES_SIZE) == true)
            token._type = Token::DIRECTIVE;
        else if (isValidKey(token._value, SERVER_DIRECTIVES, SERVER_DIRECTIVES_SIZE) == true)
            token._type = Token::DIRECTIVE;
        else if (isValidKey(token._value, LOCATION_DIRECTIVES, LOCATION_DIRECTIVES_SIZE) == true)
            token._type = Token::DIRECTIVE;
        else
            token._type = Token::PARAMETER;
        _tokens_vector.push_back(token);
    }
}


// ===== TEST/OUTPUT =====
void    Lexer::printRawTokens(void) const { 
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