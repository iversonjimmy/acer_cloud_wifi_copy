#define __LOG_H__

#undef  LOG_ALWAYS
#undef  LOG_ERROR
#undef  LOG_WARN
#undef  LOG_INFO
#undef  LOG_FUNC_ENTRY
#undef  ASSERT

#define LOG_ALWAYS(x, args...)  printf(x, ## args); printf("\n");
#define LOG_ERROR(x, args...)   printf(x, ## args); printf("\n");
#define LOG_WARN(x, args...)    printf(x, ## args); printf("\n");
#define LOG_INFO(x, args...)    printf(x, ## args); printf("\n");
#define LOG_FUNC_ENTRY(a)
#define ASSERT(A)

#define log(x, args...)                                           \
            do {                                                  \
                char  ts[40];                                     \
                                                                  \
                printf("%s  test: ", pxd_ts(ts, sizeof(ts)));     \
                printf(x, ##args);                                \
            } while (0)

#define error(x, args...)                                         \
            do {                                                  \
                char  ts[40];                                     \
                                                                  \
                printf("%s  test: *** ", pxd_ts(ts, sizeof(ts))); \
                printf(x, ##args);                                \
            } while (0)
