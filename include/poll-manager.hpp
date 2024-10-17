#ifndef POLL_MANAGER_HPP
#define POLL_MANAGER_HPP

#include <poll.h>
#include <string>
#include <iostream>
#include <mutex>
#define MAX_CONNECTIONS 10 // i think
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

    PollManager();

private:
    mutable std::mutex fdMutex;
};

#endif