#include "group-message-manager.hpp"

void GroupMsgManager::addMessage(const std::string &groupId, const std::string &message)
{
    std::lock_guard<std::mutex> lock(groupMutex);

    auto &messages = groupMessages[groupId];

    if (messages.size() >= MAX_MESSAGES)
    {
        std::cout << "Dropping message for group " << groupId << ": buffer is full." << std::endl;
        return;
    }
    messages.push_back(message);
}

std::vector<std::string> GroupMsgManager::getMessages(std::string groupId)
{
    std::lock_guard<std::mutex> lock(groupMutex);

    std::vector<std::string> messages;

    auto it = groupMessages.find(groupId);
    if (it != groupMessages.end())
    {
        messages = std::move(it->second);
        groupMessages.erase(it);
    }

    return messages;
}

int GroupMsgManager::getMessageCount(std::string groupId) const
{
    std::lock_guard<std::mutex> lock(groupMutex);

    // Find the group ID in the map
    auto it = groupMessages.find(groupId);
    if (it != groupMessages.end())
    {
        return it->second.size();
    }

    return 0;
}

std::unordered_map<std::string, int> GroupMsgManager::getAllMessagesCount(bool showClientMsgs) const
{
    std::lock_guard<std::mutex> lock(groupMutex);
    std::unordered_map<std::string, int> groupTotalMessages;

    if (showClientMsgs && !clientMessages.empty())
    {
        groupTotalMessages[MY_GROUP_ID] = clientMessages.size();
    }

    for (const auto &pair : this->groupMessages)
    {
        int counter = pair.second.size();
        if (counter > 0)
        {
            groupTotalMessages[pair.first] = counter;
        }
    };
    return groupTotalMessages;
}

void GroupMsgManager::addClientMessage(std::string message)
{
    std::lock_guard<std::mutex> lock(clientMutex);
    clientMessages.push_back(message);
}

std::vector<std::string> GroupMsgManager::getAllClientMessages()
{
    std::lock_guard<std::mutex> lock(clientMutex);
    std::vector<std::string> messages = clientMessages;
    clientMessages.clear();
    return messages;
}

int GroupMsgManager::getAllClientMessagesCount() const
{
    std::lock_guard<std::mutex> lock(clientMutex);
    return clientMessages.size();
}