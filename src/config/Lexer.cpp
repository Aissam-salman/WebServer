#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <cstdlib>
#include <stdexcept>

#include "utils.hpp"
#include "errors.hpp"
#include "Token.hpp"
#include "Lexer.hpp"
#include "configutils.hpp"

Lexer::Lexer(std::string conf_file_path): _raw_file_path(conf_file_path), _raw_conf_file(conf_file_path.c_str()) {
    if (_raw_file_path.size() < 5 || _raw_file_path.substr(_raw_file_path.size() - 5) != ".conf")
        throw std::runtime_error (ERRS_LEXER_BAD_EXTENSION);
    if (_raw_conf_file.is_open() == false)
        throw std::runtime_error (ERRS_LEXER_FILE_OPEN + _raw_file_path);
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

std::vector<Token> Lexer::getTokenVector(void) { return (_tokens_vector); }

// ===== METHODS =====
// CREATES STD::STRING _TOKENS_VECTORS
void    Lexer::initRawVector(void) {
    std::string line;
    int         line_nbr = 0;

    // GOES THROUGH THE FILE LINE BY LINE
    while (std::getline(_raw_conf_file, line)) {
        line_nbr++;

        // GETS THE FIRST NON WHITESPACE CHAR
        size_t first = line.find_first_not_of(" \t");
        if (first == std::string::npos || line[first] == '#')
            continue;

        
        // FORMATING THE LINE AND REMOVING ALL THE WHITESPACES -> ADD SPACES TO HAVE THEM EVERYWHERE FIRST
        std::istringstream clean_line(formatConfFile(line));

        std::string token_value;
        while (clean_line >> token_value) {
            Token new_token(token_value, line_nbr);
            if (isValidKey(new_token._value, SEPARATORS, SEPARATORS_SIZE) == true)
                new_token.setSeparatorType();
            else
                new_token._type = Token::WORD;
            _tokens_vector.push_back(new_token);
        }
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