//
//  Copyright 2011-2013 Acer Cloud Technology
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF Acer Cloud Technology.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF Acer Cloud Technology.
//

#ifndef PXD_LOG_H
#define PXD_LOG_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 *  Define some logging macros.  They prepend a useful timestamp,
 *  since the one from the logging routines contains the local
 *  time, but no time zone.
 */
#define log_info(format, ...)                                                         \
            do {                                                                      \
                char log_buffer[255];                                                 \
                                                                                      \
                LOG_INFO("%s  pxd:  " format, pxd_ts(log_buffer, sizeof(log_buffer)), \
                    ##__VA_ARGS__);                                                   \
            } while (0)

#define log_warn(format, ...)                                                         \
            do {                                                                      \
                char log_buffer[255];                                                 \
                                                                                      \
                LOG_WARN("%s  pxd:  " format, pxd_ts(log_buffer, sizeof(log_buffer)), \
                    ##__VA_ARGS__);                                                   \
            } while (0)

#define log_error(format, ...)                                                         \
            do {                                                                       \
                char log_buffer[255];                                                  \
                                                                                       \
                LOG_ERROR("%s  pxd:  " format, pxd_ts(log_buffer, sizeof(log_buffer)), \
                    ##__VA_ARGS__);                                                    \
            } while (0)

#define log_always(format, ...)                                                         \
            do {                                                                        \
                char log_buffer[255];                                                   \
                                                                                        \
                LOG_ALWAYS("%s  pxd:  " format, pxd_ts(log_buffer, sizeof(log_buffer)), \
                    ##__VA_ARGS__);                                                     \
            } while (0)

#define log_info_1(format)                                                             \
            do {                                                                       \
                char log_buffer[255];                                                  \
                                                                                       \
                LOG_INFO("%s  pxd:  " format, pxd_ts(log_buffer, sizeof(log_buffer))); \
            } while (0)

#define log_info_2(format, arg)                                                        \
            do {                                                                       \
                char log_buffer[255];                                                  \
                                                                                       \
                LOG_INFO("%s  pxd:  " format, pxd_ts(log_buffer, sizeof(log_buffer)),  \
                    arg);                                                              \
            } while (0)

#define log_warn_1(format)                                                             \
            do {                                                                       \
                char log_buffer[255];                                                  \
                                                                                       \
                LOG_WARN("%s  pxd:  " format, pxd_ts(log_buffer, sizeof(log_buffer))); \
            } while (0)

#define log_error_1(format)                                                             \
            do {                                                                        \
                char log_buffer[255];                                                   \
                                                                                        \
                LOG_ERROR("%s  pxd:  " format, pxd_ts(log_buffer, sizeof(log_buffer))); \
            } while (0)

#define log_always_1(format, ...)                                                         \
            do {                                                                          \
                char log_buffer[255];                                                     \
                                                                                          \
                LOG_ALWAYS("%s  pxd:  " format, pxd_ts(log_buffer, sizeof(log_buffer)));  \
            } while (0)

#ifdef pxd_test
#define log_test(format, ...)                                                         \
            do {                                                                      \
                char log_buffer[255];                                                 \
                                                                                      \
                LOG_INFO("%s  pxd:  " format, pxd_ts(log_buffer, sizeof(log_buffer)), \
                    ##__VA_ARGS__);                                                   \
            } while (0)
#else
#define log_test(format, ...)
#endif

extern char *pxd_ts(char *, int);
extern char *pxd_print_time(char *, int, VPLTime_t);

#ifdef __cplusplus
}
#endif

#endif /* PXD_LOG_H */
