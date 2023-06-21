
#include <functional>
#include <limits>
#include <exception>
#include <assert.h>
#include <opencv2/videoio.hpp>

#include "3rdparty/easyloggingpp/easylogging++.h"
#include "core/DataPoint.h"
#include "core/Ocr.h"


namespace c2matica {

int32_t const Ocr::DEFAULT_RECONNECT_INTERVAL = 1000; // ms

Ocr::Ocr(Stoppable* parent, std::string streamURL, int32_t reconnectInterval)
    : Stoppable(parent)
    , _streamURL(streamURL)
    , _cap(NULL)
    , _reconnectInterval(reconnectInterval)
{
    if (_reconnectInterval <= 0)
    {
        _reconnectInterval = DEFAULT_RECONNECT_INTERVAL;
    }
}

Ocr::~Ocr()
{
    stop();
}

bool Ocr::start()
{
    // if (_dpMap.size() <= 0)
    //     return false;

    if (!Stoppable::start())
    {
        LOG(WARNING) << _streamURL << " ocr has allready started";
        return false;
    }

    LOG(INFO) << _streamURL << " ocr started";

    _cap = std::make_shared<cv::VideoCapture>();

    calcOutFPS();

    if (!waitConnected())
    {
        stop();
        return false;
    }

    _runTimer.run(std::bind(&Ocr::run, this));

    onStart();
    return true;
}

void Ocr::stop()
{
    if (!isStart() || isStop())
        return;

    // recursive stop childrens(datapoint)
    onStop();

    LOG(INFO) << _streamURL << " ocr stoping";
    _connectTimer.stop();
    _runTimer.stop();
    _cap->release();
    
    stopped();
    Stoppable::stop();
    LOG(INFO) << _streamURL << " ocr stopped";
}

bool Ocr::addDataPoint(std::shared_ptr<DataPoint> dataPoint)
{
    std::lock_guard<std::recursive_mutex> l(_mutexDP);
    LOG(INFO) << _streamURL << " add datapoint " << dataPoint->getID();
    if (auto rc = _dpMap.emplace(dataPoint->getID(), dataPoint); rc.second)
    {
        if (isStart())
        {
            calcOutFPS();
            if (_opened.load())
            {
                setFrameInterval();
                dataPoint->start();
            }
        }
        return true;
    }
    return false;
}

bool Ocr::delDataPoint(std::string const& id)
{
    std::lock_guard<std::recursive_mutex> l(_mutexDP);
    LOG(INFO) << _streamURL << " delete datapoint " << id;
    if (auto iter = _dpMap.find(id); iter != _dpMap.end())
    {
        _dpMap.erase(iter);
        if (isStart())
        {
            calcOutFPS();
            if (_opened.load())
            {
                setFrameInterval();
            }
        }
        return true;
    }

    return false;
}

void Ocr::modDataPoint(std::shared_ptr<DataPoint> newDP)
{
    std::lock_guard<std::recursive_mutex> l(_mutexDP);
    auto iter = _dpMap.find(newDP->getID());
    if (iter == _dpMap.end())
    {
        LOG(WARNING) << _streamURL << " modify datapoint "
            << newDP->getID() << " not found";
        return;
    }

    auto oldDP = iter->second;
    uint32_t newPollingInterval = newDP->getPollingInterval();
    auto newCoordinate = newDP->getCoordinate();

    if (oldDP->getPollingInterval() != newPollingInterval)
    {
        LOG(INFO) << _streamURL << " modify datapoint " << oldDP->getID()
            << " polling interval to " << newPollingInterval;
        oldDP->setPollingInterval(newPollingInterval);
        if (isStart())
        {
            calcOutFPS();
            if (_opened.load())
            {
                setFrameInterval();
            }
        }
    }

    if (oldDP->getCoordinate() != newCoordinate)
    {
        LOG(INFO) << _streamURL << " modify datapoint " << oldDP->getID()
            << " coordinate to"
            << " x:" << std::get<0>(newCoordinate)
            << " y:" << std::get<1>(newCoordinate)
            << " width:" << std::get<2>(newCoordinate)
            << " height:" << std::get<3>(newCoordinate);
        oldDP->setCoordinate(newCoordinate);
    }
}

std::shared_ptr<DataPoint> Ocr::getDataPoint(std::string const& id)
{
    std::lock_guard<std::recursive_mutex> l(_mutexDP);
    if (auto iter = _dpMap.find(id); iter != _dpMap.end())
    {
        return iter->second;
    }
    return NULL;
}

bool Ocr::waitConnected()
{
    LOG(INFO) << _streamURL << " _opened=" << _opened.load();
    if (_opened.load())
        return true;

    LOG(INFO) << "_reconnectInterval=" << _reconnectInterval;

    LOG(INFO) << "start connect timer";
    _connectTimer.start(
        _reconnectInterval,
        true,
        std::bind(&Ocr::connect, this, std::placeholders::_1));

    {
        std::unique_lock<std::mutex> lck(_mutex);
        _cv.wait(lck, [&]() { return _opened.load() || isStop(); });
    }

    _connectTimer.stop();
    
    return _opened.load();
}

void Ocr::connect(std::chrono::time_point<std::chrono::system_clock> const& tp)
{
    (void)tp;
    LOG(INFO) << _streamURL << " connecting...";

    bool opened = _cap->open(_streamURL, cv::CAP_FFMPEG/*CAP_ANY*/);
    if (opened && _cap->isOpened())
    {
        LOG(INFO) << _streamURL << " open video stream success";
        _cap->set(cv::CAP_PROP_BUFFERSIZE, 0);
        // TODO check inPFS realtime
        _inFPS = _cap->get(cv::CAP_PROP_FPS);
        setFrameInterval();

        _opened.store(true);
        _cv.notify_all();
    }
    else
    {
        LOG(ERROR) << _streamURL << " unable to open video stream";
        _opened.store(false);
    }
}

void Ocr::run()
{
    static int pos = 0;
    std::vector<int> readyIndex;
    cv::Mat currentFrame;

    auto onError = [this]() {
        _opened.store(false);
        putFrame({});
        waitConnected();
    };
    
    try
    {
        if (_cap->grab())
        {
            if ((pos++ % getFrameInterval()) == 0 &&
                _cap->retrieve(currentFrame))
                putFrame(currentFrame);
        }
        else
        {
            LOG(WARNING) << _streamURL << " blank frame grabbed";
            onError();
        }
    }
    catch (std::exception& e)
    {
        LOG(ERROR) << _streamURL << " running excetion: " << e.what();
        onError();
    }
}

void Ocr::putFrame(cv::Mat const& frame)
{
    std::unique_lock<std::shared_mutex> l(_mutexFrame);
    _frame = frame;
}

bool Ocr::getFrame(cv::Mat& frame)
{
    std::shared_lock<std::shared_mutex> l(_mutexFrame);
    if (_frame.empty())
        return false;

    // frame = _frame.clone();
    _frame.copyTo(frame);
    return true;
}

void Ocr::calcOutFPS()
{
    uint32_t minPollingInterval = std::numeric_limits<std::uint32_t>::max();

    std::lock_guard<std::recursive_mutex> l(_mutexDP);
    for (auto const& [id, dp] : _dpMap)
    {
        (void)id;
        uint32_t dpPollingInterval = dp->getPollingInterval();
        minPollingInterval = minPollingInterval > dpPollingInterval
            ? dpPollingInterval
            : minPollingInterval;
    }

    _outFPS = std::numeric_limits<std::uint32_t>::max() == minPollingInterval
                  ? 0
                  : (double)1000 / minPollingInterval;

    LOG(INFO) << _streamURL << " calc out fps " << _outFPS;
}

void Ocr::setFrameInterval()
{
    std::lock_guard<std::recursive_mutex> l(_mutexDP);
    _frameInterval = _outFPS == 0
        ? std::numeric_limits<int>::max()
        : _inFPS / _outFPS;
    if (0 == _frameInterval)
        _frameInterval = 1;
    LOG(INFO) << _streamURL << " in fps " << _inFPS
        << ", out fps " << _outFPS
        << ", out frame interval " << _frameInterval;
}

int Ocr::getFrameInterval()
{
    std::lock_guard<std::recursive_mutex> l(_mutexDP);
    return _frameInterval;
}

// -----------------------------------------------------------------------

std::unique_ptr<Ocr> makeOcr(
    Stoppable* parent,
    std::string streamURL,
    int32_t reconnectInterval = Ocr::DEFAULT_RECONNECT_INTERVAL)
{
    return std::make_unique<Ocr>(parent, streamURL, reconnectInterval);
}

}



