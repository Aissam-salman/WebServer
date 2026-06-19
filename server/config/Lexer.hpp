#ifndef LEXER_HPP
# define LEXER_HPP

#include <string>
#include <vector>
#include <fstream>

#include "Token.hpp"
class Lexer {
    private:
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
        std::vector<Token> getTokenVector(void);

        // OUTPUTS / DEMOS
        void    printRawTokens(void) const ; 
        void    printRawConfFile(void) ; 
};

#endif
