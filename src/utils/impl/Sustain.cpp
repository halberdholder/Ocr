
#ifdef __linux__
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/prctl.h>
#include <unistd.h>
#endif
#ifdef __FreeBSD__
#include <sys/types.h>
#include <sys/wait.h>
#endif

#include <string>
#include <functional>
#include <condition_variable>
#include <mutex>
#include "utils/Sustain.h"

namespace c2matica {

#ifdef __unix__

static auto const sleepBeforeWaiting = 2;
static auto const sleepBetweenWaits = 1;

static pid_t pManager = static_cast<pid_t> (0);
static pid_t pChild = static_cast<pid_t> (0);

std::condition_variable _cv;

void waitStop()
{
    std::mutex mtx;
    std::unique_lock<std::mutex> lck(mtx);
    _cv.wait(lck);
}

static void stop_child(int)
{
    _cv.notify_one();
}

static void pass_signal(int a)
{
    kill(pChild, a);
}

static void stop_manager(int)
{
    kill(pChild, SIGINT);
    _exit(0);
}

bool haveSustain()
{
    return true;
}

std::string stopSustain()
{
    if (getppid() != pManager)
        return "";

    kill(pManager, SIGHUP);
    return "Terminating monitor";
}

static bool checkChild(pid_t pid, int options)
{
    int i;

    if (waitpid(pChild, &i, options) == -1)
        return false;

    return kill(pChild, 0) == 0;
}

std::string doSustain()
{
    pManager = getpid();
    signal(SIGINT, stop_manager);
    signal(SIGHUP, stop_manager);
    signal(SIGUSR1, pass_signal);
    signal(SIGUSR2, pass_signal);

    prctl(PR_SET_PDEATHSIG, SIGUSR1);

    // Number of times the child has exited in less than
    // 5 seconds.
    int fastExit = 0;

    for (auto childCount = 1; ; ++childCount)
    {
        pChild = fork();

        if (pChild == -1)
            _exit(0);

        auto cc = std::to_string(childCount);
        if (pChild == 0)
        {
            signal(SIGINT, SIG_DFL);
            signal(SIGHUP, SIG_DFL);
            signal(SIGUSR1, stop_child);
            signal(SIGUSR2, SIG_DFL);
            prctl(PR_SET_PDEATHSIG, SIGUSR1);
            return "Launching child " + cc;
        }

        sleep(sleepBeforeWaiting);

        // If the child has already terminated count this
        // as a fast exit and an indication that something
        // went wrong:
        if (!checkChild(pChild, WNOHANG))
        {
            if (++fastExit == 3)
                _exit(0);
        }
        else
        {
            fastExit = 0;

            while (checkChild(pChild, 0))
                sleep(sleepBetweenWaits);

            (void)rename("core",
                ("core." + std::to_string(pChild)).c_str());
        }
    }
}


#else

bool haveSustain()
{
    return false;
}

std::string doSustain()
{
    return "";
}

std::string stopSustain()
{
    return "";
}

void installPDeathSig(void(*f)(int))
{
    return;
}

#endif

}
