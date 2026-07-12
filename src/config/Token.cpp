#include <iostream>

#include "utils.hpp"
#include "Token.hpp"

Token::Token(std::string value, int line_nbr): _value(value), _type(DEFAULT), _line(line_nbr) {}

Token::~Token() {}

void    Token::setSeparatorType(void) {
    if (_value == "{")
        _type = OPEN_BRACKET;
    else if (_value == "}")
        _type = CLOSED_BRACKET;
    else if (_value == ";")
        _type = SEMICOLON;
}


void    Token::printToken(void) const {
    display("Token value = " + _value);
    std::cout << "Token type = ";
    switch (_type) {
        case WORD:                  display("WORD"); break;
        case OPEN_BRACKET:          display("OPEN_BRACKET"); break;
        case CLOSED_BRACKET:        display("CLOSED_BRACKET"); break;
        case SEMICOLON:             display("SEMICOLON"); break;
        case DEFAULT:               display("DEFAULT"); break;
        case DIRECTIVE:             display("DIRECTIVE"); break;
        case PARAMETER_STR:         display("PARAMETER_STR"); break;
        case PARAMETER_NBR:         display("PARAMETER_NBR"); break;
    }
    std::cout << BOLD_MAGENTA << "[LINE " << _line << "]\n" << endofline;
}

void    printTokens(std::vector<Token> tokens_vector) {
    for (size_t i = 0; i < tokens_vector.size(); i++) {
        tokens_vector[i].printToken();
    }
}
