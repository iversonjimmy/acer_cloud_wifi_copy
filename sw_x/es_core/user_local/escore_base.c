
// vim: ts=8:sw=8:noexpandtab

#include "escore.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _MSC_VER
#ifndef bool
#define bool int
#endif
#ifndef true
#define true 1
#endif
#ifndef false
#define false 0
#endif
#else
#include <stdbool.h>
#endif
#ifdef ESCORE_IS_REENTRANT
#include "vpl_th.h"
#endif // ESCORE_IS_REENTRANT

// This module has no middle man transport layer. It talks directly to
// the ES core.

#include <esc_funcs.h>
#include <esc_data.h>

#define _PAGE_SIZE	4096
#define _PAGE_CNT(x)	(((x) + _PAGE_SIZE - 1) / _PAGE_SIZE)

static bool _esc_is_init = false;
#ifdef ESCORE_IS_REENTRANT
static VPLMutex_t _esc_mutex;
#define ESC_INIT_LOCK	if (!_esc_is_init) { error = ESC_ERR_NOT_INITIALIZED; goto end; } \
			else { VPLMutex_Lock(&_esc_mutex);}
#define ESC_UNLOCK    	if (_esc_is_init) { VPLMutex_Unlock(&_esc_mutex); }
#else // !ESCORE_IS_REENTRANT
#define ESC_INIT_LOCK 	if (!_esc_is_init) { error = ESC_ERR_NOT_INITIALIZED; goto end; }
#define ESC_UNLOCK	{}
#endif // !ESCORE_IS_REENTRANT

static int
_buffer_split(u32 *frames, u32 frame_max, u32 *ret_len, u32 len, const u8 *buf)
{
	u32 num_frames;
	u32 i;

	num_frames = _PAGE_CNT(len);
	if ( num_frames > frame_max ) {
		return -1;
	}

	*ret_len = len;
	for( i = 0 ; i < num_frames ; i++ ) {
		frames[i] = (u32)&buf[_PAGE_SIZE * i];
	}
	
	return 0;
}

static inline int
_ghv_ticket_import(u64 tid,
		   u64 uid,
		   u32 cookie,
		   const void *tktBuf,
		   u32 tktLen,
		   const void *certBufs[],
		   u32 certLens[],
		   u32 certCnt,
		   u32 chalHandle,
		   u32 *ticketHandle)
{
	esc_ticket_t tkt_req;
	int error = -1;
	u32 i;

	ESC_INIT_LOCK;

	if ( certCnt > ESC_CERT_FRAME_MAX ) {
		goto end;
	}

	memset(&tkt_req, 0, sizeof(tkt_req));

	tkt_req.title_id = tid;
	tkt_req.user_id = uid;
	tkt_req.cookie = cookie;
	tkt_req.chal_handle = chalHandle;
	tkt_req.is_title = (tid != 0);
	tkt_req.is_user = (uid != 0);

	if ( _buffer_split(tkt_req.ticket_frames, ESC_TICKET_FRAME_MAX,
			   &tkt_req.ticket_size, tktLen, (u8 *)tktBuf) ) {
		goto end;
	}
	for( i = 0 ; i < certCnt ; i++ ) {
		tkt_req.cert_frames[i] = (u32)certBufs[i];
	}
	tkt_req.cert_count = certCnt;

	esc_ticket_import(&tkt_req);

	if ((error = tkt_req.error))
		goto end;

	*ticketHandle = tkt_req.handle;
end:
	ESC_UNLOCK;
	return error;
}

int
ESCore_ImportTicketTitle(u64 tid,
		      u32 cookie,
		      const void *tktBuf,
		      u32 tktLen,
		      const void *certBufs[],
		      u32 certLens[],
		      u32 certCnt,
		      u32 *ticketHandle)
{
	return _ghv_ticket_import(tid, 0, cookie, tktBuf, tktLen, certBufs,
				  certLens, certCnt, 0, ticketHandle);
}

#if defined(_GSS_SUPPORT)
int
ESCore_ImportTicketUser(u64 uid,
		     u32 cookie,
		     const void *tktBuf,
		     u32 tktLen,
		     const void *certBufs[],
		     u32 certLens[],
		     u32 certCnt,
		     u32 *ticketHandle)
{
	return _ghv_ticket_import(0, uid, cookie, tktBuf, tktLen, certBufs,
				  certLens, certCnt, 0, ticketHandle);
}
#endif // _GSS_SUPPORT

int
ESCore_ImportTicketUseOnce(u64 tid, u32 cookie, const void *tktBuf, u32 tktLen,
			const void *certBufs[], u32 certLens[], u32 certCnt,
			u32 chalHandle, u32 *ticketHandle)
{
	return _ghv_ticket_import(tid, 0, cookie, tktBuf, tktLen, certBufs,
				  certLens, certCnt, chalHandle, ticketHandle);
}

int
ESCore_ImportContentById(u32 titleHandle,
		 u32 cid,
		 const void *contentBuf,
		 u32 contentLen,
		 u32 *contentHandle)
{
	esc_content_t *content_req = NULL;
	int num_frames;
	int error = -1;

	ESC_INIT_LOCK;

	num_frames = _PAGE_CNT(contentLen);
	content_req = (esc_content_t *)calloc(1, sizeof(*content_req) + 
		num_frames * sizeof(u32));

	content_req->title_handle = titleHandle;
	content_req->content_id = cid;

	if ( _buffer_split(content_req->frames, num_frames, &content_req->size,
			   contentLen, (u8 *)contentBuf) ) {
		goto end;
	}

	esc_content_import(content_req);

	if ((error = content_req->error))
		goto end;

	*contentHandle = content_req->content_handle;

end:
	ESC_UNLOCK;
	if ( content_req ) {
		free(content_req);
	}
	return error;
}

int
ESCore_ImportContentByIdx(u32 titleHandle,
		 u32 contentIndex,
		 const void *contentBuf,
		 u32 contentLen,
		 u32 *contentHandle)
{
	esc_content_ind_t *content_req = NULL;
	int num_frames;
	int error = -1;

	ESC_INIT_LOCK;

	num_frames = _PAGE_CNT(contentLen);
	content_req = (esc_content_ind_t *)calloc(1, sizeof(*content_req) + 
		num_frames * sizeof(u32));

	content_req->title_handle = titleHandle;
	content_req->content_ind = contentIndex;

	if ( _buffer_split(content_req->frames, num_frames, &content_req->size,
			   contentLen, (u8 *)contentBuf) ) {
		goto end;
	}

	esc_content_import_ind(content_req);

	if ((error = content_req->error))
		goto end;

	*contentHandle = content_req->content_handle;

end:
	ESC_UNLOCK;
	if ( content_req ) {
		free(content_req);
	}
	return error;
}

int
ESCore_ImportTitle(u64 tid, u32 cookie,
		u32 tktSigHandle,
		u32 tktDataHandle,
		const void *tmdBuf,
		u32 tmdLen,
		u32 *titleHandle)
{
	esc_title_t ttl_req;
	int error = -1;

	ESC_INIT_LOCK;

	memset(&ttl_req, 0, sizeof(ttl_req));

	ttl_req.title_id = tid;
	ttl_req.cookie = cookie;
	ttl_req.ticket_sig_handle = tktSigHandle;
	ttl_req.ticket_data_handle = tktDataHandle;
	if ( _buffer_split(ttl_req.tmd_frames, ESC_TMD_FRAME_MAX,
			   &ttl_req.tmd_size, tmdLen, tmdBuf) ) {
		goto end;
	}

	esc_title_import(&ttl_req);

	if ( (error = ttl_req.error) ) {
		goto end;
	}

	*titleHandle = ttl_req.title_handle;

end:
	ESC_UNLOCK;
	return error;
}

int
ESCore_ReleaseContent(u32 contentHandle)
{
	esc_resource_rel content_req;
	int error = -1;

	ESC_INIT_LOCK;

	memset(&content_req, 0, sizeof(content_req));
	content_req.handle = contentHandle;

	esc_content_release(&content_req);

	error = content_req.error;

end:
	ESC_UNLOCK;
	return error;
}

int
ESCore_ReleaseTitle(u32 titleHandle)
{
	esc_resource_rel ttl_req;
	int error = -1;

	ESC_INIT_LOCK;

	memset(&ttl_req, 0, sizeof(ttl_req));
	ttl_req.handle = titleHandle;

	esc_title_release(&ttl_req);

	error = ttl_req.error;

end:
	ESC_UNLOCK;
	return error;
}

int
ESCore_ReleaseTicket(u32 ticketHandle)
{
	esc_resource_rel req;
	int error = -1;

	ESC_INIT_LOCK;

	memset(&req, 0, sizeof(req));
	req.handle = ticketHandle;

	esc_ticket_release(&req);

	error = req.error;

end:
	ESC_UNLOCK;
	return error;
}

int
ESCore_GetDeviceId(u32 *devId)
{
	esc_device_id_t dev_req;
	int error = -1;

	ESC_INIT_LOCK;

	memset(&dev_req, 0, sizeof(dev_req));
	esc_device_idcert_get(&dev_req);

	if ( (error = dev_req.error) ) {
		goto end;
	}

	*devId = dev_req.device_id;

end:
	ESC_UNLOCK;
	return error;
}

int
ESCore_GetDeviceType(u32 *devType)
{
	esc_device_id_t dev_req;
	int error = -1;

	ESC_INIT_LOCK;

	memset(&dev_req, 0, sizeof(dev_req));
	esc_device_idcert_get(&dev_req);

	if ( (error = dev_req.error) ) {
		goto end;
	}

	*devType = dev_req.device_type;

end:
	ESC_UNLOCK;
	return error;
}

int
ESCore_GetDeviceGuid(u64* devGuid)
{
    u32 type, id;
    int error = 0;

    error = ESCore_GetDeviceId(&id);
    if (error) {
        goto end;
    }
    error = ESCore_GetDeviceType(&type);
    if (error) {
        goto end;
    }
    *devGuid = (((u64)type) << 32) | ((u64)id);

end:
    return error;
}


int
ESCore_GetDeviceCert(ESCore_EccSignedCert* devCert)
{
	esc_device_id_t dev_req;
	int error = -1;

	ESC_INIT_LOCK;

	memset(&dev_req, 0, sizeof(dev_req));
	esc_device_idcert_get(&dev_req);

	if ( (error = dev_req.error) ) {
		goto end;
	}

	memcpy(devCert, dev_req.device_cert,
		sizeof(dev_req.device_cert));

end:
	ESC_UNLOCK;
	return error;
}

int
ESCore_QueryTitleMax(u32 *titleMax)
{
	*titleMax = ESC_TITLE_MAX;
	return 0;
}

int
ESCore_QueryTitle(u64 tid, u32 cookie, u32 mask, ESCore_Title_t *titles, u32 *titleCnt)
{
	esc_title_query_t ttl_req;
	int error = -1;
	int i;

	ESC_INIT_LOCK;

	if ( cookie == ESCORE_MATCH_ANY ) {
		cookie = ESC_MATCH_ANY;
	}

	memset(&ttl_req, 0, sizeof(ttl_req));
	ttl_req.title_id = tid;
	ttl_req.cookie = cookie;
	ttl_req.mask = mask;
	esc_title_query(&ttl_req);

	if ( (error = ttl_req.error) ) {
		goto end;
	}

	if ( *titleCnt > ttl_req.rec_cnt ) {
		*titleCnt = ttl_req.rec_cnt;
	}
	for( i = 0 ; i < *titleCnt ; i++ ) {
		titles[i].id = ttl_req.recs[i].title_id;
		titles[i].cookie = ttl_req.recs[i].cookie;
		titles[i].handle = ttl_req.recs[i].title_handle;
		titles[i].state = ttl_req.recs[i].state;
		titles[i].ticketHandle = ttl_req.recs[i].ticket_handle;
	}

end:
	ESC_UNLOCK;
	return error;
}

int
ESCore_QueryContentMax(u32 *contentMax)
{
	*contentMax = ESC_CONTENT_MAX;
	return 0;
}

int
ESCore_QueryContent(u32 titleHandle, ESCore_Content_t *contents, u32 *contentCnt)
{
	esc_content_query_t req;
	int error = -1;
	int i;

	ESC_INIT_LOCK;

	if ( titleHandle == ESCORE_MATCH_ANY ) {
		titleHandle = ESC_MATCH_ANY;
	}

	memset(&req, 0, sizeof(req));
	req.title_handle = titleHandle;
	esc_content_query(&req);

	if ( (error = req.error) ) {
		goto end;
	}

	if ( *contentCnt > req.rec_cnt ) {
		*contentCnt = req.rec_cnt;
	}
	for( i = 0 ; i < *contentCnt ; i++ ) {
		contents[i].titleHandle = req.recs[i].title_handle;
		contents[i].id = req.recs[i].content_id;
		contents[i].handle = req.recs[i].content_handle;
		contents[i].state = req.recs[i].content_state;
	}

end:
	ESC_UNLOCK;
	return error;
}

int
ESCore_QueryTicketMax(u32 *ticketMax)
{
	*ticketMax = ESC_TICKET_MAX;
	return 0;
}

int
ESCore_QueryTicket(u64 titleId, u32 cookie, u32 mask, ESCore_Ticket_t *tickets,
	        u32 *ticketCnt)
{
	esc_ticket_query_t req;
	int error = -1;
	int i;

	ESC_INIT_LOCK;

	if ( cookie == ESCORE_MATCH_ANY ) {
		cookie = ESC_MATCH_ANY;
	}

	memset(&req, 0, sizeof(req));
	req.cookie = cookie;
	req.mask = mask;
	req.title_id = titleId;
	esc_ticket_query(&req);

	if ( (error = req.error) ) {
		goto end;
	}

	if ( *ticketCnt > req.rec_cnt ) {
		*ticketCnt = req.rec_cnt;
	}
	for( i = 0 ; i < *ticketCnt ; i++ ) {
		tickets[i].titleId = req.recs[i].title_id;
		tickets[i].userId = req.recs[i].uid;
		tickets[i].handle = req.recs[i].ticket_handle;
		tickets[i].cookie = req.recs[i].cookie;
		tickets[i].refCnt = req.recs[i].ref_cnt;
		tickets[i].state = req.recs[i].state;
	}

end:
	ESC_UNLOCK;
	return error;
}

static int
_ghv_crypt_block(u32 contentHandle, u32 blockNum, u32 pageCnt,
		 void *pagesIn[ESCORE_BLOCK_PAGE_MAX],
		 void *pagesOut[ESCORE_BLOCK_PAGE_MAX],
		 bool isDecrypt)
{
	esc_block_t req;
	int error = -1;
	int i;

	ESC_INIT_LOCK;

	memset(&req, 0, sizeof(req));
	req.content_handle = contentHandle;
	req.block_number = blockNum;
	req.page_cnt = pageCnt;
	for( i = 0 ; i < pageCnt ; i++ ) {
		req.frames_in[i] = (u32)pagesIn[i];
		req.frames_out[i] = (u32)pagesOut[i];
	}
	(isDecrypt) ? esc_block_decrypt(&req) : esc_block_encrypt(&req);

	error = req.error;

end:
	ESC_UNLOCK;
	return error;
}

int
ESCore_DecryptBlock(u32 contentHandle, u32 blockNum, u32 pageCnt,
		 void *pagesIn[ESCORE_BLOCK_PAGE_MAX],
		 void *pagesOut[ESCORE_BLOCK_PAGE_MAX])
{
	return _ghv_crypt_block(contentHandle, blockNum, pageCnt,
				pagesIn, pagesOut, true);
}

static int
_escore_decrypt_content(u32 titleHandle, u32 cid, u32 cidx, void *bufIn,
			void *bufOut, u32 size)
{
	esc_crypt_t req;
	int error = -1;

	ESC_INIT_LOCK;

	memset(&req, 0, sizeof(req));
	req.title_handle = titleHandle;
	req.content_id = cid;
	req.content_ind = cidx;
	req.frame_cnt = _PAGE_CNT(size);
	if ( _buffer_split(req.frames_in, ESC_DECRYPT_FRAME_MAX,
			   &req.size, size, bufIn) ) {
		goto end;
	}
	if ( _buffer_split(req.frames_out, ESC_DECRYPT_FRAME_MAX,
			   &req.size, size, bufOut) ) {
		goto end;
	}

	esc_decrypt(&req);

	error = req.error;

end:
	ESC_UNLOCK;
	return error;
}

int
ESCore_DecryptContentById(u32 titleHandle, u32 contentId, void *bufIn,
			  void *bufOut, u32 size)
{
	return _escore_decrypt_content(titleHandle, contentId, ESC_INVALID_IND,
				       bufIn, bufOut, size);
}

int
ESCore_DecryptContentByIdx(u32 titleHandle, u32 contentIdx, void *bufIn,
			   void *bufOut, u32 size)
{
	return _escore_decrypt_content(titleHandle, ESC_INVALID_ID, contentIdx,
				       bufIn, bufOut, size);
}


#if defined(_GSS_SUPPORT)
int
ESCore_EncryptBlock(u32 contentHandle, u32 blockNum, u32 pageCnt,
		 void *pagesIn[ESCORE_BLOCK_PAGE_MAX],
		 void *pagesOut[ESCORE_BLOCK_PAGE_MAX])
{
	return _ghv_crypt_block(contentHandle, blockNum, pageCnt,
				pagesIn, pagesOut, false);
}

int
ESCore_CreateGss(u64 userId, u32 cookie, u32 ticketSignHandle,
	      u32 ticketDataHandle, u32 titleHandle, u32 contentId,
	      u32 *retTitleHandlep, u32 *retContentHandlep)
{
	esc_gss_create_t req;
	int error = -1;

	ESC_INIT_LOCK;

	memset(&req, 0, sizeof(req));
	req.user_id = userId;
	req.cookie = cookie;
	req.ticket_sign_handle = ticketSignHandle;
	req.ticket_data_handle = ticketDataHandle;
	req.title_handle = titleHandle;
	req.content_id = contentId;
	esc_gss_create(&req);

	if ( (error = req.error) ) {
		goto end;
	}

	*retTitleHandlep = req.return_title_handle;
	*retContentHandlep = req.return_content_handle;

end:
	ESC_UNLOCK;
	return error;
}

int
ESCore_ExportTitle(u32 titleHandle, u32 ticketHandle, void *tmdBuf, u32 tmdLen)
{
	esc_title_export_t req;
	int error = -1;

	memset(&req, 0, sizeof(req));
	req.title_handle = titleHandle;
	req.ticket_handle = ticketHandle;

	ESC_INIT_LOCK;

	if ( _buffer_split(req.tmd_frames, ESC_TMD_FRAME_MAX,
			   &req.tmd_size, tmdLen, tmdBuf) ) {
		goto end;
	}
	esc_title_export(&req);

	error = req.error;

end:
	ESC_UNLOCK;
	return error;
}

int
ESCore_ExportContent(u32 contentHandle, void *contentBuf, u32 contentLen)
{
	esc_content_export_t *req;
	int num_frames;
	int error = -1;

	ESC_INIT_LOCK;

	num_frames = _PAGE_CNT(contentLen);
	req = (esc_content_export_t *)calloc(1, sizeof(req) + 
		num_frames * sizeof(u32));

	req->content_handle = contentHandle;
	if ( _buffer_split(req->frames, ESC_TMD_FRAME_MAX,
			   &req->size, contentLen, contentBuf) ) {
		goto end;
	}
	esc_content_export(req);

	error = req->error;

end:
	ESC_UNLOCK;
	return error;
}

int
ESCore_GetTitleSize(u32 titleHandle, u32 *tmdLen)
{
	esc_title_export_t req;
	int error = -1;

	ESC_INIT_LOCK;

	memset(&req, 0, sizeof(req));
	req.title_handle = titleHandle;
	esc_title_export(&req);

	if ( (error = req.error) ) {
		goto end;
	}

	*tmdLen = req.tmd_size;

end:
	ESC_UNLOCK;
	return error;
}

int
ESCore_GetContentSize(u32 contentHandle, u32 *contentLen)
{
	esc_content_export_t req;
	int error = -1;

	ESC_INIT_LOCK;

	memset(&req, 0, sizeof(req));
	req.content_handle = contentHandle;
	esc_content_export(&req);

	if ( (error = req.error) ) {
		goto end;
	}

	*contentLen = req.size;

end:
	ESC_UNLOCK;
	return error;
}
#endif // _GSS_SUPPORT

int
ESCore_Init(void *credDefault, u32 defaultLen)
{
    esc_init_t req;
    int rv;

    // only perform init once.
    if ( _esc_is_init ) {
        return ESCORE_ERR_ALREADY_INIT;
    }

#ifdef ESCORE_IS_REENTRANT
    if ( VPLMutex_Init(&_esc_mutex) ) {
        return ESCORE_ERR_MUTEX;
    }
#endif // ESCORE_IS_REENTRANT

    {
#ifdef _MSC_VER
        u8 *credDefault_localCopy = malloc(defaultLen * sizeof(u8));
#else
        u8 credDefault_localCopy[defaultLen];
#endif
        memcpy(credDefault_localCopy, credDefault, defaultLen);

        memset(&req, 0, sizeof(req));
        req.cred_default = credDefault_localCopy;
        req.default_len = defaultLen;
        rv = esc_init(&req);

#ifdef _MSC_VER
        free(credDefault_localCopy);
        req.cred_default = credDefault_localCopy = NULL;
#endif

        if (rv < 0 ) {
#ifdef ESCORE_IS_REENTRANT
            VPLMutex_Destroy(&_esc_mutex);
#endif // ESCORE_IS_REENTRANT
            if (req.error == ESC_ERR_ALREADY_INIT)
                req.error = ESCORE_ERR_ALREADY_INIT;
            return req.error;
        }
    }

    // We also need to initialize the random number generator
    // for the core, but when we're doing this in the user-level
    // model, well... Seems like this should be done in a different
    // way. Need to think about this one. XXX

    _esc_is_init = true;

    return 0;
}

int
ESCore_LoadCredentials(void *credSecret, u32 secretLen, void *credClear, u32 clearLen, u32 mode)
{
    esc_cred_t req;
    int error = 1;

    ESC_INIT_LOCK;

    {
        int rv;

#ifdef _MSC_VER
        u8 *credSecret_localCopy = malloc(secretLen * sizeof(u8));
#else
        u8 credSecret_localCopy[secretLen];
#endif
        memcpy(credSecret_localCopy, credSecret, secretLen);

        memset(&req, 0, sizeof(req));
        req.credSecret = credSecret_localCopy;
        req.secretLen = secretLen;
        req.credClear = credClear;
        req.clearLen = clearLen;
        req.mode = mode;
        rv = esc_load_credentials(&req);

#ifdef _MSC_VER
        free(credSecret_localCopy);
        req.credSecret = credSecret_localCopy = NULL;
#endif

        if (rv < 0 ) {
            error = req.error;
            goto end;
        }
    }

    error = 0;

end:
    ESC_UNLOCK;
    return error;
}

// The following lines are needed by Emacs to maintain the format the file started in.
// Local Variables:
// c-basic-offset: 8
// tab-width: 8
// indent-tabs-mode: t
// End:
