#ifndef POLL_MANAGER_HPP
#define POLL_MANAGER_HPP

#include <poll.h>
#include <string>
#include <iostream>
#include <mutex>
#define MAX_CONNECTIONS 10 // +1 for listning sock, +1 for client sock, and then +8 for other servers
#define POLL_TIMEOUT 100   // 100ms

class PollManager
{
public:
    // Array to hold pollfd structures
    struct pollfd pollfds[MAX_CONNECTIONS];
    int nfds;

    int hasData(int i) const;
    int getFd(int i) const;
    void close(int sock);
    void add(int sock);
    int getPollCount();
    bool isFull();

    PollManager();

private:
    mutable std::mutex fdMutex;
};

#endif