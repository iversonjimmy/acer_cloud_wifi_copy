#include "km_types.h"
#include "compiler.h"
#include "bits.h"
#include "x86.h"
#include "setup/gvmdefs.h"
#include "printf.h"
#include "string.h"
#include "assert.h"
#include "malloc.h"
#include "vm.h"
#include "gpt.h"

#define NUM_BLOBS 64

typedef struct {
    void *blob;
    u32 len;
    u32 id;
} ghv_blob_t;

static ghv_blob_t blobs[NUM_BLOBS];

static ghv_blob_t *get_free_blob(void)
{
    static int next_blob = 0;
    int i = next_blob;
    ghv_blob_t *r = NULL;

    do {
	if (blobs[i].blob == NULL) {
	    r = &blobs[i];	    
	}
	i = (i + 1) % NUM_BLOBS;
    } while (i != next_blob);

    next_blob = i; 
    return r;
}

void *ghv_blob_retrieve(u32 id, size_t *len)
{
    int i;
    void *b;

    for (i = 0; i < NUM_BLOBS; i++) {
	if (blobs[i].blob && blobs[i].id == id) {
	    if (len) {
		*len = blobs[i].len;
		b = blobs[i].blob;
		blobs[i].blob = NULL;
		return b;
	    }
	}
    }

    return NULL;
}

int ghv_upload(u32 id, addr_t gvaddr, size_t len)
{
    if (len > MAX_BLOB_SIZE) {
	return -1;
    }

    if (len == 0) {
	return 0;
    }

    ghv_blob_t *blob = get_free_blob();
    if (!blob) {
	printf("%s: max num of blobs reached.\n", __func__);
	return -1;
    }

    blob->blob = malloc(len);
    if (!blob->blob) {
	printf("%s: cannot allocated %u bytes for blob %x.\n", __func__,
	       (u32)len, id);
	return -1;
    }
    blob->len = len;

    if (copy_from_guest(blob->blob, gvaddr, len)) {
	printf("%s: cannot copy from guest\n", __func__);
	free(blob->blob);
	blob->blob = NULL;
	blob->len = 0;
	return -1;
    }

    blob->id = id;
    return 0;
}
