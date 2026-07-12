#ifndef ERRORS_HPP
# define ERRORS_HPP


#define ERRS_ARGS_MAIN "Wrong number of argument ! Try with : ./webserv [config file path]"

// ===== LEXER (src/config/Lexer.cpp) =====
# define ERRS_LEXER_BAD_EXTENSION   "Invalid file name : must end with .conf"
# define ERRS_LEXER_FILE_OPEN       "Error opening the file : " // + file path

// ===== PARSER (src/config/Parser.cpp) =====
# define ERRS_PARSER_UNTERMINATED_DIRECTIVE \
    "Invalid conf file, reaching the end of file without finding end of directive"

# define ERRS_PARSER_OUT_OF_SCOPE   "Conf file went out of scope" // lowerScope / upperScope

# define ERRS_PARSER_INVALID_SYNTAX       "Invalid syntax for key " // + key
# define ERRS_PARSER_INVALID_SYNTAX_MSG   "Invalid syntax" // used with ParserException (token + line)

// ParserException messages : the offending token + line are appended by ParserException
# define ERRS_PARSER_DIRECTIVE_SCOPE_PREFIX   "Directive is not valid in " // + scopeName + suffix
# define ERRS_PARSER_DIRECTIVE_SCOPE_SUFFIX   " scope"
# define ERRS_PARSER_DIRECTIVE_OUTSIDE_BLOCK  "Directive found outside any block"
# define ERRS_PARSER_DIRECTIVE_IN_SERVER      "Directive is not valid in server scope"
# define ERRS_PARSER_DIRECTIVE_IN_LOCATION    "Directive is not valid in location scope"

# define ERRS_PARSER_EXISTING_LOCATION   "A location already exists with name"

# define ERRS_PARSER_INVALID_METHOD   "Unknown HTTP method in methods directive"

# define ERRS_PARSER_INVALID_AUTOINDEX   "autoindex only accepts 'on' or 'off'"

# define ERRS_PARSER_INVALID_RETURN_CODE   "Invalid error code in return directive"
# define ERRS_PARSER_INVALID_ERROR_CODE    "Invalid error code in error_page directive"

# define ERRS_PARSER_INVALID_HOST   "Invalid host in listen directive"
# define ERRS_PARSER_INVALID_PORT   "Invalid port in listen directive"

# define ERRS_PARSER_SERVER_NAME_ALREADY_SET   "Server name is already set"
# define ERRS_PARSER_DUPLICATE_SERVER_NAME     "A server already exists with name"

# define ERRS_PARSER_INVALID_DIRECTIVE   "Invalid directive found in file"

# define ERRS_PARSER_INCOMPLETE_FILE   "Incomplete conf file"
# define ERRS_PARSER_NO_SERVER         "No server has been configured"

// ===== SOCKET (src/core/Socket.cpp) =====
# define ERRS_SOCKET_GETADDRINFO   "getaddrinfo: " // + gai_strerror
# define ERRS_SOCKET_INIT          "Listening socket didn't initialize properly"
# define ERRS_SOCKET_SETOPT        "Listening socket didn't set properly"
# define ERRS_SOCKET_BIND          "Binding failed." // + strerror(errno)
# define ERRS_SOCKET_LISTEN        "Listening on socket failed"

// ===== WEBSERV (src/core/WebServ.cpp) =====
# define ERRS_WEBSERV_SIGNAL   "SIGNAL RECEIVED"
# define ERRS_WEBSERV_POLL     "Poll failed miserably"

// ===== REQUEST (src/http/Request.cpp) =====
# define ERRS_REQUEST_MISSING_CRLF   "missing separator CRLF"

#endif
