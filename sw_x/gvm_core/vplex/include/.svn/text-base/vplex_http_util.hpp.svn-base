#ifndef __VPLEX_HTTP_UTIL_HPP__
#define __VPLEX_HTTP_UTIL_HPP__

//============================================================================
/// @file
//============================================================================

#include "vplex_plat.h"
#include "vplex_time.h"

#include <map>
#include <string>
#include <utility>
#include <vector>

/**
 * Utility for the URL encoding
 * @param in String to be URL encoded
 * @param skip_delimiters (Optional) Instruct function not to escape the specified delimiters when met
 * @return Return encoded string
 */
std::string VPLHttp_UrlEncoding(const std::string &in, const char *skip_delimiters=NULL);

/**
 * Decode encoded URI
 * @param[in] encodedUri
 * @param[out] decodedUri only if VPL_OK
 * @return VPL_OK, VPL_ERR_INVALID
 */
int VPLHttp_DecodeUri(const std::string &encodedUri, std::string &decodedUri);

/**
 * Decode encoded URI.
 * @param[in] encodedUri
 * @return The decoded URI; "" if there was an error.
 */
std::string VPLHttp_DecodeUri(const std::string &encodedUri);

/**
 * Utility for the URI split.
 * Splits all the tokens between slashes and stops at first ? or end of line.
 * @param[in] uri String to be URL split.
 * @param[out] ret String vector as result.
 */
void VPLHttp_SplitUri(const std::string &uri, std::vector<std::string>& ret);

/**
 * Extracts the query parameters (all the tokens between the first ? and the end.
 * @param uri URI to be split.
 *     For example, http://junk.com/level1/level2?parm1=value1&parm2=value2&parm3=value3
 * @param paramValuePairs_out Map consisting of the query parameters.  In the above example,
 *     the pairs will be (parm1, value1), (parm2, value2) and (parm3, value3).
 */
void VPLHttp_SplitUriQueryParams(const std::string& uri,
                                 std::map<std::string, std::string>& paramValuePairs_out);

/**
 * Remove whitespace from both ends of the string.
 */
void VPLHttp_Trim(std::string &s);

/**
 * Parse Range field value.
 * @param[in] rangesSpec
 * @param[out] ranges vector of pairs of numbers
 * @note We support Acer extension that allows the byte-range-spec to be preceded by bytes-unit.
 *       See bug 1020 for details.
 */
void VPLHttp_ParseRangesSpec(const std::string &rangesSpec, std::vector<std::pair<std::string, std::string> > &ranges);

int VPLHttp_ConvToAbsRange(const std::pair<std::string, std::string> &range, u64 entitySize,
                           u64 &absFirstBytePos, u64 &absLastBytePos);

#endif // include guard
