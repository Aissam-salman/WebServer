#include "Request.hpp"

Request::Request() {}

Request::~Request() {}

std::string trim(const std::string &s) {
  size_t start = s.find_first_not_of(" \t");
  size_t end = s.find_last_not_of(" \t");

  if (start == std::string::npos) {
    return "";
  }
  return s.substr(start, end - start + 1);
}

// WARN: Host need handle error
void Request::parseRequest(const std::string &raw_request) {
  size_t separator = raw_request.find("\r\n\r\n");
  if (separator == std::string::npos)
    throw std::runtime_error("missing separator CRLF");

  std::string before_body = raw_request.substr(0, separator);
  body = raw_request.substr(separator + 4);

  size_t first_line_pos = before_body.find("\r\n");
  std::string first_line = before_body.substr(0, first_line_pos);

  parseRequestLine(first_line);

  std::string headers_str =
      before_body.substr(first_line_pos + 2, separator - (first_line_pos + 2));
  size_t pos = 0;
  while ((pos = headers_str.find("\r\n")) != std::string::npos)
    headers_str.erase(pos, 1); // Enlève le \r

  std::istringstream issh(headers_str);
  std::string line;
  while (getline(issh, line)) {
    if (line.empty())
      continue;
    size_t colon = line.find(":");
    if (colon == std::string::npos)
      continue;

    std::string key = trim(line.substr(0, colon));
    std::string value = trim(line.substr(colon + 1));

    if (!key.empty())
      this->headers[key] = value;
  }
  if (headers.count("Content-Length")) {
    size_t len = std::atoi(headers["Content-Length"].c_str());
    if (len != body.size())
      throw std::runtime_error("400");
  }
  if (body.length() > 1 && !headers.count("Content-Length"))
    throw std::runtime_error("411"); // Length Required
}

void Request::parseRequestLine(const std::string &first_line) {
  std::istringstream iss(first_line);
  iss >> method >> resource >> http_version;

  if (method.empty() || resource.empty() || http_version.empty())
    throw std::invalid_argument("400");
  if (method != "GET" && method != "POST" && method != "DELETE")
    throw std::runtime_error("405");
  if (http_version != "HTTP/1.1" && http_version != "HTTP/1.0")
    throw std::runtime_error("505");
}
