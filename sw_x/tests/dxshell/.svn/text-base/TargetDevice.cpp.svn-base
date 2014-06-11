#ifdef WIN32
#define _CRT_SECURE_NO_WARNINGS // for cross-platform getenv
#endif

#include "TargetDevice.hpp"
#include "TargetLocalDevice.hpp"
#include "TargetRemoteDevice.hpp"
#include "common_utils.hpp"

#include <log.h>

#ifdef LINUX
#include <stdlib.h>
#endif

TargetDevice::TargetDevice()
{
}

TargetDevice::~TargetDevice()
{
}

TargetDevice *getTargetDevice()
{
    if (getenv("DX_REMOTE_IP") == NULL)
        return new TargetLocalDevice();
    else
        return new TargetRemoteDevice();
}

bool isIOS(const std::string &os)
{
    if (os.find("iPhone") != std::string::npos ||
        os.find("iPad") != std::string::npos || 
        os.find("iPod") != std::string::npos) {
        return true;
    }
    return false;
}
