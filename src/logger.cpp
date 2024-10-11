#include "logger.hpp"

std::string Logger::_getTime()
{
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    std::tm now_tm = *std::localtime(&now_time);

    std::ostringstream timeStream;
    timeStream << std::put_time(&now_tm, "%Y-%m-%d %H:%M:%S");

    return timeStream.str();
}

int Logger::_openLogFile(std::ofstream &logFile)
{
    std::lock_guard<std::mutex> guard(logMutex);
    logFile.open("logs/logs.txt", std::ios::app);
    if (!logFile.is_open())
    {
        std::cerr << "Unable to open log file" << std::endl;
        return -1;
    }
    return 0;
}

void Logger::write(const std::string message, const char *buffer, size_t bufferLen, bool printToConsole)
{
    std::ofstream logFile;
    if(_openLogFile(logFile) != -1){
        logFile << "[" << _getTime() << "] " << message << ": " << std::string(buffer, bufferLen) << std::endl;
        logFile.close();
    }

    if(printToConsole){
        std::cout << "[" << _getTime() << "] " << message << std::endl;
    }
}

void Logger::write(const std::string message, bool printToConsole){
    std::ofstream logFile;
    std::string logMessage = "[" + _getTime() + "] " + message;
    if(_openLogFile(logFile) != -1){
        logFile << logMessage << std::endl;
        logFile.close();
    }

    if(printToConsole){
        std::cout << logMessage << std::endl;
    }
}