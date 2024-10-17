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

    // Write to log file and append the buffer to the log message
    void write(const std::string message, const char *buffer, size_t bufferLen, bool printToConsole = false, const std::string logFile = "logs.txt");
    // Write to log file a standard message
    void write(const std::string message, bool printToConsole = false, const std::string logFile = "logs.txt");
    void _write(const std::string message, const std::string logFile);

private:
    std::mutex logMutex;
    int _openLogFile(std::ofstream &file, const std::string logFile);
    std::string _getTimeFormat();
};

#endif