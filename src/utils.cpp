#include "utils.hpp"

#include <iomanip> // For std::setw and std::setfill
#include <sstream> // For std::stringstream

std::string stringToHex(const std::string &input)
{
    std::stringstream ss;
    for (char c : input)
    {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)(unsigned char)c;
    }
    return ss.str();
}

std::string stripQuotes(const std::string &str)
{
    if (str.front() == '"' && str.back() == '"')
    {
        return str.substr(1, str.size() - 2); // Remove the first and last quotes
    }
    return str;
}

std::string constructServerMessage(const std::string &content)
{
    std::string stuffedContent;

    // Byte-stuffing: Escape SOH (0x01) and EOT (0x04) in the content
    for (char c : content)
    {
        if (c == SOH || c == EOT)
        {
            stuffedContent += ESC;
        }
        stuffedContent += c;
    }

    std::string finalMessage;
    finalMessage += SOH;
    finalMessage += stuffedContent;
    finalMessage += EOT;

    return finalMessage;
}

std::string trim(const std::string &str)
{
    if (str.empty())
    {
        return str;
    }

    size_t first = str.find_first_not_of(" \t\n\r");

    // If no non-whitespace characters are found, return an empty string
    if (first == std::string::npos)
    {
        return "";
    }

    size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, (last - first + 1));
}

std::vector<std::string> splitMessageOnDelimiter(const char *buffer, char delimiter)
{
    std::vector<std::string> tokens;
    std::string token;
    std::stringstream ss(buffer);
    while (std::getline(ss, token, delimiter))
    {
        tokens.push_back(trim(token));
    }
    return tokens;
}