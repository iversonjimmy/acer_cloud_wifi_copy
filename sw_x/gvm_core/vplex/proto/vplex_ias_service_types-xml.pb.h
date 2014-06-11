// GENERATED FROM vplex_ias_service_types.proto, DO NOT EDIT
#ifndef __vplex_ias_service_types_xml_pb_h__
#define __vplex_ias_service_types_xml_pb_h__

#include <vplex_xml_writer.h>
#include <ProtoXmlParseState.h>
#include "vplex_ias_service_types.pb.h"

namespace vplex {
namespace ias {

class ParseStateAbstractRequestType : public ProtoXmlParseState
{
public:
    ParseStateAbstractRequestType(const char* outerTag, AbstractRequestType* message);
};

class ParseStateAbstractResponseType : public ProtoXmlParseState
{
public:
    ParseStateAbstractResponseType(const char* outerTag, AbstractResponseType* message);
};

class ParseStateCheckVirtualDeviceCredentialsRenewalRequestType : public ProtoXmlParseState
{
public:
    ParseStateCheckVirtualDeviceCredentialsRenewalRequestType(const char* outerTag, CheckVirtualDeviceCredentialsRenewalRequestType* message);
};

class ParseStateCheckVirtualDeviceCredentialsRenewalResponseType : public ProtoXmlParseState
{
public:
    ParseStateCheckVirtualDeviceCredentialsRenewalResponseType(const char* outerTag, CheckVirtualDeviceCredentialsRenewalResponseType* message);
};

class ParseStateStrAttributeType : public ProtoXmlParseState
{
public:
    ParseStateStrAttributeType(const char* outerTag, StrAttributeType* message);
};

class ParseStateGetSessionKeyRequestType : public ProtoXmlParseState
{
public:
    ParseStateGetSessionKeyRequestType(const char* outerTag, GetSessionKeyRequestType* message);
};

class ParseStateGetSessionKeyResponseType : public ProtoXmlParseState
{
public:
    ParseStateGetSessionKeyResponseType(const char* outerTag, GetSessionKeyResponseType* message);
};

class ParseStateLoginRequestType : public ProtoXmlParseState
{
public:
    ParseStateLoginRequestType(const char* outerTag, LoginRequestType* message);
};

class ParseStateLoginResponseType : public ProtoXmlParseState
{
public:
    ParseStateLoginResponseType(const char* outerTag, LoginResponseType* message);
};

class ParseStateLogoutRequestType : public ProtoXmlParseState
{
public:
    ParseStateLogoutRequestType(const char* outerTag, LogoutRequestType* message);
};

class ParseStateLogoutResponseType : public ProtoXmlParseState
{
public:
    ParseStateLogoutResponseType(const char* outerTag, LogoutResponseType* message);
};

class ParseStateRegisterVirtualDeviceRequestType : public ProtoXmlParseState
{
public:
    ParseStateRegisterVirtualDeviceRequestType(const char* outerTag, RegisterVirtualDeviceRequestType* message);
};

class ParseStateRegisterVirtualDeviceResponseType : public ProtoXmlParseState
{
public:
    ParseStateRegisterVirtualDeviceResponseType(const char* outerTag, RegisterVirtualDeviceResponseType* message);
};

class ParseStateRenewVirtualDeviceCredentialsRequestType : public ProtoXmlParseState
{
public:
    ParseStateRenewVirtualDeviceCredentialsRequestType(const char* outerTag, RenewVirtualDeviceCredentialsRequestType* message);
};

class ParseStateRenewVirtualDeviceCredentialsResponseType : public ProtoXmlParseState
{
public:
    ParseStateRenewVirtualDeviceCredentialsResponseType(const char* outerTag, RenewVirtualDeviceCredentialsResponseType* message);
};

class ParseStateGetServerKeyRequestType : public ProtoXmlParseState
{
public:
    ParseStateGetServerKeyRequestType(const char* outerTag, GetServerKeyRequestType* message);
};

class ParseStateGetServerKeyResponseType : public ProtoXmlParseState
{
public:
    ParseStateGetServerKeyResponseType(const char* outerTag, GetServerKeyResponseType* message);
};

class ParseStateRequestPairingRequestType : public ProtoXmlParseState
{
public:
    ParseStateRequestPairingRequestType(const char* outerTag, RequestPairingRequestType* message);
};

class ParseStateRequestPairingResponseType : public ProtoXmlParseState
{
public:
    ParseStateRequestPairingResponseType(const char* outerTag, RequestPairingResponseType* message);
};

class ParseStateRespondToPairingRequestRequestType : public ProtoXmlParseState
{
public:
    ParseStateRespondToPairingRequestRequestType(const char* outerTag, RespondToPairingRequestRequestType* message);
};

class ParseStateRespondToPairingRequestResponseType : public ProtoXmlParseState
{
public:
    ParseStateRespondToPairingRequestResponseType(const char* outerTag, RespondToPairingRequestResponseType* message);
};

class ParseStateRequestPairingPinRequestType : public ProtoXmlParseState
{
public:
    ParseStateRequestPairingPinRequestType(const char* outerTag, RequestPairingPinRequestType* message);
};

class ParseStateRequestPairingPinResponseType : public ProtoXmlParseState
{
public:
    ParseStateRequestPairingPinResponseType(const char* outerTag, RequestPairingPinResponseType* message);
};

class ParseStateGetPairingStatusRequestType : public ProtoXmlParseState
{
public:
    ParseStateGetPairingStatusRequestType(const char* outerTag, GetPairingStatusRequestType* message);
};

class ParseStateGetPairingStatusResponseType : public ProtoXmlParseState
{
public:
    ParseStateGetPairingStatusResponseType(const char* outerTag, GetPairingStatusResponseType* message);
};

void RegisterHandlers_vplex_ias_service_types(ProtoXmlParseStateImpl*);


void writeAbstractRequestType(VPLXmlWriter* writer, const AbstractRequestType& message);
void writeAbstractResponseType(VPLXmlWriter* writer, const AbstractResponseType& message);
void writeCheckVirtualDeviceCredentialsRenewalRequestType(VPLXmlWriter* writer, const CheckVirtualDeviceCredentialsRenewalRequestType& message);
void writeCheckVirtualDeviceCredentialsRenewalResponseType(VPLXmlWriter* writer, const CheckVirtualDeviceCredentialsRenewalResponseType& message);
void writeStrAttributeType(VPLXmlWriter* writer, const StrAttributeType& message);
void writeGetSessionKeyRequestType(VPLXmlWriter* writer, const GetSessionKeyRequestType& message);
void writeGetSessionKeyResponseType(VPLXmlWriter* writer, const GetSessionKeyResponseType& message);
void writeLoginRequestType(VPLXmlWriter* writer, const LoginRequestType& message);
void writeLoginResponseType(VPLXmlWriter* writer, const LoginResponseType& message);
void writeLogoutRequestType(VPLXmlWriter* writer, const LogoutRequestType& message);
void writeLogoutResponseType(VPLXmlWriter* writer, const LogoutResponseType& message);
void writeRegisterVirtualDeviceRequestType(VPLXmlWriter* writer, const RegisterVirtualDeviceRequestType& message);
void writeRegisterVirtualDeviceResponseType(VPLXmlWriter* writer, const RegisterVirtualDeviceResponseType& message);
void writeRenewVirtualDeviceCredentialsRequestType(VPLXmlWriter* writer, const RenewVirtualDeviceCredentialsRequestType& message);
void writeRenewVirtualDeviceCredentialsResponseType(VPLXmlWriter* writer, const RenewVirtualDeviceCredentialsResponseType& message);
void writeGetServerKeyRequestType(VPLXmlWriter* writer, const GetServerKeyRequestType& message);
void writeGetServerKeyResponseType(VPLXmlWriter* writer, const GetServerKeyResponseType& message);
void writeRequestPairingRequestType(VPLXmlWriter* writer, const RequestPairingRequestType& message);
void writeRequestPairingResponseType(VPLXmlWriter* writer, const RequestPairingResponseType& message);
void writeRespondToPairingRequestRequestType(VPLXmlWriter* writer, const RespondToPairingRequestRequestType& message);
void writeRespondToPairingRequestResponseType(VPLXmlWriter* writer, const RespondToPairingRequestResponseType& message);
void writeRequestPairingPinRequestType(VPLXmlWriter* writer, const RequestPairingPinRequestType& message);
void writeRequestPairingPinResponseType(VPLXmlWriter* writer, const RequestPairingPinResponseType& message);
void writeGetPairingStatusRequestType(VPLXmlWriter* writer, const GetPairingStatusRequestType& message);
void writeGetPairingStatusResponseType(VPLXmlWriter* writer, const GetPairingStatusResponseType& message);

} // namespace vplex
} // namespace ias
#endif /* __vplex_ias_service_types_xml_pb_h__ */
