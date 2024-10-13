#include "server-commands.hpp"

std::mutex mtx;

ServerCommands::ServerCommands(
    ServerManager &serverManager,
    PollManager &pollManager,
    Logger &logger,
    GroupMessageManager &groupMessageManager,
    ConnectionManager &connectionManager) : serverManager(serverManager),
                                            pollManager(pollManager),
                                            logger(logger),
                                            groupMessageManager(groupMessageManager),
                                            connectionManager(connectionManager),
                                            myIpAddress("-1"),
                                            myGroupId("-1"),
                                            myPort("-1") {};

void ServerCommands::setIpAddress(const char *ip)
{
    myIpAddress = ip;
}

void ServerCommands::setGroupId(const std::string &groupId)
{
    myGroupId = groupId;
}

void ServerCommands::setPort(const std::string &port)
{
    myPort = port;
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
        handleSendMsg(socket, tokens, buffer);
    }
    else if (tokens[0].compare("STATUSREQ") == 0 && tokens.size() == 1)
    {
        handleStatusReq(socket, tokens);
    }
    else if (tokens[0].compare("STATUSRESP") == 0 && tokens.size() >= 1)
    {
        handleStatusResp(socket, tokens);
    }
    else
    {
        std::cout << "Unknown command from server:" << buffer << std::endl;
    }
}

void ServerCommands::handleHelo(int socket, std::vector<std::string> tokens)
{
    serverManager.update(socket, "", tokens[1]);

    std::string msg = "SERVERS," + myGroupId + "," + myIpAddress + "," + myPort + ";";
    std::string serversInfo = serverManager.getAllServersInfo();
    std::string message = msg + serversInfo;

    connectionManager.sendTo(socket, message);
}

void ServerCommands::handleServers(int socket, std::string buffer)
{
    std::lock_guard<std::mutex> guard(mtx);
    std::vector<std::string> tokens = splitMessageOnDelimiter(buffer.substr(8).c_str(), ';');
    for (auto &token : tokens)
    {
        std::vector<std::string> serverInfo = splitMessageOnDelimiter(token.c_str());
        std::string groupId = trim(serverInfo[0]);
        std::string ipAddress = trim(serverInfo[1]);
        std::string port = trim(serverInfo[2]);

        if (token == tokens[0])
        {
            serverManager.update(socket, port, groupId);
        }

        bool isAlreadyConnected = serverManager.hasConnectedToServer(ipAddress, port, groupId);
        if (isAlreadyConnected || groupId == myGroupId || port == "-1" || ipAddress == "-1" || groupId.empty() || ipAddress.empty() || port.empty())
        {
            continue;
        }

        std::thread([this, ipAddress = std::string(ipAddress), port = std::string(port), groupId = std::string(groupId)]()
                    {
            logger.write("Attempting to connect to ip: " + ipAddress + ", port: " + port + ", groupId: " + groupId);
            int serverSock = connectionManager.connectToServer(ipAddress, port, myGroupId);
            if (serverSock != -1)
            {
                logger.write("Server connected - groupId: " + groupId + ", ipAddress:  " + ipAddress + ", port: " + port + ", sock: " + std::to_string(serverSock));
                serverManager.add(serverSock, ipAddress.c_str(), port);
                pollManager.add(serverSock);
            }
            else
            {
                logger.write("Unable to connect to server - groupId: " + groupId + ", ipAddress:  " + ipAddress + ", port: " + port);
            } })
            .detach();
    }
}
void ServerCommands::handleKeepAlive(int socket, std::vector<std::string> tokens)
{
    int numberOfMessages = stringToInt(tokens[1]);
    if (numberOfMessages <= 0)
    {
        return;
    }

    std::string message = "GETMSGS," + myGroupId;
    connectionManager.sendTo(socket, message);
}

void ServerCommands::handleSendMsg(int socket, std::vector<std::string> tokens, std::string buffer)
{
    std::string toGroupId = tokens[1];
    std::string fromGroupId = tokens[2];

    if (toGroupId != myGroupId)
    {
        std::string message = constructServerMessage(buffer);
        groupMessageManager.addMessage(toGroupId, message);
        std::cout << "found a message not for us: storing for " << toGroupId << std::endl;
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
    connectionManager.sendTo(ourClient, message, true);
}

void ServerCommands::handleGetMsgs(int socket, std::vector<std::string> tokens)
{
    std::string forGroupId = tokens[1];
    std::vector<std::string> messages = groupMessageManager.getMessages(forGroupId);

    for (auto msg : messages)
    {
        connectionManager.sendTo(socket, msg);
    };
}
void ServerCommands::handleStatusReq(int socket, std::vector<std::string> tokens)
{
    std::unordered_map<std::string, int> totalStoredMessages = groupMessageManager.getAllMessagesCount();
    std::string message = "STATUSRESP,";
    for (const auto &pair : totalStoredMessages)
    {
        message += pair.first + "," + std::to_string(pair.second) + ",";
    };
    // remove the trailing ",", even if there is no stored messages, then we remove the "," from the command
    message.pop_back();

    connectionManager.sendTo(socket, message);
}
void ServerCommands::handleStatusResp(int socket, std::vector<std::string> tokens)
{
}

std::unordered_map<int, std::string> ServerCommands::constructKeepAliveMessages()
{
    std::unordered_map<int, std::string> messages;

    std::unordered_map<int, std::string> connectedSockets = serverManager.getConnectedSockets();
    for (const auto &pair : connectedSockets)
    {
        int messageCount = groupMessageManager.getMessageCount(pair.second);
        std::string message = "KEEPALIVE," + std::to_string(messageCount);
        std::string serverMessage = constructServerMessage(message);
        messages[pair.first] = serverMessage;
    }

    return messages;
}