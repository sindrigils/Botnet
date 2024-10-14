#include "group-message-manager.hpp"

void GroupMsgManager::addMessage(const std::string &groupId, const std::string &message)
{
    std::lock_guard<std::mutex> lock(mutex);

    auto &messages = groupMessages[groupId];

    if (messages.size() >= MAX_MESSAGES)
    {
        std::cout << "Dropping message for group " << groupId << ": buffer is full." << std::endl;
        return;
    }
    messages.push_back(message);
}

std::vector<std::string> GroupMsgManager::getMessages(const std::string &groupId)
{
    std::lock_guard<std::mutex> lock(mutex);

    std::vector<std::string> messages;

    auto it = groupMessages.find(groupId);
    if (it != groupMessages.end())
    {
        messages = std::move(it->second);
        groupMessages.erase(it);
    }

    return messages;
}

int GroupMsgManager::getMessageCount(const std::string &groupId) const
{
    std::lock_guard<std::mutex> lock(mutex);

    // Find the group ID in the map
    auto it = groupMessages.find(groupId);
    if (it != groupMessages.end())
    {
        return it->second.size();
    }

    return 0;
}

std::unordered_map<std::string, int> GroupMsgManager::getAllMessagesCount() const
{
    std::lock_guard<std::mutex> lock(mutex);
    std::unordered_map<std::string, int> groupTotalMessages;

    for (const auto &pair : groupMessages)
    {
        int counter = getMessageCount(pair.first);
        if (counter > 0)
        {
            groupTotalMessages[pair.first] = counter;
        }
    };
    return groupTotalMessages;
}