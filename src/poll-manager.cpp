#include "poll-manager.hpp"

PollManager::PollManager() : nfds(0)
{
    // Initialize all pollfd entries to -1 (invalid)
    for (int i = 0; i < MAX_CONNECTIONS; ++i)
    {
        pollfds[i].fd = -1;
        pollfds[i].events = 0;
        pollfds[i].revents = 0;
    }
}

void PollManager::add(int sock)
{
    std::lock_guard<std::mutex> lock(fdMutex);

    if (nfds >= MAX_CONNECTIONS)
    {
        return;
    }

    pollfds[nfds].fd = sock;       // Set the fd as the connection sock
    pollfds[nfds].events = POLLIN; // We want to monitor for incoming connections
    nfds++;                        // Increment the number of fds
}

void PollManager::close(int sock)
{
    std::lock_guard<std::mutex> lock(fdMutex);
    for (int i = 0; i < nfds; ++i)
    {
        if (pollfds[i].fd == sock)
        {
            // Shift the remaining fds down to fill the gap
            for (int j = i; j < nfds - 1; ++j)
            {
                pollfds[j] = pollfds[j + 1];
            }

            // Set the last fd to -1 to indicate it's invalid
            pollfds[nfds - 1].fd = -1;
            nfds--; // Decrement the count of active fds
            break;  // Exit the loop after removing the socket
        }
    }
}

int PollManager::getFd(int i) const
{
    std::lock_guard<std::mutex> lock(fdMutex);
    return pollfds[i].fd;
}

int PollManager::getPollCount()
{
    // Call poll() to wait for events on sockets
    return poll(pollfds, nfds, POLL_TIMEOUT);
}

int PollManager::hasData(int i) const
{
    std::lock_guard<std::mutex> lock(fdMutex);
    if (i >= nfds)
    {
        return -1;
    }
    return pollfds[i].revents & POLLIN;
}

bool PollManager::isFull()
{
    std::lock_guard<std::mutex> lock(fdMutex);
    return nfds == MAX_CONNECTIONS;
}