//
//  Copyright 2010 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#ifndef __VPLU_SSTR_H__
#define __VPLU_SSTR_H__

//============================================================================
/// @file
/// VPL utility (VPLU) macro for converting things to std::string.
//============================================================================

#include "vpl_types.h"
#include <sstream>

/// Converts x_ to a std::string.  x_ can contain concatenation, just like a std::ostream.
/// Sample usage:
///   \code
///    int i = 42;
///    std::string s1 = SSTR( i );
///    std::string s2 = SSTR( "Value of i is: " << i );
///    std::string s3 = SSTR( i << " is the answer to the universe, life, and everything." );
///   \endcode
#define SSTR(x_) static_cast<std::ostringstream& >(std::ostringstream() << std::dec << x_).str()

// This is from http://rootdirectory.de/wiki/SSTR%28%29
// Note that std::dec is actually important:
//   Setting the integer output format to "decimal" looks like a non-op since decimal is the
//   default anyway. However, it has the effect of turning the type of our object from
//   ostringstream to ostream &. This is important if the first data element given to the macro
//   is a pointer (e.g. a C-style string): If we would use the ostringstream object directly,
//   function lookup would use ostream::operator<<( void * ) in this case (giving us a boolean
//   value) instead of the global operator<<( ostream &, char const * ) we want (giving us a
//   string output).

#endif // include guard
