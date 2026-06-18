#include <string>

bool	checkConfFilepath(std::string filepath) {
	if (filepath.size() < 5)
		return (false);
	return (filepath.substr(filepath.size() - 5) == ".conf" );
}