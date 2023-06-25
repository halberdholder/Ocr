
#ifndef _C2MATICA_APPLICATION_H_
#define _C2MATICA_APPLICATION_H_

#include <chrono>
#include <condition_variable>

#include "main/Config.h"
#include "core/Ocr.h"
#include "core/Timer.h"
#include "utils/FileCheck.h"

namespace c2matica {

class Application
{
public:
    static const std::uint32_t DEFAULT_CHECK_DP_CONFIG_FILE_INTERVAL = 1000;

public:
    Application(std::unique_ptr<Config> config);
    ~Application() = default;

    void initLog();
    bool loadConfig();
    void setup();
    bool run();
    void stop();

private:
    std::unique_ptr<Config> _config;
    std::unique_ptr<Ocr> _ocr;

    std::unique_ptr<FileCheck> _dpConfigFileCheck;
    std::uint32_t _checkDPConfigFileInterval;
    Timer _checkDPConfigTimer;

    // stop cv
    std::condition_variable _cv;

    void checkDPConfig(Timer::system_time const &tp);
};

std::unique_ptr<Application>
makeApplication(std::unique_ptr<Config> config);

}

#endif
