#ifndef GROUP_MESSAGE_MANAGER_HPP
#define GROUP_MESSAGE_MANAGER_HPP

#include <unordered_map>
#include <iostream>
#include <vector>
#include <string>
#include <mutex>

#include "constants.hpp"

// now this numberr is debatable, since we dont want other servers to be able to spamm us with
// random messages for other groups, so we limit the number of messages we keep for each server
// and even for our own clinet. To prevent the client from not getting his messsages, he gets
// a notifaction that he has some unread messages.
#define MAX_MESSAGES 20

class GroupMsgManager
{
public:
    std::vector<std::string> getMessages(std::string groupId);

    void addMessage(const std::string &groupId, const std::string &message);
    void addClientMessage(std::string message);
    std::vector<std::string> getAllClientMessages();
    std::unordered_map<std::string, int> getAllMessagesCount(bool showClientMsgs = false) const;
    int getMessageCount(std::string groupId) const;
    int getAllClientMessagesCount() const;

private:
    // a map to track all the stored messages, we map the groupId to the vector af all the stored messages
    std::unordered_map<std::string, std::vector<std::string>> groupMessages;
    // a vector to store all the messages meant for the client, we dont store his messages in groupMessages since thats for others groups
    // and those group messages are advertised in the STATUSRESP
    std::vector<std::string> clientMessages;
    mutable std::mutex groupMutex;
    mutable std::mutex clientMutex;
};

#endif
