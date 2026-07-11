#include "WebServ.hpp"
#include "Cgi.hpp"
#include "Client.hpp"
#include "Lexer.hpp"
#include "Location.hpp"
#include "Parser.hpp"
#include "Request.hpp"
#include "Response.hpp"
#include "StaticHandler.hpp"
#include "configutils.hpp"
#include "utils.hpp"
#include <csignal>
#include <cstddef>
#include <cstdlib>
#include <fcntl.h>
#include <iostream>
#include <stdexcept>
#include <string>
#include <sys/poll.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

bool WebServ::_running = true;

// ==== ~TORS ====
// Builds the whole runtime from a config file: lex -> parse into servers ->
// gather the shared listeners. Process-global concerns (signal handling) live
// here, not in any single Server.
WebServ::WebServ(const char *config_path) {
    Lexer lexer(config_path);
    lexer.initRawVector();

    Parser parser(lexer.getTokenVector(), _servers);
    parser.initServers();

    _listeners = gatherListeners(_servers);

    // TODO(discuss): CGI interpreter languages, see WebServ.hpp.
    // _languages_supported.push_back("python");
    // _languages_supported.push_back("php");

    signal(SIGINT, handle_sigint);
}

WebServ::~WebServ() {}

void WebServ::handle_sigint(int) {
    WebServ::_running = false;
    std::cerr << "SIGNAL RECEIVED" << std::endl;
}

// ==== OUTPUTS ====
void WebServ::printConfig(void) {
    for (size_t i = 0; i < _servers.size(); i++)
        _servers[i].printServer();
    printListeners(_listeners);
}

void WebServ::switchFdsToPollout(int client_fd) {
    for (size_t j = 0; j < _poll_fds.size(); j++) {
        if (_poll_fds[j].fd == client_fd) {
            _poll_fds[j].events = POLLOUT;
            break;
        }
    }
}

/*
 * read pipe from execve stdout, and switch to POLLOUT to send response http
 */
void WebServ::readCgiPipe(size_t &i, int fd) {
    int client_fd = _pipe_to_client[fd];
    Client &client = _clients[client_fd];
    char buf[STD_BUFFER];
    int n = read(fd, buf, sizeof(buf));
    if (n < 0) {
        throw std::runtime_error("500");
    }
    if (n > 0) {
        client.appendToBufferCgi(buf, n);
    } else if (n == 0) {
        waitpid(client.getPid(), NULL, WNOHANG);
        std::string resp = buildHttpResponse(client.getBufferCgi());
        client.setResponse(resp);
        client.setStatus(WRITTING);
        closeClient(i, fd);
        switchFdsToPollout(client_fd);
    }
}

// create client class, and poll_fd for client and add to pollfds
void WebServ::acceptNewClient(int listen_fd) {

    fcntl(listen_fd, F_SETFD, FD_CLOEXEC);
    // TODO: pick Server by Host header / listener instead of a global max size.
    _clients[listen_fd] =
        Client(listen_fd, Request(), _servers[0].getMaxBodySize());
    _poll_fds.push_back(_clients[listen_fd].getPollfd());
}

void WebServ::closeClient(size_t &i, int fd) {
    close(fd);
    _clients.erase(fd);
    _poll_fds.erase(_poll_fds.begin() + i--);
}

// send client to cgi execve, with request already parsed
// and add to pollfds,
// i add to pipe_to_client to keep the stdout pipe of execve, for the response
// from cgi script (non bloquant)
// associate the pipe with client
void WebServ::handleCgi(Client &client, int fd) {
    // TODO(discuss): was Cgi(_languages_supported, client); passing an empty
    // list for now since that member is commented out (see WebServ.hpp).
    Cgi cgi(std::vector<std::string>(), client);
    cgi.run();
    pollfd pfd;
    pfd.fd = client.getCgiPipefd();
    pfd.events = POLLIN;
    _poll_fds.push_back(pfd);
    _pipe_to_client[client.getCgiPipefd()] = fd;
}

void WebServ::handleReq(Client &client, int i) {
    // TODO: pick Server by Host header / listener instead of always _servers[0].
    StaticHandler handler(client._request, _servers[0].getLocations());
    Response response = handler.handle();
    std::string bu = response.build();
    client.setResponse(bu);
    _poll_fds[i].events = POLLOUT;
}

// if catch std::runtime_error in clientRead catch
void WebServ::responseError(std::runtime_error &e, int i, Client &client) {
    int code = std::atoi(e.what());
    if (code == 0)
        code = 500;
    // TODO: pick Server by Host header / listener instead of always _servers[0].
    Response response = buildErrorResponse(code, _servers[0].getErrorPages());
    std::string bu = response.build();
    client.setResponse(bu);
    _poll_fds[i].events = POLLOUT;
}

void WebServ::clientRead(size_t &i, int fd) {
    Client &client = _clients[fd];
    bool isHere;

    isHere = client.handleRecv();
    if (!isHere)
        closeClient(i, fd);
    else if (client.getStatus() == TRASH) {
        client.clearBufferRead();
    } else if (client.getStatus() == WRITTING) {
        try {
            client._request.parseRequest(client.getBufferRead());

            if (client._request.isCGI())
                handleCgi(client, fd);
            else
                handleReq(client, i);
        } catch (std::runtime_error &e) {
            responseError(e, i, client);
        }
    }
    if (client.getCounterTrash() <= 0 && client.getStatus() == TRASH) {
        Response rp = Response(413, "Playload Too Large");
        std::string ct = rp.build();
        client.setResponse(ct);
        _poll_fds[i].events = POLLOUT;
    }
}

void WebServ::clientWrite(size_t &i, int fd) {
    Client &client = _clients[fd];
    bool isDone = client.handleSend();
    if (!isDone) {
        closeClient(i, fd);
    }
}

// Called once after every poll() wake-up. poll() has just filled in the
// .revents field of each pollfd; our job here is to walk the whole set and
// react to every fd that became ready. We handle THREE kinds of fd, all living
// in the same _poll_fds array:
//   1. CGI pipe fds     -> present in _pipe_to_client (pipe read-end <- CGI
//   stdout)
//   2. listening fds    -> not a client and not a pipe (a bound server socket)
//   3. client conn fds  -> present in _clients (an accepted TCP connection)
void WebServ::loopPollFds(void) {

    // Indexed loop (not an iterator) on purpose: closeClient() erases entries
    // from _poll_fds and does `i--`, so the container is mutated mid-iteration.
    // We also re-check .size() each turn because acceptNewClient()/handleCgi()
    // push_back new fds while we loop.
    for (size_t i = 0; i < _poll_fds.size(); i++) {
        int fd = _poll_fds[i].fd;

        // .revents == 0 -> poll() reported nothing happened on this fd this
        // round. Skip it; only fds with a pending event are worth handling.
        if (!_poll_fds[i].revents)
            continue;

        // --- Case 1: this fd is a CGI output pipe
        // ------------------------------- The key exists in _pipe_to_client,
        // i.e. it maps this pipe's read-end back to the client that launched
        // the CGI. Data (or EOF) is ready from the script's stdout;
        // readCgiPipe() accumulates it and, on EOF, builds the HTTP response
        // and flips the client to POLLOUT.
        if (_pipe_to_client.count(fd)) {
            readCgiPipe(i, fd);

            // --- Case 2: this fd is a listening socket
            // ------------------------------ Not a client (count == 0) and
            // (from the else-if order) not a pipe either, so it must be one of
            // our bound listeners. A POLLIN here means a brand-new connection
            // is waiting: accept() it and register the new client fd. NOTE (CP3
            // TODO): we don't yet know WHICH Listener this fd belongs to, so
            // the accepted client can't carry its candidate servers yet.
        } else if (_clients.count(fd) == 0) {
            if (_poll_fds[i].revents & POLLIN) {
                int client_fd = accept(fd, NULL, NULL);
                if (client_fd <
                    0) // accept failed (e.g. client vanished): ignore
                    continue;
                acceptNewClient(
                    client_fd); // create Client + add its fd to _poll_fds
            }

            // --- Case 3: this fd is an established client connection
            // -----------------
        } else {
            // POLLIN: the client sent bytes -> read/parse the request (may
            // dispatch to CGI or build a response, and switch the fd to POLLOUT
            // when ready).
            if (_poll_fds[i].revents & POLLIN)
                clientRead(i, fd);
            // POLLOUT: the socket is writable -> flush (part of) the response;
            // closeClient() once everything has been sent.
            else if (_poll_fds[i].revents & POLLOUT) {
                clientWrite(i, fd);
                // POLLHUP/POLLERR: peer hung up or the socket errored -> tear
                // it down.
            } else if (_poll_fds[i].revents & (POLLHUP | POLLERR)) {
                closeClient(i, fd);
            }
        }
    }
    while (waitpid(-1, NULL, WNOHANG) > 0)
        ;
}

// RUNS THE SERVER
// Binds every listener, registers them in the poll set, then loops forever
// servicing all fds across all servers.
void WebServ::run(void) {
    setupListeners(_listeners);

    for (size_t i = 0; i < _listeners.size(); i++) {
        pollfd pfd;
        pfd.fd = _listeners[i].getSocket().getSocketFd();
        pfd.events = POLLIN;
        pfd.revents = 0;
        _poll_fds.push_back(pfd);
    }

    while (_running) {
        int ret = poll(&_poll_fds[0], _poll_fds.size(), TIMEOUT);
        if (ret == -1 && _running)
            throw std::runtime_error("Poll failed miserably");
        loopPollFds();
    }
}
