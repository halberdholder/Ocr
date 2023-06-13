
#include "core/Stoppable.h"

namespace c2matica {

Stoppable::Stoppable(Stoppable* parent)
    : _parent(parent)
    , _start(false)
    , _stopping(false)
    , _stopped(false)
{
    if (_parent)
    {
        _parent->addChild(this);
    }
}

Stoppable::~Stoppable()
{
    if (_parent)
    {
        _parent->delChild(this);
    }
}

bool Stoppable::start()
{
    std::unique_lock<std::mutex> l(_mutex);

    if (_start)
        return false;

    _start = true;
    _stopping = false;
    _stopped = false;

    return true;
}

void Stoppable::stop()
{
    std::unique_lock<std::mutex> l(_mutex);

    if (!_start || _stopping)
        return;

    _stopping = true;
    _cvStopping.notify_one();

    _cvStopped.wait(l, [this]() { return _stopped; });
    _start = false;
    _stopping = false;
    _stopped = false;
}

void Stoppable::onStart()
{
    std::unique_lock<std::mutex> l(_mutex);

    for (auto child : _childrens)
    {
        child->start();
    }
}

void Stoppable::onStop()
{
    std::unique_lock<std::mutex> l(_mutex);

    for (auto child : _childrens)
    {
        child->stop();
    }
}

bool Stoppable::isStart()
{
    std::unique_lock<std::mutex> l(_mutex);
    return _start;
}

bool Stoppable::isStop()
{
    std::unique_lock<std::mutex> l(_mutex);
    return _stopping;
}

bool Stoppable::waitUntilStop(
    std::chrono::time_point<std::chrono::steady_clock> tp)
{
    std::unique_lock<std::mutex> lck(_mutex);
    return _cvStopping.wait_until(lck, tp, [this]() { return _stopping; });
}

void Stoppable::stopped()
{
    std::unique_lock<std::mutex> l(_mutex);
    _stopped = true;
    _cvStopped.notify_one();
}

void Stoppable::addChild(Stoppable* child)
{
    std::unique_lock<std::mutex> l(_mutex);
    _childrens.push_back(child);
}

void Stoppable::delChild(Stoppable* child)
{
    std::unique_lock<std::mutex> l(_mutex);
    for (auto iter = _childrens.begin(); iter != _childrens.end(); iter++)
    {
        if (*iter == child)
        {
            _childrens.erase(iter);
            break;
        }
    }
}

}