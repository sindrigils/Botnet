#include "servers.hpp"

Server::Server(int socket, const std::string &ipAddress, const std::string &port, const std::string &name)
    : sock(socket), ipAddress(ipAddress), port(port)
{
}