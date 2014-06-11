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

#ifndef __PROTO_XML_PARSE_STATE_H__
#define __PROTO_XML_PARSE_STATE_H__

#include <vplex_xml_reader.h>

#include <memory>
#include <string>

#include <google/protobuf/message.h>

/// Typedef so that at compile time we can choose the full or lite runtime
typedef google::protobuf::Message ProtoMessage;

class ProtoXmlParseStateImpl;

/**
 * Base class for generated code to parse protocol buffer from XML.  A
 * generated subclass is instantiated with the protobuf object to fill in.
 * This class then exports a VPL XML reader which can be used to parse the
 * XML.  Afterwards, the object will be filled in or this class will
 * indicate an error.  The caller should also check that the object is
 * initialized with all requisite fields.
 *
 * Example:
 * <pre>
 *   MyProtobufType proto;
 *   ParseStateMyProtobufType state("MyWsdlServiceName", &proto);
 *   _VPLXmlReader& reader = state.reader();
 *   // Use VPL to parse XML with reader here
 *   if (state.hasError()) {
 *       // There was a parsing error
 *   } else {
 *       if (proto.IsInitialized()) {
 *           // Success
 *       } else {
 *           // Error, missing required fields
 *       }
 *   }
 * </pre>
 */
class ProtoXmlParseState
{
public:
    _VPLXmlReader& reader(void);
    bool hasError(void);
    const std::string& errorDetail(void);

protected:
    /**
     * Constructor used by subclasses.
     * @param outerTag This tag is the innermost wrapper containing the
     *     desired message type.  It is provided so the parser may ignore
     *     extra document structure surrounding the item of interest.
     * @param message Pointer to the object to parse the message into.
     */
    ProtoXmlParseState(const char* outerTag, ProtoMessage* message);
    virtual ~ProtoXmlParseState(void);
    ProtoXmlParseStateImpl* impl;
};

#endif /* __PROTO_XML_PARSE_STATE_H__ */
