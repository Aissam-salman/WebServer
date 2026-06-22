/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: alamjada <alamjada@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/20 18:16:11 by alamjada          #+#    #+#             */
/*   Updated: 2026/06/21 17:32:59 by alamjada         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once
#include "../Request.hpp"
#include <string>
#include <sys/poll.h>

enum e_state_client { READING, WRITTING, DONE };

class Client {
private:
  pollfd _poll_listen;
  std::string _buffer_read;
  std::string _buffer_send;
  e_state_client _status;
  size_t _offset_send;

public:
  Request _request;
  Client(void);
  Client(int fd);
  ~Client(void);

  pollfd getPollfd(void);
  bool handleRecv(void);
  bool isRequestCompleted(void);
  void setResponse(std::string &resp);
  bool handleSend(void);
  e_state_client getStatus(void);
  Request getRequest(void);
  std::string getBufferRead(void) { return _buffer_read; }
  std::string getBufferSend(void) { return _buffer_send; }
};
