
#ifndef _C2MATICA_STOPPABLE_H_
#define _C2MATICA_STOPPABLE_H_

#include <vector>
#include <mutex>
#include <condition_variable>

namespace c2matica {

class Stoppable
{
public:
    Stoppable() = delete;
    Stoppable(Stoppable* parent);
    ~Stoppable();

    virtual bool start();
    virtual void stop();

    void onStart();
    void onStop();

    bool isStart();
    bool isStop();

protected:
    bool waitUntilStop(std::chrono::time_point<std::chrono::steady_clock> tp);
    void stopped();

private:
    Stoppable* _parent;
    std::vector<Stoppable*> _childrens;

    std::mutex _mutex;
    bool _start;
    bool _stopping;
    bool _stopped;
    std::condition_variable _cvStopping;
    std::condition_variable _cvStopped;

    void addChild(Stoppable* child);
};

}

#endif
