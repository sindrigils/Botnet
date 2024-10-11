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

int findByteIndexInBuffer(const char *buffer, int bufferLength, int start, char sByte)
{
    int index = std::find(buffer + start, buffer + bufferLength, sByte) - buffer;
    return (index < bufferLength) ? index : -1; // Return -1 if byte is not found
}

std::string extractMessage(const char *buffer, int start, int end)
{
    return std::string(buffer + start, end - start);
}

std::vector<std::string> extractMessages(const char *buffer, int bufferLength)
{
    std::vector<std::string> messageVector;
    for (int i = 0, start, end; i < bufferLength; i = end + 1)
    {
        if ((start = findByteIndexInBuffer(buffer, bufferLength, i, SOH)) < 0)
        {
            break;
        }
        if ((end = findByteIndexInBuffer(buffer, bufferLength, start + 1, EOT)) < 0)
        {
            break;
        }
        messageVector.push_back(extractMessage(buffer, start + 1, end));
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

std::string getOwnIPFromSocket(int sock)
{
    struct sockaddr_in own_addr;

    socklen_t own_addr_len = sizeof(own_addr);

    if (getsockname(sock, (struct sockaddr *)&own_addr, &own_addr_len) < 0)
    {
        perror("can't get own IP address from socket");
        exit(1);
    }

    char own_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &own_addr.sin_addr, own_ip, INET_ADDRSTRLEN);

    return std::string(own_ip);
}

int connectToServer(const std::string &ip, int port, std::string myGroupId)
{
    std::string strPort = std::to_string(port);
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &serverAddr.sin_addr);
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (connect(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == 0)
    {
        std::string message = "HELO," + myGroupId;
        std::string heloMessage = constructServerMessage(message);
        send(serverSocket, heloMessage.c_str(), heloMessage.length(), 0);
        return serverSocket;
    }
    return -1;
}