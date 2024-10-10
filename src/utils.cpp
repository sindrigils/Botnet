#include "utils.hpp"

#include <iomanip> // For std::setw and std::setfill
#include <sstream> // For std::stringstream 

// Function to convert a string to its hexadecimal representation
std::string stringToHex(const std::string &input)
{
    std::stringstream ss;
    for (char c : input)
    {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)(unsigned char)c;
    }
    return ss.str();
}

// Function to strip quotes from a string
std::string stripQuotes(const std::string &str)
{
    if (str.front() == '"' && str.back() == '"')
    {
        return str.substr(1, str.size() - 2); // Remove the first and last quotes
    }
    return str;
}