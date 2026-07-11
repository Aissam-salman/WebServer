/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: alamjada <alamjada@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/20 18:16:11 by alamjada          #+#    #+#             */
/*   Updated: 2026/07/11 15:40:44 by alamjada         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "Request.hpp"
#include <sched.h>
#include <string>
#include <sys/poll.h>

enum e_state_client { READING, WRITTING, DONE, READING_CGI, TRASH };

class Client {
  private:
    pollfd _poll_listen;
    std::string _buffer_read;
    std::string _buffer_send;
    e_state_client _status;
    size_t _offset_send;
    int _cgi_pipe_fd;
    pid_t _pid;
    std::string _buffer_cgi;
    long _max_body_size;
    size_t _counter_trash;

  public:
    Client(void);
    Client(int fd);
    Client(const Client &src);
    Client(int fd, Request req, long max_body_size);
    Client(int fd, Request req);
    ~Client(void);

    Request _request;

    bool handleRecv(void);
    bool isRequestCompleted(void);
    bool handleSend(void);

    pollfd getPollfd(void);
    e_state_client getStatus(void);
    Request getRequest(void);
    std::string &getBufferRead(void) { return _buffer_read; }
    std::string &getBufferSend(void) { return _buffer_send; }
    int getCgiPipefd(void) { return _cgi_pipe_fd; }
    pid_t getPid(void) { return _pid; }
    std::string getBufferCgi(void) const { return _buffer_cgi; }
    size_t getCounterTrash(void);
    void clearBufferRead(void);

    void setStatus(enum e_state_client e) { _status = e; }
    void setCgiPipeFd(int fd) { _cgi_pipe_fd = fd; }
    void setResponse(std::string &resp);
    void setPid(pid_t pid) { _pid = pid; }
    void appendToBufferCgi(char *msg, int n) { _buffer_cgi.append(msg, n); }
};
