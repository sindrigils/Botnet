#ifndef POLL_MANAGER_HPP
#define POLL_MANAGER_HPP

#include <poll.h>
#include <string>
#include <iostream>
#include <mutex>
#define MAX_CONNECTIONS 10 // i think
#define POLL_TIMEOUT 50    // 50ms

class PollManager
{
public:
    int nfds;
    struct pollfd pollfds[MAX_CONNECTIONS]; // Array to hold pollfd structures

    void add(int sock);
    void close(int sock);
    int getPollCount();
    int getFd(int i) const;
    int hasData(int i) const;
    PollManager(); // Constructor to initialize the poll manager

private:
    mutable std::mutex fdMutex;
};

#endif