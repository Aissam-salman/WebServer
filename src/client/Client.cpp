/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: alamjada <alamjada@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/20 18:40:45 by alamjada          #+#    #+#             */
/*   Updated: 2026/07/11 15:56:16 by alamjada         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Client.hpp"
#include "Request.hpp"
#include "utils.hpp"
#include <cstddef>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <stdexcept>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

Client::Client(void) : _request("8080", "0", "0000", "www") {}

Client::Client(int fd)
    : _status(READING), _offset_send(0), _request("8080", "0", "0000", "www") {
  _poll_listen.fd = fd;
  _poll_listen.events = POLLIN;
  _poll_listen.revents = 0;
}

Client::Client(int fd, Request rq)
    : _status(READING), _offset_send(0), _max_body_size(20000000),_request(rq) {
  _poll_listen.fd = fd;
  _poll_listen.events = POLLIN;
  _poll_listen.revents = 0;
}

Client::Client(int fd, Request rq, long max_body_size)
    : _status(READING), _offset_send(0), _max_body_size(max_body_size),_request(rq) {
  _poll_listen.fd = fd;
  _poll_listen.events = POLLIN;
  _poll_listen.revents = 0;
}

Client::Client(const Client &src) : _request(src._request) { *this = src; }

Client::~Client(void) {}

pollfd Client::getPollfd(void) { return _poll_listen; }

bool Client::handleRecv(void) {
  char buffer[STD_BUFFER];
  ssize_t n = recv(_poll_listen.fd, buffer, STD_BUFFER, 0);
  // poll() flagged this fd readable, so recv() should not block. The 42 subject
  // forbids inspecting errno after recv(), so we can't distinguish EAGAIN from a
  // real error: on any n <= 0 we just close the connection (like a peer EOF).
  if (n <= 0)
    return (_status = DONE, false);

  if (_status == TRASH) {
    _counter_trash -= n;
    return  true;
  }

  _buffer_read.append(buffer, n);

  if (isRequestCompleted())
    _status = WRITTING;
  return true;
}

bool Client::isRequestCompleted(void) {
  size_t header_end = _buffer_read.find("\r\n\r\n");
  if (header_end == std::string::npos)
    return false;

  size_t header_contentLength_pos = _buffer_read.find("Content-Length:");
  if (header_contentLength_pos != std::string::npos &&
      header_contentLength_pos < header_end) {
    size_t len_start = header_contentLength_pos + 15;
    size_t len_end = _buffer_read.find("\r\n", len_start);
    std::string lenString = _buffer_read.substr(len_start, len_end - len_start);
    size_t len = strToInt(lenString);

    
    size_t body_size = _buffer_read.size() - (header_end + 4);

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

// Single choke point for every response (static / CGI / error / 413), so the
// access log lives here: one line per served request -> "[timestamp] code peer".
void Client::setResponse(std::string &resp) {
  _buffer_send = resp;
  _offset_send = 0;
  _poll_listen.events = POLLOUT;

  // Status line is "HTTP/1.x <code> <reason>": grab the token between spaces.
  std::string code = "???";
  size_t sp = resp.find(' ');
  if (sp != std::string::npos) {
    size_t sp2 = resp.find(' ', sp + 1);
    if (sp2 != std::string::npos)
      code = resp.substr(sp + 1, sp2 - (sp + 1));
  }

  char ts[32];
  std::time_t now = std::time(NULL);
  std::strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", std::localtime(&now));

  // Method/path may be empty on paths that never parsed a request (e.g. the
  // 413 "payload too large" trash path) -> show "-" like nginx does.
  const std::string &method = _request.getMethod();
  const std::string &path = _request.getResource();
  std::cout << "[" << ts << "] " << code << " " << _peer << " \""
            << (method.empty() ? "-" : method) << " "
            << (path.empty() ? "-" : path) << "\"" << std::endl;
}

e_state_client Client::getStatus(void) { return _status; }

Request Client::getRequest(void) { return _request; }

size_t Client::getCounterTrash(void) {
  return _counter_trash;
}

bool Client::handleSend(void) {
  ssize_t n = send(_poll_listen.fd, _buffer_send.c_str() + _offset_send,
                   _buffer_send.size() - _offset_send, 0);

  // Same rule as handleRecv: poll() flagged POLLOUT, errno is off-limits, so a
  // negative return means the socket is unusable -> close the connection.
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

void Client::clearBufferRead(void){
  _buffer_read.clear();
}
