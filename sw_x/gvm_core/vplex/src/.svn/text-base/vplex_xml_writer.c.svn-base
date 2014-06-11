#include "vplex_xml_writer.h"

#include "vplex_safe_conversion.h"
#include "vplex_serialization.h"
#include "vplex_mem_utils.h"

#include <string.h>
#include <stdarg.h>

void VPLXmlWriter_Init(VPLXmlWriter* writer, u16 bufLen)
{
    if (bufLen < sizeof(VPLXmlWriter)) {
        writer->scratchBufLen = 0;
    }
    else {
        writer->scratchBufLen = bufLen - (sizeof(VPLXmlWriter));
    }
    writer->closingTagLen = 0;
    writer->currPos = 0;
}

VPL_BOOL VPLXmlWriter_HasOverflowed(VPLXmlWriter* writer, size_t* bufLen_out)
{
    size_t required = writer->currPos + writer->closingTagLen + 1;
    if (bufLen_out != NULL) {
        *bufLen_out = required;
    }
    return VPL_AsBOOL(required > writer->scratchBufLen);
}

const char* VPLXmlWriter_GetString(VPLXmlWriter* writer, size_t* bufLen_out)
{
    VPL_BOOL overflowed = VPLXmlWriter_HasOverflowed(writer, bufLen_out);
    if (overflowed) {
        return NULL;
    }
    else {
        u16 startClosingTags = SIZE_T_TO_U16(writer->scratchBufLen - writer->closingTagLen);
        // Move the closing tags to the current position.
        // Must use memmove, since these may be overlapping.
        memmove(&writer->scratchBuf[writer->currPos],
                &writer->scratchBuf[startClosingTags],
                writer->closingTagLen);
        writer->currPos += writer->closingTagLen;
        writer->closingTagLen = 0;
        // Add the null-terminator, but don't increment currPos; this keeps
        // our invariants valid and consequently allows adding additional
        // "root" elements later.
        writer->scratchBuf[writer->currPos] = '\0';
        return writer->scratchBuf;
    }
}

static void VPLXmlWriter_priv_insertEscapedChar(VPLXmlWriter* writer, char escaped_char)
{
    if (writer->currPos < writer->scratchBufLen) {
        writer->scratchBuf[writer->currPos] = escaped_char;
    }
    writer->currPos += 1;
}

static void VPLXmlWriter_priv_backInsertEscapedChar(VPLXmlWriter* writer, char escaped_char)
{
    if (writer->closingTagLen < writer->scratchBufLen) {
        u16 startClosingTags = SIZE_T_TO_U16(writer->scratchBufLen - writer->closingTagLen);
        writer->scratchBuf[startClosingTags-1] = escaped_char;
    }
    writer->closingTagLen += 1;
}

static void VPLXmlWriter_priv_insertEscapedStr(VPLXmlWriter* writer, const char* escaped_str)
{
    int currStrPos = 0;
    char c;
    while ( (c = escaped_str[currStrPos]) != '\0') {
        VPLXmlWriter_priv_insertEscapedChar(writer, c);
        currStrPos++;
    }
}

static void VPLXmlWriter_priv_backInsertEscapedStr(VPLXmlWriter* writer, const char* escaped_str)
{
    // Safe for UTF-8 strings; we only care about number of bytes.
    size_t numBytes = strlen(escaped_str);
    while (numBytes != 0) {
        VPLXmlWriter_priv_backInsertEscapedChar(writer, escaped_str[numBytes-1]);
        numBytes--;
    }
}

static void VPLXmlWriter_priv_commonInsertEscapedStr(
        VPLXmlWriter* writer,
        const char* escaped_str,
        bool addToBack)
{
    if (addToBack) {
        VPLXmlWriter_priv_backInsertEscapedStr(writer, escaped_str);
    }
    else {
        VPLXmlWriter_priv_insertEscapedStr(writer, escaped_str);
    }
}

static void VPLXmlWriter_priv_commonInsertChar(
        VPLXmlWriter* writer,
        char unescaped_char,
        bool addToBack)
{
    switch(unescaped_char) {
        case '<':
            VPLXmlWriter_priv_commonInsertEscapedStr(writer, "&lt;", addToBack);
            break;
        case '&':
            VPLXmlWriter_priv_commonInsertEscapedStr(writer, "&amp;", addToBack);
            break;
        case '>':
            // Not strictly needed unless "]]>" appears inside CDATA section,
            // but recommended for bad parsers.
            VPLXmlWriter_priv_commonInsertEscapedStr(writer, "&gt;", addToBack);
            break;
        case '\'':
            // Only really needed within attribute values.
            VPLXmlWriter_priv_commonInsertEscapedStr(writer, "&apos;", addToBack);
            break;
        case '"':
            // Only really needed within attribute values.
            VPLXmlWriter_priv_commonInsertEscapedStr(writer, "&quot;", addToBack);
            break;
        default:
            if (addToBack) {
                VPLXmlWriter_priv_backInsertEscapedChar(writer, unescaped_char);
            }
            else {
                VPLXmlWriter_priv_insertEscapedChar(writer, unescaped_char);
            }
            break;
    }
}

static void VPLXmlWriter_priv_insertStr(
        VPLXmlWriter* writer,
        const char* escaped_str)
{
    int currStrPos = 0;
    char c;
    while ( (c = escaped_str[currStrPos]) != '\0') {
        VPLXmlWriter_priv_commonInsertChar(writer, c, false);
        currStrPos++;
    }
}

static void VPLXmlWriter_priv_backInsertStr(VPLXmlWriter* writer, const char* escaped_str)
{
    size_t numBytes = strlen(escaped_str);
    while (numBytes != 0) {
        VPLXmlWriter_priv_commonInsertChar(writer, escaped_str[numBytes-1], true);
        numBytes--;
    }
}

void VPLXmlWriter_OpenTag(
        VPLXmlWriter* writer,
        const char* tag_name,
        u16 num_attr,
        const char* attr_name[],
        const char* attr_value[])
{
    int i;
    VPLXmlWriter_priv_insertEscapedChar(writer, '<');
    VPLXmlWriter_priv_insertStr(writer, tag_name);
    for (i = 0; i < num_attr; i++) {
        VPLXmlWriter_priv_insertEscapedChar(writer, ' ');
        VPLXmlWriter_priv_insertStr(writer, attr_name[i]);
        VPLXmlWriter_priv_insertEscapedChar(writer, '=');
        VPLXmlWriter_priv_insertEscapedChar(writer, '"');
        VPLXmlWriter_priv_insertStr(writer, attr_value[i]);
        VPLXmlWriter_priv_insertEscapedChar(writer, '"');
    }
    VPLXmlWriter_priv_insertEscapedChar(writer, '>');
    
    VPLXmlWriter_priv_backInsertEscapedChar(writer, '>');
    VPLXmlWriter_priv_backInsertStr(writer, tag_name);
    VPLXmlWriter_priv_backInsertEscapedChar(writer, '/');
    VPLXmlWriter_priv_backInsertEscapedChar(writer, '<');
}

void VPLXmlWriter_OpenTagV(
        VPLXmlWriter* writer,
        const char* tag_name,
        u16 num_attr,
        ...)
{
    va_list args;
    int i;
    VPLXmlWriter_priv_insertEscapedChar(writer, '<');
    VPLXmlWriter_priv_insertStr(writer, tag_name);
    if (num_attr > 0) {
        va_start(args, num_attr);
        for (i = 0; i < num_attr; i++) {
            const char* name = va_arg(args, const char*);
            const char* value = va_arg(args, const char*);
            VPLXmlWriter_priv_insertEscapedChar(writer, ' ');
            VPLXmlWriter_priv_insertStr(writer, name);
            VPLXmlWriter_priv_insertEscapedChar(writer, '=');
            VPLXmlWriter_priv_insertEscapedChar(writer, '"');
            VPLXmlWriter_priv_insertStr(writer, value);
            VPLXmlWriter_priv_insertEscapedChar(writer, '"');
        }
        va_end(args);
    }
    VPLXmlWriter_priv_insertEscapedChar(writer, '>');
    
    VPLXmlWriter_priv_backInsertEscapedChar(writer, '>');
    VPLXmlWriter_priv_backInsertStr(writer, tag_name);
    VPLXmlWriter_priv_backInsertEscapedChar(writer, '/');
    VPLXmlWriter_priv_backInsertEscapedChar(writer, '<');
}

void VPLXmlWriter_AddData(VPLXmlWriter* writer, const char* data)
{
    VPLXmlWriter_priv_insertStr(writer, data);
}

void VPLXmlWriter_AddDataAsBase64(
        VPLXmlWriter* writer,
        const void* blob,
        u32 blobLen)
{
#define CHUNKS_PER_PASS 16
    u32 currPos = 0;
    while (currPos < blobLen) {
        // Each 3 bytes of input produces 4 characters of output
        char temp[4 * CHUNKS_PER_PASS];
        size_t tempLen = sizeof(temp);
        u32 bytesToRead = MIN((CHUNKS_PER_PASS * 3), (blobLen - currPos));
        VPL_EncodeBase64(
                VPLPtr_AddUnsigned(blob, currPos), bytesToRead,
                temp, &tempLen, VPL_TRUE, VPL_FALSE);
        VPLXmlWriter_AddData(writer, temp);
        currPos += bytesToRead;
    }
}

void VPLXmlWriter_InsertXml(VPLXmlWriter* writer, const char* xml)
{
    VPLXmlWriter_priv_insertEscapedStr(writer, xml);
}

void VPLXmlWriter_CloseTag(VPLXmlWriter* writer)
{
    if ((!VPLXmlWriter_HasOverflowed(writer, NULL)) && (writer->closingTagLen > 0)) {
        u16 startClosingTags = SIZE_T_TO_U16(writer->scratchBufLen - writer->closingTagLen);
        char c;
        do {
            if (startClosingTags >= writer->scratchBufLen) {
                FAILED_ASSERT("Data corruption detected: "FMTuSizeT", "FMTu16,
                        writer->closingTagLen,
                        writer->scratchBufLen);
                return;
            }
            c = writer->scratchBuf[startClosingTags];
            VPLXmlWriter_priv_insertEscapedChar(writer, c);
            startClosingTags++;
            writer->closingTagLen--;
        } while ( c != '>');
    }
}
