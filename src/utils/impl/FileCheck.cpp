
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "utils/FileCheck.h"
#include "3rdparty/easyloggingpp/easylogging++.h"

namespace c2matica {

FileCheck::FileCheck(const std::string& file)
    : _file(file)
{
    _lastFileInfo.mtime = time_cast(Clock::now());
    _lastFileInfo.size = 0;
}
 
bool FileCheck::checkForFileModification()
{
    FileInfo fi;

    if (!getFileInfo(fi))
    {
        return false;
    }

    bool modified = fi.mtime > _lastFileInfo.mtime ||
        fi.size != _lastFileInfo.size;

    if (modified)
        updateLastModInfo(fi);

    return modified;
}

void FileCheck::updateLastModInfo(FileInfo const& fi)
{
    _lastFileInfo.mtime = fi.mtime;
    _lastFileInfo.size = fi.size;
}

bool FileCheck::getFileInfo(FileInfo& fi) const
{
    struct stat fileStatus;
    if (stat(_file.c_str (), &fileStatus) == -1)
    {
        LOG(ERROR) << "getFileInfo " << _file << " failed";
        return false;
    }

    fi.mtime = from_time_t(fileStatus.st_mtime);
    fi.size = fileStatus.st_size;

    return true;
}

// -----------------------------------------------------------------------

std::unique_ptr<FileCheck> makeFileCheck(const std::string &file)
{
    return std::make_unique<FileCheck>(file);
}

}