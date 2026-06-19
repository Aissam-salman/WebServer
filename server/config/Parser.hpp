#ifndef PARSER_HPP
#define PARSER_HPP

#include <vector>

#include "Server.hpp"
#include "Token.hpp"

class Parser {
private:
	enum parser_state { GLOBAL, SERVER, LOCATION }; // Defines the different states of the lexer / parser

	parser_state			_state;
	std::vector<Token>		_tokens_vector;
	Server&					_server;

	// UNINITIALIZED ~Tors
	Parser &operator=(const Parser &other);
	Parser(const Parser &src);

public:
	Parser(std::vector<Token> tokens_vector, Server& server);
	~Parser();

	void setState(parser_state state);
	std::vector<Token> getTokenVector(void);

	void parsingTokensVector(void);
	void affectState(Token &token, size_t &index);
	void interpretState(Token &token, size_t &index, Server& server);

	void printState(void);
};

#endif
