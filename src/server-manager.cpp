#include "server-manager.hpp"

bool ServerManager::add(int sock, const char *ipAddress, std::string port)
{
    std::lock_guard<std::mutex> guard(serverMutex1);
    servers[sock] = new Server(sock, ipAddress, port);
    return true;
}

bool ServerManager::close(int sock)
{
    std::lock_guard<std::mutex> guard(serverMutex1);
    servers.erase(sock);
    return true;
}

bool ServerManager::update(int sock, std::string port, std::string name)
{
    std::lock_guard<std::mutex> guard(serverMutex1);
    if (port != "")
    {
        servers[sock]->port = port;
    }
    if (name != "")
    {
        servers[sock]->name = name;
    }
    return true;
}