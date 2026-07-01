/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Cgi.cpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: alamjada <alamjada@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/01 12:05:10 by alamjada          #+#    #+#             */
/*   Updated: 2026/07/01 12:05:34 by alamjada         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Cgi.hpp"

#include "utils.hpp"
#include <cerrno>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unistd.h>

Cgi::Cgi(std::vector<std::string> ls, Client *client)
    : _languagesSupported(ls), _client(client) {}

Cgi::~Cgi(void) {}

void Cgi::run(void) {
  std::vector<std::string> env_strings;
  std::vector<char *> envp;
  std::vector<char *> arg;

  std::map<std::string, std::string>::const_iterator it =
      _client->_request.getCgi_env().begin();
  std::map<std::string, std::string>::const_iterator ite =
      _client->_request.getCgi_env().end();

  for (; it != ite; it++) {
    env_strings.push_back(std::string(it->first) + "=" +
                          std::string(it->second));
  }
  env_strings.push_back("REDIRECT_STATUS=200");

  for (size_t i = 0; i < env_strings.size(); i++) {
    envp.push_back(const_cast<char *>(env_strings[i].c_str()));
  }
  envp.push_back(NULL);
  arg.push_back(const_cast<char *>((_client->_request.getResource().c_str())));

  std::string content_length_s =
      _client->_request.getCgi_env().at("CONTENT_LENGTH");
  int ct;
  std::stringstream s(content_length_s);
  s >> ct;
  int pipe_body[2];
  int pipe_resp[2];
  if (pipe(pipe_body) == -1) {
    std::string err = std::string("pipe: ").append(strerror(errno));
    logError(err);
    throw std::runtime_error(err);
  }
  if (pipe(pipe_resp) == -1) {
    std::string err = std::string("pipe: ").append(strerror(errno));
    logError(err);
    throw std::runtime_error(err);
  }
  pid_t pid = fork();
  if (pid < 0) {
    std::string err = std::string("fork: ").append(strerror(errno));
    logError(err);
    throw std::runtime_error(err);
  }
  if (pid == 0) {
    dup2(pipe_body[0], STDIN_FILENO);
    dup2(pipe_resp[1], STDOUT_FILENO);
    close(pipe_body[1]);
    close(pipe_body[0]);
    close(pipe_resp[0]);
    close(pipe_resp[1]);
    std::string path = _client->_request.getCgi_env().at("SCRIPT_FILENAME");
    execve(path.c_str(), arg.data(), envp.data());
    std::string err = std::string("excve: ").append(strerror(errno));
    logError(err);
    _exit(1);
  } else {
    close(pipe_body[0]);
    close(pipe_resp[1]);
    // send  body
    if (ct > 0) {
      write(pipe_body[1], _client->_request.getBody().c_str(), ct);
    }
    close(pipe_body[1]);

		_client->setCgiPipeFd(pipe_resp[0]);
		_client->setPid(pid);
		_client->setStatus(READING_CGI);
    return;
  }
}
