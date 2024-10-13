#ifndef CONNECTION_MANAGER_HPP
#define CONNECTION_MANAGER_HPP

#include <sys/socket.h> // For socket functions (socket, connect, send, recv, etc.)
#include <netinet/in.h> // For sockaddr_in and address family (AF_INET)
#include <arpa/inet.h>  // For inet_pton, inet_ntop (to handle IP addresses)
#include <unistd.h>     // For close function

#include "logger.hpp"
#include "server-manager.hpp"
#include "poll-manager.hpp"
#include "utils.hpp"

class ConnectionManager
{
public:
    int connectToServer(const std::string &ip, std::string port, std::string myGroupId, bool isUnknown, std::string groupId = "");
    int sendTo(int sock, std::string message, bool isFormatted = false);
    void closeSock(int sock);
    ConnectionManager(ServerManager &serverManager, PollManager &pollManager, Logger &logger);

private:
    ServerManager &serverManager;
    PollManager &pollManager;
    Logger &logger;
};

#endif