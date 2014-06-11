// GENERATED FROM vplex_ias_service_types.proto, DO NOT EDIT

#include "vplex_ias_service_types-xml.pb.h"

#include <string.h>

#include <ProtoXmlParseStateImpl.h>

static void processOpenAbstractRequestType(vplex::ias::AbstractRequestType* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenAbstractRequestTypeCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseAbstractRequestType(vplex::ias::AbstractRequestType* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseAbstractRequestTypeCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenAbstractResponseType(vplex::ias::AbstractResponseType* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenAbstractResponseTypeCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseAbstractResponseType(vplex::ias::AbstractResponseType* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseAbstractResponseTypeCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenCheckVirtualDeviceCredentialsRenewalRequestType(vplex::ias::CheckVirtualDeviceCredentialsRenewalRequestType* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenCheckVirtualDeviceCredentialsRenewalRequestTypeCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseCheckVirtualDeviceCredentialsRenewalRequestType(vplex::ias::CheckVirtualDeviceCredentialsRenewalRequestType* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseCheckVirtualDeviceCredentialsRenewalRequestTypeCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenCheckVirtualDeviceCredentialsRenewalResponseType(vplex::ias::CheckVirtualDeviceCredentialsRenewalResponseType* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenCheckVirtualDeviceCredentialsRenewalResponseTypeCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseCheckVirtualDeviceCredentialsRenewalResponseType(vplex::ias::CheckVirtualDeviceCredentialsRenewalResponseType* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseCheckVirtualDeviceCredentialsRenewalResponseTypeCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenStrAttributeType(vplex::ias::StrAttributeType* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenStrAttributeTypeCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseStrAttributeType(vplex::ias::StrAttributeType* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseStrAttributeTypeCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetSessionKeyRequestType(vplex::ias::GetSessionKeyRequestType* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetSessionKeyRequestTypeCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetSessionKeyRequestType(vplex::ias::GetSessionKeyRequestType* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetSessionKeyRequestTypeCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetSessionKeyResponseType(vplex::ias::GetSessionKeyResponseType* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetSessionKeyResponseTypeCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetSessionKeyResponseType(vplex::ias::GetSessionKeyResponseType* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetSessionKeyResponseTypeCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenLoginRequestType(vplex::ias::LoginRequestType* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenLoginRequestTypeCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseLoginRequestType(vplex::ias::LoginRequestType* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseLoginRequestTypeCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenLoginResponseType(vplex::ias::LoginResponseType* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenLoginResponseTypeCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseLoginResponseType(vplex::ias::LoginResponseType* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseLoginResponseTypeCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenLogoutRequestType(vplex::ias::LogoutRequestType* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenLogoutRequestTypeCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseLogoutRequestType(vplex::ias::LogoutRequestType* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseLogoutRequestTypeCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenLogoutResponseType(vplex::ias::LogoutResponseType* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenLogoutResponseTypeCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseLogoutResponseType(vplex::ias::LogoutResponseType* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseLogoutResponseTypeCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenRegisterVirtualDeviceRequestType(vplex::ias::RegisterVirtualDeviceRequestType* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenRegisterVirtualDeviceRequestTypeCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseRegisterVirtualDeviceRequestType(vplex::ias::RegisterVirtualDeviceRequestType* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseRegisterVirtualDeviceRequestTypeCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenRegisterVirtualDeviceResponseType(vplex::ias::RegisterVirtualDeviceResponseType* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenRegisterVirtualDeviceResponseTypeCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseRegisterVirtualDeviceResponseType(vplex::ias::RegisterVirtualDeviceResponseType* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseRegisterVirtualDeviceResponseTypeCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenRenewVirtualDeviceCredentialsRequestType(vplex::ias::RenewVirtualDeviceCredentialsRequestType* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenRenewVirtualDeviceCredentialsRequestTypeCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseRenewVirtualDeviceCredentialsRequestType(vplex::ias::RenewVirtualDeviceCredentialsRequestType* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseRenewVirtualDeviceCredentialsRequestTypeCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenRenewVirtualDeviceCredentialsResponseType(vplex::ias::RenewVirtualDeviceCredentialsResponseType* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenRenewVirtualDeviceCredentialsResponseTypeCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseRenewVirtualDeviceCredentialsResponseType(vplex::ias::RenewVirtualDeviceCredentialsResponseType* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseRenewVirtualDeviceCredentialsResponseTypeCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetServerKeyRequestType(vplex::ias::GetServerKeyRequestType* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetServerKeyRequestTypeCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetServerKeyRequestType(vplex::ias::GetServerKeyRequestType* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetServerKeyRequestTypeCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetServerKeyResponseType(vplex::ias::GetServerKeyResponseType* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetServerKeyResponseTypeCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetServerKeyResponseType(vplex::ias::GetServerKeyResponseType* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetServerKeyResponseTypeCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenRequestPairingRequestType(vplex::ias::RequestPairingRequestType* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenRequestPairingRequestTypeCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseRequestPairingRequestType(vplex::ias::RequestPairingRequestType* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseRequestPairingRequestTypeCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenRequestPairingResponseType(vplex::ias::RequestPairingResponseType* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenRequestPairingResponseTypeCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseRequestPairingResponseType(vplex::ias::RequestPairingResponseType* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseRequestPairingResponseTypeCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenRespondToPairingRequestRequestType(vplex::ias::RespondToPairingRequestRequestType* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenRespondToPairingRequestRequestTypeCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseRespondToPairingRequestRequestType(vplex::ias::RespondToPairingRequestRequestType* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseRespondToPairingRequestRequestTypeCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenRespondToPairingRequestResponseType(vplex::ias::RespondToPairingRequestResponseType* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenRespondToPairingRequestResponseTypeCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseRespondToPairingRequestResponseType(vplex::ias::RespondToPairingRequestResponseType* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseRespondToPairingRequestResponseTypeCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenRequestPairingPinRequestType(vplex::ias::RequestPairingPinRequestType* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenRequestPairingPinRequestTypeCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseRequestPairingPinRequestType(vplex::ias::RequestPairingPinRequestType* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseRequestPairingPinRequestTypeCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenRequestPairingPinResponseType(vplex::ias::RequestPairingPinResponseType* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenRequestPairingPinResponseTypeCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseRequestPairingPinResponseType(vplex::ias::RequestPairingPinResponseType* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseRequestPairingPinResponseTypeCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetPairingStatusRequestType(vplex::ias::GetPairingStatusRequestType* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetPairingStatusRequestTypeCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetPairingStatusRequestType(vplex::ias::GetPairingStatusRequestType* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetPairingStatusRequestTypeCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetPairingStatusResponseType(vplex::ias::GetPairingStatusResponseType* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetPairingStatusResponseTypeCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetPairingStatusResponseType(vplex::ias::GetPairingStatusResponseType* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetPairingStatusResponseTypeCb(const char* tag, ProtoXmlParseStateImpl* state);

vplex::ias::ParseStateAbstractRequestType::ParseStateAbstractRequestType(const char* outerTag, AbstractRequestType* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_ias_service_types(impl);
}

vplex::ias::ParseStateAbstractResponseType::ParseStateAbstractResponseType(const char* outerTag, AbstractResponseType* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_ias_service_types(impl);
}

vplex::ias::ParseStateCheckVirtualDeviceCredentialsRenewalRequestType::ParseStateCheckVirtualDeviceCredentialsRenewalRequestType(const char* outerTag, CheckVirtualDeviceCredentialsRenewalRequestType* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_ias_service_types(impl);
}

vplex::ias::ParseStateCheckVirtualDeviceCredentialsRenewalResponseType::ParseStateCheckVirtualDeviceCredentialsRenewalResponseType(const char* outerTag, CheckVirtualDeviceCredentialsRenewalResponseType* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_ias_service_types(impl);
}

vplex::ias::ParseStateStrAttributeType::ParseStateStrAttributeType(const char* outerTag, StrAttributeType* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_ias_service_types(impl);
}

vplex::ias::ParseStateGetSessionKeyRequestType::ParseStateGetSessionKeyRequestType(const char* outerTag, GetSessionKeyRequestType* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_ias_service_types(impl);
}

vplex::ias::ParseStateGetSessionKeyResponseType::ParseStateGetSessionKeyResponseType(const char* outerTag, GetSessionKeyResponseType* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_ias_service_types(impl);
}

vplex::ias::ParseStateLoginRequestType::ParseStateLoginRequestType(const char* outerTag, LoginRequestType* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_ias_service_types(impl);
}

vplex::ias::ParseStateLoginResponseType::ParseStateLoginResponseType(const char* outerTag, LoginResponseType* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_ias_service_types(impl);
}

vplex::ias::ParseStateLogoutRequestType::ParseStateLogoutRequestType(const char* outerTag, LogoutRequestType* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_ias_service_types(impl);
}

vplex::ias::ParseStateLogoutResponseType::ParseStateLogoutResponseType(const char* outerTag, LogoutResponseType* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_ias_service_types(impl);
}

vplex::ias::ParseStateRegisterVirtualDeviceRequestType::ParseStateRegisterVirtualDeviceRequestType(const char* outerTag, RegisterVirtualDeviceRequestType* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_ias_service_types(impl);
}

vplex::ias::ParseStateRegisterVirtualDeviceResponseType::ParseStateRegisterVirtualDeviceResponseType(const char* outerTag, RegisterVirtualDeviceResponseType* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_ias_service_types(impl);
}

vplex::ias::ParseStateRenewVirtualDeviceCredentialsRequestType::ParseStateRenewVirtualDeviceCredentialsRequestType(const char* outerTag, RenewVirtualDeviceCredentialsRequestType* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_ias_service_types(impl);
}

vplex::ias::ParseStateRenewVirtualDeviceCredentialsResponseType::ParseStateRenewVirtualDeviceCredentialsResponseType(const char* outerTag, RenewVirtualDeviceCredentialsResponseType* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_ias_service_types(impl);
}

vplex::ias::ParseStateGetServerKeyRequestType::ParseStateGetServerKeyRequestType(const char* outerTag, GetServerKeyRequestType* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_ias_service_types(impl);
}

vplex::ias::ParseStateGetServerKeyResponseType::ParseStateGetServerKeyResponseType(const char* outerTag, GetServerKeyResponseType* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_ias_service_types(impl);
}

vplex::ias::ParseStateRequestPairingRequestType::ParseStateRequestPairingRequestType(const char* outerTag, RequestPairingRequestType* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_ias_service_types(impl);
}

vplex::ias::ParseStateRequestPairingResponseType::ParseStateRequestPairingResponseType(const char* outerTag, RequestPairingResponseType* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_ias_service_types(impl);
}

vplex::ias::ParseStateRespondToPairingRequestRequestType::ParseStateRespondToPairingRequestRequestType(const char* outerTag, RespondToPairingRequestRequestType* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_ias_service_types(impl);
}

vplex::ias::ParseStateRespondToPairingRequestResponseType::ParseStateRespondToPairingRequestResponseType(const char* outerTag, RespondToPairingRequestResponseType* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_ias_service_types(impl);
}

vplex::ias::ParseStateRequestPairingPinRequestType::ParseStateRequestPairingPinRequestType(const char* outerTag, RequestPairingPinRequestType* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_ias_service_types(impl);
}

vplex::ias::ParseStateRequestPairingPinResponseType::ParseStateRequestPairingPinResponseType(const char* outerTag, RequestPairingPinResponseType* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_ias_service_types(impl);
}

vplex::ias::ParseStateGetPairingStatusRequestType::ParseStateGetPairingStatusRequestType(const char* outerTag, GetPairingStatusRequestType* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_ias_service_types(impl);
}

vplex::ias::ParseStateGetPairingStatusResponseType::ParseStateGetPairingStatusResponseType(const char* outerTag, GetPairingStatusResponseType* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_ias_service_types(impl);
}

void
vplex::ias::RegisterHandlers_vplex_ias_service_types(ProtoXmlParseStateImpl* impl)
{
    if (impl->openHandlers.find("vplex.ias.AbstractRequestType") == impl->openHandlers.end()) {
        impl->openHandlers["vplex.ias.AbstractRequestType"] = processOpenAbstractRequestTypeCb;
        impl->closeHandlers["vplex.ias.AbstractRequestType"] = processCloseAbstractRequestTypeCb;
        impl->openHandlers["vplex.ias.AbstractResponseType"] = processOpenAbstractResponseTypeCb;
        impl->closeHandlers["vplex.ias.AbstractResponseType"] = processCloseAbstractResponseTypeCb;
        impl->openHandlers["vplex.ias.CheckVirtualDeviceCredentialsRenewalRequestType"] = processOpenCheckVirtualDeviceCredentialsRenewalRequestTypeCb;
        impl->closeHandlers["vplex.ias.CheckVirtualDeviceCredentialsRenewalRequestType"] = processCloseCheckVirtualDeviceCredentialsRenewalRequestTypeCb;
        impl->openHandlers["vplex.ias.CheckVirtualDeviceCredentialsRenewalResponseType"] = processOpenCheckVirtualDeviceCredentialsRenewalResponseTypeCb;
        impl->closeHandlers["vplex.ias.CheckVirtualDeviceCredentialsRenewalResponseType"] = processCloseCheckVirtualDeviceCredentialsRenewalResponseTypeCb;
        impl->openHandlers["vplex.ias.StrAttributeType"] = processOpenStrAttributeTypeCb;
        impl->closeHandlers["vplex.ias.StrAttributeType"] = processCloseStrAttributeTypeCb;
        impl->openHandlers["vplex.ias.GetSessionKeyRequestType"] = processOpenGetSessionKeyRequestTypeCb;
        impl->closeHandlers["vplex.ias.GetSessionKeyRequestType"] = processCloseGetSessionKeyRequestTypeCb;
        impl->openHandlers["vplex.ias.GetSessionKeyResponseType"] = processOpenGetSessionKeyResponseTypeCb;
        impl->closeHandlers["vplex.ias.GetSessionKeyResponseType"] = processCloseGetSessionKeyResponseTypeCb;
        impl->openHandlers["vplex.ias.LoginRequestType"] = processOpenLoginRequestTypeCb;
        impl->closeHandlers["vplex.ias.LoginRequestType"] = processCloseLoginRequestTypeCb;
        impl->openHandlers["vplex.ias.LoginResponseType"] = processOpenLoginResponseTypeCb;
        impl->closeHandlers["vplex.ias.LoginResponseType"] = processCloseLoginResponseTypeCb;
        impl->openHandlers["vplex.ias.LogoutRequestType"] = processOpenLogoutRequestTypeCb;
        impl->closeHandlers["vplex.ias.LogoutRequestType"] = processCloseLogoutRequestTypeCb;
        impl->openHandlers["vplex.ias.LogoutResponseType"] = processOpenLogoutResponseTypeCb;
        impl->closeHandlers["vplex.ias.LogoutResponseType"] = processCloseLogoutResponseTypeCb;
        impl->openHandlers["vplex.ias.RegisterVirtualDeviceRequestType"] = processOpenRegisterVirtualDeviceRequestTypeCb;
        impl->closeHandlers["vplex.ias.RegisterVirtualDeviceRequestType"] = processCloseRegisterVirtualDeviceRequestTypeCb;
        impl->openHandlers["vplex.ias.RegisterVirtualDeviceResponseType"] = processOpenRegisterVirtualDeviceResponseTypeCb;
        impl->closeHandlers["vplex.ias.RegisterVirtualDeviceResponseType"] = processCloseRegisterVirtualDeviceResponseTypeCb;
        impl->openHandlers["vplex.ias.RenewVirtualDeviceCredentialsRequestType"] = processOpenRenewVirtualDeviceCredentialsRequestTypeCb;
        impl->closeHandlers["vplex.ias.RenewVirtualDeviceCredentialsRequestType"] = processCloseRenewVirtualDeviceCredentialsRequestTypeCb;
        impl->openHandlers["vplex.ias.RenewVirtualDeviceCredentialsResponseType"] = processOpenRenewVirtualDeviceCredentialsResponseTypeCb;
        impl->closeHandlers["vplex.ias.RenewVirtualDeviceCredentialsResponseType"] = processCloseRenewVirtualDeviceCredentialsResponseTypeCb;
        impl->openHandlers["vplex.ias.GetServerKeyRequestType"] = processOpenGetServerKeyRequestTypeCb;
        impl->closeHandlers["vplex.ias.GetServerKeyRequestType"] = processCloseGetServerKeyRequestTypeCb;
        impl->openHandlers["vplex.ias.GetServerKeyResponseType"] = processOpenGetServerKeyResponseTypeCb;
        impl->closeHandlers["vplex.ias.GetServerKeyResponseType"] = processCloseGetServerKeyResponseTypeCb;
        impl->openHandlers["vplex.ias.RequestPairingRequestType"] = processOpenRequestPairingRequestTypeCb;
        impl->closeHandlers["vplex.ias.RequestPairingRequestType"] = processCloseRequestPairingRequestTypeCb;
        impl->openHandlers["vplex.ias.RequestPairingResponseType"] = processOpenRequestPairingResponseTypeCb;
        impl->closeHandlers["vplex.ias.RequestPairingResponseType"] = processCloseRequestPairingResponseTypeCb;
        impl->openHandlers["vplex.ias.RespondToPairingRequestRequestType"] = processOpenRespondToPairingRequestRequestTypeCb;
        impl->closeHandlers["vplex.ias.RespondToPairingRequestRequestType"] = processCloseRespondToPairingRequestRequestTypeCb;
        impl->openHandlers["vplex.ias.RespondToPairingRequestResponseType"] = processOpenRespondToPairingRequestResponseTypeCb;
        impl->closeHandlers["vplex.ias.RespondToPairingRequestResponseType"] = processCloseRespondToPairingRequestResponseTypeCb;
        impl->openHandlers["vplex.ias.RequestPairingPinRequestType"] = processOpenRequestPairingPinRequestTypeCb;
        impl->closeHandlers["vplex.ias.RequestPairingPinRequestType"] = processCloseRequestPairingPinRequestTypeCb;
        impl->openHandlers["vplex.ias.RequestPairingPinResponseType"] = processOpenRequestPairingPinResponseTypeCb;
        impl->closeHandlers["vplex.ias.RequestPairingPinResponseType"] = processCloseRequestPairingPinResponseTypeCb;
        impl->openHandlers["vplex.ias.GetPairingStatusRequestType"] = processOpenGetPairingStatusRequestTypeCb;
        impl->closeHandlers["vplex.ias.GetPairingStatusRequestType"] = processCloseGetPairingStatusRequestTypeCb;
        impl->openHandlers["vplex.ias.GetPairingStatusResponseType"] = processOpenGetPairingStatusResponseTypeCb;
        impl->closeHandlers["vplex.ias.GetPairingStatusResponseType"] = processCloseGetPairingStatusResponseTypeCb;
    }
}

void
processOpenAbstractRequestType(vplex::ias::AbstractRequestType* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processOpenAbstractRequestTypeCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::ias::AbstractRequestType* msg = static_cast<vplex::ias::AbstractRequestType*>(state->protoStack.top());
    processOpenAbstractRequestType(msg, tag, state);
}

void
processCloseAbstractRequestType(vplex::ias::AbstractRequestType* msg, const char* tag, ProtoXmlParseStateImpl* state)
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
    } else if (strcmp(tag, "Region") == 0) {
        if (msg->has_region()) {
            state->error = "Duplicate of non-repeated field Region";
        } else {
            msg->set_region(state->lastData);
        }
    } else if (strcmp(tag, "Country") == 0) {
        if (msg->has_country()) {
            state->error = "Duplicate of non-repeated field Country";
        } else {
            msg->set_country(state->lastData);
        }
    } else if (strcmp(tag, "Language") == 0) {
        if (msg->has_language()) {
            state->error = "Duplicate of non-repeated field Language";
        } else {
            msg->set_language(state->lastData);
        }
    } else if (strcmp(tag, "SessionHandle") == 0) {
        if (msg->has_sessionhandle()) {
            state->error = "Duplicate of non-repeated field SessionHandle";
        } else {
            msg->set_sessionhandle((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "ServiceTicket") == 0) {
        if (msg->has_serviceticket()) {
            state->error = "Duplicate of non-repeated field ServiceTicket";
        } else {
            msg->set_serviceticket(parseBytes(state->lastData, state));
        }
    } else if (strcmp(tag, "ServiceId") == 0) {
        if (msg->has_serviceid()) {
            state->error = "Duplicate of non-repeated field ServiceId";
        } else {
            msg->set_serviceid(state->lastData);
        }
    } else {
        (void)msg;
    }
}
void
processCloseAbstractRequestTypeCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::ias::AbstractRequestType* msg = static_cast<vplex::ias::AbstractRequestType*>(state->protoStack.top());
    processCloseAbstractRequestType(msg, tag, state);
}

void
processOpenAbstractResponseType(vplex::ias::AbstractResponseType* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processOpenAbstractResponseTypeCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::ias::AbstractResponseType* msg = static_cast<vplex::ias::AbstractResponseType*>(state->protoStack.top());
    processOpenAbstractResponseType(msg, tag, state);
}

void
processCloseAbstractResponseType(vplex::ias::AbstractResponseType* msg, const char* tag, ProtoXmlParseStateImpl* state)
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
    } else if (strcmp(tag, "ServiceStandbyMode") == 0) {
        if (msg->has_servicestandbymode()) {
            state->error = "Duplicate of non-repeated field ServiceStandbyMode";
        } else {
            msg->set_servicestandbymode(parseBool(state->lastData, state));
        }
    } else {
        (void)msg;
    }
}
void
processCloseAbstractResponseTypeCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::ias::AbstractResponseType* msg = static_cast<vplex::ias::AbstractResponseType*>(state->protoStack.top());
    processCloseAbstractResponseType(msg, tag, state);
}

void
processOpenCheckVirtualDeviceCredentialsRenewalRequestType(vplex::ias::CheckVirtualDeviceCredentialsRenewalRequestType* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processOpenCheckVirtualDeviceCredentialsRenewalRequestTypeCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::ias::CheckVirtualDeviceCredentialsRenewalRequestType* msg = static_cast<vplex::ias::CheckVirtualDeviceCredentialsRenewalRequestType*>(state->protoStack.top());
    processOpenCheckVirtualDeviceCredentialsRenewalRequestType(msg, tag, state);
}

void
processCloseCheckVirtualDeviceCredentialsRenewalRequestType(vplex::ias::CheckVirtualDeviceCredentialsRenewalRequestType* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "IssueDate") == 0) {
        if (msg->has_issuedate()) {
            state->error = "Duplicate of non-repeated field IssueDate";
        } else {
            msg->set_issuedate((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "SerialNumber") == 0) {
        if (msg->has_serialnumber()) {
            state->error = "Duplicate of non-repeated field SerialNumber";
        } else {
            msg->set_serialnumber((u64)parseInt64(state->lastData, state));
        }
    } else {
        processCloseAbstractRequestType(msg->mutable__inherited(), tag, state);
    }
}
void
processCloseCheckVirtualDeviceCredentialsRenewalRequestTypeCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::ias::CheckVirtualDeviceCredentialsRenewalRequestType* msg = static_cast<vplex::ias::CheckVirtualDeviceCredentialsRenewalRequestType*>(state->protoStack.top());
    processCloseCheckVirtualDeviceCredentialsRenewalRequestType(msg, tag, state);
}

void
processOpenCheckVirtualDeviceCredentialsRenewalResponseType(vplex::ias::CheckVirtualDeviceCredentialsRenewalResponseType* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processOpenCheckVirtualDeviceCredentialsRenewalResponseTypeCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::ias::CheckVirtualDeviceCredentialsRenewalResponseType* msg = static_cast<vplex::ias::CheckVirtualDeviceCredentialsRenewalResponseType*>(state->protoStack.top());
    processOpenCheckVirtualDeviceCredentialsRenewalResponseType(msg, tag, state);
}

void
processCloseCheckVirtualDeviceCredentialsRenewalResponseType(vplex::ias::CheckVirtualDeviceCredentialsRenewalResponseType* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "ExpectedSerialNumber") == 0) {
        if (msg->has_expectedserialnumber()) {
            state->error = "Duplicate of non-repeated field ExpectedSerialNumber";
        } else {
            msg->set_expectedserialnumber((u64)parseInt64(state->lastData, state));
        }
    } else {
        processCloseAbstractResponseType(msg->mutable__inherited(), tag, state);
    }
}
void
processCloseCheckVirtualDeviceCredentialsRenewalResponseTypeCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::ias::CheckVirtualDeviceCredentialsRenewalResponseType* msg = static_cast<vplex::ias::CheckVirtualDeviceCredentialsRenewalResponseType*>(state->protoStack.top());
    processCloseCheckVirtualDeviceCredentialsRenewalResponseType(msg, tag, state);
}

void
processOpenStrAttributeType(vplex::ias::StrAttributeType* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processOpenStrAttributeTypeCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::ias::StrAttributeType* msg = static_cast<vplex::ias::StrAttributeType*>(state->protoStack.top());
    processOpenStrAttributeType(msg, tag, state);
}

void
processCloseStrAttributeType(vplex::ias::StrAttributeType* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "AttributeName") == 0) {
        if (msg->has_attributename()) {
            state->error = "Duplicate of non-repeated field AttributeName";
        } else {
            msg->set_attributename(state->lastData);
        }
    } else if (strcmp(tag, "AttributeValue") == 0) {
        if (msg->has_attributevalue()) {
            state->error = "Duplicate of non-repeated field AttributeValue";
        } else {
            msg->set_attributevalue(state->lastData);
        }
    } else {
        (void)msg;
    }
}
void
processCloseStrAttributeTypeCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::ias::StrAttributeType* msg = static_cast<vplex::ias::StrAttributeType*>(state->protoStack.top());
    processCloseStrAttributeType(msg, tag, state);
}

void
processOpenGetSessionKeyRequestType(vplex::ias::GetSessionKeyRequestType* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "KeyAttributes") == 0) {
        vplex::ias::StrAttributeType* proto = msg->add_keyattributes();
        state->protoStack.push(proto);
        state->protoStartDepth.push(state->tagDepth);
    } else {
        (void)msg;
    }
}
void
processOpenGetSessionKeyRequestTypeCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::ias::GetSessionKeyRequestType* msg = static_cast<vplex::ias::GetSessionKeyRequestType*>(state->protoStack.top());
    processOpenGetSessionKeyRequestType(msg, tag, state);
}

void
processCloseGetSessionKeyRequestType(vplex::ias::GetSessionKeyRequestType* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "Type") == 0) {
        if (msg->has_type()) {
            state->error = "Duplicate of non-repeated field Type";
        } else {
            msg->set_type(state->lastData);
        }
    } else if (strcmp(tag, "EncryptedSessionKey") == 0) {
        if (msg->has_encryptedsessionkey()) {
            state->error = "Duplicate of non-repeated field EncryptedSessionKey";
        } else {
            msg->set_encryptedsessionkey(parseBytes(state->lastData, state));
        }
    } else {
        processCloseAbstractRequestType(msg->mutable__inherited(), tag, state);
    }
}
void
processCloseGetSessionKeyRequestTypeCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::ias::GetSessionKeyRequestType* msg = static_cast<vplex::ias::GetSessionKeyRequestType*>(state->protoStack.top());
    processCloseGetSessionKeyRequestType(msg, tag, state);
}

void
processOpenGetSessionKeyResponseType(vplex::ias::GetSessionKeyResponseType* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processOpenGetSessionKeyResponseTypeCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::ias::GetSessionKeyResponseType* msg = static_cast<vplex::ias::GetSessionKeyResponseType*>(state->protoStack.top());
    processOpenGetSessionKeyResponseType(msg, tag, state);
}

void
processCloseGetSessionKeyResponseType(vplex::ias::GetSessionKeyResponseType* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "SessionKey") == 0) {
        if (msg->has_sessionkey()) {
            state->error = "Duplicate of non-repeated field SessionKey";
        } else {
            msg->set_sessionkey(parseBytes(state->lastData, state));
        }
    } else if (strcmp(tag, "EncryptedSessionKey") == 0) {
        if (msg->has_encryptedsessionkey()) {
            state->error = "Duplicate of non-repeated field EncryptedSessionKey";
        } else {
            msg->set_encryptedsessionkey(parseBytes(state->lastData, state));
        }
    } else if (strcmp(tag, "InstanceId") == 0) {
        if (msg->has_instanceid()) {
            state->error = "Duplicate of non-repeated field InstanceId";
        } else {
            msg->set_instanceid((u32)parseInt32(state->lastData, state));
        }
    } else {
        processCloseAbstractResponseType(msg->mutable__inherited(), tag, state);
    }
}
void
processCloseGetSessionKeyResponseTypeCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::ias::GetSessionKeyResponseType* msg = static_cast<vplex::ias::GetSessionKeyResponseType*>(state->protoStack.top());
    processCloseGetSessionKeyResponseType(msg, tag, state);
}

void
processOpenLoginRequestType(vplex::ias::LoginRequestType* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processOpenLoginRequestTypeCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::ias::LoginRequestType* msg = static_cast<vplex::ias::LoginRequestType*>(state->protoStack.top());
    processOpenLoginRequestType(msg, tag, state);
}

void
processCloseLoginRequestType(vplex::ias::LoginRequestType* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "Username") == 0) {
        if (msg->has_username()) {
            state->error = "Duplicate of non-repeated field Username";
        } else {
            msg->set_username(state->lastData);
        }
    } else if (strcmp(tag, "Namespace") == 0) {
        if (msg->has_namespace_()) {
            state->error = "Duplicate of non-repeated field Namespace";
        } else {
            msg->set_namespace_(state->lastData);
        }
    } else if (strcmp(tag, "Password") == 0) {
        if (msg->has_password()) {
            state->error = "Duplicate of non-repeated field Password";
        } else {
            msg->set_password(state->lastData);
        }
    } else if (strcmp(tag, "WeakToken") == 0) {
        if (msg->has_weaktoken()) {
            state->error = "Duplicate of non-repeated field WeakToken";
        } else {
            msg->set_weaktoken(state->lastData);
        }
    } else if (strcmp(tag, "PairingToken") == 0) {
        if (msg->has_pairingtoken()) {
            state->error = "Duplicate of non-repeated field PairingToken";
        } else {
            msg->set_pairingtoken(state->lastData);
        }
    } else if (strcmp(tag, "ACEulaAgreed") == 0) {
        if (msg->has_aceulaagreed()) {
            state->error = "Duplicate of non-repeated field ACEulaAgreed";
        } else {
            msg->set_aceulaagreed(parseBool(state->lastData, state));
        }
    } else {
        processCloseAbstractRequestType(msg->mutable__inherited(), tag, state);
    }
}
void
processCloseLoginRequestTypeCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::ias::LoginRequestType* msg = static_cast<vplex::ias::LoginRequestType*>(state->protoStack.top());
    processCloseLoginRequestType(msg, tag, state);
}

void
processOpenLoginResponseType(vplex::ias::LoginResponseType* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processOpenLoginResponseTypeCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::ias::LoginResponseType* msg = static_cast<vplex::ias::LoginResponseType*>(state->protoStack.top());
    processOpenLoginResponseType(msg, tag, state);
}

void
processCloseLoginResponseType(vplex::ias::LoginResponseType* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "SessionHandle") == 0) {
        if (msg->has_sessionhandle()) {
            state->error = "Duplicate of non-repeated field SessionHandle";
        } else {
            msg->set_sessionhandle((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "SessionSecret") == 0) {
        if (msg->has_sessionsecret()) {
            state->error = "Duplicate of non-repeated field SessionSecret";
        } else {
            msg->set_sessionsecret(parseBytes(state->lastData, state));
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
    } else if (strcmp(tag, "DisplayName") == 0) {
        if (msg->has_displayname()) {
            state->error = "Duplicate of non-repeated field DisplayName";
        } else {
            msg->set_displayname(state->lastData);
        }
    } else if (strcmp(tag, "WeakToken") == 0) {
        if (msg->has_weaktoken()) {
            state->error = "Duplicate of non-repeated field WeakToken";
        } else {
            msg->set_weaktoken(state->lastData);
        }
    } else if (strcmp(tag, "OldFgSessionHandle") == 0) {
        if (msg->has_oldfgsessionhandle()) {
            state->error = "Duplicate of non-repeated field OldFgSessionHandle";
        } else {
            msg->set_oldfgsessionhandle((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "StorageRegion") == 0) {
        if (msg->has_storageregion()) {
            state->error = "Duplicate of non-repeated field StorageRegion";
        } else {
            msg->set_storageregion(state->lastData);
        }
    } else if (strcmp(tag, "StorageClusterId") == 0) {
        if (msg->has_storageclusterid()) {
            state->error = "Duplicate of non-repeated field StorageClusterId";
        } else {
            msg->set_storageclusterid(parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "persistentCredentials") == 0) {
        if (msg->has_persistentcredentials()) {
            state->error = "Duplicate of non-repeated field persistentCredentials";
        } else {
            msg->set_persistentcredentials(state->lastData);
        }
    } else {
        processCloseAbstractResponseType(msg->mutable__inherited(), tag, state);
    }
}
void
processCloseLoginResponseTypeCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::ias::LoginResponseType* msg = static_cast<vplex::ias::LoginResponseType*>(state->protoStack.top());
    processCloseLoginResponseType(msg, tag, state);
}

void
processOpenLogoutRequestType(vplex::ias::LogoutRequestType* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processOpenLogoutRequestTypeCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::ias::LogoutRequestType* msg = static_cast<vplex::ias::LogoutRequestType*>(state->protoStack.top());
    processOpenLogoutRequestType(msg, tag, state);
}

void
processCloseLogoutRequestType(vplex::ias::LogoutRequestType* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        processCloseAbstractRequestType(msg->mutable__inherited(), tag, state);
    }
}
void
processCloseLogoutRequestTypeCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::ias::LogoutRequestType* msg = static_cast<vplex::ias::LogoutRequestType*>(state->protoStack.top());
    processCloseLogoutRequestType(msg, tag, state);
}

void
processOpenLogoutResponseType(vplex::ias::LogoutResponseType* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processOpenLogoutResponseTypeCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::ias::LogoutResponseType* msg = static_cast<vplex::ias::LogoutResponseType*>(state->protoStack.top());
    processOpenLogoutResponseType(msg, tag, state);
}

void
processCloseLogoutResponseType(vplex::ias::LogoutResponseType* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        processCloseAbstractResponseType(msg->mutable__inherited(), tag, state);
    }
}
void
processCloseLogoutResponseTypeCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::ias::LogoutResponseType* msg = static_cast<vplex::ias::LogoutResponseType*>(state->protoStack.top());
    processCloseLogoutResponseType(msg, tag, state);
}

void
processOpenRegisterVirtualDeviceRequestType(vplex::ias::RegisterVirtualDeviceRequestType* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processOpenRegisterVirtualDeviceRequestTypeCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::ias::RegisterVirtualDeviceRequestType* msg = static_cast<vplex::ias::RegisterVirtualDeviceRequestType*>(state->protoStack.top());
    processOpenRegisterVirtualDeviceRequestType(msg, tag, state);
}

void
processCloseRegisterVirtualDeviceRequestType(vplex::ias::RegisterVirtualDeviceRequestType* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "Username") == 0) {
        if (msg->has_username()) {
            state->error = "Duplicate of non-repeated field Username";
        } else {
            msg->set_username(state->lastData);
        }
    } else if (strcmp(tag, "Password") == 0) {
        if (msg->has_password()) {
            state->error = "Duplicate of non-repeated field Password";
        } else {
            msg->set_password(state->lastData);
        }
    } else if (strcmp(tag, "HardwareInfo") == 0) {
        if (msg->has_hardwareinfo()) {
            state->error = "Duplicate of non-repeated field HardwareInfo";
        } else {
            msg->set_hardwareinfo(parseBytes(state->lastData, state));
        }
    } else if (strcmp(tag, "DeviceName") == 0) {
        if (msg->has_devicename()) {
            state->error = "Duplicate of non-repeated field DeviceName";
        } else {
            msg->set_devicename(state->lastData);
        }
    } else if (strcmp(tag, "Namespace") == 0) {
        if (msg->has_namespace_()) {
            state->error = "Duplicate of non-repeated field Namespace";
        } else {
            msg->set_namespace_(state->lastData);
        }
    } else if (strcmp(tag, "WeakToken") == 0) {
        if (msg->has_weaktoken()) {
            state->error = "Duplicate of non-repeated field WeakToken";
        } else {
            msg->set_weaktoken(state->lastData);
        }
    } else if (strcmp(tag, "PairingToken") == 0) {
        if (msg->has_pairingtoken()) {
            state->error = "Duplicate of non-repeated field PairingToken";
        } else {
            msg->set_pairingtoken(state->lastData);
        }
    } else {
        processCloseAbstractRequestType(msg->mutable__inherited(), tag, state);
    }
}
void
processCloseRegisterVirtualDeviceRequestTypeCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::ias::RegisterVirtualDeviceRequestType* msg = static_cast<vplex::ias::RegisterVirtualDeviceRequestType*>(state->protoStack.top());
    processCloseRegisterVirtualDeviceRequestType(msg, tag, state);
}

void
processOpenRegisterVirtualDeviceResponseType(vplex::ias::RegisterVirtualDeviceResponseType* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processOpenRegisterVirtualDeviceResponseTypeCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::ias::RegisterVirtualDeviceResponseType* msg = static_cast<vplex::ias::RegisterVirtualDeviceResponseType*>(state->protoStack.top());
    processOpenRegisterVirtualDeviceResponseType(msg, tag, state);
}

void
processCloseRegisterVirtualDeviceResponseType(vplex::ias::RegisterVirtualDeviceResponseType* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "RenewalToken") == 0) {
        if (msg->has_renewaltoken()) {
            state->error = "Duplicate of non-repeated field RenewalToken";
        } else {
            msg->set_renewaltoken(parseBytes(state->lastData, state));
        }
    } else {
        processCloseAbstractResponseType(msg->mutable__inherited(), tag, state);
    }
}
void
processCloseRegisterVirtualDeviceResponseTypeCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::ias::RegisterVirtualDeviceResponseType* msg = static_cast<vplex::ias::RegisterVirtualDeviceResponseType*>(state->protoStack.top());
    processCloseRegisterVirtualDeviceResponseType(msg, tag, state);
}

void
processOpenRenewVirtualDeviceCredentialsRequestType(vplex::ias::RenewVirtualDeviceCredentialsRequestType* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processOpenRenewVirtualDeviceCredentialsRequestTypeCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::ias::RenewVirtualDeviceCredentialsRequestType* msg = static_cast<vplex::ias::RenewVirtualDeviceCredentialsRequestType*>(state->protoStack.top());
    processOpenRenewVirtualDeviceCredentialsRequestType(msg, tag, state);
}

void
processCloseRenewVirtualDeviceCredentialsRequestType(vplex::ias::RenewVirtualDeviceCredentialsRequestType* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "SerialNumber") == 0) {
        if (msg->has_serialnumber()) {
            state->error = "Duplicate of non-repeated field SerialNumber";
        } else {
            msg->set_serialnumber((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "IssueDate") == 0) {
        if (msg->has_issuedate()) {
            state->error = "Duplicate of non-repeated field IssueDate";
        } else {
            msg->set_issuedate((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "RenewalToken") == 0) {
        if (msg->has_renewaltoken()) {
            state->error = "Duplicate of non-repeated field RenewalToken";
        } else {
            msg->set_renewaltoken(parseBytes(state->lastData, state));
        }
    } else {
        processCloseAbstractRequestType(msg->mutable__inherited(), tag, state);
    }
}
void
processCloseRenewVirtualDeviceCredentialsRequestTypeCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::ias::RenewVirtualDeviceCredentialsRequestType* msg = static_cast<vplex::ias::RenewVirtualDeviceCredentialsRequestType*>(state->protoStack.top());
    processCloseRenewVirtualDeviceCredentialsRequestType(msg, tag, state);
}

void
processOpenRenewVirtualDeviceCredentialsResponseType(vplex::ias::RenewVirtualDeviceCredentialsResponseType* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processOpenRenewVirtualDeviceCredentialsResponseTypeCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::ias::RenewVirtualDeviceCredentialsResponseType* msg = static_cast<vplex::ias::RenewVirtualDeviceCredentialsResponseType*>(state->protoStack.top());
    processOpenRenewVirtualDeviceCredentialsResponseType(msg, tag, state);
}

void
processCloseRenewVirtualDeviceCredentialsResponseType(vplex::ias::RenewVirtualDeviceCredentialsResponseType* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "SecretDeviceCredentials") == 0) {
        if (msg->has_secretdevicecredentials()) {
            state->error = "Duplicate of non-repeated field SecretDeviceCredentials";
        } else {
            msg->set_secretdevicecredentials(parseBytes(state->lastData, state));
        }
    } else if (strcmp(tag, "ClearDeviceCredentials") == 0) {
        if (msg->has_cleardevicecredentials()) {
            state->error = "Duplicate of non-repeated field ClearDeviceCredentials";
        } else {
            msg->set_cleardevicecredentials(parseBytes(state->lastData, state));
        }
    } else if (strcmp(tag, "RenewalToken") == 0) {
        if (msg->has_renewaltoken()) {
            state->error = "Duplicate of non-repeated field RenewalToken";
        } else {
            msg->set_renewaltoken(parseBytes(state->lastData, state));
        }
    } else if (strcmp(tag, "AttestProgram") == 0) {
        if (msg->has_attestprogram()) {
            state->error = "Duplicate of non-repeated field AttestProgram";
        } else {
            msg->set_attestprogram(parseBytes(state->lastData, state));
        }
    } else if (strcmp(tag, "IssueDate") == 0) {
        if (msg->has_issuedate()) {
            state->error = "Duplicate of non-repeated field IssueDate";
        } else {
            msg->set_issuedate((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "SerialNumber") == 0) {
        if (msg->has_serialnumber()) {
            state->error = "Duplicate of non-repeated field SerialNumber";
        } else {
            msg->set_serialnumber((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "AttestTMD") == 0) {
        if (msg->has_attesttmd()) {
            state->error = "Duplicate of non-repeated field AttestTMD";
        } else {
            msg->set_attesttmd(parseBytes(state->lastData, state));
        }
    } else if (strcmp(tag, "DeviceCert") == 0) {
        if (msg->has_devicecert()) {
            state->error = "Duplicate of non-repeated field DeviceCert";
        } else {
            msg->set_devicecert(parseBytes(state->lastData, state));
        }
    } else if (strcmp(tag, "PlatformKey") == 0) {
        if (msg->has_platformkey()) {
            state->error = "Duplicate of non-repeated field PlatformKey";
        } else {
            msg->set_platformkey(parseBytes(state->lastData, state));
        }
    } else {
        processCloseAbstractResponseType(msg->mutable__inherited(), tag, state);
    }
}
void
processCloseRenewVirtualDeviceCredentialsResponseTypeCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::ias::RenewVirtualDeviceCredentialsResponseType* msg = static_cast<vplex::ias::RenewVirtualDeviceCredentialsResponseType*>(state->protoStack.top());
    processCloseRenewVirtualDeviceCredentialsResponseType(msg, tag, state);
}

void
processOpenGetServerKeyRequestType(vplex::ias::GetServerKeyRequestType* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processOpenGetServerKeyRequestTypeCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::ias::GetServerKeyRequestType* msg = static_cast<vplex::ias::GetServerKeyRequestType*>(state->protoStack.top());
    processOpenGetServerKeyRequestType(msg, tag, state);
}

void
processCloseGetServerKeyRequestType(vplex::ias::GetServerKeyRequestType* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "UserId") == 0) {
        if (msg->has_userid()) {
            state->error = "Duplicate of non-repeated field UserId";
        } else {
            msg->set_userid((u64)parseInt64(state->lastData, state));
        }
    } else {
        processCloseAbstractRequestType(msg->mutable__inherited(), tag, state);
    }
}
void
processCloseGetServerKeyRequestTypeCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::ias::GetServerKeyRequestType* msg = static_cast<vplex::ias::GetServerKeyRequestType*>(state->protoStack.top());
    processCloseGetServerKeyRequestType(msg, tag, state);
}

void
processOpenGetServerKeyResponseType(vplex::ias::GetServerKeyResponseType* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processOpenGetServerKeyResponseTypeCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::ias::GetServerKeyResponseType* msg = static_cast<vplex::ias::GetServerKeyResponseType*>(state->protoStack.top());
    processOpenGetServerKeyResponseType(msg, tag, state);
}

void
processCloseGetServerKeyResponseType(vplex::ias::GetServerKeyResponseType* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "ServerKey") == 0) {
        if (msg->has_serverkey()) {
            state->error = "Duplicate of non-repeated field ServerKey";
        } else {
            msg->set_serverkey(parseBytes(state->lastData, state));
        }
    } else {
        processCloseAbstractResponseType(msg->mutable__inherited(), tag, state);
    }
}
void
processCloseGetServerKeyResponseTypeCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::ias::GetServerKeyResponseType* msg = static_cast<vplex::ias::GetServerKeyResponseType*>(state->protoStack.top());
    processCloseGetServerKeyResponseType(msg, tag, state);
}

void
processOpenRequestPairingRequestType(vplex::ias::RequestPairingRequestType* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "PairingAttributes") == 0) {
        vplex::ias::StrAttributeType* proto = msg->add_pairingattributes();
        state->protoStack.push(proto);
        state->protoStartDepth.push(state->tagDepth);
    } else {
        (void)msg;
    }
}
void
processOpenRequestPairingRequestTypeCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::ias::RequestPairingRequestType* msg = static_cast<vplex::ias::RequestPairingRequestType*>(state->protoStack.top());
    processOpenRequestPairingRequestType(msg, tag, state);
}

void
processCloseRequestPairingRequestType(vplex::ias::RequestPairingRequestType* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "HostHardwareId") == 0) {
        if (msg->has_hosthardwareid()) {
            state->error = "Duplicate of non-repeated field HostHardwareId";
        } else {
            msg->set_hosthardwareid(parseBytes(state->lastData, state));
        }
    } else if (strcmp(tag, "HostDeviceId") == 0) {
        if (msg->has_hostdeviceid()) {
            state->error = "Duplicate of non-repeated field HostDeviceId";
        } else {
            msg->set_hostdeviceid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "DeviceHardwareId") == 0) {
        if (msg->has_devicehardwareid()) {
            state->error = "Duplicate of non-repeated field DeviceHardwareId";
        } else {
            msg->set_devicehardwareid(parseBytes(state->lastData, state));
        }
    } else if (strcmp(tag, "PIN") == 0) {
        if (msg->has_pin()) {
            state->error = "Duplicate of non-repeated field PIN";
        } else {
            msg->set_pin(state->lastData);
        }
    } else {
        processCloseAbstractRequestType(msg->mutable__inherited(), tag, state);
    }
}
void
processCloseRequestPairingRequestTypeCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::ias::RequestPairingRequestType* msg = static_cast<vplex::ias::RequestPairingRequestType*>(state->protoStack.top());
    processCloseRequestPairingRequestType(msg, tag, state);
}

void
processOpenRequestPairingResponseType(vplex::ias::RequestPairingResponseType* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processOpenRequestPairingResponseTypeCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::ias::RequestPairingResponseType* msg = static_cast<vplex::ias::RequestPairingResponseType*>(state->protoStack.top());
    processOpenRequestPairingResponseType(msg, tag, state);
}

void
processCloseRequestPairingResponseType(vplex::ias::RequestPairingResponseType* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "PairingToken") == 0) {
        if (msg->has_pairingtoken()) {
            state->error = "Duplicate of non-repeated field PairingToken";
        } else {
            msg->set_pairingtoken(state->lastData);
        }
    } else {
        processCloseAbstractResponseType(msg->mutable__inherited(), tag, state);
    }
}
void
processCloseRequestPairingResponseTypeCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::ias::RequestPairingResponseType* msg = static_cast<vplex::ias::RequestPairingResponseType*>(state->protoStack.top());
    processCloseRequestPairingResponseType(msg, tag, state);
}

void
processOpenRespondToPairingRequestRequestType(vplex::ias::RespondToPairingRequestRequestType* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processOpenRespondToPairingRequestRequestTypeCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::ias::RespondToPairingRequestRequestType* msg = static_cast<vplex::ias::RespondToPairingRequestRequestType*>(state->protoStack.top());
    processOpenRespondToPairingRequestRequestType(msg, tag, state);
}

void
processCloseRespondToPairingRequestRequestType(vplex::ias::RespondToPairingRequestRequestType* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "TransactionId") == 0) {
        if (msg->has_transactionid()) {
            state->error = "Duplicate of non-repeated field TransactionId";
        } else {
            msg->set_transactionid(state->lastData);
        }
    } else if (strcmp(tag, "AcceptedPairing") == 0) {
        if (msg->has_acceptedpairing()) {
            state->error = "Duplicate of non-repeated field AcceptedPairing";
        } else {
            msg->set_acceptedpairing(parseBool(state->lastData, state));
        }
    } else {
        processCloseAbstractRequestType(msg->mutable__inherited(), tag, state);
    }
}
void
processCloseRespondToPairingRequestRequestTypeCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::ias::RespondToPairingRequestRequestType* msg = static_cast<vplex::ias::RespondToPairingRequestRequestType*>(state->protoStack.top());
    processCloseRespondToPairingRequestRequestType(msg, tag, state);
}

void
processOpenRespondToPairingRequestResponseType(vplex::ias::RespondToPairingRequestResponseType* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processOpenRespondToPairingRequestResponseTypeCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::ias::RespondToPairingRequestResponseType* msg = static_cast<vplex::ias::RespondToPairingRequestResponseType*>(state->protoStack.top());
    processOpenRespondToPairingRequestResponseType(msg, tag, state);
}

void
processCloseRespondToPairingRequestResponseType(vplex::ias::RespondToPairingRequestResponseType* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        processCloseAbstractResponseType(msg->mutable__inherited(), tag, state);
    }
}
void
processCloseRespondToPairingRequestResponseTypeCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::ias::RespondToPairingRequestResponseType* msg = static_cast<vplex::ias::RespondToPairingRequestResponseType*>(state->protoStack.top());
    processCloseRespondToPairingRequestResponseType(msg, tag, state);
}

void
processOpenRequestPairingPinRequestType(vplex::ias::RequestPairingPinRequestType* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processOpenRequestPairingPinRequestTypeCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::ias::RequestPairingPinRequestType* msg = static_cast<vplex::ias::RequestPairingPinRequestType*>(state->protoStack.top());
    processOpenRequestPairingPinRequestType(msg, tag, state);
}

void
processCloseRequestPairingPinRequestType(vplex::ias::RequestPairingPinRequestType* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        processCloseAbstractRequestType(msg->mutable__inherited(), tag, state);
    }
}
void
processCloseRequestPairingPinRequestTypeCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::ias::RequestPairingPinRequestType* msg = static_cast<vplex::ias::RequestPairingPinRequestType*>(state->protoStack.top());
    processCloseRequestPairingPinRequestType(msg, tag, state);
}

void
processOpenRequestPairingPinResponseType(vplex::ias::RequestPairingPinResponseType* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processOpenRequestPairingPinResponseTypeCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::ias::RequestPairingPinResponseType* msg = static_cast<vplex::ias::RequestPairingPinResponseType*>(state->protoStack.top());
    processOpenRequestPairingPinResponseType(msg, tag, state);
}

void
processCloseRequestPairingPinResponseType(vplex::ias::RequestPairingPinResponseType* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "PairingPin") == 0) {
        if (msg->has_pairingpin()) {
            state->error = "Duplicate of non-repeated field PairingPin";
        } else {
            msg->set_pairingpin(state->lastData);
        }
    } else {
        processCloseAbstractResponseType(msg->mutable__inherited(), tag, state);
    }
}
void
processCloseRequestPairingPinResponseTypeCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::ias::RequestPairingPinResponseType* msg = static_cast<vplex::ias::RequestPairingPinResponseType*>(state->protoStack.top());
    processCloseRequestPairingPinResponseType(msg, tag, state);
}

void
processOpenGetPairingStatusRequestType(vplex::ias::GetPairingStatusRequestType* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processOpenGetPairingStatusRequestTypeCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::ias::GetPairingStatusRequestType* msg = static_cast<vplex::ias::GetPairingStatusRequestType*>(state->protoStack.top());
    processOpenGetPairingStatusRequestType(msg, tag, state);
}

void
processCloseGetPairingStatusRequestType(vplex::ias::GetPairingStatusRequestType* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "PairingToken") == 0) {
        if (msg->has_pairingtoken()) {
            state->error = "Duplicate of non-repeated field PairingToken";
        } else {
            msg->set_pairingtoken(state->lastData);
        }
    } else {
        processCloseAbstractRequestType(msg->mutable__inherited(), tag, state);
    }
}
void
processCloseGetPairingStatusRequestTypeCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::ias::GetPairingStatusRequestType* msg = static_cast<vplex::ias::GetPairingStatusRequestType*>(state->protoStack.top());
    processCloseGetPairingStatusRequestType(msg, tag, state);
}

void
processOpenGetPairingStatusResponseType(vplex::ias::GetPairingStatusResponseType* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processOpenGetPairingStatusResponseTypeCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::ias::GetPairingStatusResponseType* msg = static_cast<vplex::ias::GetPairingStatusResponseType*>(state->protoStack.top());
    processOpenGetPairingStatusResponseType(msg, tag, state);
}

void
processCloseGetPairingStatusResponseType(vplex::ias::GetPairingStatusResponseType* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "Status") == 0) {
        if (msg->has_status()) {
            state->error = "Duplicate of non-repeated field Status";
        } else {
            msg->set_status(state->lastData);
        }
    } else if (strcmp(tag, "Username") == 0) {
        if (msg->has_username()) {
            state->error = "Duplicate of non-repeated field Username";
        } else {
            msg->set_username(state->lastData);
        }
    } else {
        processCloseAbstractResponseType(msg->mutable__inherited(), tag, state);
    }
}
void
processCloseGetPairingStatusResponseTypeCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::ias::GetPairingStatusResponseType* msg = static_cast<vplex::ias::GetPairingStatusResponseType*>(state->protoStack.top());
    processCloseGetPairingStatusResponseType(msg, tag, state);
}

void
vplex::ias::writeAbstractRequestType(VPLXmlWriter* writer, const vplex::ias::AbstractRequestType& message)
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
    if (message.has_region()) {
        VPLXmlWriter_InsertSimpleElement(writer, "Region", message.region().c_str());
    }
    if (message.has_country()) {
        VPLXmlWriter_InsertSimpleElement(writer, "Country", message.country().c_str());
    }
    if (message.has_language()) {
        VPLXmlWriter_InsertSimpleElement(writer, "Language", message.language().c_str());
    }
    if (message.has_sessionhandle()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.sessionhandle());
        VPLXmlWriter_InsertSimpleElement(writer, "SessionHandle", val);
    }
    if (message.has_serviceticket()) {
        std::string val = writeBytes(message.serviceticket());
        VPLXmlWriter_InsertSimpleElement(writer, "ServiceTicket", val.c_str());
    }
    if (message.has_serviceid()) {
        VPLXmlWriter_InsertSimpleElement(writer, "ServiceId", message.serviceid().c_str());
    }
}

void
vplex::ias::writeAbstractResponseType(VPLXmlWriter* writer, const vplex::ias::AbstractResponseType& message)
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
    if (message.has_servicestandbymode()) {
        const char* val = (message.servicestandbymode() ? "true" : "false");
        VPLXmlWriter_InsertSimpleElement(writer, "ServiceStandbyMode", val);
    }
}

void
vplex::ias::writeCheckVirtualDeviceCredentialsRenewalRequestType(VPLXmlWriter* writer, const vplex::ias::CheckVirtualDeviceCredentialsRenewalRequestType& message)
{
    if (message.has__inherited()) {
        vplex::ias::writeAbstractRequestType(writer, message._inherited());
    }
    if (message.has_issuedate()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.issuedate());
        VPLXmlWriter_InsertSimpleElement(writer, "IssueDate", val);
    }
    if (message.has_serialnumber()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.serialnumber());
        VPLXmlWriter_InsertSimpleElement(writer, "SerialNumber", val);
    }
}

void
vplex::ias::writeCheckVirtualDeviceCredentialsRenewalResponseType(VPLXmlWriter* writer, const vplex::ias::CheckVirtualDeviceCredentialsRenewalResponseType& message)
{
    if (message.has__inherited()) {
        vplex::ias::writeAbstractResponseType(writer, message._inherited());
    }
    if (message.has_expectedserialnumber()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.expectedserialnumber());
        VPLXmlWriter_InsertSimpleElement(writer, "ExpectedSerialNumber", val);
    }
}

void
vplex::ias::writeStrAttributeType(VPLXmlWriter* writer, const vplex::ias::StrAttributeType& message)
{
    if (message.has_attributename()) {
        VPLXmlWriter_InsertSimpleElement(writer, "AttributeName", message.attributename().c_str());
    }
    if (message.has_attributevalue()) {
        VPLXmlWriter_InsertSimpleElement(writer, "AttributeValue", message.attributevalue().c_str());
    }
}

void
vplex::ias::writeGetSessionKeyRequestType(VPLXmlWriter* writer, const vplex::ias::GetSessionKeyRequestType& message)
{
    if (message.has__inherited()) {
        vplex::ias::writeAbstractRequestType(writer, message._inherited());
    }
    if (message.has_type()) {
        VPLXmlWriter_InsertSimpleElement(writer, "Type", message.type().c_str());
    }
    for (int i = 0; i < message.keyattributes_size(); i++) {
        VPLXmlWriter_OpenTagV(writer, "KeyAttributes", 0);
        vplex::ias::writeStrAttributeType(writer, message.keyattributes(i));
        VPLXmlWriter_CloseTag(writer);
    }
    if (message.has_encryptedsessionkey()) {
        std::string val = writeBytes(message.encryptedsessionkey());
        VPLXmlWriter_InsertSimpleElement(writer, "EncryptedSessionKey", val.c_str());
    }
}

void
vplex::ias::writeGetSessionKeyResponseType(VPLXmlWriter* writer, const vplex::ias::GetSessionKeyResponseType& message)
{
    if (message.has__inherited()) {
        vplex::ias::writeAbstractResponseType(writer, message._inherited());
    }
    if (message.has_sessionkey()) {
        std::string val = writeBytes(message.sessionkey());
        VPLXmlWriter_InsertSimpleElement(writer, "SessionKey", val.c_str());
    }
    if (message.has_encryptedsessionkey()) {
        std::string val = writeBytes(message.encryptedsessionkey());
        VPLXmlWriter_InsertSimpleElement(writer, "EncryptedSessionKey", val.c_str());
    }
    if (message.has_instanceid()) {
        char val[15];
        sprintf(val, FMTs32, (s32)message.instanceid());
        VPLXmlWriter_InsertSimpleElement(writer, "InstanceId", val);
    }
}

void
vplex::ias::writeLoginRequestType(VPLXmlWriter* writer, const vplex::ias::LoginRequestType& message)
{
    if (message.has__inherited()) {
        vplex::ias::writeAbstractRequestType(writer, message._inherited());
    }
    if (message.has_username()) {
        VPLXmlWriter_InsertSimpleElement(writer, "Username", message.username().c_str());
    }
    if (message.has_namespace_()) {
        VPLXmlWriter_InsertSimpleElement(writer, "Namespace", message.namespace_().c_str());
    }
    if (message.has_password()) {
        VPLXmlWriter_InsertSimpleElement(writer, "Password", message.password().c_str());
    }
    if (message.has_weaktoken()) {
        VPLXmlWriter_InsertSimpleElement(writer, "WeakToken", message.weaktoken().c_str());
    }
    if (message.has_pairingtoken()) {
        VPLXmlWriter_InsertSimpleElement(writer, "PairingToken", message.pairingtoken().c_str());
    }
    if (message.has_aceulaagreed()) {
        const char* val = (message.aceulaagreed() ? "true" : "false");
        VPLXmlWriter_InsertSimpleElement(writer, "ACEulaAgreed", val);
    }
}

void
vplex::ias::writeLoginResponseType(VPLXmlWriter* writer, const vplex::ias::LoginResponseType& message)
{
    if (message.has__inherited()) {
        vplex::ias::writeAbstractResponseType(writer, message._inherited());
    }
    if (message.has_sessionhandle()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.sessionhandle());
        VPLXmlWriter_InsertSimpleElement(writer, "SessionHandle", val);
    }
    if (message.has_sessionsecret()) {
        std::string val = writeBytes(message.sessionsecret());
        VPLXmlWriter_InsertSimpleElement(writer, "SessionSecret", val.c_str());
    }
    if (message.has_accountid()) {
        VPLXmlWriter_InsertSimpleElement(writer, "AccountId", message.accountid().c_str());
    }
    if (message.has_userid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.userid());
        VPLXmlWriter_InsertSimpleElement(writer, "UserId", val);
    }
    if (message.has_displayname()) {
        VPLXmlWriter_InsertSimpleElement(writer, "DisplayName", message.displayname().c_str());
    }
    if (message.has_weaktoken()) {
        VPLXmlWriter_InsertSimpleElement(writer, "WeakToken", message.weaktoken().c_str());
    }
    if (message.has_oldfgsessionhandle()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.oldfgsessionhandle());
        VPLXmlWriter_InsertSimpleElement(writer, "OldFgSessionHandle", val);
    }
    if (message.has_storageregion()) {
        VPLXmlWriter_InsertSimpleElement(writer, "StorageRegion", message.storageregion().c_str());
    }
    if (message.has_storageclusterid()) {
        char val[30];
        sprintf(val, FMTs64, message.storageclusterid());
        VPLXmlWriter_InsertSimpleElement(writer, "StorageClusterId", val);
    }
    if (message.has_persistentcredentials()) {
        VPLXmlWriter_InsertSimpleElement(writer, "persistentCredentials", message.persistentcredentials().c_str());
    }
}

void
vplex::ias::writeLogoutRequestType(VPLXmlWriter* writer, const vplex::ias::LogoutRequestType& message)
{
    if (message.has__inherited()) {
        vplex::ias::writeAbstractRequestType(writer, message._inherited());
    }
}

void
vplex::ias::writeLogoutResponseType(VPLXmlWriter* writer, const vplex::ias::LogoutResponseType& message)
{
    if (message.has__inherited()) {
        vplex::ias::writeAbstractResponseType(writer, message._inherited());
    }
}

void
vplex::ias::writeRegisterVirtualDeviceRequestType(VPLXmlWriter* writer, const vplex::ias::RegisterVirtualDeviceRequestType& message)
{
    if (message.has__inherited()) {
        vplex::ias::writeAbstractRequestType(writer, message._inherited());
    }
    if (message.has_username()) {
        VPLXmlWriter_InsertSimpleElement(writer, "Username", message.username().c_str());
    }
    if (message.has_password()) {
        VPLXmlWriter_InsertSimpleElement(writer, "Password", message.password().c_str());
    }
    if (message.has_hardwareinfo()) {
        std::string val = writeBytes(message.hardwareinfo());
        VPLXmlWriter_InsertSimpleElement(writer, "HardwareInfo", val.c_str());
    }
    if (message.has_devicename()) {
        VPLXmlWriter_InsertSimpleElement(writer, "DeviceName", message.devicename().c_str());
    }
    if (message.has_namespace_()) {
        VPLXmlWriter_InsertSimpleElement(writer, "Namespace", message.namespace_().c_str());
    }
    if (message.has_weaktoken()) {
        VPLXmlWriter_InsertSimpleElement(writer, "WeakToken", message.weaktoken().c_str());
    }
    if (message.has_pairingtoken()) {
        VPLXmlWriter_InsertSimpleElement(writer, "PairingToken", message.pairingtoken().c_str());
    }
}

void
vplex::ias::writeRegisterVirtualDeviceResponseType(VPLXmlWriter* writer, const vplex::ias::RegisterVirtualDeviceResponseType& message)
{
    if (message.has__inherited()) {
        vplex::ias::writeAbstractResponseType(writer, message._inherited());
    }
    if (message.has_renewaltoken()) {
        std::string val = writeBytes(message.renewaltoken());
        VPLXmlWriter_InsertSimpleElement(writer, "RenewalToken", val.c_str());
    }
}

void
vplex::ias::writeRenewVirtualDeviceCredentialsRequestType(VPLXmlWriter* writer, const vplex::ias::RenewVirtualDeviceCredentialsRequestType& message)
{
    if (message.has__inherited()) {
        vplex::ias::writeAbstractRequestType(writer, message._inherited());
    }
    if (message.has_serialnumber()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.serialnumber());
        VPLXmlWriter_InsertSimpleElement(writer, "SerialNumber", val);
    }
    if (message.has_issuedate()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.issuedate());
        VPLXmlWriter_InsertSimpleElement(writer, "IssueDate", val);
    }
    if (message.has_renewaltoken()) {
        std::string val = writeBytes(message.renewaltoken());
        VPLXmlWriter_InsertSimpleElement(writer, "RenewalToken", val.c_str());
    }
}

void
vplex::ias::writeRenewVirtualDeviceCredentialsResponseType(VPLXmlWriter* writer, const vplex::ias::RenewVirtualDeviceCredentialsResponseType& message)
{
    if (message.has__inherited()) {
        vplex::ias::writeAbstractResponseType(writer, message._inherited());
    }
    if (message.has_secretdevicecredentials()) {
        std::string val = writeBytes(message.secretdevicecredentials());
        VPLXmlWriter_InsertSimpleElement(writer, "SecretDeviceCredentials", val.c_str());
    }
    if (message.has_cleardevicecredentials()) {
        std::string val = writeBytes(message.cleardevicecredentials());
        VPLXmlWriter_InsertSimpleElement(writer, "ClearDeviceCredentials", val.c_str());
    }
    if (message.has_renewaltoken()) {
        std::string val = writeBytes(message.renewaltoken());
        VPLXmlWriter_InsertSimpleElement(writer, "RenewalToken", val.c_str());
    }
    if (message.has_attestprogram()) {
        std::string val = writeBytes(message.attestprogram());
        VPLXmlWriter_InsertSimpleElement(writer, "AttestProgram", val.c_str());
    }
    if (message.has_issuedate()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.issuedate());
        VPLXmlWriter_InsertSimpleElement(writer, "IssueDate", val);
    }
    if (message.has_serialnumber()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.serialnumber());
        VPLXmlWriter_InsertSimpleElement(writer, "SerialNumber", val);
    }
    if (message.has_attesttmd()) {
        std::string val = writeBytes(message.attesttmd());
        VPLXmlWriter_InsertSimpleElement(writer, "AttestTMD", val.c_str());
    }
    if (message.has_devicecert()) {
        std::string val = writeBytes(message.devicecert());
        VPLXmlWriter_InsertSimpleElement(writer, "DeviceCert", val.c_str());
    }
    if (message.has_platformkey()) {
        std::string val = writeBytes(message.platformkey());
        VPLXmlWriter_InsertSimpleElement(writer, "PlatformKey", val.c_str());
    }
}

void
vplex::ias::writeGetServerKeyRequestType(VPLXmlWriter* writer, const vplex::ias::GetServerKeyRequestType& message)
{
    if (message.has__inherited()) {
        vplex::ias::writeAbstractRequestType(writer, message._inherited());
    }
    if (message.has_userid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.userid());
        VPLXmlWriter_InsertSimpleElement(writer, "UserId", val);
    }
}

void
vplex::ias::writeGetServerKeyResponseType(VPLXmlWriter* writer, const vplex::ias::GetServerKeyResponseType& message)
{
    if (message.has__inherited()) {
        vplex::ias::writeAbstractResponseType(writer, message._inherited());
    }
    if (message.has_serverkey()) {
        std::string val = writeBytes(message.serverkey());
        VPLXmlWriter_InsertSimpleElement(writer, "ServerKey", val.c_str());
    }
}

void
vplex::ias::writeRequestPairingRequestType(VPLXmlWriter* writer, const vplex::ias::RequestPairingRequestType& message)
{
    if (message.has__inherited()) {
        vplex::ias::writeAbstractRequestType(writer, message._inherited());
    }
    if (message.has_hosthardwareid()) {
        std::string val = writeBytes(message.hosthardwareid());
        VPLXmlWriter_InsertSimpleElement(writer, "HostHardwareId", val.c_str());
    }
    if (message.has_hostdeviceid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.hostdeviceid());
        VPLXmlWriter_InsertSimpleElement(writer, "HostDeviceId", val);
    }
    if (message.has_devicehardwareid()) {
        std::string val = writeBytes(message.devicehardwareid());
        VPLXmlWriter_InsertSimpleElement(writer, "DeviceHardwareId", val.c_str());
    }
    if (message.has_pin()) {
        VPLXmlWriter_InsertSimpleElement(writer, "PIN", message.pin().c_str());
    }
    for (int i = 0; i < message.pairingattributes_size(); i++) {
        VPLXmlWriter_OpenTagV(writer, "PairingAttributes", 0);
        vplex::ias::writeStrAttributeType(writer, message.pairingattributes(i));
        VPLXmlWriter_CloseTag(writer);
    }
}

void
vplex::ias::writeRequestPairingResponseType(VPLXmlWriter* writer, const vplex::ias::RequestPairingResponseType& message)
{
    if (message.has__inherited()) {
        vplex::ias::writeAbstractResponseType(writer, message._inherited());
    }
    if (message.has_pairingtoken()) {
        VPLXmlWriter_InsertSimpleElement(writer, "PairingToken", message.pairingtoken().c_str());
    }
}

void
vplex::ias::writeRespondToPairingRequestRequestType(VPLXmlWriter* writer, const vplex::ias::RespondToPairingRequestRequestType& message)
{
    if (message.has__inherited()) {
        vplex::ias::writeAbstractRequestType(writer, message._inherited());
    }
    if (message.has_transactionid()) {
        VPLXmlWriter_InsertSimpleElement(writer, "TransactionId", message.transactionid().c_str());
    }
    if (message.has_acceptedpairing()) {
        const char* val = (message.acceptedpairing() ? "true" : "false");
        VPLXmlWriter_InsertSimpleElement(writer, "AcceptedPairing", val);
    }
}

void
vplex::ias::writeRespondToPairingRequestResponseType(VPLXmlWriter* writer, const vplex::ias::RespondToPairingRequestResponseType& message)
{
    if (message.has__inherited()) {
        vplex::ias::writeAbstractResponseType(writer, message._inherited());
    }
}

void
vplex::ias::writeRequestPairingPinRequestType(VPLXmlWriter* writer, const vplex::ias::RequestPairingPinRequestType& message)
{
    if (message.has__inherited()) {
        vplex::ias::writeAbstractRequestType(writer, message._inherited());
    }
}

void
vplex::ias::writeRequestPairingPinResponseType(VPLXmlWriter* writer, const vplex::ias::RequestPairingPinResponseType& message)
{
    if (message.has__inherited()) {
        vplex::ias::writeAbstractResponseType(writer, message._inherited());
    }
    if (message.has_pairingpin()) {
        VPLXmlWriter_InsertSimpleElement(writer, "PairingPin", message.pairingpin().c_str());
    }
}

void
vplex::ias::writeGetPairingStatusRequestType(VPLXmlWriter* writer, const vplex::ias::GetPairingStatusRequestType& message)
{
    if (message.has__inherited()) {
        vplex::ias::writeAbstractRequestType(writer, message._inherited());
    }
    if (message.has_pairingtoken()) {
        VPLXmlWriter_InsertSimpleElement(writer, "PairingToken", message.pairingtoken().c_str());
    }
}

void
vplex::ias::writeGetPairingStatusResponseType(VPLXmlWriter* writer, const vplex::ias::GetPairingStatusResponseType& message)
{
    if (message.has__inherited()) {
        vplex::ias::writeAbstractResponseType(writer, message._inherited());
    }
    if (message.has_status()) {
        VPLXmlWriter_InsertSimpleElement(writer, "Status", message.status().c_str());
    }
    if (message.has_username()) {
        VPLXmlWriter_InsertSimpleElement(writer, "Username", message.username().c_str());
    }
}

