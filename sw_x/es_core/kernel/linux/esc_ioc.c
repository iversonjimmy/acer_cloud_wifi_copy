
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/time.h>
#include <linux/kthread.h>
#include <linux/io.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/sysfs.h>
#include <linux/random.h>
#include <linux/slab.h>

#include "esc_ioc.h"
#include "esc_ioctl.h"
#include "esc_data.h"
#include "escsvc.h"

#define _PAGE_CNT(x)	(((x) + PAGE_SIZE - 1) / PAGE_SIZE)
#define MIN(x,y)	(((x) < (y)) ? (x) : (y))

#define UADDR(b, s, m) (void __user *)((char __user *)b + offsetof(s, m))


//
// pull down args
//

static int
_esc_ioc_args_get(unsigned int cmd, void __user *arg_v, void *buf, u32 len)
{
	// Make sure we can access the callers buffer
	if ((_IOC_DIR(cmd) & _IOC_READ)
	    && !access_ok(VERIFY_WRITE, arg_v, _IOC_SIZE(cmd))) {
		return -EFAULT;
	}

	// pull down the callers buffer
	if (copy_from_user(buf, arg_v, len)) {
		return  -EFAULT;
	}

	return 0;
}

// allocate pages to copy user-space data, and then copy
static int
_esc_ioc_data_page(u8 __user *buf, u32 len, u32 *frames, u32 frame_max)
{
	u32 page_cnt;
	u8 *bp;
	u32 i;
	u32 xfer_cnt;
	struct page *page;
	u32 paddr;

	page_cnt = _PAGE_CNT(len);
	if ( page_cnt > frame_max ) {
		return -EINVAL;
	}

	for( i = 0 ; i < page_cnt ; i++ ) {
		page = alloc_page(GFP_KERNEL);
		frames[i] = page_to_pfn(page);
		paddr = page_to_phys(page);
		bp = (u8 *)phys_to_virt(paddr);
		xfer_cnt = MIN(PAGE_SIZE, len);
		if ( copy_from_user(bp, &buf[i*PAGE_SIZE], xfer_cnt) ) {
			return -EFAULT;
		}
		len -= xfer_cnt;
	}

	return 0;
}

// copy from frames to ub
static int
_esc_ioc_data_copy_out(u8 __user *ub, u32 len, u32 *frames)
{
	u32 paddr;
	u8 *bp;
	int i;
	int pg_cnt;
	int err = 0;
	u32 xfer_cnt;

	pg_cnt = _PAGE_CNT(len);
	for( i = 0 ; i < pg_cnt ; i++, len -= xfer_cnt) {
		xfer_cnt = (len < PAGE_SIZE) ? len : PAGE_SIZE;
		if ( frames[i] == 0 ) {
			continue;
		}
		paddr = page_to_phys(pfn_to_page(frames[i]));
		bp = (u8 *)phys_to_virt(paddr);
		err = copy_to_user(&ub[i * PAGE_SIZE], bp, xfer_cnt);
		if ( err ) {
			err = -EFAULT;
			break;
		}
	}

	return err;
}

/* allocate pages to store result
 * len is the size needed
 * frames[] stores the virtual address of each page
 * frame_max is the num of elements in frames[]
 */
static int
_esc_ioc_data_page_out(u32 len, u32 *frames, u32 frame_max)
{
	u32 page_cnt;
	u32 i;
	struct page *page;

	page_cnt = _PAGE_CNT(len);
	if ( page_cnt > frame_max ) {
		return -EINVAL;
	}

	for( i = 0 ; i < page_cnt ; i++ ) {
		page = alloc_page(GFP_KERNEL);
		frames[i] = page_to_pfn(page);
		if ( frames[i] == 0 ) {
			printk(KERN_INFO "%d is zero?\n", i);
		}
	}

	return 0;
}

/* free pages
 * frames[] has the virtual address of each page
 * len is the size of data for which pages were allocated
 */
static void
_esc_ioc_data_release(u32 *frames, u32 len)
{
	u32 page_cnt;
	struct page *page;
	u32 i;
	
	page_cnt = _PAGE_CNT(len);

	for( i = 0 ; i < page_cnt ; i++ ) {
		if ( frames[i] == 0 ) {
			continue;
		}
		page = pfn_to_page(frames[i]);
		__free_page(page);
	}
}

static int
_ioc_init(unsigned int cmd, void __user *arg_v, int svccmd)
{
	extern int esc_mod_device_info_setup(void);  // defined in esc.c

	struct esc_init arg;
	esc_init_t req;
	int err = 0;

	if ( (err = _esc_ioc_args_get(cmd, arg_v, &arg, sizeof(arg))) ) {
		return err;
	}

	memset(&req, 0, sizeof(req));

	req.credSecret = kmalloc(arg.secretLen, GFP_KERNEL);
	if (req.credSecret == NULL) {
		err = -ENOMEM;
		goto end;
	}
	if (copy_from_user(req.credSecret, arg.credSecret, arg.secretLen)) {
		err = -EFAULT;
		goto end;
	}
	req.secretLen = arg.secretLen;
	req.credClear = kmalloc(arg.clearLen, GFP_KERNEL);
	if (req.credClear == NULL) {
		err = -ENOMEM;
		goto end;
	}
	if (copy_from_user(req.credClear, arg.credClear, arg.clearLen)) {
		err = -EFAULT;
		goto end;
	}
	req.clearLen = arg.clearLen;
	req.mode = arg.mode;

	err = escsvc_request_sync(svccmd, &req, sizeof(req));
	if (err) {
		printk(KERN_ERR "%s: failed to queue request, err=%d\n", __func__, err);
		err = -ENOBUFS;
		goto end;
	}

	// copy out the error code
	if (copy_to_user(UADDR(arg_v, struct esc_init, return_error),
			 &req.error, sizeof(req.error))) {
		err = -EFAULT;
		goto end;
	}
	if (req.error) {
		err = -EINVAL;
		goto end;
	}

	err = esc_mod_device_info_setup();
end:
	if (req.credSecret != NULL)
		kfree(req.credSecret);
	if (req.credClear != NULL)
		kfree(req.credClear);

	return err;
}

int
esc_ioc_init(unsigned int cmd, void __user *arg_v)
{
	return _ioc_init(cmd, arg_v, ESCSVC_ES_INIT);
}

int
esc_ioc_tkt_import(unsigned int cmd, void __user *arg_v)
{
	struct esc_ticket_import arg;
	esc_ticket_t tkt_req;
	u32 i;
	int err = 0;

	if ( (err = _esc_ioc_args_get(cmd, arg_v, &arg, sizeof(arg))) ) {
		printk(KERN_ERR "%s: _esc_ioc_args_get failed err=%d\n", __func__, err);
		return err;
	}

	// Base sanity checks
	if ( arg.cert_cnt > ESC_CERT_FRAME_MAX ) {
		printk(KERN_ERR "%s: too many certs %d\n", __func__, arg.cert_cnt);
		return -EINVAL;
	}

	// set up the ticket for verification
	memset(&tkt_req, 0, sizeof(tkt_req));

	// At present, the title id/user id is not optional for GVM RT.
	tkt_req.title_id = arg.title_id;
	if ( arg.title_id ) {
		tkt_req.is_title = true;
	}
	tkt_req.user_id = arg.user_id;
	if ( arg.user_id ) {
		tkt_req.is_user = true;
	}
	tkt_req.ticket_size = arg.ticket_len;
	tkt_req.chal_handle = arg.chal_handle;
	if ( (err = _esc_ioc_data_page(arg.ticket, arg.ticket_len,
				        tkt_req.ticket_frames,
					ESC_TICKET_FRAME_MAX))) {
		printk(KERN_ERR "%s: _esc_ioc_data_page failed err=%d\n", __func__, err);
		goto end;
	}
	for( i = 0 ; i < arg.cert_cnt ; i++ ) {
		if ( (err = _esc_ioc_data_page(arg.certs[i],
						arg.cert_lens[i],
						&tkt_req.cert_frames[i],
						1)) ) {
			printk(KERN_ERR "%s: _esc_ioc_data_page failed err=%d\n", __func__, err);
			goto end;
		}
	}
	tkt_req.cert_count = arg.cert_cnt;
	tkt_req.cookie = arg.cookie;

	err = escsvc_request_sync(ESCSVC_ES_TICKET_IMPORT, &tkt_req, sizeof(tkt_req));
	if (err) {
		printk(KERN_ERR "%s: failed to queue req, err=%d\n", __func__, err);
		err = -ENOBUFS;
		goto end;
	}

	// copy out the error code
	if (copy_to_user(UADDR(arg_v, struct esc_ticket_import, return_error),
			 &tkt_req.error, sizeof(tkt_req.error))) {
		err = -EFAULT;
		goto end;
	}
	if (tkt_req.error) {
		err = -EINVAL;
		goto end;
	}

	// copy out the handle
	if (copy_to_user(UADDR(arg_v, struct esc_ticket_import, return_handle),
			 &tkt_req.handle, sizeof(tkt_req.handle))) {
		err = -EFAULT;
		goto end;
	}

end:
	_esc_ioc_data_release(tkt_req.ticket_frames, 
			       arg.ticket_len);
	for( i = 0 ; i < arg.cert_cnt ; i++ ) {
		if ( tkt_req.cert_frames[i] == 0 ) {
			break;
		}
		_esc_ioc_data_release(&tkt_req.cert_frames[i], 
				       arg.cert_lens[i]);
	}

	return err;
}

int
esc_ioc_tkt_release(unsigned int cmd, void __user *arg_v)
{
	struct esc_ticket_release arg;
	esc_resource_rel tkt_req;
	int err = 0;

	if ( (err = _esc_ioc_args_get(cmd, arg_v, &arg, sizeof(arg))) ) {
		return err;
	}

	memset(&tkt_req, 0, sizeof(tkt_req));
	tkt_req.handle = arg.handle;

	err = escsvc_request_sync(ESCSVC_ES_TICKET_RELEASE, &tkt_req, sizeof(tkt_req));
	if (err) {
		printk(KERN_ERR "%s: failed to queue request, err=%d\n", __func__, err);
		err = -ENOBUFS;
		goto end;
	}

	// copy out the error code
	if (copy_to_user(UADDR(arg_v, struct esc_ticket_release, return_error),
			 &tkt_req.error, sizeof(tkt_req.error))) {
		err = -EFAULT;
		goto end;
	}
	if (tkt_req.error) {
		err = -EINVAL;
		goto end;
	}

 end:
	return err;
}

int
esc_ioc_tkt_query(unsigned int cmd, void __user *arg_v)
{
	struct esc_ticket_query *arg = NULL;
	esc_ticket_query_t *tkt_req = NULL;
	int err = 0;

	arg = kcalloc(1, sizeof(*arg), GFP_KERNEL);
	if ( arg == NULL ) {
		return -ENOMEM;
	}

	if ( (err = _esc_ioc_args_get(cmd, arg_v, arg, sizeof(*arg))) ) {
		return err;
	}

	tkt_req = kcalloc(1, sizeof(*tkt_req), GFP_KERNEL);
	if ( tkt_req == NULL ) {
		kfree(arg);
		return -ENOMEM;
	}
	memset(tkt_req, 0, sizeof(*tkt_req));

	tkt_req->title_id = arg->title_id;
	tkt_req->cookie = arg->cookie;
	tkt_req->mask = arg->mask;

	err = escsvc_request_sync(ESCSVC_ES_TICKET_QUERY, tkt_req, sizeof(*tkt_req));
	if (err) {
		printk(KERN_ERR "%s: failed to queue request, err=%d\n", __func__, err);
		err = -ENOBUFS;
		goto end;
	}

	// copy out the error code
	if (copy_to_user(UADDR(arg_v, struct esc_ticket_query, return_error),
			 &tkt_req->error, sizeof(tkt_req->error))) {
		err = -EFAULT;
		goto end;
	}
	if (tkt_req->error) {
		err = -EINVAL;
		goto end;
	}

	// copy out the records
	if (copy_to_user(UADDR(arg_v, struct esc_ticket_query, return_recs),
			 tkt_req->recs, sizeof(tkt_req->recs))) {
		err = -EFAULT;
		goto end;
	}

	// copy out the record count
	if (copy_to_user(UADDR(arg_v, struct esc_ticket_query, return_rec_cnt),
			 &tkt_req->rec_cnt, sizeof(tkt_req->rec_cnt))) {
		err = -EFAULT;
		goto end;
	}

end:
	kfree(arg);
	kfree(tkt_req);

	return err;
}

int
esc_ioc_title_import(unsigned int cmd, void __user *arg_v)
{
	struct esc_title_import arg;
	esc_title_t ttl_req;
	int err = 0;

	if ( (err = _esc_ioc_args_get(cmd, arg_v, &arg, sizeof(arg))) ) {
        	printk(KERN_INFO "title_import: failed pulling down args\n");
		return err;
	}

	// set up the ticket for verification
	memset(&ttl_req, 0, sizeof(ttl_req));

	ttl_req.title_id = arg.title_id;
	ttl_req.ticket_sig_handle = arg.ticket_sig_handle;
	ttl_req.ticket_data_handle = arg.ticket_data_handle;
	ttl_req.cookie = arg.cookie;

	// set up the TMD
	ttl_req.tmd_size = arg.tmd_len;
	if ( (err = _esc_ioc_data_page(arg.tmd, arg.tmd_len,
				        ttl_req.tmd_frames,
					ESC_TMD_FRAME_MAX))) {
        	printk(KERN_INFO "title_import: paging tmd\n");
		goto end;
	}

	err = escsvc_request_sync(ESCSVC_ES_TITLE_IMPORT, &ttl_req, sizeof(ttl_req));
	if (err) {
		printk(KERN_ERR "%s: failed to queue request, err=%d\n", __func__, err);
		err = -ENOBUFS;
		goto end;
	}

	// copy out the return error
	if (copy_to_user(UADDR(arg_v, struct esc_title_import, return_error),
			 &ttl_req.error, sizeof(ttl_req.error))) {
		err = -EFAULT;
		goto end;
	}
	if (ttl_req.error) {
		err = -EINVAL;
		goto end;
	}

	// copy out the return title handle
	if (copy_to_user(UADDR(arg_v, struct esc_title_import, return_handle),
			 &ttl_req.title_handle, sizeof(ttl_req.title_handle))) {
		err = -EFAULT;
		goto end;
	}


end:
	_esc_ioc_data_release(ttl_req.tmd_frames, arg.tmd_len);

	return err;
}

int
esc_ioc_title_release(unsigned int cmd, void __user *arg_v)
{
	struct esc_title_release arg;
	esc_resource_rel ttl_req;
	int err = 0;

	if ( (err = _esc_ioc_args_get(cmd, arg_v, &arg, sizeof(arg))) ) {
		return err;
	}

	memset(&ttl_req, 0, sizeof(ttl_req));
	ttl_req.handle = arg.handle;

	err =  escsvc_request_sync(ESCSVC_ES_TITLE_RELEASE, &ttl_req, sizeof(ttl_req));
	if (err) {
		printk(KERN_ERR "%s: failed to queue request, err=%d\n", __func__, err);
		err = -ENOBUFS;
		goto end;
	}

	// copy out the error code
	if (copy_to_user(UADDR(arg_v, struct esc_title_release, return_error),
			 &ttl_req.error, sizeof(ttl_req.error))) {
		err = -EFAULT;
		goto end;
	}
	if (ttl_req.error) {
		err = -EINVAL;
		goto end;
	}

 end:
	return err;
}

int
esc_ioc_title_query(unsigned int cmd, void __user *arg_v)
{
	struct esc_title_query *arg = NULL;
	esc_title_query_t *ttl_req = NULL;
	int err = 0;

	arg = kcalloc(1, sizeof(*arg), GFP_KERNEL);
	if ( arg == NULL ) {
		return -ENOMEM;
	}
	if ( (err = _esc_ioc_args_get(cmd, arg_v, arg, sizeof(*arg))) ) {
		return err;
	}

	ttl_req = kcalloc(1, sizeof(*ttl_req), GFP_KERNEL);
	if ( ttl_req == NULL ) {
		kfree(arg);
		return -ENOMEM;
	}
	memset(ttl_req, 0, sizeof(*ttl_req));

	ttl_req->title_id = arg->title_id;
	ttl_req->cookie = arg->cookie;
	ttl_req->mask = arg->mask;

	err = escsvc_request_sync(ESCSVC_ES_TITLE_QUERY, ttl_req, sizeof(*ttl_req));
	if (err) {
		printk(KERN_ERR "%s: failed to queue request, err=%d\n", __func__, err);
		err = -ENOBUFS;
		goto end;
	}

	// copy out the error code
	if (copy_to_user(UADDR(arg_v, struct esc_title_query, return_error),
			 &ttl_req->error, sizeof(ttl_req->error))) {
		err = -EFAULT;
		goto end;
	}
	if (ttl_req->error) {
		err = -EINVAL;
		goto end;
	}

	// copy out the records
	if (copy_to_user(UADDR(arg_v, struct esc_title_query, return_recs),
			 &ttl_req->recs, sizeof(ttl_req->recs))) {
		err = -EFAULT;
		goto end;
	}

	// copy out the record count
	if (copy_to_user(UADDR(arg_v, struct esc_title_query, return_rec_cnt),
			 &ttl_req->rec_cnt, sizeof(ttl_req->rec_cnt))) {
		err = -EFAULT;
		goto end;
	}

end:
	kfree(arg);
	kfree(ttl_req);
	return err;
}

int
esc_ioc_title_export(unsigned int cmd, void __user *arg_v)
{
	struct esc_title_export arg;
	esc_title_export_t req;
	int err = 0;

	if ( (err = _esc_ioc_args_get(cmd, arg_v, &arg, sizeof(arg))) ) {
        	printk(KERN_INFO "title_import: failed pulling down args\n");
		return err;
	}

	// set up the request
	memset(&req, 0, sizeof(req));
	req.title_handle = arg.title_handle;
	req.ticket_handle = arg.ticket_handle;
	req.tmd_size = arg.tmd_len;

	// if the length is zero, it's a request for the length
	// prior to the actual request for data.
	if ( arg.tmd_len ) {
		// set up the output buffer
		if ( (err = _esc_ioc_data_page_out(arg.tmd_len,
						    req.tmd_frames,
						    ESC_TMD_FRAME_MAX))) {
			printk(KERN_INFO "title_export: paging tmd\n");
			goto end;
		}
	}

	err = escsvc_request_sync(ESCSVC_ES_TITLE_EXPORT, &req, sizeof(req));
	if (err) {
		printk(KERN_ERR "%s: failed to queue request, err=%d\n", __func__, err);
		err = -ENOBUFS;
		goto end;
	}

	// copy out the error code
	if (copy_to_user(UADDR(arg_v, struct esc_title_export, return_error),
			 &req.error, sizeof(req.error))) {
		err = -EFAULT;
		goto end;
	}
	if (req.error) {
		err = -EINVAL;
		goto end;
	}

	// return the length of the TMD
	if ( arg.tmd_len == 0 ) {
		if (copy_to_user(UADDR(arg_v, struct esc_title_export, tmd_len),
				 &req.tmd_size, sizeof(req.tmd_size))) {
			err = -EFAULT;
			goto end;
		}
		goto end;
	}

	// copy out the return TMD data
	err = _esc_ioc_data_copy_out(arg.tmd, arg.tmd_len,
				     req.tmd_frames);
	if ( err ) {
		goto end;
	}

end:
	_esc_ioc_data_release(req.tmd_frames, arg.tmd_len);

	return err;
}

int
esc_ioc_content_import_id(unsigned int cmd, void __user *arg_v)
{
	struct esc_content_import_id arg;
	esc_content_t *cfm_req = NULL;
	int frame_cnt;
	int req_len;
	int err = 0;

	if ( (err = _esc_ioc_args_get(cmd, arg_v, &arg, sizeof(arg))) ) {
		return err;
	}

	// allocate a buffer for the variable length cfm_req structure
	frame_cnt = _PAGE_CNT(arg.cfm_len);
	req_len = sizeof(esc_content_t) + (frame_cnt * sizeof(u32));
	cfm_req = kcalloc(1, req_len, GFP_KERNEL);
	if ( cfm_req == NULL ) {
		return -ENOMEM;
	}

	memset(cfm_req, 0, req_len);

	cfm_req->title_handle = arg.title_handle;
	cfm_req->content_id = arg.content_id;
	cfm_req->size = arg.cfm_len;
	if ( (err = _esc_ioc_data_page(arg.cfm, arg.cfm_len,
				        cfm_req->frames,
					frame_cnt))) {
		goto end;
	}

	err = escsvc_request_sync(ESCSVC_ES_CONTENT_IMPORT_ID, cfm_req, req_len);
	if (err) {
		printk(KERN_ERR "%s: failed to queue request, err=%d\n", __func__, err);
		err = -ENOBUFS;
		goto end;
	}

	// copy out the error code
	if (copy_to_user(UADDR(arg_v, struct esc_content_import_id, return_error),
			 &cfm_req->error, sizeof(cfm_req->error))) {
		err = -EFAULT;
		goto end;
	}
	if (cfm_req->error) {
		err = -EINVAL;
		goto end;
	}

	// copy out the handle
	if (copy_to_user(UADDR(arg_v, struct esc_content_import_id, return_handle),
			 &cfm_req->content_handle, sizeof(cfm_req->content_handle))) {
		err = -EFAULT;
		goto end;
	}

end:
	_esc_ioc_data_release(cfm_req->frames, arg.cfm_len);

	kfree(cfm_req);
	
	return err;
}

int
esc_ioc_content_import_idx(unsigned int cmd, void __user *arg_v)
{
	struct esc_content_import_idx arg;
	esc_content_ind_t *cfm_req = NULL;
	int frame_cnt;
	int req_len;
	int err = 0;

	if ( (err = _esc_ioc_args_get(cmd, arg_v, &arg, sizeof(arg))) ) {
		return err;
	}

	// allocate a buffer for the variable length cfm_req structure
	frame_cnt = _PAGE_CNT(arg.cfm_len);
	req_len = sizeof(esc_content_ind_t) + (frame_cnt * sizeof(u32));
	cfm_req = kcalloc(1, req_len, GFP_KERNEL);
	if ( cfm_req == NULL ) {
		return -ENOMEM;
	}

	memset(cfm_req, 0, req_len);

	cfm_req->title_handle = arg.title_handle;
	cfm_req->content_ind = arg.content_index;
	cfm_req->size = arg.cfm_len;
	if ( (err = _esc_ioc_data_page(arg.cfm, arg.cfm_len,
				        cfm_req->frames,
					frame_cnt))) {
		goto end;
	}

	err = escsvc_request_sync(ESCSVC_ES_CONTENT_IMPORT_IDX, cfm_req, req_len);
	if (err) {
		printk(KERN_ERR "%s: failed to queue request, err=%d\n", __func__, err);
		err = -ENOBUFS;
		goto end;
	}

	// copy out the error code
	if (copy_to_user(UADDR(arg_v, struct esc_content_import_idx, return_error),
			 &cfm_req->error, sizeof(cfm_req->error))) {
		err = -EFAULT;
		goto end;
	}
	if (cfm_req->error) {
		err = -EINVAL;
		goto end;
	}

	// copy out the handle
	if (copy_to_user(UADDR(arg_v, struct esc_content_import_idx, return_handle),
			 &cfm_req->content_handle, sizeof(cfm_req->content_handle))) {
		err = -EFAULT;
		goto end;
	}

end:
	_esc_ioc_data_release(cfm_req->frames, arg.cfm_len);

	kfree(cfm_req);
	
	return err;
}

int
esc_ioc_content_release(unsigned int cmd, void __user *arg_v)
{
	struct esc_content_release arg;
	esc_resource_rel cfm_req;
	int err = 0;

	if ( (err = _esc_ioc_args_get(cmd, arg_v, &arg, sizeof(arg))) ) {
		return err;
	}

	memset(&cfm_req, 0, sizeof(cfm_req));
	cfm_req.handle = arg.handle;

	err = escsvc_request_sync(ESCSVC_ES_CONTENT_RELEASE, &cfm_req, sizeof(cfm_req));
	if (err) {
		printk(KERN_ERR "%s: failed to queue request, err=%d\n", __func__, err);
		err = -ENOBUFS;
		goto end;
	}

	// copy out the error code
	if (copy_to_user(UADDR(arg_v, struct esc_content_release, return_error),
			 &cfm_req.error, sizeof(cfm_req.error))) {
		err = -EFAULT;
		goto end;
	}
	if (cfm_req.error) {
		err = -EINVAL;
		goto end;
	}

 end:
	return err;
}

int
esc_ioc_content_query(unsigned int cmd, void __user *arg_v)
{
	struct esc_content_query *arg = kmalloc(sizeof(*arg), GFP_KERNEL);
	esc_content_query_t *cfm_req = kcalloc(1, sizeof(*cfm_req), GFP_KERNEL);
	int err = 0;

	if (arg == NULL || cfm_req == NULL) {
		err = -ENOMEM;
		goto end;
	}

	if ( (err = _esc_ioc_args_get(cmd, arg_v, arg, sizeof(*arg))) ) {
		goto end;
	}

	cfm_req->title_handle = arg->title_handle;

	err = escsvc_request_sync(ESCSVC_ES_CONTENT_QUERY, cfm_req, sizeof(*cfm_req));
	if (err) {
		printk(KERN_ERR "%s: failed to queue request, err=%d\n", __func__, err);
		err = -ENOBUFS;
		goto end;
	}

	// copy out the error code
	if (copy_to_user(UADDR(arg_v, struct esc_content_query, return_error),
			 &cfm_req->error, sizeof(cfm_req->error))) {
		err = -EFAULT;
		goto end;
	}
	if (cfm_req->error) {
		err = -EINVAL;
		goto end;
	}

	// copy out the records
	if (copy_to_user(UADDR(arg_v, struct esc_content_query, return_recs),
			 &cfm_req->recs, sizeof(cfm_req->recs))) {
		err = -EFAULT;
		goto end;
	}

	// copy out the record count
	if (copy_to_user(UADDR(arg_v, struct esc_content_query, return_rec_cnt),
			 &cfm_req->rec_cnt, sizeof(cfm_req->rec_cnt))) {
		err = -EFAULT;
		goto end;
	}

end:
	if (arg)
		kfree(arg);
	if (cfm_req)
		kfree(cfm_req);

	return err;
}

int
esc_ioc_content_export(unsigned int cmd, void __user *arg_v)
{
	struct esc_content_export arg;
	esc_content_export_t *req = NULL;
	u32 req_sz;
	u32 pg_cnt;
	int err = 0;

	if ( (err = _esc_ioc_args_get(cmd, arg_v, &arg, sizeof(arg))) ) {
        	printk(KERN_INFO "title_import: failed pulling down args\n");
		return err;
	}

	// allocate the request
	pg_cnt = _PAGE_CNT(arg.size);
	req_sz = sizeof(*req) + (pg_cnt * sizeof(u32));
	req = kcalloc(1, req_sz, GFP_KERNEL);
	if ( req == NULL ) {
		return -ENOMEM;
	}

	// set up the request
	memset(req, 0, req_sz);
	req->content_handle = arg.handle;
	req->size = arg.size;

	// if the length is zero, it's a request for the length
	// prior to the actual request for data.
	if ( arg.size ) {
		// set up the output buffer
		if ( (err = _esc_ioc_data_page_out(arg.size,
						    req->frames,
						    pg_cnt))) {
			printk(KERN_INFO "content_export: paging content\n");
			goto end;
		}
	}

	err = escsvc_request_sync(ESCSVC_ES_CONTENT_EXPORT, req, req_sz);
	if (err) {
		printk(KERN_ERR "%s: failed to queue request, err=%d\n", __func__, err);
		err = -ENOBUFS;
		goto end;
	}

	// copy out the error code
	if (copy_to_user(UADDR(arg_v, struct esc_content_export, return_error),
			 &req->error, sizeof(req->error))) {
		err = -EFAULT;
		goto end;
	}
	if (req->error) {
		err = -EINVAL;
		goto end;
	}

	// return the length of the CFM
	if ( arg.size == 0 ) {
		if (copy_to_user(UADDR(arg_v, struct esc_content_export, size),
				 &req->size, sizeof(req->size))) {
			err = -EFAULT;
			goto end;
		}
		goto end;
	}

	// copy out the return TMD data
	err = _esc_ioc_data_copy_out(arg.data, arg.size, req->frames);
	if ( err ) {
		goto end;
	}

end:
	_esc_ioc_data_release(req->frames, arg.size);
	if ( req ) {
		kfree(req);
	}

	return err;
}

static int
_block_crypt(unsigned int cmd, void __user *arg_v, int is_decrypt )
{
	struct esc_block_crypt arg;
	struct page *page;
	esc_block_t req;
	u32 paddr;
	int err = 0;
	int i;
	u8 *bp;

	if ( (err = _esc_ioc_args_get(cmd, arg_v, &arg, sizeof(arg))) ) {
		printk(KERN_INFO "_block_crypt: failed reading args\n");
		return err;
	}
	
	if ( arg.page_cnt > ESC_BLOCK_PAGE_MAX ) {
		printk(KERN_INFO "_block_crypt: bad page cnt %d\n",
			arg.page_cnt);
		return -EINVAL;
	}

	memset(&req, 0, sizeof(req));
	req.content_handle = arg.content_handle;
	req.block_number = arg.block_number;
	req.page_cnt = arg.page_cnt;

	// map in the input pages
	for( i = 0 ; i < arg.page_cnt ; i++ ) {
		err =  _esc_ioc_data_page(arg.pages_in[i], PAGE_SIZE,
					   &req.frames_in[i], 1);
		if ( err ) {
			printk(KERN_INFO "_block_crypt: bad page in %d\n", i);
			goto end;
		}
	}

	// map in the output pages
	for( i = 0 ; i < arg.page_cnt ; i++ ) {
		if ( arg.pages_out[i] == NULL ) {
			continue;
		}
		page = alloc_page(GFP_KERNEL);
		req.frames_out[i] = page_to_pfn(page);
	}

	err = escsvc_request_sync(is_decrypt ? ESCSVC_ES_BLOCK_DECRYPT :
					       ESCSVC_ES_BLOCK_ENCRYPT,
				  &req, sizeof(req));
	if (err) {
		printk(KERN_ERR "%s: failed to queue request, err=%d\n", __func__, err);
		err = -ENOBUFS;
		goto end;
	}

	// copy out the error code
	if (copy_to_user(UADDR(arg_v, struct esc_block_crypt, return_error),
			 &req.error, sizeof(req.error))) {
		err = -EFAULT;
		goto end;
	}
	if (req.error) {
		err = -EINVAL;
		goto end;
	}

	// copy out the data
	for( i = 0 ; i < arg.page_cnt ; i++ ) {
		if ( !req.frames_out[i] ) {
			continue;
		}
		paddr = page_to_phys(pfn_to_page(req.frames_out[i]));
		bp = (u8 *)phys_to_virt(paddr);
		if (copy_to_user(arg.pages_out[i], bp, PAGE_SIZE)) {
			err = -EFAULT;
			goto end;
		}
	}

end:
	// free up any allocated pages
	_esc_ioc_data_release(req.frames_in, arg.page_cnt * PAGE_SIZE);
	_esc_ioc_data_release(req.frames_out, arg.page_cnt * PAGE_SIZE);

	return err;
}

int
esc_ioc_block_decrypt(unsigned int cmd, void __user *arg)
{
	return _block_crypt(cmd, arg, 1);
}

int
esc_ioc_block_encrypt(unsigned int cmd, void __user *arg)
{
	return _block_crypt(cmd, arg, 0);
}

int
esc_ioc_device_info(unsigned int cmd, void __user *arg_v)
{
	extern esc_device_id_t esc_dev_id;  // defined in esc.c
	struct esc_device_info *arg;
	int err = 0;

	arg = kmalloc(sizeof(*arg), GFP_KERNEL);
	if (arg == NULL) {
		err = -ENOMEM;
		goto end;
	}

	// we already have this information cached so just return it without
	// making a call to escsvc
	arg->id = esc_dev_id.device_id;
	arg->type = esc_dev_id.device_type;
	memcpy(arg->cert, esc_dev_id.device_cert, sizeof(arg->cert));
	arg->return_error = 0;

	if (copy_to_user((void __user *)arg_v, arg, sizeof(*arg))) {
		err = -EFAULT;
		goto end;
	}

 end:
	if (arg)
		kfree(arg);
	return err;
}

int
esc_ioc_gss_create(unsigned int cmd, void __user *arg_v)
{
	struct esc_gss_create arg;
	esc_gss_create_t req;
	int err = 0;

	if ( (err = _esc_ioc_args_get(cmd, arg_v, &arg, sizeof(arg))) ) {
		return err;
	}
	
	memset(&req, 0, sizeof(req));

	req.user_id = arg.user_id;
	req.cookie = arg.cookie;
	req.ticket_sign_handle = arg.ticket_sign_handle;
	req.ticket_data_handle = arg.ticket_data_handle;
	req.title_handle = arg.title_handle;
	req.content_id = arg.content_id;

	err = escsvc_request_sync(ESCSVC_ES_GSS_CREATE, &req, sizeof(req));
	if (err) {
		printk(KERN_ERR "%s: failed to queue request, err=%d\n", __func__, err);
		err = -ENOBUFS;
		goto end;
	}

	// copy out the results
	if (copy_to_user(UADDR(arg_v, struct esc_gss_create, return_error),
			 &req.error, sizeof(req.error))) {
		err = -EFAULT;
		goto end;
	}
	if (req.error) {
		err = -EINVAL;
		goto end;
	}

	// copy out the data
	if (copy_to_user(UADDR(arg_v, struct esc_gss_create, return_title_handle),
			 &req.return_title_handle, sizeof(req.return_title_handle))) {
		err = -EFAULT;
		goto end;
	}
	if (copy_to_user(UADDR(arg_v, struct esc_gss_create, return_content_handle),
			 &req.return_content_handle, sizeof(req.return_content_handle))) {
		err = -EFAULT;
		goto end;
	}

end:
	return err;
}

long
escdev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	void __user *u_arg = (void __user*)arg;
	int err = 0;

	switch (cmd) {
	case ESC_IOC_INIT:
		err = esc_ioc_init(cmd, u_arg);
		break;
	case ESC_IOC_TICKET_IMPORT:
		err = esc_ioc_tkt_import(cmd, u_arg);
		break;
	case ESC_IOC_TICKET_RELEASE:
		err = esc_ioc_tkt_release(cmd, u_arg);
		break;
	case ESC_IOC_TICKET_QUERY:
		err = esc_ioc_tkt_query(cmd, u_arg);
		break;
	case ESC_IOC_TITLE_IMPORT:
		err = esc_ioc_title_import(cmd, u_arg);
		break;
	case ESC_IOC_TITLE_RELEASE:
		err = esc_ioc_title_release(cmd, u_arg);
		break;
	case ESC_IOC_TITLE_QUERY:
		err = esc_ioc_title_query(cmd, u_arg);
		break;
	case ESC_IOC_TITLE_EXPORT:
		err = esc_ioc_title_export(cmd, u_arg);
		break;
	case ESC_IOC_CONTENT_IMPORT_ID:
		err = esc_ioc_content_import_id(cmd, u_arg);
		break;
	case ESC_IOC_CONTENT_IMPORT_IDX:
		err = esc_ioc_content_import_idx(cmd, u_arg);
		break;
	case ESC_IOC_CONTENT_RELEASE:
		err = esc_ioc_content_release(cmd, u_arg);
		break;
	case ESC_IOC_CONTENT_QUERY:
		err = esc_ioc_content_query(cmd, u_arg);
		break;
	case ESC_IOC_CONTENT_EXPORT:
		err = esc_ioc_content_export(cmd, u_arg);
		break;
	case ESC_IOC_BLOCK_DECRYPT:
		err = esc_ioc_block_decrypt(cmd, u_arg);
		break;
	case ESC_IOC_BLOCK_ENCRYPT:
		err = esc_ioc_block_encrypt(cmd, u_arg);
		break;
	case ESC_IOC_DEVICE_INFO:
		err = esc_ioc_device_info(cmd, u_arg);
		break;
	case ESC_IOC_GSS_CREATE:
		err = esc_ioc_gss_create(cmd, u_arg);
		break;
	default:
		printk(KERN_INFO "Unknown ioctl cmd (0x%08x)\n", cmd);
		err = -EINVAL;
		break;
	}

	return err;
}

// The following lines are needed by Emacs to maintain the format the file started in.
// Local Variables:
// c-basic-offset: 8
// tab-width: 8
// indent-tabs-mode: t
// End:
