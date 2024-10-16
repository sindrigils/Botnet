#ifndef CLIENT_COMMANDS_HPP
#define CLIENT_COMMANDS_HPP

#include "utils.hpp"
#include "server-manager.hpp"
#include "poll-manager.hpp"
#include "logger.hpp"
#include "group-message-manager.hpp"
#include "connection-manager.hpp"

class ClientCommands
{
public:
    void findCommand(std::string message);
    ClientCommands(ServerManager &serverManager, Logger &logger, GroupMsgManager &groupMsgManager, ConnectionManager &connectionManager);

private:
    ServerManager &serverManager;
    Logger &logger;
    GroupMsgManager &groupMsgManager;
    ConnectionManager &connectionManager;

    void handleGetMsg(std::vector<std::string> tokens);
    void handleGetMsgFrom(std::vector<std::string> tokens);
    void handleSendMsg(std::vector<std::string> tokens);
    void handleSendMsgToSock(std::vector<std::string> tokens);
    void handleListServers();
    void handleListUnknownServers();
    void handleConnect(std::vector<std::string> tokens);
    void handleStatusREQ(std::vector<std::string> tokens);
    void handleShortConnect(std::vector<std::string> tokens);
    void handleDropConnection(std::vector<std::string> tokens);
};

#endif