
#ifndef __MANIFEST_H__
#define __MANIFEST_H__

//#include "loadgen_common.h"

#include <vplu.h>

struct manifest_file_s {
    struct manifest_file_s *next;
    char *path;
    u32 size;
};
typedef struct manifest_file_s manifest_file_t;

struct manifest_s {
    manifest_file_t *list;
    manifest_file_t **array;
    u32 file_cnt;
};
typedef struct manifest_s manifest_t;

int mani_load(manifest_t *mani, const char *path, bool create_dir, int thrd_cnt, char *root_dir, int thrd_id_start, bool randomize);

void mani_free(manifest_t *mani);

#endif // __MANIFEST_H__
