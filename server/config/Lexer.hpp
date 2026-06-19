#ifndef LEXER_HPP
# define LEXER_HPP

#include <string>
#include <vector>
#include <fstream>
#include <sstream>

class Token {
    private:
        enum token_type { GLOBAL_KEY, SERVER_KEY, LOCATION_KEY, VALUE, SEPARATOR, DEFAULT}; // Defines the different token types

        std::string _value;
        token_type  _type;

    public:
        Token(std::string _value);
        ~Token();
        void printToken(void) const;
};
class Lexer {
    private:
        enum lexer_state { GLOBAL, SERVER, LOCATION}; // Defines the different states of the lexer / parser

        lexer_state                 _state;
        std::string                 _raw_file_path;
        std::ifstream               _raw_conf_file;
        std::vector<std::string>    _raw_tokens_vector;
        std::vector<Token>          _tokens_vector;


        // IGNORED CONSTRUCTORS
        Lexer(const Lexer &src);
        Lexer& operator= (const Lexer &other);

    public:
        Lexer(std::string conf_file_path);
        ~Lexer();

        void    initRawVector(void); // Creates the raw string vector for each word / separator
        void    initTokensVector(void); // Create the tokens with associated type

        // OUTPUTS / DEMOS
        void    printRawTokens(void) const ; 
        void    printRawConfFile(void) ; 
        void    printTokens(void) const ;

};

#endif
