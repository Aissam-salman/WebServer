#ifndef CONFIGUTILS_HPP
# define CONFIGUTILS_HPP

#include <string>
#include <cstddef>

extern const std::string GLOBAL_KEY[];
extern const size_t      GLOBAL_KEY_SIZE;

extern const std::string SERVER_KEY[];
extern const size_t      SERVER_KEY_SIZE;

extern const std::string LOCATION_KEY[];
extern const size_t      LOCATION_KEY_SIZE;

// std::map<int, std::string> initErrorPages(void); // TODO : RAJOUTER NUMERO ERREUR AVEC PAGE
#endif