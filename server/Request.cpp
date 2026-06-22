#include "Request.hpp"

Request::Request(const std::string& server_port,
    const std::string& client_ip,
    const std::string& client_port,
    const std::string& document_root) : _server_port(server_port),
                                        _client_ip(client_ip),
                                        _client_port(client_port),
                                        _document_root(document_root) {}

Request::~Request() {}

static std::string trim(const std::string& s)
{
    size_t start = s.find_first_not_of(" \t"); //\r
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

    std::string headers_str = before_body.substr(first_line_pos + 2);
    size_t pos = 0;
    while ((pos = headers_str.find("\r\n")) != std::string::npos)
        headers_str.erase(pos, 1); // Enlève le \r
    
    std::istringstream issh(headers_str);
    std::string line;
    while (getline(issh, line))
    {
        if (line.empty())
            continue;    
        size_t pos = line.find(":");
        if (pos == std::string::npos)
            continue;  // Skip si pas de colon
        
        std::string key = trim(line.substr(0, pos));
        std::string value = trim(line.substr(pos + 1));

        if (!key.empty())
            headers[key] = value;
    }
    if (headers.count("Content-Length"))
    {
        size_t len = std::atoi(headers["Content-Length"].c_str());
        if (len != body.size())
            throw std::runtime_error("400");
    }
    if (!body.empty() && !headers.count("Content-Length"))
        throw std::runtime_error("411"); // Length Required
    if (isCGI())
        parseCgi_env();
}

void Request::parseRequestLine(const std::string &first_line) {
  std::istringstream iss(first_line);
  iss >> method >> resource >> http_version;

    if (method.empty() || resource.empty() || http_version.empty())
        throw std::invalid_argument("400");
    
    std::string extra;
    if (iss >> extra)
        throw std::invalid_argument("400");
	if (method != "GET" && method != "POST" && method != "DELETE")
		throw std::runtime_error("405");
	if (http_version != "HTTP/1.1" && http_version != "HTTP/1.0")
		throw std::runtime_error("505");
}

void    Request::parseCgi_env()
{
    cgi_env["REQUEST_METHOD"] = method;
    cgi_env["SERVER_PROTOCOL"] = http_version;
    cgi_env["GATEWAY_INTERFACE"] = "CGI/1.1";
    cgi_env["SERVER_SOFTWARE"] = "webserv/1.0";
    cgi_env["SERVER_PORT"] = _server_port;
    cgi_env["REMOTE_ADDR"] = _client_ip;
    cgi_env["REMOTE_PORT"] = _client_port;
    
    if (headers.count("Host"))
        cgi_env["SERVER_NAME"] = headers.at("Host");
    if (headers.count("Content-Length"))
        cgi_env["CONTENT_LENGTH"] = headers.at("Content-Length");
    if (headers.count("Content-Type"))
        cgi_env["CONTENT_TYPE"] = headers.at("Content-Type");
    
    size_t query_pos = resource.find("?");
    std::string path;
    if (query_pos != std::string::npos) 
        path = resource.substr(0, query_pos);
    else
        path = resource;
    if (query_pos != std::string::npos)
    {
        size_t space_pos = resource.find_first_of(" \r\t\n", query_pos);
        if (space_pos != std::string::npos)
            cgi_env["QUERY_STRING"] = resource.substr(query_pos +1, space_pos - query_pos - 1);
        else
            cgi_env["QUERY_STRING"] = resource.substr(query_pos + 1);
    }
    else
        cgi_env["QUERY_STRING"] = "";

    cgi_env["SCRIPT_NAME"] = path;
    cgi_env["PATH_INFO"] = "";
    size_t pos = 7;
    while ((pos = path.find("/", pos + 1)) != std::string::npos)
    {
        std::string s = path.substr(pos + 1, path.find("/", pos +1) - (pos + 1));
        if (s.find(".") != std::string::npos)
        {
            cgi_env["SCRIPT_NAME"] = path.substr(0, pos + 1 + s.length());
            cgi_env["PATH_INFO"] = path.substr(pos + 1 + s.length());
            break;
        }
    }

    cgi_env["SCRIPT_FILENAME"] = _document_root + cgi_env["SCRIPT_NAME"];
    if (!cgi_env["PATH_INFO"].empty())
        cgi_env["PATH_TRANSLATED"] = _document_root + cgi_env["PATH_INFO"];
    //"The server SHOULD set PATH_TRANSLATED only if PATH_INFO is not empty"
    for (std::map<std::string, std::string>::const_iterator it = headers.begin(); it != headers.end(); ++it)
    {
        const std::string& key = it->first;
        const std::string& value = it->second;
        
        std::string env_key = "HTTP_" + key;
        for (size_t i = 0; i < env_key.length(); ++i)
            env_key[i] = std::toupper(env_key[i]);
        std::replace(env_key.begin(), env_key.end(), '-', '_');
        cgi_env[env_key] = value;
    }
}

void Request::clear()
{
    method.clear();
    resource.clear();
    http_version.clear();
    headers.clear();
    body.clear();
    cgi_env.clear();
}