#include "logger.hpp"

std::string Logger::_getTimeFormat()
{
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    std::tm now_tm = *std::localtime(&now_time);

    std::ostringstream timeStream;
    timeStream << std::put_time(&now_tm, "%Y-%m-%d %H:%M:%S");

    return "[" + timeStream.str() + "] ";
}

int Logger::_openLogFile(std::ofstream &file, const std::string logFile)
{
    file.open("logs/" + logFile, std::ios::app | std::ios::out);
    if (!file.is_open())
    {
        perror("Error opening log file");
        return -1;
    }
    return 0;
}

void Logger::_write(const std::string message, const std::string logFile)
{
    std::lock_guard<std::mutex> guard(logMutex);
    std::ofstream file;
    if (_openLogFile(file, logFile) != -1)
    {
        file << this->_getTimeFormat() << message << std::endl;
        file.close();
    }
}

void Logger::write(const std::string message, const char *buffer, size_t bufferLen, bool printToConsole, const std::string logFile)
{
    std::string logMessage = message + ": " + std::string(buffer, bufferLen);
    this->_write(logMessage, logFile);
    if (printToConsole)
    {
        std::cout << this->_getTimeFormat() << message << std::endl;
    }
}

void Logger::write(const std::string message, bool printToConsole, const std::string logFile)
{
    std::string logMessage = message;
    this->_write(logMessage, logFile);
    if (printToConsole)
    {
        std::cout << logMessage << std::endl;
    }
}

