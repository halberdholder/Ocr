
#ifndef _C2MATICA_DATAPOINT_H_
#define _C2MATICA_DATAPOINT_H_

#include <tuple>
#include <shared_mutex>
#include <tesseract/baseapi.h>

#include "core/Stoppable.h"
#include "core/Timer.h"
#include "core/Ocr.h"

namespace c2matica {

class DataPoint : public Stoppable
{
public:
    static const uint32_t DEFAULT_POOLING_INTERVAL;

public:
    DataPoint() = delete;
    DataPoint(
        Stoppable* parent,
        std::string id,
        std::string dataPath,
        std::string language,
        Ocr* ocr);
    ~DataPoint();

    std::string getID() const { return _id; }

    void setCoordinate(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
    {
        std::unique_lock<std::shared_mutex> l(_mutex);
        _coordinate = std::make_tuple(x, y, width, height);
    }
    void setCoordinate(
        std::tuple<uint32_t, uint32_t, uint32_t, uint32_t> const& coordinate)
    {
        std::unique_lock<std::shared_mutex> l(_mutex);
        _coordinate = coordinate;
    }
    void setPollingInterval(uint32_t poolingInterval);
    auto getCoordinate()
        -> std::tuple<uint32_t, uint32_t, uint32_t, uint32_t>&
    {
        std::shared_lock<std::shared_mutex> l(_mutex);
        return _coordinate;
    }
    uint32_t getPollingInterval()
    {
        std::shared_lock<std::shared_mutex> l(_mutex);
        return _pollingInterval;
    }

    bool start() override;
    void stop() override;

private:
    const std::string _id;
    const std::string _dataPath; // tessdata path
    const std::string _language; // language in tessdata

    // x, y, width, height
    std::tuple<uint32_t, uint32_t, uint32_t, uint32_t> _coordinate;
    uint32_t _pollingInterval;
    std::shared_mutex _mutex;

    Ocr* _ocr;
    std::recursive_mutex _rMutex;
    tesseract::TessBaseAPI* _api;
    Timer _timer;

    bool initTessApi();
    void releaseTessApi();

    void run(std::chrono::time_point<std::chrono::system_clock> const& tp);
};

std::shared_ptr<DataPoint> makeDataPoint(
    Stoppable* parent,
    std::string id,
    std::string dataPath,
    std::string language,
    Ocr* ocr);

}

#endif
