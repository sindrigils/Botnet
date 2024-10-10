#include "logger.hpp"

std::mutex logMutex;

void Logger::write(const std::string message, const char *buffer)
{
    std::lock_guard<std::mutex> guard(logMutex);
    std::ofstream logFile("logs/logs.txt", std::ios::app);
    if (!logFile.is_open())
    {
        std::cerr << "Unable to open log file" << std::endl;
    }

    logFile << "[" << __DATE__ << " " << __TIME__ << "]" << " " << message << ": " << buffer << std::endl;

    logFile.close();
}
