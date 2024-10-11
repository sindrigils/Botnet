#include "logger.hpp"

void Logger::write(const std::string message, const char *buffer, size_t bufferLen)
{
    std::lock_guard<std::mutex> guard(logMutex);
    std::ofstream logFile("logs/logs.txt", std::ios::app);
    if (!logFile.is_open())
    {
        std::cerr << "Unable to open log file" << std::endl;
    }

    logFile << "[" << __DATE__ << " " << __TIME__ << "]" << " " << message << ": " << std::string(buffer, bufferLen) << std::endl;

    logFile.close();
}
