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

std::string ConnectionManager::getOurIpAddress() const
{
    return this->ourIpAddress;
}

int ConnectionManager::connectToServer(const std::string &ip, std::string strPort, std::string myGroupId, bool isUnknown, std::string groupId)
{

    bool isAlreadyConnected = serverManager.hasConnectedToServer(ip, strPort, "");
    if (isAlreadyConnected)
    {
        logger.write("Attempting to connect to an already connected server - ip: " + ip + ", port: " + strPort);
        return -1;
    }
    logger.write("Attempting to connect to ip: " + ip + ", port: " + strPort + ", groupId: " + groupId);
    int port = stringToInt(strPort);
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &serverAddr.sin_addr);
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (connect(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == 0)
    {
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

        std::string message = "HELO," + myGroupId;
        sendTo(serverSocket, message);
        return serverSocket;
    }
    logger.write("Unable to connect to server - groupId: " + groupId + ", ipAddress:  " + ip + ", port: " + strPort);
    return -1;
}

void ConnectionManager::handleNewConnection(int &listenSock, char *GROUP_ID)
{
    int clientSock;
    struct sockaddr_in client;
    socklen_t clientLen = sizeof(client);

    // New connection on the listening socket
    clientSock = accept(listenSock, (struct sockaddr *)&client, &clientLen);

    char clientIpAddress[INET_ADDRSTRLEN]; // Buffer to store the IP address
    inet_ntop(AF_INET, &(client.sin_addr), clientIpAddress, INET_ADDRSTRLEN);

    if (this->ourClientSock == -1)
    {
        this->ourClientSock = clientSock;
        this->ourIpAddress = getOwnIPFromSocket(clientSock);
        logger.write("Our client connected: " + std::string(ourIpAddress) + ", sock: " + std::to_string(ourClientSock), true);
        this->sendTo(clientSock, "Well hello there!");
        return;
    }

    serverManager.addUnknown(clientSock, clientIpAddress);
    pollManager.add(clientSock);

    logger.write("New server connected: " + std::string(clientIpAddress) + ", sock: " + std::to_string(clientSock), true);
    std::string message = "HELO," + std::string(GROUP_ID);
    this->sendTo(clientSock, message);
    
}

int ConnectionManager::sendTo(int sock, std::string message, bool isFormatted)
{
    std::string groupId = serverManager.getName(sock);
    logger.write("Sending to " + groupId + ": " + message);
    std::string serverMessage = isFormatted ? message : constructServerMessage(message);
    return send(sock, serverMessage.c_str(), serverMessage.length(), 0);
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
        std::string groupId = serverManager.getName(sock);
        bytesRead = recv(sock, buffer + offset, bufferLength - offset, 0);

        if (bytesRead == -1)
        { 
            logger.write("ERROR: Failed to read from " + groupId + ", received -1 from recv. Error: " + strerror(errno), true);
            return ERROR;
        }
        if (bytesRead == 0)
        {
            logger.write("Remote server disconnected: " + groupId, true);
            return SERVER_DISCONNECTED;
        }

        logger.write("Received RAW from " + groupId, buffer + offset, bytesRead);

        if (offset + bytesRead >= MAX_MSG_LENGTH)
        {
            logger.write("Message from " + groupId + ": message exceeds " + std::to_string(MAX_MSG_LENGTH) + " bytes.", true);
            return MSG_TOO_LONG;
        }

        if (buffer[0] != SOH && offset == 0)
        {
            logger.write("Message from " + groupId + ": message does not start with SOH", true);
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
            logger.write("Message from " + groupId + ": message does not end with EOT", true);
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
    logger.write("Closing connection to " + groupId);
    close(sock);
    serverManager.close(sock);
    pollManager.close(sock);
}