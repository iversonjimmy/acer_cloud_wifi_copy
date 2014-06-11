// GENERATED FROM vplex_nus_service_types.proto, DO NOT EDIT

#include "vplex_nus_service_types-xml.pb.h"

#include <string.h>

#include <ProtoXmlParseStateImpl.h>

static void processOpenAbstractRequestType(vplex::nus::AbstractRequestType* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenAbstractRequestTypeCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseAbstractRequestType(vplex::nus::AbstractRequestType* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseAbstractRequestTypeCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenAbstractResponseType(vplex::nus::AbstractResponseType* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenAbstractResponseTypeCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseAbstractResponseType(vplex::nus::AbstractResponseType* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseAbstractResponseTypeCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenTitleVersionType(vplex::nus::TitleVersionType* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenTitleVersionTypeCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseTitleVersionType(vplex::nus::TitleVersionType* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseTitleVersionTypeCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetSystemUpdateRequestType(vplex::nus::GetSystemUpdateRequestType* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetSystemUpdateRequestTypeCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetSystemUpdateRequestType(vplex::nus::GetSystemUpdateRequestType* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetSystemUpdateRequestTypeCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetSystemUpdateResponseType(vplex::nus::GetSystemUpdateResponseType* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetSystemUpdateResponseTypeCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetSystemUpdateResponseType(vplex::nus::GetSystemUpdateResponseType* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetSystemUpdateResponseTypeCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetSystemTMDRequestType(vplex::nus::GetSystemTMDRequestType* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetSystemTMDRequestTypeCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetSystemTMDRequestType(vplex::nus::GetSystemTMDRequestType* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetSystemTMDRequestTypeCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetSystemTMDResponseType(vplex::nus::GetSystemTMDResponseType* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetSystemTMDResponseTypeCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetSystemTMDResponseType(vplex::nus::GetSystemTMDResponseType* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetSystemTMDResponseTypeCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetSystemPersonalizedETicketRequestType(vplex::nus::GetSystemPersonalizedETicketRequestType* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetSystemPersonalizedETicketRequestTypeCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetSystemPersonalizedETicketRequestType(vplex::nus::GetSystemPersonalizedETicketRequestType* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetSystemPersonalizedETicketRequestTypeCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetSystemPersonalizedETicketResponseType(vplex::nus::GetSystemPersonalizedETicketResponseType* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetSystemPersonalizedETicketResponseTypeCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetSystemPersonalizedETicketResponseType(vplex::nus::GetSystemPersonalizedETicketResponseType* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetSystemPersonalizedETicketResponseTypeCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetSystemCommonETicketRequestType(vplex::nus::GetSystemCommonETicketRequestType* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetSystemCommonETicketRequestTypeCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetSystemCommonETicketRequestType(vplex::nus::GetSystemCommonETicketRequestType* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetSystemCommonETicketRequestTypeCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetSystemCommonETicketResponseType(vplex::nus::GetSystemCommonETicketResponseType* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetSystemCommonETicketResponseTypeCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetSystemCommonETicketResponseType(vplex::nus::GetSystemCommonETicketResponseType* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetSystemCommonETicketResponseTypeCb(const char* tag, ProtoXmlParseStateImpl* state);

vplex::nus::ParseStateAbstractRequestType::ParseStateAbstractRequestType(const char* outerTag, AbstractRequestType* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_nus_service_types(impl);
}

vplex::nus::ParseStateAbstractResponseType::ParseStateAbstractResponseType(const char* outerTag, AbstractResponseType* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_nus_service_types(impl);
}

vplex::nus::ParseStateTitleVersionType::ParseStateTitleVersionType(const char* outerTag, TitleVersionType* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_nus_service_types(impl);
}

vplex::nus::ParseStateGetSystemUpdateRequestType::ParseStateGetSystemUpdateRequestType(const char* outerTag, GetSystemUpdateRequestType* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_nus_service_types(impl);
}

vplex::nus::ParseStateGetSystemUpdateResponseType::ParseStateGetSystemUpdateResponseType(const char* outerTag, GetSystemUpdateResponseType* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_nus_service_types(impl);
}

vplex::nus::ParseStateGetSystemTMDRequestType::ParseStateGetSystemTMDRequestType(const char* outerTag, GetSystemTMDRequestType* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_nus_service_types(impl);
}

vplex::nus::ParseStateGetSystemTMDResponseType::ParseStateGetSystemTMDResponseType(const char* outerTag, GetSystemTMDResponseType* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_nus_service_types(impl);
}

vplex::nus::ParseStateGetSystemPersonalizedETicketRequestType::ParseStateGetSystemPersonalizedETicketRequestType(const char* outerTag, GetSystemPersonalizedETicketRequestType* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_nus_service_types(impl);
}

vplex::nus::ParseStateGetSystemPersonalizedETicketResponseType::ParseStateGetSystemPersonalizedETicketResponseType(const char* outerTag, GetSystemPersonalizedETicketResponseType* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_nus_service_types(impl);
}

vplex::nus::ParseStateGetSystemCommonETicketRequestType::ParseStateGetSystemCommonETicketRequestType(const char* outerTag, GetSystemCommonETicketRequestType* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_nus_service_types(impl);
}

vplex::nus::ParseStateGetSystemCommonETicketResponseType::ParseStateGetSystemCommonETicketResponseType(const char* outerTag, GetSystemCommonETicketResponseType* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_nus_service_types(impl);
}

void
vplex::nus::RegisterHandlers_vplex_nus_service_types(ProtoXmlParseStateImpl* impl)
{
    if (impl->openHandlers.find("vplex.nus.AbstractRequestType") == impl->openHandlers.end()) {
        impl->openHandlers["vplex.nus.AbstractRequestType"] = processOpenAbstractRequestTypeCb;
        impl->closeHandlers["vplex.nus.AbstractRequestType"] = processCloseAbstractRequestTypeCb;
        impl->openHandlers["vplex.nus.AbstractResponseType"] = processOpenAbstractResponseTypeCb;
        impl->closeHandlers["vplex.nus.AbstractResponseType"] = processCloseAbstractResponseTypeCb;
        impl->openHandlers["vplex.nus.TitleVersionType"] = processOpenTitleVersionTypeCb;
        impl->closeHandlers["vplex.nus.TitleVersionType"] = processCloseTitleVersionTypeCb;
        impl->openHandlers["vplex.nus.GetSystemUpdateRequestType"] = processOpenGetSystemUpdateRequestTypeCb;
        impl->closeHandlers["vplex.nus.GetSystemUpdateRequestType"] = processCloseGetSystemUpdateRequestTypeCb;
        impl->openHandlers["vplex.nus.GetSystemUpdateResponseType"] = processOpenGetSystemUpdateResponseTypeCb;
        impl->closeHandlers["vplex.nus.GetSystemUpdateResponseType"] = processCloseGetSystemUpdateResponseTypeCb;
        impl->openHandlers["vplex.nus.GetSystemTMDRequestType"] = processOpenGetSystemTMDRequestTypeCb;
        impl->closeHandlers["vplex.nus.GetSystemTMDRequestType"] = processCloseGetSystemTMDRequestTypeCb;
        impl->openHandlers["vplex.nus.GetSystemTMDResponseType"] = processOpenGetSystemTMDResponseTypeCb;
        impl->closeHandlers["vplex.nus.GetSystemTMDResponseType"] = processCloseGetSystemTMDResponseTypeCb;
        impl->openHandlers["vplex.nus.GetSystemPersonalizedETicketRequestType"] = processOpenGetSystemPersonalizedETicketRequestTypeCb;
        impl->closeHandlers["vplex.nus.GetSystemPersonalizedETicketRequestType"] = processCloseGetSystemPersonalizedETicketRequestTypeCb;
        impl->openHandlers["vplex.nus.GetSystemPersonalizedETicketResponseType"] = processOpenGetSystemPersonalizedETicketResponseTypeCb;
        impl->closeHandlers["vplex.nus.GetSystemPersonalizedETicketResponseType"] = processCloseGetSystemPersonalizedETicketResponseTypeCb;
        impl->openHandlers["vplex.nus.GetSystemCommonETicketRequestType"] = processOpenGetSystemCommonETicketRequestTypeCb;
        impl->closeHandlers["vplex.nus.GetSystemCommonETicketRequestType"] = processCloseGetSystemCommonETicketRequestTypeCb;
        impl->openHandlers["vplex.nus.GetSystemCommonETicketResponseType"] = processOpenGetSystemCommonETicketResponseTypeCb;
        impl->closeHandlers["vplex.nus.GetSystemCommonETicketResponseType"] = processCloseGetSystemCommonETicketResponseTypeCb;
    }
}

void
processOpenAbstractRequestType(vplex::nus::AbstractRequestType* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processOpenAbstractRequestTypeCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::nus::AbstractRequestType* msg = static_cast<vplex::nus::AbstractRequestType*>(state->protoStack.top());
    processOpenAbstractRequestType(msg, tag, state);
}

void
processCloseAbstractRequestType(vplex::nus::AbstractRequestType* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "Version") == 0) {
        if (msg->has_version()) {
            state->error = "Duplicate of non-repeated field Version";
        } else {
            msg->set_version(state->lastData);
        }
    } else if (strcmp(tag, "MessageId") == 0) {
        if (msg->has_messageid()) {
            state->error = "Duplicate of non-repeated field MessageId";
        } else {
            msg->set_messageid(state->lastData);
        }
    } else if (strcmp(tag, "DeviceId") == 0) {
        if (msg->has_deviceid()) {
            state->error = "Duplicate of non-repeated field DeviceId";
        } else {
            msg->set_deviceid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "RegionId") == 0) {
        if (msg->has_regionid()) {
            state->error = "Duplicate of non-repeated field RegionId";
        } else {
            msg->set_regionid(state->lastData);
        }
    } else if (strcmp(tag, "CountryCode") == 0) {
        if (msg->has_countrycode()) {
            state->error = "Duplicate of non-repeated field CountryCode";
        } else {
            msg->set_countrycode(state->lastData);
        }
    } else if (strcmp(tag, "VirtualDeviceType") == 0) {
        if (msg->has_virtualdevicetype()) {
            state->error = "Duplicate of non-repeated field VirtualDeviceType";
        } else {
            msg->set_virtualdevicetype(parseInt32(state->lastData, state));
        }
    } else if (strcmp(tag, "Language") == 0) {
        if (msg->has_language()) {
            state->error = "Duplicate of non-repeated field Language";
        } else {
            msg->set_language(state->lastData);
        }
    } else if (strcmp(tag, "SerialNo") == 0) {
        if (msg->has_serialno()) {
            state->error = "Duplicate of non-repeated field SerialNo";
        } else {
            msg->set_serialno(state->lastData);
        }
    } else if (strcmp(tag, "AccountId") == 0) {
        if (msg->has_accountid()) {
            state->error = "Duplicate of non-repeated field AccountId";
        } else {
            msg->set_accountid(state->lastData);
        }
    } else if (strcmp(tag, "UserId") == 0) {
        if (msg->has_userid()) {
            state->error = "Duplicate of non-repeated field UserId";
        } else {
            msg->set_userid((u64)parseInt64(state->lastData, state));
        }
    } else {
        (void)msg;
    }
}
void
processCloseAbstractRequestTypeCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::nus::AbstractRequestType* msg = static_cast<vplex::nus::AbstractRequestType*>(state->protoStack.top());
    processCloseAbstractRequestType(msg, tag, state);
}

void
processOpenAbstractResponseType(vplex::nus::AbstractResponseType* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processOpenAbstractResponseTypeCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::nus::AbstractResponseType* msg = static_cast<vplex::nus::AbstractResponseType*>(state->protoStack.top());
    processOpenAbstractResponseType(msg, tag, state);
}

void
processCloseAbstractResponseType(vplex::nus::AbstractResponseType* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "Version") == 0) {
        if (msg->has_version()) {
            state->error = "Duplicate of non-repeated field Version";
        } else {
            msg->set_version(state->lastData);
        }
    } else if (strcmp(tag, "DeviceId") == 0) {
        if (msg->has_deviceid()) {
            state->error = "Duplicate of non-repeated field DeviceId";
        } else {
            msg->set_deviceid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "MessageId") == 0) {
        if (msg->has_messageid()) {
            state->error = "Duplicate of non-repeated field MessageId";
        } else {
            msg->set_messageid(state->lastData);
        }
    } else if (strcmp(tag, "TimeStamp") == 0) {
        if (msg->has_timestamp()) {
            state->error = "Duplicate of non-repeated field TimeStamp";
        } else {
            msg->set_timestamp((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "ErrorCode") == 0) {
        if (msg->has_errorcode()) {
            state->error = "Duplicate of non-repeated field ErrorCode";
        } else {
            msg->set_errorcode(parseInt32(state->lastData, state));
        }
    } else if (strcmp(tag, "ErrorMessage") == 0) {
        if (msg->has_errormessage()) {
            state->error = "Duplicate of non-repeated field ErrorMessage";
        } else {
            msg->set_errormessage(state->lastData);
        }
    } else {
        (void)msg;
    }
}
void
processCloseAbstractResponseTypeCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::nus::AbstractResponseType* msg = static_cast<vplex::nus::AbstractResponseType*>(state->protoStack.top());
    processCloseAbstractResponseType(msg, tag, state);
}

void
processOpenTitleVersionType(vplex::nus::TitleVersionType* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processOpenTitleVersionTypeCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::nus::TitleVersionType* msg = static_cast<vplex::nus::TitleVersionType*>(state->protoStack.top());
    processOpenTitleVersionType(msg, tag, state);
}

void
processCloseTitleVersionType(vplex::nus::TitleVersionType* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "TitleId") == 0) {
        if (msg->has_titleid()) {
            state->error = "Duplicate of non-repeated field TitleId";
        } else {
            msg->set_titleid(state->lastData);
        }
    } else if (strcmp(tag, "Version") == 0) {
        if (msg->has_version()) {
            state->error = "Duplicate of non-repeated field Version";
        } else {
            msg->set_version(parseInt32(state->lastData, state));
        }
    } else if (strcmp(tag, "FsSize") == 0) {
        if (msg->has_fssize()) {
            state->error = "Duplicate of non-repeated field FsSize";
        } else {
            msg->set_fssize(parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "TicketSize") == 0) {
        if (msg->has_ticketsize()) {
            state->error = "Duplicate of non-repeated field TicketSize";
        } else {
            msg->set_ticketsize(parseInt32(state->lastData, state));
        }
    } else if (strcmp(tag, "TMDSize") == 0) {
        if (msg->has_tmdsize()) {
            state->error = "Duplicate of non-repeated field TMDSize";
        } else {
            msg->set_tmdsize(parseInt32(state->lastData, state));
        }
    } else if (strcmp(tag, "AppGUID") == 0) {
        if (msg->has_appguid()) {
            state->error = "Duplicate of non-repeated field AppGUID";
        } else {
            msg->set_appguid(state->lastData);
        }
    } else if (strcmp(tag, "AppVersion") == 0) {
        if (msg->has_appversion()) {
            state->error = "Duplicate of non-repeated field AppVersion";
        } else {
            msg->set_appversion(state->lastData);
        }
    } else if (strcmp(tag, "AppMinVersion") == 0) {
        if (msg->has_appminversion()) {
            state->error = "Duplicate of non-repeated field AppMinVersion";
        } else {
            msg->set_appminversion(state->lastData);
        }
    } else if (strcmp(tag, "CcdMinVersion") == 0) {
        if (msg->has_ccdminversion()) {
            state->error = "Duplicate of non-repeated field CcdMinVersion";
        } else {
            msg->set_ccdminversion(state->lastData);
        }
    } else if (strcmp(tag, "AppMessage") == 0) {
        if (msg->has_appmessage()) {
            state->error = "Duplicate of non-repeated field AppMessage";
        } else {
            msg->set_appmessage(state->lastData);
        }
    } else {
        (void)msg;
    }
}
void
processCloseTitleVersionTypeCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::nus::TitleVersionType* msg = static_cast<vplex::nus::TitleVersionType*>(state->protoStack.top());
    processCloseTitleVersionType(msg, tag, state);
}

void
processOpenGetSystemUpdateRequestType(vplex::nus::GetSystemUpdateRequestType* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "TitleVersion") == 0) {
        vplex::nus::TitleVersionType* proto = msg->add_titleversion();
        state->protoStack.push(proto);
        state->protoStartDepth.push(state->tagDepth);
    } else {
        (void)msg;
    }
}
void
processOpenGetSystemUpdateRequestTypeCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::nus::GetSystemUpdateRequestType* msg = static_cast<vplex::nus::GetSystemUpdateRequestType*>(state->protoStack.top());
    processOpenGetSystemUpdateRequestType(msg, tag, state);
}

void
processCloseGetSystemUpdateRequestType(vplex::nus::GetSystemUpdateRequestType* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "Attribute") == 0) {
        if (msg->has_attribute()) {
            state->error = "Duplicate of non-repeated field Attribute";
        } else {
            msg->set_attribute(parseInt32(state->lastData, state));
        }
    } else if (strcmp(tag, "AuditData") == 0) {
        if (msg->has_auditdata()) {
            state->error = "Duplicate of non-repeated field AuditData";
        } else {
            msg->set_auditdata(state->lastData);
        }
    } else if (strcmp(tag, "RunTimeTypeMask") == 0) {
        if (msg->has_runtimetypemask()) {
            state->error = "Duplicate of non-repeated field RunTimeTypeMask";
        } else {
            msg->set_runtimetypemask(parseInt32(state->lastData, state));
        }
    } else if (strcmp(tag, "Group") == 0) {
        if (msg->has_group()) {
            state->error = "Duplicate of non-repeated field Group";
        } else {
            msg->set_group(state->lastData);
        }
    } else {
        processCloseAbstractRequestType(msg->mutable__inherited(), tag, state);
    }
}
void
processCloseGetSystemUpdateRequestTypeCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::nus::GetSystemUpdateRequestType* msg = static_cast<vplex::nus::GetSystemUpdateRequestType*>(state->protoStack.top());
    processCloseGetSystemUpdateRequestType(msg, tag, state);
}

void
processOpenGetSystemUpdateResponseType(vplex::nus::GetSystemUpdateResponseType* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "TitleVersion") == 0) {
        vplex::nus::TitleVersionType* proto = msg->add_titleversion();
        state->protoStack.push(proto);
        state->protoStartDepth.push(state->tagDepth);
    } else {
        (void)msg;
    }
}
void
processOpenGetSystemUpdateResponseTypeCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::nus::GetSystemUpdateResponseType* msg = static_cast<vplex::nus::GetSystemUpdateResponseType*>(state->protoStack.top());
    processOpenGetSystemUpdateResponseType(msg, tag, state);
}

void
processCloseGetSystemUpdateResponseType(vplex::nus::GetSystemUpdateResponseType* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "ContentPrefixURL") == 0) {
        if (msg->has_contentprefixurl()) {
            state->error = "Duplicate of non-repeated field ContentPrefixURL";
        } else {
            msg->set_contentprefixurl(state->lastData);
        }
    } else if (strcmp(tag, "UncachedContentPrefixURL") == 0) {
        if (msg->has_uncachedcontentprefixurl()) {
            state->error = "Duplicate of non-repeated field UncachedContentPrefixURL";
        } else {
            msg->set_uncachedcontentprefixurl(state->lastData);
        }
    } else if (strcmp(tag, "PcsPrefiURL") == 0) {
        if (msg->has_pcsprefiurl()) {
            state->error = "Duplicate of non-repeated field PcsPrefiURL";
        } else {
            msg->set_pcsprefiurl(state->lastData);
        }
    } else if (strcmp(tag, "UploadAuditData") == 0) {
        if (msg->has_uploadauditdata()) {
            state->error = "Duplicate of non-repeated field UploadAuditData";
        } else {
            msg->set_uploadauditdata(parseInt32(state->lastData, state));
        }
    } else if (strcmp(tag, "IsQA") == 0) {
        if (msg->has_isqa()) {
            state->error = "Duplicate of non-repeated field IsQA";
        } else {
            msg->set_isqa(parseBool(state->lastData, state));
        }
    } else if (strcmp(tag, "IsAutoUpdateDisabled") == 0) {
        if (msg->has_isautoupdatedisabled()) {
            state->error = "Duplicate of non-repeated field IsAutoUpdateDisabled";
        } else {
            msg->set_isautoupdatedisabled(parseBool(state->lastData, state));
        }
    } else if (strcmp(tag, "InfraDownload") == 0) {
        if (msg->has_infradownload()) {
            state->error = "Duplicate of non-repeated field InfraDownload";
        } else {
            msg->set_infradownload(parseBool(state->lastData, state));
        }
    } else {
        processCloseAbstractResponseType(msg->mutable__inherited(), tag, state);
    }
}
void
processCloseGetSystemUpdateResponseTypeCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::nus::GetSystemUpdateResponseType* msg = static_cast<vplex::nus::GetSystemUpdateResponseType*>(state->protoStack.top());
    processCloseGetSystemUpdateResponseType(msg, tag, state);
}

void
processOpenGetSystemTMDRequestType(vplex::nus::GetSystemTMDRequestType* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "TitleVersion") == 0) {
        vplex::nus::TitleVersionType* proto = msg->add_titleversion();
        state->protoStack.push(proto);
        state->protoStartDepth.push(state->tagDepth);
    } else {
        (void)msg;
    }
}
void
processOpenGetSystemTMDRequestTypeCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::nus::GetSystemTMDRequestType* msg = static_cast<vplex::nus::GetSystemTMDRequestType*>(state->protoStack.top());
    processOpenGetSystemTMDRequestType(msg, tag, state);
}

void
processCloseGetSystemTMDRequestType(vplex::nus::GetSystemTMDRequestType* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        processCloseAbstractRequestType(msg->mutable__inherited(), tag, state);
    }
}
void
processCloseGetSystemTMDRequestTypeCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::nus::GetSystemTMDRequestType* msg = static_cast<vplex::nus::GetSystemTMDRequestType*>(state->protoStack.top());
    processCloseGetSystemTMDRequestType(msg, tag, state);
}

void
processOpenGetSystemTMDResponseType(vplex::nus::GetSystemTMDResponseType* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processOpenGetSystemTMDResponseTypeCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::nus::GetSystemTMDResponseType* msg = static_cast<vplex::nus::GetSystemTMDResponseType*>(state->protoStack.top());
    processOpenGetSystemTMDResponseType(msg, tag, state);
}

void
processCloseGetSystemTMDResponseType(vplex::nus::GetSystemTMDResponseType* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "TMD") == 0) {
        msg->add_tmd(parseBytes(state->lastData, state));
    } else {
        processCloseAbstractResponseType(msg->mutable__inherited(), tag, state);
    }
}
void
processCloseGetSystemTMDResponseTypeCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::nus::GetSystemTMDResponseType* msg = static_cast<vplex::nus::GetSystemTMDResponseType*>(state->protoStack.top());
    processCloseGetSystemTMDResponseType(msg, tag, state);
}

void
processOpenGetSystemPersonalizedETicketRequestType(vplex::nus::GetSystemPersonalizedETicketRequestType* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processOpenGetSystemPersonalizedETicketRequestTypeCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::nus::GetSystemPersonalizedETicketRequestType* msg = static_cast<vplex::nus::GetSystemPersonalizedETicketRequestType*>(state->protoStack.top());
    processOpenGetSystemPersonalizedETicketRequestType(msg, tag, state);
}

void
processCloseGetSystemPersonalizedETicketRequestType(vplex::nus::GetSystemPersonalizedETicketRequestType* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "TitleId") == 0) {
        msg->add_titleid(state->lastData);
    } else if (strcmp(tag, "DeviceCert") == 0) {
        if (msg->has_devicecert()) {
            state->error = "Duplicate of non-repeated field DeviceCert";
        } else {
            msg->set_devicecert(parseBytes(state->lastData, state));
        }
    } else {
        processCloseAbstractRequestType(msg->mutable__inherited(), tag, state);
    }
}
void
processCloseGetSystemPersonalizedETicketRequestTypeCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::nus::GetSystemPersonalizedETicketRequestType* msg = static_cast<vplex::nus::GetSystemPersonalizedETicketRequestType*>(state->protoStack.top());
    processCloseGetSystemPersonalizedETicketRequestType(msg, tag, state);
}

void
processOpenGetSystemPersonalizedETicketResponseType(vplex::nus::GetSystemPersonalizedETicketResponseType* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processOpenGetSystemPersonalizedETicketResponseTypeCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::nus::GetSystemPersonalizedETicketResponseType* msg = static_cast<vplex::nus::GetSystemPersonalizedETicketResponseType*>(state->protoStack.top());
    processOpenGetSystemPersonalizedETicketResponseType(msg, tag, state);
}

void
processCloseGetSystemPersonalizedETicketResponseType(vplex::nus::GetSystemPersonalizedETicketResponseType* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "ETicket") == 0) {
        msg->add_eticket(parseBytes(state->lastData, state));
    } else if (strcmp(tag, "Certs") == 0) {
        msg->add_certs(parseBytes(state->lastData, state));
    } else {
        processCloseAbstractResponseType(msg->mutable__inherited(), tag, state);
    }
}
void
processCloseGetSystemPersonalizedETicketResponseTypeCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::nus::GetSystemPersonalizedETicketResponseType* msg = static_cast<vplex::nus::GetSystemPersonalizedETicketResponseType*>(state->protoStack.top());
    processCloseGetSystemPersonalizedETicketResponseType(msg, tag, state);
}

void
processOpenGetSystemCommonETicketRequestType(vplex::nus::GetSystemCommonETicketRequestType* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processOpenGetSystemCommonETicketRequestTypeCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::nus::GetSystemCommonETicketRequestType* msg = static_cast<vplex::nus::GetSystemCommonETicketRequestType*>(state->protoStack.top());
    processOpenGetSystemCommonETicketRequestType(msg, tag, state);
}

void
processCloseGetSystemCommonETicketRequestType(vplex::nus::GetSystemCommonETicketRequestType* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "TitleId") == 0) {
        msg->add_titleid(state->lastData);
    } else {
        processCloseAbstractRequestType(msg->mutable__inherited(), tag, state);
    }
}
void
processCloseGetSystemCommonETicketRequestTypeCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::nus::GetSystemCommonETicketRequestType* msg = static_cast<vplex::nus::GetSystemCommonETicketRequestType*>(state->protoStack.top());
    processCloseGetSystemCommonETicketRequestType(msg, tag, state);
}

void
processOpenGetSystemCommonETicketResponseType(vplex::nus::GetSystemCommonETicketResponseType* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processOpenGetSystemCommonETicketResponseTypeCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::nus::GetSystemCommonETicketResponseType* msg = static_cast<vplex::nus::GetSystemCommonETicketResponseType*>(state->protoStack.top());
    processOpenGetSystemCommonETicketResponseType(msg, tag, state);
}

void
processCloseGetSystemCommonETicketResponseType(vplex::nus::GetSystemCommonETicketResponseType* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "CommonETicket") == 0) {
        msg->add_commoneticket(parseBytes(state->lastData, state));
    } else if (strcmp(tag, "Certs") == 0) {
        msg->add_certs(parseBytes(state->lastData, state));
    } else {
        processCloseAbstractResponseType(msg->mutable__inherited(), tag, state);
    }
}
void
processCloseGetSystemCommonETicketResponseTypeCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::nus::GetSystemCommonETicketResponseType* msg = static_cast<vplex::nus::GetSystemCommonETicketResponseType*>(state->protoStack.top());
    processCloseGetSystemCommonETicketResponseType(msg, tag, state);
}

void
vplex::nus::writeAbstractRequestType(VPLXmlWriter* writer, const vplex::nus::AbstractRequestType& message)
{
    if (message.has_version()) {
        VPLXmlWriter_InsertSimpleElement(writer, "Version", message.version().c_str());
    }
    if (message.has_messageid()) {
        VPLXmlWriter_InsertSimpleElement(writer, "MessageId", message.messageid().c_str());
    }
    if (message.has_deviceid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.deviceid());
        VPLXmlWriter_InsertSimpleElement(writer, "DeviceId", val);
    }
    if (message.has_regionid()) {
        VPLXmlWriter_InsertSimpleElement(writer, "RegionId", message.regionid().c_str());
    }
    if (message.has_countrycode()) {
        VPLXmlWriter_InsertSimpleElement(writer, "CountryCode", message.countrycode().c_str());
    }
    if (message.has_virtualdevicetype()) {
        char val[15];
        sprintf(val, FMTs32, message.virtualdevicetype());
        VPLXmlWriter_InsertSimpleElement(writer, "VirtualDeviceType", val);
    }
    if (message.has_language()) {
        VPLXmlWriter_InsertSimpleElement(writer, "Language", message.language().c_str());
    }
    if (message.has_serialno()) {
        VPLXmlWriter_InsertSimpleElement(writer, "SerialNo", message.serialno().c_str());
    }
    if (message.has_accountid()) {
        VPLXmlWriter_InsertSimpleElement(writer, "AccountId", message.accountid().c_str());
    }
    if (message.has_userid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.userid());
        VPLXmlWriter_InsertSimpleElement(writer, "UserId", val);
    }
}

void
vplex::nus::writeAbstractResponseType(VPLXmlWriter* writer, const vplex::nus::AbstractResponseType& message)
{
    if (message.has_version()) {
        VPLXmlWriter_InsertSimpleElement(writer, "Version", message.version().c_str());
    }
    if (message.has_deviceid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.deviceid());
        VPLXmlWriter_InsertSimpleElement(writer, "DeviceId", val);
    }
    if (message.has_messageid()) {
        VPLXmlWriter_InsertSimpleElement(writer, "MessageId", message.messageid().c_str());
    }
    if (message.has_timestamp()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.timestamp());
        VPLXmlWriter_InsertSimpleElement(writer, "TimeStamp", val);
    }
    if (message.has_errorcode()) {
        char val[15];
        sprintf(val, FMTs32, message.errorcode());
        VPLXmlWriter_InsertSimpleElement(writer, "ErrorCode", val);
    }
    if (message.has_errormessage()) {
        VPLXmlWriter_InsertSimpleElement(writer, "ErrorMessage", message.errormessage().c_str());
    }
}

void
vplex::nus::writeTitleVersionType(VPLXmlWriter* writer, const vplex::nus::TitleVersionType& message)
{
    if (message.has_titleid()) {
        VPLXmlWriter_InsertSimpleElement(writer, "TitleId", message.titleid().c_str());
    }
    if (message.has_version()) {
        char val[15];
        sprintf(val, FMTs32, message.version());
        VPLXmlWriter_InsertSimpleElement(writer, "Version", val);
    }
    if (message.has_fssize()) {
        char val[30];
        sprintf(val, FMTs64, message.fssize());
        VPLXmlWriter_InsertSimpleElement(writer, "FsSize", val);
    }
    if (message.has_ticketsize()) {
        char val[15];
        sprintf(val, FMTs32, message.ticketsize());
        VPLXmlWriter_InsertSimpleElement(writer, "TicketSize", val);
    }
    if (message.has_tmdsize()) {
        char val[15];
        sprintf(val, FMTs32, message.tmdsize());
        VPLXmlWriter_InsertSimpleElement(writer, "TMDSize", val);
    }
    if (message.has_appguid()) {
        VPLXmlWriter_InsertSimpleElement(writer, "AppGUID", message.appguid().c_str());
    }
    if (message.has_appversion()) {
        VPLXmlWriter_InsertSimpleElement(writer, "AppVersion", message.appversion().c_str());
    }
    if (message.has_appminversion()) {
        VPLXmlWriter_InsertSimpleElement(writer, "AppMinVersion", message.appminversion().c_str());
    }
    if (message.has_ccdminversion()) {
        VPLXmlWriter_InsertSimpleElement(writer, "CcdMinVersion", message.ccdminversion().c_str());
    }
    if (message.has_appmessage()) {
        VPLXmlWriter_InsertSimpleElement(writer, "AppMessage", message.appmessage().c_str());
    }
}

void
vplex::nus::writeGetSystemUpdateRequestType(VPLXmlWriter* writer, const vplex::nus::GetSystemUpdateRequestType& message)
{
    if (message.has__inherited()) {
        vplex::nus::writeAbstractRequestType(writer, message._inherited());
    }
    for (int i = 0; i < message.titleversion_size(); i++) {
        VPLXmlWriter_OpenTagV(writer, "TitleVersion", 0);
        vplex::nus::writeTitleVersionType(writer, message.titleversion(i));
        VPLXmlWriter_CloseTag(writer);
    }
    if (message.has_attribute()) {
        char val[15];
        sprintf(val, FMTs32, message.attribute());
        VPLXmlWriter_InsertSimpleElement(writer, "Attribute", val);
    }
    if (message.has_auditdata()) {
        VPLXmlWriter_InsertSimpleElement(writer, "AuditData", message.auditdata().c_str());
    }
    if (message.has_runtimetypemask()) {
        char val[15];
        sprintf(val, FMTs32, message.runtimetypemask());
        VPLXmlWriter_InsertSimpleElement(writer, "RunTimeTypeMask", val);
    }
    if (message.has_group()) {
        VPLXmlWriter_InsertSimpleElement(writer, "Group", message.group().c_str());
    }
}

void
vplex::nus::writeGetSystemUpdateResponseType(VPLXmlWriter* writer, const vplex::nus::GetSystemUpdateResponseType& message)
{
    if (message.has__inherited()) {
        vplex::nus::writeAbstractResponseType(writer, message._inherited());
    }
    if (message.has_contentprefixurl()) {
        VPLXmlWriter_InsertSimpleElement(writer, "ContentPrefixURL", message.contentprefixurl().c_str());
    }
    if (message.has_uncachedcontentprefixurl()) {
        VPLXmlWriter_InsertSimpleElement(writer, "UncachedContentPrefixURL", message.uncachedcontentprefixurl().c_str());
    }
    if (message.has_pcsprefiurl()) {
        VPLXmlWriter_InsertSimpleElement(writer, "PcsPrefiURL", message.pcsprefiurl().c_str());
    }
    for (int i = 0; i < message.titleversion_size(); i++) {
        VPLXmlWriter_OpenTagV(writer, "TitleVersion", 0);
        vplex::nus::writeTitleVersionType(writer, message.titleversion(i));
        VPLXmlWriter_CloseTag(writer);
    }
    if (message.has_uploadauditdata()) {
        char val[15];
        sprintf(val, FMTs32, message.uploadauditdata());
        VPLXmlWriter_InsertSimpleElement(writer, "UploadAuditData", val);
    }
    if (message.has_isqa()) {
        const char* val = (message.isqa() ? "true" : "false");
        VPLXmlWriter_InsertSimpleElement(writer, "IsQA", val);
    }
    if (message.has_isautoupdatedisabled()) {
        const char* val = (message.isautoupdatedisabled() ? "true" : "false");
        VPLXmlWriter_InsertSimpleElement(writer, "IsAutoUpdateDisabled", val);
    }
    if (message.has_infradownload()) {
        const char* val = (message.infradownload() ? "true" : "false");
        VPLXmlWriter_InsertSimpleElement(writer, "InfraDownload", val);
    }
}

void
vplex::nus::writeGetSystemTMDRequestType(VPLXmlWriter* writer, const vplex::nus::GetSystemTMDRequestType& message)
{
    if (message.has__inherited()) {
        vplex::nus::writeAbstractRequestType(writer, message._inherited());
    }
    for (int i = 0; i < message.titleversion_size(); i++) {
        VPLXmlWriter_OpenTagV(writer, "TitleVersion", 0);
        vplex::nus::writeTitleVersionType(writer, message.titleversion(i));
        VPLXmlWriter_CloseTag(writer);
    }
}

void
vplex::nus::writeGetSystemTMDResponseType(VPLXmlWriter* writer, const vplex::nus::GetSystemTMDResponseType& message)
{
    if (message.has__inherited()) {
        vplex::nus::writeAbstractResponseType(writer, message._inherited());
    }
    for (int i = 0; i < message.tmd_size(); i++) {
        std::string val = writeBytes(message.tmd(i));
        VPLXmlWriter_InsertSimpleElement(writer, "TMD", val.c_str());
    }
}

void
vplex::nus::writeGetSystemPersonalizedETicketRequestType(VPLXmlWriter* writer, const vplex::nus::GetSystemPersonalizedETicketRequestType& message)
{
    if (message.has__inherited()) {
        vplex::nus::writeAbstractRequestType(writer, message._inherited());
    }
    for (int i = 0; i < message.titleid_size(); i++) {
        VPLXmlWriter_InsertSimpleElement(writer, "TitleId", message.titleid(i).c_str());
    }
    if (message.has_devicecert()) {
        std::string val = writeBytes(message.devicecert());
        VPLXmlWriter_InsertSimpleElement(writer, "DeviceCert", val.c_str());
    }
}

void
vplex::nus::writeGetSystemPersonalizedETicketResponseType(VPLXmlWriter* writer, const vplex::nus::GetSystemPersonalizedETicketResponseType& message)
{
    if (message.has__inherited()) {
        vplex::nus::writeAbstractResponseType(writer, message._inherited());
    }
    for (int i = 0; i < message.eticket_size(); i++) {
        std::string val = writeBytes(message.eticket(i));
        VPLXmlWriter_InsertSimpleElement(writer, "ETicket", val.c_str());
    }
    for (int i = 0; i < message.certs_size(); i++) {
        std::string val = writeBytes(message.certs(i));
        VPLXmlWriter_InsertSimpleElement(writer, "Certs", val.c_str());
    }
}

void
vplex::nus::writeGetSystemCommonETicketRequestType(VPLXmlWriter* writer, const vplex::nus::GetSystemCommonETicketRequestType& message)
{
    if (message.has__inherited()) {
        vplex::nus::writeAbstractRequestType(writer, message._inherited());
    }
    for (int i = 0; i < message.titleid_size(); i++) {
        VPLXmlWriter_InsertSimpleElement(writer, "TitleId", message.titleid(i).c_str());
    }
}

void
vplex::nus::writeGetSystemCommonETicketResponseType(VPLXmlWriter* writer, const vplex::nus::GetSystemCommonETicketResponseType& message)
{
    if (message.has__inherited()) {
        vplex::nus::writeAbstractResponseType(writer, message._inherited());
    }
    for (int i = 0; i < message.commoneticket_size(); i++) {
        std::string val = writeBytes(message.commoneticket(i));
        VPLXmlWriter_InsertSimpleElement(writer, "CommonETicket", val.c_str());
    }
    for (int i = 0; i < message.certs_size(); i++) {
        std::string val = writeBytes(message.certs(i));
        VPLXmlWriter_InsertSimpleElement(writer, "Certs", val.c_str());
    }
}

