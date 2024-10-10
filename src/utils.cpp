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

int findByteIndexInBuffer(const char* buffer, int bufferLength, int start, char sByte) {
    int index = std::find(buffer + start, buffer + bufferLength, sByte) - buffer;
    return (index < bufferLength) ? index : -1; // Return -1 if byte is not found
}

std::string extractMessage(const char* buffer, int start, int end) {
    return std::string(buffer + start, end - start);
}