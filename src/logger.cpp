#include "logger.hpp"

void Logger::write(const std::string message, const char *buffer, size_t bufferLen)
{
    std::lock_guard<std::mutex> guard(logMutex);
    std::ofstream logFile("logs/logs.txt", std::ios::app);
    if (!logFile.is_open())
    {
        std::cerr << "Unable to open log file" << std::endl;
        return;
    }

    // Get the current time
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    std::tm now_tm = *std::localtime(&now_time);

    // Format the time
    std::ostringstream timeStream;
    timeStream << std::put_time(&now_tm, "%Y-%m-%d %H:%M:%S");

    // Write to the log file
    logFile << "[" << timeStream.str() << "] " << message << ": " << std::string(buffer, bufferLen) << std::endl;

    logFile.close();
}