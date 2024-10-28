#include "client-commands.hpp"

ClientCommands::ClientCommands(ServerManager &serverManager, Logger &logger, GroupMsgManager &groupMsgManager, ConnectionManager &connectionManager)
    : serverManager(serverManager), logger(logger), groupMsgManager(groupMsgManager), connectionManager(connectionManager)
{
}

void ClientCommands::findCommand(std::string message)
{
    std::vector<std::string> tokens = splitMessageOnDelimiter(message.c_str());

    // required commands
    if (tokens[0].compare("GETMSG") == 0 && tokens.size() == 2)
    {
        handleGetMsg(tokens);
    }
    else if (tokens[0].compare("SENDMSG") == 0 && tokens.size() >= 3)
    {
        handleSendMsg(tokens);
    }
    else if (tokens[0].compare("LISTSERVERS") == 0)
    {
        handleListServers();
    }
    // custom commands we made
    else if (tokens[0].compare("CONNECT") == 0 && tokens.size() == 3)
    {
        handleConnect(tokens);
    }
    else if (tokens[0].compare("DROP") == 0 && tokens.size() == 2)
    {
        handleDropConnection(tokens);
    }
    else if (tokens[0].compare("BLACKLIST") == 0 && tokens.size() == 4)
    {
        handleAddToBlacklist(tokens);
    }
    else if (tokens[0].compare("HELP") == 0 && tokens.size() == 1)
    {
        handleHelp();
    }
    else if (tokens[0].compare("BLACKLIST") == 0 && tokens.size() == 1)
    {
        handleViewBlacklist();
    }
    else if (tokens[0].compare("MSGS") == 0 && tokens.size() == 1)
    {
        handleViewMsgs();
    }
    // custom functon for us, to connect faster to other servers, however the prefered way
    // is to use the CONNECT command
    else if (tokens[0].compare("c") == 0 && tokens.size() == 2)
    {
        handleShortConnect(tokens);
    }
    else
    {
        std::cout << "Unknown command from client:" << message << std::endl;
    }
}

void ClientCommands::handleGetMsg(std::vector<std::string> tokens)
{
    int sock = serverManager.getOurClientSock();

    std::string groupId = tokens[1];
    std::vector<std::string> messages = (groupId == MY_GROUP_ID)
                                            ? groupMsgManager.getAllClientMessages()
                                            : groupMsgManager.getMessages(groupId);

    if (messages.size() == 0)
    {
        std::string emptyMessage = "There are no currently stored messages for you.";
        connectionManager.sendTo(sock, emptyMessage, true);
        return;
    }

    for (const auto &message : messages)
    {
        std::string content = (groupId == MY_GROUP_ID)
                                  ? message
                                  : formatGroupMessage(message);
        connectionManager.sendTo(sock, content, true);
    }
}

void ClientCommands::handleSendMsg(std::vector<std::string> tokens)
{
    std::string groupId = tokens[1];
    if (groupId == MY_GROUP_ID)
    {
        std::string message = "Why are you talking to yourself?";
        int sock = serverManager.getOurClientSock();
        connectionManager.sendTo(sock, message, true);
        return;
    }

    std::ostringstream contentStream;
    for (auto it = tokens.begin() + 2; it != tokens.end(); it++)
    {

        contentStream << *it;
        // since if it got split then that means that there was a comma there
        if (it + 1 != tokens.end())
        {
            // but we dont know if he had a space in front of the comma or not
            // but it looks better with an extra space :D
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

    // if the port is between 1 and 3 then we are trying to connect to an instructor server
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
    connectionManager.addToBlacklist(tokens[1], tokens[2], tokens[3]);
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
    message += "MSGS                            View all stored messages\n";
    message += "DROP,<groupId>                  Drop the connection to that group\n";
    message += "BLACKLIST                       View all blacklisted servers\n";
    message += "BLACKLIST,<groupId>,<ip>,<port> Blacklist the server\n";
    message += "==============================================================================\n";

    int sock = serverManager.getOurClientSock();
    connectionManager.sendTo(sock, message, true);
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

void ClientCommands::handleViewMsgs()
{
    std::string message;
    std::unordered_map<std::string, int> totalStoredMessages = groupMsgManager.getAllMessagesCount(true);

    if (totalStoredMessages.size() == 0)
    {
        message += "There are no stored messages.";
    }
    else
    {
        message += "";
        for (const auto &pair : totalStoredMessages)
        {
            message += pair.first + ": " + std::to_string(pair.second) + "\n";
        };
    }

    int sock = serverManager.getOurClientSock();
    connectionManager.sendTo(sock, message, true);
}