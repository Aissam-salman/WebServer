/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Cgi.cpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: alamjada <alamjada@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/18 18:29:37 by alamjada          #+#    #+#             */
/*   Updated: 2026/06/19 21:10:48 by alamjada         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Cgi.hpp"
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
  std::string content_length_s =
      _request.getHeaders().find("Content-Length")->second;
  int ct;
  std::stringstream s(content_length_s);
  s >> ct;

  std::vector<const char *> env;

  env.push_back(
      std::string("REQUEST_METHOD=")
        .append(_request.getMethod())
        .c_str());
  env.push_back(
      std::string("SERVER_PROTOCOL=")
        .append(_request.getHttpVersion())
        .c_str());
  env.push_back(
      std::string("SCRIPT_NAME=")
        .append(_request.getResource())
        .c_str());

  // SCRIPT_FILENAME : need document_root from server config Location ?? 
  // PATH_INFO  check if value after .py depend on ls 
  // PATH_TRANSLA

  // for (int i = 0; i < list.size(); ++i)
  //     strings.push_back(list[i].c_str();

// const char *envp[]
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
  //                       NULL};

  const char *argv[] = {"serve.py", NULL};

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
  if (pid == 0) {
    dup2(pipe_body[0], STDIN_FILENO);
    dup2(pipe_resp[1], STDOUT_FILENO);
    close(pipe_body[1]);
    close(pipe_resp[0]);
    execve("serve.py", const_cast<char *const *>(argv),
           const_cast<char *const *>(envp));
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
