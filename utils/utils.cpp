#include "utils.hpp"
#include <sstream>

std::ostream &endofline(std::ostream &os) { return os << RESET << std::endl; }

size_t strToInt(std::string str) {
  size_t val;
  std::stringstream s(str);
  s >> val;
  return val;
}
