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

#ifndef __ESC_IOCTL_H__
#define __ESC_IOCTL_H__

#ifdef __KERNEL__
#include <linux/time.h>
#include <linux/list.h>
#include <linux/uaccess.h>
#else
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <linux/types.h>
#endif
#include "esc_data.h"

#define ESC_IOC_MAGIC   'V'

#define ESC_IOC_INIT		_IOWR(ESC_IOC_MAGIC, 8, struct esc_init)

#define ESC_IOC_TICKET_IMPORT  	_IOWR(ESC_IOC_MAGIC, 10, struct esc_ticket_import)
#define ESC_IOC_TICKET_RELEASE  _IOWR(ESC_IOC_MAGIC, 11, struct esc_ticket_release)
#define ESC_IOC_TICKET_QUERY  	_IOWR(ESC_IOC_MAGIC, 12, struct esc_ticket_query)
#define ESC_IOC_TITLE_IMPORT   	_IOWR(ESC_IOC_MAGIC, 15, struct esc_title_import)
#define ESC_IOC_TITLE_RELEASE  	_IOWR(ESC_IOC_MAGIC, 16, struct esc_title_release)
#define ESC_IOC_TITLE_QUERY  	_IOWR(ESC_IOC_MAGIC, 17, struct esc_title_query)
#define ESC_IOC_TITLE_EXPORT  	_IOWR(ESC_IOC_MAGIC, 18, struct esc_title_export)
#define ESC_IOC_CONTENT_IMPORT_ID _IOWR(ESC_IOC_MAGIC, 20, struct esc_content_import_id)
#define ESC_IOC_CONTENT_IMPORT_IDX _IOWR(ESC_IOC_MAGIC, 24, struct esc_content_import_idx)
#define ESC_IOC_CONTENT_RELEASE	_IOWR(ESC_IOC_MAGIC, 21, struct esc_content_release)
#define ESC_IOC_CONTENT_QUERY	_IOWR(ESC_IOC_MAGIC, 22, struct esc_content_query)
#define ESC_IOC_CONTENT_EXPORT	_IOWR(ESC_IOC_MAGIC, 23, struct esc_content_export)
#define ESC_IOC_DEVICE_INFO	_IOWR(ESC_IOC_MAGIC, 25, struct esc_device_info)
#define ESC_IOC_BLOCK_DECRYPT	_IOWR(ESC_IOC_MAGIC, 30, struct esc_block_crypt)
#define ESC_IOC_BLOCK_ENCRYPT	_IOWR(ESC_IOC_MAGIC, 31, struct esc_block_crypt)
#define ESC_IOC_GSS_CREATE	_IOWR(ESC_IOC_MAGIC, 35, struct esc_gss_create)

struct esc_init {
	void	*credSecret;
	u32	secretLen;
	void	*credClear;
	u32	clearLen;
	u32	mode;
	s32	return_error;
};

struct esc_ticket_import {
	u64	title_id;
	u64	user_id;
	void   *ticket;
	u32	ticket_len;
	u32	cert_cnt;
	void   *certs[ESC_CERT_FRAME_MAX];
	u32     cert_lens[ESC_CERT_FRAME_MAX];
	u32     cookie;
	u32	chal_handle;
	u32     return_handle;
	s32	return_error;
};

struct esc_ticket_release {
	u32	handle;
	s32	return_error;
};

struct esc_ticket_rec {
	u64	title_id;
	u64	uid;
	u32	handle;
	u32     cookie;
	u32	ref_cnt;
	u32	state;
};

struct esc_ticket_query {
	u64			title_id;
	u32			cookie;
	u32			mask;
	struct esc_ticket_rec	return_recs[ESC_TICKET_MAX];
	u32			return_rec_cnt;
	u32			return_error;
};

struct esc_title_import {
	u64	title_id;
	u32	ticket_sig_handle;
	u32	ticket_data_handle;
	void   *tmd;
	u32     tmd_len;
	u32	cookie;
	u32	return_handle;
	s32	return_error;
};

struct esc_title_release {
	u32	handle;
	s32	return_error;
};

struct esc_title_export {
	u32	title_handle;
	u32	ticket_handle;
	u32	tmd_len;
	void   *tmd;
	s32	return_error;
};

struct esc_content_import_id {
	u32	title_handle;
	u32	content_id;
	void   *cfm;
	u32	cfm_len;
	u32     return_handle;
	s32	return_error;
};

struct esc_content_import_idx {
	u32	title_handle;
	u32	content_index;
	void   *cfm;
	u32	cfm_len;
	u32     return_handle;
	s32	return_error;
};

struct esc_content_release {
	u32     handle;
	s32	return_error;
};

struct esc_content_export {
	u32	handle;
	u32	size;
	void   *data;
	s32	return_error;
};

struct esc_device_info {
	u32	id;
	u32	type;
	u8      cert[ESC_DEV_CERT_SIZE];
	s32     return_error;
};

struct esc_title_rec {
	u64	title_id;
	u32     cookie;
	u32     title_handle;
	u32	state;
	u32	ticket_handle;
};

struct esc_title_query {
	u64			title_id;
	u32			cookie;
	u32			mask;
	struct esc_title_rec	return_recs[ESC_TITLE_MAX];
	u32			return_rec_cnt;
	u32			return_error;
};

struct esc_content_rec {
	u32	title_handle;
	u32	content_id;
	u32	content_handle;
	u32	content_state;
};

struct esc_content_query {
	u32				title_handle;
	struct esc_content_rec		return_recs[ESC_CONTENT_MAX];
	u32				return_rec_cnt;
	u32				return_error;
};

struct esc_block_crypt {
	void	       *pages_in[ESC_BLOCK_PAGE_MAX];
	void	       *pages_out[ESC_BLOCK_PAGE_MAX];
	u32		content_handle;
	u32		block_number;
	u32		page_cnt;
	u32		return_error;
};

struct esc_gss_create {
	u64		user_id;
	u32		cookie;
	u32		ticket_sign_handle;
	u32		ticket_data_handle;
	u32		title_handle;
	u32		content_id;
	u64		return_title_handle;
	u32		return_content_handle;
	u32		return_error;
};

#endif // __ESC_IOCTL_H__

// The following lines are needed by Emacs to maintain the format the file started in.
// Local Variables:
// c-basic-offset: 8
// tab-width: 8
// indent-tabs-mode: t
// End:
