#include "client-commands.hpp"

ClientCommands::ClientCommands(
    ServerManager &serverManager,
    PollManager &pollManager,
    Logger &logger) : serverManager(serverManager),
                      pollManager(pollManager),
                      logger(logger) {};

void ClientCommands::setGroupId(const std::string &groupId)
{
    myGroupId = groupId; // Set the group ID
}

void ClientCommands::setSock(int newSock)
{
    sock = newSock;
}

void ClientCommands::findCommand(std::vector<std::string> tokens, const char *buffer)
{
    // á eftir að abstracta þetta meira
    if (tokens[0].compare("GETMSG") == 0 && tokens.size() == 2)
    {
        std::string message = "NOT IMPLEMENTED";
        send(sock, message.c_str(), message.length(), 0);
    }
    else if (tokens[0].compare("GETMSG") == 0 && tokens.size() == 3)
    {
        std::string forGroupId = tokens[1];
        std::string fromGroupId = tokens[2];
        std::string msg = "GETMSGS," + forGroupId;
        std::string message = constructServerMessage(msg);

        for (auto &pair : serverManager.servers)
        {
            if (pair.second->name == fromGroupId)
            {
                std::cout << "SENDING GETMSGS" << std::endl;
                send(pair.second->sock, message.c_str(), message.length(), 0);
            }
        }
    }
    else if (tokens[0].compare("SENDMSG") == 0 && tokens.size() == 3)
    {
        std::string groupId = tokens[1];
        for (auto const &pair : serverManager.servers)
        {
            if (pair.second->name.compare(groupId) == 0)
            {
                std::string content;
                for (auto i = tokens.begin() + 2; i != tokens.end(); i++)
                {
                    content += *i + " ";
                }
                std::string message = "SENDMSG," + groupId + "," + myGroupId + "," + content;
                std::string serverMessage = constructServerMessage(message);
                send(pair.second->sock, serverMessage.c_str(), serverMessage.length(), 0);
            }
        }
    }
    else if (tokens[0].compare("MSG") == 0 && tokens[1].compare("ALL") == 0)
    {
        std::string msg;
        for (auto i = tokens.begin() + 2; i != tokens.end(); i++)
        {
            msg += *i + " ";
        }

        for (auto const &pair : serverManager.servers)
        {
            send(pair.second->sock, msg.c_str(), msg.length(), 0);
        }
    }
    else if (tokens[0].compare("LISTSERVERS") == 0)
    {
        std::string message = "";
        if (serverManager.servers.size() == 0)
        {
            message = "Not connected to any servers.";
        }
        else
        {
            for (const auto &pair : serverManager.servers)
            {
                message += pair.second->name + ", ";
            }
            message = message.substr(0, message.length() - 2);
        }

        send(sock, message.c_str(), message.length(), 0);
    }
    else if (tokens[0].compare("CONNECT") == 0 && tokens.size() == 3)
    {
        std::string message = "";
        std::string ip = tokens[1];
        int port = std::stoi(tokens[2]);
        bool serverSock = connectToServer(trim(tokens[1]), port, myGroupId);
        if (serverSock == -1)
        {
            std::cout << "ERRROOOOOORRRR" << std::endl;
            return;
        }
        serverManager.add(serverSock, ip.c_str(), std::to_string(port));
        pollManager.add(serverSock);
        message = serverSock ? "Successfully connected to the server." : "Unable to connect to the server.";
        send(sock, message.c_str(), message.length(), 0);
    }
    else if (tokens[0].compare("STATUSREQ") == 0 && tokens.size() == 2)
    {
        std::string groupId = tokens[1];
        std::string message = constructServerMessage("STATUSREQ");

        for (const auto &pair : serverManager.servers)
        {
            if (pair.second->name == groupId)
            {
                std::cout << "found server sending STATUSREQ" << std::endl;
                send(pair.second->sock, message.c_str(), message.length(), 0);
                break;
            }
        }
    }
    else
    {
        std::cout << "Unknown command from client:" << buffer << std::endl;
    }
}
