
#ifndef _C2MATICA_CONFIG_H_
#define _C2MATICA_CONFIG_H_

#include <string>
#include <optional>
#include <vector>

#include "3rdparty/nlohmann/json.hpp"
#include "core/Ocr.h"
#include "core/DataPoint.h"

using json = nlohmann::json;

namespace c2matica {

class Config {
public:
    Config(std::string base = Config::BASE_PATH);
    ~Config() = default;

public:
    struct ProtocolConfig
    {
        std::string streamURL;
        bool saveOneImage;
    };

    struct DataPointConfig
    {
        std::string dpId;
        uint32_t pollingInterval;
        struct CoordinateDetail {
            uint32_t width;
            uint32_t height;
            uint32_t x;
            uint32_t y;

            NLOHMANN_DEFINE_TYPE_INTRUSIVE(
                DataPointConfig::CoordinateDetail, width, height, x, y);
        } coordinateDetail;

        bool equalTo(DataPointConfig const& other) const
        {
            return pollingInterval == other.pollingInterval &&
                coordinateDetail.x == other.coordinateDetail.x && 
                coordinateDetail.y == other.coordinateDetail.y &&
                coordinateDetail.width == other.coordinateDetail.width &&
                coordinateDetail.height == other.coordinateDetail.height;
        }
    };
    bool load();

public:
    static const std::string BASE_PATH;
    static const std::string TESSDATA_DIR;
    static const std::string PROTOCOL_CONFIG_FILE;
    static const std::string DATAPOINT_CONFIG_FILE;

public:
    const int32_t RECONNECT_INTERVAL = Ocr::DEFAULT_RECONNECT_INTERVAL;
    const std::string LANGUAGE = "eng+chi_sim";

    std::string basePath;
    std::string tessdataPath;
    std::string protocolConfigFile;
    std::string datapointConfigFile;

public:
    struct ProtocolConfig mProtocolConfig;
    std::vector<struct DataPointConfig> vDataPointConfig;

public:
    bool loadDPCOnfig();

private:
    bool loadAppConfig();
};

}

#endif
