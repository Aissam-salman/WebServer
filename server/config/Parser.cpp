#include <cstddef>
#include <cstdlib>
#include <cctype>
#include <cerrno>
#include <climits>
#include <iostream>
#include <stdexcept>
#include <vector>

#include "Token.hpp"
#include "Parser.hpp"
#include "Server.hpp"
#include "utils.hpp"
#include "errors.hpp"
#include "configutils.hpp"



static std::map<std::string, e_methods> makeMethodMap(void) {
	std::map<std::string, e_methods> m;
	m["GET"]     = GET;
	m["HEAD"]    = HEAD;
	m["POST"]    = POST;
	m["PUT"]     = PUT;
	m["DELETE"]  = DELETE;
	m["PATCH"]   = PATCH;
	m["OPTIONS"] = OPTIONS;
	m["CONNECT"] = CONNECT;
	m["TRACE"]   = TRACE;
	return m;
}

static const std::map<std::string, e_methods> MethodMap = makeMethodMap();

Parser::Parser(std::vector<Token> tokens_vector, std::vector<Server> &servers_vector): _state(GLOBAL), _tokens_vector(tokens_vector), _servers_vector(servers_vector) {
	std::cout << "Parser constructor called" << std::endl;
}

Parser::~Parser() {
	std::cout << "Parser Destructor called" << std::endl;
}

// ===== UTILS FUNCTONS =====
std::vector<Token> Parser::getTokenVector(void) { return (_tokens_vector); }

Server&             Parser::getCurrentServer(void) { return (_servers_vector.back()); }

Location&           Parser::getCurrentLocation(void) { return (_servers_vector.back().getCurrentLocation()); }


void	Parser::lowerScope(void) {
	std::cout << BOLD_WHITE << "LOWERING SCOPE" << endofline;
	if (_state == SERVER)
		_state = GLOBAL;
	else if (_state == LOCATION)
		_state = SERVER;
	else
		throw std::runtime_error (ERRS_PARSER_SCOPE_UNDERFLOW);
}

void	Parser::upperScope(void) {
	std::cout << BOLD_WHITE << "UPPING SCOPE" << endofline;
	if (_state == SERVER)
		_state = LOCATION;
	else if (_state == GLOBAL)
		_state = SERVER;
	else
		throw std::runtime_error (ERRS_PARSER_SCOPE_OVERFLOW);
}

// ===== METHODS =====
// PARSES DIRECTIVE SERVER / LOCATION
std::vector<Token>	Parser::findNextSemicolon(size_t index) {

	for (size_t i = index; i < _tokens_vector.size(); i++) {
		_temp_vector.push_back(_tokens_vector[i]);
		if (_tokens_vector[i]._type == Token::SEMICOLON) {
			return (_temp_vector);
		}
	}
	throw std::runtime_error(ERRS_PARSER_UNTERMINATED_DIRECTIVE);
}

void	Parser::parseStateDirective(size_t& index) {
	const std::string& key = _tokens_vector[index]._value;

	if (_tokens_vector[index]._value == "server" && _tokens_vector[index + 1]._type == Token::OPEN_BRACKET) {
		if (_state != GLOBAL)
				throw std::runtime_error(ERRS_PARSER_DIRECTIVE_PREFIX + key + ERRS_PARSER_DIRECTIVE_IN_LOCATION);

		Server new_server;
		_servers_vector.push_back(new_server);
		upperScope();
		index++;
	}

	else if (_tokens_vector[index]._value == "location" && _tokens_vector[index + 2]._type == Token::OPEN_BRACKET) {
		if (_state != SERVER)
				throw std::runtime_error(ERRS_PARSER_DIRECTIVE_PREFIX + key + ERRS_PARSER_DIRECTIVE_IN_LOCATION);
		// else if () // TODO : Check if folder doesn´t exist already

		std::vector<Location>& locations_vector = getCurrentServer().getServerLocationsVector();
		for (size_t i = 0; i < locations_vector.size(); i++) {
			if (locations_vector[i].getName() == _tokens_vector[index + 1]._value)
				throw std::runtime_error ("A location already exists with name " + _tokens_vector[index + 1]._value);
		}
		Location new_location(_tokens_vector[index + 1]._value, getCurrentServer().getMaxBodySize());
		_servers_vector.back().addLocation(new_location);
		upperScope();
		index += 2;
	}
	else
		throw std::runtime_error (ERRS_PARSER_INVALID_SYNTAX + key);
}


// ===== DIRECTIVES FUNCTIONS =====
// directive + value + ';' == 3 tokens
void	Parser::expectSingleValue(void) {
	if (_temp_vector.size() != 3)
		throw std::runtime_error (ERRS_PARSER_INVALID_SYNTAX + _temp_vector[0]._value);
}

void	Parser::setupMethods(void) {

	for (size_t i = 1; i + 1 < _temp_vector.size(); i++) {
			std::map<std::string, e_methods>::const_iterator it =
					MethodMap.find(_temp_vector[i]._value);
			if (it == MethodMap.end())
					throw std::runtime_error(ERRS_PARSER_INVALID_METHOD_PREFIX
							+ _temp_vector[i]._value + ERRS_PARSER_INVALID_METHOD_SUFFIX);
			getCurrentLocation().getMethodFlag() |= it->second;   // duplicate method = harmless no-op
	}
}

void				Parser::setupMaxBodySize(void) {
	expectSingleValue();

	const std::string& value = _temp_vector[1]._value;

	errno = 0;
	char* end = NULL;
	long  number = std::strtol(value.c_str(), &end, 10);

	if (end == value.c_str())                     
		throw std::runtime_error(ERRS_PARSER_INVALID_SYNTAX + value);
	if (errno == ERANGE || number < 0)
		throw std::runtime_error(ERRS_PARSER_INVALID_SYNTAX + value);

	long multiplier = 1;
	if (end[0] != '\0' && end[1] == '\0') {
		switch (std::toupper(static_cast<unsigned char>(end[0]))) {
			case 'K': multiplier = 1024L; break;
			case 'M': multiplier = 1024L * 1024; break;
			case 'G': multiplier = 1024L * 1024 * 1024; break;
			default : throw std::runtime_error(ERRS_PARSER_INVALID_SYNTAX + value);
		}
	}
	else if (end[0] != '\0')                    
		throw std::runtime_error(ERRS_PARSER_INVALID_SYNTAX + value);

	if (number > LONG_MAX / multiplier)           
		throw std::runtime_error(ERRS_PARSER_INVALID_SYNTAX + value);

	long result = number * multiplier;
	if (_state == LOCATION)
		getCurrentLocation().setMaxBodySize(result);
	else
		getCurrentServer().setMaxBodySize(result);
}

void	Parser::setupListen(void) {
	expectSingleValue();

	// CHECK FORMAT : 2 DISPO Soit X.X.X.X:PORT || PORT

	Socket new_socket;
	// new_socket.setSocket(PORT);
	getCurrentServer().getSockets().push_back(new_socket);

}

void				Parser::setupAutoIndex(void) {
	expectSingleValue();

	if (_temp_vector[1]._value == "off")
		getCurrentLocation().setAutoIndex(false);
	else if (_temp_vector[1]._value == "on")
		getCurrentLocation().setAutoIndex(true);
	else
		throw std::runtime_error (ERRS_PARSER_INVALID_AUTOINDEX);
}

void				Parser::setupServerName(void) {
	expectSingleValue();

	if (getCurrentServer().getServerName() != "")
		throw std::runtime_error (ERRS_PARSER_SERVER_NAME_ALREADY_SET_PREFIX
				+ getCurrentServer().getServerName() + ERRS_PARSER_SERVER_NAME_SUFFIX);

	for (size_t i = 0; i < _servers_vector.size(); i++) {
		if (_servers_vector[i].getServerName() == _temp_vector[1]._value)
			throw std::runtime_error (ERRS_PARSER_DUPLICATE_SERVER_NAME_PREFIX
					+ _temp_vector[1]._value + ERRS_PARSER_SERVER_NAME_SUFFIX);
	}

	getCurrentServer().setServerName(_temp_vector[1]._value);
}

// RETURNS THE VECTOR OF TOKENS FOR THE DIRECTIVE TO TREAT
void	Parser::findDirectiveTokenVector(size_t& index) {
	findNextSemicolon(index);

	std::cout << BOLD_RED << "[ DIRECTIVE = " << _tokens_vector[index]._value << " AT LINE " << _tokens_vector[index]._line << "]" << endofline;
	for (size_t i = 0; i < _temp_vector.size(); i++) {
		_temp_vector[i].printToken();
	}
	index += _temp_vector.size() - 1;
}


void	Parser::parseDirective(size_t& index) {
	const std::string& key = _tokens_vector[index]._value;

	switch (_state) {
		case GLOBAL:
			throw std::runtime_error(ERRS_PARSER_DIRECTIVE_PREFIX + key + ERRS_PARSER_DIRECTIVE_OUTSIDE_BLOCK);
		case SERVER:
			if (!isValidKey(key, SERVER_DIRECTIVES, SERVER_DIRECTIVES_SIZE))
				throw std::runtime_error(ERRS_PARSER_DIRECTIVE_PREFIX + key + ERRS_PARSER_DIRECTIVE_IN_SERVER);
			break;
		case LOCATION:
			if (!isValidKey(key, LOCATION_DIRECTIVES, LOCATION_DIRECTIVES_SIZE))
				throw std::runtime_error(ERRS_PARSER_DIRECTIVE_PREFIX + key + ERRS_PARSER_DIRECTIVE_IN_LOCATION);
			break;
	}
	findDirectiveTokenVector(index);
	if (_temp_vector[0]._value == "methods")
		setupMethods();
	else if (_temp_vector[0]._value == "autoindex")
		setupAutoIndex();
	else if (_temp_vector[0]._value == "server_name")
		setupServerName();
	else if (_temp_vector[0]._value == "client_max_body_size")
		setupMaxBodySize();
	else if (_temp_vector[0]._value == "listen")
		setupListen();
	// Fill appropriate field if valid
}

// MAIN PARSING FUNCTION -> CREATING THE CLASSES
void	Parser::initServers(void) {
	for (size_t i = 0; i < _tokens_vector.size(); i++) {
		if (_tokens_vector[i]._type == Token::CLOSED_BRACKET)
			lowerScope();
		else if (isValidKey(_tokens_vector[i]._value, STATE_DIRECTIVES, STATE_DIRECTIVES_SIZE))
			parseStateDirective(i);
		else if (isValidKey(_tokens_vector[i]._value, SERVER_DIRECTIVES, SERVER_DIRECTIVES_SIZE) || isValidKey(_tokens_vector[i]._value, LOCATION_DIRECTIVES, LOCATION_DIRECTIVES_SIZE))
			parseDirective(i);
		else
			throw std::runtime_error (ERRS_PARSER_INVALID_DIRECTIVE_PREFIX + _tokens_vector[i]._value + ERRS_PARSER_INVALID_DIRECTIVE_SUFFIX);
		_temp_vector.clear();
	}
	if (_state != GLOBAL)
		throw std::runtime_error (ERRS_PARSER_INCOMPLETE_FILE);
	else if (_servers_vector.size() < 1) {
		throw std::runtime_error (ERRS_PARSER_NO_SERVER);
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