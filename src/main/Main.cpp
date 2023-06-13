
#include "3rdparty/easyloggingpp/easylogging++.h"
#include "main/Application.h"
#include "main/Config.h"
#include "utils/Sustain.h"

INITIALIZE_EASYLOGGINGPP

using namespace c2matica;

int main(int argc, char **argv)
{
    std::unique_ptr<Config> config = (2 == argc)
        ? std::make_unique<Config>(argv[1])
        : std::make_unique<Config>();

    std::unique_ptr<Application> app = makeApplication(std::move(config));

    app->initLog();

    if (!app->loadConfig())
    {
        return -1;
    }

    app->setup();

    if (haveSustain())
    {
        auto const ret = doSustain();
        if (!ret.empty())
            LOG(ERROR) << "watchdog: " << ret;
    }

    if (!app->run())
    {
        return -1;
    }

    waitStop();

    LOG(INFO) << "received shutdown request";
    app->stop();
    LOG(INFO) << "done.";

    stopSustain();

    return 0;
}
