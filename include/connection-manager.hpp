#ifndef CONNECTION_MANAGER_HPP
#define CONNECTION_MANAGER_HPP

#include <sys/socket.h> // For socket functions (socket, connect, send, recv, etc.)
#include <netinet/in.h> // For sockaddr_in and address family (AF_INET)
#include <arpa/inet.h>  // For inet_pton, inet_ntop (to handle IP addresses)
#include <unistd.h>     // For close function
#include <errno.h>      // For errno
#include <string.h>     // For strerror

// fix SOCK_NONBLOCK for OSX
#ifndef SOCK_NONBLOCK
#include <fcntl.h>
#define SOCK_NONBLOCK O_NONBLOCK
#endif

#include "logger.hpp"
#include "server-manager.hpp"
#include "poll-manager.hpp"
#include "utils.hpp"

#include <mutex>
#include <set>
#include <thread>


enum RecvStatus {
    ERROR = -1,
    SERVER_DISCONNECTED = 0,
    MSG_RECEIVED = 1,
    MSG_DROPPED = 2,
    MSG_TOO_LONG = 3,
    MSG_INVALID_SOH = 4,
    MSG_INVALID_EOT = 5,
};

// fix SOCK_NONBLOCK for OSX
#ifndef SOCK_NONBLOCK
#include <fcntl.h>
#define SOCK_NONBLOCK O_NONBLOCK
#endif

class ConnectionManager
{
public:
    void connectToServer(const std::string &ip, std::string strPort, bool isUnknown, std::string groupId = "");
    int sendTo(int sock, std::string message, bool isFormatted = false);

    // Attempts to receive a message encapsulated in SOH and EOT from a socket, can be split into multiple packets
    RecvStatus recvFrame(int sock, char *buffer, int bufferLength);

    // Responds to a new connection with a HELO and adds it to the server manager
    void handleNewConnection(int &listenSock);

    int openSock(int portno);
    void closeSock(int sock);

    int getOurClientSock() const;
    std::string getOurIpAddress() const;

    ConnectionManager(ServerManager &serverManager, PollManager &pollManager, Logger &logger);

private:
    int _connectToServer(const std::string &ip, std::string port, bool isUnknown, std::string groupId = "");
    int ourClientSock = -1;
    std::string ourIpAddress = "127.0.0.1"; // default is local, it is changed when we connect to a server

    std::set<std::string> ongoingConnections;
    mutable std::mutex connectionMutex;

    ServerManager &serverManager;
    PollManager &pollManager;
    Logger &logger;
};

#endif