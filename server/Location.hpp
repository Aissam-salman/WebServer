#ifndef LOCATION_HPP
# define LOCATION_HPP

#include <string>

#include "utils.hpp"

class Location {
	private:
		std::string				    _name;
		long						_max_body_size;
		std::string				    _root_path;
		std::string				    _index_path;
		bool						_autoindex;
		int							_methods_flag; // SETS BITS CORRESPONDING TO ALLOWED METHODS

	public:

		Location(std::string name);
		Location(const Location &src);
		Location& operator= (const Location &other);
		~Location();

		void	printLocation(void);
		void	setMethods(e_methods method);
};

#endif
