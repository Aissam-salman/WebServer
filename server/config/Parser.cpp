#include <iostream>
#include <stdexcept>
#include <vector>

#include "Token.hpp"
#include "Parser.hpp"
#include "Server.hpp"
#include "utils.hpp"
#include "configutils.hpp"

Parser::Parser(std::vector<Token> tokens_vector, Server& server): _state(GLOBAL), _tokens_vector(tokens_vector), _server(server) {
	std::cout << "Parser constructor called" << std::endl;
}

Parser::~Parser() {
	std::cout << "Parser Destructor called" << std::endl;
}

// ===== UTILS FUNCTONS =====
void	Parser::setState(parser_state state) {
	_state = state;
}

std::vector<Token> Parser::getTokenVector(void) {
	return (_tokens_vector);
}

// ===== METHODS =====
void	Parser::affectState(Token &token, size_t &index) {
	switch(_state) {
		case GLOBAL : {
            break;
		}
		case SERVER : {
            break;
		}
		case LOCATION : {
			break;
		}
	}
}

void	Parser::interpretState(Token &token, size_t &index, Server& server) {
	switch (_state) {
		case GLOBAL : {
			break;
		}
		case SERVER : {
			break;
		}
		case LOCATION : {
			break;
		}
	}
}

void	Parser::parsingTokensVector(void) {
	for (size_t i = 0; i < _tokens_vector.size(); i++) {
		affectState(_tokens_vector[i], i);
		interpretState(_tokens_vector[i], i, _server);
	}
}

// ===== TEST/OUTPUT =====
void	Parser::printState(void) {
	switch (_state) {
		case GLOBAL : display("GLOBAL"); break;
		case SERVER : display("SERVER"); break;
		case LOCATION : display("LOCATION"); break;
	}
}