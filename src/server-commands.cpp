#include "server-commands.hpp"

ServerCommands::ServerCommands(
    ServerManager &serverManager,
    PollManager &pollManager,
    Logger &logger) : serverManager(serverManager),
                      pollManager(pollManager),
                      logger(logger), myIpAddress("-1"), myGroupId("-1"), myPort("-1") {};

void ServerCommands::setIpAddress(const char *ip)
{
    myIpAddress = ip; // Set the IP address
}

// Setter for Group ID
void ServerCommands::setGroupId(const std::string &groupId)
{
    myGroupId = groupId; // Set the group ID
}

// Setter for Port
void ServerCommands::setPort(const std::string &port)
{
    myPort = port; // Set the port
}

void ServerCommands::findCommand(int socket, std::string buffer)
{
    std::vector<std::string> tokens = splitMessageOnDelimiter(buffer.c_str());
    if (tokens[0].compare("HELO") == 0 && tokens.size() == 2)
    {
        handleHelo(socket, tokens);
    }
    else if (tokens[0].compare("SERVERS") == 0 && tokens.size() >= 2)
    {
        handleServers(socket, buffer);
    }

    else if (tokens[0].compare("KEEPALIVE") == 0 && tokens.size() == 2)
    {
        handleKeepAlive(socket, tokens);
    }
    else if (tokens[0].compare("GETMSGS") == 0 && tokens.size() == 2)
    {
        handleGetMsgs(socket, tokens);
    }
    else if (tokens[0].compare("SENDMSG") == 0 && tokens.size() >= 4)
    {
        handleSendMsg(socket, tokens);
    }
    else if (tokens[0].compare("STATUSREQ") == 0 && tokens.size() == 1)
    {
        handleStatusREQ(socket, tokens);
    }
    else if (tokens[0].compare("STATUSRESP") == 0 && tokens.size() >= 1)
    {
        handleStatusRESP(socket, tokens);
    }
    else
    {
        std::cout << "Unknown command from server:" << buffer << std::endl;
    }
}

void ServerCommands::handleHelo(int socket, std::vector<std::string> tokens)
{
    serverManager.update(socket, "", tokens[1]);

    std::string message = "SERVERS," + myGroupId + "," + myIpAddress + "," + myPort + ";";
    for (auto &pair : serverManager.servers)
    {
        if (pair.second->name == "N/A")
        {
            continue;
        }

        message += pair.second->name + ",";
        message += pair.second->ipAddress + ",";
        message += pair.second->port + ";";
    }
    std::string sendMessage = constructServerMessage(message);
    send(socket, sendMessage.c_str(), sendMessage.length(), 0);
}
void ServerCommands::handleServers(int socket, std::string buffer)
{

    std::vector<std::string> tokens = splitMessageOnDelimiter(buffer.substr(8).c_str(), ';');
    for (auto token : tokens)
    {
        std::vector<std::string> serverInfo = splitMessageOnDelimiter(token.c_str());
        std::string groupId = trim(serverInfo[0]);
        std::string ipAddress = trim(serverInfo[1]);
        std::string port = trim(serverInfo[2]);

        // TODO better, since the first token is just the server info of the server that just send the message
        if (token == tokens[0])
        {
            serverManager.update(socket, port);
            continue;
        }

        // abstracta þetta
        bool skip = false;
        if (groupId == myGroupId || port == "-1")
        {
            skip = true;
        }
        else
        {
            // TODO FIX
            for (auto &pair : serverManager.servers)
            {
                if (pair.second->ipAddress == ipAddress && (pair.second->name == groupId || pair.second->port == port))
                {
                    skip = true;
                    break;
                }
            }
        }
        if (skip)
            continue;

        // hægt er að crasha serverinn ef port er ekki tölur
        int serverSock = connectToServer(ipAddress, std::stoi(port), myGroupId);
        if (serverSock != -1)
        {
            logger.write("New client connected " + std::to_string(serverSock), "", 0);
            serverManager.add(serverSock, ipAddress.c_str(), port);
            pollManager.add(serverSock);
        }
    }
}
void ServerCommands::handleKeepAlive(int socket, std::vector<std::string> tokens)
{
}
void ServerCommands::handleSendMsg(int socket, std::vector<std::string> tokens)
{
    std::string toGroupId = tokens[1];
    std::string fromGroupId = tokens[2];

    std::string content;
    for (auto i = tokens.begin() + 3; i != tokens.end(); i++)
    {
        content += *i + " ";
    }
    std::string message = "Message from " + fromGroupId + ": " + content;
    send(socket, message.c_str(), message.length(), 0);
}
void ServerCommands::handleGetMsgs(int socket, std::vector<std::string> tokens)
{
}
void ServerCommands::handleStatusREQ(int socket, std::vector<std::string> tokens)
{
}
void ServerCommands::handleStatusRESP(int socket, std::vector<std::string> tokens)
{
}
