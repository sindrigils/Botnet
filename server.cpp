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

#define MAX_EOT_TRIES 5

// fix SOCK_NONBLOCK for OSX
#ifndef SOCK_NONBLOCK
#include <fcntl.h>
#define SOCK_NONBLOCK O_NONBLOCK
#endif

#define Backlog 5
#define MAX_MESSAGE_LENGTH 5001

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

// Function to send KEEPALIVE messages
void sendKeepAliveMessages(ServerManager &serverManager)
{
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::minutes(1)); // Wait for 2 minutes

        // Get the connected sockets
        std::unordered_map<int, std::string> keepAliveMessages = serverCommands.constructKeepAliveMessages();
        for (auto &pair : keepAliveMessages)
        {
            logger.write("SENDINGG KEEPALIVE TO " + std::to_string(pair.first) + ": here is msg " + pair.second);
            send(pair.first, pair.second.c_str(), pair.second.length(), 0);
        }
    }
}

int main(int argc, char *argv[])
{
    char *serverPort = argv[1];
    char *GROUP_ID = argv[2];

    // remove both lines
    serverCommands.setGroupId(GROUP_ID);
    clientCommands.setGroupId(GROUP_ID);
    serverCommands.setPort(serverPort);

    int listenSock, clientSock;
    struct sockaddr_in client;
    socklen_t clientLen = sizeof(client);
    char buffer[MAX_MESSAGE_LENGTH]; // Buffer for reading from clients

    if (argc != 3)
    {
        printf("Usage: chat_server <port>\n");
        exit(0);
    }

    // Setup socket for server to listen to
    listenSock = open_socket(atoi(argv[1]));

    printf("Listening on port: %d\n", atoi(argv[1]));

    if (listen(listenSock, Backlog) < 0)
    {
        printf("Listen failed on port %s\n", argv[1]);
        exit(0);
    }

    pollManager.add(listenSock);
    std::thread keepAliveThread(sendKeepAliveMessages, std::ref(serverManager));

    // Main server loop
    while (true)
    {

        int pollCount = pollManager.getPollCount();

        if (pollCount == -1)
        {
            perror("poll failed");
            close(listenSock);
            break;
        }

        // Check for events on the listening socket
        if (pollManager.hasData(0)) // listen sock should always be 0
        {
            // New connection on the listening socket
            clientSock = accept(listenSock, (struct sockaddr *)&client, &clientLen);

            char clientIpAddress[INET_ADDRSTRLEN]; // Buffer to store the IP address
            inet_ntop(AF_INET, &(client.sin_addr), clientIpAddress, INET_ADDRSTRLEN);

            serverManager.add(clientSock, clientIpAddress);
            serverManager.update(clientSock, serverPort);
            pollManager.add(clientSock);

            logger.write("New client connected: " + std::string(clientIpAddress), true);
            std::string serverMessage = constructServerMessage("HELO," + std::string(GROUP_ID));
            send(clientSock, serverMessage.c_str(), serverMessage.length(), 0);
        }

        // Check for events on existing connections (clients)
        for (int i = 1; i < pollManager.nfds; i++)
        {
            if (!pollManager.hasData(i))
            {
                continue;
            }

            int clientSocket = pollManager.getFd(i);
            std::string serverName = serverManager.getName(clientSocket);

            // Read the data into a buffer, it's expected to start with SOH and and with EOT
            // however, it may be split into multiple packets, so we need to handle that.
            // We also need to handle the cases where the client does not wrap the message in SOH and EOT
            for (int i = 0, offset = 0, bytesRead = 0; i < MAX_EOT_TRIES; i++, offset += bytesRead)
            {
                bytesRead = recv(clientSocket, buffer + offset, sizeof(buffer) - offset, 0);
                logger.write("Received from " + serverName, buffer + offset, bytesRead);

                if (offset + bytesRead > MAX_MESSAGE_LENGTH)
                {
                    logger.write("Dropped message from " + serverName + ": message exceeds " + std::to_string(MAX_MESSAGE_LENGTH) + " bytes.", true);
                    closeClient(clientSocket);
                    break;
                }

                if (buffer[0] != SOH && offset == 0)
                {
                    // Client did not wrap the message in SOH and EOT
                    logger.write("Invalid message format from (First packet did not start with SOH) " + serverName, buffer, bytesRead, true);
                    closeClient(clientSocket);
                    break;
                }

                if (bytesRead <= 0)
                {
                    // Client disconnected
                    logger.write("Remote server disconnected: " + serverName, true);
                    closeClient(clientSocket);
                    break;
                }

                if (buffer[offset + bytesRead - 1] == EOT)
                {
                    buffer[offset + bytesRead] = '\0';
                    handleCommand(clientSocket, buffer);
                    break;
                }

                if (i == MAX_EOT_TRIES - 1)
                {
                    // Client did not wrap the message in SOH and EOT
                    logger.write("Invalid message format (No SOH, EOT) from " + serverName, buffer, bytesRead + offset, true);
                    closeClient(clientSocket);
                    break;
                }
            }
        }
    }

    close(listenSock);
    return 0;
}
