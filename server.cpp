//
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
#include <vector>
#include <list>

#include <iostream>
#include <sstream>
#include <thread>
#include <map>

#include <unistd.h>

// fix SOCK_NONBLOCK for OSX
#ifndef SOCK_NONBLOCK
#include <fcntl.h>
#define SOCK_NONBLOCK O_NONBLOCK
#endif

#define BACKLOG 5         // Allowed length of queue of waiting connections
#define MAX_CONNECTIONS 8 // i think
#define GROUP_ID "5"
#define SOH 0x01
#define EOT 0x04
#define ESC 0x10 // Escape character for byte-stuffing

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

int clientSock = -1;
char *serverIpAddress;
char *serverPort;
class Server
{
public:
    int sock;
    std::string name;
    std::string ipAddress;
    std::string port;

    Server(int socket) : sock(socket)
    {
        name = "N/A";
    }
};

// Note: map is not necessarily the most efficient method to use here,
// especially for a server with large numbers of simulataneous connections,
// where performance is also expected to be an issue.
//
// Quite often a simple array can be used as a lookup table,
// (indexed on socket no.) sacrificing memory for speed.

std::map<int, Server *> servers; // Lookup table for per Client information

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

std::string constructServerMessage(const std::string &content)
{
    std::string stuffedContent;

    // Byte-stuffing: Escape SOH (0x01) and EOT (0x04) in the content
    // ekki buinn að testa þetta, tekið beint af chat
    for (char c : content)
    {
        if (c == SOH || c == EOT)
        {
            stuffedContent += ESC;
        }
        stuffedContent += c;
    }

    std::string finalMessage;
    finalMessage += SOH;
    finalMessage += stuffedContent;
    finalMessage += EOT;

    return finalMessage;
}

bool connectToServer(const std::string &ip, int port, const std::string &groupId)
{
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &serverAddr.sin_addr);

    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (connect(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == 0)
    {
        servers[serverSocket] = new Server(serverSocket);

        std::cout << "here is the ip address: " << ip << std::endl;
        std::cout << "here is the port: " << port << std::endl;
        std::cout << "here is the groupId: " << groupId << std::endl;

        servers[serverSocket]->ipAddress = ip;
        servers[serverSocket]->port = port;
        servers[serverSocket]->name = groupId;

        // abstract this shit
        pollfds[nfds].fd = serverSocket;
        pollfds[nfds].events = POLLIN;
        nfds++;

        std::cout << "Connected to server: " << ip << ":" << port << std::endl;
        std::string message = std::string("HELO,") + GROUP_ID;
        std::string heloMessage = constructServerMessage(message);
        send(serverSocket, heloMessage.c_str(), heloMessage.length(), 0);
        return true;
    }
    else
    {
        std::cout << "Failed to connect to server: " << ip << ":" << port << std::endl;
        return false;
    }
}

// Close a client's connection, remove it from the client list, and
// tidy up select sockets afterwards.

void closeClient(int clientSocket)
{
    printf("Client closed connection: %d\n", clientSocket);
    close(clientSocket);
    servers.erase(clientSocket); // Remove from the clients map
    nfds--;
}

void clientCommand(std::vector<std::string> tokens, char *buffer)
{
    if (tokens[0].compare("GETMSG") == 0 && tokens.size() == 2)
    {
    }
    else if (tokens[0].compare("SENDMSG") == 0 && tokens.size() == 3)
    {
        for (auto const &pair : servers)
        {
            if (pair.second->name.compare(tokens[1]) == 0)
            {
                std::string msg;
                for (auto i = tokens.begin() + 2; i != tokens.end(); i++)
                {
                    msg += *i + " ";
                }
                send(pair.second->sock, msg.c_str(), msg.length(), 0);
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

        for (auto const &pair : servers)
        {
            send(pair.second->sock, msg.c_str(), msg.length(), 0);
        }
    }
    else if (tokens[0].compare("LISTSERVERS") == 0)
    {
        std::string message = "";
        if (servers.size() == 0)
        {
            message = "Not connected to any servers.";
        }
        else
        {
            for (const auto &pair : servers)
            {
                message += pair.second->name + ", ";
            }
            message = message.substr(0, message.length() - 2);
        }

        send(clientSock, message.c_str(), message.length(), 0);
    }
    else if (tokens[0].compare("LEAVE") == 0)
    {
        close(clientSock);
        clientSock = -1;
    }
    else if (tokens[0].compare("CONNECT") == 0 && tokens.size() == 4)
    {
        std::string message = "";
        std::string ip = tokens[1];
        int port = std::stoi(tokens[2]);
        std::string groupId = tokens[3];
        bool isSuccess = connectToServer(tokens[1], port, groupId);
        message = isSuccess ? "Successfully connected to A5_" + groupId + " server." : "Unable to connect to A5_" + groupId + " server.";
        send(clientSock, message.c_str(), message.length(), 0);
    }
    else
    {
        std::cout << "Unknown command from client:" << buffer << std::endl;
    }
}

// Process command from client on the server
void handleCommand(int clientSocket, char *buffer)
{

    // need to abstract
    std::vector<std::string> tokens;
    std::stringstream ss(buffer);
    std::string token;

    // hugsa þetta betur
    while (std::getline(ss, token, ','))
    {
        tokens.push_back(token);
    }

    if (tokens[0].compare("kaladin") == 0 && clientSock == -1)
    {
        // þurfum aðeins að hugsa þetta, bara hægt að hafa einn client right now, en þurfum svo sem ekki fleiri
        servers.erase(clientSocket);
        clientSock = clientSocket;
        std::string message = "Well hello there!";
        send(clientSocket, message.c_str(), message.length(), 0);
        return;
    }
    else if (clientSocket == clientSock)
    {
        return clientCommand(tokens, buffer);
    }

    int bufferLength = strlen(buffer);
    // TODO do this a lot better
    if (buffer[0] == SOH && buffer[bufferLength - 1] == EOT)
    {
        std::string message(buffer + 1, bufferLength - 2); // Skip SOH and EOT

        // Now process the message as normal
        std::stringstream sohStream(message);
        tokens.clear(); // Clear the token vector for the new message

        while (std::getline(sohStream, token, ','))
        {
            tokens.push_back(token);
        }

        if (tokens[0].compare("HELO") == 0 && tokens.size() == 2)
        {
            // reply with SERVERS, which is all the servers that we are connected to
            servers[clientSocket]->name = tokens[1];

            std::string groupId = std::string(GROUP_ID);
            std::string message = "SERVERS,A5_" + groupId + "," + serverIpAddress + "," + serverPort + ";";
            for (auto &pair : servers)
            {
                if (pair.first == clientSocket)
                {
                    std::cout << "found the server" << std::endl;
                    continue;
                }

                message += "A5_" + pair.second->name + ",";
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

            std::stringstream serversStream(message);
            tokens.clear(); // Clear the token vector for the new message

            while (std::getline(serversStream, token, ';'))
            {
                tokens.push_back(token);
            }

            for (auto token : tokens)
            {
                std::vector<std::string> serverInfo;
                std::stringstream serverInfoStream(token);
                std::string info;

                while (std::getline(serverInfoStream, info, ','))
                {
                    tokens.push_back(info);
                }

                std::string ipAddress = tokens[1];
                std::string port = tokens[2];
                // abstracta þetta
                bool skip = false;
                for (auto &pair : servers)
                {
                    if (pair.second->ipAddress == ipAddress && pair.second->port == port)
                    {
                        skip = true;
                        break;
                    }
                }
                if (skip)
                    continue;

                connectToServer(ipAddress, std::stoi(port), tokens[3]);
            }

            std::cout << "here is buffer: " << buffer << std::endl;
        }
        else if (tokens[0].compare("KEEPALIVE") == 0 && tokens.size() == 2)
        {
        }
        else if (tokens[0].compare("GETMSGS") == 0 && tokens.size() == 2)
        {
        }
        else if (tokens[0].compare("SENDMSG") == 0 && tokens.size() == 4)
        {
        }
        else if (tokens[0].compare("STATUSREQ") == 0 && tokens.size() == 1)
        {
        }
        else if (tokens[0].compare("STATUSREQ") == 0 && tokens.size() >= 2)
        {
        }
        else
        {
            std::cout << "Unknown command from server:" << buffer << std::endl;
        }
    }
    else
    {
        std::cout << "Unknown command from server:" << buffer << std::endl;
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

    bool finished = false;
    int listenSock, clientSock;
    struct sockaddr_in client;
    socklen_t clientLen = sizeof(client);
    char buffer[1025]; // Buffer for reading from clients

    if (argc != 2)
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

    while (!finished)
    {
        // Call poll() to wait for events on sockets
        int pollCount = poll(pollfds, nfds, -1); // Wait indefinitely

        if (pollCount == -1)
        {
            perror("poll failed");
            finished = true;
        }
        else
        {
            // Check for events on the listening socket
            if (pollfds[0].revents & POLLIN)
            {
                // New connection on the listening socket
                clientSock = accept(listenSock, (struct sockaddr *)&client, &clientLen);
                printf("New client connected: %d\n", clientSock);

                // Add the new client to the clients map and pollfds array
                servers[clientSock] = new Server(clientSock);

                char clientIpAddress[INET_ADDRSTRLEN]; // Buffer to store the IP address
                inet_ntop(AF_INET, &(client.sin_addr), clientIpAddress, INET_ADDRSTRLEN);
                servers[clientSock]->ipAddress = clientIpAddress;

                pollfds[nfds].fd = clientSock;
                pollfds[nfds].events = POLLIN; // Monitor for incoming data
                nfds++;
            }

            // Check for events on existing connections (clients)
            for (int i = 1; i < nfds; i++)
            {
                if (pollfds[i].revents & POLLIN)
                {
                    // Incoming data on client socket
                    int clientSocket = pollfds[i].fd;
                    int bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
                    if (bytesRead <= 0)
                    {
                        // Client disconnected
                        printf("Client disconnected: %d\n", clientSocket);
                        closeClient(clientSocket);
                    }
                    else
                    {
                        // Process command from client
                        buffer[bytesRead] = '\0';
                        handleCommand(clientSocket, buffer);
                    }
                }
            }
        }
    }

    close(listenSock); // Close the listening socket when done
    return 0;
}
