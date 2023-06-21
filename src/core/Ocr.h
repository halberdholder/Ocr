
#ifndef _C2MATICA_OCR_H_
#define _C2MATICA_OCR_H_

#include <mutex>
#include <atomic>
#include <thread>
#include <mutex>
#include <unordered_map>
#include <condition_variable>
#include <opencv2/videoio.hpp>

#include "core/Stoppable.h"
#include "core/Timer.h"

namespace c2matica {

class DataPoint;

class Ocr : public Stoppable
{
public:
    static int32_t const DEFAULT_RECONNECT_INTERVAL;

public:
    Ocr() = delete;
    Ocr(Stoppable* parent,
        std::string streamURL,
        int32_t reconnectInterval = DEFAULT_RECONNECT_INTERVAL);
    ~Ocr();

    bool addDataPoint(std::shared_ptr<DataPoint> dataPoint);
    bool delDataPoint(std::string const& id);
    void modDataPoint(std::shared_ptr<DataPoint> newDP);
    std::shared_ptr<DataPoint> getDataPoint(std::string const& id);

    // Start grab and recoginize
    bool start() override;

    // Stop grab and release resource
    void stop() override;

    bool getFrame(cv::Mat& frame);

private:
    const std::string _streamURL;

    std::shared_ptr<cv::VideoCapture> _cap;
    
    std::recursive_mutex _mutexDP;
    std::unordered_map<std::string, std::shared_ptr<DataPoint>> _dpMap;
    double _inFPS;
    double _outFPS;
    int _frameInterval;

    std::shared_mutex _mutexFrame;
    cv::Mat _frame;

    int32_t _reconnectInterval;

    std::mutex _mutex;
    std::atomic<bool> _opened{ false };
    std::condition_variable _cv;

    Timer _connectTimer;
    Timer _runTimer;

private:
    bool waitConnected();
    void connect(Timer::system_time const& tp);
    void run();
    void putFrame(cv::Mat const& Frame);
    void calcOutFPS();
    void setFrameInterval();
    int getFrameInterval();
};


std::unique_ptr<Ocr> makeOcr(
    Stoppable* parent,
    std::string streamURL,
    int32_t reconnectInterval);

}

#endif
