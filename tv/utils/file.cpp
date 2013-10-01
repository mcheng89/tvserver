#include "file.h"
#include <stdio.h>
#include <windows.h>
#include <sys/stat.h>

namespace TV {
namespace Utils {

std::string File::getPathSeparator() {
    return "\\";
}

std::string File::getAbsolutePath(std::string file) {
    char fullpath[MAX_PATH];
    if (_fullpath(fullpath, file.c_str(), MAX_PATH ) != NULL)
        return fullpath;
    return "";
}

bool File::exists(std::string file) {
    FILE *fp = fopen(file.c_str(), "r");
    if (fp == NULL) return false;
    fclose(fp);
    return true;
}

long File::lastModified(std::string file) {
    struct stat buf;
    if(stat(file.c_str(),&buf ) == -1)
        return 0;
    return buf.st_mtime;
}

std::vector<std::string> File::ls(std::string dir) {
    std::vector<std::string> fileList;
    std::string findDir = dir + "\\*.*";
    WIN32_FIND_DATA fd;
    HANDLE hFind = FindFirstFile(findDir.c_str(), &fd);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            std::string findFilename = fd.cFileName;
            if (findFilename.compare(".") != 0 && findFilename.compare("..") != 0) {
                fileList.push_back(findFilename);
            }
        } while(FindNextFile(hFind,&fd) != FALSE);
    }
    FindClose(hFind);
    return fileList;
}

}}
