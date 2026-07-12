#include <string>
#include <vector>
#include <map>

#include "utils.hpp"
#include "Token.hpp"
#include "configutils.hpp"

const std::string STATE_DIRECTIVES[] = { "server", "location" };
const size_t      STATE_DIRECTIVES_SIZE = sizeof(STATE_DIRECTIVES) / sizeof(STATE_DIRECTIVES[0]);

const std::string SERVER_DIRECTIVES[] = { "listen", "server_name", "client_max_body_size", "error_page"};
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

	error_pages[404] = "/errors/404.html";
	error_pages[500] = "/errors/5xx.html";
	error_pages[502] = "/errors/5xx.html";
	error_pages[503] = "/errors/5xx.html";
	error_pages[504] = "/errors/5xx.html";
	return (error_pages);
}