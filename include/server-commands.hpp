#ifndef SERVER_COMMANDS_HPP
#define SERVER_COMMANDS_HPP

#include <iostream>
#include <vector>

#include "utils.hpp"
#include "server-manager.hpp"
#include "poll-manager.hpp"
#include "logger.hpp"

class ServerCommands
{
public:
    void setIpAddress(const char *ip);
    void setGroupId(const std::string &groupId);
    void setPort(const std::string &port);
    void findCommand(int socket, std::string buffer);
    ServerCommands(ServerManager &serverManager, PollManager &pollManager, Logger &logger);

private:
    ServerManager &serverManager;
    PollManager &pollManager;
    Logger &logger;

    const char *myIpAddress;
    std::string myGroupId;
    std::string myPort;

    void handleServers(int socket, std::string buffer);
    void handleHelo(int socket, std::vector<std::string> tokens);
    void handleKeepAlive(int socket, std::vector<std::string> tokens);
    void handleSendMsg(int socket, std::vector<std::string> tokens);
    void handleGetMsgs(int socket, std::vector<std::string> tokens);
    void handleStatusREQ(int socket, std::vector<std::string> tokens);
    void handleStatusRESP(int socket, std::vector<std::string> tokens);
};

#endif