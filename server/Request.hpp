#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <iostream>
#include <stdio.h>
#include <string>
#include <stdlib.h>
#include <vector>
#include <errno.h> 
#include <netdb.h>
#include <unistd.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h> 
#include <string>
#include <map>
#include <algorithm>
#include <istream>
#include <sstream>


class Request
{
private:

    std::string method;
    std::string http_version;
    std::string resource;
    std::map<std::string, std::string> headers;
    std::string body;
    std::map<std::string, std::string> cgi_env;
    
    const std::string& _server_port;
    const std::string& _client_ip;
    const std::string& _client_port;
    const std::string& _document_root;
    
    void    parseRequestLine(const std::string& first_line);
    void    parseCgi_env();

public:

    bool isCGI() const { return (resource.find("/cgi-bin/") == 0); };
    void    parseRequest(const std::string& raw_request);

    const std::string& getMethod() const { return method; }
    const std::string& getResource() const { return resource; }
    const std::string& getHttpVersion() const { return http_version; }
    const std::map<std::string, std::string>& getHeaders() const { return headers; }
    const std::string& getBody() const { return body; }
    const std::map<std::string, std::string>& getCgi_env() const { return cgi_env; }

    void    clear();
    Request(const std::string& server_port,
    const std::string& client_ip,
    const std::string& client_port,
    const std::string& document_root);
    ~Request();

};

#endif
