// GENERATED FROM vplex_common_types.proto, DO NOT EDIT

#include "vplex_common_types-xml.pb.h"

#include <string.h>

#include <ProtoXmlParseStateImpl.h>

static void processOpenContentRating(vplex::common::ContentRating* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenContentRatingCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseContentRating(vplex::common::ContentRating* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseContentRatingCb(const char* tag, ProtoXmlParseStateImpl* state);

vplex::common::ParseStateContentRating::ParseStateContentRating(const char* outerTag, ContentRating* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_common_types(impl);
}

void
vplex::common::RegisterHandlers_vplex_common_types(ProtoXmlParseStateImpl* impl)
{
    if (impl->openHandlers.find("vplex.common.ContentRating") == impl->openHandlers.end()) {
        impl->openHandlers["vplex.common.ContentRating"] = processOpenContentRatingCb;
        impl->closeHandlers["vplex.common.ContentRating"] = processCloseContentRatingCb;
    }
}

void
processOpenContentRating(vplex::common::ContentRating* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processOpenContentRatingCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::common::ContentRating* msg = static_cast<vplex::common::ContentRating*>(state->protoStack.top());
    processOpenContentRating(msg, tag, state);
}

void
processCloseContentRating(vplex::common::ContentRating* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "rating_system") == 0) {
        if (msg->has_rating_system()) {
            state->error = "Duplicate of non-repeated field rating_system";
        } else {
            msg->set_rating_system(state->lastData);
        }
    } else if (strcmp(tag, "rating_value") == 0) {
        if (msg->has_rating_value()) {
            state->error = "Duplicate of non-repeated field rating_value";
        } else {
            msg->set_rating_value(state->lastData);
        }
    } else if (strcmp(tag, "content_descriptors") == 0) {
        msg->add_content_descriptors(state->lastData);
    } else {
        (void)msg;
    }
}
void
processCloseContentRatingCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::common::ContentRating* msg = static_cast<vplex::common::ContentRating*>(state->protoStack.top());
    processCloseContentRating(msg, tag, state);
}

void
vplex::common::writeContentRating(VPLXmlWriter* writer, const vplex::common::ContentRating& message)
{
    if (message.has_rating_system()) {
        VPLXmlWriter_InsertSimpleElement(writer, "rating_system", message.rating_system().c_str());
    }
    if (message.has_rating_value()) {
        VPLXmlWriter_InsertSimpleElement(writer, "rating_value", message.rating_value().c_str());
    }
    for (int i = 0; i < message.content_descriptors_size(); i++) {
        VPLXmlWriter_InsertSimpleElement(writer, "content_descriptors", message.content_descriptors(i).c_str());
    }
}

