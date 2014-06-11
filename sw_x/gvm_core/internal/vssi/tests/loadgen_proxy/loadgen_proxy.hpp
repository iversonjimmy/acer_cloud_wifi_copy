/*
 *  Copyright 2013 Acer Cloud Technology Inc.
 *  All Rights Reserved.
 *
 *  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
 *  TRADE SECRETS OF ACER CLOUD TECHNOLOGY INC.
 *  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
 *  THE PRIOR EXPRESS WRITTEN PERMISSION OF ACER CLOUD TECHNOLOGY INC.
 *
 */

#ifndef __LOADGEN_PROXY_HPP__
#define __LOADGEN_PROXY_HPP__

#include "vplu_common.h"
#include "vplu_types.h"

#include <stdlib.h>

void compute_hmac(const char* buf, size_t len,
                  const void* key, char* hmac, int hmac_size);
void encrypt_data(char* dest, const char* src, size_t len, 
                  const char* iv_seed, const char* key);
void decrypt_data(char* dest, const char* src, size_t len, 
                  const char* iv_seed, const char* key);
void sign_vss_msg(char* msg, const char* enc_key, const char* sign_key);

#endif // include guard
