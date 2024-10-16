#include "connection-manager.hpp"

ConnectionManager::ConnectionManager(

    ServerManager &serverManager,
    PollManager &pollManager,
    Logger &logger) : serverManager(serverManager),
                      pollManager(pollManager),
                      logger(logger) {};

int ConnectionManager::getOurClientSock() const
{
    return this->ourClientSock;
}

std::string ConnectionManager::getOwnIPFromSocket(int sock)
{
    struct sockaddr_in own_addr;

    socklen_t own_addr_len = sizeof(own_addr);

    if (getsockname(sock, (struct sockaddr *)&own_addr, &own_addr_len) < 0)
    {
        perror("can't get own IP address from socket");
        exit(1);
    }

    char own_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &own_addr.sin_addr, own_ip, INET_ADDRSTRLEN);

    return std::string(own_ip);
}

// Creates a connection-key from the ip and port, then checks if the connection is already in progress
// before attempting to connect to the server
void ConnectionManager::_connectToServer(const std::string &ip, std::string strPort, bool isUnknown, std::string groupId)
{
    if (groupId == "A5_666")
    {
        return;
    }
    std::string connectionKey = ip + ":" + strPort;
    {
        std::lock_guard<std::mutex> lock(connectionMutex);
        if (ongoingConnections.find(connectionKey) != ongoingConnections.end())
        {
            return;
        }
        ongoingConnections.insert(connectionKey);
    }

    if (serverManager.hasConnectedToServer(ip, strPort, ""))
    {
        logger.write("Attempting to connect to an already connected server - ip: " + ip + ", port: " + strPort);
        std::lock_guard<std::mutex> lock(connectionMutex);
        ongoingConnections.erase(connectionKey);
        return;
    }

    logger.write("Attempting to connect to ip: " + ip + ", port: " + strPort + ", groupId: " + groupId);

    int port = stringToInt(strPort);
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &serverAddr.sin_addr);
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (connect(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        logger.write("Failed to connect to server - groupId: " + groupId + ", ipAddress: " + ip + ", port: " + strPort + ", sock: " + std::to_string(serverSocket) + ", error: " + strerror(errno));
        close(serverSocket);
        std::lock_guard<std::mutex> lock(connectionMutex);
        ongoingConnections.erase(connectionKey);
        return;
    }

    if (isUnknown)
    {
        
        serverManager.addUnknown(serverSocket, ip.c_str(), strPort);
    }
    else
    {
        serverManager.add(serverSocket, ip.c_str(), strPort, groupId);
    }

    logger.write("Server connected - groupId: " + groupId + ", ipAddress: " + ip + ", port: " + strPort + ", sock: " + std::to_string(serverSocket));

    pollManager.add(serverSocket);

    std::string message = "HELO," + std::string(MY_GROUP_ID);
    this->sendTo(serverSocket, message);
    std::lock_guard<std::mutex> lock(connectionMutex);
    ongoingConnections.erase(connectionKey);
}

void ConnectionManager::connectToServer(const std::string &ip, std::string strPort, bool isUnknown, std::string groupId)
{

    std::thread(&ConnectionManager::_connectToServer, this, ip, strPort, isUnknown, groupId).detach();
}

void ConnectionManager::handleNewConnection(int &listenSock)
{
    int clientSock;
    struct sockaddr_in client;
    socklen_t clientLen = sizeof(client);

    // New connection on the listening socket
    clientSock = accept(listenSock, (struct sockaddr *)&client, &clientLen);
    if (clientSock < 0)
    {
        logger.write("Failed to accept connection: " + std::string(strerror(errno)), true);
        return;
    }

    char clientIpAddress[INET_ADDRSTRLEN]; // Buffer to store the IP address
    inet_ntop(AF_INET, &(client.sin_addr), clientIpAddress, INET_ADDRSTRLEN);

    pollManager.add(clientSock);

    // if this is the first client to connect, we will treat it as our client
    if (this->ourClientSock == -1)
    {
        this->ourClientSock = clientSock;

        logger.write("Our client connected: " + this->getOwnIPFromSocket(clientSock) + ", sock: " + std::to_string(ourClientSock), true);
        this->sendTo(clientSock, "Well hello there!");
        return;
    }

    serverManager.addUnknown(clientSock, clientIpAddress);

    logger.write("New server connected: " + std::string(clientIpAddress) + ", sock: " + std::to_string(clientSock), true);
    std::string message = "HELO," + std::string(MY_GROUP_ID);
    this->sendTo(clientSock, message);
}

int ConnectionManager::sendTo(int sock, std::string message, bool isFormatted)
{
    auto server = serverManager.getServer(sock);

    logger.write("Sending to " + server->name + "(" + server->ipAddress + ":" + server->port + "): " + message);
    std::string serverMessage = isFormatted ? message : constructServerMessage(message);
    int bytesSent = send(sock, serverMessage.c_str(), serverMessage.length(), 0);
    if (bytesSent == -1)
    {
        logger.write("Failed to send to " + server->name + " (" + server->ipAddress + ":" + server->port + "): " + message + ". Error: " + strerror(errno), true);
        this->closeSock(sock);
    }
    return bytesSent;
}

RecvStatus ConnectionManager::recvFrame(int sock, char *buffer, int bufferLength)
{
    // Read the data into a buffer, it's expected to start with SOH and and with EOT
    // however, it may be split into multiple packets, so we need to handle that.
    // We will assume that the message starts with SOH and ends with EOT, however, there maybe
    // multiple "frames" in the message, we will return the entire buffer, and the caller will
    // needs to split the buffer into individual messages.
    for (int i = 0, offset = 0, bytesRead = 0; i < MAX_EOT_TRIES; i++, offset += bytesRead)
    {
        auto server = serverManager.getServer(sock);
        std::string serverNameIpPort = server->name + " (" + server->ipAddress + ":" + server->port + ")";

        bytesRead = recv(sock, buffer + offset, bufferLength - offset, 0);

        if (bytesRead == -1)
        {
            logger.write("ERROR: Failed to read from " + serverNameIpPort + ", sock: " + std::to_string(sock) + ". Error: " + strerror(errno), true);
            return ERROR;
        }
        if (bytesRead == 0)
        {
            logger.write("Remote server disconnected: " + serverNameIpPort, true);
            return SERVER_DISCONNECTED;
        }

        logger.write("Received RAW from " + serverNameIpPort, buffer + offset, bytesRead);

        if (offset + bytesRead >= MAX_MSG_LENGTH)
        {
            logger.write("Message from " + serverNameIpPort + ": message exceeds " + std::to_string(MAX_MSG_LENGTH) + " bytes.", true);
            return MSG_TOO_LONG;
        }

        if (buffer[0] != SOH && offset == 0)
        {
            logger.write("Message from " + serverNameIpPort + ": message does not start with SOH", true);
            return MSG_INVALID_SOH;
        }

        if (buffer[offset + bytesRead - 1] == EOT && buffer[offset + bytesRead - 2] != ESC)
        {
            buffer[offset + bytesRead] = '\0';
            return MSG_RECEIVED;
        }

        // Tried to read the message MAX_EOT_TRIES times, but still no EOT
        if (i == MAX_EOT_TRIES - 1)
        {
            logger.write("Message from " + serverNameIpPort + ": message does not end with EOT", true);
            return MSG_INVALID_EOT;
        }
    }

    logger.write("ConnectionManager.recvFrame: Should not reach here", true);
    return ERROR;
}

int ConnectionManager::openSock(int portno)
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
        logger.write("Failed to open socket: " + std::string(strerror(errno)));
        return (-1);
    }
#endif

    // Turn on SO_REUSEADDR to allow socket to be quickly reused after
    // program exit.

    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &set, sizeof(set)) < 0)
    {
        logger.write("Failed to set SO_REUSEADDR: " + std::string(strerror(errno)));
    }
    set = 1;
#ifdef __APPLE__
    if (setsockopt(sock, SOL_SOCKET, SOCK_NONBLOCK, &set, sizeof(set)) < 0)
    {
        logger.write("Failed to set SOCK_NOBBLOCK: " + std::string(strerror(errno)));
    }
#endif
    memset(&sk_addr, 0, sizeof(sk_addr));

    sk_addr.sin_family = AF_INET;
    sk_addr.sin_addr.s_addr = INADDR_ANY;
    sk_addr.sin_port = htons(portno);

    // Bind to socket to listen for connections from clients
    if (bind(sock, (struct sockaddr *)&sk_addr, sizeof(sk_addr)) < 0)
    {
        logger.write("Failed to bind to socket: " + std::string(strerror(errno)));
        return (-1);
    }
    else
    {
        return (sock);
    }
}

void ConnectionManager::closeSock(int sock)
{
    std::string groupId = serverManager.getName(sock);
    logger.write("Closing connection to " + groupId, true);
    close(sock);
    serverManager.close(sock);
    pollManager.close(sock);
}
