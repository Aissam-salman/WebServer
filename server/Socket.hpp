#ifndef SOCKET_HPP
# define SOCKET_HPP

#include <string>
#include <sys/socket.h>
#include <netinet/in.h>

class Socket {
    private:
        std::string     _name;
        int             _listen_fd;
        sockaddr_in     _addr;

    public:
        Socket(void);
        Socket(std::string name);
        Socket(const Socket &src);
        Socket& operator= (const Socket &other);
        ~Socket();

        void setSocket(int port);
        int     getSocketFd(void);

        void    printSocket(void);
};

#endif
