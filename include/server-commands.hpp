#ifndef SERVER_COMMANDS_HPP
#define SERVER_COMMANDS_HPP

#include <iostream>
#include <vector>
#include <thread>
#include <mutex>

#include "utils.hpp"
#include "server-manager.hpp"
#include "poll-manager.hpp"
#include "logger.hpp"
#include "group-message-manager.hpp"
#include "connection-manager.hpp"
#include "logger.hpp"

class ServerCommands
{
public:
    std::unordered_map<int, std::string> constructKeepAliveMessages();
    void findCommand(int socket, std::string buffer);
    void setPort(const std::string &port);

    ServerCommands(ServerManager &serverManager, GroupMsgManager &groupMsgManager, ConnectionManager &connectionManager, Logger &logger);

private:
    ServerManager &serverManager;
    GroupMsgManager &groupMsgManager;
    ConnectionManager &connectionManager;
    Logger &logger;

    std::string myPort;

    void handleSendMsg(int socket, std::vector<std::string> tokens, std::string buffer);
    void handleStatusResp(int socket, std::vector<std::string> tokens);
    void handleStatusReq(int socket, std::vector<std::string> tokens);
    void handleKeepAlive(int socket, std::vector<std::string> tokens);
    void handleGetMsgs(int socket, std::vector<std::string> tokens);
    void handleHelo(int socket, std::vector<std::string> tokens);
    void handleServers(int socket, std::string buffer);
};

#endif
