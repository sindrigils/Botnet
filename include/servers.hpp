#ifndef SERVERS_HPP
#define SERVERS_HPP
#include <iostream>

class Server
{
public:
    int sock;
    std::string ipAddress;
    std::string port;
    std::string name;

    Server(int socket, const std::string &ipAddress, const std::string &port = "-1", const std::string &name = "");
};
#endif