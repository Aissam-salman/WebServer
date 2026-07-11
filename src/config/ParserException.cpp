#include <sstream>

#include "ParserException.hpp"

// Builds "<message> for token '<value>' line <line>" once, at construction,
// because std::runtime_error needs the final string up front.
std::string ParserException::buildMessage(const std::string& message,
                                          const std::string& value, int line) {
    std::ostringstream oss;
    oss << message << " for token '" << value << "' line " << line;
    return oss.str();
}

ParserException::ParserException(const std::string& message, const Token& token)
    : std::runtime_error(buildMessage(message, token._value, token._line)) {}

ParserException::ParserException(const std::string& message,
                                 const std::string& value, int line)
    : std::runtime_error(buildMessage(message, value, line)) {}
