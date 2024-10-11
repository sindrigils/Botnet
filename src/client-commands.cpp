#include "client-commands.hpp"

ClientCommands::ClientCommands(
    ServerManager &serverManager,
    PollManager &pollManager,
    Logger &logger,
    GroupMessageManager &groupMessageManager) : serverManager(serverManager),
                                                pollManager(pollManager),
                                                logger(logger),
                                                groupMessageManager(groupMessageManager) {};

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
        handleGetMsg(tokens);
    }
    else if (tokens[0].compare("GETMSG") == 0 && tokens.size() == 3)
    {
        handleGetMsgFrom(tokens);
    }
    else if (tokens[0].compare("SENDMSG") == 0 && tokens.size() >= 3)
    {
        handleSendMsg(tokens);
    }
    else if (tokens[0].compare("MSG") == 0 && tokens[1].compare("ALL") == 0)
    {
        handleMsgAll(tokens);
    }
    else if (tokens[0].compare("LISTSERVERS") == 0)
    {
        handleListServers(tokens);
    }
    else if (tokens[0].compare("CONNECT") == 0 && tokens.size() == 3)
    {
        handleConnect(tokens);
    }
    else if (tokens[0].compare("STATUSREQ") == 0 && tokens.size() == 2)
    {
        handleStatusREQ(tokens);
    }
    // custom macros
    else if (tokens[0].compare("c") == 0 && tokens.size() == 2)
    {
        handleShortConnect(tokens);
    }
    else
    {
        std::cout << "Unknown command from client:" << buffer << std::endl;
    }
}

void ClientCommands::handleGetMsg(std::vector<std::string> tokens)
{
    std::string message = "NOT IMPLEMENTED";
    send(sock, message.c_str(), message.length(), 0);
}

void ClientCommands::handleGetMsgFrom(std::vector<std::string> tokens)
{
    std::string forGroupId = tokens[1];
    std::string fromGroupId = tokens[2];
    std::string msg = "GETMSGS," + forGroupId;
    std::string message = constructServerMessage(msg);

    for (auto &pair : serverManager.servers)
    {
        if (pair.second->name == fromGroupId)
        {
            send(pair.second->sock, message.c_str(), message.length(), 0);
        }
    }
}

void ClientCommands::handleSendMsg(std::vector<std::string> tokens)
{
    std::string groupId = tokens[1];

    std::ostringstream contentStream;
    for (auto it = tokens.begin() + 2; it != tokens.end(); it++)
    {

        contentStream << *it;
        // since if it got split then that means that there was a comma there
        if (it + 1 != tokens.end())
        {
            // but we dont know if he had a space in front of the comma or not
            contentStream << ", ";
        }
    }

    std::string message = "SENDMSG," + groupId + "," + myGroupId + "," + contentStream.str();
    std::string serverMessage = constructServerMessage(message);

    for (auto const &pair : serverManager.servers)
    {
        if (pair.second->name.compare(groupId) == 0)
        {
            send(pair.second->sock, serverMessage.c_str(), serverMessage.length(), 0);
            return;
        }
    }
    logger.write("Storing message for " + groupId, true);
    groupMessageManager.addMessage(trim(groupId), serverMessage);
}

void ClientCommands::handleMsgAll(std::vector<std::string> tokens)
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

void ClientCommands::handleListServers(std::vector<std::string> tokens)
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

void ClientCommands::handleConnect(std::vector<std::string> tokens)
{
    logger.write("Attempting to connect to server (by client), ip: " + tokens[1] + ", port: " + tokens[2], true);
    std::string message = "";
    std::string ip = tokens[1];
    int port = stringToInt(tokens[2]);
    int serverSock = connectToServer(trim(tokens[1]), port, myGroupId);
    if (serverSock == -1)
    {
        return;
    }
    serverManager.add(serverSock, ip.c_str(), std::to_string(port));
    pollManager.add(serverSock);
    message = serverSock ? "Successfully connected to the server." : "Unable to connect to the server.";
    send(sock, message.c_str(), message.length(), 0);
}

void ClientCommands::handleStatusREQ(std::vector<std::string> tokens)
{
    std::string groupId = tokens[1];
    std::string message = constructServerMessage("STATUSREQ");

    for (const auto &pair : serverManager.servers)
    {
        if (pair.second->name == groupId)
        {
            send(pair.second->sock, message.c_str(), message.length(), 0);
            break;
        }
    }
}

void ClientCommands::handleShortConnect(std::vector<std::string> tokens)
{
    std::string ip = "130.208.246.249";
    int port = 5000 + stringToInt(tokens[1]);

    if(port < 5000 || port > 5003) {
        logger.write("Invalid port number for INSTR server in short connect (" + std::to_string(port) + ")", true);
        return;
    }

    handleConnect({tokens[0], ip, std::to_string(port)});
}