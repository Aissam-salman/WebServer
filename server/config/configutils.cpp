#include <string>
#include <map>

#include "utils.hpp"
#include "configutils.hpp"

const std::string GLOBAL_KEYS[] = { "server" };
const size_t      GLOBAL_KEYS_SIZE = sizeof(GLOBAL_KEYS) / sizeof(GLOBAL_KEYS[0]);

const std::string SERVER_KEYS[] = { "listen", "server_name", "client_max_body_size", "error_page", "location" };
const size_t      SERVER_KEYS_SIZE = sizeof(SERVER_KEYS) / sizeof(SERVER_KEYS[0]);

const std::string LOCATION_KEYS[] = { "root", "index", "autoindex", "methods", "upload_dir", "cgi", "return", "client_max_body_size" };
const size_t      LOCATION_KEYS_SIZE = sizeof(LOCATION_KEYS) / sizeof(LOCATION_KEYS[0]);

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