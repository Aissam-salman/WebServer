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
		bool						_return;
		std::string					_return_path;

	public:

		Location(std::string name, long max_body_size);
		Location(const Location &src);
		Location& operator= (const Location &other);
		~Location();

		// SETTERS
		void	setName(std::string name);
		void	setMaxBodySize(long size);
		void	setRootPath(std::string path);
		void	setIndexPath(std::string path);
		void	setAutoIndex(bool state);
		void	setMethods(e_methods method);
		void	setReturn(bool state);
		void	setReturnPath(std::string path);

		// GETTERS
		std::string&	getName(void);
		long			getMaxBodySize(void) const;
		std::string&	getRootPath(void);
		std::string&	getIndexPath(void);
		bool			getAutoIndex(void) const;
		int&			getMethodFlag(void);
		bool			getReturn(void) const;
		std::string&	getReturnPath(void);

		void	printLocation(void) const;
};

#endif
