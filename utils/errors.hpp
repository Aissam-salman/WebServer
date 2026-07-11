#ifndef ERRORS_HPP
# define ERRORS_HPP


#define ERRS_ARGS_MAIN "Wrong number of argument ! Try with : ./webserv [config file path]"

// ===== LEXER (server/config/Lexer.cpp) =====
# define ERRS_LEXER_BAD_EXTENSION   "Invalid file name : must end with .conf"
# define ERRS_LEXER_FILE_OPEN       "Error opening the file : " // + file path

// ===== PARSER (server/config/Parser.cpp) =====
# define ERRS_PARSER_UNTERMINATED_DIRECTIVE \
    "Invalid conf file, reaching the end of file without finding end of directive"

# define ERRS_PARSER_SCOPE_UNDERFLOW   "Conf file went out of scope" // lowerScope
# define ERRS_PARSER_SCOPE_OVERFLOW    "Conf file went out of scope" // upperScope

# define ERRS_PARSER_DIRECTIVE_PREFIX        "Directive '"
# define ERRS_PARSER_DIRECTIVE_IN_LOCATION   "' is not valid in location scope"
# define ERRS_PARSER_DIRECTIVE_IN_SERVER     "' is not valid in server scope"
# define ERRS_PARSER_DIRECTIVE_OUTSIDE_BLOCK "' found outside any block"

# define ERRS_PARSER_INVALID_SYNTAX   "Invalid syntax for key " // + key
# define ERRS_PARSER_INVALID_SYNTAX_MSG   "Invalid syntax" // used with ParserException (token + line)
# define ERRS_PARSER_EXISTING_LOCATION   "A location already exists with name "  // + key

# define ERRS_PARSER_INVALID_METHOD_PREFIX   "Unknown HTTP method '" // + value
# define ERRS_PARSER_INVALID_METHOD_SUFFIX   "' in methods directive"

# define ERRS_PARSER_INVALID_AUTOINDEX   "autoindex only accepts 'on' or 'off'"

# define ERRS_PARSER_INVALID_ERROR_CODE   "Invalid error code in error_page directive: " // + code

# define ERRS_PARSER_INVALID_HOST   "Invalid host in listen directive: " // + host
# define ERRS_PARSER_INVALID_PORT   "Invalid port in listen directive: " // + port

# define ERRS_PARSER_DUPLICATE_SERVER_NAME_PREFIX   "A server already exists with name '" // + name
# define ERRS_PARSER_SERVER_NAME_ALREADY_SET_PREFIX "Server name is already set to '"     // + name
# define ERRS_PARSER_SERVER_NAME_SUFFIX             "'"

# define ERRS_PARSER_INVALID_DIRECTIVE_PREFIX   "Invalid directive " // + value
# define ERRS_PARSER_INVALID_DIRECTIVE_SUFFIX   " found in file"

# define ERRS_PARSER_INCOMPLETE_FILE   "Incomplete conf file"
# define ERRS_PARSER_NO_SERVER         "No server has been configured"

#endif
