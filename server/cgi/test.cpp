#include "Cgi.hpp"
#include <vector>
#include <iostream>

int main(void) {
  std::vector<std::string> ls;
  ls.push_back("python");
  Request rq;

  std::string request = "POST /serve.py HTTP/1.1\r\nHost: example.com\r\nContent-Type: application/json\r\nContent-Length: 36\r\n\r\n{\"user\":\"alice\",\"password\":\"hunter\"}";

  std::string body = "{\"user\":\"alice\",\"password\":\"hunter\"}";
  std::cout << body.length() << std::endl;
  rq.parseRequest(request); 
  Cgi cgi(ls, rq);

  cgi.run();
  return 0;
}
