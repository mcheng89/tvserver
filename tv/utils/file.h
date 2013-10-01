#ifndef TV_UTILS_FILE_H
#define TV_UTILS_FILE_H

#include <string>
#include <vector>

namespace TV {
namespace Utils {

    class File {
    public:
        static std::string getPathSeparator();
        static std::string getAbsolutePath(std::string file);
        static bool exists(std::string file);
        static long lastModified(std::string file);
        static std::vector<std::string> ls(std::string dir);

    private:
        File(const File&);
        File& operator = (const File&);
    };

}}

#endif // TV_UTILS_FILE_H
