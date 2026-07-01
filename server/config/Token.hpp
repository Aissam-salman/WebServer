#ifndef TOKEN_HPP
# define TOKEN_HPP

#include <string>
#include <vector>

struct Token {
    enum token_type {WORD, DIRECTIVE, PARAMETER_STR, PARAMETER_NBR, OPEN_BRACKET, CLOSED_BRACKET, SEMICOLON, DEFAULT };

    std::string _value;
    token_type  _type;
    int         _line;

    Token(std::string value, int line_nbr);
    ~Token();
    void    printToken(void) const;
    void    setSeparatorType(void);
};

void    printTokens(std::vector<Token> tokens_vector);

#endif
