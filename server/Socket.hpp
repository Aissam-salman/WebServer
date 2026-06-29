#ifndef SOCKET_HPP
# define SOCKET_HPP

#include <string>
#include <sys/socket.h>
#include <netinet/in.h>

class Socket {
    private:
        std::string     _name;
        std::string     _host;
        int             _port;
        int             _listen_fd;
        sockaddr_in     _addr;

    public:
        Socket(void);
        Socket(std::string name);
        Socket(const Socket &src);
        Socket& operator= (const Socket &other);
        ~Socket();

        void setSocket(int port);

        void            setHost(std::string host);
        void            setPort(int port);
        std::string     getHost(void) const;
        int             getPort(void) const;

        int     getSocketFd(void);

        void    printSocket(void);
};

#endif
