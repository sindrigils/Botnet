#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <fstream>
#include <iostream>
#include <string>
#include <mutex>

class Logger
{
public:
    std::string filePath;
    void write(const std::string message, const char *buffer, size_t bufferLen);

private:
    std::mutex logMutex;
};

#endif