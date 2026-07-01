#include "Request.hpp"

class Response {

private:
    
    int                                 _status_code;
    std::string                         _body;
    std::string                         _content_type;
    std::map<std::string, std::string>  _headers;

    void    parseCgi_output(const std::string& raw_output);
public:

    Response(int code);
    Response(int code, const std::string& cgi_output);
    std::string& build() const; // construit la reponse final HTTP
};