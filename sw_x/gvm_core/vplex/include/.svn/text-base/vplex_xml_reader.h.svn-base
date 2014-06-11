//
//  Copyright 2010 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#ifndef __VPL_XML_READER_H__
#define __VPL_XML_READER_H__

#include "vplex_plat.h"

#ifdef __cplusplus
extern "C" {
#endif

/// Callback for when an opening tag is processed.
typedef void (*VPLXmlReader_TagOpenCallback)(const char* tag,
        const char* attr_name[], const char* attr_value[], void* param);

/// Callback for when a closing tag is processed.  This is also called after
/// a self-closing tag.
typedef void (*VPLXmlReader_TagCloseCallback)(const char* tag, void* param);

/// Callback for data sections.  This is called for every nonempty sequence
/// between tags.  Multiple calls will be made for data segments that are longer
/// than the #_VPLXmlReader's buffer length.
typedef void (*VPLXmlReader_DataCallback)(const char* data, void* param);

/// Maximum attributes per tag handled by the parser
#define VPLEX_XML_MAXATTR 10

/// XML reader.  The reader consists of multiple callbacks which are called
/// as the XML structure is parsed, along with a user-specified parameter to
/// also pass to the callbacks and the parser internal state and buffers.
//% TODO: remove underscore
typedef struct _VPLXmlReader {
    VPLXmlReader_TagOpenCallback openCallback;
    VPLXmlReader_TagCloseCallback closeCallback;
    VPLXmlReader_DataCallback dataCallback;
    void* readParam;
    void* buffer;
    size_t bufferSize;
  //% Internal parser state:
    //% Position of next unescaped character.  As the buffer is processed,
    //% an escaped and an unescaped cursor advance and the buffer is
    //% overwritten with unescaped content.
    size_t pos;
    //% Position of next escaped character.
    size_t escapedPos;
    //% Attribute names and values of the tag being parsed.
    int numAttr;
    char *attr_name[VPLEX_XML_MAXATTR+1];
    char *attr_value[VPLEX_XML_MAXATTR+1];
    //% Parser state-machine state
    int state;
    //% Are we in an escape sequence?
    VPL_BOOL inEscape;
    //% The start of the escape sequence we are in.
    size_t escStart;
} _VPLXmlReader;

/// Initialize a VPL XML reader to be used with #VPLXmlReader_CurlWriteCallback.
/// @param reader The reader to initialize.
/// @param buffer Buffer space for the write callback to copy data for the
///     XML parser.
///     To successfully parse an XML stream, this buffer must be large enough
///     to hold the largest opening tag.  This includes just the element name string,
///     any attribute name strings, and any attribute value strings (plus a null-terminator
///     for each string).  The data between the open tag and close tag is handled separately.
///     Data segments longer than the buffer length will be passed to \a dataCallback in chunks.
/// @param bufferLen Allocated size of \a buffer.
/// @param openCallback Callback to be called when an opening tag, or
///     self-closing tag, is processed.
/// @param closeCallback Callback to be called when a closing tag, or
///     self-closing tag after the open callback is called, is processed.
/// @param dataCallback Callback for strings of characters between tags.
/// @param param Parameter to be supplied to callbacks.
void VPLXmlReader_Init(_VPLXmlReader* reader,
        void* buffer,
        size_t bufferLen,
        VPLXmlReader_TagOpenCallback openCallback,
        VPLXmlReader_TagCloseCallback closeCallback,
        VPLXmlReader_DataCallback dataCallback,
        void* param);

/// Use this as a libcurl (or other HTTP layer) write callback.
/// \a reader should point to the #_VPLXmlReader to use when parsing the XML.
/// The reader can handle tags with up to #VPLEX_XML_MAXATTR attributes and
/// tag or data lengths up to the reader buffer size.  If a tag is longer than
/// this, it will cause an error.  If data is longer than this, it will be
/// passed to the data callback in segments.
size_t VPLXmlReader_CurlWriteCallback(const void* buf, size_t size, size_t nmemb, void* reader);

#if 0
#define VPL_INVALID_CHAR  '?'

static void VPLXmlReader_priv_correctUtf8Str(
        VPLXmlReader* reader,
        const utf8* str)
{
    int currStrPos = 0;
    for (;;) {
        utf8 c = str[currStrPos];
        VPLUtf8ByteType type = VPLString_ClassifyUtf8Byte(c);
        switch(type) {
            case VPLUtf8_NULL_TERM:
                return;
            case VPLUtf8_ASCII:
                VPLXmlReader_priv_insertChar(reader, c);
                break;
            case VPLUtf8_START_OF_2_BYTE_CHAR:
            case VPLUtf8_START_OF_3_BYTE_CHAR:
            case VPLUtf8_START_OF_4_BYTE_CHAR: 
            {
                utf8 c2 = str[currStrPos+1];
                VPLUtf8ByteType type2 = VPLString_ClassifyUtf8Byte(c2);
                if (type2 == VPLUtf8_NULL_TERM) {
                    // Invalid!  TODO: log?
                    VPLXmlReader_priv_insertEscapedChar(reader, VPL_INVALID_CHAR);
                    return;
                }
                else if (type2 == VPLUtf8_BYTE_2_3_OR_4) {
                    if ((type == VPLUtf8_START_OF_3_BYTE_CHAR) || (type == VPLUtf8_START_OF_4_BYTE_CHAR)) {
                        utf8 c3 = str[currStrPos+2];
                        VPLUtf8ByteType type3 = VPLString_ClassifyUtf8Byte(c3);
                        if (type3 == VPLUtf8_NULL_TERM) {
                            // Invalid!  TODO: log?
                            VPLXmlReader_priv_insertEscapedChar(reader, VPL_INVALID_CHAR);
                            return;
                        }
                        else if (type3 == VPLUtf8_BYTE_2_3_OR_4) {
                            if (type == VPLUtf8_START_OF_4_BYTE_CHAR) {
                                utf8 c4 = str[currStrPos+3];
                                VPLUtf8ByteType type4 = VPLString_ClassifyUtf8Byte(c4);
                                if (type4 == VPLUtf8_NULL_TERM) {
                                    // Invalid!  TODO: log?
                                    VPLXmlReader_priv_insertEscapedChar(reader, VPL_INVALID_CHAR);
                                    return;
                                }
                                else if (type4 == VPLUtf8_BYTE_2_3_OR_4) {
                                    VPLXmlReader_priv_insertEscapedChar(reader, c);
                                    VPLXmlReader_priv_insertEscapedChar(reader, c2);
                                    VPLXmlReader_priv_insertEscapedChar(reader, c3);
                                    VPLXmlReader_priv_insertEscapedChar(reader, c4);
                                    currStrPos += 3;
                                }
                                else {
                                    // Invalid!  TODO: log?
                                    VPLXmlReader_priv_insertEscapedChar(reader, VPL_INVALID_CHAR);
                                    currStrPos += 3;
                                }
                            }
                            else {
                                VPLXmlReader_priv_insertEscapedChar(reader, c);
                                VPLXmlReader_priv_insertEscapedChar(reader, c2);
                                VPLXmlReader_priv_insertEscapedChar(reader, c3);
                                currStrPos += 2;
                            }
                        }
                        else {
                            // Invalid!  TODO: log?
                            VPLXmlReader_priv_insertEscapedChar(reader, VPL_INVALID_CHAR);
                            currStrPos += 2;
                        }
                    }
                    else {
                        VPLXmlReader_priv_insertEscapedChar(reader, c);
                        VPLXmlReader_priv_insertEscapedChar(reader, c2);
                        currStrPos += 1;
                    }
                }
                else {
                    // Invalid!  TODO: log?
                    VPLXmlReader_priv_insertEscapedChar(reader, VPL_INVALID_CHAR);
                    currStrPos += 1;
                    break;
                }
                break;
            }
            default:
                // Invalid!  TODO: log?
                VPLXmlReader_priv_insertEscapedChar(reader, VPL_INVALID_CHAR);
                break;
        }
        currStrPos++;
    }
}
#endif

#ifdef __cplusplus
}
#endif

#endif // include guard
