#ifndef SERVER_MANAGER_HPP
#define SERVER_MANAGER_HPP

#include <mutex>
#include <map>
#include <unordered_map>

#include "servers.hpp"

class ServerManager
{
public:
    void add(int sock, const char *ipAddress, std::string port = "-1");
    void close(int sock);
    void update(int sock, std::string port = "", std::string name = "");
    std::unordered_map<int, std::string> getConnectedSockets() const;
    std::string getName(int sock) const;
    std::string getListOfServers() const;
    int getSockByName(std::string name) const;
    bool hasConnectedToServer(std::string ipAddress, std::string port, std::string groupId);
    std::string getAllServersInfo() const;

private:
    std::map<int, Server *> servers;
    mutable std::mutex serverMutex;
};

#endif