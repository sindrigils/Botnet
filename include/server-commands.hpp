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
    void setPort(const std::string &port);
    void findCommand(int socket, std::string buffer);
    std::unordered_map<int, std::string> constructKeepAliveMessages();
    ServerCommands(ServerManager &serverManager, GroupMsgManager &groupMsgManager, ConnectionManager &connectionManager, Logger &logger);

private:
    ServerManager &serverManager;
    GroupMsgManager &groupMsgManager;
    ConnectionManager &connectionManager;
    Logger &logger;

    std::string myPort;

    void handleServers(int socket, std::string buffer);
    void handleHelo(int socket, std::vector<std::string> tokens);
    void handleKeepAlive(int socket, std::vector<std::string> tokens);
    void handleSendMsg(int socket, std::vector<std::string> tokens, std::string buffer);
    void handleGetMsgs(int socket, std::vector<std::string> tokens);
    void handleStatusReq(int socket, std::vector<std::string> tokens);
    void handleStatusResp(int socket, std::vector<std::string> tokens);
};

#endif
