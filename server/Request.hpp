#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <map>
#include <string>

class Request {
private:
  std::string method;
  std::string http_version;
  std::string resource;
  std::map<std::string, std::string> headers;
  std::string body;
  std::map<std::string, std::string> cgi_env;

  std::string _server_port;
  std::string _client_ip;
  std::string _client_port;
  std::string _document_root;

  void parseRequestLine(const std::string &first_line);
  void parseCgi_env();

public:
  bool isCGI() const { return (resource.find("/cgi-bin/") == 0); }
  void parseRequest(const std::string &raw_request);

  std::string decodeChunk(std::string &body_raw);
  const std::string &getMethod() const { return method; }
  const std::string &getResource() const { return resource; }
  const std::string &getHttpVersion() const { return http_version; }
  const std::map<std::string, std::string> &getHeaders() const { return headers; }
  const std::string &getBody() const { return body; }
  const std::map<std::string, std::string> &getCgi_env() const { return cgi_env; }
  std::string &getDocumentRoot(void) { return _document_root; }
  void addToCgiEnv(std::string key, std::string val);

  void clear();
  Request(const std::string &server_port, const std::string &client_ip,
          const std::string &client_port, const std::string &document_root);
  Request(void);
  ~Request();
};

#endif
