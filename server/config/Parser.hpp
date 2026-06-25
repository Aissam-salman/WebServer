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
	std::vector<Server>&	_servers_vector;

	// UNINITIALIZED ~Tors
	Parser &operator=(const Parser &other);
	Parser(const Parser &src);


public:
	Parser(std::vector<Token> tokens_vector, std::vector<Server> &servers_vector);
	~Parser();

	void 				setState(parser_state state);

	std::vector<Token>	getTokenVector(void);
	std::vector<Token>	findNextSemicolon(size_t index);

	void				lowerScope(void);
	void				upperScope(void);

	void				parseDirective(size_t& index);
	void				parseStateDirective(size_t& index);
	void				findDirectiveTokenVector(size_t& index);

	void	initServers(void) ;
	void printState(void);
};

struct Transition {
    Parser::parser_state  current_state;
    Token::token_type     token_type;
    Parser::parser_state  next_state;
};

#endif
