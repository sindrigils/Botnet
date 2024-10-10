#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <fstream>
#include <iostream>
#include <string>

extern std::mutex logMutex;

class Logger
{
public:
    std::string filePath;
    void write(const std::string message, const char *buffer);

private:
};

#endif