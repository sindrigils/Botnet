#ifndef CONNECTION_MANAGER_HPP
#define CONNECTION_MANAGER_HPP

#include <sys/socket.h> // For socket functions (socket, connect, send, recv, etc.)
#include <netinet/in.h> // For sockaddr_in and address family (AF_INET)
#include <arpa/inet.h>  // For inet_pton, inet_ntop (to handle IP addresses)
#include <unistd.h>     // For close function
#include <errno.h>      // For errno
#include <string.h>     // For strerror

#include <tuple>
#include <mutex>
#include <set>
#include <thread>

// fix SOCK_NONBLOCK for OSX
#ifndef SOCK_NONBLOCK
#include <fcntl.h>
#define SOCK_NONBLOCK O_NONBLOCK
#endif

#include "logger.hpp"
#include "server-manager.hpp"
#include "poll-manager.hpp"
#include "utils.hpp"
#include "constants.hpp"

enum RecvStatus
{
    ERROR = -1,
    SERVER_DISCONNECTED = 0,
    MSG_RECEIVED = 1,
};

// fix SOCK_NONBLOCK for OSX
#ifndef SOCK_NONBLOCK
#include <fcntl.h>
#define SOCK_NONBLOCK O_NONBLOCK
#endif

class ConnectionManager
{
public:
    void connectToServer(const std::string &ip, std::string strPort, std::string groupId = "");
    std::set<std::tuple<std::string, std::string, std::string>> getBlacklistedServers() const;
    bool isBlacklisted(std::string groupId, std::string ip = "", std::string port = "");
    void addToBlacklist(std::string groupId, std::string ip, std::string port);
    int sendTo(int sock, std::string message, bool isFormatted = false);

    // Attempts to receive a message encapsulated in SOH and EOT from a socket, can be split into multiple packets
    RecvStatus recvFrame(int sock, char *buffer, int bufferLength);
    void handleNewConnection(int listenSock);

    std::string getOwnIPFromSocket(int sock);
    int openSock(int portno);
    void closeSock(int sock);
    ConnectionManager(ServerManager &serverManager, PollManager &pollManager, Logger &logger);

private:
    ServerManager &serverManager;
    PollManager &pollManager;
    Logger &logger;

    // a set of servers that are blacklisted
    std::set<std::tuple<std::string, std::string, std::string>> blacklist;
    // a set of ongoingConnections at this moment, so two threads don't try to connect to the same server at similar times
    std::set<std::string> ongoingConnections;

    mutable std::mutex connectionMutex;
    mutable std::mutex blacklistMutex;

    void _connectToServer(const std::string &ip, std::string port, std::string groupId = "");
};

#endif