#ifndef TV_UTILS_STRINGUTILS_H
#define TV_UTILS_STRINGUTILS_H

#include <vector>
#include <string>
#include <windows.h>

namespace TV {
namespace Utils {

class StringUtils {
public:
    static size_t findIgnoreCase(std::string s1, std::string s2);

    static int hexToInt(std::string str);
    static std::string intToHex(int dec);

    static bool isNumeric(std::string str);

    static void replaceAll(std::string& str, const std::string& from, const std::string& to);

    static std::vector<std::string> split(const std::string& s, const std::string& delim, const bool keep_empty = true);

    static std::string toUpper(const std::string & s);

    static std::string toString(int value);
};

}}

#endif // TV_UTILS_STRINGUTILS_H
