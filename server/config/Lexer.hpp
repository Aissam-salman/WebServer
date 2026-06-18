#ifndef LEXER_HPP
# define LEXER_HPP

#include <string>

#include <string>
#include <vector>
#include <fstream>
// #include <sstream>

static const std::string GLOBAL_KEY[] =  {
    "server"
};

static const std::string SERVER_KEY[] = {
    "listen", "server_name", "client_max_body_size",
    "error_page", "location"
};

static const std::string LOCATION_KEY[] = {
    "root", "index", "autoindex", "methods", "upload_dir",
    "cgi", "return", "client_max_body_size"
};

// bool    isValidKey(const std::string &key, const std::string keys_list[], const size_t size) {
//     for (size_t i = 0; i < size; i++) {
//         if (keys_list[i] == key)
//             return (true);
//     }
//     return (false);
// }

class Lexer {
    private:
        // int                         _file_fd;
        std::string                 _raw_file_path;
        std::ifstream               _raw_conf_file;
        std::vector<std::string>    _raw_tokens_vector;

    public:
        Lexer(void);
        Lexer(std::string conf_file_path);
        Lexer(const Lexer &src);
        Lexer& operator= (const Lexer &other);
        ~Lexer();

        // OUTPUTS / DEMOS
        void    printTokens(void) const ; 
        void    printRawConfFile(void) ; 
        void    initTokensVector(void);


};

#endif
