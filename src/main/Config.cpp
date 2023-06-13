
#include <iostream>
#include <fstream>

#include "3rdparty/easyloggingpp/easylogging++.h"
#include "main/Config.h"

namespace c2matica {

const std::string Config::BASE_PATH = "./";
const std::string Config::TESSDATA_DIR = "tessdata";
const std::string Config::PROTOCOL_CONFIG_FILE = "protocolConfig";
const std::string Config::DATAPOINT_CONFIG_FILE = "dpConfig";

Config::Config(std::string base)
{
    basePath = base;
    if (basePath.length() <= 0)
        basePath = BASE_PATH;

    if (basePath.back() != '/')
    {
        basePath.push_back('/');
    }
    
    protocolConfigFile = basePath + PROTOCOL_CONFIG_FILE;
    datapointConfigFile = basePath + DATAPOINT_CONFIG_FILE;
    tessdataPath = basePath + TESSDATA_DIR;
}

bool Config::load()
{
    LOG(INFO) << "basePath=" << basePath;
    LOG(INFO) << "protocolConfigFile=" << protocolConfigFile;
    LOG(INFO) << "datapointConfigFile=" << datapointConfigFile;
    LOG(INFO) << "tessdataPath=" << tessdataPath;
 
    return loadAppConfig() && loadDPCOnfig();
}

bool Config::loadAppConfig()
{
    json j;
    try {
        std::ifstream i(protocolConfigFile);
        i >> j;
        i.close();
    }
    catch (std::exception& e)
    {
        LOG(ERROR) << "open protocolConfig file `" << protocolConfigFile
            << "' failed: " << e.what();
        return false;
    }

    LOG(INFO) << "protocolConfig file `" << protocolConfigFile
        << "' open success";

    try
    {
        json protocolConfig = j["protocolConfig"];
        for (std::size_t i = 0; i < protocolConfig.size(); i++)
        {
            mProtocolConfig = protocolConfig[i].get<ProtocolConfig>();
            if (mProtocolConfig.category == "streamURL")
            {
                break;
            }
        }
    }
    catch (std::exception& e)
    {
        LOG(ERROR) << "parse protocolConfig file failed: " << e.what();
        return false;
    }

    return true;
}

bool Config::loadDPCOnfig()
{
    json j;
    try {
        std::ifstream i(datapointConfigFile);
        i >> j;
        i.close();
    }
    catch (std::exception& e)
    {
        LOG(ERROR) << "open dpConfig file `" << datapointConfigFile
            <<  "' failed: " << e.what();
        return false;
    }

    LOG(INFO) << "dpConfig file `" << datapointConfigFile << "' open success";

    try {
        std::optional<int> posDPID;
        std::optional<int> posPollingInterval;
        std::optional<int> posCoordinateDetail;
        json header = j[0];
        for (std::size_t i = 0; i < header.size(); i++)
        {
            if (header[i] == "dpId")
                posDPID = i;
            else if (header[i] == "pollingInterval")
                posPollingInterval = i;
            else if (header[i] == "coordinateDetail")
                posCoordinateDetail = i;
        }
        if (!posDPID || !posPollingInterval || !posCoordinateDetail)
        {
            LOG(ERROR) << "dpConfig missing column";
            return false;
        }

        std::vector<struct DataPointConfig> tmp;

        for (std::size_t i = 1; i < j.size(); i++)
        {
            struct DataPointConfig dataPointConfig;
            dataPointConfig.dpId = j[i][*posDPID];
            dataPointConfig.pollingInterval =
                stoi(j[i][*posPollingInterval].get<std::string>());

            json coordinateDetail = 
                json::parse(j[i][*posCoordinateDetail].get<std::string>());
            dataPointConfig.coordinateDetail = coordinateDetail
                .get<DataPointConfig::CoordinateDetail>();

            tmp.push_back(dataPointConfig);
        };

        vDataPointConfig = std::move(tmp);
    }
    catch (std::exception& e)
    {
        LOG(ERROR) << "parse dpConfig file failed: " << e.what();
        return false;
    }

    return true;
}

}
