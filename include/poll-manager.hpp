#ifndef POLL_MANAGER_HPP
#define POLL_MANAGER_HPP

#include <poll.h>
#include <string>
#include <iostream>
#define MAX_CONNECTIONS 8 // i think
#define POLL_TIMEOUT 50   // 50ms

class PollManager
{
public:
    int nfds;
    struct pollfd pollfds[MAX_CONNECTIONS]; // Array to hold pollfd structures

    void add(int sock);
    void close(int sock);
    int getFd(int i);
    int getPollCount();
    int hasData(int i);
    PollManager(); // Constructor to initialize the poll manager

private:
};

#endif