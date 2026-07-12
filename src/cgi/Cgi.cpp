/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Cgi.cpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: alamjada <alamjada@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/01 12:05:10 by alamjada          #+#    #+#             */
/*   Updated: 2026/07/12 12:34:48 by alamjada         ###   ########.fr       */
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

typedef std::map<std::string, std::string>::const_iterator mapConstIter;

// CGI FOR THIS CLIENT (WITH THE LIST OF SUPPORTED LANGUAGES)
Cgi::Cgi(std::vector<std::string> ls, Client &client)
    : _languagesSupported(ls), _client(client) {}

// DESTRUCTOR
Cgi::~Cgi(void) {}

// ENVIRONMENT VARIABLE NEEDED BY CGI CONVENTION
std::vector<char *> Cgi::createEnvp(std::vector<std::string> &env_strings) {
  mapConstIter it = _client._request.getCgi_env().begin();
  mapConstIter ite = _client._request.getCgi_env().end();
  for (; it != ite; it++) {
    env_strings.push_back(std::string(it->first) + "=" +
                          std::string(it->second));
  }
  env_strings.push_back("REDIRECT_STATUS=200");

  std::vector<char *> envp;
  for (size_t i = 0; i < env_strings.size(); i++) {
    envp.push_back(const_cast<char *>(env_strings[i].c_str()));
  }
  envp.push_back(NULL);
  return envp;
}

// BUILD THE ARGV ARRAY (THE SCRIPT RESOURCE THEN NULL)
std::vector<char *> Cgi::createArg(void) {
  std::vector<char *> arg;
  arg.push_back(const_cast<char *>((_client._request.getResource().c_str())));
  arg.push_back(NULL);
  return arg;
}

// CONTENT_LENGTH FROM THE CGI ENVIRONMENT AS AN INT
int Cgi::getContentLength(void) {
  std::string content_length_s =
      _client._request.getCgi_env().at("CONTENT_LENGTH");
  int ct;
  std::stringstream s(content_length_s);
  s >> ct;
  return ct;
}

// CREATE THE TWO PIPES AND FORK (THROWS ON ANY FAILURE)
pid_t pipe_and_fork(int pipe_body[2], int pipe_resp[2]) {
  if (pipe(pipe_body) == -1) {
    std::string err = std::string("pipe: ").append(strerror(errno));
    logError(err);
    throw std::runtime_error(err);
  }
  if (pipe(pipe_resp) == -1) {
    close(pipe_body[0]);
    close(pipe_body[1]);
    std::string err = std::string("pipe: ").append(strerror(errno));
    logError(err);
    throw std::runtime_error(err);
  }
  pid_t pid = fork();

  if (pid < 0) {
    close(pipe_body[0]);
    close(pipe_resp[0]);
    close(pipe_body[1]);
    close(pipe_resp[1]);
    std::string err = std::string("fork: ").append(strerror(errno));
    logError(err);
    throw std::runtime_error(err);
  }
  return pid;
}

// CHILD SIDE: WIRE PIPES TO STDIN/STDOUT AND EXECVE THE SCRIPT
void Cgi::childExec(int pipe_body[2], int pipe_resp[2], std::vector<char *> arg,
                    std::vector<char *> envp) {
  dup2(pipe_body[0], STDIN_FILENO);
  dup2(pipe_resp[1], STDOUT_FILENO);
  close(pipe_body[0]);
  close(pipe_body[1]);
  close(pipe_resp[0]);
  close(pipe_resp[1]);
  std::string path = _client._request.getCgi_env().at("SCRIPT_FILENAME");
  execve(path.c_str(), arg.data(), envp.data());
  std::string err = std::string("execve: ").append(strerror(errno));
  logError(err);
  _exit(1);
}

// PARENT SIDE: WRITE THE BODY, KEEP THE RESPONSE PIPE, MARK READING_CGI
void Cgi::dadaExec(int pipe_body[2], int pipe_resp[2], pid_t pid) {
  close(pipe_body[0]);
  close(pipe_resp[1]);
  int content_length = getContentLength();
  if (content_length > 0) {
    int len = write(pipe_body[1], _client._request.getBody().c_str(), content_length);
    if (len < 0) {
      std::string err = std::string("write: ").append(strerror(errno));
      logError(err);
      throw std::runtime_error(err);
    }
  }
  close(pipe_body[1]);
  _client.setCgiPipeFd(pipe_resp[0]);
  _client.setPid(pid);
  _client.setStatus(READING_CGI);
}

// RUN THE CGI: BUILD ENV/ARGV, PIPE+FORK, THEN CHILD EXEC OR PARENT SETUP
void Cgi::run(void) {
	//INFO: env_strings is here to keep the pointer alive after createEnvp and don't lose the env
  std::vector<std::string> env_strings;
  std::vector<char *> envp = createEnvp(env_strings);
  std::vector<char *> arg = createArg();

  int pipe_body[2];
  int pipe_resp[2];
  pid_t pid = pipe_and_fork(pipe_body, pipe_resp);

  if (pid == 0) {
    childExec(pipe_body, pipe_resp, arg, envp);
  } else {
    dadaExec(pipe_body, pipe_resp, pid);
    return;
  }
}
