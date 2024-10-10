#ifndef UTILS_HPP
#define UTILS_HPP
#include <string>
#include <iomanip>   // For std::setw and std::setfill
#include <sstream>   // For std::stringstream
#include <algorithm> // For std::find
#include <vector>

#define SOH 0x01
#define EOT 0x04
#define ESC 0x10 // Escape character for byte-stuffing

// Converts a string to its hexadecimal representation
std::string stringToHex(const std::string &input);

// Strips quotes from a string
std::string stripQuotes(const std::string &str);

// Adds SOH and EOT to the message and performs byte-stuffing
std::string constructServerMessage(const std::string &content);

// Finds the index of a byte in a buffer
int findByteIndexInBuffer(const char *buffer, int bufferLength, int start, char sByte);

// Extracts a message from the buffer between start and end
std::string extractMessage(const char *buffer, int start, int end);
std::string trim(const std::string &str);
std::vector<std::string> splitMessageOnDelimiter(const char *buffer, char delimiter = ',');

#endif