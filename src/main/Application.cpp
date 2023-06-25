
#include <string>
#include <mutex>

#include "3rdparty/easyloggingpp/easylogging++.h"
#include "core/DataPoint.h"
#include "main/Application.h"

// using namespace std::string_literals;

namespace c2matica {

Application::Application(std::unique_ptr<Config> config)
    : _config(std::move(config))
{
    _checkDPConfigFileInterval = DEFAULT_CHECK_DP_CONFIG_FILE_INTERVAL;
}

void Application::initLog()
{
    el::Configurations defaultConf;
    defaultConf.setToDefault();
/*
	std::string configStr = R"delimter(
*GLOBAL:
    ENABLED             = true
    TO_FILE             = true
    TO_STANDARD_OUTPUT  = false
    FORMAT              = "%datetime %level %msg"
    FILENAME            = "logs/log_%datetime{%Y%M%d%H}.log"
    SUBSECOND_PRECISION = 3
    MAX_LOG_FILE_SIZE   = 10485760
*DEBUG
    ##ENABLED           = false
*TRACE
    ##ENABLED           = false
)delimter";
    defaultConf.parseFromText(configStr.c_str());
*/
    defaultConf.setGlobally(el::ConfigurationType::Enabled, "true");
    defaultConf.setGlobally(el::ConfigurationType::ToFile, "true");
    defaultConf.setGlobally(el::ConfigurationType::ToStandardOutput, "false");
    defaultConf.setGlobally(el::ConfigurationType::Format,
       "%datetime %level %msg");
    defaultConf.setGlobally(el::ConfigurationType::Filename,
       _config->basePath + "logs/log_%datetime{%Y%M%d%H}.log");
    defaultConf.setGlobally(el::ConfigurationType::MillisecondsWidth, "3");
    defaultConf.setGlobally(el::ConfigurationType::MaxLogFileSize, "10485760");
#ifndef DEBUG_LOG
    defaultConf.set(el::Level::Debug, el::ConfigurationType::Enabled, "false");
#endif
#ifndef TRACE_LOG
    defaultConf.set(el::Level::Trace, el::ConfigurationType::Enabled, "false");
#endif

    el::Loggers::reconfigureLogger("default", defaultConf);
}

bool Application::loadConfig()
{
    return _config->load();
}

void Application::setup()
{
    _ocr = std::move(makeOcr(
        NULL,
        _config->mProtocolConfig.streamURL,
        _config->mProtocolConfig.saveOneImage
            ? _config->basePath
            : "",
        _config->RECONNECT_INTERVAL));

    for (auto const& dpConfig : _config->vDataPointConfig)
    {
        std::shared_ptr<DataPoint> dp = 
            makeDataPoint(
                _ocr.get(),
                dpConfig.dpId,
                _config->tessdataPath,
                _config->LANGUAGE,
                _ocr.get());
        dp->setPollingInterval(dpConfig.pollingInterval);
        dp->setCoordinate(
            dpConfig.coordinateDetail.x,
            dpConfig.coordinateDetail.y,
            dpConfig.coordinateDetail.width,
            dpConfig.coordinateDetail.height);

        // keep shared_ptr
        _ocr->addDataPoint(dp);
    }

    _dpConfigFileCheck = std::move(
        makeFileCheck(_config->datapointConfigFile));
}

bool Application::run()
{
    if (_ocr->start())
    {
        _checkDPConfigTimer.start(
            _checkDPConfigFileInterval,
            false,
            std::bind(&Application::checkDPConfig, this, std::placeholders::_1));
        return true;
    }

    return false;
}

void Application::checkDPConfig(Timer::system_time const& tp)
{
    (void)tp;
    bool modified = _dpConfigFileCheck->checkForFileModification();
    if (!modified)
        return;
    
    LOG(INFO) << "datapoint config file modified";
    auto oldDPConfigs = _config->vDataPointConfig;
    if (!_config->loadDPCOnfig())
    {
        LOG(ERROR) << "check datapoint config file failed";
        return;
    }

    auto newDPConfigs = _config->vDataPointConfig;

    // check delete and modify
    for (auto const& oldDPConfig : oldDPConfigs)
    {
        bool found = [&]() -> bool {
            for (auto const& newDPConfig : newDPConfigs)
            {
                if (newDPConfig.dpId == oldDPConfig.dpId)
                {
                    if (newDPConfig.equalTo(oldDPConfig))
                        return true;

                    std::shared_ptr<DataPoint> newDP =
                        makeDataPoint(
                            NULL,
                            newDPConfig.dpId,
                            _config->tessdataPath,
                            _config->LANGUAGE,
                            NULL);
                    newDP->setPollingInterval(newDPConfig.pollingInterval);
                    newDP->setCoordinate(
                        newDPConfig.coordinateDetail.x,
                        newDPConfig.coordinateDetail.y,
                        newDPConfig.coordinateDetail.width,
                        newDPConfig.coordinateDetail.height);

                    _ocr->modDataPoint(newDP);

                    return true;
                }
            }
            return false;
        }();

        if (!found)
        {
            _ocr->delDataPoint(oldDPConfig.dpId);
        }
    }

    // check add
    for (auto const& newDPConfig : newDPConfigs)
    {
        bool found = [&]() ->bool {
            for (auto const& oldDPConfig : oldDPConfigs)
            {
                if (oldDPConfig.dpId == newDPConfig.dpId)
                    return true;
            }
            return false;
        }();

        if (!found)
        {
            std::shared_ptr<DataPoint> newDP =
                makeDataPoint(
                    _ocr.get(),
                    newDPConfig.dpId,
                    _config->tessdataPath,
                    _config->LANGUAGE,
                    _ocr.get());
            newDP->setPollingInterval(newDPConfig.pollingInterval);
            newDP->setCoordinate(
                newDPConfig.coordinateDetail.x,
                newDPConfig.coordinateDetail.y,
                newDPConfig.coordinateDetail.width,
                newDPConfig.coordinateDetail.height);

            _ocr->addDataPoint(newDP);
        }
    }
}

void Application::stop()
{
    _checkDPConfigTimer.stop();
    _ocr->stop();
}

// -----------------------------------------------------------------------

std::unique_ptr<Application>
makeApplication(std::unique_ptr<Config> config)
{
    return std::make_unique<Application>(std::move(config));
}

}
