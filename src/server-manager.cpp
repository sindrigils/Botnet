#include "server-manager.hpp"

void ServerManager::add(int sock, const char *ipAddress, std::string port)
{
    std::lock_guard<std::mutex> guard(serverMutex);
    servers[sock] = new Server(sock, ipAddress, port);
}

void ServerManager::close(int sock)
{
    std::lock_guard<std::mutex> guard(serverMutex);
    servers.erase(sock);
}

void ServerManager::update(int sock, std::string port, std::string name)
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
}

std::string ServerManager::getName(int sock)
{
    std::lock_guard<std::mutex> guard(serverMutex);
    // TODO!!! Make sure this absolutely works, maybe figure out another way to handle the server sockets
    // This tries to find the server in the map, if it is not found it will return "N/A"
    auto it = servers.find(sock);
    if (it != servers.end() && it->second != nullptr)
    {
        return it->second->name;
    }
    else
    {
        return "N/A";
    }
}