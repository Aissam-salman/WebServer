#include <sstream>

#include "ParserException.hpp"

// BUILD "<MESSAGE> FOR TOKEN '<VALUE>' LINE <LINE>" ONCE, AT CONSTRUCTION
std::string ParserException::buildMessage(const std::string& message,
                                          const std::string& value, int line) {
    std::ostringstream oss;
    oss << message << " for token '" << value << "' line " << line;
    return oss.str();
}

// BUILD THE EXCEPTION FROM A TOKEN (USES ITS VALUE AND LINE)
ParserException::ParserException(const std::string& message, const Token& token)
    : std::runtime_error(buildMessage(message, token._value, token._line)) {}

// BUILD THE EXCEPTION FROM AN EXPLICIT VALUE AND LINE
ParserException::ParserException(const std::string& message,
                                 const std::string& value, int line)
    : std::runtime_error(buildMessage(message, value, line)) {}
