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
    void add(int sock, const char *ipAddress, std::string port, std::string groupId);
    void addUnknown(int sock, const char *ipAddress, std::string port = "-1");
    // this functions moves the server sock from known to unknown, this happens when he has send a HELO
    int moveFromUnknown(int sock, std::string groupId);
    void close(int sock);
    void update(int sock, std::string port = "");
    std::unordered_map<int, std::string> getConnectedSockets() const;
    std::string getName(int sock) const;
    std::string getListOfServers() const;
    std::string getListOfUnknownServers() const;
    std::shared_ptr<Server> getServer(int sock) const;

    int getSockByName(std::string name) const;
    bool hasConnectedToServer(std::string ipAddress, std::string port, std::string groupId) const;
    std::string getAllServersInfo() const;
    std::vector<int> getAllServerSocks() const;
    bool isConnectedToGroupId(std::string groupId, int fromSock) const;

private:
    // map of all the valid connected servers
    std::unordered_map<int, std::shared_ptr<Server>> servers;
    // map to keep all the sockets that have not sent an HELO (trying to give them an chance), and if they
    // dont send it soon we will drop them
    std::unordered_map<int, std::shared_ptr<Server>> unknownServers;
    mutable std::mutex serverMutex;
};

#endif