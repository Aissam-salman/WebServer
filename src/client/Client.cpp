/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: fardeau <fardeau@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/20 18:40:45 by alamjada          #+#    #+#             */
/*   Updated: 2026/07/12 11:44:01 by fardeau          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Client.hpp"
#include "Request.hpp"
#include "utils.hpp"
#include <cstddef>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

// ==== ~TORS ====

// DEFAULT CLIENT: EMBEDDED REQUEST WITH PLACEHOLDER CONNECTION INFO
Client::Client(void) : _request("8080", "0", "0000", "www") {}

// FROM AN ACCEPTED FD: START IN READING AND ARM THE POLLFD FOR POLLIN
Client::Client(int fd)
    : _status(READING), _offset_send(0), _request("8080", "0", "0000", "www") {
  _poll_listen.fd = fd;
  _poll_listen.events = POLLIN;
  _poll_listen.revents = 0;
}

// FROM AN FD + PARSED REQUEST (DEFAULT MAX BODY SIZE)
Client::Client(int fd, Request rq)
    : _status(READING), _offset_send(0), _max_body_size(20000000),_request(rq) {
  _poll_listen.fd = fd;
  _poll_listen.events = POLLIN;
  _poll_listen.revents = 0;
}

// FROM AN FD + PARSED REQUEST + PER-SERVER MAX BODY SIZE
Client::Client(int fd, Request rq, long max_body_size)
    : _status(READING), _offset_send(0), _max_body_size(max_body_size),_request(rq) {
  _poll_listen.fd = fd;
  _poll_listen.events = POLLIN;
  _poll_listen.revents = 0;
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
    _counter_trash -= n;
    return  true;
  }

  // accumulate and flip to WRITTING once the whole request has arrived
  _buffer_read.append(buffer, n);

  if (isRequestCompleted())
    _status = WRITTING;
  return true;
}

// TELL WHETHER THE FULL REQUEST (HEADERS + EXPECTED BODY) HAS BEEN RECEIVED
bool Client::isRequestCompleted(void) {
  // need the end of headers before anything else
  size_t header_end = _buffer_read.find("\r\n\r\n");
  if (header_end == std::string::npos)
    return false;

  // content-length body: complete once we hold that many body bytes
  size_t header_contentLength_pos = _buffer_read.find("Content-Length:");
  if (header_contentLength_pos != std::string::npos &&
      header_contentLength_pos < header_end) {
    size_t len_start = header_contentLength_pos + 15;
    size_t len_end = _buffer_read.find("\r\n", len_start);
    std::string lenString = _buffer_read.substr(len_start, len_end - len_start);
    size_t len = strToInt(lenString);


    size_t body_size = _buffer_read.size() - (header_end + 4);

    // announced or received body over the limit -> trash the rest
    if (static_cast<long>(len) > _max_body_size) {
      _status = TRASH;
      _counter_trash = static_cast<long>(len) - static_cast<long>(body_size);
    }

    if (static_cast<long>(body_size) > _max_body_size) {
      _status = TRASH;
      _counter_trash = static_cast<long>(len) - static_cast<long>(body_size);
    }

    return body_size >= len;
  }

  size_t chunked_pos = _buffer_read.find("Transfer-Encoding: chunked");
  if (chunked_pos != std::string::npos && chunked_pos < header_end) {
    if (static_cast<long>(_buffer_read.size()) > _max_body_size)
      _status = TRASH;
    // requete chunked : pas complete tant que le chunk terminal
    // "0\r\n\r\n" n'est pas arrive dans le body (apres header_end)
    return _buffer_read.find("0\r\n\r\n", header_end) != std::string::npos;
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
size_t Client::getCounterTrash(void) {
  return _counter_trash;
}

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

// DROP THE READ BUFFER (USED WHILE TRASHING AN OVERSIZED BODY)
void Client::clearBufferRead(void){
  _buffer_read.clear();
}
