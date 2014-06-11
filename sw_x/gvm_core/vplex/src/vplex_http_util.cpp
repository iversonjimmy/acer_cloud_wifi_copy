#include "vplex_http_util.hpp"

#include "scopeguard.hpp"
#include "vpl_conv.h"
#include "vplex_private.h"
#include "vplex_strings.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <functional>
#include <locale>
#include <sstream>

static inline u8 toHex(const u8 &x) {
    return x > 9 ? x + 55: x + 48;
}

std::string VPLHttp_UrlEncoding(const std::string &in, const char *skip_delimiters)
{
    std::string out;

    // Non-reserved: ALPHA / DIGIT / "-" / "." / "_" / "~"
    static const char *nonreserved_delimiters = "-._~";

    for (u16 ix = 0; ix < in.size(); ix++) {
        u8 buf[4];
        memset(buf, 0, 4);
       if ((isalnum((u8)in[ix]) ||
            ((skip_delimiters != NULL) && (strchr(skip_delimiters, in[ix]) != NULL)) ||
            (strchr(nonreserved_delimiters, in[ix]) != NULL))) {
            buf[0] = in[ix];
        } else {
            buf[0] = '%';
            buf[1] = toHex((u8)in[ix] >> 4);
            buf[2] = toHex((u8)in[ix] % 16);
        }
        out += (char *)buf;
    }
    return out;
}

int VPLHttp_DecodeUri(const std::string &encodedUri, std::string &decodedUri)
{
    std::ostringstream oss;

    // 3-state automaton
    enum States {
        ReadAnyChar = 0,    // normal char-reading state
        ReadFirstHex = 1,   // about to read first hex digit after '%'
        ReadSecondHex = 2,  // about to read second hex digit after '%'
        Start = ReadAnyChar,
        Accept = ReadAnyChar,
    };

    States state = Start;
    unsigned char escapedChar = 0;
    for (size_t i = 0; i < encodedUri.size(); i++) {
        const char &c = encodedUri[i];
        switch (state) {
        case ReadAnyChar:
            switch (c) {
            case '+':
                oss << ' ';
                break;
            case '%':
                state = ReadFirstHex;
                break;
            default:
                oss << c;
            }
            break;
        case ReadFirstHex:
        case ReadSecondHex:
            if (!isxdigit(c)) {
                VPL_LIB_LOG_ERR(VPL_SG_HTTP, "Unexpected non-hex digit %c", c);
                return VPL_ERR_INVALID;
            }
            {
                unsigned char val = 0;
                if (isdigit(c))
                    val = c - '0';
                else if (islower(c))
                    val = c - 'a' + 10;
                else // isupper(c)
                    val = c - 'A' + 10;
                if (state == ReadFirstHex) {
                    escapedChar = val << 4;
                    state = ReadSecondHex;
                }
                else { // state == ReadSecondHex
                    escapedChar |= val;
                    oss << escapedChar;
                    state = ReadAnyChar;
                }
            }
            break;
        }
    }

    if (state != Accept) {
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "Unexpected end: uri %s", encodedUri.c_str());
        return VPL_ERR_INVALID;
    }

    decodedUri.assign(oss.str());
    return VPL_OK;
}

std::string VPLHttp_DecodeUri(const std::string &encodedUri)
{
    std::string decodedUri;
    int err = VPLHttp_DecodeUri(encodedUri, decodedUri);
    if (err) return "";
    return decodedUri;
}

#ifdef TEST_VPLHTTP_DECODEURI
static void testVPLHttp_DecodeUri(const std::string &teststr, const std::string &ans)
{
    std::string result;
    int err = VPLHttp_DecodeUri(teststr, result);
    if (err) {
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "\"%s\" parse error", teststr.c_str());
        return;
    }
    if (result != ans) {
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "\"%s\" FAILED; got \"%s\", expected \"%s\"", teststr.c_str(), result.c_str(), ans.c_str());
    }
    else {
        VPL_LIB_LOG_INFO(VPL_SG_HTTP, "\"%s\" PASS", teststr.c_str());
    }
}

void testVPLHttp_DecodeUri();
void testVPLHttp_DecodeUri()
{
    testVPLHttp_DecodeUri("", "");
    testVPLHttp_DecodeUri("abc", "abc");
    testVPLHttp_DecodeUri("+", " ");
    testVPLHttp_DecodeUri("%31", "1");
    testVPLHttp_DecodeUri("%", "");   // should raise "unexpected end" error
    testVPLHttp_DecodeUri("%1", "");  // should raise "unexpected end" error
    testVPLHttp_DecodeUri("%z", "");  // should raise "unexpected non-hex digit" error
    testVPLHttp_DecodeUri("%ff", "\xff");
    testVPLHttp_DecodeUri("%FF", "\xff");
    testVPLHttp_DecodeUri("a%31b%FFc", "a1b\xff""c");
}
#endif

// TODO: copy & pasted to ccd_util.cpp; clean up.
// Mask any text between begin-pattern and end-pattern.
// Masking will be done by replacing the substring with a single asterisk.
// E.g., maskText("abc<tag>def</tag>ghi", _, "<tag>", "</tag>") -> "abc<tag>*</tag>ghi"
// If begin-pattern is found but no end-pattern, mask until end of text.
// E.g., maskText("abc<tag>def", _, "<tag>", "</tag>") -> "abc<tag>*"
static size_t maskText(/*INOUT*/char *text, size_t len, const char *begin, const char *end)
{
    char *p = text;
    while ((p = VPLString_strnstr(p, begin, text + len - p)) != NULL) {
        // assert: "p" points to occurrence of begin-pattern.
        p += strlen(begin);
        // assert: "p" points to one char after begin-pattern.
        if (p >= text + len) break;

        char *q = VPLString_strnstr(p, end, text + len - p);
        if (q == NULL) {  // no end-pattern
            q = text + len;
        }
        // assert: "q" points to beginning of end-pattern, or one-char past end of text.

        // assert: text between begin-pattern and end-pattern is [p,q)

        // replace non-empty text inbetween with a single asterisk
        // we do this to hide the actual length of the text inbetween
        if (p < q) {
            *p++ = '*';
            if (p < q) {
                if (q < text + len) {
                    memmove(p, q, text + len - q);
                }
                len -= q - p;
            }
        }
        // assert: "p" points to beginning of end-pattern

        // prepare for next search
        p += strlen(end);
        if (p >= text + len) break;
    }
    return len;
}

void VPL_LogHttpBuffer(const char* label, const void* buf, size_t len)
{
    char* worktext = (char*)malloc(len);
    if (worktext == NULL) {
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "Failed to malloc memory");
        return;
    }
    ON_BLOCK_EXIT(free, worktext);
    memcpy(worktext, buf, len);

    size_t origLen = len;
    len = maskText(worktext, len, "<accessTicket>", "</accessTicket>");
    len = maskText(worktext, len, "<devSpecAccessTicket>", "</devSpecAccessTicket>");
    len = maskText(worktext, len, "<Password>", "</Password>");
    len = maskText(worktext, len, "<PlatformKey>", "</PlatformKey>");
    len = maskText(worktext, len, "<RenewalToken>", "</RenewalToken>");
    len = maskText(worktext, len, "<SecretDeviceCredentials>", "</SecretDeviceCredentials>");
    len = maskText(worktext, len, "<ServiceTicket>", "</ServiceTicket>");
    len = maskText(worktext, len, "<serviceTicket>", "</serviceTicket>");
    len = maskText(worktext, len, "<SessionKey>", "</SessionKey>");
    len = maskText(worktext, len, "<SessionSecret>", "</SessionSecret>");
    len = maskText(worktext, len, "<WeakToken>", "</WeakToken>");
    // Intended to match both "userPwd" and "reenterUserPwd" (from OPS register InfraHttpRequest).
    len = maskText(worktext, len, "serPwd=", "&");
    len = maskText(worktext, len, "x-ac-serviceTicket: ", "\x0a");
    len = maskText(worktext, len, "X-ac-serviceTicket: ", "\x0a");
    // **IMPORTANT** Be sure to update #Util_LogSensitiveString() if a sensitive string can
    //   also be present at the CCDI layer.

    // Check for any non-ASCII characters
    int numToPrint = 0;
    VPL_BOOL hasBinaryData = VPL_FALSE;
    for (; numToPrint < (int)len; numToPrint++) {
        if (!VPL_IsSafeForPrintf(worktext[numToPrint])) {
            // Found unprintable character; display as "binary data".
            hasBinaryData = VPL_TRUE;
            goto log_now;
        }
    }
    // All characters printable; display verbatim, but chop off any trailing "\r\n".
    if ((numToPrint > 0) && (worktext[numToPrint-1] == '\n')) {
        numToPrint--;
    }
    if ((numToPrint > 0) && (worktext[numToPrint-1] == '\r')) {
        numToPrint--;
    }
log_now:
    VPL_LIB_LOG_ALWAYS(VPL_SG_HTTP, "HTTP %s: len="FMTuSizeT"%s, text=%.*s%s",
            label,
            len,
            ((len != origLen) ? " (filtered)" : ""),
            numToPrint, worktext, // (must specify how many bytes of "worktext" to actually print)
            (hasBinaryData ? "[[binary data]]" : "")
            );
}

// split all the tokens between slashes and stop at first ? or end of line
void VPLHttp_SplitUri(const std::string &uri, std::vector<std::string>& ret)
{
    const std::string delimit = "/?";
    ret.clear();

    if (uri.empty() || uri[0] != '/')
        return;

    size_t start = 1;
    size_t end = uri.find_first_of(delimit, start);
    while (end != std::string::npos && uri[end] != '?') {
        ret.push_back(uri.substr(start, end-start));
        start = end + 1;
        end = uri.find_first_of(delimit, start);
    }
    if (end-start > 0) {
        // push_back all query parameters unparsed at Uri end in one return element.
        ret.push_back(uri.substr(start,end-start));
    }
}

// Example:
//   Input: http://junk.com/level1/level2?parm1=value1&parm2=value2&parm3=value3
//   Output: paramValuePairs_out - map consisting of the pairs
//               (parm1, value1), (parm2, value2) and (parm3, value3)
void VPLHttp_SplitUriQueryParams(const std::string& uri,
                                 std::map<std::string, std::string>& paramValuePairs_out)
{
    // For a URI, example http://www.example.com?query1=40&query2=50&query3=60
    // This function will place the pairs (query1,40), (query2,50), and (query3,60)
    // in the map queryPairs_out.
    paramValuePairs_out.clear();
    std::size_t nextArgIndex = uri.find('?');

    for(;nextArgIndex != std::string::npos && nextArgIndex < uri.size()
        ;nextArgIndex = uri.find('&', nextArgIndex))
    {
        ++nextArgIndex;  // skip over '?' or '&' delimeter
        if (nextArgIndex >= uri.size()) {
            break;
        }

        std::size_t endArgIndex = uri.find('&', nextArgIndex);
        if (endArgIndex == std::string::npos) {
            endArgIndex = uri.size();
        }
        std::string argKeyValue = uri.substr(nextArgIndex, endArgIndex - nextArgIndex);
        std::size_t equalIndex = argKeyValue.find('=');
        if (equalIndex == std::string::npos) {
            continue;
        }
        std::string key = argKeyValue.substr(0, equalIndex);
        std::string value = argKeyValue.substr(equalIndex+1, argKeyValue.size()-equalIndex+1);
        paramValuePairs_out[key] = value;
    }
}

void VPLHttp_Trim(std::string &s)
{
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
    s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
}

void VPLHttp_ParseRangesSpec(const std::string &rangesSpec, std::vector<std::pair<std::string, std::string> > &ranges)
{
    // http://www.w3.org/Protocols/rfc2616/rfc2616-sec14.html#sec14.35
    // This function supports Acer extension that allows "bytes=" to precede each byte-range-spec.
    // For details, see http://bugs.ctbg.acer.com/show_bug.cgi?id=1020

    size_t startToken = 0;
    size_t endToken = 0;
    bool isBytesUnit = false;

    for(;startToken < rangesSpec.size(); startToken = endToken + 1) {
        std::string currentToken;

        endToken = rangesSpec.find(',', startToken);
        if (endToken != std::string::npos) {
            currentToken.assign(rangesSpec, startToken, endToken-startToken);
        }
        else {
            currentToken.assign(rangesSpec, startToken, std::string::npos);
            endToken = rangesSpec.size();
        }

        // Check for "bytes=" in byte-range-spec.
        // Example:
        //     bytes=100-199
        size_t optionalBytes = currentToken.find("=");
        if(optionalBytes != std::string::npos) {
            std::string str_bytes_unit = currentToken.substr(0, optionalBytes);
            VPLHttp_Trim(str_bytes_unit);
            if(str_bytes_unit.compare("bytes") != 0) {
                // Invalid range unit
                isBytesUnit = false;
                continue;
            }
            isBytesUnit = true;
            // Skip past "="
            currentToken = currentToken.substr(optionalBytes+1);
        }
        if(!isBytesUnit) {
            // 3.12: The only range unit defined by HTTP/1.1 is "bytes". HTTP/1.1
            // implementations MAY ignore ranges specified using other units.
            continue;
        }

        // currentToken should be in format "( byte-range-spec | suffix-byte-range-spec )"
        // Referring to 14.35 of http://www.ietf.org/rfc/rfc2616.txt

        // Finding dash in: byte-range-spec = first-byte-pos "-" [last-byte-pos]
        size_t dashPos = currentToken.find('-');
        if(dashPos == std::string::npos) {
            continue;
        }

        // http://www.ietf.org/rfc/rfc2616.txt  Secion 14.35
        // first-byte-pos  = 1*DIGIT
        // last-byte-pos   = 1*DIGIT
        std::string str_first_byte_pos;
        std::string str_last_byte_pos;
        str_first_byte_pos = currentToken.substr(0, dashPos);
        str_last_byte_pos = currentToken.substr(dashPos + 1);
        VPLHttp_Trim(str_first_byte_pos);
        VPLHttp_Trim(str_last_byte_pos);

        // Eliminate range definitions with invalid characters
        if(str_first_byte_pos.find_first_not_of("0123456789") != std::string::npos) {
            continue;
        }
        if(str_last_byte_pos.find_first_not_of("0123456789") != std::string::npos) {
            continue;
        }

        // At least one of the two ends must be specified.
        if (str_first_byte_pos.empty() && str_last_byte_pos.empty()) {
            continue;
        }

        u64 firstBytePos = VPLConv_strToU64(str_first_byte_pos.c_str(), NULL, 10);
        u64 lastBytePos = VPLConv_strToU64(str_last_byte_pos.c_str(), NULL, 10);
        if (!str_last_byte_pos.empty() && firstBytePos > lastBytePos) {
            continue;
        }

        ranges.push_back(std::make_pair(str_first_byte_pos, str_last_byte_pos));
    }
}

int VPLHttp_ConvToAbsRange(const std::pair<std::string, std::string> &range, u64 entitySize,
                           u64 &absFirstBytePos, u64 &absLastBytePos)
{
    int err = VPL_OK;

    if (range.first.empty()) {
        if (range.second.empty()) {
            err = VPL_ERR_INVALID;
        }
        else {
            // suffix-byte-range-spec case
            u64 suffixLen = VPLConv_strToU64(range.second.c_str(), NULL, 10);
            absFirstBytePos = entitySize - suffixLen;
            absLastBytePos = entitySize - 1;
        }
    }
    else {
        if (range.second.empty()) {
            // last-byte-pos missing case
            absFirstBytePos = VPLConv_strToU64(range.first.c_str(), NULL, 10);
            absLastBytePos = entitySize - 1;
        }
        else {
            absFirstBytePos = VPLConv_strToU64(range.first.c_str(), NULL, 10);
            absLastBytePos = VPLConv_strToU64(range.second.c_str(), NULL, 10);
            if (absFirstBytePos > absLastBytePos)
                err = VPL_ERR_INVALID;
            else if (absLastBytePos >= entitySize)
                absLastBytePos = entitySize - 1;
            // ELSE no change needed to absLastBytePos
        }
    }

    return err;
}

