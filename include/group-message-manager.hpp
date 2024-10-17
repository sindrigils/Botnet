#ifndef GROUP_MESSAGE_MANAGER_HPP
#define GROUP_MESSAGE_MANAGER_HPP

#include <unordered_map>
#include <iostream>
#include <vector>
#include <string>
#include <mutex>

#define MAX_MESSAGES 5

class GroupMsgManager
{
public:
    std::vector<std::string> getMessages(std::string groupId);

    void addMessage(const std::string &groupId, const std::string &message);
    std::unordered_map<std::string, int> getAllMessagesCount() const;
    int getMessageCount(std::string groupId) const;

private:
    // a map to track all the stored messages, we map the groupId to the vector af all the stored messages
    std::unordered_map<std::string, std::vector<std::string>> groupMessages;
    mutable std::mutex groupMutex;
};

#endif
