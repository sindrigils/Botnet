#ifndef SERVER_MANAGER_HPP
#define SERVER_MANAGER_HPP

#include <mutex>
#include <sstream>
#include <unordered_map>
#include <memory>

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
    bool hasConnectedToServer(std::string ipAddress, std::string port, std::string groupId) const;
    std::string getAllServersInfo() const;
    std::vector<int> getAllServerSocks() const;

private:
    std::unordered_map<int, std::shared_ptr<Server>> servers;

    mutable std::mutex serverMutex;
};

#endif