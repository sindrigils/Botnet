#ifndef SERVER_COMMANDS_HPP
#define SERVER_COMMANDS_HPP

#include <iostream>
#include <vector>

#include "utils.hpp"
#include "server-manager.hpp"
#include "poll-manager.hpp"
#include "logger.hpp"
#include "group-message-manager.hpp"

class ServerCommands
{
public:
    void setIpAddress(const char *ip);
    void setGroupId(const std::string &groupId);
    void setPort(const std::string &port);
    void setOurClient(int sock);
    void findCommand(int socket, std::string buffer);
    std::unordered_map<int, std::string> constructKeepAliveMessages();
    ServerCommands(ServerManager &serverManager, PollManager &pollManager, Logger &logger, GroupMessageManager &groupMessageManager);

private:
    GroupMessageManager &groupMessageManager;
    ServerManager &serverManager;
    PollManager &pollManager;
    Logger &logger;

    const char *myIpAddress;
    std::string myGroupId;
    std::string myPort;
    int ourClient;

    void handleServers(int socket, std::string buffer);
    void handleHelo(int socket, std::vector<std::string> tokens);
    void handleKeepAlive(int socket, std::vector<std::string> tokens);
    void handleSendMsg(int socket, std::vector<std::string> tokens);
    void handleGetMsgs(int socket, std::vector<std::string> tokens);
    void handleStatusREQ(int socket, std::vector<std::string> tokens);
    void handleStatusRESP(int socket, std::vector<std::string> tokens);
};

#endif