
#include <escore.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <sys/mman.h>


#include "esc_data.h"
#include "esc_ioctl.h"

#ifndef ESC_DEV_PATH
#define ESC_DEV_PATH "/dev/esc"
#endif

const char *dev = ESC_DEV_PATH;

static int
_esc_ioctl(int cmd, void *arg)
{
	int fd;
	int error;

	// open the device
	fd = open(dev, O_RDONLY);
	if (fd < 0) {
		return -1;
	}

	error = ioctl(fd, cmd, arg);

	close(fd);

	return error;
}

static inline int
_esc_ticket_import(u64 tid,
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
	struct esc_ticket_import tkt_req;
	int error = -1;
	u32 i;

	memset(&tkt_req, 0, sizeof(tkt_req));

	tkt_req.title_id = tid;
	tkt_req.user_id = uid;
	tkt_req.cookie = cookie;
	tkt_req.chal_handle = chalHandle;
	tkt_req.ticket = (void *)tktBuf;
	tkt_req.ticket_len = tktLen;

	for( i = 0 ; i < certCnt ; i++ ) {
		tkt_req.certs[i] = (void *)certBufs[i];
		tkt_req.cert_lens[i] = certLens[i];
	}
	tkt_req.cert_cnt = certCnt;

	error = _esc_ioctl(ESC_IOC_TICKET_IMPORT, &tkt_req);
	if (error < 0) {
		error = tkt_req.return_error;
		if (!error)
			error = ESCORE_ERR_ESC_DEVICE;
		goto end;
	}

	*ticketHandle = tkt_req.return_handle;
end:
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
	return _esc_ticket_import(tid, 0, cookie, tktBuf, tktLen, certBufs,
				  certLens, certCnt, 0, ticketHandle);
}

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
	return _esc_ticket_import(0, uid, cookie, tktBuf, tktLen, certBufs,
				  certLens, certCnt, 0, ticketHandle);
}

int
ESCore_ImportTicketUseOnce(u64 tid, u32 cookie, const void *tktBuf, u32 tktLen,
			const void *certBufs[], u32 certLens[], u32 certCnt,
			u32 chalHandle, u32 *ticketHandle)
{
	return _esc_ticket_import(tid, 0, cookie, tktBuf, tktLen, certBufs,
				  certLens, certCnt, chalHandle, ticketHandle);
}

int
ESCore_ImportContentById(u32 titleHandle,
		 u32 cid,
		 const void *cfmBuf,
		 u32 cfmLen,
		 u32 *cfmHandle)
{
	struct esc_content_import_id cfm_req;
	int error = -1;

	memset(&cfm_req, 0, sizeof(cfm_req));

	cfm_req.title_handle = titleHandle;
	cfm_req.content_id = cid;

	cfm_req.cfm = (void *)cfmBuf;
	cfm_req.cfm_len = cfmLen;

	error = _esc_ioctl(ESC_IOC_CONTENT_IMPORT_ID, &cfm_req);
	if (error < 0) {
		error = cfm_req.return_error;
		if (!error)
			error = ESCORE_ERR_ESC_DEVICE;
		goto end;
	}

	*cfmHandle = cfm_req.return_handle;

end:
	return error;
}

int
ESCore_ImportContentByIdx(u32 titleHandle, u32 contentIndex,
		      const void *contentBuf, u32 contentLen,
		      u32 *contentHandle)
{
	struct esc_content_import_idx cfm_req;
	int error = -1;

	memset(&cfm_req, 0, sizeof(cfm_req));

	cfm_req.title_handle = titleHandle;
	cfm_req.content_index = contentIndex;

	cfm_req.cfm = (void *)contentBuf;
	cfm_req.cfm_len = contentLen;

	error = _esc_ioctl(ESC_IOC_CONTENT_IMPORT_IDX, &cfm_req);
	if (error < 0) {
		error = cfm_req.return_error;
		if (!error)
			error = ESCORE_ERR_ESC_DEVICE;
		goto end;
	}

	*contentHandle = cfm_req.return_handle;

end:
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
	struct esc_title_import ttl_req;
	int error = -1;

	memset(&ttl_req, 0, sizeof(ttl_req));

	ttl_req.title_id = tid;
	ttl_req.cookie = cookie;
	ttl_req.ticket_sig_handle = tktSigHandle;
	ttl_req.ticket_data_handle = tktDataHandle;
	ttl_req.tmd = (void *)tmdBuf;
	ttl_req.tmd_len = tmdLen;

	error = _esc_ioctl(ESC_IOC_TITLE_IMPORT, &ttl_req);
	if (error < 0) {
		error = ttl_req.return_error;
		if (!error)
			error = ESCORE_ERR_ESC_DEVICE;
		goto end;
	}

	*titleHandle = ttl_req.return_handle;

end:
	return error;
}

// TODO: copied from ../user_local version; refactor common code
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
ESCore_ReleaseContent(u32 cfmHandle)
{
	struct esc_content_release cfm_req;
	int error = -1;

	memset(&cfm_req, 0, sizeof(cfm_req));

	cfm_req.handle = cfmHandle;

	error = _esc_ioctl(ESC_IOC_CONTENT_RELEASE, &cfm_req);
	if (error < 0) {
		error = cfm_req.return_error;
		if (!error)
			error = ESCORE_ERR_ESC_DEVICE;
		goto end;
	}

 end:
	return error;
}

int
ESCore_ReleaseTitle(u32 titleHandle)
{
	struct esc_title_release ttl_req;
	int error = -1;

	memset(&ttl_req, 0, sizeof(ttl_req));

	ttl_req.handle = titleHandle;

	error = _esc_ioctl(ESC_IOC_TITLE_RELEASE, &ttl_req);
	if (error < 0) {
		error = ttl_req.return_error;
		if (!error)
			error = ESCORE_ERR_ESC_DEVICE;
		goto end;
	}

 end:
	return error;
}

int
ESCore_ReleaseTicket(u32 ticketHandle)
{
	struct esc_ticket_release req;
	int error = -1;

	memset(&req, 0, sizeof(req));

	req.handle = ticketHandle;

	error = _esc_ioctl(ESC_IOC_TICKET_RELEASE, &req);
	if (error < 0) {
		error = req.return_error;
		if (!error)
			error = ESCORE_ERR_ESC_DEVICE;
		goto end;
	}

 end:
	return error;
}

int
ESCore_GetDeviceId(u32 *devId)
{
	struct esc_device_info dev_req;
	int error = -1;

	memset(&dev_req, 0, sizeof(dev_req));
	error = _esc_ioctl(ESC_IOC_DEVICE_INFO, &dev_req);
	if (error < 0) {
		error = dev_req.return_error;
		if (!error)
			error = ESCORE_ERR_ESC_DEVICE;
		goto end;
	}

	*devId = dev_req.id;

end:
	return error;
}

int
ESCore_GetDeviceType(u32 *devType)
{
	struct esc_device_info dev_req;
	int error = -1;

	memset(&dev_req, 0, sizeof(dev_req));
	error = _esc_ioctl(ESC_IOC_DEVICE_INFO, &dev_req);
	if (error < 0) {
		error = dev_req.return_error;
		if (!error)
			error = ESCORE_ERR_ESC_DEVICE;
		goto end;
	}

	*devType = dev_req.type;

end:
	return error;
}


int
ESCore_GetDeviceCert(ESCore_EccSignedCert *devCert)
{
	struct esc_device_info dev_req;
	int error = -1;

	memset(&dev_req, 0, sizeof(dev_req));
	error = _esc_ioctl(ESC_IOC_DEVICE_INFO, &dev_req);
	if (error < 0) {
		error = dev_req.return_error;
		if (!error)
			error = ESCORE_ERR_ESC_DEVICE;
		goto end;
	}

	memcpy(devCert, dev_req.cert, sizeof(dev_req.cert));

end:
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
	struct esc_title_query ttl_req;
	int error = -1;
	int i;

	if ( cookie == ESC_MATCH_ANY ) {
		cookie = ESC_MATCH_ANY;
	}

	memset(&ttl_req, 0, sizeof(ttl_req));
	ttl_req.title_id = tid;
	ttl_req.cookie = cookie;
	ttl_req.mask = mask;
	error = _esc_ioctl(ESC_IOC_TITLE_QUERY, &ttl_req);
	if (error < 0) {
		error = ttl_req.return_error;
		if (!error)
			error = ESCORE_ERR_ESC_DEVICE;
		goto end;
	}

	if ( *titleCnt > ttl_req.return_rec_cnt ) {
		*titleCnt = ttl_req.return_rec_cnt;
	}
	for( i = 0 ; i < *titleCnt ; i++ ) {
		titles[i].id = ttl_req.return_recs[i].title_id;
		titles[i].cookie = ttl_req.return_recs[i].cookie;
		titles[i].handle = ttl_req.return_recs[i].title_handle;
		titles[i].state = ttl_req.return_recs[i].state;
		titles[i].ticketHandle =
			ttl_req.return_recs[i].ticket_handle;
	}

end:
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
	struct esc_content_query req;
	int error = -1;
	int i;

	if ( titleHandle == ESC_MATCH_ANY ) {
		titleHandle = ESC_MATCH_ANY;
	}

	memset(&req, 0, sizeof(req));
	req.title_handle = titleHandle;
	error = _esc_ioctl(ESC_IOC_CONTENT_QUERY, &req);
	if (error < 0) {
		error = req.return_error;
		if (!error)
			error = ESCORE_ERR_ESC_DEVICE;
		goto end;
	}

	if ( *contentCnt > req.return_rec_cnt ) {
		*contentCnt = req.return_rec_cnt;
	}
	for( i = 0 ; i < *contentCnt ; i++ ) {
		contents[i].titleHandle = req.return_recs[i].title_handle;
		contents[i].id = req.return_recs[i].content_id;
		contents[i].handle = req.return_recs[i].content_handle;
		contents[i].state = req.return_recs[i].content_state;
	}

end:
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
	struct esc_ticket_query req;
	int error = -1;
	int i;

	if ( cookie == ESC_MATCH_ANY ) {
		cookie = ESC_MATCH_ANY;
	}

	memset(&req, 0, sizeof(req));
	req.cookie = cookie;
	req.mask = mask;
	req.title_id = titleId;
	error = _esc_ioctl(ESC_IOC_TICKET_QUERY, &req);
	if (error < 0) {
		error = req.return_error;
		if (!error)
			error = ESCORE_ERR_ESC_DEVICE;
		goto end;
	}

	if ( *ticketCnt > req.return_rec_cnt ) {
		*ticketCnt = req.return_rec_cnt;
	}
	for( i = 0 ; i < *ticketCnt ; i++ ) {
		tickets[i].titleId = req.return_recs[i].title_id;
		tickets[i].userId = req.return_recs[i].uid;
		tickets[i].handle = req.return_recs[i].handle;
		tickets[i].cookie = req.return_recs[i].cookie;
		tickets[i].refCnt = req.return_recs[i].ref_cnt;
		tickets[i].state = req.return_recs[i].state;
	}

end:
	return error;
}

static int
_esc_crypt_block(u32 contentHandle, u32 blockNum, u32 pageCnt,
		 void *pagesIn[ESC_BLOCK_PAGE_MAX],
		 void *pagesOut[ESC_BLOCK_PAGE_MAX],
		 bool isDecrypt)
{
	struct esc_block_crypt req;
	int error = -1;
	int i;

	memset(&req, 0, sizeof(req));
	req.content_handle = contentHandle;
	req.block_number = blockNum;
	req.page_cnt = pageCnt;
	for( i = 0 ; i < pageCnt ; i++ ) {
		req.pages_in[i] = pagesIn[i];
		req.pages_out[i] = pagesOut[i];
	}
	error = _esc_ioctl(isDecrypt ? ESC_IOC_BLOCK_DECRYPT
			   : ESC_IOC_BLOCK_ENCRYPT,
			   &req);
	if (error < 0) {
		error = req.return_error;
		if (!error)
			error = ESCORE_ERR_ESC_DEVICE;
		goto end;
	}

 end:
	return error;
}

int
ESCore_DecryptBlock(u32 contentHandle, u32 blockNum, u32 pageCnt,
		 void *pagesIn[ESC_BLOCK_PAGE_MAX],
		 void *pagesOut[ESC_BLOCK_PAGE_MAX])
{
	return _esc_crypt_block(contentHandle, blockNum, pageCnt,
				pagesIn, pagesOut, true);
}

int
ESCore_EncryptBlock(u32 contentHandle, u32 blockNum, u32 pageCnt,
		 void *pagesIn[ESC_BLOCK_PAGE_MAX],
		 void *pagesOut[ESC_BLOCK_PAGE_MAX])
{
	return _esc_crypt_block(contentHandle, blockNum, pageCnt,
				pagesIn, pagesOut, false);
}


int
ESCore_CreateGss(u64 userId, u32 cookie, u32 ticketSignHandle,
	      u32 ticketDataHandle, u32 titleHandle, u32 contentId,
	      u32 *retTitleHandlep, u32 *retContentHandlep)
{
	struct esc_gss_create req;
	int error;

	memset(&req, 0, sizeof(req));
	req.user_id = userId;
	req.cookie = cookie;
	req.ticket_sign_handle = ticketSignHandle;
	req.ticket_data_handle = ticketDataHandle;
	req.title_handle = titleHandle;
	req.content_id = contentId;
	error = _esc_ioctl(ESC_IOC_GSS_CREATE, &req);
	if (error < 0) {
		error = req.return_error;
		if (!error)
			error = ESCORE_ERR_ESC_DEVICE;
		goto end;
	}

	*retTitleHandlep = req.return_title_handle;
	*retContentHandlep = req.return_content_handle;

end:
	return error;
}

int
ESCore_ExportTitle(u32 titleHandle, u32 ticketHandle, void *tmdBuf, u32 tmdLen)
{
	struct esc_title_export req;
	int error;

	memset(&req, 0, sizeof(req));
	req.title_handle = titleHandle;
	req.ticket_handle = ticketHandle;
	req.tmd_len = tmdLen;
	req.tmd = tmdBuf;
	error = _esc_ioctl(ESC_IOC_TITLE_EXPORT, &req);
	if (error < 0) {
		error = req.return_error;
		if (!error) 
			error = ESCORE_ERR_ESC_DEVICE;
		goto end;
	}

 end:
	return error;
}

int
ESCore_ExportContent(u32 contentHandle, void *cfmBuf, u32 cfmLen)
{
	struct esc_content_export req;
	int error;

	memset(&req, 0, sizeof(req));
	req.handle = contentHandle;
	req.size = cfmLen;
	req.data = cfmBuf;
	error = _esc_ioctl(ESC_IOC_CONTENT_EXPORT, &req);
	if (error < 0) {
		error = req.return_error;
		if (!error)
			error = ESCORE_ERR_ESC_DEVICE;
		goto end;
	}

 end:
	return error;
}

int
ESCore_GetTitleSize(u32 titleHandle, u32 *tmdLen)
{
	struct esc_title_export req;
	int error;

	memset(&req, 0, sizeof(req));
	req.title_handle = titleHandle;
	error = _esc_ioctl(ESC_IOC_TITLE_EXPORT, &req);
	if (error < 0) {
		error = req.return_error;
		if (!error)
			error = ESCORE_ERR_ESC_DEVICE;
		goto end;
	}

	*tmdLen = req.tmd_len;

end:
	return error;
}

int
ESCore_GetContentSize(u32 contentHandle, u32 *cfmLen)
{
	struct esc_content_export req;
	int error;

	memset(&req, 0, sizeof(req));
	req.handle = contentHandle;
	error = _esc_ioctl(ESC_IOC_CONTENT_EXPORT, &req);
	if (error < 0) {
		error = req.return_error;
		if (!error)
			error = ESCORE_ERR_ESC_DEVICE;
		goto end;
	}

	*cfmLen = req.size;

end:
	return error;
}

int
ESCore_Init(void *credSecret, u32 secretLen, void *credClear, u32 clearLen, u32 mode)
{
	struct esc_init req;
	int error;

	memset(&req, 0, sizeof(req));
	req.credSecret = credSecret;
	req.secretLen = secretLen;
	req.credClear = credClear;
	req.clearLen = clearLen;
	req.mode = mode;

	error = _esc_ioctl(ESC_IOC_INIT, &req);
	if (error < 0) {
		error = req.return_error;
		if (error == ESC_ERR_ALREADY_INIT)
			error = ESCORE_ERR_ALREADY_INIT;
		if (!error)
			error = ESCORE_ERR_ESC_DEVICE;
		goto end;
	}

 end:
	return error;
}

// The following lines are needed by Emacs to maintain the format the file started in.
// Local Variables:
// c-basic-offset: 8
// tab-width: 8
// indent-tabs-mode: t
// End:
