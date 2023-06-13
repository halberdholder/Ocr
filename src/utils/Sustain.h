
#ifndef _C2MATICA_SUSTAIN_H_
#define _C2MATICA_SUSTAIN_H_

#include <functional>

namespace c2matica {

// "Sustain" is a system for a buddy process that monitors the main process
// and relaunches it on a fault.
bool haveSustain();
std::string stopSustain();
std::string doSustain();

void waitStop();

}

#endif