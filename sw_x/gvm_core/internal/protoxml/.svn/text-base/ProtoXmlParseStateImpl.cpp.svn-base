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

#include "ProtoXmlParseStateImpl.h"

#include <vpl_conv.h>
#include <vplex_serialization.h>

/**
 * VPL XML reader open callback which will dispatch to generated methods.
 */
static void
baseOpenCallback(const char* tag, const char* attr_name[],
        const char* attr_value[], void* param)
{
    ProtoXmlParseStateImpl* state = (ProtoXmlParseStateImpl*)param;
    if (state->hasError() || state->protoStack.empty()) {
        goto out;
    }
    state->tagDepth++;
    state->lastData.clear();
    if (state->protoStartDepth.empty()) {
        // We have not yet encountered the outer tag.
        if (state->outerTag.compare(tag) == 0) {
            state->protoStartDepth.push(state->tagDepth);
        }
    } else if (state->protoStartDepth.top() == state->tagDepth - 1) {
        // We are inside a message object and not inside any unknown tag.
        std::string topType = state->protoStack.top()->GetTypeName();
        if (state->openHandlers.find(topType) != state->openHandlers.end()) {
            state->openHandlers[topType](tag, state);
        } else {
            state->error = "No open handler registered for type ";
            state->error.append(topType);
            state->error.append(
                    "; did you use the correct ProtoXmlParseState subclass?");
        }
    }
out:
    (void)0;
}

/**
 * VPL XML reader close callback which will dispatch to generated methods.
 */
static void
baseCloseCallback(const char* tag, void* param)
{
    ProtoXmlParseStateImpl* state = (ProtoXmlParseStateImpl*)param;
    if (state->hasError() || state->protoStack.empty()) {
        goto out;
    }
    state->tagDepth--;
    if (state->protoStartDepth.empty()) {
        goto out;
    }
    if (state->tagDepth < state->protoStartDepth.top()) {
        // We have finished parsing the top message on the stack.
        state->protoStack.pop();
        state->protoStartDepth.pop();
    } else if (state->tagDepth == state->protoStartDepth.top()) {
        // We are inside a message object, and not inside an unknown tag.
        // We have finished parsing a primitive or enum field.  Dispatch.
        std::string topType = state->protoStack.top()->GetTypeName();
        if (state->closeHandlers.find(topType) != state->closeHandlers.end()) {
            state->closeHandlers[topType](tag, state);
        } else {
            state->error = "No close handler registered for type ";
            state->error.append(topType);
            state->error.append("; this should not happen!");
        }
    }
out:
    state->lastData.clear();
}

/**
 * VPL XML reader data callback.  Simply stores the data in the parse state
 * for use by generated close methods.
 */
static void
baseDataCallback(const char* data, void* param)
{
    ProtoXmlParseStateImpl* state = (ProtoXmlParseStateImpl*)param;
    state->lastData.append(data);
}

ProtoXmlParseStateImpl::ProtoXmlParseStateImpl(const char* _outerTag,
        ProtoMessage* _message)
    : outerTag(_outerTag), tagDepth(0), lastData(), error()
{
    // Null-terminate the parse buffer so if a data segment overruns the buffer
    // the append in baseDataCallback will work as intended to recover the full
    // data.
    // TODO: XML reader data callback could take a length parameter rather than
    // rely on null-terminated input.
    parseBuffer[PARSE_BUFFER_SIZE] = '\0';
    protoStack.push(_message);
    VPLXmlReader_Init(&xmlReader, parseBuffer, PARSE_BUFFER_SIZE,
            baseOpenCallback, baseCloseCallback, baseDataCallback, this);
}

_VPLXmlReader&
ProtoXmlParseStateImpl::reader(void)
{
    return xmlReader;
}

bool
ProtoXmlParseStateImpl::hasError(void)
{
    return !error.empty();
}

const std::string&
ProtoXmlParseStateImpl::errorDetail(void)
{
    return error;
}

bool
parseBool(std::string& data, ProtoXmlParseStateImpl* state)
{
    static std::string _false("false");
    static std::string _true("true");
    bool out = false;
    if (_false.compare(data) == 0) {
        out = false;
    } else if (_true.compare(data) == 0) {
        out = true;
    } else {
        state->error = "Invalid boolean value: ";
        state->error.append(data);
    }
    return out;
}

double
parseDouble(std::string& _data, ProtoXmlParseStateImpl* state)
{
    const char* data = _data.c_str();
    char* end = NULL;
    double out = strtod(data, &end);
    if (data == end) {
        state->error = "Invalid double value: ";
        state->error.append(data);
    }
    return out;
}

float
parseFloat(std::string& _data, ProtoXmlParseStateImpl* state)
{
    const char* data = _data.c_str();
    char* end = NULL;
    float out = (float)strtod(data, &end);
    if (data == end) {
        state->error = "Invalid float value: ";
        state->error.append(data);
    }
    return out;
}

int32_t
parseInt32(std::string& _data, ProtoXmlParseStateImpl* state)
{
    const char* data = _data.c_str();
    char* end = NULL;
    int32_t out = (int32_t)strtol(data, &end, 10);
    if (data == end) {
        state->error = "Invalid int32 value: ";
        state->error.append(data);
    }
    return out;
}

int64_t
parseInt64(std::string& _data, ProtoXmlParseStateImpl* state)
{
    const char* data = _data.c_str();
    char* end = NULL;
    int64_t out = VPLConv_strToS64(data, &end, 10);
    if (data == end) {
        state->error = "Invalid int64 value: ";
        state->error.append(data);
    }
    return out;
}

std::string
parseBytes(const std::string& data, ProtoXmlParseStateImpl* state)
{
    char* out = new char[data.size()];
    size_t outLen = data.size();
    VPL_DecodeBase64(data.c_str(), data.size(), out, &outLen);
    std::string out_s(out, outLen);
    delete[] out;
    return out_s;
}

std::string
writeBytes(const std::string& data)
{
    size_t outLen = VPL_BASE64_ENCODED_BUF_LEN(data.size());
    char* out = new char[outLen];
    VPL_EncodeBase64(data.c_str(), data.size(), out, &outLen, VPL_TRUE, VPL_FALSE);
    std::string out_s(out, outLen);
    delete[] out;
    return out_s;
}
