#include "server-manager.hpp"

void ServerManager::add(int sock, const char *ipAddress, std::string port)
{
    std::lock_guard<std::mutex> guard(serverMutex);
    servers[sock] = std::make_shared<Server>(sock, ipAddress, port);
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

std::string ServerManager::getName(int sock) const
{
    std::lock_guard<std::mutex> guard(serverMutex);
    // TODO!!! Make sure this absolutely works, maybe figure out another way to handle the server sockets
    // This tries to find the server in the map, if it is not found it will return "N/A"
    auto it = servers.find(sock);
    if (it != servers.end() && it->second != nullptr)
    {
        return it->second->name;
    }

    return "N/A";
}

std::unordered_map<int, std::string> ServerManager::getConnectedSockets() const
{
    std::lock_guard<std::mutex> guard(serverMutex);
    std::unordered_map<int, std::string> sockAndGroupId;

    for (const auto &pair : servers)
    {
        int sock = pair.first;

        if (sock != 0)
        {
            sockAndGroupId[sock] = pair.second->name;
        }
    }

    return sockAndGroupId;
}

std::string ServerManager::getListOfServers() const
{
    std::lock_guard<std::mutex> guard(serverMutex);
    std::string message;

    if (servers.size() == 0)
    {
        message = "Not connected to any servers";
        return message;
    }

    for (const auto &pair : servers)
    {
        message += pair.second->name + ", ";
    }
    return message.substr(0, message.length() - 2);
}

int ServerManager::getSockByName(std::string name) const
{
    std::lock_guard<std::mutex> guard(serverMutex);
    for (const auto &pair : servers)
    {
        if (pair.second->name == name)
        {
            return pair.first;
        }
    }
    return -1;
}

bool ServerManager::hasConnectedToServer(std::string ipAddress, std::string port, std::string groupId)
{
    std::lock_guard<std::mutex> guard(serverMutex);
    for (const auto &pair : servers)
    {
        if (pair.second->ipAddress == ipAddress && (pair.second->name == groupId || pair.second->port == port))
        {
            return true;
        }
    }
    return false;
}

std::string ServerManager::getAllServersInfo() const
{
    std::lock_guard<std::mutex> guard(serverMutex);
    std::stringstream ss;

    for (const auto &pair : servers)
    {
        if (pair.second->name == "N/A")
        {
            continue;
        }
        ss << pair.second->name << "," << pair.second->ipAddress << "," << pair.second->port << ";";
    }
    return ss.str();
}