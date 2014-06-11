#include "ec_cache.h"

#include "ec_common.h"
#include "esc_data.h"

#define CACHE_HANDLE_MAKE(c, i) \
	((c->ch_id << 16) \
	 | ((c->ch_cache[i].cn_seq & 0xff) << 8) \
	 | (i))

s32
cache_find(ec_cache_head_t *head, u32 handle, void **datap)
{
	u32 ind;
	ec_cache_node_t *node;

	// make sure this really represents a title handle
	if ( !CACHE_HANDLE_IS_VALID(head, handle) ) {
		DBLOG("%s: Invalid handle - 0x%08x\n", head->ch_debug_name,
		      handle);
		return ESC_ERR_HANDLE_INVALID;
	}


	ind = CACHE_HANDLE_IND_GET(handle);
	node = &head->ch_cache[ind];

	// See if the handle is stale.
	if ( (node->cn_data == NULL) ||
	     ((node->cn_seq & 0xff) != CACHE_HANDLE_SEQ_GET(handle)) ) {
		return ESC_ERR_HANDLE_STALE;
	}

	*datap = node->cn_data;
	return ESC_ERR_OK;
}

s32
cache_add(ec_cache_head_t *head, void *data, u32 *handle)
{
	ec_cache_node_t *node;
	u32 i;

	for( i = 0 ; i < head->ch_size ; i++ ) {
		if ( head->ch_cache[i].cn_data) {
			continue;
		}

		// found
		goto found;
	}

	DBLOG("%s: no space in cache!\n", head->ch_debug_name);
	return -1;
	
found:
	// populate the node
	node = &head->ch_cache[i];
	node->cn_data = data;

	// update the head
	head->ch_count++;

	// place it on the active list
	node->cn_next = head->ch_active;
	head->ch_active = node;
	if ( node->cn_next ) {
		node->cn_next->cn_last = node;
	}

	// return a handle to it.
	*handle = CACHE_HANDLE_MAKE(head, i);

	return 0;
}

void
cache_del(ec_cache_head_t *head, u32 handle)
{
	ec_cache_node_t *node;

	node = &head->ch_cache[CACHE_HANDLE_IND_GET(handle)];
	// remove from the head

	// check the start of the active list
	if ( head->ch_active == node) {
		head->ch_active = node->cn_next;
	}

	// remove from the list
	if ( node->cn_next ) {
		node->cn_next->cn_last = node->cn_last;
	}
	if ( node->cn_last ) {
		node->cn_last->cn_next = node->cn_next;
	}
	head->ch_count--;

	// clean up the node
	node->cn_next = node->cn_last = NULL;
	node->cn_data = NULL;
	node->cn_seq++;
}
