#include "connection-manager.hpp"

ConnectionManager::ConnectionManager(

    ServerManager &serverManager,
    PollManager &pollManager,
    Logger &logger) : serverManager(serverManager),
                      pollManager(pollManager),
                      logger(logger) {};

int ConnectionManager::connectToServer(const std::string &ip, std::string strPort, std::string myGroupId)
{
    int port = stringToInt(strPort);
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &serverAddr.sin_addr);
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (connect(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == 0)
    {
        std::string message = "HELO," + myGroupId;
        std::string heloMessage = constructServerMessage(message);
        send(serverSocket, heloMessage.c_str(), heloMessage.length(), 0);
        return serverSocket;
    }
    return -1;
}

int ConnectionManager::sendTo(int sock, std::string message, bool isFormatted)
{
    std::string groupId = serverManager.getName(sock);
    logger.write("Sending to " + groupId + ": " + message);
    std::string serverMessage = isFormatted ? message : constructServerMessage(message);
    return send(sock, serverMessage.c_str(), serverMessage.length(), 0);
}

void ConnectionManager::closeSock(int sock)
{
    std::string groupId = serverManager.getName(sock);
    logger.write("Closing connection to " + groupId);
    close(sock);
    serverManager.close(sock);
    pollManager.close(sock);
}