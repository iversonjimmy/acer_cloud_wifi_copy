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

#ifndef __ESCSVC_H__
#define __ESCSVC_H__

#define ESCSVC_RING_SIZE 256

#define ESCSVC_DEVICE_TYPE_VP		6

#define ESCSVC_ES_RANDSEED		1

#define ESCSVC_ES_INIT			8

// Ticket services
#define ESCSVC_ES_TICKET_IMPORT		10
#define ESCSVC_ES_TICKET_RELEASE	11
#define ESCSVC_ES_TICKET_QUERY		12

// Challenge Services
#define ESCSVC_ES_CHAL_IMPORT		15
#define ESCSVC_ES_CHAL_RELEASE		16
#define ESCSVC_ES_CHAL_QUERY		17
#define ESCSVC_ES_CHAL_RESULT		18

// Title services
#define ESCSVC_ES_TITLE_IMPORT		20
#define ESCSVC_ES_TITLE_EXPORT		21
#define ESCSVC_ES_TITLE_RELEASE		22
#define ESCSVC_ES_TITLE_QUERY		23

// Content services
#define ESCSVC_ES_CONTENT_IMPORT_ID	30
#define ESCSVC_ES_CONTENT_EXPORT	31
#define ESCSVC_ES_CONTENT_RELEASE	32
#define ESCSVC_ES_CONTENT_QUERY		33
#define ESCSVC_ES_CONTENT_IMPORT_IDX	34

// Block services
#define ESCSVC_ES_BLOCK_DECRYPT		40
#define ESCSVC_ES_BLOCK_ENCRYPT		41

// Device services
#define ESCSVC_ES_DEVICE_ID_GET		50

// GSS services
#define ESCSVC_ES_GSS_CREATE		55

// GHV services
#define ESCSVC_ES_MEM_USAGE		60
#define ESCSVC_ES_INVENT		61

// The following should be calculated
#define	ESCSVC_PAGES_PER_CRYPTO_BLOCK	8
#define ESCSVC_TITLE_KEYLEN		16
#define ESCSVC_BLOCK_PAGE_MAX		8

// XXX - needs to match ES_DEVICE_CERT_SIZE in estypes.h
// But that file can't safely be pulled in by ghvcmod, bvd, etc.
#define ESCSVC_DEV_CERT_SIZE		384

#define ESCSVC_ERR_OK			0
#define ESCSVC_ERR_UNSPECIFIED		-1
#define ESCSVC_ERR_PARMS		-2
#define ESCSVC_ERR_CERTS_VERIFY		-3
#define ESCSVC_ERR_TKT_VERIFY		-4
#define ESCSVC_ERR_EXISTS		-5
#define ESCSVC_ERR_NOMEM		-6
#define ESCSVC_ERR_TITLE_MATCH		-7
#define ESCSVC_ERR_TMD_VERIFY		-8
#define ESCSVC_ERR_CONTENT_MATCH	-9
#define ESCSVC_ERR_BLK_HASH		-10
#define ESCSVC_ERR_CONTENT_HASH		-11
#define ESCSVC_ERR_CONTENT_INVALID	-12
#define ESCSVC_ERR_TKT_RIGHTS		-13
#define ESCSVC_ERR_NO_RIGHT		-14
#define ESCSVC_ERR_TITLE_GVM		-15
#define ESCSVC_ERR_HANDLE_INVALID	-16
#define ESCSVC_ERR_HANDLE_STALE		-17
#define ESCSVC_ERR_BUSY			-18
#define ESCSVC_ERR_GSS_SIGN		-19
#define ESCSVC_ERR_USER_ID		-20
#define ESCSVC_ERR_READONLY		-21
#define ESCSVC_ERR_SIZE_MISMATCH	-22
#define ESCSVC_ERR_BUF_ACCESS		-23
#define ESCSVC_ERR_NO_CONTENT		-24
#define ESCSVC_ERR_TKT_TYPE		-25
#define ESCSVC_ERR_PAGE_PROT_ON		-26
#define ESCSVC_ERR_PAGE_PROT_OFF	-27
#define ESCSVC_ERR_TKT_KEYID		-28
#define ESCSVC_ERR_TKT_DEVICEID		-29

//#define ESCSVC_ERR_OK			0	
//#define ESCSVC_ERR_UNSPECIFIED		-101
#define ESCSVC_ERR_HC_UNKNOWN		-102
#define ESCSVC_ERR_HC_PARMS		-103
#define ESCSVC_ERR_RING_FULL		-104
#define ESCSVC_ERR_NO_HYPERVISOR	-105
//#define ESCSVC_ERR_NOMEM		-106

#define ESCSVC_MATCH_ANY		0xffffffff

struct escsvc_completion;

typedef void (escsvc_callback_t)(struct escsvc_completion *comp,
				    int err);
struct escsvc_completion {
    escsvc_callback_t *callback;
};

static inline void
escsvc_completion_init(struct escsvc_completion *comp,
              escsvc_callback_t *callback)
{
	    comp->callback = callback;
}

int escsvc_init(void);
void escsvc_cleanup(void);

void escsvc_bvd_cfm_release(u32 cfm);

// Exported to other drivers
int escsvc_request_sync(int svc, void *buffer, u32 length);
int escsvc_request_async(int svc, void *buffer, u32 length,
                         struct escsvc_completion *comp);
int escsvc_bvd_register(u32 major, u32 minor_max);
int escsvc_bvd_unregister(void);
int escsvc_bvd_dev_map(u32 minor, u32 cfm);

#endif // __ESVSVC_H__

// The following lines are needed by Emacs to maintain the format the file started in.
// Local Variables:
// c-basic-offset: 8
// tab-width: 8
// indent-tabs-mode: t
// End:
