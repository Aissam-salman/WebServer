/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: alamjada <alamjada@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/20 18:40:45 by alamjada          #+#    #+#             */
/*   Updated: 2026/07/12 12:35:37 by alamjada         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Client.hpp"
#include "Request.hpp"
#include "utils.hpp"
#include <cstddef>
#include <cstdlib>
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
  _counter_trash = 0;
}

Client::Client(int fd, Request rq)
    : _status(READING), _offset_send(0), _max_body_size(20000000),_request(rq) {
  _poll_listen.fd = fd;
  _poll_listen.events = POLLIN;
  _poll_listen.revents = 0;
  _counter_trash = 0;
}

Client::Client(int fd, Request rq, long max_body_size)
    : _status(READING), _offset_send(0), _max_body_size(max_body_size),_request(rq) {
  _poll_listen.fd = fd;
  _poll_listen.events = POLLIN;
  _poll_listen.revents = 0;
  _counter_trash = 0;
}

Client::Client(const Client &src) : _request(src._request) { *this = src; }

Client::~Client(void) {}

pollfd Client::getPollfd(void) { return _poll_listen; }

bool Client::handleRecv(void) {
  char buffer[STD_BUFFER];
  ssize_t n = recv(_poll_listen.fd, buffer, STD_BUFFER, 0);
  if (n < 0)
    throw std::runtime_error("500");
  if (n == 0)
    return (_status = DONE, false);

  if (_status == TRASH) {
    long tmp = _counter_trash - n < 0 ? 0 : _counter_trash - n;
    _counter_trash = static_cast<size_t>(tmp);
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

    if (len > _max_body_size) {
      _status = TRASH;
      _counter_trash = static_cast<long>(len) - static_cast<long>(body_size);
    }

    if (body_size > _max_body_size) {
      _status = TRASH;
      _counter_trash = static_cast<long>(len) - static_cast<long>(body_size);
    }

    return body_size >= len;
  }

  size_t chunked_pos = _buffer_read.find("Transfer-Encoding: chunked");
  if (chunked_pos != std::string::npos && chunked_pos < header_end) {
    if (_buffer_read.size() > _max_body_size)
      _status = TRASH;
    // requete chunked : pas complete tant que le chunk terminal
    // "0\r\n\r\n" n'est pas arrive dans le body (apres header_end)
    return _buffer_read.find("0\r\n\r\n", header_end) != std::string::npos;
  }

  // ni Content-Length ni chunked -> pas de body attendu (ex: GET)
  return true;
}

void Client::setResponse(std::string &resp) {
  _buffer_send = resp;
  _offset_send = 0;
  _poll_listen.events = POLLOUT;
}

e_state_client Client::getStatus(void) { return _status; }

Request Client::getRequest(void) { return _request; }

size_t Client::getCounterTrash(void) {
  return _counter_trash;
}

bool Client::handleSend(void) {
  ssize_t n = send(_poll_listen.fd, _buffer_send.c_str() + _offset_send,
                   _buffer_send.size() - _offset_send, 0);

  if (n < 0)
    throw std::runtime_error("500");
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
