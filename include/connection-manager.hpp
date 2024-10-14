#ifndef CONNECTION_MANAGER_HPP
#define CONNECTION_MANAGER_HPP

#include <sys/socket.h> // For socket functions (socket, connect, send, recv, etc.)
#include <netinet/in.h> // For sockaddr_in and address family (AF_INET)
#include <arpa/inet.h>  // For inet_pton, inet_ntop (to handle IP addresses)
#include <unistd.h>     // For close function
#include <errno.h>      // For errno
#include <string.h>     // For strerror

#include "logger.hpp"
#include "server-manager.hpp"
#include "poll-manager.hpp"
#include "utils.hpp"

class ConnectionManager
{
public:
    int connectToServer(const std::string &ip, std::string port, std::string myGroupId, bool isUnknown, std::string groupId = "");
    int sendTo(int sock, std::string message, bool isFormatted = false);

    // Attempts to receive a message encapsulated in SOH and EOT from a socket, can be split into multiple packets
    RecvStatus recvFrame(int sock, char *buffer, int bufferLength);

    // Responds to a new connection with a HELO and adds it to the server manager
    void handleNewConnection(int &listenSock, char *GROUP_ID);

    int openSock(int portno);
    void closeSock(int sock);
    
    int getOurClientSock() const;
    std::string getOurIpAddress() const;

    ConnectionManager(ServerManager &serverManager, PollManager &pollManager, Logger &logger);

private:
    int ourClientSock = -1;
    std::string ourIpAddress = "123.123.123.123"; // default so its hard to spot if not set

    ServerManager &serverManager;
    PollManager &pollManager;
    Logger &logger;
};

#endif