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
	// std::cout << BOLD_WHITE << "LOWERING SCOPE" << endofline;
	if (_state == SERVER)
		_state = GLOBAL;
	else if (_state == LOCATION)
		_state = SERVER;
	else
		throw std::runtime_error (ERRS_PARSER_SCOPE_UNDERFLOW);
}

void	Parser::upperScope(void) {
	// std::cout << BOLD_WHITE << "UPPING SCOPE" << endofline;
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
		// TODO : Check if folder doesn´t exist already

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

void				Parser::setupRoot(void) {
	expectSingleValue();

	// std::cout << BOLD_GREEN << "ROOT = " << _temp_vector[1]._value << endofline;
	getCurrentLocation().setRootPath(_temp_vector[1]._value);
}

void				Parser::setupIndex(void) {
	expectSingleValue();

	// std::cout << BOLD_GREEN << "INDEX = " << _temp_vector[1]._value << endofline;
	getCurrentLocation().setIndexPath(_temp_vector[1]._value);
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

static bool	isValidIP(const std::string& host) {
	int		octets = 0;
	size_t	i = 0;

	while (i < host.size()) {
		size_t	start = i;
		int		value = 0;

		while (i < host.size() && std::isdigit(static_cast<unsigned char>(host[i]))) {
			value = value * 10 + (host[i] - '0');
			if (value > 255)
				return (false);
			i++;
		}
		if (i == start || i - start > 3)
			return (false);
		octets++;
		if (i < host.size()) {
			if (host[i] != '.' || i + 1 == host.size())
				return (false);
			i++;
		}
	}
	return (octets == 4);
}

static int	parsePort(const std::string& port_str) {
	if (port_str.empty())
		throw std::runtime_error(ERRS_PARSER_INVALID_PORT + port_str);
	for (size_t i = 0; i < port_str.size(); i++) {
		if (!std::isdigit(static_cast<unsigned char>(port_str[i])))
			throw std::runtime_error(ERRS_PARSER_INVALID_PORT + port_str);
	}

	errno = 0;
	char*	end = NULL;
	long	port = std::strtol(port_str.c_str(), &end, 10);

	if (errno == ERANGE || *end != '\0' || port < 1 || port > 65535)
		throw std::runtime_error(ERRS_PARSER_INVALID_PORT + port_str);
	return (static_cast<int>(port));
}


void	Parser::setupListen(void) {
	expectSingleValue();
	const std::string&	value = _temp_vector[1]._value;

	// Two accepted formats: "X.X.X.X:PORT" or "PORT" (host defaults to 0.0.0.0)
	std::string	host = "0.0.0.0";
	std::string	port_str = value;
	size_t	colon = value.rfind(':');

	if (colon != std::string::npos) {
		host = value.substr(0, colon);
		port_str = value.substr(colon + 1);
		if (!isValidIP(host))
			throw std::runtime_error(ERRS_PARSER_INVALID_HOST + host);
	}

	int	port = parsePort(port_str);

	Socket	new_socket;
	new_socket.setHost(host);
	new_socket.setPort(port);
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


void				Parser::setupReturn(void) {
	if (_temp_vector.size() != 4)
		throw std::runtime_error (ERRS_PARSER_INVALID_SYNTAX + _temp_vector[0]._value);

	int error_code = std::atoi( _temp_vector[1]._value.c_str());
	getCurrentLocation().setReturnErrorCode(error_code);
	getCurrentLocation().setReturnPath( _temp_vector[2]._value );
}

static int	parseErrorCode(const std::string& code_str) {
	if (code_str.empty())
		throw std::runtime_error(ERRS_PARSER_INVALID_ERROR_CODE + code_str);
	for (size_t i = 0; i < code_str.size(); i++) {
		if (!std::isdigit(static_cast<unsigned char>(code_str[i])))
			throw std::runtime_error(ERRS_PARSER_INVALID_ERROR_CODE + code_str);
	}

	errno = 0;
	char*	end = NULL;
	long	code = std::strtol(code_str.c_str(), &end, 10);

	// nginx accepts response codes in the 300..599 range for error_page
	if (errno == ERANGE || *end != '\0' || code < 300 || code > 599)
		throw std::runtime_error(ERRS_PARSER_INVALID_ERROR_CODE + code_str);
	return (static_cast<int>(code));
}

// error_page <code> [<code>...] <path>;  -> at least directive + code + path + ';'
void				Parser::setupErrorPages(void) {
	if (_temp_vector.size() < 4)
		throw std::runtime_error(ERRS_PARSER_INVALID_SYNTAX + _temp_vector[0]._value);

	// Last token is the SEMICOLON, the one before it is the page path.
	const std::string&	path = _temp_vector[_temp_vector.size() - 2]._value;

	// Every token between the directive and the path is an error code.
	for (size_t i = 1; i + 2 < _temp_vector.size(); i++) {
		int	code = parseErrorCode(_temp_vector[i]._value);
		// std::cout << BOLD_GREEN << "ERROR_PAGE " << code << " = " << path << endofline;
		getCurrentServer().addErrorPage(code, path);
	}
}

// RETURNS THE VECTOR OF TOKENS FOR THE DIRECTIVE TO TREAT
void	Parser::findDirectiveTokenVector(size_t& index) {
	findNextSemicolon(index);

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
	else if (_temp_vector[0]._value == "root")
		setupRoot();
	else if (_temp_vector[0]._value == "index")
		setupIndex();
	else if (_temp_vector[0]._value == "return")
		setupReturn();
	else if (_temp_vector[0]._value == "error_page")
		setupErrorPages();
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