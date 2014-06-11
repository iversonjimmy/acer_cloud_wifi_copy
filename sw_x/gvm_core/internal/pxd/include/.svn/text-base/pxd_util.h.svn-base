//
//  Copyright 2013 Acer Cloud Technology
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF ACER CLOUD TECHNOLOGY.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF Acer Cloud Technology.
//

#ifndef PXD_UTIL_H
#define PXD_UTIL_H

#ifdef __cplusplus
extern "C" {
#endif

void pxd_mutex_init  (VPLMutex_t *mutex);
void pxd_mutex_lock  (VPLMutex_t *mutex);
void pxd_mutex_unlock(VPLMutex_t *mutex);

void pxd_convert_address(VPLNet_addr_t address, char *buffer, int max_length);

#define clear_error(error)             \
            do {                       \
                (error)->error   = 0;  \
                (error)->message = 0;  \
            } while (0)

#ifdef __cplusplus
}
#endif

#endif /* PXD_UTIL_H */
