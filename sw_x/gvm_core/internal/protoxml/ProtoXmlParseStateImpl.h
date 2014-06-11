/*
 *               Copyright (C) 2010, BroadOn Communications Corp.
 *
 *  These coded instructions, statements, and computer programs contain
 *  unpublished  proprietary information of BroadOn Communications Corp.,
 *  and  are protected by Federal copyright law. They may not be disclosed
 *  to  third  parties or copied or duplicated in any form, in whole or in
 *  part, without the prior written consent of BroadOn Communications Corp.
 *
 */

#ifndef __PROTO_XML_PARSE_STATE_IMPL_H__
#define __PROTO_XML_PARSE_STATE_IMPL_H__

#include "vplex_plat.h"

#include <map>
#include <stack>
#include <string>

#include "ProtoXmlParseState.h"

#define PARSE_BUFFER_SIZE 512

typedef void (*TagHandler)(const char*, ProtoXmlParseStateImpl*);

/// Helper to parse a boolean.  Records errors in \a state.
bool parseBool(std::string& data, ProtoXmlParseStateImpl* state);

/// Helper to parse a double.  Records errors in \a state.
double parseDouble(std::string& data, ProtoXmlParseStateImpl* state);

/// Helper to parse a float.  Records errors in \a state.
float parseFloat(std::string& data, ProtoXmlParseStateImpl* state);

/// Helper to parse an int32.  Records errors in \a state.
int32_t parseInt32(std::string& data, ProtoXmlParseStateImpl* state);

/// Helper to parse an int64.  Records errors in \a state.
int64_t parseInt64(std::string& data, ProtoXmlParseStateImpl* state);

/// Helper to base64 decode a bytestring.  Records errors in \a state.
std::string parseBytes(const std::string& data, ProtoXmlParseStateImpl* state);

/// Helper to base64 encode a bytestring.
std::string writeBytes(const std::string& data);

/**
 * This class should only be used by generated code.  It contains and fully
 * exposes the base implementation of \a ProtoXmlParseState for use by
 * (non-friend) generated code.
 */
class ProtoXmlParseStateImpl
{
public:
    ProtoXmlParseStateImpl(const char* outerTag, ProtoMessage* message);
    _VPLXmlReader& reader(void);
    bool hasError(void);
    const std::string& errorDetail(void);

    /// Contains tag open handlers by protobuf message type.  Parsing
    /// dispatches by the innermost type currently being parsed.
    std::map<std::string, TagHandler> openHandlers;

    /// Contains tag close handlers by protobuf message type.
    std::map<std::string, TagHandler> closeHandlers;

    /// The current stack of nested protobuf messages being populated.  When
    /// a nested message is entered, the new object to populate is placed on
    /// the stack, and removed when the nested object is fully read.
    std::stack<ProtoMessage*> protoStack;

    /// Parallel to #protoStack, this contains the tag depth at which each
    /// nested message was entered.  When the tag depth decreases below this
    /// level the nested object is complete.  This stack may be empty if
    /// \a outerTag hasn't been seen yet.
    std::stack<int> protoStartDepth;

    /// The tag name which will signify the start of the message object.
    /// Document structure which is outside this tag is not interpreted.
    std::string outerTag;

    /// The current tag depth.  Starts at zero (not inside any tags).
    int tagDepth;

    /// The last data element encountered.  Close handlers in the generated
    /// code will parse the data content as appropriate.
    std::string lastData;

    /// Error detail, nonempty if a parse error has occurred.
    std::string error;

    /// Buffer space for VPL reader
    int8_t parseBuffer[PARSE_BUFFER_SIZE+1];

    /// Reader to be exported through \a ProtoXmlParseState.
    _VPLXmlReader xmlReader;
};

#endif /* __PROTO_XML_PARSE_STATE_IMPL_H__ */
