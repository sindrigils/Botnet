#include "server-manager.hpp"

std::mutex serverMutex;

bool ServerManager::add(int sock, const char *ipAddress, std::string port)
{
    std::lock_guard<std::mutex> guard(serverMutex);
    servers[sock] = new Server(sock, ipAddress, port);
    return true;
}

bool ServerManager::close(int sock)
{
    std::lock_guard<std::mutex> guard(serverMutex);
    servers.erase(sock);
    return true;
}

bool ServerManager::update(int sock, std::string port, std::string name)
{
    std::lock_guard<std::mutex> guard(serverMutex);
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