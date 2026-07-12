#ifndef PARSEREXCEPTION_HPP
# define PARSEREXCEPTION_HPP

#include <stdexcept>
#include <string>

#include "Token.hpp"

// Exception carrying a config-file token value + line number.
// Derives from std::runtime_error so existing catch (runtime_error&) blocks
// still catch it. Produces messages like:
//   "Invalid syntax for token 'foo' line 12"
class ParserException : public std::runtime_error {
public:
    ParserException(const std::string& message, const Token& token);
    ParserException(const std::string& message, const std::string& value, int line);

private:
    static std::string buildMessage(const std::string& message, const std::string& value, int line);
};

#endif
