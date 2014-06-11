#include "dx_remote_agent.h"
#include "DxRemoteQueryDeviceAgent.hpp"
#include <string>
#include <algorithm>

std::string DxRemoteQueryDeviceAgent::GetDeviceClass()
{
    //return std::string("iOS Machine");
    NSString *nsDeviceClass = [[UIDevice currentDevice] model];
    char szDeviceClass[256] = "";
    strncpy(szDeviceClass, [nsDeviceClass UTF8String], 256);
    std::string strDeviceClass(szDeviceClass);
    return strDeviceClass;
}

std::string DxRemoteQueryDeviceAgent::GetOSVersion()
{
    //return std::string("iOS");
    NSString *nsSystemName = [[UIDevice currentDevice] systemName];
    NSString *nsSystemVersion = [[UIDevice currentDevice] systemVersion];
    char szSystemName[256];
    char szSystemVersion[256];
    strncpy(szSystemName, [nsSystemName UTF8String], 256);
    strncpy(szSystemVersion, [nsSystemVersion UTF8String], 256);
    std::string strOSVersion(szSystemName);
    strOSVersion.append(" ");
    strOSVersion.append(szSystemVersion);
    return strOSVersion;
}

bool DxRemoteQueryDeviceAgent::GetIsAcer()
{
    return false;
}

bool DxRemoteQueryDeviceAgent::GetHasCamera()
{
    return true;
}
