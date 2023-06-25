
#include <ctime>

#include "utils/Common.h"

namespace c2matica {

std::string timeFormatNow()
{
    std::time_t now = std::time(NULL);
    char buf[20];
    std::strftime(buf, sizeof(buf), "%Y%m%d%H%M%S", std::localtime(&now));
    return std::string(buf);
}

}