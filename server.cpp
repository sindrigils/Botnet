// Simple chat server for TSAM-409
//
// Command line: ./chat_server 4000
//
// Author: Jacky Mallett (jacky@ru.is)
//
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
#include <algorithm>
#include <map>
#include <list>

#include <sstream>
#include <thread>
#include <map>
#include <fstream>
#include <unistd.h>

#include "utils.hpp"

#include "servers.hpp"
#include "server-manager.hpp"

// fix SOCK_NONBLOCK for OSX
#ifndef SOCK_NONBLOCK
#include <fcntl.h>
#define SOCK_NONBLOCK O_NONBLOCK
#endif

#define BACKLOG 5         // Allowed length of queue of waiting connections
#define MAX_CONNECTIONS 8 // i think
// #define GROUP_ID "5"

ServerManager serverManager;

char *GROUP_ID;

// Create an array of pollfd structs to track the file descriptors and their events
struct pollfd pollfds[MAX_CONNECTIONS];
int nfds = 0; // Number of active file descriptors

// Function to initialize the listening socket and add it to pollfds
void setupPollFd(int listenSock)
{
    pollfds[0].fd = listenSock; // Listening socket is the first entry
    pollfds[0].events = POLLIN; // We want to monitor for incoming connections
    nfds = 1;                   // Only 1 fd initially (the listening socket)
}

int ourClientSock = -1;
char *serverIpAddress;
char *serverPort;

// Note: map is not necessarily the most efficient method to use here,
// especially for a server with large numbers of simulataneous connections,
// where performance is also expected to be an issue.
//
// Quite often a simple array can be used as a lookup table,
// (indexed on socket no.) sacrificing memory for speed.

// std::map<int, Server *> servers; // Lookup table for per Client information

// Open socket for specified port.
//
// Returns -1 if unable to create the socket for any reason.

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

bool connectToServer(const std::string &ip, int port)
{
    std::string strPort = std::to_string(port);

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &serverAddr.sin_addr);

    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (connect(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == 0)
    {
        // servers[serverSocket] = new Server(serverSocket, ip, strPort);
        serverManager.add(serverSocket, ip.c_str(), strPort);

        // abstract this shit
        pollfds[nfds].fd = serverSocket;
        pollfds[nfds].events = POLLIN;
        nfds++;

        std::cout << "Connected to server: " << ip << ":" << strPort << std::endl;
        std::string message = "HELO," + std::string(GROUP_ID);
        std::string heloMessage = constructServerMessage(message);
        send(serverSocket, heloMessage.c_str(), heloMessage.length(), 0);
        return true;
    }
    else
    {
        std::cout << "Failed to connect to server: " << ip << ":" << strPort << std::endl;
        return false;
    }
}

void closeClient(int clientSocket)
{
    printf("Client closed connection: %d\n", clientSocket);
    close(clientSocket);

    // servers.erase(clientSocket); // Remove from the clients map
    serverManager.close(clientSocket);
    nfds--;
}

void clientCommand(std::vector<std::string> tokens, const char *buffer)
{

    if (tokens[0].compare("GETMSG") == 0 && tokens.size() == 2)
    {
        std::string message = "NOT IMPLEMENTED";
        send(ourClientSock, message.c_str(), message.length(), 0);
    }
    else if (tokens[0].compare("GETMSG") == 0 && tokens.size() == 3)
    {
        std::string forGroupId = tokens[1];
        std::string fromGroupId = tokens[2];
        std::string msg = "GETMSGS," + forGroupId;
        std::string message = constructServerMessage(msg);

        for (auto &pair : serverManager.servers)
        {
            if (pair.second->name == fromGroupId)
            {
                std::cout << "SENDING GETMSGS" << std::endl;
                send(pair.second->sock, message.c_str(), message.length(), 0);
            }
        }
    }
    // ONLY FOR TESTINGS ON INSTR_1
    else if (tokens[0].compare("HELO") == 0)
    {
        std::string message = "HELO," + std::string(GROUP_ID);
        std::string heloMessage = constructServerMessage(message);
        for (auto &pair : serverManager.servers)
        {
            if (pair.second->name == "Instr_1")
            {
                std::cout << "MANUALLY SENDING HELO" << std::endl;
                send(pair.first, heloMessage.c_str(), heloMessage.length(), 0);
            }
        }
    }
    else if (tokens[0].compare("SENDMSG") == 0 && tokens.size() == 3)
    {
        std::string groupId = tokens[1];
        for (auto const &pair : serverManager.servers)
        {
            if (pair.second->name.compare(groupId) == 0)
            {
                std::string content;
                for (auto i = tokens.begin() + 2; i != tokens.end(); i++)
                {
                    content += *i + " ";
                }
                std::string message = "SENDMSG," + groupId + "," + GROUP_ID + "," + content;
                std::string serverMessage = constructServerMessage(message);
                send(pair.second->sock, serverMessage.c_str(), serverMessage.length(), 0);
            }
        }
    }
    else if (tokens[0].compare("MSG") == 0 && tokens[1].compare("ALL") == 0)
    {
        std::string msg;
        for (auto i = tokens.begin() + 2; i != tokens.end(); i++)
        {
            msg += *i + " ";
        }

        for (auto const &pair : serverManager.servers)
        {
            send(pair.second->sock, msg.c_str(), msg.length(), 0);
        }
    }
    else if (tokens[0].compare("LISTSERVERS") == 0)
    {
        std::string message = "";
        if (serverManager.servers.size() == 0)
        {
            message = "Not connected to any servers.";
        }
        else
        {
            for (const auto &pair : serverManager.servers)
            {
                message += pair.second->name + ", ";
            }
            message = message.substr(0, message.length() - 2);
        }

        send(ourClientSock, message.c_str(), message.length(), 0);
    }
    else if (tokens[0].compare("LEAVE") == 0)
    {
        close(ourClientSock);
        ourClientSock = -1;
    }
    else if (tokens[0].compare("CONNECT") == 0 && tokens.size() == 3)
    {
        std::string message = "";
        std::string ip = tokens[1];
        int port = std::stoi(tokens[2]);
        bool isSuccess = connectToServer(trim(tokens[1]), port);
        message = isSuccess ? "Successfully connected to the server." : "Unable to connect to the server.";
        send(ourClientSock, message.c_str(), message.length(), 0);
    }
    else if (tokens[0].compare("STATUSREQ") == 0 && tokens.size() == 2)
    {
        std::string groupId = tokens[1];
        std::string message = constructServerMessage("STATUSREQ");

        for (const auto &pair : serverManager.servers)
        {
            if (pair.second->name == groupId)
            {
                std::cout << "found server sending STATUSREQ" << std::endl;
                send(pair.second->sock, message.c_str(), message.length(), 0);
                break;
            }
        }
    }
    else
    {
        std::cout << "Unknown command from client:" << buffer << std::endl;
    }
}

void processServerMessage(int clientSocket, std::string buffer)
{
    std::vector<std::string> tokens = splitMessageOnDelimiter(buffer.c_str());

    if (tokens[0].compare("HELO") == 0 && tokens.size() == 2)
    {
        // reply with SERVERS, which is all the servers that we are connected to
        serverManager.update(clientSocket, "", tokens[1]);
        std::cout << "HERE IS NAME AFTER UPDATE: " << serverManager.servers[clientSocket]->name << std::endl;
        std::string groupId = std::string(GROUP_ID);
        std::string message = "SERVERS," + groupId + "," + serverIpAddress + "," + serverPort + ";";
        for (auto &pair : serverManager.servers)
        {
            if (pair.second->name == "N/A")
                continue;
            message += pair.second->name + ",";
            message += pair.second->ipAddress + ",";
            message += pair.second->port + ";";
        }
        std::string sendMessage = constructServerMessage(message);
        send(clientSocket, sendMessage.c_str(), sendMessage.length(), 0);
    }
    else if (tokens[0].compare("SERVERS") == 0 && tokens.size() >= 2)
    {
        // now we just got all the servers that the server we just connect to, is connect to, proceed to
        // connect to the ones that we are not connected to
        tokens.clear();
        tokens = splitMessageOnDelimiter(buffer.substr(8).c_str(), ';');
        for (auto token : tokens)
        {
            std::vector<std::string> serverInfo = splitMessageOnDelimiter(token.c_str());
            std::string groupId = trim(serverInfo[0]);
            std::string ipAddress = trim(serverInfo[1]);
            std::string port = trim(serverInfo[2]);

            // TODO better, since the first token is just the server info of the server that just send the message
            if (token == tokens[0])
            {
                serverManager.update(clientSocket, port);
                // servers[clientSocket]->port = port;
                continue;
            }

            // abstracta þetta
            bool skip = false;
            if (groupId == GROUP_ID || port == "-1")
            {
                skip = true;
            }
            else
            {
                // TODO FIX
                for (auto &pair : serverManager.servers)
                {
                    if (pair.second->ipAddress == ipAddress && (pair.second->name == groupId || pair.second->port == port))
                    {
                        skip = true;
                        break;
                    }
                }
            }
            if (skip)
                continue;

            // hægt er að crasha serverinn ef port er ekki tölur
            std::cout << "Attempting to connect to server: " << ipAddress << "," << port << "," << groupId;
            connectToServer(ipAddress, std::stoi(port));
        }
    }
    else if (tokens[0].compare("KEEPALIVE") == 0 && tokens.size() == 2)
    {
    }
    else if (tokens[0].compare("GETMSGS") == 0 && tokens.size() == 2)
    {
        // std::string groupMessage = getMessageById(trim(tokens[1]));
        // std::string message = "" send(clientSock, message.c_str(), message.length(), 0);
    }
    else if (tokens[0].compare("SENDMSG") == 0 && tokens.size() >= 4)
    {
        std::string toGroupId = tokens[1];
        std::string fromGroupId = tokens[2];

        std::string content;
        for (auto i = tokens.begin() + 3; i != tokens.end(); i++)
        {
            content += *i + " ";
        }
        std::string message = "Message from " + fromGroupId + ": " + content;
        send(ourClientSock, message.c_str(), message.length(), 0);
    }
    else if (tokens[0].compare("STATUSREQ") == 0 && tokens.size() == 1)
    {
    }
    else if (tokens[0].compare("STATUSRESP") == 0 && tokens.size() >= 1)
    {
        std::cout << "STATUSRESP: " << buffer << std::endl;
        // for (auto i = tokens.begin(); i != tokens.end(); i++)
        // {
        //     std::cout << *i << std::endl;
        // }
    }
    else
    {
        std::cout << "Unknown command from server:" << buffer << std::endl;
    }
}

// Process command from client on the server
void handleCommand(int clientSocket, const char *buffer)
{
    int bufferLength = strlen(buffer);
    std::vector<std::string> messageVector;

    // Add messages to the message vector that are between <SOH> and <EOT>
    for (int i = 0, start, end; i < bufferLength; i = end + 1)
    {
        if ((start = findByteIndexInBuffer(buffer, bufferLength, i, SOH)) < 0)
        {
            break;
        }
        if ((end = findByteIndexInBuffer(buffer, bufferLength, start + 1, EOT)) < 0)
        {
            break;
        }
        messageVector.push_back(extractMessage(buffer, start + 1, end));
    }

    // need to abstract
    for (auto message : messageVector)
    {
        std::vector<std::string> tokens = splitMessageOnDelimiter(message.c_str());

        if (tokens[0].compare("kaladin") == 0 && ourClientSock == -1)
        {
            // þurfum aðeins að hugsa þetta, bara hægt að hafa einn client right now, en þurfum svo sem ekki fleiri
            // SERVER CHANGE
            // servers.erase(clientSocket);
            serverManager.close(clientSocket);

            ourClientSock = clientSocket;
            std::string message = "Well hello there!";
            send(clientSocket, message.c_str(), message.length(), 0);

            struct sockaddr_in own_addr;
            socklen_t own_addr_len = sizeof(own_addr);
            if (getsockname(clientSocket, (struct sockaddr *)&own_addr, &own_addr_len) < 0)
            {
                perror("can't get own IP address from socket");
                exit(1);
            }
            else
            {
                char own_ip[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &own_addr.sin_addr, own_ip, INET_ADDRSTRLEN);
                std::cout << "Here is the server's IP address: " << own_ip << std::endl;
            }

            return;
        }
        else if (clientSocket == ourClientSock)
        {
            return clientCommand(tokens, buffer);
        }

        processServerMessage(clientSocket, message);
    }
}

int main(int argc, char *argv[])
{
    char hostname[1024];
    hostname[1023] = '\0'; // Ensure null-termination
    gethostname(hostname, 1023);

    // Get the local IP address
    struct hostent *host_entry;
    host_entry = gethostbyname(hostname);
    if (host_entry == NULL)
    {
        perror("gethostbyname");
        exit(1);
    }

    // Convert the IP address to a human-readable form
    serverIpAddress = inet_ntoa(*(struct in_addr *)host_entry->h_addr_list[0]);

    serverPort = argv[1];
    GROUP_ID = argv[2];

    int listenSock, clientSock;
    struct sockaddr_in client;
    socklen_t clientLen = sizeof(client);
    char buffer[1025]; // Buffer for reading from clients

    if (argc != 3)
    {
        printf("Usage: chat_server <port>\n");
        exit(0);
    }

    // Setup socket for server to listen to
    listenSock = open_socket(atoi(argv[1]));
    printf("Listening on port: %d\n", atoi(argv[1]));

    if (listen(listenSock, BACKLOG) < 0)
    {
        printf("Listen failed on port %s\n", argv[1]);
        exit(0);
    }

    // Initialize pollfds with the listening socket
    setupPollFd(listenSock);

    // Main server loop
    while (true)
    {
        // Call poll() to wait for events on sockets
        int pollCount = poll(pollfds, nfds, -1); // Wait indefinitely

        if (pollCount == -1)
        {
            perror("poll failed");
            close(listenSock);
            break; // EXIT POINT
        }

        // Check for events on the listening socket
        if (pollfds[0].revents & POLLIN)
        {
            // New connection on the listening socket
            clientSock = accept(listenSock, (struct sockaddr *)&client, &clientLen);
            printf("New client connected: %d\n", clientSock);

            char clientIpAddress[INET_ADDRSTRLEN]; // Buffer to store the IP address
            inet_ntop(AF_INET, &(client.sin_addr), clientIpAddress, INET_ADDRSTRLEN);
            serverManager.add(clientSock, clientIpAddress);

            // abstract
            pollfds[nfds].fd = clientSock;
            pollfds[nfds].events = POLLIN; // Monitor for incoming data
            nfds++;

            // abstract
            std::string message = "HELO," + std::string(GROUP_ID);
            std::string heloMessage = constructServerMessage(message);
            send(clientSock, heloMessage.c_str(), heloMessage.length(), 0);
        }

        // Check for events on existing connections (clients)
        for (int i = 1; i < nfds; i++)
        {
            int offset = 0;
            int bytesRead = 0;

            if (pollfds[i].revents & POLLIN)
            {
                // Incoming data on client socket
                int clientSocket = pollfds[i].fd;

                memset(buffer, 0, sizeof(buffer));
                while (true)
                {
                    bytesRead = recv(clientSocket, buffer + offset, sizeof(buffer) - offset, 0);
                    if (bytesRead <= 0)
                    {
                        // Client disconnected
                        printf("Client disconnected: %d\n", clientSocket);
                        closeClient(clientSocket);
                        break;
                    }

                    if (buffer[offset + bytesRead - 1] == EOT)
                    {
                        // Process command from client
                        buffer[offset + bytesRead] = '\0';
                        handleCommand(clientSocket, buffer);
                        break;
                    }
                    offset += bytesRead;
                }
            }
        }
    }

    close(listenSock); // Close the listening socket when done
    return 0;
}
