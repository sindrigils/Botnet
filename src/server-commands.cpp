#include "server-commands.hpp"

std::mutex mtx;

ServerCommands::ServerCommands(
    ServerManager &serverManager,
    GroupMsgManager &groupMsgManager,
    ConnectionManager &connectionManager,
    Logger &logger) : serverManager(serverManager),
                      groupMsgManager(groupMsgManager),
                      connectionManager(connectionManager),
                      logger(logger),
                      myPort("-1") {};

void ServerCommands::setPort(const std::string &port)
{
    myPort = port;
}

void ServerCommands::findCommand(int socket, std::string message)
{
    std::vector<std::string> tokens = splitMessageOnDelimiter(message.c_str());

    if (tokens[0].compare("PW") == 0 && tokens.size() == 2){
        return handlePW(socket, tokens);
    }

    if (tokens[0].compare("HELO") == 0 && tokens.size() == 2)
    {
        return handleHelo(socket, tokens);
    }

    if (!serverManager.isKnown(socket))
    {
        logger.write("[INFO] Message from unknown server, sock: " + std::to_string(socket) + ", message: " + message);
        return;
    }

    if (tokens[0].compare("SERVERS") == 0 && tokens.size() >= 2)
    {
        return handleServers(socket, message);
    }

    if (tokens[0].compare("KEEPALIVE") == 0 && tokens.size() == 2)
    {
        return handleKeepAlive(socket, tokens);
    }

    if (tokens[0].compare("GETMSGS") == 0 && tokens.size() == 2)
    {
        return handleGetMsgs(socket, tokens);
    }

    if (tokens[0].compare("SENDMSG") == 0 && tokens.size() >= 4)
    {
        return handleSendMsg(socket, tokens, message);
    }

    if (tokens[0].compare("STATUSREQ") == 0 && tokens.size() == 1)
    {
        return handleStatusReq(socket, tokens);
    }
    
    if (tokens[0].compare("STATUSRESP") == 0 && tokens.size() >= 1)
    {
        return handleStatusResp(socket, tokens);
    }

    logger.write("[FAILURE] Unknown command from server:" + message, true);
    return;
}

void ServerCommands::handleHelo(int socket, std::vector<std::string> tokens)
{
    std::string groupId = tokens[1];

    if (connectionManager.isBlacklisted(groupId) || serverManager.isConnectedToGroupId(groupId, socket) || !validateGroupId(groupId))
    {
        connectionManager.closeSock(socket);
        return;
    }

    int success = serverManager.moveFromUnknown(socket, tokens[1]);
    if (success == -1)
    {
        connectionManager.closeSock(socket);
        return;
    }
    std::string msg = "SERVERS," + std::string(MY_GROUP_ID) + "," + connectionManager.getOwnIPFromSocket(socket) + "," + myPort + ";";
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
        if (serverInfo.size() != 3)
        {
            continue;
        }

        std::string groupId = trim(serverInfo[0]);
        std::string ipAddress = trim(serverInfo[1]);
        std::string port = trim(serverInfo[2]);

        if (token == tokens[0])
        {
            serverManager.update(socket, port);
        }

        bool isAlreadyConnected = serverManager.hasConnectedToServer(ipAddress, port, groupId);
        if (isAlreadyConnected || groupId == std::string(MY_GROUP_ID) || port == "-1" || ipAddress == "-1" || groupId.empty() || ipAddress.empty() || port.empty())
        {
            continue;
        }

        connectionManager.connectToServer(ipAddress, port, groupId);
    }
}
void ServerCommands::handleKeepAlive(int socket, std::vector<std::string> tokens)
{
    int numberOfMessages = stringToInt(tokens[1]);
    if (numberOfMessages <= 0)
    {
        return;
    }

    std::string message = "GETMSGS," + std::string(MY_GROUP_ID);
    connectionManager.sendTo(socket, message);
}

void ServerCommands::handleSendMsg(int socket, std::vector<std::string> tokens, std::string buffer)
{
    std::string toGroupId = tokens[1];
    std::string fromGroupId = tokens[2];

    if (toGroupId == MY_GROUP_ID)
    {
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
        std::string message = "Message from " + fromGroupId + ": " + contentStream.str();

        // Seperately log numbers to collect them... gotta collect them all.
        if (fromGroupId == "NUMBER")
        {
            logger.write(contentStream.str(), false, "numbers.txt");
        }

        int ourClient = serverManager.getOurClientSock();
        connectionManager.sendTo(ourClient, message, true);
        return;
    }

    bool isConnected = serverManager.isConnectedToGroupId(toGroupId, socket);
    if (isConnected)
    {
        int groupSock = serverManager.getSockByName(toGroupId);
        connectionManager.sendTo(groupSock, buffer);
        return;
    }

    groupMsgManager.addMessage(toGroupId, buffer);
    logger.write("[INFO] Storing message for " + toGroupId, true);
}

void ServerCommands::handleGetMsgs(int socket, std::vector<std::string> tokens)
{
    std::string forGroupId = tokens[1];
    std::vector<std::string> messages = groupMsgManager.getMessages(forGroupId);

    for (auto msg : messages)
    {
        connectionManager.sendTo(socket, msg);
    };
}

void ServerCommands::handleStatusReq(int socket, std::vector<std::string> tokens)
{
    std::unordered_map<std::string, int> totalStoredMessages = groupMsgManager.getAllMessagesCount();
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
    for (size_t i = 1; i + 1 < tokens.size(); i += 2)
    {
        std::string groupId = tokens[i];
        int amountOfMessages = stringToInt(tokens[i + 1]);
        bool isConnectedTo = serverManager.isConnectedToGroupId(groupId, socket);
        if (isConnectedTo && amountOfMessages > 0)
        {
            connectionManager.sendTo(socket, "GETMSGS," + groupId);
        }
    }
}

void ServerCommands::handlePW(int socket, std::vector<std::string> tokens){
    // Add the server as "our" client if the second token is the client pw and there is no current client
    if (serverManager.getOurClientSock() == -1 && tokens[1] == std::string(CLIENT_PW))
    {
        serverManager.setOurClientSock(socket);
        logger.write("[INFO] Our client connected:, sock: " + std::to_string(socket), true);
        connectionManager.sendTo(socket, "Well hello there!");
        // Remove our socket from the remote servers.
        serverManager.close(socket);
        return;
    }
}

std::unordered_map<int, std::string> ServerCommands::constructKeepAliveMessages()
{
    std::unordered_map<int, std::string> messages;

    std::unordered_map<int, std::string> connectedSockets = serverManager.getConnectedSockets();
    for (const auto &pair : connectedSockets)
    {
        int messageCount = groupMsgManager.getMessageCount(pair.second);
        std::string message = "KEEPALIVE," + std::to_string(messageCount);
        messages[pair.first] = message;
    }

    return messages;
}
