#include "DxRemoteQueryDeviceAgent.hpp"
#include <vplex_plat.h>
#include "dx_remote_agent.pb.h"
#include "log.h"

DxRemoteQueryDeviceAgent::DxRemoteQueryDeviceAgent(VPLSocket_t skt, char *buf, uint32_t pktSize) : IDxRemoteAgent(skt, buf, pktSize)
{
}

DxRemoteQueryDeviceAgent::~DxRemoteQueryDeviceAgent()
{
}

int DxRemoteQueryDeviceAgent::doAction()
{
    igware::dxshell::QueryDeviceOutput output;
    output.set_device_name(GetDeviceName());
    output.set_device_class(GetDeviceClass());
    output.set_os_version(GetOSVersion());
    output.set_is_acer_device(GetIsAcer());
    output.set_device_has_camera(GetHasCamera());

    LOG_INFO("Serialize Response for device_name: %s", output.device_name().c_str());
    LOG_INFO("Serialize Response for device_class: %s", output.device_class().c_str());
    LOG_INFO("Serialize Response for os_version: %s", output.os_version().c_str());
    LOG_INFO("Serialize Response for is_acer_device: %s", (output.is_acer_device() ? "true" : "false"));
    LOG_INFO("Serialize Response for device_has_camera: %s", (output.device_has_camera() ? "true" : "false"));

    response = output.SerializeAsString();
    return 0;
}

std::string DxRemoteQueryDeviceAgent::GetDeviceName()
{
    std::string strDevName = std::string();
    char hostName[256];
    int rc;

#ifdef VPL_PLAT_IS_WINRT
    auto hostnames = Windows::Networking::Connectivity::NetworkInformation::GetHostNames();
    if (hostnames->Size == 0) {
        return strDevName;
    }

    std::wstring wHostName = std::wstring(hostnames->GetAt(0)->DisplayName->Begin(), hostnames->GetAt(0)->DisplayName->End());
    _VPL__wstring_to_utf8(wHostName.c_str(), wHostName.size(), hostName, 256);

    strDevName = std::string(hostName);
#else
    rc = VPL_GetLocalHostname(hostName, sizeof hostName);
    if (rc == VPL_OK) {
        strDevName = std::string(hostName);
    }
#endif

    return strDevName;
}
