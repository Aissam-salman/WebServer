#ifndef STATIC_HANDLER_HPP
#define STATIC_HANDLER_HPP

#include "Location.hpp"
#include "Socket.hpp"
#include "client/Client.hpp"
#include "utils.hpp"
#include "Response.hpp"
#include "Request.hpp"
#include <map>
#include <string>
#include <vector>

class StaticHandler {
private:
    const Request&   _request;
    Location&  _location;

    std::string buildPath() const;
    bool        isSafePath(const std::string& path) const;
    std::string getMimeType(const std::string& path) const;
    std::string generateAutoindex(const std::string& path) const;
    std::string readFile(const std::string& path) const;

public:
    StaticHandler(const Request& req, std::vector<Location>& locs);
    Response handle() const;
};


#endif
