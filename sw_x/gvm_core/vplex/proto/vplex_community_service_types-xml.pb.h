// GENERATED FROM vplex_community_service_types.proto, DO NOT EDIT
#ifndef __vplex_community_service_types_xml_pb_h__
#define __vplex_community_service_types_xml_pb_h__

#include <vplex_xml_writer.h>
#include <ProtoXmlParseState.h>
#include "vplex_community_service_types.pb.h"

namespace vplex {
namespace community {

class ParseStateSessionInfo : public ProtoXmlParseState
{
public:
    ParseStateSessionInfo(const char* outerTag, SessionInfo* message);
};

void RegisterHandlers_vplex_community_service_types(ProtoXmlParseStateImpl*);


void writeSessionInfo(VPLXmlWriter* writer, const SessionInfo& message);

} // namespace vplex
} // namespace community
#endif /* __vplex_community_service_types_xml_pb_h__ */
