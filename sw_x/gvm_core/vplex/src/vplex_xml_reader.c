//
//  Copyright (C) 2010, BroadOn Communications Corp.
//
//  These coded instructions, statements, and computer programs contain
//  unpublished  proprietary information of BroadOn Communications Corp.,
//  and are protected by Federal copyright law. They may not be disclosed
//  to third parties or copied or duplicated in any form, in whole or in
//  part, without the prior written consent of BroadOn Communications Corp.
//

#include "vplex_xml_reader.h"
#include "vplex_private.h"

#define DEBUG_XML_READER  0

#define MAX_UTF8_CHAR_SIZE   4
#define MAX_NCR_UTF8_VALUE   0x10FFFF

#define MAX_NCR_DIGITS_HEX   6
#define MAX_NCR_CHARS_HEX    (3 + MAX_NCR_DIGITS_HEX + 1)
#define MIN_NCR_CHARS_HEX    (3 + 1 + 1)

#define MAX_NCR_DIGITS_DEC   7
#define MAX_NCR_CHARS_DEC    (2 + MAX_NCR_DIGITS_HEX + 1)
#define MIN_NCR_CHARS_DEC    (2 + 1 + 1)

#define UTF8_2BYTE_RANGE_MIN  0x80
#define UTF8_3BYTE_RANGE_MIN  0x800
#define UTF8_4BYTE_RANGE_MIN  0x10000
#define UTF8_4BYTE_RANGE_MAX  0x10FFFF

#define LONGEST_ESCAPE_SEQ  8

// XML parser state machine states
typedef enum {
    // Cursor is not inside a tag.  marker is after end of last tag.
    RS_DATA,
    // Cursor is immediately after the start of a tag ('<').
    RS_START_TAG,
    // The cursor is in an opening tag name.  marker is at the start of the
    // name.
    RS_OPEN_TAG_NAME,
    // The cursor is in a closing tag name.  marker is at the start of the
    // name.
    RS_CLOSE_TAG_NAME,
    // The cursor is inside a tag but not inside a name or attribute.  marker
    // is at the start of the tag name.
    RS_TAG_WHITESPACE,
    // The cursor is in an attribute name.  marker is at the start of the
    // tag name.  attr_name[numAttr] is at the start of the name.
    RS_ATTR_NAME,
    // The cursor is immediately after the '=' for an attribute.  marker is at
    // the start of the tag name.
    RS_ATTR_VAL,
    // The cursor is inside a single quoted attribute value.  marker is at the
    // start of the tag name.  attr_value[numAttr] is immediately after the
    // single quote.
    RS_ATTR_VAL_SQUOT,
    // The cursor is inside a double quoted attribute value.  marker is at the
    // start of the tag name.  attr_value[numAttr] is immediately after the
    // double quote.
    RS_ATTR_VAL_DBLQT,
    // The cursor is immediately after a '/' and a '>' is expected.
    RS_END_TAG
} ReaderState;

/// Converts one NCR (numeric character reference) to the equivalent UTF-8 byte sequence.
/// @param in A null-terminated c-string containing the NCR value to convert.
///     Does not include the "&#" or ";" (for example, "x963F").
/// @param utf8Char_out The Unicode character, encoded in UTF-8.
/// @param utf8CharSize_in_out In: max number of bytes to write to \a utf8Char_out.
///     Out: Actual number of bytes written to \a utf8Char_out (not counting null-terminator).
static int ncrToUtf8(const char* in, char* utf8Char_out, size_t* utf8CharSize_in_out);

void VPLXmlReader_Init(_VPLXmlReader* reader,
        void* buffer,
        size_t bufferLen,
        VPLXmlReader_TagOpenCallback openCallback,
        VPLXmlReader_TagCloseCallback closeCallback,
        VPLXmlReader_DataCallback dataCallback,
        void* param)
{
    reader->openCallback = openCallback;
    reader->closeCallback = closeCallback;
    reader->dataCallback = dataCallback;
    reader->readParam = param;
    reader->buffer = buffer;
    reader->bufferSize = bufferLen;
    reader->pos = 0;
    reader->escapedPos = 0;
    reader->numAttr = 0;
    reader->state = RS_DATA;
    reader->inEscape = 0;
    reader->escStart = 0;
    if (bufferLen < 16) {
        VPL_LIB_LOG_WARN(VPL_SG_HTTP, "Dangerously small buffer ("FMTuSizeT")", bufferLen);
    }
}

#define NUM_ESC 5
static const char *ESC_SEQUENCES[NUM_ESC] = {"quot", "apos", "lt", "gt", "amp"};
static const char ESC_CHARS[NUM_ESC] = {'\"', '\'', '<', '>', '&'};

static void VPLXmlReader_Parse(char* const cbuf, size_t end, _VPLXmlReader* reader)
{
    for (; reader->escapedPos < end;
            reader->pos++, reader->escapedPos++) {

        // Process one byte of the input.
        char cur = cbuf[reader->escapedPos];

        // The number of unescaped bytes that we produce during this iteration of the loop.
        size_t numUnescapedBytesWritten;

        // Tracks if the current unescaped byte(s) had been escaped in the input.
        // We use this to avoid treating the result of an escape sequence as markup.
        VPL_BOOL wasEscaped = VPL_FALSE;

        size_t j;

        // Write the raw byte as the next byte in the unescaped string.
        // If it is actually part of an escape sequence, we will overwrite it with the correct
        // unescaped character when we encounter the ';' that ends the escape sequence.
        cbuf[reader->pos] = cur;

#if DEBUG_XML_READER
        VPL_LIB_LOG_INFO(VPL_SG_HTTP, "before read: pos="FMTu_size_t" inEscape=%d cur=%c", reader->pos, reader->inEscape, cur);
#endif

        if (reader->inEscape) {
            if (cur == ';') {
                size_t escEnd = reader->pos;
                // This makes it so that &cbuf[reader->escStart] points to a
                // null-terminated string containing just the escape sequence (without the '&' or ';').
                cbuf[escEnd] = '\0';
                // Move the "to-write" pointer back to the beginning of the escape sequence.
                // We will overwrite the escape sequence with the original byte(s) now.
                reader->pos = reader->escStart;
                if (cbuf[reader->escStart] == '#') {
                    size_t len = escEnd - reader->pos;
#if DEBUG_XML_READER
                        VPL_LIB_LOG_INFO(VPL_SG_HTTP,
                                        "Converting NCR \"%s\"", &cbuf[reader->escStart+1]);
#endif
                    int temp_rv = ncrToUtf8(&cbuf[reader->escStart+1], &cbuf[reader->pos], &len);
                    if (temp_rv == 0) {
                        //assert(len >= 1);
                        numUnescapedBytesWritten = len;
#if DEBUG_XML_READER
                        VPL_LIB_LOG_INFO(VPL_SG_HTTP,
                                        "UTF-8 escape sequence: %.*s; length=%d",
                                        len, &cbuf[reader->pos], len);
#endif
                        // If more than 1 byte was written, reader->pos will be incremented in
                        // the "Process the unescaped byte(s)" loop below.
                    } else {
                        VPL_LIB_LOG_ERR(VPL_SG_HTTP,
                                "Bad NCR escape sequence: &%s; (error %d)",
                                &cbuf[reader->escStart], temp_rv);
                        cbuf[reader->pos] = '?';
                        numUnescapedBytesWritten = 1;
                    }
                } else {
                    int i;
                    for (i = 0; i < NUM_ESC; i++) {
                        if (strcmp(&cbuf[reader->escStart], ESC_SEQUENCES[i]) == 0) {
                            cbuf[reader->pos] = ESC_CHARS[i];
                            break;
                        }
                    }
                    if (i == NUM_ESC) {
                        VPL_LIB_LOG_ERR(VPL_SG_HTTP,
                                "Unrecognized XML escape sequence: &%s;",
                                &cbuf[reader->escStart]);
                        cbuf[reader->pos] = '?';
                    }
                    numUnescapedBytesWritten = 1;
                }
                reader->inEscape = VPL_FALSE;
                wasEscaped = VPL_TRUE;
            } else {
                // The current byte is within an escape sequence.
                // We won't process it until we see a ';'.
                numUnescapedBytesWritten = 0;
            }
        } else if (cur == '&') {
            reader->inEscape = VPL_TRUE;
            reader->escStart = reader->pos;
            reader->pos--;
            numUnescapedBytesWritten = 0;
        } else {
            // Not part of an escape sequence.
            numUnescapedBytesWritten = 1;
        }

#if DEBUG_XML_READER
        VPL_LIB_LOG_INFO(VPL_SG_HTTP, "pos="FMTu_size_t" inEscape=%d numUnescapedBytesWritten="FMTu_size_t, reader->pos, reader->inEscape, numUnescapedBytesWritten);
        {
            int tempSize = reader->pos + 1 + numUnescapedBytesWritten;
            int i;
            char* tempDebug = malloc(tempSize);
            memcpy(tempDebug, cbuf, tempSize);
            for (i = 0; i < tempSize; i++) {
                if (tempDebug[i] == '\0') {
                    tempDebug[i] = '|';
                }
            }
            VPL_LIB_LOG_INFO(VPL_SG_HTTP, "before processing: "FMTu_size_t" byte(s): \"%.*s\"", tempSize, tempSize, tempDebug);
            free(tempDebug);
        }
#endif

        // Process the unescaped byte(s).
        for (j = 0; j < numUnescapedBytesWritten; j++) {
            if (j > 0) {
                reader->pos++;
            }
#if DEBUG_XML_READER
            VPL_LIB_LOG_INFO(VPL_SG_HTTP, "Processing byte "FMT0x8" pos="FMTu_size_t, (u8)cbuf[reader->pos], reader->pos);
#endif
            switch (reader->state) {
            case RS_DATA:
                if ((!wasEscaped) && (cur == '<')) {
                    cbuf[reader->pos] = '\0';
                    reader->dataCallback(cbuf, reader->readParam);
                    reader->state = RS_START_TAG;
                    reader->pos = -1;
                    reader->numAttr = 0;
                    reader->attr_name[0] = NULL;
                    reader->attr_value[0] = NULL;
                } else if ((reader->pos + 7 + LONGEST_ESCAPE_SEQ) >= reader->bufferSize) {
#if DEBUG_XML_READER
                    VPL_LIB_LOG_INFO(VPL_SG_HTTP, "Ready to flush data (pos="FMTu_size_t")", reader->pos);
#endif
                    // Split long data, but try to avoid splitting in the middle of a UTF8 character.
                    // Pre-RFC 3629, a UTF-8 character could be at most 6 bytes long.
                    // We also need to save the last byte to stick on the null-terminator.
                    VPL_BOOL processNow;
                    if ((cbuf[reader->pos] & 0x80) == 0) {
                        // ASCII character
                        processNow = VPL_TRUE;
                    } else if ((cbuf[reader->pos] & 0xC0) == 0xC0) {
                        // First byte of a multibyte UTF8 character.
                        processNow = VPL_TRUE;
                    } else {
                        // Continuation byte.
                        if ((reader->pos + 2 + LONGEST_ESCAPE_SEQ) >= reader->bufferSize) {
                            // The buffer is nearly full but we're not at the end of a character.
                            // Call the callback now so that we can proceed.
                            // This should only happen if the reader's buffer is ridiculously small
                            // (<16 bytes) or we're reading invalid UTF8.
                            VPL_LIB_LOG_WARN(VPL_SG_HTTP, "Needed to flush data in the middle of a UTF8 character; input may not be valid UTF8");
                            processNow = VPL_TRUE;
                        } else {
                            processNow = VPL_FALSE;
                        }
                    }
                    if (processNow) {
                        // Temporarily replace the current byte with null-terminator.
                        char temp = cbuf[reader->pos];
                        cbuf[reader->pos] = '\0';
                        // Call the callback with the null-terminated string.
                        reader->dataCallback(cbuf, reader->readParam);
                        // Restore the current byte.
                        cbuf[reader->pos] = temp;
                        // Move any unprocessed bytes to the beginning of the buffer.
                        // Need to use memmove (not memcpy) because the regions can overlap.
                        memmove(cbuf, &cbuf[reader->pos], numUnescapedBytesWritten - j);
                        reader->pos = 0;
                    }
                }
                break;
            case RS_START_TAG:
                if ((!wasEscaped) && (cur == '/')) {
                    reader->state = RS_CLOSE_TAG_NAME;
                    reader->pos = -1;
                } else if ((!wasEscaped) && (cur == '>')) {
                    VPL_LIB_LOG_ERR(VPL_SG_HTTP, "Error: tag with no name (<>)");
                    reader->state = RS_DATA;
                    reader->pos = -1;
                } else {
                    reader->state = RS_OPEN_TAG_NAME;
                }
                break;
            case RS_OPEN_TAG_NAME:
                if ((!wasEscaped) && (cur == ' ' || cur == '\t' || cur == '\n'
                        || cur == '\r' || cur == '/' || cur == '>')) {
                    cbuf[reader->pos] = '\0';
                    if (cur == '/') {
                        reader->openCallback(cbuf, (const char**)reader->attr_name,
                                (const char**)reader->attr_value, reader->readParam);
                        reader->closeCallback(cbuf, reader->readParam);
                        reader->state = RS_END_TAG;
                        reader->pos = -1;
                    } else if (cur == '>') {
                        reader->openCallback(cbuf, (const char**)reader->attr_name,
                                (const char**)reader->attr_value, reader->readParam);
                        reader->state = RS_DATA;
                        reader->pos = -1;
                    } else {
                        reader->state = RS_TAG_WHITESPACE;
                    }
                }
                break;
            case RS_CLOSE_TAG_NAME:
                if ((!wasEscaped) && (cur == '>' || cur == ' ' || cur == '\t'
                        || cur == '\n' || cur == '\r')) {
                    cbuf[reader->pos] = '\0';
                    reader->closeCallback(cbuf, reader->readParam);
                    if (cur == '>') {
                        reader->state = RS_DATA;
                        reader->pos = -1;
                    } else {
                        reader->state = RS_END_TAG;
                    }
                }
                break;
            case RS_TAG_WHITESPACE:
                if ((!wasEscaped) && (cur == ' ' || cur == '\t' || cur == '\n'
                        || cur == '\r')) {
                    // Eat the redundant whitespace character.
                    reader->pos--;
                } else if ((!wasEscaped) && (cur == '/' || cur == '>')) {
                    reader->attr_name[reader->numAttr] = NULL;
                    reader->attr_value[reader->numAttr] = NULL;
                    reader->openCallback(cbuf, (const char**)reader->attr_name,
                            (const char**)reader->attr_value, reader->readParam);
                    if (cur == '/') {
                        reader->closeCallback(cbuf, reader->readParam);
                        reader->state = RS_END_TAG;
                    } else {
                        reader->state = RS_DATA;
                    }
                    reader->pos = -1;
                } else {
                    reader->attr_name[reader->numAttr] = &cbuf[reader->pos];
                    reader->state = RS_ATTR_NAME;
                }
                break;
            case RS_ATTR_NAME:
                if ((!wasEscaped) && (cur == '=')) {
                    cbuf[reader->pos] = '\0';
                    reader->attr_value[reader->numAttr] = &cbuf[reader->pos+1];
                    reader->state = RS_ATTR_VAL;
                }
                break;
            case RS_ATTR_VAL:
                if ((!wasEscaped) && (cur == '\"')) {
                    // Eat the punctuation.
                    reader->pos--;
                    reader->state = RS_ATTR_VAL_DBLQT;
                } else if ((!wasEscaped) && (cur == '\'')) {
                    // Eat the punctuation.
                    reader->pos--;
                    reader->state = RS_ATTR_VAL_SQUOT;
                } else {
                    VPL_LIB_LOG_ERR(VPL_SG_HTTP,
                            "Error: unquoted attribute value in tag %s",
                            cbuf);
                    reader->attr_name[reader->numAttr] = NULL;
                    reader->attr_value[reader->numAttr] = NULL;
                    reader->state = RS_TAG_WHITESPACE;
                }
                break;
            case RS_ATTR_VAL_SQUOT:
                if ((!wasEscaped) && (cur == '\'')) {
                    cbuf[reader->pos] = '\0';
                    reader->numAttr++;
                    reader->state = RS_TAG_WHITESPACE;
                }
                break;
            case RS_ATTR_VAL_DBLQT:
                if ((!wasEscaped) && (cur == '\"')) {
                    cbuf[reader->pos] = '\0';
                    reader->numAttr++;
                    reader->state = RS_TAG_WHITESPACE;
                }
                break;
            case RS_END_TAG:
                if ((!wasEscaped) && (cur == '>')) {
                    reader->state = RS_DATA;
                    reader->pos = -1;
                }
                break;
            default:
                VPL_LIB_LOG_ERR(VPL_SG_HTTP, "Corrupt reader->state(%d)", reader->state);
                return;
            } // switch (reader->state)

            if (reader->numAttr == VPLEX_XML_MAXATTR) {
                VPL_LIB_LOG_ERR(VPL_SG_HTTP, "Too many attributes (>=%d) on tag %s",
                        VPLEX_XML_MAXATTR, cbuf);
                return;
            }
        } // for (i = 0; i < numUnescapedBytesWrittenThisLoop; i++)
    }
}

size_t VPLXmlReader_CurlWriteCallback(const void* inBuf, size_t size, size_t nmemb, void* xmlReader)
{
    _VPLXmlReader* reader = (_VPLXmlReader*)xmlReader;
    char* const cbuf = (char*)reader->buffer;
    size_t remain = size * nmemb;
    const char* inBufPos = (const char*)inBuf;

    while (remain > 0) {
        size_t avail = reader->bufferSize - reader->pos;
        size_t numCopy = MIN(remain, avail);
        memcpy(cbuf + reader->pos, inBufPos, numCopy);
        inBufPos += numCopy;
        remain -= numCopy;

        reader->escapedPos = reader->pos;
        VPLXmlReader_Parse(cbuf, reader->pos + numCopy, reader);

#if DEBUG_XML_READER
        {
            int i;
            char* tempDebug = malloc(reader->pos);
            memcpy(tempDebug, cbuf, reader->pos);
            for (i = 0; i < reader->pos; i++) {
                if (tempDebug[i] == '\0') {
                    tempDebug[i] = '|';
                }
            }
            VPL_LIB_LOG_INFO(VPL_SG_HTTP, FMTu_size_t" byte(s): \"%.*s\"", reader->pos, reader->pos, tempDebug);
            free(tempDebug);
        }
#endif

        if (reader->pos >= reader->bufferSize) {
            VPL_LIB_LOG_ERR(VPL_SG_HTTP,
                    "Overflow error: it seems a tag or escape sequence was longer "
                    "than reader->bufferSize("FMTu_size_t").", reader->bufferSize);
            return 0;
        }
    }
    return size * nmemb;
}

//---------
// TODO: can refactor into new file
//---------

/// Converts one UTF-32 code point to a multibyte UTF-8 character.
/// Return UTF-8 multibyte char and num bytes in char.
static int utf32CodePointToUtf8(char* utf8Char_out, size_t* utf8CharSize_in_out, unsigned int utf32CodePoint)
{
    int rv = VPL_OK;

    unsigned int cp32 = utf32CodePoint;

    //  1. determine number of bytes needed from range
    //  2. combine upper bits of bytes with appropriate cp bits

    if (cp32 < UTF8_2BYTE_RANGE_MIN) {
        // 1 byte
        if (*utf8CharSize_in_out < 1) {
            rv = VPL_ERR_MAX;
            goto end;
        }
        utf8Char_out[0] = (unsigned char) cp32;
        *utf8CharSize_in_out = 1;
    }
    else if (cp32 < UTF8_3BYTE_RANGE_MIN) {
        // 2 bytes
        if (*utf8CharSize_in_out < 2) {
            rv = VPL_ERR_MAX;
            goto end;
        }
        utf8Char_out[1] = (unsigned char) (0x80 | ( cp32        & 0x3F));  // low  6 bits
        utf8Char_out[0] = (unsigned char) (0xC0 | ((cp32 >>  6) & 0x1F));  // next 5 bits
        *utf8CharSize_in_out = 2;
    }
    else if (cp32 < UTF8_4BYTE_RANGE_MIN) {
        // 3 bytes
        if (*utf8CharSize_in_out < 3) {
            rv = VPL_ERR_MAX;
            goto end;
        }
        utf8Char_out[2] = (unsigned char) (0x80 | ( cp32        & 0x3F));  // low  6 bits
        utf8Char_out[1] = (unsigned char) (0x80 | ((cp32 >>  6) & 0x3F));  // next 6 bits
        utf8Char_out[0] = (unsigned char) (0xE0 | ((cp32 >> 12) & 0x0F));  // next 4 bits
        *utf8CharSize_in_out = 3;
    }
    else if (cp32 <= UTF8_4BYTE_RANGE_MAX) {
        // 4 bytes
        if (*utf8CharSize_in_out < 4) {
            rv = VPL_ERR_MAX;
            goto end;
        }
        utf8Char_out[3] = (unsigned char) (0x80 | ( cp32        & 0x3F));  // low  6 bits
        utf8Char_out[2] = (unsigned char) (0x80 | ((cp32 >>  6) & 0x3F));  // next 6 bits
        utf8Char_out[1] = (unsigned char) (0x80 | ((cp32 >> 12) & 0x3F));  // next 6 bits
        utf8Char_out[0] = (unsigned char) (0xF0 | ((cp32 >> 18) & 0x07));  // next 3 bits
        *utf8CharSize_in_out = 4;
    }
    else {
        rv = VPL_ERR_INVALID;
    }

end:
    if (rv != VPL_OK) {
        *utf8CharSize_in_out = 0;
    }

    return rv;
}

/// Converts the NCR to a Unicode code point.
/// On any error returns VPL_ERR_INVALID.
static int utf32CodePointFromNCR(const char* in, unsigned int* utf32CodePoint)
{
    int rv = VPL_ERR_INVALID;

    unsigned int inSize = (unsigned int)(strlen(in) + 1); // include \0 because it is used for termination
    unsigned int k = 0;
    unsigned int ncrChars = 2;
    unsigned int result = 0;
    int nibble;
    char c;

    if (in[k] == 'x') {
        ++ncrChars;
        if (inSize <= ++k) { 
            rv = VPL_ERR_MAX;
            goto end;
        }
        c = in[k];
        // c should be 1st hex digit of NCR
        // ncrChars should be 3
        while (c != '\0' &&
                ++k < inSize &&
                ++ncrChars < MAX_NCR_CHARS_HEX) {

            if (c >= '0' && c <= '9')
                nibble = c - '0';
            else if (c >= 'a' && c <= 'f')
                nibble = c - 'a' + 10;
            else if (c >= 'A' && c <= 'F')
                nibble = c - 'A' + 10;
            else {
                break;
            }
            result = (result << 4) + nibble;
            c = in[k];
        }

        if (c == '\0'
               && ++ncrChars >= MIN_NCR_CHARS_HEX
               && result <= MAX_NCR_UTF8_VALUE) {
            rv = VPL_OK;
        } else {
            result = 0;
        }
    }
    else {
        // c should be 1st decimal digit of NCR
        // k is index of c in input string
        // ncrChars should be 2
        c = in[k];
        while (c != '\0' &&
                (c >= '0' || c <= '9') &&
                ++k < inSize &&
                ++ncrChars < MAX_NCR_CHARS_DEC) {
            result = result*10 + (c - '0');
            c = in[k];
        }

        if (c == '\0'
               && ++ncrChars >= MIN_NCR_CHARS_DEC
               && result <= MAX_NCR_UTF8_VALUE) {
            rv = VPL_OK;
        } else {
            result = 0;
        }
    }

end:
    *utf32CodePoint = result;

    if (rv != VPL_OK) {
        VPL_LIB_LOG_ERR(VPL_SG_HTTP,
                        "utf32CodePointFromNCR returning VPL_ERR_INVALID;");
    }

    return rv;
}

int ncrToUtf8(const char* in, char* utf8Char_out, size_t* utf8CharSize_in_out)
{
    int rv = VPL_OK;
    unsigned int utf32CodePoint;

    rv = utf32CodePointFromNCR(in, &utf32CodePoint);
    if (rv != VPL_OK) {
        VPL_LIB_LOG_ERR(VPL_SG_HTTP,
                        "utf32CodePointFromNCR error: %d",
                        rv);
        goto end;
    }
    
    rv = utf32CodePointToUtf8(utf8Char_out, utf8CharSize_in_out, utf32CodePoint);
    if (rv != VPL_OK) {
        VPL_LIB_LOG_ERR(VPL_SG_HTTP,
                        "utf32CodePointToUtf8 error: %d",
                        rv);
        goto end;
    }
    
end:
    return rv;
}

