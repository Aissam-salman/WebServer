#include <string>
#include <map>

#include "utils.hpp"
#include "configutils.hpp"

const std::string GLOBAL_KEY[] = { "server" };
const size_t      GLOBAL_KEY_SIZE = sizeof(GLOBAL_KEY) / sizeof(GLOBAL_KEY[0]);

const std::string SERVER_KEY[] = { "listen", "server_name", "client_max_body_size", "error_page", "location" };
const size_t      SERVER_KEY_SIZE = sizeof(SERVER_KEY) / sizeof(SERVER_KEY[0]);

const std::string LOCATION_KEY[] = { "root", "index", "autoindex", "methods", "upload_dir", "cgi", "return", "client_max_body_size" };
const size_t      LOCATION_KEY_SIZE = sizeof(LOCATION_KEY) / sizeof(LOCATION_KEY[0]);

// bool	checkConfFilepath(std::string filepath) {
// 	if (filepath.size() < 5)
// 		return (false);
// 	return (filepath.substr(filepath.size() - 5) == ".conf" );
// }

// INITIALISE LES ERROR PAGES PAR DEFAUT
std::map<int, std::string> initErrorPages(void) {

	std::map<int, std::string> error_pages;

	return (error_pages);
}