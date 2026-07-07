#include "Request.hpp"

class Response {

private:
    
    int                                 _status_code;
    std::string                         _body;
    std::string                         _content_type;
    std::map<std::string, std::string>  _headers;

    
public:
    
    Response(int code, const std::string& content, const std::string& mimetype);
    
    std::string         build(); // construit la reponse final HTTP
    void                setHeader(const std::string& key, const std::string& value);
    static std::string  reasonPhrase(int code);
    
};
