#include "Response.hpp"
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <sstream>
#include <string>
#include <map>

// MAP AN HTTP STATUS CODE TO ITS REASON PHRASE
std::string Response::reasonPhrase(int code)
{
    std::map<int, std::string> r;
    r[200] = "OK";
    r[201] = "Created";
    r[204] = "No Content";
    r[301] = "Moved Permanently";
    r[302] = "Found";
    r[400] = "Bad Request";
    r[403] = "Forbidden";
    r[413] = "Payload Too Large";
    r[404] = "Not Found";
    r[405] = "Method Not Allowed";
    r[411] = "Length Required";
    r[500] = "Internal Server Error";
    r[505] = "HTTP Version Not Supported";
    return r.count(code) ? r.at(code) : "Unknown";
}

// SET (OR OVERWRITE) ONE RESPONSE HEADER
void    Response::setHeader(const std::string& key, const std::string& value)
{
    _headers[key] = value;
}

// RESPONSE WITH AN EXPLICIT CONTENT-TYPE (SEEDS SERVER + DATE HEADERS)
Response::Response(int code, const std::string& content, const std::string& mimetype)
    : _status_code(code), _body(content), _content_type(mimetype)
{
    _headers["Server"] = "webserv/1.0";
    char buf[128];
    time_t now = time(NULL);
    strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S GMT", gmtime(&now));
    _headers["Date"] = buf;
}

// RESPONSE WITHOUT A CONTENT-TYPE (SEEDS SERVER + DATE HEADERS)
Response::Response(int code, const std::string& content)
    : _status_code(code), _body(content)
{
    _headers["Server"] = "webserv/1.0";
    char buf[128];
    time_t now = time(NULL);
    strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S GMT", gmtime(&now));
    _headers["Date"] = buf;
}


// SERIALIZE THE STATUS LINE, HEADERS AND BODY INTO ONE HTTP RESPONSE STRING
std::string Response::build() //build la reponse finale
{
    std::ostringstream oss;
    oss << "HTTP/1.1 " << _status_code << " " << reasonPhrase(_status_code) << "\r\n";
    // Content-Type seulement si non vide
    if (!_content_type.empty())
        oss << "Content-Type: " << _content_type << "\r\n";
    oss << "Content-Length: " << _body.size() << "\r\n";

    std::map< std::string , std::string >::const_iterator ite = _headers.end();
    std::map< std::string , std::string >::const_iterator it = _headers.begin();
    for (; it != ite; ++it)
        oss << it->first << ": " << it->second << "\r\n";
    oss << "\r\n";
    oss << _body;
    return oss.str();
}
