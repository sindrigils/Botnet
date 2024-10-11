#include "server-commands.hpp"

ServerCommands::ServerCommands(
    ServerManager &serverManager,
    PollManager &pollManager,
    Logger &logger,
    GroupMessageManager &groupMessageManager) : serverManager(serverManager),
                                                pollManager(pollManager),
                                                logger(logger),
                                                groupMessageManager(groupMessageManager),
                                                myIpAddress("-1"),
                                                myGroupId("-1"),
                                                myPort("-1") {};

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

void ServerCommands::setOurClient(int sock)
{
    ourClient = sock;
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
    for (auto &token : tokens)
    {
        std::vector<std::string> serverInfo = splitMessageOnDelimiter(token.c_str());
        std::string groupId = trim(serverInfo[0]);
        std::string ipAddress = trim(serverInfo[1]);
        std::string port = trim(serverInfo[2]);

        logger.write("Attempting to connect to server " + groupId + " " + ipAddress + ":" + port);
        // abstracta þetta
        if (groupId == myGroupId || port == "-1" || ipAddress == "-1")
        {
            logger.write("Skipping self-connection!");
            continue;
        }

        // Abstract þetta?
        bool isAlreadyConnected = std::any_of(serverManager.servers.begin(), serverManager.servers.end(),
                                              [&](const std::pair<const int, Server *> &pair)
                                              {
                                                  return pair.second->ipAddress == ipAddress && (pair.second->name == groupId || pair.second->port == port);
                                              });

        if (isAlreadyConnected)
        {
            logger.write(groupId + " " + ipAddress + ":" + port + " already connected!");
            continue;
        }

        int serverSock = connectToServer(ipAddress, stringToInt(port), myGroupId);
        if (serverSock != -1)
        {
            logger.write("Server connected: " + groupId + " " + ipAddress + ":" + port);
            serverManager.add(serverSock, ipAddress.c_str(), port);
            pollManager.add(serverSock);
        }
        else
        {
            logger.write("Unable to connect to server: " + groupId + " " + ipAddress + ":" + port);
        }
    }
}
void ServerCommands::handleKeepAlive(int socket, std::vector<std::string> tokens)
{
    int numberOfMessages = stringToInt(tokens[1]);
    if (numberOfMessages == -1 || numberOfMessages <= 0)
    {
        return;
    }

    std::string message = "GETMSGS," + myGroupId;
    std::string serverMessage = constructServerMessage(message);
    send(socket, serverMessage.c_str(), serverMessage.length(), 0);
}

void ServerCommands::handleSendMsg(int socket, std::vector<std::string> tokens)
{
    std::string toGroupId = tokens[1];
    std::string fromGroupId = tokens[2];

    if (toGroupId != myGroupId)
    {
        std::cout << "found a message not for us" << std::endl;
        return;
    }

    std::ostringstream contentStream;
    for (auto it = tokens.begin() + 3; it != tokens.end(); it++)
    {
        contentStream << *it;
        // since if it got split then that means that there was a comma there
        if (it + 1 != tokens.end())
        {
            // but we dont know if he had a space in front of the comma or not
            contentStream << ", ";
        }
    }
    std::string message = "Message from " + fromGroupId + ": " + contentStream.str() + "\n";
    send(ourClient, message.c_str(), message.length(), 0);
}

void ServerCommands::handleGetMsgs(int socket, std::vector<std::string> tokens)
{
    std::string forGroupId = tokens[1];
    std::vector<std::string> messages = groupMessageManager.getMessages(forGroupId);

    // the message is already stores as "<SOH>SENDMSG....<EOT>", so we can just send it right away
    for (auto msg : messages)
    {
        send(socket, msg.c_str(), msg.length(), 0);
    };
}
void ServerCommands::handleStatusREQ(int socket, std::vector<std::string> tokens)
{
}
void ServerCommands::handleStatusRESP(int socket, std::vector<std::string> tokens)
{
}

std::unordered_map<int, std::string> ServerCommands::constructKeepAliveMessages()
{
    std::unordered_map<int, std::string> messages;

    std::unordered_map<int, std::string> connectedSockets = serverManager.getConnectedSockets();
    for (auto &pair : connectedSockets)
    {
        int messageCount = groupMessageManager.getMessageCount(pair.second);
        std::string message = "KEEPALIVE," + std::to_string(messageCount);
        std::string serverMessage = constructServerMessage(message);
        messages[pair.first] = serverMessage;
    }

    return messages;
}