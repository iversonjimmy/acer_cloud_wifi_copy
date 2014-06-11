// GENERATED FROM vplex_nus_service_types.proto, DO NOT EDIT
#ifndef __vplex_nus_service_types_xml_pb_h__
#define __vplex_nus_service_types_xml_pb_h__

#include <vplex_xml_writer.h>
#include <ProtoXmlParseState.h>
#include "vplex_nus_service_types.pb.h"

namespace vplex {
namespace nus {

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

class ParseStateTitleVersionType : public ProtoXmlParseState
{
public:
    ParseStateTitleVersionType(const char* outerTag, TitleVersionType* message);
};

class ParseStateGetSystemUpdateRequestType : public ProtoXmlParseState
{
public:
    ParseStateGetSystemUpdateRequestType(const char* outerTag, GetSystemUpdateRequestType* message);
};

class ParseStateGetSystemUpdateResponseType : public ProtoXmlParseState
{
public:
    ParseStateGetSystemUpdateResponseType(const char* outerTag, GetSystemUpdateResponseType* message);
};

class ParseStateGetSystemTMDRequestType : public ProtoXmlParseState
{
public:
    ParseStateGetSystemTMDRequestType(const char* outerTag, GetSystemTMDRequestType* message);
};

class ParseStateGetSystemTMDResponseType : public ProtoXmlParseState
{
public:
    ParseStateGetSystemTMDResponseType(const char* outerTag, GetSystemTMDResponseType* message);
};

class ParseStateGetSystemPersonalizedETicketRequestType : public ProtoXmlParseState
{
public:
    ParseStateGetSystemPersonalizedETicketRequestType(const char* outerTag, GetSystemPersonalizedETicketRequestType* message);
};

class ParseStateGetSystemPersonalizedETicketResponseType : public ProtoXmlParseState
{
public:
    ParseStateGetSystemPersonalizedETicketResponseType(const char* outerTag, GetSystemPersonalizedETicketResponseType* message);
};

class ParseStateGetSystemCommonETicketRequestType : public ProtoXmlParseState
{
public:
    ParseStateGetSystemCommonETicketRequestType(const char* outerTag, GetSystemCommonETicketRequestType* message);
};

class ParseStateGetSystemCommonETicketResponseType : public ProtoXmlParseState
{
public:
    ParseStateGetSystemCommonETicketResponseType(const char* outerTag, GetSystemCommonETicketResponseType* message);
};

void RegisterHandlers_vplex_nus_service_types(ProtoXmlParseStateImpl*);


void writeAbstractRequestType(VPLXmlWriter* writer, const AbstractRequestType& message);
void writeAbstractResponseType(VPLXmlWriter* writer, const AbstractResponseType& message);
void writeTitleVersionType(VPLXmlWriter* writer, const TitleVersionType& message);
void writeGetSystemUpdateRequestType(VPLXmlWriter* writer, const GetSystemUpdateRequestType& message);
void writeGetSystemUpdateResponseType(VPLXmlWriter* writer, const GetSystemUpdateResponseType& message);
void writeGetSystemTMDRequestType(VPLXmlWriter* writer, const GetSystemTMDRequestType& message);
void writeGetSystemTMDResponseType(VPLXmlWriter* writer, const GetSystemTMDResponseType& message);
void writeGetSystemPersonalizedETicketRequestType(VPLXmlWriter* writer, const GetSystemPersonalizedETicketRequestType& message);
void writeGetSystemPersonalizedETicketResponseType(VPLXmlWriter* writer, const GetSystemPersonalizedETicketResponseType& message);
void writeGetSystemCommonETicketRequestType(VPLXmlWriter* writer, const GetSystemCommonETicketRequestType& message);
void writeGetSystemCommonETicketResponseType(VPLXmlWriter* writer, const GetSystemCommonETicketResponseType& message);

} // namespace vplex
} // namespace nus
#endif /* __vplex_nus_service_types_xml_pb_h__ */
