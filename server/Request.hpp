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

public:

    void    parseRequest(const std::string& raw_request);
    void    parseRequestLine(const std::string& first_line);

    const std::string& getMethod() const { return method; }
    const std::string& getResource() const { return resource; }
    const std::string& getHttpVersion() const { return http_version; }
    const std::map<std::string, std::string>& getHeaders() const { return headers; }
    const std::string& getBody() const { return body; }

    Request();
    ~Request();

};

#endif