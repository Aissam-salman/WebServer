/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: alamjada <alamjada@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/20 18:16:11 by alamjada          #+#    #+#             */
/*   Updated: 2026/07/12 22:38:44 by salman           ###   ########.fr       */
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
    int _cgi_pipe_fd_write;
    pid_t _pid;
    std::string _buffer_cgi;
    std::string _buffer_cgi_write;
    size_t _offset_cgi_write;
    size_t _max_body_size;
    size_t _counter_trash;
    std::string _peer; // "IP:port" of the connected client, for access logging

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
    bool handleSendCgi(void);

    pollfd getPollfd(void);
    e_state_client getStatus(void);
    Request getRequest(void);
    std::string &getBufferRead(void) { return _buffer_read; }
    std::string &getBufferSend(void) { return _buffer_send; }
    int getCgiPipefd(void) { return _cgi_pipe_fd; }
    int getCgiPipefdWrite(void) { return _cgi_pipe_fd_write; }
    pid_t getPid(void) { return _pid; }
    std::string getBufferCgi(void) const { return _buffer_cgi; }
    size_t getCounterTrash(void);
    void clearBufferRead(void);

    void setStatus(enum e_state_client e) { _status = e; }
    void setCgiPipeFd(int fd) { _cgi_pipe_fd = fd; }
    void setCgiPipeFdWrite(int fd) { _cgi_pipe_fd_write = fd; }
    void setResponse(std::string &resp);
    void setPeer(const std::string &peer) { _peer = peer; }
    void setPid(pid_t pid) { _pid = pid; }
    void appendToBufferCgi(char *msg, int n) { _buffer_cgi.append(msg, n); }
    void appendToBufferCgiWrite(char *msg, int n) { _buffer_cgi_write.append(msg, n); }
    void appendToBufferCgiWrite(const char *msg, int n) { _buffer_cgi_write.append(msg, n); }
};
