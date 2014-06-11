#ifndef __EC_DATA_INT_H__
#define __EC_DATA_INT_H__

// vim: sw=8:ts=8:noexpandtab

#include "ec_common.h"

#include "iosc.h"
#include "bsl.h"
#include "crypto_impl.h"
#include "crypto_ghv.h"
#include "istorage_impl.h"

#include "es_container.h"
#include "core_glue.h"
// #include "vmcall_ring_ghv.h"
// #include "es_hypercalls.h"
#include "esc_funcs.h"
#include "ghv_endian.h"
#include "es_storage.h"

#include "ec_cache.h"

#define EC_RIGHTS_MAX	(ES_ITEM_RIGHT_ACCESS_TITLE + 1)

struct ec_right_s {
	void		       *data;
	u32			cnt;
};
typedef struct ec_right_s hc_right;

// this defines an imported ticket
struct ec_ticket_s {
	struct ec_title_s      *title;	// list of referencing titles
	ESTitleId		title_id;
	u64			user_id;
	IOSCSecretKeyHandle	key;
	u32			cookie;
	u32			ref_cnt;
	u32			handle;
	bool			is_user_key;
	bool			is_user_sign;
	hc_right		rights[EC_RIGHTS_MAX];
};
typedef struct ec_ticket_s ec_ticket_t;

struct ec_selfmod_s {
	u32			start;
	u32			end;
};
typedef struct ec_selfmod_s ec_selfmod_t;

// this defines an imported content
struct ec_cfm_s {
	struct ec_title_s      *title;
	struct ec_cfm_s	       *next;
	struct ec_cfm_s	       *last;
	ESContentId		content_id;
	u32			content_handle;
	ESContentIndex		content_index;
	u32			blk_cnt;
	u32			blk_size;
	u32			page_size;
	u32			cfm_len;
	u32			pages_per_blk;
	bool			is_rw;
	bool			is_dirty;
	u8		       *cfm_data;
	IOSCHash256	       *hashes;
	u8		       *zero_bits;
};
typedef struct ec_cfm_s ec_cfm;

struct ec_dec_stream_s {
	struct ec_dec_stream_s *next;
	struct ec_dec_stream_s *last;
	ESContentId		content_id;
	IOSCHashContext 	hash_ctx;
	IOSCHash256 		tmd_hash;
	u8			ivec[16];
	u64			offset;
	u64			size;
};
typedef struct ec_dec_stream_s ec_dec_stream_t;

// This defines an imported title.
struct ec_title_s {
	struct ec_title_s      *data_link;
	struct ec_title_s      *parent_link;
	ESTitleId		title_id;
	u64			user_id;
	ec_ticket_t	       *ticket;
	ESV1TitleMeta		title_meta;
	IInputStream	       *tmd_rdr;
	void		       *tmd_data;
	u32			tmd_data_len;
	u32			tmd_len;
	void		       *tmd_certs[ESC_CERT_FRAME_MAX];
	u32			tmd_cert_cnt;
	u32			cookie;
	ec_cfm		       *cfm_list;
	ec_dec_stream_t	        dec_streams;	// dummy node
	u32			handle;
	u32			gss_size;
	bool			is_data_title;
	bool			data_is_referenced;
	bool			is_gss;
	bool			is_dirty;
};
typedef struct ec_title_s ec_title;

#define COPY_OUT(ua, s, f, e) 	\
	copy_to_guest(arg + offsetof(s,f), &(e), sizeof(e))

// Create the various caches
#define CACHE_INST(lname, uname, debug)	\
static ec_cache_node_t	__##lname##_cache_nodes[uname##_CACHE_MAX]; \
static ec_cache_head_t  __##lname##_cache = { \
	__##lname##_cache_nodes, \
	NULL, \
	debug, \
	uname##_CACHE_MAX, \
	0, \
	CACHE_HANDLE_ID(uname##_KEY) \
};
// End CACHE_INST macro

#define _NUM_PAGES(x)		(((x) + VM_PAGE_SIZE - 1)/ VM_PAGE_SIZE)

s32 eshi_title_import(u64 tid, u32 cookie, u8 **tmd_datap, u32 tmd_len, ec_ticket_t *tkt_sig, ec_ticket_t *tkt_data, ec_title **titlep);
s32 eshi_tmd_load(u32 *frames, u32 tmd_len, u8 **tmd_datap);
s32 eshi_title_release(u32 handle);

#endif // __EC_DATA_INT_H__
