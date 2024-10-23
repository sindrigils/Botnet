#include "client-commands.hpp"

ClientCommands::ClientCommands(ServerManager &serverManager, Logger &logger, GroupMsgManager &groupMsgManager, ConnectionManager &connectionManager)
    : serverManager(serverManager), logger(logger), groupMsgManager(groupMsgManager), connectionManager(connectionManager)
{
}

void ClientCommands::findCommand(std::string message)
{
    std::vector<std::string> tokens = splitMessageOnDelimiter(message.c_str());

    // á eftir að abstracta þetta meira
    if (tokens[0].compare("GETMSG") == 0 && tokens.size() == 2)
    {
        handleGetMsg(tokens);
    }
    else if (tokens[0].compare("GETMSG") == 0 && tokens.size() == 3)
    {
        handleGetMsgFrom(tokens);
    }
    // this function is just to send messages to servers that did not respond with HELO
    else if (tokens[0].compare("SENDMSG") == 0 && tokens[1].compare("SOCK") == 0 && tokens.size() >= 4)
    {
        handleSendMsgToSock(tokens);
    }
    else if (tokens[0].compare("SENDMSG") == 0 && tokens.size() >= 3)
    {
        handleSendMsg(tokens);
    }
    else if (tokens[0].compare("LISTSERVERS") == 0)
    {
        handleListServers();
    }
    else if (tokens[0].compare("LISTUNKNOWN") == 0)
    {
        handleListUnknownServers();
    }
    else if (tokens[0].compare("CONNECT") == 0 && tokens.size() == 3)
    {
        handleConnect(tokens);
    }
    else if (tokens[0].compare("STATUSREQ") == 0 && tokens.size() == 2)
    {
        handleStatusREQ(tokens);
    }
    else if (tokens[0].compare("DROP") == 0 && tokens.size() == 2)
    {
        handleDropConnection(tokens);
    }
    else if (tokens[0].compare("BLACKLIST") == 0 && tokens.size() == 4)
    {
        handleAddToBlacklist(tokens);
    }
    // custom macros
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

void ClientCommands::handleGetMsgFrom(std::vector<std::string> tokens)
{
    std::string forGroupId = tokens[1];
    std::string fromGroupId = tokens[2];
    std::string msg = "GETMSGS," + forGroupId;
    std::string message = constructServerMessage(msg);

    int sock = serverManager.getSockByName(fromGroupId);
    if (sock != -1)
    {
        send(sock, message.c_str(), message.length(), 0);
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

void ClientCommands::handleSendMsgToSock(std::vector<std::string> tokens)
{
    std::string sock = tokens[2];
    std::string groupId = tokens[3];
    std::string message = "SENDMSG," + groupId + "," + MY_GROUP_ID + "," + tokens[4];
    connectionManager.sendTo(stringToInt(sock), message);
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
    connectionManager.connectToServer(ip, port, true);
}

void ClientCommands::handleStatusREQ(std::vector<std::string> tokens)
{
    std::string groupId = tokens[1];
    std::string message = "STATUSREQ";

    int sock = serverManager.getSockByName(groupId);
    if (sock != -1)
    {
        connectionManager.sendTo(sock, message);
    }
}

void ClientCommands::handleShortConnect(std::vector<std::string> tokens)
{
    std::string ip = "130.208.246.249";

    int port = stringToInt(tokens[1]);

    if(port >= 1 && port <= 3){
        port += 5000;
    }

    handleConnect({tokens[0], ip, std::to_string(port)});
}

void ClientCommands::handleListUnknownServers()
{
    int ourClientSock = serverManager.getOurClientSock();
    std::string message = serverManager.getListOfUnknownServers();
    send(ourClientSock, message.c_str(), message.length(), 0);
}

void ClientCommands::handleDropConnection(std::vector<std::string> tokens)
{
    int dropSock = stringToInt(tokens[1]);
    connectionManager.closeSock(dropSock);
}

void ClientCommands::handleAddToBlacklist(std::vector<std::string> tokens)
{
    connectionManager.addToBlacklist(tokens[1], tokens[2], tokens[3]);
}