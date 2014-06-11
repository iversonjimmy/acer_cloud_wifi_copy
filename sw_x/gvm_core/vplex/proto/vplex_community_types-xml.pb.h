// GENERATED FROM vplex_community_types.proto, DO NOT EDIT
#ifndef __vplex_community_types_xml_pb_h__
#define __vplex_community_types_xml_pb_h__

#include <vplex_xml_writer.h>
#include <ProtoXmlParseState.h>
#include "vplex_community_types.pb.h"

namespace vplex {
namespace community {

void RegisterHandlers_vplex_community_types(ProtoXmlParseStateImpl*);

vplex::community::OnlineState_t parseOnlineState_t(const std::string& data, ProtoXmlParseStateImpl* state);
const char* writeOnlineState_t(vplex::community::OnlineState_t val);


} // namespace vplex
} // namespace community
#endif /* __vplex_community_types_xml_pb_h__ */
