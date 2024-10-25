#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <iomanip>   // For std::setw and std::setfill
#include <sstream>   // For std::stringstream
#include <algorithm> // For std::find
#include <vector>
#include <sys/socket.h> // For getsockname
#include <arpa/inet.h>  // For inet_ntop

#define SOH 0x01
#define EOT 0x04
#define ESC 0x1B

#define MAX_EOT_TRIES 10
#define Backlog 5
#define MAX_MSG_LENGTH 3 * 5000
#define MY_GROUP_ID "A5_5"
#define CLIENT_PW "kaladin"

// Converts a string to its hexadecimal representation
std::string stringToHex(const std::string &input);

// Strips quotes from a string
std::string stripQuotes(const std::string &str);

// Adds SOH and EOT to the message and performs byte-stuffing
std::string constructServerMessage(const std::string &content);

// Finds the index of a byte in a buffer
int findSohEotIndexInBuffer(const char *buffer, int bufferLength, int start, char sByte);

// Extracts a message from the buffer between start and end
std::string extractCommand(const char *buffer, int start, int end);

// Extracts messages from a buffer and adds them to a vector
std::vector<std::string> extractCommands(const char *buffer, int bufferLength);

// Trim leading and trailing whitespace from a string
std::string trim(const std::string &str);

// Split a message on a delimiter
std::vector<std::string> splitMessageOnDelimiter(const char *buffer, char delimiter = ',');

int stringToInt(const std::string &str);
bool validateGroupId(std::string groupId, bool allowEmpty = false);

#endif