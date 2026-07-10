#include "Response.hpp"
#include "Location.hpp"
#include "utils.hpp"
#include <vector>
#include <ctime>
#include <fstream>
#include <sstream>

std::ostream &endofline(std::ostream &os) { return os << RESET << std::endl; }

size_t strToInt(std::string str) {
  size_t val;
  std::stringstream s(str);
  s >> val;
  return val;
}

void	printSetMethods(int flag) {
    std::cout << BOLD_CYAN << " === ALLOWED METHODS ===\n";
    if (flag & GET)
        std::cout << "[ GET ] ";
    if (flag & HEAD)
        std::cout << "[ HEAD ] ";
    if (flag & POST)
        std::cout << "[ POST ] ";
    if (flag & PUT)
        std::cout << "[ PUT ] ";
    if (flag & DELETE)
        std::cout << "[ DELETE ] ";
    if (flag & PATCH)
        std::cout << "[ PATCH ] ";
    if (flag & OPTIONS)
        std::cout << "[ OPTIONS ] ";
    if (flag & CONNECT)
        std::cout << "[ CONNECT ] ";
    if (flag & TRACE)
        std::cout << "[ TRACE ] ";
    std::cout << endofline;
}

// PRINTS STRING
void display(std::string print) { std::cout << print << endofline; }

// CHECKS IF CURRENT KEY BELONGS TO KEYS_LIST
bool    isValidKey(const std::string &key, const std::string keys_list[], const size_t size) {
    for (size_t i = 0; i < size; i++) {
        if (keys_list[i] == key)
            return (true);
    }
    return (false);
}

bool	endsWith(const std::string& path, const std::string& suffix) {
	return (path.size() >= suffix.size() && path.compare(path.size() - suffix.size(), suffix.size(), suffix) == 0);
}

void logError(std::string &msg) {
	std::ofstream log_file_path("logs/server_ws_errors.log", std::ios::app);
	if (log_file_path.is_open()){
		time_t now = time(0);
		log_file_path << "[" << now << "] -|" << msg << std::endl;
	}
}

const std::string buildHttpResponse(const std::string &cgi_output) {
  size_t sep_pos = cgi_output.find("\r\n\r\n");
  size_t sep_len = 4;

  if (sep_pos == std::string::npos) {
    // sep dif sometimes language change
    sep_pos = cgi_output.find("\n\n");
    sep_len = 2;
  }
  std::string header;
  std::string body;
  if (sep_pos != std::string::npos) {
    header = cgi_output.substr(0, sep_pos);
    body = cgi_output.substr(sep_pos + sep_len);
  } else {
    // no separator all is body
    body = cgi_output;
  }

  // default status
  std::string status_line = "200 OK";
  std::istringstream header_stream(header);
  std::string line;
  std::string saved_header;

  while (std::getline(header_stream, line)) {
    if (!line.empty() && line[line.size() - 1] == '\r') {
      line.erase(line.size() - 1);
    }
    if (line.empty())
      continue;
    if (line.compare(0, 8, "Status: ") == 0)
      status_line = line.substr(8);
    else
      saved_header += line + "\r\n";
  }
  saved_header += "Connection: close\r\n";

  // build resp
  std::ostringstream resp;
  resp << "HTTP/1.1 " << status_line << "\r\n";
  resp << saved_header;
  if (saved_header.find("Content-Length:") == std::string::npos) {
    resp << "Content-Length: " << body.size() << "\r\n";
  }
  resp << "\r\n" << body;
  return resp.str();
}

//root vient des error_pages directement
Response    buildErrorResponse(int code, const MapIntStr& error_pages)
{
    if (error_pages.count(code)) {
        std::ifstream f(error_pages.at(code).c_str());
        if (f.is_open()) {
            std::ostringstream ss;
            ss << f.rdbuf();
            return Response(code, ss.str(), "text/html");
        }
    }
    //normalement on arrive jamais la, mais build une vrai reponse au cas ou
    std::string reason = Response::reasonPhrase(code);
    std::ostringstream body;
    body << "<html><head><title>" << code << " " << reason << "</title></head>"
         << "<body><h1>" << code << " " << reason << "</h1>"
         << "<hr><p>webserv/1.0</p></body></html>";
    return Response(code, body.str(), "text/html");}


std::string extractBoundary(std::string header_content_type) {
  std::string word_start = "boundary=";
  std::string boundary;
  size_t pos = header_content_type.find(word_start);
  if (pos != std::string::npos) {
    pos += word_start.length();
    boundary = header_content_type.substr(pos);
  }
  return boundary;
}