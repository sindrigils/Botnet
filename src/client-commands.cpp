#include "client-commands.hpp"

ClientCommands::ClientCommands(ServerManager &serverManager, Logger &logger, GroupMsgManager &groupMsgManager, ConnectionManager &connectionManager)
    : serverManager(serverManager), logger(logger), groupMsgManager(groupMsgManager), connectionManager(connectionManager)
{
}

void ClientCommands::findCommand(std::string message)
{
    std::vector<std::string> tokens = splitMessageOnDelimiter(message.c_str());
    std::string command = toLower(tokens[0]);

    // required commands
    if (command.compare("getmsg") == 0 && tokens.size() == 2)
    {
        this->handleGetMsg(tokens);
    }
    else if (command.compare("sendmsg") == 0 && tokens.size() >= 3)
    {
        this->handleSendMsg(tokens);
    }
    else if (command.compare("listservers") == 0)
    {
        this->handleListServers();
    }
    // custom commands that we made
    else if (command.compare("connect") == 0 && tokens.size() == 3)
    {
        this->handleConnect(tokens);
    }
    else if (command.compare("drop") == 0 && tokens.size() == 2)
    {
        this->handleDropConnection(tokens);
    }
    else if (command.compare("blacklist") == 0 && tokens.size() == 4)
    {
        this->handleAddToBlacklist(tokens);
    }
    else if (command.compare("blacklist") == 0 && tokens.size() == 1)
    {
        this->handleViewBlacklist();
    }
    else if (command.compare("help") == 0 && tokens.size() == 1)
    {
        this->handleHelp();
    }
    else if (command.compare("c") == 0 && tokens.size() == 2)
    {
        this->handleShortConnect(tokens);
    }
    else
    {
        logger.write("[FAILURE] Unknown command from client:" + message, true);
    }
}

void ClientCommands::handleGetMsg(std::vector<std::string> tokens)
{
    std::string groupId = tokens[1];
    std::vector<std::string> messages = groupMsgManager.getMessages(groupId);

    for (auto message : messages)
    {
        std::vector<std::string> msg = splitMessageOnDelimiter(message.c_str());

        std::ostringstream contentStream;
        for (auto it = msg.begin() + 3; it != msg.end(); it++)
        {
            contentStream << *it;
            if (it + 1 != msg.end())
            {
                contentStream << ", ";
            }
        }
        int sock = serverManager.getOurClientSock();
        connectionManager.sendTo(sock, contentStream.str(), true);
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

    std::string message = "SENDMSG," + groupId + "," + MY_GROUP_ID + "," + contentStream.str();

    int sock = serverManager.getSockByName(groupId);
    if (sock == -1)
    {
        logger.write("[INFO] Storing message for " + groupId, true);
        groupMsgManager.addMessage(trim(groupId), message);
        return;
    }

    connectionManager.sendTo(sock, message);
}

void ClientCommands::handleListServers()
{
    int ourClientSock = serverManager.getOurClientSock();
    std::string message = serverManager.getListOfServers();
    send(ourClientSock, message.c_str(), message.length(), 0);
}

void ClientCommands::handleConnect(std::vector<std::string> tokens)
{
    std::string message = "";
    std::string ip = trim(tokens[1]);
    std::string port = trim(tokens[2]);
    connectionManager.connectToServer(ip, port);
}

void ClientCommands::handleShortConnect(std::vector<std::string> tokens)
{
    std::string ip = "130.208.246.249";

    int port = stringToInt(tokens[1]);

    if (port >= 1 && port <= 3)
    {
        port += 5000;
    }

    handleConnect({tokens[0], ip, std::to_string(port)});
}

void ClientCommands::handleDropConnection(std::vector<std::string> tokens)
{
    int sock = serverManager.getSockByName(tokens[1]);
    connectionManager.closeSock(sock);
}

void ClientCommands::handleAddToBlacklist(std::vector<std::string> tokens)
{
    int sock = serverManager.getSockByName(tokens[1]);
    if (sock != -1)
    {
        this->handleDropConnection({"drop", tokens[1]});
    }
    connectionManager.addToBlacklist(tokens[1], tokens[2], tokens[3]);
}

void ClientCommands::handleViewBlacklist()
{
    std::string message;
    std::set<std::tuple<std::string, std::string, std::string>> blacklist = connectionManager.getBlacklistedServers();

    if (blacklist.empty())
    {
        message += "No servers are currently blacklisted.\n";
    }
    else
    {
        message += "Blacklisted Servers:\n";

        int count = 1;
        for (const auto &server : blacklist)
        {
            std::string groupId = std::get<0>(server);
            std::string ip = std::get<1>(server);
            std::string port = std::get<2>(server);

            message += std::to_string(count++) + ". Group ID: " + groupId + ", IP: " + ip + ", Port: " + port + "\n";
        }
    }

    int sock = serverManager.getOurClientSock();
    connectionManager.sendTo(sock, message, true);
}

void ClientCommands::handleHelp()
{
    std::string message = "================================== HELP =====================================\n";
    message += "Command                         Description\n";
    message += "----------------------------------------------\n";
    message += "LISTSERVERS                     View all connected servers by name\n";
    message += "GETMSG,<groupId>                Get a message from the server for that group\n";
    message += "SENDMSG,<groupId>,<msg>         Send a message from you to that server\n";
    message += "CONNECT,<ip>,<port>             Connect to the server with that IP and port\n";
    message += "DROP,<groupId>                  Drop the connection to that group\n";
    message += "BLACKLIST,<groupId>,<ip>,<port> Blacklist the server\n";
    message += "BLACKLIST                       View all blacklisted servers\n";
    message += "==============================================================================\n";

    int sock = serverManager.getOurClientSock();
    connectionManager.sendTo(sock, message, true);
}
