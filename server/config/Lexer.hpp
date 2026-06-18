#ifndef LEXER_HPP
# define LEXER_HPP

#include <string>
#include <vector>
#include <fstream>
#include <sstream>

class Token {
    private:
        enum token_type { KEY, VALUE, SEPARATOR}; // Defines the different token types

        std::string _value;
        // token_type  _type;

    public:
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
        void    printTokens(void) const ; 
        void    printRawConfFile(void) ; 

};

#endif
