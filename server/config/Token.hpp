#ifndef TOKEN_HPP
# define TOKEN_HPP

#include <string>
#include <vector>

struct Token {
    enum token_type { DIRECTIVE, PARAMETER, OPEN_BRACKET, CLOSED_BRACKET, SEMICOLON, DEFAULT };

    std::string _value;
    token_type  _type;

    Token(std::string value);
    ~Token();
    void    printToken(void) const;
    void    setSeparatorType(void);
};

void    printTokens(std::vector<Token> tokens_vector);

#endif
