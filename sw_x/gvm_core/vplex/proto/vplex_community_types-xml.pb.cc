// GENERATED FROM vplex_community_types.proto, DO NOT EDIT

#include "vplex_community_types-xml.pb.h"

#include <string.h>

#include <ProtoXmlParseStateImpl.h>


void
vplex::community::RegisterHandlers_vplex_community_types(ProtoXmlParseStateImpl* impl)
{
}

vplex::community::OnlineState_t
vplex::community::parseOnlineState_t(const std::string& data, ProtoXmlParseStateImpl* state)
{
    vplex::community::OnlineState_t out;
    if (data.compare("OFFLINE") == 0) {
        out = vplex::community::OFFLINE;
    } else if (data.compare("AVAILABLE") == 0) {
        out = vplex::community::AVAILABLE;
    } else if (data.compare("AWAY") == 0) {
        out = vplex::community::AWAY;
    } else if (data.compare("IDLE") == 0) {
        out = vplex::community::IDLE;
    } else if (data.compare("INVISIBLE") == 0) {
        out = vplex::community::INVISIBLE;
    } else {
        state->error = "Invalid vplex.community.OnlineState_t: ";
        state->error.append(data);
        out = vplex::community::OFFLINE;
    }
    return out;
}

const char*
vplex::community::writeOnlineState_t(vplex::community::OnlineState_t val)
{
    const char* out;
    if (val == vplex::community::OFFLINE) {
        out = "OFFLINE";
    } else if (val == vplex::community::AVAILABLE) {
        out = "AVAILABLE";
    } else if (val == vplex::community::AWAY) {
        out = "AWAY";
    } else if (val == vplex::community::IDLE) {
        out = "IDLE";
    } else if (val == vplex::community::INVISIBLE) {
        out = "INVISIBLE";
    } else {
        out = "";
    }
    return out;
}

