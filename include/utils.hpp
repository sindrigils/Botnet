#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <iomanip>   // For std::setw and std::setfill
#include <sstream>   // For std::stringstream
#include <algorithm> // For std::find
#include <vector>
#include <sys/socket.h> // For getsockname
#include <arpa/inet.h>  // For inet_ntop

#include "constants.hpp"

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

// A safe function to convert a string to an integer
int stringToInt(const std::string &str);

// A function to check if the groupId is valid, meaning it starts with either A5_ or Instr_.
// So we dont connect to the Orcale and Number since we can receive their messages from others, when
// other people forward it to us
bool validateGroupId(std::string groupId, bool allowEmpty = false);
std::string formatGroupMessage(std::string message);

#endif