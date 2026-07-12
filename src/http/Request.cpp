#include "Request.hpp"
#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdlib>
#include <sstream>
#include <stdexcept>
#include <string>
#include <cstdlib>

#include "utils.hpp"
#include "errors.hpp"

// REQUEST BOUND TO CONNECTION INFO (PORTS, CLIENT IP, DOCUMENT ROOT)
Request::Request(const std::string &server_port, const std::string &client_ip,
                 const std::string &client_port,
                 const std::string &document_root)
    : _server_port(server_port), _client_ip(client_ip),
      _client_port(client_port), _document_root(document_root) {}

// EMPTY REQUEST
Request::Request(void) {}

// DESTRUCTOR
Request::~Request() {}

// STRIP LEADING/TRAILING SPACES AND TABS
static std::string trim(const std::string &s) {
    size_t start = s.find_first_not_of(" \t");
    size_t end = s.find_last_not_of(" \t");
    if (start == std::string::npos)
        return "";
    return s.substr(start, end - start + 1);
}

// TRUE IF THE RAW REQUEST DECLARES TRANSFER-ENCODING: CHUNKED
bool isChunked(const std::string &raw) {
    size_t chunk_header = raw.find("Transfer-Encoding: chunked");
    if (chunk_header != std::string::npos) {
        return true;
    }
    return false;
}

// DECODE A CHUNKED BODY INTO THE FULL BODY STRING
std::string Request::decodeChunk(std::string &body_raw) {

    // on a la taille finale
    std::string full_body;
    size_t start = 0;
    size_t end = body_raw.find("0\r\n\r\n");

    // boucle tant que start est inferieur a la taille du body
    while (start < end) {

        // on trouve le prochain bloc d'info size du chunk
        size_t end_hex = body_raw.find("\r\n", start);
        if (end_hex == std::string::npos)
            throw std::runtime_error("400");

        // on recupere la valeur en hexa et on la convertit en decimal
        std::string hex_val = body_raw.substr(start, end_hex - start);
        size_t chunk_size = std::strtol(hex_val.c_str(), NULL, 16);
        start = end_hex + 2;

        // decoupe du body
        std::string chunk_body = body_raw.substr(start, chunk_size);

        // ajoute au body complet
        full_body += chunk_body;
        start += chunk_size + 2;
    }

    return full_body;
}

// PARSE A RAW REQUEST INTO REQUEST LINE, HEADERS AND BODY (CHUNKED OR NOT)
void Request::parseRequest(const std::string &raw_request) {
    // the blank line splits the header block from the body
    size_t separator = raw_request.find("\r\n\r\n");
    if (separator == std::string::npos)
        throw std::runtime_error(ERRS_REQUEST_MISSING_CRLF);

    // first line -> request line
    std::string before_body = raw_request.substr(0, separator);
    size_t first_line_pos = before_body.find("\r\n");
    try {
        parseRequestLine(before_body.substr(0, first_line_pos));
    } catch (std::runtime_error &e) {
        throw std::runtime_error(e);
    }

    // strip the \r from every header line
    std::string headers_str = before_body.substr(first_line_pos + 2);
    size_t pos = 0;
    while ((pos = headers_str.find("\r\n")) != std::string::npos)
        headers_str.erase(pos, 1); // Enlève le \r


    // collect "key: value" pairs
    std::istringstream issh(headers_str);
    std::string line;
    while (getline(issh, line)) {
        if (line.empty())
            continue;
        size_t pose = line.find(":");
        if (pose == std::string::npos)
            continue; // skip si pas de colon

        std::string key = trim(line.substr(0, pose));
        std::string value = trim(line.substr(pose + 1));

        if (!key.empty())
            headers[key] = value;
    }

    if (!isChunked(raw_request)) {
        // non-chunked: take the body and validate against Content-Length
        //TODO: check body size ?? with max_size
        body = raw_request.substr(separator + 4);
        if (headers.count("Content-Length")) {
            size_t len = std::atoi(headers["Content-Length"].c_str());
            if (len != body.size())
                throw std::runtime_error("400");
        }
        if (body.length() > 1 && !headers.count("Content-Length"))
            throw std::runtime_error("411"); // Length Required
    } else {
        // chunked: decode into the final body
        std::string tmp = raw_request.substr(separator + 4);
        body = decodeChunk(tmp);

        std::ostringstream oss;
        oss << body.size();
        headers["Content-Length"] = oss.str();
    }
    // cgi requests also need their environment built
    if (isCGI())
        parseCgi_env();
}

// CHECKS THE REQUEST FIRST LINE SENT BY THE CLIENT
void Request::parseRequestLine(const std::string &first_line) {

#if DEBUG_REQUEST == 1
    std::cout << "Request Line = " << first_line << endofline;
#endif

    // split "METHOD RESOURCE VERSION"
    std::istringstream iss(first_line);
    iss >> method >> resource >> http_version;

    if (method.empty() || resource.empty() || http_version.empty())
        throw std::invalid_argument("400");

    // nothing may follow the version token
    std::string extra;
    if (iss >> extra)
        throw std::invalid_argument("400");

    // only known methods and HTTP versions are accepted
    if (method != "GET" && method != "POST" && method != "DELETE" &&
        method != "PUT")
        throw std::runtime_error("405");
    if (http_version != "HTTP/1.1" && http_version != "HTTP/1.0")
        throw std::runtime_error("505");
}

// BUILD THE CGI ENVIRONMENT (META-VARIABLES) FROM THE PARSED REQUEST
void Request::parseCgi_env() {
    // fixed and connection-derived meta-variables
    cgi_env["REQUEST_METHOD"] = method;

    cgi_env["SERVER_PROTOCOL"] = http_version;
    cgi_env["GATEWAY_INTERFACE"] = "CGI/1.1";
    cgi_env["SERVER_SOFTWARE"] = "webserv/1.0";
    cgi_env["SERVER_PORT"] = _server_port;
    cgi_env["REMOTE_ADDR"] = _client_ip;
    cgi_env["REMOTE_PORT"] = _client_port;

    // header-derived meta-variables
    if (headers.count("Host"))
        cgi_env["SERVER_NAME"] = headers.at("Host");
    cgi_env["CONTENT_LENGTH"] =
        headers.count("Content-Length") ? headers.at("Content-Length") : "0";
    if (headers.count("Content-Type"))
        cgi_env["CONTENT_TYPE"] = headers.at("Content-Type");

    // split the resource into path and query string
    size_t query_pos = resource.find("?");
    std::string path = (query_pos != std::string::npos)
                           ? resource.substr(0, query_pos)
                           : resource;

    if (query_pos != std::string::npos) {
        size_t space_pos = resource.find_first_of(" \r\t\n", query_pos);
        cgi_env["QUERY_STRING"] =
            (space_pos != std::string::npos)
                ? resource.substr(query_pos + 1, space_pos - query_pos - 1)
                : resource.substr(query_pos + 1);
    } else {
        cgi_env["QUERY_STRING"] = "";
    }

    // derive SCRIPT_NAME / PATH_INFO from the script extension
    size_t extension_pos = path.find(".py");
    if (extension_pos != std::string::npos) {
        size_t end = path.find("/", extension_pos);
        if (end != std::string::npos) {
            cgi_env["SCRIPT_NAME"] = path.substr(0, end);
            cgi_env["PATH_INFO"] = path.substr(end);
        } else {
            cgi_env["SCRIPT_NAME"] = path;
            cgi_env["PATH_INFO"] = "";
        }
    } else if ((extension_pos = path.find(".php")) != std::string::npos) {
        size_t end = path.find("/", extension_pos);
        if (end != std::string::npos) {
            cgi_env["SCRIPT_NAME"] = path.substr(0, end);
            cgi_env["PATH_INFO"] = path.substr(end);
        } else {
            cgi_env["SCRIPT_NAME"] = path;
            cgi_env["PATH_INFO"] = "";
        }
    }
    cgi_env["SCRIPT_FILENAME"] = _document_root + cgi_env["SCRIPT_NAME"];
    if (!cgi_env["PATH_INFO"].empty())
        cgi_env["PATH_TRANSLATED"] = _document_root + cgi_env["PATH_INFO"];

    // forward every request header as an HTTP_* meta-variable
    for (std::map<std::string, std::string>::const_iterator it =
             headers.begin();
         it != headers.end(); ++it) {
        std::string env_key = "HTTP_" + it->first;
        for (size_t i = 0; i < env_key.length(); ++i)
            env_key[i] = std::toupper(env_key[i]);
        std::replace(env_key.begin(), env_key.end(), '-', '_');
        cgi_env[env_key] = it->second;
    }
}

// ADD ONE KEY/VALUE PAIR TO THE CGI ENVIRONMENT
void Request::addToCgiEnv(std::string key, std::string val) {
    cgi_env[key] = val;
}

// RESET THE REQUEST (NO-OP FOR NOW)
void Request::clear() {}
