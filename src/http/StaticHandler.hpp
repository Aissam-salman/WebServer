#ifndef STATIC_HANDLER_HPP
#define STATIC_HANDLER_HPP

#include "Location.hpp"
#include "Request.hpp"
#include "Response.hpp"
#include <string>
#include <vector>

class StaticHandler {
  private:
    const Request &_request;
    Location &_location;

    std::string buildPath() const;
    bool isSafePath(const std::string &path) const;
    std::string getMimeType(const std::string &path) const;
    std::string getMimeTypeAllowForPost(const std::string &path) const;
    std::string generateAutoindex(const std::string &path) const;
    std::string readFile(const std::string &path) const;
    bool isSafeFile(std::string &file_path, std::string &file_type) const;
    bool isFileAlreadyExist(std::string &file_path) const;

  public:
    StaticHandler(const Request &req, std::vector<Location> &locs);
    Response handle() const;
    static Location &findLocation(std::vector<Location> &locs, const std::string &resource);
};

#endif
