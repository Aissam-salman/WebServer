/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: fardeau <fardeau@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/20 18:40:45 by alamjada          #+#    #+#             */
/*   Updated: 2026/07/13 by fardeau                 ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Client.hpp"
#include "Request.hpp"
#include "Server.hpp"
#include "StaticHandler.hpp"
#include "utils.hpp"
#include <cstddef>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <sstream>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

// STRIP LEADING/TRAILING WHITESPACE (LOCAL COPY: Request.cpp's trim() IS FILE-STATIC)
static std::string trimWs(const std::string &s) {
  size_t start = s.find_first_not_of(" \t");
  if (start == std::string::npos)
    return "";
  size_t end = s.find_last_not_of(" \t");
  return s.substr(start, end - start + 1);
}

// ==== ~TORS ====

// DEFAULT CLIENT: EMBEDDED REQUEST WITH PLACEHOLDER CONNECTION INFO
Client::Client(void) : _max_body_resolved(false), _listener(NULL), _request("8080", "0", "0000", "www") {
  _counter_trash = 0;
  _pid = 0;
  _cgi_pipe_fd_write = -1;
  _offset_cgi_write = 0;
}

// FROM AN ACCEPTED FD: START IN READING AND ARM THE POLLFD FOR POLLIN
Client::Client(int fd)
    : _status(READING), _offset_send(0), _max_body_resolved(false), _listener(NULL), _request("8080", "0", "0000", "www") {
  _poll_listen.fd = fd;
  _poll_listen.events = POLLIN;
  _poll_listen.revents = 0;
  _counter_trash = 0;
  _pid = 0;
  _cgi_pipe_fd_write = -1;
  _offset_cgi_write = 0;
}

// FROM AN FD + PARSED REQUEST (DEFAULT MAX BODY SIZE)
Client::Client(int fd, Request rq)
    : _status(READING), _offset_send(0), _max_body_size(20000000), _max_body_resolved(false), _listener(NULL),_request(rq) {
  _poll_listen.fd = fd;
  _poll_listen.events = POLLIN;
  _poll_listen.revents = 0;
  _counter_trash = 0;
  _pid = 0;
  _cgi_pipe_fd_write = -1;
  _offset_cgi_write = 0;
}

// FROM AN FD + PARSED REQUEST + PER-SERVER MAX BODY SIZE
Client::Client(int fd, Request rq, long max_body_size)
    : _status(READING), _offset_send(0), _max_body_size(max_body_size), _max_body_resolved(false), _listener(NULL),_request(rq) {
  _poll_listen.fd = fd;
  _poll_listen.events = POLLIN;
  _poll_listen.revents = 0;
  _counter_trash = 0;
  _pid = 0;
  _cgi_pipe_fd_write = -1;
  _offset_cgi_write = 0;
}

// COPY CTOR: DELEGATE TO THE COMPILER-GENERATED ASSIGNMENT
Client::Client(const Client &src) : _request(src._request) { *this = src; }

// DESTRUCTOR
Client::~Client(void) {}

// RETURN THE CLIENT'S POLLFD (COPY) FOR REGISTRATION IN THE POLL SET
pollfd Client::getPollfd(void) { return _poll_listen; }

// RECV ONE CHUNK; UPDATE STATE; RETURN FALSE WHEN THE CONNECTION MUST CLOSE
bool Client::handleRecv(void) {
    char buffer[STD_BUFFER];
    ssize_t n = recv(_poll_listen.fd, buffer, STD_BUFFER, 0);
    // n <= 0: peer closed or socket unusable (errno is off-limits) -> close
    if (n <= 0)
        return (_status = DONE, false);

    // draining an oversized body: just count the bytes down
    if (_status == TRASH) {
        long tmp = static_cast<long>(_counter_trash) - n;
        tmp = tmp < 0 ? 0 : tmp;
        _counter_trash = static_cast<size_t>(tmp);
        return true;
    }

    // accumulate and flip to WRITTING once the whole request has arrived
    _buffer_read.append(buffer, n);

    if (isRequestCompleted())
        _status = WRITTING;
    return true;
}

// PICKS THE LOCATION MATCHING THIS REQUEST'S HOST+URI AND ADOPTS ITS MAX BODY SIZE
// (headers are already fully in _buffer_read; _request itself isn't parsed until
// the whole body has arrived, so the request line/Host are read straight off the buffer)
void Client::resolveMaxBodySize(void) {
  size_t line_end = _buffer_read.find("\r\n");
  if (line_end == std::string::npos)
    return;

  std::istringstream iss(_buffer_read.substr(0, line_end));
  std::string method, resource, version;
  iss >> method >> resource >> version;
  if (resource.empty())
    return;

  std::string host;
  size_t host_pos = _buffer_read.find("\r\nHost:");
  if (host_pos != std::string::npos) {
    size_t val_start = host_pos + 7;
    size_t val_end = _buffer_read.find("\r\n", val_start);
    host = trimWs(_buffer_read.substr(val_start, val_end - val_start));
  }

  if (_listener == NULL || _listener->getLinkedServers().empty())
    return;

  Server *server = matchServerByHost(_listener->getLinkedServers(), host);

  try {
    Location &location = StaticHandler::findLocation(server->getLocations(), resource);
    _max_body_size = static_cast<size_t>(location.getMaxBodySize());
  } catch (std::runtime_error &) {
    // no matching location: keep the server-level default, the router 404s later
  }
}

// TELL WHETHER THE FULL REQUEST (HEADERS + EXPECTED BODY) HAS BEEN RECEIVED
// header_end/content_length are parsed once (cached) instead of rescanning the
// whole, possibly huge, buffer from byte 0 on every single recv() chunk
bool Client::isRequestCompleted(void) {
  if (!_max_body_resolved) {
    size_t header_end = _buffer_read.find("\r\n\r\n");
    if (header_end == std::string::npos)
      return false;
    _header_end = header_end;

    resolveMaxBodySize();

    size_t header_contentLength_pos = _buffer_read.find("Content-Length:");
    if (header_contentLength_pos != std::string::npos &&
        header_contentLength_pos < header_end) {
      size_t len_start = header_contentLength_pos + 15;
      size_t len_end = _buffer_read.find("\r\n", len_start);
      std::string lenString = _buffer_read.substr(len_start, len_end - len_start);
      _content_length = static_cast<long>(strToInt(lenString));
    } else {
      size_t chunked_pos = _buffer_read.find("Transfer-Encoding: chunked");
      bool chunked = chunked_pos != std::string::npos && chunked_pos < header_end;
      _content_length = chunked ? -2 : -1; // -2 chunked, -1 no body expected
    }
    _max_body_resolved = true;
  }

  // content-length body: complete once we hold that many body bytes
  if (_content_length >= 0) {
    size_t len = static_cast<size_t>(_content_length);
    size_t body_size = _buffer_read.size() - (_header_end + 4);

    // announced or received body over the limit -> trash the rest
    if (len > _max_body_size || body_size > _max_body_size) {
      _status = TRASH;
      _counter_trash = static_cast<long>(len) - static_cast<long>(body_size);
    }

    return body_size >= len;
  }

  // chunked body: complete once the terminal chunk has arrived. The buffer
  // holds exactly what's been received so far and nothing more, so once the
  // last chunk lands "0\r\n\r\n" is necessarily at the very end of the
  // buffer -> a cheap suffix check replaces a full rescan on every call.
  if (_content_length == -2) {
    if (_buffer_read.size() > _max_body_size)
      _status = TRASH;
    size_t n = _buffer_read.size();
    return n >= 5 && _buffer_read.compare(n - 5, 5, "0\r\n\r\n") == 0;
  }

  // ni Content-Length ni chunked -> pas de body attendu (ex: GET)
  return true;
}

// STORE THE RESPONSE, ARM POLLOUT, AND EMIT ONE ACCESS-LOG LINE
void Client::setResponse(std::string &resp) {
    _buffer_send = resp;
    _offset_send = 0;
    _poll_listen.events = POLLOUT;

    // status line is "HTTP/1.x <code> <reason>": grab the code token
    std::string code = "???";
    size_t sp = resp.find(' ');
    if (sp != std::string::npos) {
        size_t sp2 = resp.find(' ', sp + 1);
        if (sp2 != std::string::npos)
            code = resp.substr(sp + 1, sp2 - (sp + 1));
    }

    // timestamp
    char ts[32];
    std::time_t now = std::time(NULL);
    std::strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", std::localtime(&now));

    // method/path are "-" when no request was parsed (e.g. the 413 trash path)
    const std::string &method = _request.getMethod();
    const std::string &path = _request.getResource();
    std::cout << "[" << ts << "] " << code << " " << _peer << " \""
              << (method.empty() ? "-" : method) << " "
              << (path.empty() ? "-" : path) << "\"" << std::endl;
}

// CURRENT CONNECTION STATE
e_state_client Client::getStatus(void) { return _status; }

// COPY OF THE PARSED REQUEST
Request Client::getRequest(void) { return _request; }

// REMAINING BYTES TO DRAIN FROM AN OVERSIZED BODY
size_t Client::getCounterTrash(void) { return _counter_trash; }

// SEND THE NEXT SLICE OF THE RESPONSE; RETURN FALSE ONCE SENT OR ON CLOSE
bool Client::handleSend(void) {
    ssize_t n = send(_poll_listen.fd, _buffer_send.c_str() + _offset_send,
                     _buffer_send.size() - _offset_send, 0);

    // n <= 0: socket unusable (errno is off-limits) -> close
    if (n <= 0)
        return (_status = DONE, false);
    _offset_send += n;

    // all is send ??
    if (_offset_send >= _buffer_send.size()) {
        _status = DONE;
        return false;
    }
    // still have data
    return true;
}

bool Client::handleSendCgi(void) {
    ssize_t n =
        write(_cgi_pipe_fd_write, _buffer_cgi_write.c_str() + _offset_cgi_write,
              _buffer_cgi_write.size() - _offset_cgi_write);
    // n <= 0: socket unusable (errno is off-limits) -> close
    if (n <= 0)
        return (_status = DONE, false);
    _offset_cgi_write += n;

    // all is send ??
    if (_offset_cgi_write >= _buffer_cgi_write.size()) {
        _status = DONE;
        return false;
    }
    // still have data
    return true;
}

// DROP THE READ BUFFER (USED WHILE TRASHING AN OVERSIZED BODY)
void Client::clearBufferRead(void) { _buffer_read.clear(); }
