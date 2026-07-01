#include "Response.hpp"
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <sstream>
#include <string>
#include <map>

Response:: Response(int code, const std::string& cgi_output) : _status_code(code), _content_type("text/html")
{
    _headers["Server"]     = "webserv/1.0";
    char buf[128];
    time_t now = time(NULL);
    strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S GMT", gmtime(&now));
    _headers["Date"] = buf;

    if (!cgi_output.empty())
        parseCgi_output(cgi_output);
}

void Response::parseCgi_output(const std::string& raw)
{
    size_t sep = raw.find("\r\n\r\n");
    if (sep == std::string::npos)
        sep = raw.find("\n\n");
    if (sep == std::string::npos) //no headers, ducoup que du body
    {
        _body = raw;
        return;
    }
    std::string cgi_headers = raw.substr(0, sep);
    _body = raw.substr(sep + (raw[sep + 1] == '\n' ? 2 : 4));

    //parse chaque headers
    std::istringstream iss(cgi_headers);
    std::string line;
    while (std::getline(iss, line)) {
        if (!line.empty() && line.size() - 1 == '\r')
            line.erase(line.size() - 1);
        size_t colon = line.find(':');
        if (colon == std::string::npos)
            continue;
        std::string key = line.substr(0, colon);
        std::string val = line.substr(colon + 2);

        if (key == "Status")
            _status_code = std::atoi(val.c_str());
        else if (key == "Content-Type")
            _content_type = val;
        else
            _headers[key] = val;
    }
}

Response::Response(int code) : _status_code(code), _content_type("text/html")
{
    _headers["Server"] = "webserv/1.0";
    char buf[128];
    time_t now = time(NULL);
    strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S GMT", gmtime(&now));
    _headers["Date"] = buf;

    std::map<int, std::string> reasons;
    reasons[400] = "Bad Request";
    reasons[403] = "Forbidden";
    reasons[404] = "Not Found";
    reasons[405] = "Method Not Allowed";
    reasons[411] = "Length Required";
    reasons[500] = "Internal Server Error";
    reasons[505] = "HTTP Version Not Supported";

    std::string reason = reasons.count(code) ? reasons.at(code) : "Unknown";

    //body minimal par default (page derreur customapres avec config)
    std::ostringstream body;
    body << "<html><head><title>" << code << " " << reason << "</title></head>"
         << "<body><h1>" << code << " " << reason << "</h1>"
         << "<hr><p>webserv/1.0</p></body></html>";
    _body = body.str();
}

//FIX: take request, handle if file... 
std::string Response::build()
{
    std::map<int, std::string> reasons;
    reasons[200] = "OK";
    reasons[201] = "Created";
    reasons[204] = "No Content";
    reasons[301] = "Moved Permanently";
    reasons[400] = "Bad Request";
    reasons[403] = "Forbidden";
    reasons[404] = "Not Found";
    reasons[405] = "Method Not Allowed";
    reasons[500] = "Internal Server Error";

    std::string reason = reasons.count(_status_code) ? reasons[_status_code] : "Unknown";
    
    std::ostringstream oss;
    oss << "HTTP/1.1 " << _status_code << " " << reason << "\r\n";
    oss << "Content-Type: "   << _content_type << "\r\n";
    oss << "Content-Length: " << _body.size() << "\r\n";

		std::map< std::string , std::string >::const_iterator it = _headers.begin();
		std::map< std::string , std::string >::const_iterator ite = _headers.end();
    
    for (; it != ite; ++it)
        oss << it->first << ": " << it->second << "\r\n";
    
    oss << "\r\n";
    oss << _body;

		std::cerr << "[DEBUG] body: " << _body << std::endl;

    return oss.str();
}
