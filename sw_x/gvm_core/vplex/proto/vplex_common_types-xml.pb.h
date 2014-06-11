// GENERATED FROM vplex_common_types.proto, DO NOT EDIT
#ifndef __vplex_common_types_xml_pb_h__
#define __vplex_common_types_xml_pb_h__

#include <vplex_xml_writer.h>
#include <ProtoXmlParseState.h>
#include "vplex_common_types.pb.h"

namespace vplex {
namespace common {

class ParseStateContentRating : public ProtoXmlParseState
{
public:
    ParseStateContentRating(const char* outerTag, ContentRating* message);
};

void RegisterHandlers_vplex_common_types(ProtoXmlParseStateImpl*);


void writeContentRating(VPLXmlWriter* writer, const ContentRating& message);

} // namespace vplex
} // namespace common
#endif /* __vplex_common_types_xml_pb_h__ */
