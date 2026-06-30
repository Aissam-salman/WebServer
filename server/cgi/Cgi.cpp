/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Cgi.cpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: alamjada <alamjada@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/18 18:29:37 by alamjada          #+#    #+#             */
/*   Updated: 2026/06/27 03:09:04 by alamjada         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Cgi.hpp"
#include "Request.hpp"
#include "utils.hpp"
#include <cerrno>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>


Cgi::Cgi(std::vector<std::string> ls, Request rq)
    : _languagesSupported(ls), _request(rq) {}

Cgi::~Cgi(void) {}

std::string buildHttpResponse(const std::string &cgi_output){
	size_t sep_pos = cgi_output.find("\r\n\r\n");
	size_t sep_len = 4;

	if (sep_pos == std::string::npos) {
		// sep dif sometimes language change
		sep_pos = cgi_output.find("\n\n");
		sep_len = 2;
	}
	std::string header;
	std::string body;
	if (sep_pos != std::string::npos) {
		header = cgi_output.substr(0, sep_pos);
		body = cgi_output.substr(sep_pos + sep_len);
	} else {
		// no separator all is body 
		body = cgi_output;
	}
	
	// default status
	std::string status_line = "200 OK";
	std::istringstream header_stream(header);
	std::string line;
	std::string saved_header;

	while (std::getline(header_stream, line)) {
		if (!line.empty() && line[line.size() - 1] == '\r'){
			line.erase(line.size() - 1);
		}
		if (line.empty())
			continue;
		if (line.compare(0, 8, "Status: ") == 0)
			status_line = line.substr(8);
		else
			saved_header += line + "\r\n";
	}
	saved_header += "Connection: close\r\n";
	
	// build resp
	std::ostringstream resp;
	resp << "HTTP/1.1 " << status_line << "\r\n";
	resp << saved_header;
	if (saved_header.find("Content-Length:") == std::string::npos){
		resp << "Content-Length: " << body.size() << "\r\n";
	}
	resp << "\r\n" << body;
	return resp.str();
}

std::string Cgi::run(void) {
  std::vector<std::string> env_strings;
  std::vector<char *> envp;
  std::vector<char *> arg;

  std::map<std::string, std::string>::const_iterator it =
      _request.getCgi_env().begin();
  std::map<std::string, std::string>::const_iterator ite =
      _request.getCgi_env().end();

  for (; it != ite; it++) {
    env_strings.push_back(std::string(it->first) + "=" + std::string(it->second));
  }
	env_strings.push_back("REDIRECT_STATUS=200");

  for (size_t i = 0; i < env_strings.size(); i++) {
      envp.push_back(const_cast<char *>(env_strings[i].c_str()));
  }
  envp.push_back(NULL);
  arg.push_back(const_cast<char *>((_request.getResource().c_str())));

  std::string content_length_s = _request.getCgi_env().at("CONTENT_LENGTH");
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
    close(pipe_resp[0]);
    std::string path = _request.getCgi_env().at("SCRIPT_FILENAME");
    execve(path.c_str(), arg.data(), envp.data());
		std::string err = std::string("excve: ").append(strerror(errno));
		logError(err);
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
    return buildHttpResponse(resp);
  }
}
