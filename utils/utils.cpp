#include "utils.hpp"
#include <string>


// RESETS + ENDL THE OSTREAM
std::ostream& endofline(std::ostream& os) {
    return os << RESET << std::endl;
}

void	printSetMethods(int flag) {
    std::cout << BOLD_CYAN << " === ALLOWED METHODS ===\n";
    if (flag & GET)
        std::cout << "[ GET ] ";
    if (flag & HEAD)
        std::cout << "[ HEAD ] ";
    if (flag & POST)
        std::cout << "[ POST ] ";
    if (flag & PUT)
        std::cout << "[ PUT ] ";
    if (flag & DELETE)
        std::cout << "[ DELETE ] ";
    if (flag & PATCH)
        std::cout << "[ PATCH ] ";
    if (flag & OPTIONS)
        std::cout << "[ OPTIONS ] ";
    if (flag & CONNECT)
        std::cout << "[ CONNECT ] ";
    if (flag & TRACE)
        std::cout << "[ TRACE ] ";
    std::cout << endofline;
}

// PRINTS STRING
void display(std::string print) { std::cout << print << endofline; }

// CHECKS IF CURRENT KEY BELONGS TO KEYS_LIST
bool    isValidKey(const std::string &key, const std::string keys_list[], const size_t size) {
    for (size_t i = 0; i < size; i++) {
        if (keys_list[i] == key)
            return (true);
    }
    return (false);
}