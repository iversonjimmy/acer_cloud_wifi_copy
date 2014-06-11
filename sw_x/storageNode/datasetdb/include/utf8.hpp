/*
 *  Copyright 2012 Acer Cloud Technology, Inc.
 *  All Rights Reserved.
 *
 *  This software contains confidential information and
 *  trade secrets of Acer Cloud Technology, Inc.
 *  Use, disclosure or reproduction is prohibited without
 *  the prior express written permission of Acer Cloud Technology, Inc.
 */

#ifndef __UTF8_HPP__
#define __UTF8_HPP__

#include <string>

void utf8_upper(const char *s, std::string &s_upper);

int utf8_casencmp(int s1_len, const char *s1, int s2_len, const char *s2);

#endif  // __UTF8_HPP__
