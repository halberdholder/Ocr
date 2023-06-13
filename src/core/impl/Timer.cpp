
#include "3rdparty/easyloggingpp/easylogging++.h"
#include "core/Timer.h"

namespace c2matica {

Timer::~Timer()
{
    stop();
}

void Timer::start(
    int interval,
    bool immediately,
    std::function<void(
        std::chrono::time_point<std::chrono::system_clock> const& tp)> task)
{
    if (!Stoppable::start())
        return;

    setInterval(interval);

    auto now = std::chrono::steady_clock::now();
    if (immediately)
        now -= std::chrono::milliseconds(getInterval());

    std::thread([this, task](
        std::chrono::time_point<std::chrono::steady_clock> tp) {
        while (!isStop())
        {
            LOG(TRACE) << "on timer timepoint="
                << tp.time_since_epoch().count()
                << ", interval=" << getInterval();
            if (!waitUntilStop(tp + std::chrono::milliseconds(getInterval())))
            {
                tp = std::chrono::steady_clock::now();
                task(std::chrono::system_clock::now());
            }
        }
        stopped();
    }, now).detach();
}

void Timer::run(std::function<void()> task)
{
    if (!Stoppable::start())
        return;
    
    std::thread([this, task]() {
        while (!isStop())
        {
            task();
        }
        stopped();
    }).detach();
}

void Timer::stop()
{
    Stoppable::stop();
}

}
