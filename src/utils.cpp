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