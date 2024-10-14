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
    void addMessage(const std::string &groupId, const std::string &message);
    std::vector<std::string> getMessages(const std::string &groupId);
    int getMessageCount(const std::string &groupId) const;
    std::unordered_map<std::string, int> getAllMessagesCount() const;

private:
    std::unordered_map<std::string, std::vector<std::string>> groupMessages;
    mutable std::mutex mutex;
};

#endif
