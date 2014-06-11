//
//  Copyright (C) 2005-2010, BroadOn Communications Corp.
//
//  These coded instructions, statements, and computer programs contain
//  unpublished  proprietary information of BroadOn Communications Corp.,
//  and are protected by Federal copyright law. They may not be disclosed
//  to third parties or copied or duplicated in any form, in whole or in
//  part, without the prior written consent of BroadOn Communications Corp.
//

#ifndef __VPL_XML_WRITER_H__
#define __VPL_XML_WRITER_H__

#include "vplex_plat.h"

#ifdef __cplusplus
extern "C" {
#endif

///
/// Structure for tracking the construction of an XML string.
/// It is safe to use UTF-8 encoded strings with these functions; they only care
/// about the number of bytes and not the number of characters.
/// Fields should be treated as opaque.
///
typedef struct VPLXmlWriter {
    
    /// currPos is the next position we would write if the buffer were infinite length.
    size_t currPos;
    
    /// Closing tags grow from the end of the buffer toward the front.
    size_t closingTagLen;
    
    /// The length of scratchBuf.
    u16 scratchBufLen;
    
    /// Variable length buffer.
#ifndef _MSC_VER
    char scratchBuf[0];
#else
    // Wasted byte, but it gives C4200 otherwise, even if I ask it to suppress that warning or provide my own copy-ctor and copy-assignment op.
    char scratchBuf[1];
#endif
} VPLXmlWriter;

///
/// Initializes a #VPLXmlWriter within the provided buffer.
/// \a bufLen needs to be at least sizeof(VPLXmlWriter) + 1, for '\\0'.
///
void VPLXmlWriter_Init(VPLXmlWriter* writer, u16 bufLen);

///
/// Open a new tag with the specified attributes.
///
void VPLXmlWriter_OpenTag(
                        VPLXmlWriter* writer,
                        const char* tag_name,
                        u16 num_attr,
                        const char* attr_name[],
                        const char* attr_value[]);

///
/// Open a new tag with the specified attributes.
/// @param writer
/// @param tag_name
/// @param num_attr The number of name-value pairs to follow.  Each attribute
///     consists of two (const char*) parameters and are expected in this order:
///     "name1", "value1", "name2", "value2", etc.  The number of strings must
///     therefore be (num_attr * 2).
///
void VPLXmlWriter_OpenTagV(
                        VPLXmlWriter* writer,
                        const char* tag_name,
                        u16 num_attr,
                        ...);

///
/// Inserts bytes of content and escapes any control characters.
///
void VPLXmlWriter_AddData(
                        VPLXmlWriter* writer,
                        const char* data);

///
/// Utility function for inserting bytes of content as base64.
///
void VPLXmlWriter_AddDataAsBase64(
                        VPLXmlWriter* writer,
                        const void* blob,
                        u32 blobLen);

///
/// Close the current tag.
///
void VPLXmlWriter_CloseTag(
                        VPLXmlWriter* writer);

///
/// Returns whether or not the #VPLXmlWriter ran out of space, and optionally,
/// the actual number of bytes that the constructed XML string would contain if
/// the #VPLXmlWriter buffer were infinite length.
/// @param writer The writer to operate on.
/// @param bufLen_out The number of bytes that the resulting string should
///     contain (includes the null-terminator).
///     Don't forget to add \code sizeof(#VPLXmlWriter) \endcode to
///     this value if allocating a new #VPLXmlWriter).
///
VPL_BOOL VPLXmlWriter_HasOverflowed(
                        VPLXmlWriter* writer,
                        size_t* bufLen_out);

///
/// Close all open tags and return the resulting XML string if the construction
/// was successful.  If the buffer was too small (#VPLXmlWriter_HasOverflowed
/// returns true), this function will return NULL instead.
///
const char* VPLXmlWriter_GetString(
                        VPLXmlWriter* writer,
                        size_t* bufLen_out);

///
/// Inserts a block of XML without any escaping; please use caution when calling this.
///
void VPLXmlWriter_InsertXml(
                        VPLXmlWriter* writer,
                        const char* xml);

///
/// Utility function for the common case of adding "<tagName>data</tagName>".
///
static inline
void VPLXmlWriter_InsertSimpleElement(
                        VPLXmlWriter* writer,
                        const char* tagName,
                        const char* data)
{
    VPLXmlWriter_OpenTagV(writer, tagName, 0);
    VPLXmlWriter_AddData(writer, data);
    VPLXmlWriter_CloseTag(writer);
}

///
/// Utility function for the common case of adding "<tagName>value</tagName>".
///
static inline
void VPLXmlWriter_InsertSimpleS32(VPLXmlWriter* writer, const char* tagName, s32 value)
{
    char buf[16]; // should only need 11 at most
    snprintf(buf, sizeof(buf), FMTs32, value);
    VPLXmlWriter_InsertSimpleElement(writer, tagName, buf);
}

///
/// Utility function for the common case of adding "<tagName>value</tagName>".
///
static inline
void VPLXmlWriter_InsertSimpleS64(VPLXmlWriter* writer, const char* tagName, s64 value)
{
    char buf[32]; // should only need 21 at most
    snprintf(buf, sizeof(buf), FMTs64, value);
    VPLXmlWriter_InsertSimpleElement(writer, tagName, buf);
}

///
/// Utility function for the common case of adding "<tagName>base64(blob)</tagName>".
///
static inline
void VPLXmlWriter_InsertSimpleBlob(
                        VPLXmlWriter* writer,
                        const char* tagName,
                        const void* blob,
                        u32 blobLen)
{
    VPLXmlWriter_OpenTagV(writer, tagName, 0);
    VPLXmlWriter_AddDataAsBase64(writer, blob, blobLen);
    VPLXmlWriter_CloseTag(writer);
}

#ifdef __cplusplus
}
#endif

#endif // include guard
