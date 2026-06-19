#include <string>
#include <vector>
#include <map>

#include "utils.hpp"
#include "Token.hpp"
#include "configutils.hpp"

const std::string GLOBAL_DIRECTIVES[] = { "server" };
const size_t      GLOBAL_DIRECTIVES_SIZE = sizeof(GLOBAL_DIRECTIVES) / sizeof(GLOBAL_DIRECTIVES[0]);

const std::string SERVER_DIRECTIVES[] = { "listen", "server_name", "client_max_body_size", "error_page", "location" };
const size_t      SERVER_DIRECTIVES_SIZE = sizeof(SERVER_DIRECTIVES) / sizeof(SERVER_DIRECTIVES[0]);

const std::string LOCATION_DIRECTIVES[] = { "root", "index", "autoindex", "methods", "upload_dir", "cgi", "return", "client_max_body_size" };
const size_t      LOCATION_DIRECTIVES_SIZE = sizeof(LOCATION_DIRECTIVES) / sizeof(LOCATION_DIRECTIVES[0]);

const std::string SEPARATORS[] = {"{", "}", ";"};
const size_t      SEPARATORS_SIZE = sizeof(SEPARATORS) / sizeof(SEPARATORS[0]);

Token&	getTokenAtIndex(std::vector<Token> &tokens_vector, size_t index) {
	return (tokens_vector[index]);
}

// INITIALISE LES ERROR PAGES PAR DEFAUT
MapIntStr initErrorPages(void) {
	MapIntStr error_pages;
	return (error_pages);
}