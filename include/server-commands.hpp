#ifndef SERVER_COMMANDS_HPP
#define SERVER_COMMANDS_HPP

#include <iostream>
#include <vector>

#include "utils.hpp"
#include "server-manager.hpp"
class ServerCommands
{
public:
    void findCommand(int socket, std::string buffer);
    ServerCommands(ServerManager serverManager);

private:
    ServerManager serverManager;
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