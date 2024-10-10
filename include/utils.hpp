#include <string>

#define SOH 0x01
#define EOT 0x04
#define ESC 0x10 // Escape character for byte-stuffing

// Function to convert a string to its hexadecimal representation
std::string stringToHex(const std::string &input);

// Function to strip quotes from a string
std::string stripQuotes(const std::string &str);

// Adds SOH and EOT to the message and performs byte-stuffing
std::string constructServerMessage(const std::string &content);