#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <fstream>
#include <iostream>
#include <string>
#include <mutex>

#include <ctime>
#include <iomanip>
#include <sstream>


class Logger
{
public:
    std::string filePath;
    void write(const std::string message, const char *buffer, size_t bufferLen, bool printToConsole = false);
    void write(const std::string message, bool printToConsole = false);

private:
    std::mutex logMutex;
    std::string _getTime();
    int _openLogFile(std::ofstream &logFile);
};

#endif