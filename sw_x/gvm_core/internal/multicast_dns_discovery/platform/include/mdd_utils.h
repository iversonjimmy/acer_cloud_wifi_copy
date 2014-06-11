#ifndef mdd_utils_h
#define mdd_utils_h

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <string.h>

#define MDD_TRUE 1
#define MDD_FALSE 0

#define MDD_OK 0
#define MDD_ERROR -1

char* MDDUtils_Strdup(const char *str);

#ifdef __cplusplus
}
#endif

#endif //mdd_utils_h
