#ifndef __EC_CACHE_H__
#define __EC_CACHE_H__

#include <km_types.h>

#define CACHE_HANDLE_ID(k)	   (((k) << 8) | 'H')

#define CACHE_HANDLE_ID_GET(h)	(((h) >> 16) & 0xffff)
#define CACHE_HANDLE_SEQ_GET(h)	(((h) >> 8) & 0xff)
#define CACHE_HANDLE_IND_GET(h)	((h) & 0xff)

#define CACHE_IS_FULL(c)	((c)->ch_count >= (c)->ch_size)

#define CACHE_DATA_IND_GET(c, i, t) (t*)((c)->ch_cache[i].cn_data)

// Make sure the index is within range and IDs match
#define CACHE_HANDLE_IS_VALID(c, h) \
	((CACHE_HANDLE_IND_GET(h) < c->ch_size) && \
	 (CACHE_HANDLE_ID_GET(h) == c->ch_id))

struct ec_cache_node_s {
	struct ec_cache_node_s    *cn_next;
	struct ec_cache_node_s    *cn_last;
	void 		       *cn_data;
	u32			cn_seq;
};
typedef struct ec_cache_node_s ec_cache_node_t;

struct ec_cache_head_s {
	ec_cache_node_t     *ch_cache;
	ec_cache_node_t     *ch_active;
	const char     *ch_debug_name;
	u32		ch_size;
	u32		ch_count;
	u32		ch_id;
};
typedef struct ec_cache_head_s ec_cache_head_t;

s32 cache_find(ec_cache_head_t *head, u32 handle, void **datap);
s32 cache_add(ec_cache_head_t *head, void *data, u32 *handle);
void cache_del(ec_cache_head_t *head, u32 handle);

#endif // __EC_CACHE_H__
