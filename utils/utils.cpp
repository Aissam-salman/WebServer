#include "utils.hpp"
#include <string>

std::ostream& endofline(std::ostream& os) {
    return os << RESET << std::endl;
}

void display(std::string print) { std::cout << print << endofline; }

bool    isValidKey(const std::string &key, const std::string keys_list[], const size_t size) {
    for (size_t i = 0; i < size; i++) {
        if (keys_list[i] == key)
            return (true);
    }
    return (false);
}