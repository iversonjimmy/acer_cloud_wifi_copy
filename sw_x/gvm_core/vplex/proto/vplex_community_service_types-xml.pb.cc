// GENERATED FROM vplex_community_service_types.proto, DO NOT EDIT

#include "vplex_community_service_types-xml.pb.h"

#include <string.h>

#include <ProtoXmlParseStateImpl.h>

static void processOpenSessionInfo(vplex::community::SessionInfo* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenSessionInfoCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseSessionInfo(vplex::community::SessionInfo* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseSessionInfoCb(const char* tag, ProtoXmlParseStateImpl* state);

vplex::community::ParseStateSessionInfo::ParseStateSessionInfo(const char* outerTag, SessionInfo* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_community_service_types(impl);
}

void
vplex::community::RegisterHandlers_vplex_community_service_types(ProtoXmlParseStateImpl* impl)
{
    if (impl->openHandlers.find("vplex.community.SessionInfo") == impl->openHandlers.end()) {
        impl->openHandlers["vplex.community.SessionInfo"] = processOpenSessionInfoCb;
        impl->closeHandlers["vplex.community.SessionInfo"] = processCloseSessionInfoCb;
    }
}

void
processOpenSessionInfo(vplex::community::SessionInfo* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processOpenSessionInfoCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::community::SessionInfo* msg = static_cast<vplex::community::SessionInfo*>(state->protoStack.top());
    processOpenSessionInfo(msg, tag, state);
}

void
processCloseSessionInfo(vplex::community::SessionInfo* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "sessionHandle") == 0) {
        if (msg->has_sessionhandle()) {
            state->error = "Duplicate of non-repeated field sessionHandle";
        } else {
            msg->set_sessionhandle((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "serviceTicket") == 0) {
        if (msg->has_serviceticket()) {
            state->error = "Duplicate of non-repeated field serviceTicket";
        } else {
            msg->set_serviceticket(parseBytes(state->lastData, state));
        }
    } else {
        (void)msg;
    }
}
void
processCloseSessionInfoCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::community::SessionInfo* msg = static_cast<vplex::community::SessionInfo*>(state->protoStack.top());
    processCloseSessionInfo(msg, tag, state);
}

void
vplex::community::writeSessionInfo(VPLXmlWriter* writer, const vplex::community::SessionInfo& message)
{
    if (message.has_sessionhandle()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.sessionhandle());
        VPLXmlWriter_InsertSimpleElement(writer, "sessionHandle", val);
    }
    if (message.has_serviceticket()) {
        std::string val = writeBytes(message.serviceticket());
        VPLXmlWriter_InsertSimpleElement(writer, "serviceTicket", val.c_str());
    }
}

