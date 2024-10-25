#include "connection-manager.hpp"

ConnectionManager::ConnectionManager(

    ServerManager &serverManager,
    PollManager &pollManager,
    Logger &logger) : serverManager(serverManager),
                      pollManager(pollManager),
                      logger(logger) {};

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
void ConnectionManager::_connectToServer(const std::string &ip, std::string strPort, std::string groupId)
{
    int port = stringToInt(strPort);
    // some SERVERS are advertising the ephemeral port of other servers, we are trying to filter them out
    if (port > 6000 || port == -1)
    {
        logger.write("[INFO] Not going to connect because of invalid port: ip: " + ip + ", port: " + strPort + ", groupId: " + groupId);
        return;
    }

    if (this->isBlacklisted(groupId, ip, strPort) || !validateGroupId(groupId, true))
    {
        return;
    }

    std::string connectionKey = ip + ":" + strPort;
    {
        std::lock_guard<std::mutex> lock(this->connectionMutex);
        if (ongoingConnections.find(connectionKey) != ongoingConnections.end())
        {
            return;
        }
        ongoingConnections.insert(connectionKey);
    }

    if (serverManager.hasConnectedToServer(ip, strPort, ""))
    {
        logger.write("[INFO] Attempting to connect to an already connected server - ip: " + ip + ", port: " + strPort);
        std::lock_guard<std::mutex> lock(connectionMutex);
        ongoingConnections.erase(connectionKey);
        return;
    }

    logger.write("[INFO] Attempting to connect to ip: " + ip + ", port: " + strPort + ", groupId: " + groupId);
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &serverAddr.sin_addr);
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (connect(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        logger.write("[ERROR] Failed to connect to server - groupId: " + groupId + ", ipAddress: " + ip + ", port: " + strPort + ", sock: " + std::to_string(serverSocket) + ", error: " + strerror(errno));
        close(serverSocket);
        std::lock_guard<std::mutex> lock(connectionMutex);
        ongoingConnections.erase(connectionKey);
        return;
    }

    serverManager.addUnknown(serverSocket, ip.c_str(), strPort);
    logger.write("[INFO] Server connected - groupId: " + groupId + ", ipAddress: " + ip + ", port: " + strPort + ", sock: " + std::to_string(serverSocket));
    pollManager.add(serverSocket);

    std::string message = "HELO," + std::string(MY_GROUP_ID);
    this->sendTo(serverSocket, message);
    std::lock_guard<std::mutex> lock(connectionMutex);
    ongoingConnections.erase(connectionKey);
}

void ConnectionManager::connectToServer(const std::string &ip, std::string strPort, std::string groupId)
{

    std::thread(&ConnectionManager::_connectToServer, this, ip, strPort, groupId).detach();
}

void ConnectionManager::handleNewConnection(int listenSock)
{
    int clientSock;
    struct sockaddr_in client;
    socklen_t clientLen = sizeof(client);

    // New connection on the listening socket
    clientSock = accept(listenSock, (struct sockaddr *)&client, &clientLen);
    if (clientSock < 0)
    {
        logger.write("[ERROR] Failed to accept connection: " + std::string(strerror(errno)), true);
        return;
    }

    char clientIpAddress[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(client.sin_addr), clientIpAddress, INET_ADDRSTRLEN);

    pollManager.add(clientSock);
    serverManager.addUnknown(clientSock, clientIpAddress);

    logger.write("[INFO] New server connected: " + std::string(clientIpAddress) + ", sock: " + std::to_string(clientSock), true);
    std::string message = "HELO," + std::string(MY_GROUP_ID);
    this->sendTo(clientSock, message);
}

int ConnectionManager::sendTo(int sock, std::string message, bool isFormatted)
{
    auto server = serverManager.getServer(sock);

    logger.write("[SENDING] " + server->name + "(" + server->port + "): " + message);
    std::string serverMessage = isFormatted ? message : constructServerMessage(message);
    int bytesSent = send(sock, serverMessage.c_str(), serverMessage.length(), 0);
    if (bytesSent == -1)
    {
        logger.write("[ERROR] Failed to send to " + server->name + " (" + server->ipAddress + ":" + server->port + "): " + message + ". Error: " + strerror(errno));
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
        std::string serverNamePort = server->name + " (" + server->port + ")";

        bytesRead = recv(sock, buffer + offset, bufferLength - offset, 0);

        if (bytesRead == -1)
        {
            logger.write("[FAILURE] Failed to read from " + serverNamePort + ", sock: " + std::to_string(sock) + ". Error: " + strerror(errno));
            return ERROR;
        }
        if (bytesRead == 0)
        {
            logger.write("[INFO] Remote server disconnected: " + serverNamePort, true);
            return SERVER_DISCONNECTED;
        }

        logger.write("[RECEIVED] " + serverNamePort, buffer + offset, bytesRead);

        if (offset + bytesRead >= MAX_MSG_LENGTH)
        {
            logger.write("[FAILURE] Message from " + serverNamePort + ": message exceeds " + std::to_string(MAX_MSG_LENGTH) + " bytes.");
            return MSG_TOO_LONG;
        }

        if (buffer[0] != SOH && offset == 0)
        {
            logger.write("[FAILURE] Message from " + serverNamePort + ": message does not start with SOH");
            return ERROR;
        }

        if (buffer[offset + bytesRead - 1] == EOT && buffer[offset + bytesRead - 2] != ESC)
        {
            buffer[offset + bytesRead] = '\0';
            return MSG_RECEIVED;
        }

        // Tried to read the message MAX_EOT_TRIES times, but still no EOT
        if (i == MAX_EOT_TRIES - 1)
        {
            logger.write("[FAILURE] Message from " + serverNamePort + ": message does not end with EOT");
            return ERROR;
        }
    }

    logger.write("[ERROR] ConnectionManager.recvFrame: Should not reach here", true);
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
        logger.write("[ERROR] Failed to open socket: " + std::string(strerror(errno)));
        return (-1);
    }
#else
    if ((sock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0)) < 0)
    {
        logger.write("[ERROR] Failed to open socket: " + std::string(strerror(errno)));
        return (-1);
    }
#endif

    // Turn on SO_REUSEADDR to allow socket to be quickly reused after
    // program exit.

    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &set, sizeof(set)) < 0)
    {
        logger.write("[ERROR] Failed to set SO_REUSEADDR: " + std::string(strerror(errno)));
    }
    set = 1;
#ifdef __APPLE__
    if (setsockopt(sock, SOL_SOCKET, SOCK_NONBLOCK, &set, sizeof(set)) < 0)
    {
        logger.write("[ERROR] Failed to set SOCK_NOBBLOCK: " + std::string(strerror(errno)));
    }
#endif
    memset(&sk_addr, 0, sizeof(sk_addr));

    sk_addr.sin_family = AF_INET;
    sk_addr.sin_addr.s_addr = INADDR_ANY;
    sk_addr.sin_port = htons(portno);

    // Bind to socket to listen for connections from clients
    if (bind(sock, (struct sockaddr *)&sk_addr, sizeof(sk_addr)) < 0)
    {
        logger.write("[ERROR] Failed to bind to socket: " + std::string(strerror(errno)));
        return (-1);
    }
    else
    {
        return (sock);
    }
}

void ConnectionManager::closeSock(int sock)
{

    // Reset our client sock if our client disconnected
    if(sock == serverManager.getOurClientSock()){
        serverManager.setOurClientSock(-1);
        logger.write("[INFO] Our client disconnected", true);
    }

    auto server = serverManager.getServer(sock);
    logger.write("[INFO] Closing connection to " + server->name + ", sock: " + std::to_string(sock), true);
    close(sock);
    serverManager.close(sock);
    pollManager.close(sock);
}

void ConnectionManager::addToBlacklist(std::string groupId, std::string ip, std::string port)
{
    std::lock_guard<std::mutex> lock(blacklistMutex);
    std::tuple<std::string, std::string, std::string> server(groupId, ip, port);
    blacklist.insert(server);
}

bool ConnectionManager::isBlacklisted(std::string groupId, std::string ip, std::string port)
{
    std::lock_guard<std::mutex> lock(blacklistMutex);

    for (const auto &server : blacklist)
    {

        if (std::get<0>(server) == groupId || (std::get<1>(server) == ip && std::get<2>(server) == port))
        {
            return true;
        }
    }

    return false;
}

std::set<std::tuple<std::string, std::string, std::string>> ConnectionManager::getBlacklistedServers() const
{
    std::lock_guard<std::mutex> lock(blacklistMutex);
    return this->blacklist;
}