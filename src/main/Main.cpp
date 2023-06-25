
#include <cstdlib>
#include <iostream>
#include <getopt.h>

#include "3rdparty/easyloggingpp/easylogging++.h"
#include "main/Application.h"
#include "main/Config.h"
#include "utils/Sustain.h"

INITIALIZE_EASYLOGGINGPP

using namespace c2matica;

int parseArgs(int argc, char **argv);

int main(int argc, char **argv)
{
    parseArgs(argc, argv);

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

void printHelp()
{
    std::cout << R"(
DESCRIPTION
    Captrue RTSP video stream by OpenCV, and recognize image frame by tesseract.

SYNOPSIS
    Usage
        ocr [BASEDIR] [OPTIONS...]

    Examples
        ./ocr
        ocr /path/to/base/dir/
        LD_LIBRARY_PATH=/path/to/libs ocr /path/to/base/dir/
        ./ocr --version

BASEDIR
    Base directory include the tessdata directory, and protocolConfig, dpConfig files.
    libs directory in base directory include the shared libraries related to image codec.
    Also you can specify the libraries directory with LD_LIBRARY_PATH environment variables.
    The tree command with BASEDIR maybe show as below:

    ├── dpConfig
    ├── libs
    │   ├── libavcodec.so.58
    │   ├── libavformat.so.58
    │   ├── libavutil.so.56
    │   ├── libopenjp2.so.7
    │   └── libswscale.so.5
    ├── ocr
    ├── protocolConfig
    └── tessdata
        ├── chi_sim.traineddata
        ├── chi_sim_vert.traineddata
        └── eng.traineddata

OPTIONS
    -h, --help
        Display this usage.
    -v, --version
        Display version string.
)" << std::endl;
}

void printVersion()
{
#define VERSION_STRING "v1.0.1"
    std::cout << VERSION_STRING << std::endl;
}

int parseArgs(int argc, char **argv)
{
    static const char *shortOptions = "hv";
    static const struct option longOtions[] = {
        { "help", no_argument, NULL, 'h' },
        { "version", no_argument, NULL, 'v' },
        { NULL, 0, NULL, 0}
    };

    int c;
    int optionIndex = 0;
    while (true)
    {
        c = getopt_long(argc, argv, shortOptions, longOtions, &optionIndex);
        if (c == -1)
            break;

        switch (c)
        {
            case 'h':
                printHelp();
                exit(EXIT_SUCCESS);
                break;
            case 'v':
                printVersion();
                exit(EXIT_SUCCESS);
                break;
            case '?':
            default:
                break;
        }
    }
    return true;
}
