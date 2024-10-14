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

class ServerCommands
{
public:
    void setGroupId(const std::string &groupId);
    void setPort(const std::string &port);
    void findCommand(int socket, std::string buffer);
    std::unordered_map<int, std::string> constructKeepAliveMessages();
    ServerCommands(ServerManager &serverManager, GroupMessageManager &groupMessageManager, ConnectionManager &connectionManager);

private:
    ServerManager &serverManager;
    GroupMessageManager &groupMessageManager;
    ConnectionManager &connectionManager;

    std::string myGroupId;
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