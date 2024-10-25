#ifndef SERVER_MANAGER_HPP
#define SERVER_MANAGER_HPP

#include <mutex>
#include <sstream>
#include <unordered_map>
#include <memory>
#include <vector>
#include "servers.hpp"
#include "utils.hpp"

class ServerManager
{
public:
    bool hasConnectedToServer(std::string ipAddress, std::string port, std::string groupId) const;
    void add(int sock, const char *ipAddress, std::string port, std::string groupId);
    void addUnknown(int sock, const char *ipAddress, std::string port = "-1");
    bool isConnectedToGroupId(std::string groupId, int fromSock = -1) const;
    std::unordered_map<int, std::string> getConnectedSockets() const;
    // this functions moves the server sock from known to unknown, this happens when he has send a HELO
    int moveFromUnknown(int sock, std::string groupId);

    std::shared_ptr<Server> getServer(int sock) const;
    void update(int sock, std::string port = "");
    std::vector<int> getAllServerSocks() const;
    int getSockByName(std::string name) const;
    std::string getAllServersInfo() const;
    std::string getListOfServers() const;
    std::vector<int> getListOfServersToRemove();
    bool isKnown(int sock) const;
    void close(int sock);
    int getOurClientSock() const;
    void setOurClientSock(int sock);

private:
    // map of all the valid connected servers
    std::unordered_map<int, std::shared_ptr<Server>> servers;
    // map to keep all the sockets that have not sent an HELO (trying to give them an chance), and if they
    // dont send it soon we will drop them
    std::unordered_map<int, std::shared_ptr<Server>> unknownServers;

    // a map to track the how long ago the unknown servers connected, we use this
    // to keep track of them so we can drop them if we don't receive HELO from them
    std::unordered_map<int, std::chrono::time_point<std::chrono::steady_clock>> connectionTime;
    const std::chrono::seconds heloTimeout = std::chrono::seconds(5);

    int ourClientSock = -1;

    mutable std::mutex serverMutex;
};

#endif