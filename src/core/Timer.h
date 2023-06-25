
#ifndef _C2MATICA_TIMER_H_
#define _C2MATICA_TIMER_H_

#include <functional>
#include <chrono>
#include <thread>
#include <shared_mutex>

#include "core/Stoppable.h"

namespace c2matica {

class Timer : public Stoppable
{
public:
    Timer(Stoppable* parent = NULL)
        : Stoppable(parent) {}
    ~Timer();

public:
    using system_clock = std::chrono::system_clock;
    using steady_clock = std::chrono::steady_clock;
    typedef std::chrono::time_point<system_clock> system_time;
    typedef std::chrono::time_point<steady_clock> steady_time;

public:
    // start task every interval milliseconds
    void start(
        int interval,
        bool immediately,
        std::function<void(system_time const &tp)> task);
    // run stask no interval
    void run(std::function<void()> task);
    void stop() override;

    void setInterval(int interval)
    {
        std::unique_lock<std::shared_mutex> l(_mtx);
        _interval = interval;
    }
    int getInterval()
    {
        std::shared_lock<std::shared_mutex> l(_mtx);
        return _interval;
    }

private:
    std::shared_mutex _mtx;
    int _interval;
};

}

#endif
