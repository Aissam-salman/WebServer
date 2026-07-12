/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Cgi.hpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: alamjada <alamjada@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/18 17:37:36 by alamjada          #+#    #+#             */
/*   Updated: 2026/07/11 09:38:57 by alamjada         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "Client.hpp"
#include <string>
#include <vector>

class Cgi {
private:
  std::vector<std::string> _languagesSupported;
  Client &_client;

  Cgi(void);
	int getContentLength(void);
	std::vector<char *> createArg(void);
	std::vector<char *> createEnvp(std::vector<std::string> &env_strings);
	void childExec(int pipe_body[2], int pipe_resp[2], std::vector<char *> arg, std::vector<char *> envp);
	void dadaExec(int pipe_body[2], int pipe_resp[2], pid_t pid);

public:
  Cgi(std::vector<std::string> ls, Client &client);
  ~Cgi(void);

  void run(void);
};
