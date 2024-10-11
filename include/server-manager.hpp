#ifndef SERVER_MANAGER_HPP
#define SERVER_MANAGER_HPP

#include <mutex>
#include <map>

#include "servers.hpp"

class ServerManager
{
public:
    std::map<int, Server *> servers;
    void add(int sock, const char *ipAddress, std::string port = "-1");
    void close(int sock);
    void update(int sock, std::string port = "", std::string name = "");
    std::string getName(int sock);

private:
    std::mutex serverMutex;
};

#endif