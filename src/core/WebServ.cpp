#include "WebServ.hpp"
#include "Cgi.hpp"
#include "Client.hpp"
#include "Lexer.hpp"
#include "Parser.hpp"
#include "Request.hpp"
#include "Response.hpp"
#include "StaticHandler.hpp"
#include "errors.hpp"
#include "utils.hpp"
#include <arpa/inet.h>
#include <csignal>
#include <cstddef>
#include <cstdlib>
#include <fcntl.h>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/poll.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

bool WebServ::_running = true;

// FORMAT A PEER ADDRESS AS "A.B.C.D:PORT" (INET_NTOA IS NOT SUBJECT-ALLOWED)
static std::string formatPeer(const struct sockaddr_in &addr) {
    unsigned long ip = ntohl(addr.sin_addr.s_addr);
    unsigned int port = ntohs(addr.sin_port);
    // build the dotted-quad by hand from the host-order address
    std::ostringstream oss;
    oss << ((ip >> 24) & 0xFF) << '.' << ((ip >> 16) & 0xFF) << '.'
        << ((ip >> 8) & 0xFF) << '.' << (ip & 0xFF) << ':' << port;
    return oss.str();
}

// ==== ~TORS ====

// BUILD THE RUNTIME FROM A CONFIG FILE: LEX -> PARSE SERVERS -> GATHER
// LISTENERS
WebServ::WebServ(const char *config_path) {
    // lex + parse the config into the server vector
    Lexer lexer(config_path);
    lexer.initRawVector();

    Parser parser(lexer.getTokenVector(), _servers);
    parser.initServers();

    // collect the shared host:port listeners across all servers
    _listeners = gatherListeners(_servers);

    // TODO(discuss): CGI interpreter languages, see WebServ.hpp.
    // _languages_supported.push_back("python");
    // _languages_supported.push_back("php");

    signal(SIGINT, handle_sigint);
}

WebServ::~WebServ() {}

// SIGINT HANDLER: FLIP THE RUN FLAG SO THE EVENT LOOP EXITS CLEANLY
void WebServ::handle_sigint(int) {
    WebServ::_running = false;
    std::cerr << ERRS_WEBSERV_SIGNAL << std::endl;
}

// ==== OUTPUTS ====

// PRINT EVERY SERVER BLOCK AND THE SHARED LISTENERS
void WebServ::printConfig(void) {
    for (size_t i = 0; i < _servers.size(); i++)
        _servers[i].printServer();
    printListeners(_listeners);
}

// SWITCH A CLIENT'S POLLFD TO POLLOUT SO ITS RESPONSE CAN BE SENT
void WebServ::switchFdsToPollout(int client_fd) {
    for (size_t j = 0; j < _poll_fds.size(); j++) {
        if (_poll_fds[j].fd == client_fd) {
            _poll_fds[j].events = POLLOUT;
            break;
        }
    }
}

// READ CGI STDOUT FROM THE PIPE; ON EOF BUILD THE RESPONSE AND SWITCH TO
// POLLOUT
void WebServ::readCgiPipe(size_t &i, int fd) {
    int client_fd = _pipe_to_client[fd];
    Client &client = _clients[client_fd];
    char buf[STD_BUFFER];
    int n = read(fd, buf, sizeof(buf));
    if (n < 0) {
        throw std::runtime_error("500");
    }
    // still streaming: accumulate stdout
    if (n > 0) {
        client.appendToBufferCgi(buf, n);
    } else if (n == 0) {
        // eof: reap the child, build the response, close the pipe, flush client
        if (client.getPid() > 0) {
            waitpid(client.getPid(), NULL, WNOHANG);
            std::string resp = buildHttpResponse(client.getBufferCgi());
            client.setResponse(resp);
            client.setStatus(WRITTING);
            switchFdsToPollout(client_fd);
        }
        _pipe_to_client.erase(fd);
        closeClient(i, fd);
    }
}

// CREATE THE CLIENT AND ITS POLLFD, THEN ADD IT TO THE POLL SET (NO PEER)
void WebServ::acceptNewClient(int client_fd) { acceptNewClient(client_fd, ""); }

// SAME, BUT RECORDS THE PEER'S "IP:PORT" ON THE CLIENT FOR THE ACCESS LOG
void WebServ::acceptNewClient(int client_fd, const std::string &peer) {
    // non-blocking is mandatory so one slow peer can't stall the whole loop
    fcntl(client_fd, F_SETFL, O_NONBLOCK | FD_CLOEXEC);
    // TODO: pick Server by Host header / listener instead of a global max size.
    _clients[client_fd] =
        Client(client_fd, Request(), _servers[0].getMaxBodySize());
    _clients[client_fd].setPeer(peer);
    _poll_fds.push_back(_clients[client_fd].getPollfd());
}

// CLOSE A CLIENT FD AND DROP IT FROM THE CLIENT MAP AND THE POLL SET
void WebServ::closeClient(size_t &i, int fd) {
    close(fd);
    _clients.erase(fd);
    _poll_fds.erase(_poll_fds.begin() + i--);
}

// FORK/EXEC THE CGI AND REGISTER ITS OUTPUT PIPE IN THE POLL SET
void WebServ::handleCgi(Client &client, int fd) {
    // TODO(discuss): was Cgi(_languages_supported, client); passing an empty
    // list for now since that member is commented out (see WebServ.hpp).
    Cgi cgi(std::vector<std::string>(), client);
    cgi.run();
    pollfd pfd;
    pfd.fd = client.getCgiPipefd();
    // the pipe read-end must be non-blocking too, or a stuck cgi freezes the
    // loop
    fcntl(pfd.fd, F_SETFL, O_NONBLOCK | FD_CLOEXEC);
    pfd.events = POLLIN;
    pfd.revents = 0;
    _poll_fds.push_back(pfd);
    // map the pipe back to its client so readCgiPipe() can find it
    _pipe_to_client[client.getCgiPipefd()] = fd;
}

// SERVE A STATIC REQUEST: BUILD THE RESPONSE AND SWITCH TO POLLOUT
void WebServ::handleReq(Client &client, int i) {
    // TODO: pick Server by Host header / listener instead of always
    // _servers[0].
    StaticHandler handler(client._request, _servers[0].getLocations());
    Response response = handler.handle();
    std::string bu = response.build();
    client.setResponse(bu);
    _poll_fds[i].events = POLLOUT;
}

// BUILD AN ERROR RESPONSE FROM A THROWN STATUS CODE AND SWITCH TO POLLOUT
void WebServ::responseError(std::runtime_error &e, int i, Client &client) {
    // the exception message carries the http status code (default 500)
    int code = std::atoi(e.what());
    if (code == 0)
        code = 500;
    // TODO: pick Server by Host header / listener instead of always
    // _servers[0].
    Response response = buildErrorResponse(code, _servers[0].getErrorPages());
    std::string bu = response.build();
    client.setResponse(bu);
    _poll_fds[i].events = POLLOUT;
}

// ON POLLIN: RECV BYTES, PARSE WHEN COMPLETE, DISPATCH TO CGI OR STATIC HANDLER
void WebServ::clientRead(size_t &i, int fd) {
    Client &client = _clients[fd];
    bool isHere;

    isHere = client.handleRecv();
    // peer closed the connection
    if (!isHere) {
        closeClient(i, fd);
        return;
    }
    // oversized body: keep draining into the trash
    else if (client.getStatus() == TRASH) {
        client.clearBufferRead();
        // request fully received: parse and route it
    } else if (client.getStatus() == WRITTING) {
        try {
            client._request.parseRequest(client.getBufferRead());

            // GET LOCATION MATCH WITH URL, AND ADD SCRIPT FILENAME TO RUN
            //
            // EXECVE
            Location loc = StaticHandler::findLocation(
                _servers[0].getLocations(), client._request.getResource());

            std::map<std::string, std::string>::const_iterator script_name =
                client._request.getCgi_env().find("SCRIPT_NAME");

            if (script_name != client._request.getCgi_env().end()) {
                std::string root_doc =
                    StaticHandler::resolvePath(loc, script_name->second);
                client._request.addToCgiEnv("SCRIPT_FILENAME", root_doc);
            }

            if (client._request.isCGI())
                handleCgi(client, fd);
            else
                handleReq(client, i);
        } catch (std::runtime_error &e) {
            responseError(e, i, client);
        }
    }
    // finished trashing an oversized body: answer 413
    if (client.getCounterTrash() <= 0 && client.getStatus() == TRASH) {
        Response rp = Response(413, "Playload Too Large");
        std::string ct = rp.build();
        client.setResponse(ct);
        _poll_fds[i].events = POLLOUT;
    }
}

// ON POLLOUT: FLUSH THE RESPONSE; CLOSE THE CLIENT ONCE IT IS FULLY SENT
void WebServ::clientWrite(size_t &i, int fd) {
    Client &client = _clients[fd];
    bool isDone = client.handleSend();
    if (!isDone) {
        closeClient(i, fd);
    }
}

// WALK THE POLL SET AFTER EACH WAKE-UP AND SERVICE EVERY READY FD
void WebServ::loopPollFds(void) {
    // indexed loop: closeClient() erases entries and does i--, so we mutate
    // the container mid-iteration and re-check size() each turn
    for (size_t i = 0; i < _poll_fds.size(); i++) {
        try {

            int fd = _poll_fds[i].fd;

            // nothing happened on this fd this round
            if (!_poll_fds[i].revents)
                continue;

            // case 1: a cgi output pipe has data or eof
            if (_pipe_to_client.count(fd)) {
                readCgiPipe(i, fd);
                // case 2: a listening socket has a new connection to accept
            } else if (_clients.count(fd) == 0) {
                if (_poll_fds[i].revents & POLLIN) {
                    struct sockaddr_in addr;
                    socklen_t addrlen = sizeof(addr);
                    int client_fd =
                        accept(fd, reinterpret_cast<struct sockaddr *>(&addr),
                               &addrlen);
                    // accept failed (client vanished): ignore
                    if (client_fd < 0)
                        continue;
                    acceptNewClient(client_fd, formatPeer(addr));
                }
                // case 3: an established client connection
            } else {
                // client sent bytes: read/parse the request
                if (_poll_fds[i].revents & POLLIN)
                    clientRead(i, fd);
                // socket writable: flush (part of) the response
                else if (_poll_fds[i].revents & POLLOUT) {
                    clientWrite(i, fd);
                    // peer hung up or socket errored: tear it down
                } else if (_poll_fds[i].revents & (POLLHUP | POLLERR)) {
                    closeClient(i, fd);
                }
            }
        } catch (std::runtime_error &e) {
            std::cerr << "Error: loopPoll: " << e.what() << std::endl;
        } catch (std::exception &e) {
            std::cerr << "Error: loopPoll: " << e.what() << std::endl;
        } catch (...) {
            std::cerr << "Error: loopPoll: ?" << std::endl;
        }
    }
    // reap any finished cgi children
    while (waitpid(-1, NULL, WNOHANG) > 0)
        ;
}

// RUN THE SERVER: BIND LISTENERS, REGISTER THEM, THEN LOOP FOREVER
void WebServ::run(void) {
    setupListeners(_listeners);

    // register every listener socket in the poll set
    for (size_t i = 0; i < _listeners.size(); i++) {
        pollfd pfd;
        pfd.fd = _listeners[i].getSocket().getSocketFd();
        pfd.events = POLLIN;
        pfd.revents = 0;
        _poll_fds.push_back(pfd);
    }

    // event loop
    while (_running) {
        int ret = poll(&_poll_fds[0], _poll_fds.size(), TIMEOUT);
        if (ret == -1 && _running)
            throw std::runtime_error(ERRS_WEBSERV_POLL);
        loopPollFds();
    }
}
