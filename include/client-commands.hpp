#ifndef CLIENT_COMMANDS_HPP
#define CLIENT_COMMANDS_HPP

#include "utils.hpp"
#include "server-manager.hpp"
#include "poll-manager.hpp"
#include "logger.hpp"
#include "group-message-manager.hpp"

class ClientCommands
{
public:
    int sock;
    // remove
    void setGroupId(const std::string &groupId);
    void setSock(int newSock);
    void findCommand(std::vector<std::string> tokens, const char *buffer);
    ClientCommands(ServerManager &serverManager, PollManager &pollManager, Logger &logger, GroupMessageManager &groupMessageManager);

private:
    // remove
    std::string myGroupId;
    ServerManager &serverManager;
    PollManager &pollManager;
    Logger &logger;
    GroupMessageManager &groupMessageManager;

    void handleGetMsg(std::vector<std::string> tokens);
    void handleGetMsgFrom(std::vector<std::string> tokens);
    void handleSendMsg(std::vector<std::string> tokens);
    void handleMsgAll(std::vector<std::string> tokens);
    void handleListServers(std::vector<std::string> tokens);
    void handleConnect(std::vector<std::string> tokens);
    void handleStatusREQ(std::vector<std::string> tokens);
    void handleShortConnect(std::vector<std::string> tokens);
};

#endif