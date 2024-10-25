#include "utils.hpp"

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
    if (str.empty())
    {
        return str;
    }

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

int findSohEotIndexInBuffer(const char *buffer, int bufferLength, int startIndex, char byteType)
{
    for (int i = startIndex; i < bufferLength; i++)
    {
        if (buffer[i] == ESC)
        {
            i++;
            continue;
        }
        if (buffer[i] == byteType)
        {
            return i;
        }
    }
    return -1;
}

std::string extractCommand(const char *buffer, int start, int end)
{
    return std::string(buffer + start, end - start);
}

std::vector<std::string> extractCommands(const char *buffer, int bufferLength)
{
    std::vector<std::string> messageVector;

    for (int i = 0, start, end; i < bufferLength; i = end + 1)
    {
        if ((start = findSohEotIndexInBuffer(buffer, bufferLength, i, SOH)) < 0)
        {
            break;
        }
        if ((end = findSohEotIndexInBuffer(buffer, bufferLength, start + 1, EOT)) < 0)
        {
            break;
        }
        if (start + 1 == end)
        {
            break;
        }
        messageVector.push_back(extractCommand(buffer, start + 1, end));
    }
    return messageVector;
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

int stringToInt(const std::string &str)
{
    try
    {
        size_t pos;
        int value = std::stoi(str, &pos);

        if (pos < str.length())
        {
            return -1;
        }

        return value;
    }
    catch (...)
    {
        return -1;
    }
}

bool validateGroupId(std::string groupId, bool allowEmpty)
{
    if (groupId.rfind("A5_", 0) == 0 || groupId.rfind("Instr_", 0) == 0)
    {
        return true;
    }
    return groupId == "" && allowEmpty ? true : false;
}