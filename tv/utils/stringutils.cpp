#include "stringutils.h"
#include <sstream>
#include <algorithm>

namespace TV {
namespace Utils {

size_t StringUtils::findIgnoreCase(std::string s1, std::string s2) {
    std::string s1_copy = toUpper(s1), s2_copy = toUpper(s2);
    return s1_copy.find(s2_copy);
}

int StringUtils::hexToInt(std::string str) {
    std::istringstream ss( str );
    int n;
    ss >> std::hex >> n;
    return n;
}
std::string StringUtils::intToHex(int dec) {
    std::stringstream ss;
    ss << std::hex << dec;
    return ss.str();
}

bool StringUtils::isNumeric(std::string str)
{
    std::string::const_iterator it = str.begin();
    while (it != str.end() && std::isdigit(*it)) ++it;
    return !str.empty() && it == str.end();
}

void StringUtils::replaceAll(std::string& str, const std::string& from, const std::string& to) {
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
    }
}

std::vector<std::string> StringUtils::split(const std::string& s, const std::string& delim, const bool keep_empty) {
    std::vector<std::string> result;
    if (delim.empty()) {
        result.push_back(s);
        return result;
    }
    std::string::const_iterator substart = s.begin(), subend;
    while (true) {
        subend = search(substart, s.end(), delim.begin(), delim.end());
        std::string temp(substart, subend);
        if (keep_empty || !temp.empty()) {
            result.push_back(temp);
        }
        if (subend == s.end()) {
            break;
        }
        substart = subend + delim.size();
    }
    return result;
}

std::string StringUtils::toUpper(const std::string & s)
{
    std::string ret(s.size(), char());
    for(unsigned int i = 0; i < s.size(); ++i)
        ret[i] = (s[i] <= 'z' && s[i] >= 'a') ? s[i]-('a'-'A') : s[i];
    return ret;
}

std::string StringUtils::toString(int value) {
    std::stringstream ss;
    ss << value;
    return ss.str();
}

}}

