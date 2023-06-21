
#include <iostream>
#include <functional>
#include <chrono>
#include <exception>

#include "3rdparty/easyloggingpp/easylogging++.h"
#include "3rdparty/nlohmann/json.hpp"
#include "core/DataPoint.h"

using json = nlohmann::json;

namespace c2matica {

const uint32_t DataPoint::DEFAULT_POOLING_INTERVAL = 1000; // ms

DataPoint::DataPoint(
    Stoppable* parent,
    std::string id,
    std::string dataPath,
    std::string language,
    Ocr* ocr)
    : Stoppable(parent)
    , _id(id)
    , _dataPath(dataPath)
    , _language(language)
    , _ocr(ocr)
    , _api(NULL)
{
    _coordinate = std::make_tuple(0, 0, 0, 0);
    _pollingInterval = DEFAULT_POOLING_INTERVAL;
}

DataPoint::~DataPoint()
{
    stop();
}

bool DataPoint::start()
{
    if (!Stoppable::start())
    {
        LOG(WARNING) << _id << " datapoint has allready started";
        return false;
    }

    LOG(INFO) << _id << " datapoint started";

    onStart();
    if (!initTessApi())
    {
        stop();
        return false;
    }
    _timer.start(_pollingInterval, true,
        std::bind(&DataPoint::run, this, std::placeholders::_1));
    return true;
}

void DataPoint::stop()
{
    if (!isStart() || isStop())
        return;

    // recursive stop childrens
    onStop();

    LOG(INFO) << _id << " datapoint stoping";
    _timer.stop();
    releaseTessApi();
    stopped();
    Stoppable::stop();
    LOG(INFO) << _id << " datapoint stopped";
}

bool DataPoint::initTessApi()
{
    std::lock_guard<std::recursive_mutex> l(_rMutex);
    releaseTessApi();
    _api = new tesseract::TessBaseAPI();
    if (-1 == _api->Init(_dataPath.c_str(), _language.c_str()))
    {
        LOG(ERROR) << "tesseract API init failed";
        delete _api;
        _api = NULL;
        return false;
    }
    return true;
}

void DataPoint::releaseTessApi()
{
    std::lock_guard<std::recursive_mutex> l(_rMutex);
    if (_api)
    {
        _api->End();
        delete _api;
        _api = NULL;
    }
}

void DataPoint::run(
    std::chrono::time_point<std::chrono::system_clock> const& tp)
{
    cv::Mat frame;

    if (!_ocr->getFrame(frame) || frame.empty())
    {
        LOG(ERROR) << _id << " blank frame grabbed";
        return;
    }
    auto coordinate = getCoordinate();
    uint32_t x = std::get<0>(coordinate);
    uint32_t y = std::get<1>(coordinate);
    uint32_t width = std::get<2>(coordinate);
    uint32_t height = std::get<3>(coordinate);

    if (width > 0 && height > 0)
    {
        cv::Rect rect(x, y, width, height);
        frame = frame(rect);
    }

    _api->SetImage(
        (uchar*)frame.data,
        frame.size().width,
        frame.size().height,
        frame.channels(),
        frame.step1());
    // char* out = _api->GetUNLVText();
    char* out = _api->GetUTF8Text();
    std::string stringOut = std::string(out);
    if (stringOut.length() > 0 && stringOut.back() == '\n')
    {
        stringOut.erase(stringOut.find_last_not_of("\n") + 1);
    }

    try
    {
        json j;
        j["dpId"] = _id;
        j["time"] = std::chrono::duration_cast<std::chrono::milliseconds>(
            tp.time_since_epoch()).count();
        j["value"] = stringOut;
        std::cout << j << std::endl;
    }
    catch (std::exception& e)
    {
        LOG(ERROR) << _id << " parse out exception: " << e.what();
    }
    
    delete[] out;
}

void DataPoint::setPollingInterval(uint32_t poolingInterval)
{
    if (poolingInterval == 0)
    {
        LOG(WARNING) << _id << " cannot set polling interval to 0";
        return;
    }

    std::unique_lock<std::shared_mutex> l(_mutex);
    if (_pollingInterval == poolingInterval)
        return;

    _pollingInterval = poolingInterval;

    if (_timer.isStart())
    {
        _timer.setInterval(_pollingInterval);
    }
}

// -----------------------------------------------------------------------

std::shared_ptr<DataPoint> makeDataPoint(
    Stoppable* parent,
    std::string id,
    std::string dataPath,
    std::string language,
    Ocr* ocr)
{
    return std::make_shared<DataPoint>(parent, id, dataPath, language, ocr);
}

}
