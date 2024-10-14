
#include "utils.hpp"
#include "servers.hpp"
#include "connection-manager.hpp"
#include "server-manager.hpp"
#include "server-commands.hpp"
#include "client-commands.hpp"
#include "logger.hpp"
#include "poll-manager.hpp"
#include "group-message-manager.hpp"

#include <thread>
#include <chrono>
#include <unordered_map>

#define MY_GROUP_ID "A5_55"

ServerManager serverManager;
Logger logger;
PollManager pollManager;
GroupMessageManager groupMessageManager;
ConnectionManager connectionManager = ConnectionManager(serverManager, pollManager, logger);
ServerCommands serverCommands = ServerCommands(serverManager, groupMessageManager, connectionManager);
ClientCommands clientCommands = ClientCommands(serverManager, logger, groupMessageManager, connectionManager);
std::string serverIpAddress;

void handleCommand(int sock, std::vector<std::string>commands)
{
    for (auto command : commands)
    {
        if (sock == connectionManager.getOurClientSock())
        {
            return clientCommands.findCommand(command);
        }
        serverCommands.findCommand(sock, command);
    }
}

void sendStatusReqMessages()
{
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::minutes(5));
        std::string message = "STATUSREQ";
        std::vector<int> socks = serverManager.getAllServerSocks();

        for (auto sock : socks)
        {
            connectionManager.sendTo(sock, message);
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
            connectionManager.sendTo(pair.first, pair.second, true);
        }
    }
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Usage: chat_server <port>\n");
        exit(0);
    }

    char *serverPort = argv[1];
    char *GROUP_ID = argv[2];

    serverCommands.setPort(serverPort);

    int listenSock;

    // Setup socket for server to listen to
    listenSock = connectionManager.openSock(atoi(serverPort));

    logger.write("Listening on port: " + std::string(argv[1]) + " as group: " + MY_GROUP_ID, true);

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
            connectionManager.handleNewConnection(listenSock, GROUP_ID);
        }


        // Check for events on the remote-server sockets
        for (int i = 1; i < pollManager.nfds; i++)
        {
            if (!pollManager.hasData(i))
            {
                continue;
            }

            char buffer[MAX_MSG_LENGTH];
            int sock = pollManager.getFd(i);
            RecvStatus status = connectionManager.recvFrame(sock, buffer, MAX_MSG_LENGTH);

            if (status == ERROR || status == SERVER_DISCONNECTED)
            {
                connectionManager.closeSock(sock);
                continue;
            }

            std::vector<std::string> messages = extractMessages(buffer, strlen(buffer));

            if (status == MSG_RECEIVED)
            {
                handleCommand(sock, messages);
            }
        }
    }

    close(listenSock);
    return 0;
}
