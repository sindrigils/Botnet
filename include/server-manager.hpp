#ifndef SERVER_MANAGER_HPP
#define SERVER_MANAGER_HPP

#include <mutex>
#include <map>

#include "servers.hpp"

extern std::mutex serverMutex;

class ServerManager
{
public:
    std::map<int, Server *> servers;
    bool add(int sock, const char *ipAddress, std::string port = "-1");
    bool close(int sock);
    bool update(int sock, std::string port = "", std::string name = "");
    std::string getName(int sock);

private:
};

#endif