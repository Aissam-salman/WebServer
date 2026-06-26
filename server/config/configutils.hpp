#ifndef CONFIGUTILS_HPP
# define CONFIGUTILS_HPP

#include <string>
#include <vector>
#include <cstddef>

#include "Token.hpp"

# ifndef DEBUG
# define DEBUG 0
# endif

extern const std::string STATE_DIRECTIVES[];
extern const size_t      STATE_DIRECTIVES_SIZE;

extern const std::string SERVER_DIRECTIVES[];
extern const size_t      SERVER_DIRECTIVES_SIZE;

extern const std::string LOCATION_DIRECTIVES[];
extern const size_t      LOCATION_DIRECTIVES_SIZE;

extern const std::string SEPARATORS[];
extern const size_t      SEPARATORS_SIZE;


// std::map<int, std::string> initErrorPages(void); // TODO : RAJOUTER NUMERO ERREUR AVEC PAGE
Token&	getTokenAtIndex(std::vector<Token> &tokens_vector, size_t index) ;

#endif
