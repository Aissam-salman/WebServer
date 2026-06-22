/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Cgi.hpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: alamjada <alamjada@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/18 17:37:36 by alamjada          #+#    #+#             */
/*   Updated: 2026/06/22 16:59:34 by alamjada         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once
#include "../Request.hpp"
#include <string>
#include <vector>

class Cgi {
private:
  std::vector<std::string> _languagesSupported;
  Request _request;

public:
  Cgi(void);
  ~Cgi(void);

  Cgi(std::vector<std::string> ls);
  Cgi(std::vector<std::string> ls, Request rq);

  std::string run(void);
};

/*
 *

The flow is: fork → pipe setup → execve with env → write body to stdin → read
response from stdout.

The execve call

char *envp[] = {
    "REQUEST_METHOD=POST",
    "CONTENT_LENGTH=42",
    "CONTENT_TYPE=application/x-www-form-urlencoded",
    "QUERY_STRING=name=foo",
    // ...
    NULL
};

// Child process
dup2(pipe_in[0], STDIN_FILENO);   // request body → stdin
dup2(pipe_out[1], STDOUT_FILENO); // stdout → response
execve(script_path, argv, envp);   // envp is your CGI env vars
                                   //
* Request meta
*
* =



* REQUEST_METHOD :  GET, POST, DELETE
* SERVER_PROTOCOL : HTTP/1.1
* SERVER_NAME :  from Host: in header
* SERVER_PORT : 8080
* GATEWAY_INTERFACE : GCI/1.1
* SERVER_SOFTWARE: server identifier  (ex: webserv/1.0)
* REMOTE_ADDR: client op form accept() sockaddr
* REMOTE_PORT: client port
*
*
* URL parsing
* SCRIPT_NAME : /script/hello.py
* SCRIPT_FILENAME : /var/www/script/hello.py
* PATH_INFO : extra after /hello.py/foo/bar
* PATH_TRANSLATED: PATH_INFO + doc root
* QUERY_STRING : txt after ? in url     /script/hello/py?name=foo
*
*
*Requête : POST /cgi-bin/router.py/user/create
Avec document_root = /var/www/html
SCRIPT_NAME     = /cgi-bin/router.py
SCRIPT_FILENAME = /var/www/html/cgi-bin/router.py   (chemin réel du script
exécuté) PATH_INFO       = /user/create PATH_TRANSLATED =
/var/www/html/user/create          (PATH_INFO traduit en chemin disque)

* Body
* CONTENT_LENGTH byte size
* CONTENT_TYPE : from Content-Type
*
* extra
*
| `User-Agent: curl/8.4` | `HTTP_USER_AGENT=curl/8.4` |
| ---------------------- | -------------------------- |
| `Cookie: a=1; b=2`     | `HTTP_COOKIE=a=1; b=2`     |
| `Accept: text/html`    | `HTTP_ACCEPT=text/html`    |
| `Host: example.com`    | `HTTP_HOST=example.com`    |
*
*
*/
