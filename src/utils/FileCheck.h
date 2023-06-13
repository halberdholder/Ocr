#ifndef _C2MATICA_FILECHECK_H_
#define _C2MATICA_FILECHECK_H_

#include <ctime>
#include <chrono>
#include <string>
#include <memory>

namespace c2matica {

class FileCheck
{
public:
    FileCheck(const std::string& file);
    ~FileCheck() = default;

public:
    typedef std::chrono::system_clock Clock;
    typedef std::chrono::duration<long long, std::micro> Duration;
    typedef std::chrono::time_point<Clock, Duration> Time;

    template <typename FromDuration>
    Time
    time_cast(std::chrono::time_point<Clock, FromDuration> const &tp) const
    {
        return std::chrono::time_point_cast<Duration, Clock>(tp);
    }

    Time now() const
    {
        return time_cast(Clock::now());
    }

    Time from_time_t(time_t t_time) const
    {
        return time_cast(Clock::from_time_t(t_time));
    }

public:
    struct FileInfo
    {
        Time mtime;
        off_t size;
    };

public:
    bool checkForFileModification();

private:
    void updateLastModInfo(FileInfo const& fi);
    bool getFileInfo(FileInfo& fi) const;

private:
    FileInfo _lastFileInfo;
    std::string _file;
};

std::unique_ptr<FileCheck> makeFileCheck(const std::string &file);

}
 
#endif