#include "server-manager.hpp"

void ServerManager::add(int sock, const char *ipAddress, std::string port, std::string groupId)
{
    std::lock_guard<std::mutex> guard(serverMutex);
    servers[sock] = std::make_shared<Server>(sock, ipAddress, port, groupId);
}

std::shared_ptr<Server> ServerManager::getServer(int sock) const
{
    std::lock_guard<std::mutex> guard(serverMutex);
    auto it = servers.find(sock);
    if (it != servers.end())
    {
        return it->second;
    }

    auto it2 = unknownServers.find(sock);
    if (it2 != unknownServers.end())
    {
        return it2->second;
    }

    // Return a default Server object with meaningful default values
    return std::make_shared<Server>(sock, "N/A", "N/A", "N/A");
}

void ServerManager::addUnknown(int sock, const char *ipAddress, std::string port)
{
    std::lock_guard<std::mutex> guard(serverMutex);
    unknownServers[sock] = std::make_shared<Server>(sock, ipAddress, port);
    connectionTime[sock] = std::chrono::steady_clock::now();
}

int ServerManager::moveFromUnknown(int sock, std::string groupId)
{
    auto i = servers.find(sock);
    if (i != servers.end())
    {
        return 0;
    }

    auto it = unknownServers.find(sock);
    if (it != unknownServers.end())
    {
        std::shared_ptr<Server> server = it->second;
        server->name = groupId;
        servers[sock] = server;
        unknownServers.erase(it);
        return 0;
    }
    std::cout << "ADD FAILED FOR: " << std::to_string(sock) << std::endl;
    return -1;
}

void ServerManager::close(int sock)
{
    std::lock_guard<std::mutex> guard(serverMutex);
    servers.erase(sock);
    unknownServers.erase(sock);
    connectionTime.erase(sock);
}

void ServerManager::update(int sock, std::string port)
{
    std::lock_guard<std::mutex> guard(serverMutex);
    if (port != "")
    {
        servers[sock]->port = port;
    }
}

std::string ServerManager::getName(int sock) const
{
    std::lock_guard<std::mutex> guard(serverMutex);
    auto it = servers.find(sock);
    if (it != servers.end() && it->second != nullptr)
    {
        std::string name = it->second->name;
        if (name.empty())
        {
            name = "Sock-" + std::to_string(it->first);
        }
        return name;
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
        message += std::to_string(pair.first) + ":" + pair.second->name + ", ";
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

bool ServerManager::hasConnectedToServer(std::string ipAddress, std::string port, std::string groupId) const
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

std::vector<int> ServerManager::getAllServerSocks() const
{
    std::vector<int> socks;
    socks.reserve(servers.size());
    for (const auto &pair : servers)
    {
        socks.push_back(pair.first);
    }
    return socks;
}

std::string ServerManager::getListOfUnknownServers() const
{
    std::lock_guard<std::mutex> guard(serverMutex);
    std::string message;

    if (unknownServers.size() == 0)
    {
        message = "Not connected to any unknown servers";
        return message;
    }

    for (const auto &pair : unknownServers)
    {
        message += std::to_string(pair.first) + ":" + pair.second->ipAddress + ", ";
    }
    return message.substr(0, message.length() - 2);
}

bool ServerManager::isConnectedToGroupId(std::string groupId, int fromSock) const
{
    if (groupId == MY_GROUP_ID)
    {
        return true;
    }

    std::lock_guard<std::mutex> guard(serverMutex);
    for (const auto &pair : servers)
    {
        if ((pair.second->name == groupId && pair.first != fromSock))
        {
            return true;
        }
    }
    return false;
}

std::vector<int> ServerManager::getListOfServersToRemove()
{
    std::lock_guard<std::mutex> guard(serverMutex);
    std::vector<int> toRemove;
    auto now = std::chrono::steady_clock::now();

    for (const auto &pair : unknownServers)
    {
        if (now - connectionTime[pair.first] > heloTimeout)
        {
            toRemove.push_back(pair.first);
        }
    }
    return toRemove;
}

bool ServerManager::isKnown(int sock) const
{
    std::lock_guard<std::mutex> guard(serverMutex);
    auto i = servers.find(sock);
    if (i != servers.end())
    {
        return true;
    }
    return false;
}