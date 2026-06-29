#ifndef PARSER_HPP
#define PARSER_HPP

#include <vector>

#include "Server.hpp"
#include "Token.hpp"

class Parser {
public:
	enum parser_state { GLOBAL, SERVER, LOCATION }; // Defines the different states of the lexer / parser

private:
	parser_state			_state;
	std::vector<Token>		_tokens_vector;
	std::vector<Token>		_temp_vector;
	std::vector<Server>&	_servers_vector;

	// UNINITIALIZED ~Tors
	Parser &operator=(const Parser &other);
	Parser(const Parser &src);


public:
	Parser	(std::vector<Token> tokens_vector, std::vector<Server> &servers_vector);
	~Parser	();

	std::vector<Token>	getTokenVector(void);
	std::vector<Token>	findNextSemicolon(size_t index);


	// GLOBAL METHODS
	void				parseDirective(size_t& index);
	void				parseStateDirective(size_t& index);

	// DIRECTIVE SETUP
	void				setupListen(void);
	void				setupServerName(void);
	void				setupMaxBodySize(void);
	void				setupErrorPages(void);
	void				setupRoot(void); // ATTENTION, PAS DE CHECK EXISTING
	void				setupIndex(void); // ATTENTION, PAS DE CHECK EXISTING
	void				setupAutoIndex(void);
	void				setupMethods(void);
	// void				setupUploadDir(void);
	void				setupCGI(void);
	void				setupReturn(void);
	void				findDirectiveTokenVector(size_t& index);

	// UTILS METHODS
	void				expectSingleValue(void);
	void				setupAddMethod(std::string method);
	void				lowerScope(void);
	void				upperScope(void);
	void				initServers(void) ;
	void 				printState(void);
	Server&				getCurrentServer(void);
	Location&			getCurrentLocation(void);
};

#endif
