#include "group-message-manager.hpp"

void GroupMessageManager::addMessage(const std::string &groupId, const std::string &message)
{
    std::lock_guard<std::mutex> lock(mutex); // Lock the mutex for thread safety

    // Check if the vector for this group already exists
    auto &messages = groupMessages[groupId];

    // Check if the vector has reached its maximum length
    if (messages.size() >= MAX_MESSAGE_LENGTH)
    {
        // Optionally log that the message was dropped
        std::cout << "Dropping message for group " << groupId << ": buffer is full." << std::endl;
        return;

        // Add the message to the vector corresponding to the groupId
        messages.push_back(message);
    }
}

std::vector<std::string> GroupMessageManager::getMessages(const std::string &groupId)
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

int GroupMessageManager::getMessageCount(const std::string &groupId) const
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