/*
 * Copyright 2010 iGware Inc. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; version 2 of the License and no later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE, GOOD TITLE or
 * NON INFRINGEMENT. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * The full GNU General Public License is included in this distribution in
 * the file called "COPYING".
 *
 */

#ifndef __ESC_DATA_H__
#define __ESC_DATA_H__

// The following should be calculated
#define	ESC_PAGES_PER_CRYPTO_BLOCK	8
#define ESC_TITLE_KEYLEN		16
#define ESC_BLOCK_PAGE_MAX		8

// XXX - Need real numbers for these things. Current numbers
// are completely arbitrary.
#define ESC_INIT_FRAME_MAX		4
#define ESC_TICKET_FRAME_MAX		4
#define ESC_CERT_FRAME_MAX		10
#define ESC_TMD_FRAME_MAX		6
#define ESC_DECRYPT_FRAME_MAX		10
#define ESC_TITLE_MAX		20
#define ESC_CONTENT_MAX		30
#define ESC_TICKET_MAX		(2 * ESC_TITLE_MAX)
#define ESC_PAGE_FRAME_MAX		20

// XXX - needs to match ES_DEVICE_CERT_SIZE in estypes.h
// But that file can't safely be pulled in by ghvcmod, bvd, etc.
#define ESC_DEV_CERT_SIZE		384

#define ESC_ERR_OK              0
#define ESC_ERR_UNSPECIFIED     -17101
#define ESC_ERR_PARMS           -17102
#define ESC_ERR_CERTS_VERIFY    -17103
#define ESC_ERR_TKT_VERIFY      -17104
#define ESC_ERR_EXISTS          -17105
#define ESC_ERR_NOMEM           -17106
#define ESC_ERR_TITLE_MATCH     -17107
#define ESC_ERR_TMD_VERIFY      -17108
#define ESC_ERR_CONTENT_MATCH   -17109
#define ESC_ERR_BLK_HASH        -17110
#define ESC_ERR_CONTENT_HASH    -17111
#define ESC_ERR_CONTENT_INVALID -17112
#define ESC_ERR_TKT_RIGHTS      -17113
#define ESC_ERR_NO_RIGHT        -17114
#define ESC_ERR_TITLE_GVM       -17115
#define ESC_ERR_HANDLE_INVALID  -17116
#define ESC_ERR_HANDLE_STALE    -17117
#define ESC_ERR_BUSY            -17118
#define ESC_ERR_GSS_SIGN        -17119
#define ESC_ERR_USER_ID         -17120
#define ESC_ERR_READONLY        -17121
#define ESC_ERR_SIZE_MISMATCH   -17122
#define ESC_ERR_BUF_ACCESS      -17123
#define ESC_ERR_NO_CONTENT      -17124
#define ESC_ERR_TKT_TYPE        -17125
#define ESC_ERR_PAGE_PROT_ON    -17126
#define ESC_ERR_PAGE_PROT_OFF   -17127
#define ESC_ERR_TKT_KEYID       -17128
#define ESC_ERR_TKT_DEVICEID    -17129
#define ESC_ERR_NOT_YET_INIT	-17130
#define ESC_ERR_CRED_LOADED	-17131
#define ESC_ERR_CRED_DECRYPT	-17132
#define ESC_ERR_CRED_SIZE	-17133
#define ESC_ERR_NOT_INITIALIZED	-17134
#define ESC_ERR_CRYPTO		-17135
#define ESC_ERR_ALREADY_INIT	-17136

#define ESC_MATCH_ANY		0xffffffff

#define ESC_INVALID_ID		(0xffffffff)
#define ESC_INVALID_IND		(0xffffffff)

#define ESC_INIT_MODE_NORMAL		0
#define ESC_INIT_MODE_INSECURE_CRED	1

struct esc_device_id_s {
	u32		device_id;
	u32		device_type;
	s32		error;
	u8		device_cert[ESC_DEV_CERT_SIZE];
	u32		pad;
};
typedef struct esc_device_id_s esc_device_id_t;

struct esc_init_s {
	u8	        *cred_default;
	u32		default_len;
	s32		error;
        u32             pad;
};
typedef struct esc_init_s esc_init_t;

struct esc_cred_s {
	u8		*credSecret;
	u32		secretLen;
	u8		*credClear;
	u32		clearLen;
	u32		mode;
	s32		error;
};
typedef struct esc_cred_s esc_cred_t;

// The ticket and certs needs to be page aligned.
struct esc_ticket_s {
    	u64 		title_id;
	u64		user_id;
	u32		is_title;
	u32		is_user;
	u32		ticket_frames[ESC_TICKET_FRAME_MAX];
	u32		cert_frames[ESC_CERT_FRAME_MAX];
	u32		ticket_size;
	u32		cert_count;
	u32		cookie;
	u32		chal_handle;
	u32		handle;
	s32		error;
};
typedef struct esc_ticket_s esc_ticket_t;

struct esc_resource_rel_s {
    	u32 		handle;
	s32		error;
};
typedef struct esc_resource_rel_s esc_resource_rel;

struct esc_title_s {
    	u64 		title_id;
	u32		tmd_frames[ESC_TMD_FRAME_MAX];
	u32		ticket_sig_handle;
	u32		ticket_data_handle;
	u32		tmd_size;
	u32		cookie;		// import cookie
	u32		title_handle;	// return handle
	s32		error;
};
typedef struct esc_title_s esc_title_t;

struct esc_title_export_s {
	u32		title_handle;
	u32		ticket_handle;
	u32		tmd_frames[ESC_TMD_FRAME_MAX];
	u32		tmd_size;
	s32		error;
	u32		pad;
};
typedef struct esc_title_export_s esc_title_export_t;

//
// This is a special purpose call
// - It performs a title import and a content import at the same time
// - This would be more natural to the existing APIs if we were importing
// - pre-published templates.
//
struct esc_gss_create_s {
	u64		user_id;
	u64		title_id;
	u32		ticket_sign_handle;
	u32		ticket_data_handle;
	u32		title_handle;
	u32		content_id;
	u32		cookie;
	u32		return_title_handle;
	u32		return_content_handle;
	s32		error;
};
typedef struct esc_gss_create_s esc_gss_create_t;

struct esc_content_s {
	u32		title_handle;
	s32		error;
	u32		content_id;
	u32		content_handle;
	u32		size;
	u32		frames[0];
};
typedef struct esc_content_s esc_content_t;

struct esc_content_ind_s {
	u32		title_handle;
	s32		error;
	u32		content_ind;
	u32		content_handle;
	u32		size;
	u32		frames[0];
};
typedef struct esc_content_ind_s esc_content_ind_t;


struct esc_content_export_s {
	u32		content_handle;
	u32		size;
	s32		error;
	u32		pad;
	u32		frames[0];
};
typedef struct esc_content_export_s esc_content_export_t;

struct esc_block_s {
	u32		content_handle;
	u32		block_number;
	u32		page_cnt;
	s32		error;
	u32		frames_in[ESC_BLOCK_PAGE_MAX];
	u32		frames_out[ESC_BLOCK_PAGE_MAX];
};
typedef struct esc_block_s esc_block_t;

struct esc_crypt_s {
	u32		title_handle;
	u32		content_id;
	u32		content_ind;
	u32		frame_cnt;
	u32		size;
	s32		error;
	u32		frames_in[ESC_DECRYPT_FRAME_MAX];
	u32		frames_out[ESC_DECRYPT_FRAME_MAX];
};
typedef struct esc_crypt_s esc_crypt_t;

struct esc_title_rec_s {
	u64		title_id;
	u32		cookie;
	u32		title_handle;
	u32		state;
#define ESC_TTLST_GAME	0x00000001
#define ESC_TTLST_DATA	0x00000002
#define ESC_TTLST_GSS	0x00000004
#define ESC_TTLST_DIRTY	0x00000010
	u32		ticket_handle;
};
typedef struct esc_title_rec_s esc_title_rec_t;

struct esc_title_query_s {
	esc_title_rec_t	recs[ESC_TITLE_MAX];
	u64			title_id;
	u32			cookie;
	u32			mask;
	u32			rec_cnt;	// return count
	u32			error;
};
typedef struct esc_title_query_s esc_title_query_t;

struct esc_content_rec_s {
	u32		title_handle;
	u32		content_id;
	u32		content_handle;
	u32		content_state;
#define ESC_CFMST_RW		0x00000001
#define ESC_CFMST_DIRTY	0x00000002
};
typedef struct esc_content_rec_s esc_content_rec_t;

struct esc_content_query_s {
	esc_content_rec_t	recs[ESC_CONTENT_MAX];
	u32			title_handle;
	u32			rec_cnt;
	u32			error;
	u32			pad;
};
typedef struct esc_content_query_s esc_content_query_t;

struct esc_ticket_rec_s {
	u64		title_id;
	u64		uid;
	u32		ticket_handle;
	u32		cookie;
	u32		ref_cnt;
	u32		state;
#define ESC_TKTST_USER	0x00000001
};
typedef struct esc_ticket_rec_s esc_ticket_rec_t;

struct esc_ticket_query_s {
	esc_ticket_rec_t	recs[ESC_TICKET_MAX];
	u64			title_id;
	u32			cookie;
	u32			mask;
	u32			rec_cnt;
	u32			error;
};
typedef struct esc_ticket_query_s esc_ticket_query_t;

// Mem counts are in 4KiB pages
struct esc_mem_usage_s {
	u32			mem_total;
	u32			mem_free;
	u32			error;
};
typedef struct esc_mem_usage_s esc_mem_usage_t;

#define ESC_RS_SEED_LEN	4
struct esc_randseed_s {
	u64			seeds[ESC_RS_SEED_LEN];
	s32			error;
	u32			pad;
};
typedef struct esc_randseed_s esc_randseed_t;

#endif // __ESC_DATA_H__

// The following lines are needed by Emacs to maintain the format the file started in.
// Local Variables:
// c-basic-offset: 8
// tab-width: 8
// indent-tabs-mode: t
// End:
