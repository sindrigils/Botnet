#ifndef CLIENT_COMMANDS_HPP
#define CLIENT_COMMANDS_HPP

#include "utils.hpp"
#include "server-manager.hpp"
#include "poll-manager.hpp"
#include "logger.hpp"
#include "group-message-manager.hpp"
#include "connection-manager.hpp"
#include "constants.hpp"
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

    void handleDropConnection(std::vector<std::string> tokens);
    void handleAddToBlacklist(std::vector<std::string> tokens);
    void handleShortConnect(std::vector<std::string> tokens);
    void handleSendMsg(std::vector<std::string> tokens);
    void handleConnect(std::vector<std::string> tokens);
    void handleGetMsg(std::vector<std::string> tokens);
    void handleViewBlacklist();
    void handleListServers();
    void handleViewMsgs();
    void handleHelp();
};

#endif