/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Cgi.cpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: alamjada <alamjada@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/18 18:29:37 by alamjada          #+#    #+#             */
/*   Updated: 2026/06/20 18:02:42 by alamjada         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Cgi.hpp"
#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

// std::string method;
// std::string http_version;
// std::string resource;
// std::map<std::string, std::string> headers;
// std::string body;

// const char *envp[] = {"REQUEST_METHOD=POST",
//                       "SERVER_PROTOCOL=HTTP/1.1",
//                       "SERVER_NAME=localhost",
//                       "SERVER_PORT=8080",
//                       "GATEWAY_INTERFACE=CGI/1.1",
//                       "SERVER_SOFTWARE=webserv/1.0",
//                       "REMOTE_ADDR=0",
//                       "REMOTE_PORT=0",
//                       "SCRIPT_NAME=/server.py",
//                       "SCRIPT_FILENAME=/www/scripts/server.py",
//                       "PATH_INFO=",
//                       "PATH_TRANSLATED=",
//                       "QUERY_STRING=",
//                       "CONTENT_LENGTH=36",
//                       "CONTENT_TYPE=text/plain",
// path script

Cgi::Cgi(void) { _languagesSupported.push_back("python"); }

Cgi::Cgi(std::vector<std::string> ls) : _languagesSupported(ls) {}

Cgi::Cgi(std::vector<std::string> ls, Request rq)
    : _languagesSupported(ls), _request(rq) {}

Cgi::~Cgi(void) {}

void Cgi::run(void) {
  std::vector<char *> envp;
  std::vector<std::string> envString;
  std::vector<char *> arg;

  std::string content_length_s =
      _request.getHeaders().find("Content-Length")->second;
  int ct;
  std::stringstream s(content_length_s);
  s >> ct;

  envString.push_back("CONTENT_LENGTH=" + content_length_s);
  envString.push_back("CONTENT_TYPE=" +
                      _request.getHeaders().find("Content-Type")->second);

  envString.push_back("REQUEST_METHOD=" + _request.getMethod());
  envString.push_back("SERVER_PROTOCOL=" + _request.getHttpVersion());

  // /cgi-bin/serve.py/create/user?option=true
  if (_request.getResource().find(".py") != std::string::npos) {
    envString.push_back("SCRIPT_NAME=" +
                        _request.getResource().substr(
                            0, _request.getResource().find(".py") + 3));
    envString.push_back("SCRIPT_FILENAME=/www/cgi-bin" +
                        _request.getResource().substr(
                            0, _request.getResource().find(".py") + 3));
  }

  if (_request.getResource().find('?') != std::string::npos) {
    std::size_t start = _request.getResource().find(".py") + 3;
    std::size_t end = _request.getResource().find('?');
    envString.push_back("PATH_INFO=" +
                        _request.getResource().substr(start, end - start));
    envString.push_back("PATH_TRANSLATED=/www" +
                        _request.getResource().substr(start, end - start));
    envString.push_back("QUERY_STRING=" +
                        _request.getResource().substr(end + 1));
  }

  std::vector<std::string>::iterator it = envString.begin();
  std::vector<std::string>::iterator ite = envString.end();
  for (; it != ite; ++it) {
    envp.push_back(const_cast<char *>((*it).c_str()));
  }
  envp.push_back(NULL);

  // from server
  //                       "SERVER_NAME=localhost",
  //                       "SERVER_PORT=8080",
  //                       "GATEWAY_INTERFACE=CGI/1.1",
  //                       "SERVER_SOFTWARE=webserv/1.0",
  //                       "REMOTE_ADDR=0",
  //                       "REMOTE_PORT=0",

  // const char *envp[]
  //                       "SERVER_NAME=localhost",
  //                       "SERVER_PORT=8080",
  //                       "GATEWAY_INTERFACE=CGI/1.1",
  //                       "SERVER_SOFTWARE=webserv/1.0",
  //                       "REMOTE_ADDR=0",
  //                       "REMOTE_PORT=0",
  //                       "SCRIPT_NAME=/cgi-bin/server.py",
  //                       "SCRIPT_FILENAME=/www/cgi-bin/server.py",
  //                       "PATH_INFO=",
  //                       "PATH_TRANSLATED=",
  //                       "QUERY_STRING=",
  //                       "CONTENT_LENGTH=36",
  //                       "CONTENT_TYPE=text/plain",
  //                       NULL};

  arg.push_back(const_cast<char *>((_request.getResource().c_str())));

  int pipe_body[2];
  int pipe_resp[2];
  if (pipe(pipe_body) == -1) {
    std::cerr << "Error: pipe: " << strerror(errno) << std::endl;
    return;
  }
  if (pipe(pipe_resp) == -1) {
    std::cerr << "Error: pipe: " << strerror(errno) << std::endl;
    return;
  }

  pid_t pid = fork();
  if (pid < 0) {
    std::cerr << "Error: fork: " << strerror(errno) << std::endl;
  }
  if (pid == 0) {
    dup2(pipe_body[0], STDIN_FILENO);
    dup2(pipe_resp[1], STDOUT_FILENO);
    close(pipe_body[1]);
    close(pipe_resp[0]);
    // FIX: SCRIPT_FILENAME
    execve("serve.py", arg.data(), envp.data());
    _exit(1);
  } else {
    close(pipe_body[0]);
    close(pipe_resp[1]);

    // send  body
    if (ct > 0) {
      write(pipe_body[1], _request.getBody().c_str(), ct);
    }
    close(pipe_body[1]);

    // read resp
    char buf[4096];
    std::string resp;
    size_t n;
    while ((n = read(pipe_resp[0], buf, sizeof(buf))) > 0)
      resp.append(buf, n);
    waitpid(pid, NULL, 0);
    close(pipe_resp[0]);
    std::cout << resp << std::endl;
  }
}
