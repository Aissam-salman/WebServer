#ifndef LOCATION_HPP
# define LOCATION_HPP

#include <string>
#include <vector>
#include "utils.hpp"

class Location {
	private:
		std::string				    _name;
		std::string				    _root_path;
		std::string				    _index_path;
		std::vector<e_methods>	    _allowed_methods;
		long						_max_body_size;
		bool						_autoindex;

	public:

		Location(std::string name);
		Location(const Location &src);
		Location& operator= (const Location &other);
		~Location();
};

#endif
