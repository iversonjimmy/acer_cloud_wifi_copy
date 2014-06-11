
#include "manifest.h"

#include "vplex_trace.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#define MAX_PATH    128

// Note: *this* malloc() zeros its data, no reason to memset.
#define ARRAY(t, n) (t *)malloc(sizeof(t) * n)
#define ALLOC(t)        ARRAY(t, 1)

#define MODE 0777
int
mani_load(manifest_t *mani, const char *path, bool create_dir, int thrd_cnt, char *root_dir, int thrd_id_start, bool randomize)
{
    FILE *fp;
    int cnt;
    char *file_path;
    u32 file_size;
    char path_type; 
    int line_no = 0;
    manifest_file_t *mf;
    manifest_file_t *tmp = NULL;
    u32 j;
    char buf[MAX_PATH + 1];
    buf[MAX_PATH] = '\0';

    memset(mani, 0, sizeof(*mani));

    if ( (fp = fopen(path, "r")) == NULL ) {
        fprintf(stderr, "Unable to open manifest file %s\n", path);
        return -1;
    }
    // read it in
    while ( (cnt = fscanf(fp, "%c %as %d\n", &path_type, &file_path, &file_size))
            != EOF) {
        line_no++;

        if ( cnt < 1 ) {
            continue;
        }
        if (path_type == 'D') {
            if (create_dir) {
                int i;
                for (i = thrd_id_start; i < thrd_cnt; ++i) {
                    if (NULL != root_dir) {
                        snprintf(buf, MAX_PATH, "%s/%04d/%s", root_dir, i, file_path);
                    } else {
                        snprintf(buf, MAX_PATH, "%s/%04d/%s", root_dir, i, file_path);
                    }
                    if (mkdir(buf, MODE)) {
                        if (errno != EEXIST) {
                            fprintf(stderr, "Unable to create directory %s: %d\n", buf, errno);
                            return -1;
                        }
                    } else {
                        VPLTRACE_LOG_FINE(TRACE_APP, 0,
                                          "Created manifest directory: %s\n", buf);
                    }
                }
            }
                continue;
            }
            if ( path_type != 'F' ) {
                continue;
            }

        if ( cnt != 3 ) {
            if ( cnt == 2 ) {
                free(file_path);
            }
            fprintf(stderr, "bad line in manifest - line %d\n", line_no);
            goto fail;
        }

        // create a record for this match
        mf = ALLOC(manifest_file_t);
        if ( mf == NULL ) {
            fprintf(stderr, "out of mem - line %d\n", line_no);
            goto fail;
        }

        mf->path = file_path;
        mf->size = file_size;
        if ( tmp ) {
            tmp->next = mf;
        }
        else {
            mani->list = mf;
        }
        tmp = mf;
        mani->file_cnt++;
    }

    // create the array
    mani->array = ARRAY(manifest_file_t *, mani->file_cnt);
    if ( mani->array == NULL ) {
        fprintf(stderr, "Out of mem on the array\n");
        goto fail;
    }

    tmp = mani->list;
    for( j = 0 ; j < mani->file_cnt ; j++ ) {
        mani->array[j] = tmp;
        tmp = tmp->next;
    }
    if (randomize) {
        // Note that we do not initialize the seed 
        // so the list will always shuffle to the same result.
        for ( j = 0; j < mani->file_cnt; j++ )  {
            int k = rand() % mani->file_cnt;
            tmp = mani->array[k];
            mani->array[k] = mani->array[j];
            mani->array[j] = tmp;
        }
    }

printf("manifest loaded %d items out of %d lines shuffle %s\n",
    mani->file_cnt, line_no, randomize? "ON" : "OFF");

    return 0;

fail:
    fclose(fp);

    mani_free(mani);

    return -1;
}

void
mani_free(manifest_t *mani)
{
    manifest_file_t *mf = NULL;
    manifest_file_t *tmp = NULL;

    if ( mani->array ) {
        free(mani->array);
        mani->array = NULL;
    }

    for( mf = mani->list ; mf ;  mf = tmp ) {
        tmp = mf->next;
        if ( mf->path ) {
            free(mf->path);
        }
        free(mf);
    }

    mani->list = NULL;
    mani->file_cnt = 0;
}
