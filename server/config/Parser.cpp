#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <vector>

#include "Token.hpp"
#include "Parser.hpp"
#include "Server.hpp"
#include "utils.hpp"
#include "configutils.hpp"

// TODO: wire this into a nextState() lookup, then uncomment
// static const Transition transitions[] = {
//     { Parser::GLOBAL,   Token::OPEN_BRACKET,   Parser::SERVER   },
//     { Parser::SERVER,   Token::OPEN_BRACKET,   Parser::LOCATION },
//     { Parser::LOCATION, Token::CLOSED_BRACKET, Parser::SERVER   },
//     { Parser::SERVER,   Token::CLOSED_BRACKET, Parser::GLOBAL   },
// };

Parser::Parser(std::vector<Token> tokens_vector, std::vector<Server> &servers_vector): _state(GLOBAL), _tokens_vector(tokens_vector), _servers_vector(servers_vector) {
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

std::vector<Token>	Parser::findNextSemicolon(size_t index) {
	std::vector<Token> directive_token_vector;

	for (size_t i = index; i < _tokens_vector.size(); i++) {
		directive_token_vector.push_back(_tokens_vector[i]);
		if (_tokens_vector[i]._type == Token::SEMICOLON) {
			return (directive_token_vector);
		}
	}
	throw std::runtime_error("Invalid conf file, reaching the end of file without finding end of directive");
}

void	Parser::lowerScope(void) {
	std::cout << BOLD_WHITE << "LOWERING SCOPE" << endofline;
	if (_state == SERVER)
		_state = GLOBAL;
	else if (_state == LOCATION)
		_state = SERVER;
	else
		throw std::runtime_error ("Conf file went out of scope"); // TODO : Modify error message for more accurate
}

void	Parser::upperScope(void) {
	std::cout << BOLD_WHITE << "UPPING SCOPE" << endofline;
	if (_state == SERVER)
		_state = LOCATION;
	else if (_state == GLOBAL)
		_state = SERVER;
	else
		throw std::runtime_error ("Conf file went out of scope"); // TODO : Modify error message for more accurate
}
void	Parser::findDirectiveTokenVector(size_t& index) {
	std::vector<Token> directive_token_vector = findNextSemicolon(index);

	std::cout << BOLD_RED << "[ DIRECTIVE = " << _tokens_vector[index]._value << " AT LINE " << _tokens_vector[index]._line << "]" << endofline;
	for (size_t i = 0; i < directive_token_vector.size(); i++) {
		directive_token_vector[i].printToken();
	}
	// land on the ';' (last consumed token); the caller's for-loop i++ moves past it
	index += directive_token_vector.size() - 1;
}
// Validates the directive against the current scope, then consumes it.
// The scope dictates which directive table is legal here, which also
// resolves directives that are valid in more than one scope (e.g.
// client_max_body_size, allowed in both server and location).
void	Parser::parseDirective(size_t& index) {
	const std::string& key = _tokens_vector[index]._value;

	switch (_state) {
		case GLOBAL:
			throw std::runtime_error("Directive '" + key + "' found outside any block");
		case SERVER:
			if (!isValidKey(key, SERVER_DIRECTIVES, SERVER_DIRECTIVES_SIZE))
				throw std::runtime_error("Directive '" + key + "' is not valid in server scope");
			break;
		case LOCATION:
			if (!isValidKey(key, LOCATION_DIRECTIVES, LOCATION_DIRECTIVES_SIZE))
				throw std::runtime_error("Directive '" + key + "' is not valid in location scope");
			break;
	}
	findDirectiveTokenVector(index);
}

void	Parser::parseStateDirective(size_t& index) {
	if (_tokens_vector[index]._value == "server" && _tokens_vector[index + 1]._type == Token::OPEN_BRACKET) {
		Server new_server;
		_servers_vector.push_back(new_server);
		upperScope();
		index++;
	}
	if (_tokens_vector[index]._value == "location" && _tokens_vector[index + 2]._type == Token::OPEN_BRACKET) {
		Location new_location(_tokens_vector[index + 1]._value);
		_servers_vector.back().addLocation(new_location);
		upperScope();
		index += 2;
	}
}

// MAIN PARSING FUNCTION -> CREATING THE CLASSES
void	Parser::initServers(void) {
	for (size_t i = 0; i < _tokens_vector.size(); i++) {
		if (_tokens_vector[i]._type == Token::CLOSED_BRACKET)
			lowerScope();
		else if (isValidKey(_tokens_vector[i]._value, STATE_DIRECTIVES, STATE_DIRECTIVES_SIZE))
			parseStateDirective(i);
		else
			parseDirective(i);
	}
	if (_state != GLOBAL)
		throw std::runtime_error ("Incomplete conf file");
}

// ===== METHODS =====
// void	Parser::affectState(Token &token, size_t &index) {
// 	switch(_state) {
// 		case GLOBAL : {
//             break;
// 		}
// 		case SERVER : {
//             break;
// 		}
// 		case LOCATION : {
// 			break;
// 		}
// 	}
// }

// void	Parser::interpretState(Token &token, size_t &index, Server& server) {
// 	switch (_state) {
// 		case GLOBAL : {
// 			break;
// 		}
// 		case SERVER : {
// 			break;
// 		}
// 		case LOCATION : {
// 			break;
// 		}
// 	}
// }

// void	Parser::parsingTokensVector(void) {
// 	for (size_t i = 0; i < _tokens_vector.size(); i++) {
// 		affectState(_tokens_vector[i], i);
// 		interpretState(_tokens_vector[i], i, _server);
// 	}
// }

// ===== TEST/OUTPUT =====
void	Parser::printState(void) {
	switch (_state) {
		case GLOBAL : display("GLOBAL"); break;
		case SERVER : display("SERVER"); break;
		case LOCATION : display("LOCATION"); break;
	}
}