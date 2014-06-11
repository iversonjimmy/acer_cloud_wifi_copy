//
//  Copyright (C) 2007-2009, BroadOn Communications Corp.
//
//  These coded instructions, statements, and computer programs contain
//  unpublished  proprietary information of BroadOn Communications Corp.,
//  and are protected by Federal copyright law. They may not be disclosed
//  to third parties or copied or duplicated in any form, in whole or in
//  part, without the prior written consent of BroadOn Communications Corp.
//

#ifndef __VPLEX_WSTRING_H__
#define __VPLEX_WSTRING_H__

#include <vplex_plat.h>
#include <wchar.h>

#ifdef  __cplusplus
extern "C" {
#endif

//============================================================================
/// @name vpl_wstring
/// Functions to map between different string encodings, such as UTF-8,
/// UTF-16 (Big Endian), and local-host-defined wchar_t strings.
///@{

//% TODO: if the input is stated to be null-terminated, should we really ask for "inlen"?
bool vpl_wstring_to_UTF16(const wchar_t* in, size_t inlen, utf16be* out, size_t* outlen);
///< Transcodes a given null-terminated wchar_t wide string to a
///< null-terminated string encoded in UTF-16BE.  No BOMs are present or inserted.
///<
///< @param[in]      in      The input wide string.
///< @param[in]      inlen   The length of the input wide string.
///< @param[out]     out     The address at which the UTF-16BE string will be returned on success.
///< @param[in,out]  outlen [in]:  Address of the maximum size of the buffer, in bytes.
///<                      \n[out]: Length, in bytes, of the returned UTF-16BE string.
///< @note Due to restrictions of some hosts supported by this library,
///<   the input string \a should contain only characters in the BMP subset
///<   of UTF-16. This restriction applies on all platforms, irrespective
///<   of the local definition of wchar_t.


//% TODO: Doc: Is \a inlen the number of bytes or u16s?  Does this include the null-terminator?
//% TODO: Doc: Does \a out include the null-terminator?
//% TODO: if the input is stated to be null-terminated, should we really ask for "inlen"?
bool vpl_UTF16_to_wstring(const utf16be* in, size_t inlen, wchar_t* out, size_t* outlen);
///< Transcodes a given, null-terminated string encoded in UTF16-BE,
///< to a null-terminated  wchar_t wide string. No BOMs are present or inserted.
///<
///< @param[in]      in      The input UTF-16BE string.
///< @param[in]      inlen   The length of the input string.
///< @param[out]     out     The address at which the wide string will be returned on success.
///< @param[in,out]  outlen [in]:  Address of the maximum size of the buffer, in bytes.
///<                      \n[out]: Length, in bytes, of the returned UTF-8 string.
///< @note Due to restrictions of some hosts supported by this library,
///<   the input string \a should contain only characters in the BMP subset
///<   of UTF-16. This restriction applies on all platforms, irrespective
///<   of the local definition of wchar_t.


//% TODO: if the input is stated to be null-terminated, should we really ask for "inlen"?
bool vpl_UTF16_to_UTF8(const u16* in, size_t inlen, utf8* out, size_t* outlen);
///< Transcodes a given, null-terminated wide string encoded in UTF-16BE, to a
///< string encoded in UTF-8.
///<
///< @param[in]      in      The input wide string.
///< @param[in]      inlen   The length, in number of 16-bit words (code units), of \a in.
///< @param[out]     out     The address at which the UTF-8 string will be returned on success.
///< @param[in,out]  outlen [in]:  Address of the maximum size of the buffer, in bytes.
///<                      \n[out]: Length, in bytes, of the returned UTF-8 string.
///< @note Due to restrictions of some hosts supported by this library,
///<   the input string \a should contain only characters in the BMP subset
///<   of UTF-16. This restriction applies on all platforms, irrespective
///<   of the local definition of wchar_t.


//% TODO: if the input is stated to be null-terminated, should we really ask for "inlen"?
bool vpl_UTF8_to_UTF16(const utf8* in, size_t inlen, u16* out, size_t* outlen);
///< Transcodes a given null-terminated string encoded in UTF-8,
///< to a null-terminated wide string encoded as UTF-16BE.
///<
///< @param[in]      in      The UTF-8 string input.
///< @param[in]      inlen   The length, in bytes, of the input UTF-8 string.
///< @param[out]     out     The address at which the UTF-16BE string will be returned on success
///< @param[in,out]  outlen [in]:  Address of the maximum size of the buffer, in bytes.
///<                      \n[out]: Length, in bytes, of the returned UTF-8 string.
///< @note Due to restrictions of some hosts supported by this library,
///<   the input string \a should contain only characters in the BMP subset
///<   of UTF-16. This restriction applies on all platforms, irrespective
///<   of the local definition of wchar_t.


void vplUTF16Copy(u16* dest, size_t* destlen, const u16* src);
///< Copies the UTF16 string.
//% TODO: documentation


typedef enum VPLUtf8ByteType
{
    VPLUtf8_NULL_TERM,
    ///< The null-terminator, '\\0'.
    
    VPLUtf8_ASCII,
    ///< A single-byte ASCII character other than '\\0'.
    
    VPLUtf8_START_OF_2_BYTE_CHAR,
    ///< The first byte in a 2 byte sequence that represents a single character (code point).
    
    VPLUtf8_START_OF_3_BYTE_CHAR,
    ///< The first byte in a 3 byte sequence that represents a single character (code point).
    
    VPLUtf8_START_OF_4_BYTE_CHAR,
    ///< The first byte in a 4 byte sequence that represents a single character (code point).
    
    VPLUtf8_BYTE_2_3_OR_4,
    ///< Second, third, or fourth byte of a sequence that represents a single character (code point).
    
    VPLUtf8_INVALID
    ///< The byte value should not appear in a valid UTF-8 string.
    
} VPLUtf8ByteType;
///< The meaning of a byte within a UTF-8 string.


VPLUtf8ByteType VPLString_ClassifyUtf8Byte(utf8 byte);
///< Returns the meaning of the byte value within a UTF-8 string.
///< @return The #VPLUtf8ByteType enum value that describes the meaning of
///<     \a byte when encountered within a UTF-8 string.


///@}

#ifdef  __cplusplus
}
#endif

#endif // include guard
