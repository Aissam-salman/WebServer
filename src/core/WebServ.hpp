#ifndef WEBSERV_HPP
#define WEBSERV_HPP

#include "Client.hpp"
#include "Server.hpp"
#include <cstddef>
#include <map>
#include <stdexcept>
#include <string>
#include <sys/poll.h>
#include <vector>

// Owns every Server (virtual host), every Listener (shared host:port socket),
// and the single poll() event loop that services all of them. The
// per-connection state (clients, poll fds, CGI pipe mapping) lives here because
// it spans every server, not any single one.
class WebServ {
  private:
    std::vector<Server> _servers;
    std::vector<Listener> _listeners;
    std::vector<pollfd> _poll_fds;
    std::map<int, Client> _clients;
    std::map<int, int> _pipe_to_client;
    std::map<int, int> _pipe_to_client_write;
    // TODO(discuss): list of supported CGI interpreter languages. Seeded with
    // "python"/"php" but never read by Cgi — commented out pending a decision
    // on whether to finish the interpreter-gating feature or drop it.
    // std::vector<std::string> _languages_supported;

    WebServ(const WebServ &other);
    WebServ &operator=(const WebServ &other);

    // ==== EVENT LOOP (formerly Server::) ====
    void acceptNewClient(int client_fd, int listen_fd, const std::string &peer);
    void readCgiPipe(size_t &i, int fd);
    void writeCgiPipe(size_t &i, int fd);
    void clientRead(size_t &i, int fd);
    void clientWrite(size_t &i, int fd);
    void closeClient(size_t &i, int fd);
    void handleCgi(Client &client, int fd);
    void handleReq(Client &client, int i);
    void responseError(std::runtime_error &e, int i, Client &client);
    void loopPollFds(void);
    void switchFdsToPollout(int client_fd);

    Server &resolveServer(Client &client);

  public:
    WebServ(const char *config_path);
    ~WebServ();

    static bool _running;

    void run(void);
    void printConfig(void);
    static void handle_sigint(int);
};

#endif
