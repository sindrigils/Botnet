#include <poll.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <map>

#include <thread>
#include <chrono>

#include "utils.hpp"
#include "servers.hpp"
#include "server-manager.hpp"
#include "server-commands.hpp"
#include "client-commands.hpp"
#include "logger.hpp"
#include "poll-manager.hpp"
#include "group-message-manager.hpp"


// fix SOCK_NONBLOCK for OSX
#ifndef SOCK_NONBLOCK
#include <fcntl.h>
#define SOCK_NONBLOCK O_NONBLOCK
#endif

#define MAX_EOT_TRIES 5
#define Backlog 5
#define MAX_MESSAGE_LENGTH 3 * 5000

// #define GROUP_ID "5"

ServerManager serverManager;
Logger logger;
PollManager pollManager;
GroupMessageManager groupMessageManager;
ServerCommands serverCommands = ServerCommands(serverManager, pollManager, logger, groupMessageManager);
ClientCommands clientCommands = ClientCommands(serverManager, pollManager, logger, groupMessageManager);
std::string serverIpAddress;

int ourClientSock = -1;

int open_socket(int portno)
{
    struct sockaddr_in sk_addr; // address settings for bind()
    int sock;                   // socket opened for this port
    int set = 1;                // for setsockopt

    // Create socket for connection. Set to be non-blocking, so recv will
    // return immediately if there isn't anything waiting to be read.
#ifdef __APPLE__
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Failed to open socket");
        return (-1);
    }
#else
    if ((sock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0)) < 0)
    {
        perror("Failed to open socket");
        return (-1);
    }
#endif

    // Turn on SO_REUSEADDR to allow socket to be quickly reused after
    // program exit.

    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &set, sizeof(set)) < 0)
    {
        perror("Failed to set SO_REUSEADDR:");
    }
    set = 1;
#ifdef __APPLE__
    if (setsockopt(sock, SOL_SOCKET, SOCK_NONBLOCK, &set, sizeof(set)) < 0)
    {
        perror("Failed to set SOCK_NOBBLOCK");
    }
#endif
    memset(&sk_addr, 0, sizeof(sk_addr));

    sk_addr.sin_family = AF_INET;
    sk_addr.sin_addr.s_addr = INADDR_ANY;
    sk_addr.sin_port = htons(portno);

    // Bind to socket to listen for connections from clients
    if (bind(sock, (struct sockaddr *)&sk_addr, sizeof(sk_addr)) < 0)
    {
        perror("Failed to bind to socket:");
        return (-1);
    }
    else
    {
        return (sock);
    }
}

void closeClient(int clientSocket)
{
    close(clientSocket);
    serverManager.close(clientSocket);
    pollManager.close(clientSocket);
}

// Process command from client on the server
void handleCommand(int clientSocket, const char *buffer)
{
    int bufferLength = strlen(buffer);
    std::vector<std::string> messageVector = extractMessages(buffer, bufferLength);

    for (auto message : messageVector)
    {
        std::vector<std::string> tokens = splitMessageOnDelimiter(message.c_str());

        if (tokens[0].compare("kaladin") == 0 && ourClientSock == -1)
        {
            serverIpAddress = getOwnIPFromSocket(clientSocket);
            serverCommands.setIpAddress(serverIpAddress.c_str());
            serverCommands.setOurClient(clientSocket);
            serverManager.close(clientSocket);

            ourClientSock = clientSocket;
            clientCommands.setSock(clientSocket);
            std::string message = "Well hello there!";
            send(clientSocket, message.c_str(), message.length(), 0);
            return;
        }

        if (clientSocket == ourClientSock)
        {
            return clientCommands.findCommand(tokens, buffer);
        }
        serverCommands.findCommand(clientSocket, message);
    }
}

void sendStatusReqMessages()
{
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::minutes(5));
        std::string message = constructServerMessage("STATUSREQ");
        std::vector<int> socks = serverManager.getAllServerSocks();

        for (auto sock : socks)
        {
            send(sock, message.c_str(), message.length(), 0);
        }
    }
}

void sendKeepAliveMessages()
{
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::minutes(2));

        std::unordered_map<int, std::string> keepAliveMessages = serverCommands.constructKeepAliveMessages();
        for (const auto &pair : keepAliveMessages)
        {
            send(pair.first, pair.second.c_str(), pair.second.length(), 0);
        }
    }
}

void handleNewConnection(int &listenSock, char *GROUP_ID)
{
    int clientSock;
    struct sockaddr_in client;
    socklen_t clientLen = sizeof(client);

    // New connection on the listening socket
    clientSock = accept(listenSock, (struct sockaddr *)&client, &clientLen);

    char clientIpAddress[INET_ADDRSTRLEN]; // Buffer to store the IP address
    inet_ntop(AF_INET, &(client.sin_addr), clientIpAddress, INET_ADDRSTRLEN);

    serverManager.add(clientSock, clientIpAddress);
    pollManager.add(clientSock);

    logger.write("New client connected: " + std::string(clientIpAddress), true);
    std::string serverMessage = constructServerMessage("HELO," + std::string(GROUP_ID));
    send(clientSock, serverMessage.c_str(), serverMessage.length(), 0);
    
}

// returns -1 = error, 0 = client disconnected, 1 = message received, 2 = message dropped
// TODO: Logger should not write here, this should only return recv status code.
int recvAndParseMsg(char *buffer, int bufferLength, int &clientSocket, std::string &serverName){
    // Read the data into a buffer, it's expected to start with SOH and and with EOT
    // however, it may be split into multiple packets, so we need to handle that.
    for (int i = 0, offset = 0, bytesRead = 0; i < MAX_EOT_TRIES; i++, offset += bytesRead)
    {
        bytesRead = recv(clientSocket, buffer + offset, bufferLength - offset, 0);

        if (bytesRead == -1)
        {
            logger.write("ERROR: Failed to read from " + serverName + ", received -1 from recv. Error: " + strerror(errno), true);
            return -1;
        }

        logger.write("Received from " + serverName, buffer + offset, bytesRead);

        if (offset + bytesRead >= MAX_MESSAGE_LENGTH)
        {
            logger.write("Message from " + serverName + ": message exceeds " + std::to_string(MAX_MESSAGE_LENGTH) + " bytes.", true);
            return 2;
        }

        if (buffer[0] != SOH && offset == 0)
        {
            logger.write("Invalid message format (First packet did not start with SOH) from " + serverName, buffer, bytesRead, true);
            return -1;
        }

        if (bytesRead == 0)
        {
            logger.write("Remote server disconnected: " + serverName, true);
            return 0;
        }

        if (buffer[offset + bytesRead - 1] == EOT && buffer[offset + bytesRead - 2] != ESC)
        {
            buffer[offset + bytesRead] = '\0';
            return 1;
        }

        if (i == MAX_EOT_TRIES - 1)
        {
            // Tried to read the message MAX_EOT_TRIES times, but still no EOT
            logger.write("Invalid message format (No SOH, EOT) from " + serverName, buffer, bytesRead + offset, true);
            return -1;
        }
    }

    logger.write("recvAndParseMsg: Should not reach here", true);
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Usage: chat_server <port> <group_no>\n");
        exit(0);
    }

    char *serverPort = argv[1];
    char *GROUP_ID = argv[2];

    // remove both lines
    serverCommands.setGroupId(GROUP_ID);
    clientCommands.setGroupId(GROUP_ID);
    serverCommands.setPort(serverPort);

    int listenSock;

    // Setup socket for server to listen to
    listenSock = open_socket(atoi(argv[1]));

    logger.write("Listening on port: " + std::string(argv[1]), true);

    if (listen(listenSock, Backlog) < 0)
    {
        logger.write("Listen failed on port " + std::string(argv[1]), true);
        exit(0);
    }

    pollManager.add(listenSock);

    std::thread statusReqThread(sendStatusReqMessages);
    std::thread keepAliveThread(sendKeepAliveMessages);

    // Main server loop
    while (true)
    {
        int pollCount = pollManager.getPollCount();

        if (pollCount == -1)
        {
            logger.write("Poll failed: " + std::string(strerror(errno)), true);
            close(listenSock);
            break;
        }

        // Check for events on the listening socket
        if (pollManager.hasData(0)) // listen sock should always be 0
        {
            handleNewConnection(listenSock, GROUP_ID);
        }

        // Check for events on the remote-server sockets
        for (int i = 1; i < pollManager.nfds; i++)
        {
            if (!pollManager.hasData(i))
            {
                continue;
            }

            char buffer[MAX_MESSAGE_LENGTH]; // Buffer for reading from clients
            int clientSocket = pollManager.getFd(i);
            std::string serverName = serverManager.getName(clientSocket);

            int recvResult = recvAndParseMsg(buffer, sizeof(buffer), clientSocket, serverName);
            if(recvResult == -1 || recvResult == 0)
            {
                closeClient(clientSocket);
                logger.write("Closing connection to " + serverName, true);
                continue;
            } 
        
            if(recvResult == 1)
            {
                handleCommand(clientSocket, buffer);
            }

        }
    }

    close(listenSock);
    return 0;
}
