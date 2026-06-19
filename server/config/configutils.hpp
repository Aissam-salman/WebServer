#ifndef CONFIGUTILS_HPP
# define CONFIGUTILS_HPP

#include <string>
#include <cstddef>

extern const std::string GLOBAL_KEYS[];
extern const size_t      GLOBAL_KEYS_SIZE;

extern const std::string SERVER_KEYS[];
extern const size_t      SERVER_KEYS_SIZE;

extern const std::string LOCATION_KEYS[];
extern const size_t      LOCATION_KEYS_SIZE;

// std::map<int, std::string> initErrorPages(void); // TODO : RAJOUTER NUMERO ERREUR AVEC PAGE
#endif