/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: tgomez-f <tgomez-f@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/20 18:40:45 by alamjada          #+#    #+#             */
/*   Updated: 2026/06/24 11:29:43 by tgomez-f         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Client.hpp"
#include "Cgi.hpp"
#include "Request.hpp"
#include "Response.hpp"
#include "utils.hpp"
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

Client::Client(void) : _request("8080", "0", "0000", "www") {}

Client::Client(int fd)
    : _status(READING), _offset_send(0), _request("8080", "0", "0000", "www") {
  _poll_listen.fd = fd;
  _poll_listen.events = POLLIN;
  _poll_listen.revents = 0;
}

Client::Client(int fd, Request rq)
    : _status(READING), _offset_send(0), _request(rq) {
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
  if (n < 0)
    throw std::runtime_error("recv fail.");
  if (n == 0)
    return (_status = DONE, false);

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
  if (header_contentLength_pos != std::string::npos) {
    size_t len_start = header_contentLength_pos + 15;
    size_t len_end = _buffer_read.find("\r\n", len_start);
    std::string lenString = _buffer_read.substr(len_start, len_end - len_start);
    size_t len = strToInt(lenString);
    size_t body_size = _buffer_read.size() - (header_end + 4);
    return body_size >= len;
  }
  return true;
}

void Client::setResponse(std::string &resp) {
  _buffer_send = resp;
  _offset_send = 0;
  _poll_listen.events = POLLOUT;
}

e_state_client Client::getStatus(void) { return _status; }

Request Client::getRequest(void) { return _request; }

bool Client::handleSend(void) {
  ssize_t n = send(_poll_listen.fd, _buffer_send.c_str() + _offset_send,
                   _buffer_send.size() - _offset_send, 0);

  if (n < 0)
    throw std::runtime_error("send fail.");
  _offset_send += n;

  // all is send ??
  if (_offset_send >= _buffer_send.size()) {
    _status = DONE;
    return false;
  }
  // still have data
  return true;
}

Client *Client::clone(void) { return new Client(*this); }

void Client::process(void) {
  try {
		std::cerr << "[DEBUG]" << std::endl;
    _request.parseRequest(_buffer_read);
    Response resp(200);
    std::string bu = resp.build();
    setResponse(bu);
  } catch (std::runtime_error &e) {
    int code = std::atoi(e.what());
    if (code == 0)
      code = 500; // si e.what() n'est pas un code HTTP
    Response resp(code);
    std::string bu = resp.build();
    setResponse(bu);
  } catch (...) {
		std::cerr << "process client" << std::endl;
	}
}
