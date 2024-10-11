#ifndef CLIENT_COMMANDS_HPP
#define CLIENT_COMMANDS_HPP

#include "utils.hpp"
#include "server-manager.hpp"
#include "poll-manager.hpp"
#include "logger.hpp"

class ClientCommands
{
public:
    int sock;
    // remove
    void setGroupId(const std::string &groupId);
    void setSock(int newSock);
    void findCommand(std::vector<std::string> tokens, const char *buffer);
    ClientCommands(ServerManager &serverManager, PollManager &pollManager, Logger &logger);

private:
    // remove
    std::string myGroupId;
    ServerManager &serverManager;
    PollManager &pollManager;
    Logger &logger;
};

#endif