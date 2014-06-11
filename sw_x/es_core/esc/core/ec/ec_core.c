
// vim: sw=8:ts=8:noexpandtab

#include "core_glue.h"

#define EC_DEBUG	1
#include "ec_data_int.h"

#include "cfm.h"
#include "tmd_tmpl.h"

// For stat collection
// #include "ept.h"
// #include "tmap.h"

// Temporary random number generation support
#include "mt64.h"

#include "ghv_crypto_impl.h"

#include "ec_pers.h"

// TODO: is it safe to remove this?
#ifdef __KERNEL__
#  ifndef _KERNEL_MODULE
#    error Unexpected!
#  endif
#endif
//#ifdef __KERNEL__
//#include <linux/slab.h>
//#endif

#define BIT_IND(blk)		((blk)/8)
// BIT_MASK renamed to BIT_BYTEMASK to avoid name clash with linux kernel header (linux/bitops.h),
//  which has a different definition
#define BIT_BYTEMASK(blk)	(1 << ((blk)%8))
#define BIT_IS_SET(bits, blk)	(bits[BIT_IND(blk)] & BIT_BYTEMASK(blk))

// default size in bytes
#define _GSS_DFLT_SIZE		(128)		// 128 MiB
#define _GSS_BLK_SIZE		(4096)

#define _GSS_TMD_SIG_BYTE_START	320
#define _GSS_TMD_SIG_BYTE_END	516
#define _GSS_TMD_SIG_DATA_LEN	(_GSS_TMD_SIG_BYTE_END - _GSS_TMD_SIG_BYTE_START)
#define _GSS_TMD_SIG_LEN	20

struct hc_aes_iv_s {
	u64	hav_title_id;
	u32	hav_content_index;
	u32	hav_block_number;
};
typedef struct hc_aes_iv_s hc_aes_iv;

#ifndef MIN
#define MIN(a,b)	(((a) < (b)) ? (a) : (b)) 
#endif

// This set of macros produces unique-ish handles that differentiate
// between titles and contents and current handles from older handles.
// It also instantiates the data structures necessary to maintain them.

#define TKT_KEY		'K'
#define TTL_KEY		'T'
#define CFM_KEY		'C'

#define CFM_CACHE_MAX		ESC_CONTENT_MAX
#define TKT_CACHE_MAX		ESC_TICKET_MAX
#define TTL_CACHE_MAX		ESC_TITLE_MAX

CACHE_INST(tkt, TKT, "ticket")
CACHE_INST(ttl, TTL, "title")
CACHE_INST(cfm, CFM, "content")

// Following constants pulled from lib/es/es_title.cpp
#define TKT_BUF_SIZE 		(sizeof(ESTicket) + sizeof(ESV1TicketHeader))


// IV buffer
static u8 __iv_buf[sizeof(IOSCAesIv)] ATTR_AES_ALIGN;
static u8 __codec_buf[VM_PAGE_SIZE * ESC_BLOCK_PAGE_MAX] ATTR_AES_ALIGN;

static un __mem_pool_size;

static u32 rights_size[EC_RIGHTS_MAX] = {
	0,
	sizeof(ESV1PermanentRecord),
	sizeof(ESV1SubscriptionRecord),
	sizeof(ESV1ContentRecord),
	sizeof(ESV1ContentConsumptionRecord),
	sizeof(ESV1AccessTitleRecord)
};

#if defined(GVM_FEAT_PLAT_CMN_KEY)
#define IGWARE_PART_CHAN_ID	0x00000006
#endif // defined(GVM_FEAT_PLAT_CMN_KEY)

static int _esh_content_ind_2_id(ec_title *title, u32 ind, u32 *idp);

static u32 device_id;

static bool esc_initialized = 0;
static bool esc_cred_loaded = 0;

static un
_mem_avail(void)
{
	return heap_free_size()/VM_PAGE_SIZE;
}

static int
_es_hypercalls_init(void)
{
	if ( ESC_DEV_CERT_SIZE != ES_DEVICE_CERT_SIZE ) {
		DBLOG("DEV Cert Sizes don't match\n");
		return -1;	
	}

	__mem_pool_size = _mem_avail();
	DBLOG_N("mem pool size = %d", (u32)__mem_pool_size);

	return 0;
}

int
esc_init(esc_init_t *req)
{
	req->error = ESC_ERR_UNSPECIFIED;  // assume error and prove otherwise

	if (esc_initialized) {
		req->error = ESC_ERR_ALREADY_INIT;
		return -1;
	}

	if ( crypto_impl_init((dev_cred_default*)req->cred_default,
			      req->default_len) ) {
		LOG("Failed initializing the crypto library\n");
		req->error = ESC_ERR_CRYPTO;
		return -1;
	}

	if ( _es_hypercalls_init() ) {
		LOG("Failed initializing the ES support\n");
		return -1;
	}

	esc_initialized = 1;
	req->error = ESC_ERR_OK;

	return 0;
}

int
esc_load_credentials(esc_cred_t *req)
{
	IOSCError error;

	req->error = ESC_ERR_UNSPECIFIED;  // assume error and prove otherwise

	if (!esc_initialized) {
		req->error = ESC_ERR_NOT_YET_INIT;
		return -1;
	}

	if (esc_cred_loaded) {
		req->error = ESC_ERR_ALREADY_INIT;
		return -1;
	}

	if (!(req->mode & ESC_INIT_MODE_INSECURE_CRED)) {
		u32 err = esc_decrypt_cred(req->credSecret, req->secretLen, 
					   req->credClear, req->clearLen);
		if (err) {
			LOG("Failed to decrypt creds\n");
			req->error = ESC_ERR_CRED_DECRYPT;
			return -1;
		}
	}

	if ( crypto_impl_load_cred((dev_cred_secret*)req->credSecret, (dev_cred_clear*)req->credClear, req->clearLen) ) {
		LOG("Failed initializing the crypto library\n");
		req->error = ESC_ERR_CRYPTO;
		return -1;
	}

	error = IOSC_GetData(IOSC_DEV_ID_HANDLE, &device_id);
	if ( error ) {
		DBLOG_N("Failed retrieving device ID - %d", error);
		return -1;
	}

	esc_cred_loaded = 1;
	req->error = ESC_ERR_OK;

	return 0;
}

static ec_cfm *
_esh_content_find_id(ec_title *title, u32 content_id)
{
	ec_cfm *cfm;

	// walk the title's content list.
	for( cfm = title->cfm_list ; cfm ; cfm = cfm->next ) {
		if ( cfm->content_id != content_id ) {
			continue;
		}

		return cfm;
	}

	return NULL;
}

static s32
_esh_content_release(u32 content_handle)
{
	ec_cfm *cfm;
	int err;

	DBLOG("Content Handle 0x%08x\n", content_handle);

	// look up the title
	if ( (err = cache_find(&__cfm_cache, content_handle, (void **)&cfm)) ) {
		return err;
	}

	DBLOG("Content id 0x%08x\n", cfm->content_id);

	// remove it from the title's list
	if ( cfm->title->cfm_list == cfm ) {
		cfm->title->cfm_list = cfm->next;
	}
	if ( cfm->next ) {
		cfm->next->last = cfm->last;
	}
	if ( cfm->last ) {
		cfm->last->next = cfm->next;
	}

	// remove it from the cache
	cache_del(&__cfm_cache, content_handle);

	free(cfm->cfm_data);
	free(cfm);

	return ESC_ERR_OK;
}

static void
_esh_ticket_rights_free(ec_ticket_t *ticket)
{
	u32 i;

	for( i = 0 ; i < EC_RIGHTS_MAX ; i++ ) {
		if ( ticket->rights[i].data == NULL ) {
			continue;
		}
		free(ticket->rights[i].data);
		ticket->rights[i].data = NULL;
	}
}

static void
_esh_data_title_release(ec_title *title)
{
	ec_title *tp;
	ec_title *tmp;

	// clean up any data title references
	if ( !title->is_data_title ) {
		tmp = title->data_link;
		title->data_link = NULL;
		for( tp = tmp ; tp ; tp = tmp ) {
			tmp = tp->data_link;
			tp->data_link = NULL;
			tp->data_is_referenced = false;
			tp->parent_link = NULL;
		}
	}

	// clean up an parent referencs
	if ( !title->is_data_title || !title->data_is_referenced ) {
		return;
	}

	// remove from the parents list
	// See if it's the head of the list
	if ( title->parent_link->data_link == title ) {
		title->parent_link->data_link = title->data_link;
		title->data_link = NULL;
		return;
	}

	// remove from the list
	for( tp = title->parent_link->data_link ; tp->data_link ;
	     tp = tp->data_link ) {
		if ( tp->data_link == title ) {
			tp->data_link = title->data_link;
			title->data_link = NULL;
			return;
		}
	}

	DBLOG("*** Didn't find data title "FMT0x64" in parent "FMT0x64"\n",
		title->title_id, title->parent_link->title_id);
}

s32
eshi_title_release(u32 handle)
{
	ec_title *title;
	ec_cfm *cfm;
	s32     err;
	ec_dec_stream_t *stream;

	DBLOG("handle "FMT0x32"\n", handle);

	// look up the title
	if ( (err = cache_find(&__ttl_cache, handle, (void **)&title)) ) {
		return err;
	}

	DBLOG("Title "FMT0x64"\n", title->title_id);

	// release any associated content
	for( cfm = title->cfm_list ; cfm ; cfm = title->cfm_list ) {
		(void)_esh_content_release(cfm->content_handle);
	}

	// remove it from the cache
	cache_del(&__ttl_cache, handle);

	// clean up and data/parent references
	_esh_data_title_release(title);

	// release the reference on the tickets
	if ( title->ticket ) {
		title->ticket->ref_cnt--;
	}

	// free up any active content stream decrypts
	for( stream = title->dec_streams.next ; stream != &title->dec_streams ;
			stream = title->dec_streams.next ) {
		DBLOG("deleting stream cid 0x%08x", stream->content_id);
		stream->next->last = stream->last;
		stream->last->next = stream->next;
		free(stream);
	}

	// release it
        iStorageMemFree(&title->tmd_rdr);
	free(title->tmd_data);
	free(title);

	return ESC_ERR_OK;
}

static s32
_esh_ticket_release(u32 handle)
{
	s32 svc_err;
	ec_ticket_t *tkt;

	// look up the ticket
	svc_err = cache_find(&__tkt_cache, handle, (void **)&tkt);
	if ( svc_err ) {
		DBLOG("Bad handle - 0x%08x\n", handle);
		goto end;
	}

	// do we allow the release to continue if anyone refers
	// to it? No.
	if ( tkt->ref_cnt ) {
		DBLOG("Ticket is in use - %d\n", tkt->ref_cnt);
		svc_err = ESC_ERR_BUSY;
		goto end;
	}

	DBLOG("release tkt handle 0x%08x - title "FMT0x64"\n", handle,
	      tkt->title_id);

	// remove it from the cache.
	cache_del(&__tkt_cache, handle);

	// free up any loaded rights
	_esh_ticket_rights_free(tkt);

	if ( tkt->key ) {
		(void)IOSC_DeleteObject(tkt->key);
	}
	free(tkt);

end:
	return svc_err;
}

int
esc_device_idcert_get(esc_device_id_t *req)
{
	IOSCError error;

	if (!esc_initialized || !esc_cred_loaded) {
		req->error = ESC_ERR_NOT_YET_INIT;
		return -1;
	}

	req->error = ESC_ERR_UNSPECIFIED;

	req->device_id = device_id;

	error = IOSCryptoGetDeviceType(&req->device_type);
	if ( error ) {
		DBLOG("Failed retrieving device type\n");
		goto end;
	}

	error = IOSCryptoGetDeviceCert(req->device_cert);
	if ( error ) {
		DBLOG("Failed retrieving device cert\n");
		goto end;
	}

	LOG_DEBUG("dev id 0x%08x - type 0x%08x", req->device_id,
	      req->device_type);

	req->error = ESC_ERR_OK;
end:
	return (req->error) ? -1 : 0;
}

#ifdef EC_DEBUG
static void
_esh_tkt_sect_hdr_dump(ESV1SectionHeader *shp)
{
	DBLOG("Ticket Section Dump - %p\n", shp);
	DBLOG("sectOfst 0x%08x\n", cpu_to_be32(shp->sectOfst));
	DBLOG("nRecords 0x%08x\n", cpu_to_be32(shp->nRecords));
	DBLOG("recordSize 0x%08x\n", cpu_to_be32(shp->recordSize));
	DBLOG("sectionSize 0x%08x\n", cpu_to_be32(shp->sectionSize));
	DBLOG("sectionType 0x%08x\n", cpu_to_be16(shp->sectionType));
	DBLOG("flags 0x%08x\n", cpu_to_be16(shp->flags));
}
#endif // EC_DEBUG

//
// Pull the rights out of the ETicket. Roughly based on ETicket::GetItemRights()
// Although that doesn't quite work the way I'd want it to work.
// 
static u32
_esh_rights_get(ec_ticket_t *tkt, IInputStream *tkt_rdr, u8 *tkt_buf)
{
	u32 i;
	ESV1TicketHeader *thp;
	ESV1SectionHeader *shp = NULL;
    	u32 pos, size;
	u32 rv = ESC_ERR_OK;
	u32 sect_type;
	u32 sect_cnt;
	hc_right *rights;

	// get a pointer to the header (just past the ticket)
	thp = (ESV1TicketHeader *)&tkt_buf[sizeof(ESTicket)];

	DBLOG("ESTicket size 0x%08x - ESV1 Header size 0x%08x\n", 
		(u32)sizeof(ESTicket), (u32)sizeof(ESV1SectionHeader));

	// sanity check the header
	if ( cpu_to_be16(thp->sectHdrEntrySize) != sizeof(ESV1SectionHeader) ) {
		DBLOG("data structure sizes incompatible\n");
		return -1;
	}

	// allocate a temporary buffer to hold the section headers
	sect_cnt = cpu_to_be16(thp->nSectHdrs);

	if ( sect_cnt == 0 ) {
		DBLOG("No rights sections found!\n");
		return -1;
	}

	DBLOG("%d sections\n", sect_cnt);

	size = sizeof(ESV1SectionHeader) * sect_cnt;
	shp = ARRAY(ESV1SectionHeader, sect_cnt);
	if ( shp == NULL ) {
		DBLOG("Failed alloc of section headers\n");
		return -1;
	}

	// read in the section headers
	pos = sizeof(ESTicket) + cpu_to_be32(thp->sectHdrOfst);
	DBLOG("Sections at 0x%08x\n", pos);
	if ((rv = esSeek(tkt_rdr, pos)) != ES_ERR_OK) {
		DBLOG("Failed ticket seek\n");
		goto fail;
	}
	if ((rv = esRead(tkt_rdr, size, shp)) != ES_ERR_OK) {
		DBLOG("Failed read of section headers\n");
		goto fail;
	}

	rights = tkt->rights;
	// walk through the sections
	for( i = 0 ; i < sect_cnt ; i++ ) {
#ifdef EC_DEBUG
		_esh_tkt_sect_hdr_dump(&shp[i]);
#endif // EC_DEBUG
		sect_type = cpu_to_be16(shp[i].sectionType);

		DBLOG("Reading section[%d] = type %d\n", i, sect_type);

		if ( (sect_type >= EC_RIGHTS_MAX) || (sect_type == 0) ) {
			DBLOG("Skipping unknown section type %d\n", sect_type);
			continue;
		}
		// For each section, figure out how many items are present,
		rights[sect_type].cnt = cpu_to_be32(shp[i].nRecords);
		if ( rights[sect_type].cnt == 0 ) {
			DBLOG("No records in section[%d] %d\n", i, sect_type);
			continue;
		}

		// allocate a buffer and cause it to be filled.
		size = rights[sect_type].cnt * rights_size[sect_type];
		rights[sect_type].data = malloc(size);
		memset(rights[sect_type].data, 0, size);

		// read it into our buffer.
		pos = sizeof(ESTicket) + cpu_to_be32(shp[i].sectOfst);
		if ((rv = esSeek(tkt_rdr, pos)) != ES_ERR_OK) {
			DBLOG("Failed seek of sect[%d] - pos 0x%08x\n", i, pos);
			goto fail;
		}
		rv = esRead(tkt_rdr, size, rights[sect_type].data);
		if ( rv != ES_ERR_OK) {
			DBLOG("Failed read of section[%d]\n", i);
			goto fail;
		}
	}

fail:
	if ( shp ) {
		free(shp);
	}

	return rv;
}

int
esc_ticket_import(esc_ticket_t *t)
{
	s32 svc_err = ESC_ERR_UNSPECIFIED;
	void *cert_map[ESC_CERT_FRAME_MAX];
	u8 *tkt_buf = NULL;
	bool cert_is_mapped = false;
	ec_ticket_t *tkt = NULL;
	IInputStream *tkt_rdr = NULL;
	IOutputStream *tkt_wrtr = NULL;
	ESTicketWriter etw;
	bool key_is_created = false;
	ESError ee;
	ESTicket *etkt;
	IOSCObjectSubType keySubType = IOSC_ENC_SUBTYPE;
	ESGVMTicketPlatformData *pd;
#ifdef ESC_CHAL_SUPPORT
	hc_chal *chl = NULL;
#endif // ESC_CHAL_SUPPORT
	IOSCSecretKeyHandle wrap_hndl;
	u8 key_buf[sizeof(IOSCAesKey)];
	IOSCHmacKey bare_key;
	IOSCError ie;

	if (!esc_initialized) {
		t->error = ESC_ERR_NOT_YET_INIT;
		return -1;
	}

	// don't even try if there's no room in the cache.
	if ( CACHE_IS_FULL(&__tkt_cache) ) {
		DBLOG("__ticket_cache full\n");
		svc_err = ESC_ERR_NOMEM;
		goto end;
	}

	// The use of title IDs in the request has morphed over time.
	// At this time they're optional and advisory. Mainly meant to aid
	// debug. If they're present, they'll be checked. If not present, 
	// they'll be ignored.

	// a couple sanity checks. See if this is a user ticker
	// or a title ticket
	if ( t->is_title && t->is_user ) {
		DBLOG("Request indicates both user and title ticket\n");
		svc_err = ESC_ERR_PARMS;
		goto end;
	}

	if ( !t->is_title && !t->is_user ) {
		DBLOG("Ticket represents neither title nor user\n");
		svc_err = ESC_ERR_PARMS;
		goto end;
	}

	// If this references a challenge, pull it up.
	if ( t->chal_handle ) {
#ifdef ESC_CHAL_SUPPORT
		if ( (err = esc_chal_find(t->chal_handle, &chl)) ) {
			DBLOG("Challenge 0x%08x unknown\n", t->chal_handle);
			svc_err = err;
			goto end;
		}

		wrap_hndl = chl->hc_rc_key;
#else
		DBLOG("Challenge handle non-zero - non supported\n");
		svc_err = ESC_ERR_PARMS;
		goto end;
#endif // ! ESC_CHAL_SUPPORT
	}

	// map in the certs
	if (pfns_map(cert_map, t->cert_frames, t->cert_count)) {
		DBLOG("cert map failed\n");
		svc_err = ESC_ERR_PARMS;
		goto end;
	}
	cert_is_mapped = true;

	// create a record of this ticket
	tkt = ALLOC(ec_ticket_t);
	if ( tkt == NULL ) {
		DBLOG("ticket alloc failed\n");
		svc_err = ESC_ERR_NOMEM;
		goto end;
	}

	if (iStoragePfnInit(&tkt_rdr, t->ticket_frames, t->ticket_size)) {
		DBLOG("tkt_rdr init failed\n");
		goto end;
	}

	if ( iStorageNullInit(&tkt_wrtr, t->ticket_size) ) {
		DBLOG("Failed mem init - wrtr\n");
		svc_err = ESC_ERR_NOMEM;
		goto end;
	}
	etw.ticketDest = tkt_wrtr;
	etw.isImport = true;

	tkt_buf = malloc(TKT_BUF_SIZE);
	if (tkt_buf == NULL) {
		DBLOG("tik_buf alloc failed\n");
		svc_err = ESC_ERR_NOMEM;
		goto end;
	}

	// Verify/depersonalize the ticket
    	ee = esVerifyTicket(tkt_rdr, (const void **)cert_map, t->cert_count,
			     true, &etw, NULL, tkt_buf);
    	if ( ee != ES_ERR_OK) {
		DBLOG("esVerifyTicket failed - %d\n", ee);
		svc_err = ESC_ERR_TKT_VERIFY;
		goto end;
	}

	// Grab a pointer to the v0 ticket
	etkt = (ESTicket *)tkt_buf;
	pd = (ESGVMTicketPlatformData *)etkt->customData;
	pd->type = cpu_to_be16(pd->type);

	DBLOG("Ticket Key ID %d\n", etkt->keyId);

	// We can't handle personalized tickets before credentials have
	// been loaded.
	if ( etkt->deviceId && !esc_cred_loaded ) {
		DBLOG("personalized ticket without credentials loaded");
		svc_err = ESC_ERR_NOT_YET_INIT;
		goto end;
	}

	// Verify the device ID matches that of the device
	if ( etkt->deviceId && (cpu_to_be32(etkt->deviceId) != device_id) ) {
		DBLOG("Unexpected device id in ticket - 0x%08x\n",
			cpu_to_be32(etkt->deviceId));
		svc_err = ESC_ERR_TKT_DEVICEID;
		goto end;
	}

	DBLOG("Device ID 0x%08x\n", cpu_to_be32(etkt->deviceId));

	// determine the type of ticket
	switch (etkt->keyId) {
#if defined(GVM_FEAT_PLAT_CMN_KEY)
	case ESGVM_KEYID_DEVELOPMENT:
	case ESGVM_KEYID_PLATFORM_APC:
#if defined(GVM_FEAT_PCK_SYS)
		// We have one variant (DEVEL) where we will only allow system
		// titles to be decoded with the platform common key.
		if ( (tkt->title_id >> 32) != IGWARE_PART_CHAN_ID ) {
			DBLOG("Non system title using platform key\n");
			svc_err = ESC_ERR_TKT_KEYID;
			goto end;
		}
#endif // defined(GVM_FEAT_PCK_SYS)
		crypto_impl_platkey_get(&wrap_hndl);
		break;
#endif // defined(GVM_FEAT_PLAT_CMN_KEY)
	case ESGVM_KEYID_OFFLINE:
		wrap_hndl = IOSC_COMMON_ENC_HANDLE;
		break;
#ifdef ESC_CHAL_SUPPORT
	case ESGVM_KEYID_ONLINE:
		if ( !t->chal_handle ) {
			DBLOG("No challenge specified for online ticket\n");
			svc_err = ESC_ERR_TKT_KEYID;
			goto end;
		}
		wrap_hndl = chl->hc_rc_key;
		break;
#endif // ESC_CHAL_SUPPORT
	default:
		DBLOG("Invalid KeyID - %d\n", etkt->keyId);
		svc_err = ESC_ERR_TKT_KEYID;
		goto end;
	}

	// Another policy check
	if ( t->chal_handle && (etkt->keyId != ESGVM_KEYID_ONLINE) ) {
		DBLOG("Online ticket expected - received %d\n", etkt->keyId);
		svc_err = ESC_ERR_TKT_KEYID;
		goto end;
	}

	// If this is a title ID perform a title check
	// otherwise verify the user id.
	if ( t->is_title ) {
		if ( pd->type & ES_GVM_TICKET_TYPE_USER ) {
			DBLOG("Wrong ticket type received - got user\n");
			svc_err = ESC_ERR_TKT_TYPE;
			goto end;
		}
    		tkt->title_id = cpu_to_be64(etkt->titleId);
		DBLOG("Ticket Title ID "FMT0x64"\n", tkt->title_id);

		if (t->title_id && (t->title_id != tkt->title_id) ) {
			DBLOG("title id match failed\n");
			svc_err = ESC_ERR_TITLE_MATCH;
			goto end;
		}
	}
	else {
		tkt->is_user_key = true;

		if ( !(pd->type & ES_GVM_TICKET_TYPE_USER) ) {
			DBLOG("Wrong ticket type received - not user 0x%08x\n",
				pd->type);
			svc_err = ESC_ERR_TKT_TYPE;
			goto end;
		}
    		tkt->user_id = cpu_to_be64(pd->userId);
		if ( t->user_id && (tkt->user_id != t->user_id) ) {
			DBLOG("User ID match: wanted "FMT0x64" got "FMT0x64"\n",
			      t->user_id, pd->userId);
			svc_err = ESC_ERR_USER_ID;
			goto end;
		}

		// If this is a signature ticket than we need to
		// change the keySubType. Otherwise the key can't be
		// used to for HMAC SHA1.
		if ( pd->type & ES_GVM_TICKET_TYPE_SIGNATURE ) {
			tkt->is_user_sign = true;
			keySubType = IOSC_MAC_SUBTYPE;
		}
		DBLOG("%s User ticket for "FMT0x64"\n",
		      (tkt->is_user_sign) ? "Sig" : "Data", pd->userId);

		DBLOG("Ticket user ID "FMT0x64"\n", tkt->user_id);
	}

	// pull out the ticket rights
	if ( _esh_rights_get(tkt, tkt_rdr, tkt_buf) ) {
		DBLOG("Bad V1 style ticket received.\n");			
		svc_err = ESC_ERR_TKT_RIGHTS;
		goto end;
	}

	// Decrypt the title key
	if ( IOSC_CreateObject(&tkt->key, IOSC_SECRETKEY_TYPE, keySubType)
	     != IOSC_ERROR_OK) {
		DBLOG("secret key object create failed\n");
		svc_err = ESC_ERR_NOMEM;
		goto end;
	}
	key_is_created = true;

	memset(__iv_buf, 0, sizeof(IOSCAesIv));
	memcpy(__iv_buf, (u8 *) &etkt->titleId, sizeof(ESTitleId));
	memcpy(key_buf, etkt->titleKey, sizeof(IOSCAesKey));

	// If this is a user signature then ...
	if ( tkt->is_user_sign ) {
		// The AES key is 16 bytes. The signature key is hmac sha1
		// and that's 20 bytes. The eTicket only has room for 16
		// bytes in its header. So, we unwrap the key, pad the last
		// 4 bytes with 0's and use that. Yup, it's ugly.
		memset(bare_key, 0, sizeof(bare_key));
		if ( BSL_Decrypt(wrap_hndl, __iv_buf, key_buf, sizeof(key_buf),
				 bare_key) ) {
			DBLOG("Failed unwrapping user sign key\n");
			goto end;
		}

		dbg_bytes_dump("user sig key", bare_key, sizeof(bare_key));

		ie = IOSC_ImportSecretKey(tkt->key, 0, 0, IOSC_NOSIGN_NOENC,
					  NULL, NULL, bare_key);
		memset(bare_key, 0, sizeof(bare_key));
	}
	else {
		ie = IOSC_ImportSecretKey(tkt->key, 0, wrap_hndl,
					  IOSC_NOSIGN_ENC, NULL, __iv_buf,
					  key_buf);
	}

	if ( ie != IOSC_ERROR_OK) {
		DBLOG("secret key import failed\n");
		goto end;
	}

	// Add this ticket to the cache
	if ( cache_add(&__tkt_cache, tkt, &tkt->handle) ) {
		svc_err = ESC_ERR_NOMEM;
		DBLOG("Ticket cache unexpectedly full\n");
		goto end;
	}

	DBLOG("ticket_handle 0x%08x\n", tkt->handle);

	// copy out the handle
	t->handle = tkt->handle;
	svc_err = ESC_ERR_OK;

end:
	if ( tkt_wrtr ) {
		iStorageNullFree(&tkt_wrtr);
	}

	if (tkt_rdr) {
		iStoragePfnFree(&tkt_rdr);
	}

	// Unmap the certs
	if (cert_is_mapped) {
		pfns_unmap(t->cert_frames, t->cert_count);
	}

	// clean up from failure
	if ( svc_err ) {
		if ( key_is_created ) {
			(void)IOSC_DeleteObject(tkt->key);
		}
		if ( tkt ) {
			_esh_ticket_rights_free(tkt);
			free(tkt);
		}
	}

	t->error = svc_err;

	if (tkt_buf)
		free(tkt_buf);

	return svc_err ? -1 : 0;
}

int
esc_ticket_release(esc_resource_rel *req)
{
	if (!esc_initialized) {
		req->error = ESC_ERR_NOT_YET_INIT;
		return -1;
	}

	req->error = _esh_ticket_release(req->handle);

	return req->error ? -1 : 0;
}

int
esc_ticket_query(esc_ticket_query_t *q)
{
	ec_cache_node_t *cn;
	ec_ticket_t *tkt;
	u32 i;

	if (!esc_initialized) {
		q->error = ESC_ERR_NOT_YET_INIT;
		return -1;
	}

	// Walk thru the list of imported contents. A handle of ANY matches
	// any imported title.
	i = 0;
	for( cn = __tkt_cache.ch_active ; cn ; cn = cn->cn_next ) {
		tkt = (ec_ticket_t *)cn->cn_data;

		// are we interested in this ticket?
		if ( q->title_id &&
		     (q->title_id != tkt->title_id) ) {
			continue;
		}

		// are we interested in this cookie
		if ( q->cookie != (tkt->cookie & q->mask) ) {
			continue;
		}

		q->recs[i].title_id = tkt->title_id;
		q->recs[i].uid = tkt->user_id;
		q->recs[i].ticket_handle = tkt->handle;
		q->recs[i].cookie = tkt->cookie;
		q->recs[i].ref_cnt = tkt->ref_cnt;
		q->recs[i].state = 0;
		if ( tkt->user_id ) {
			q->recs[i].state |= ESC_TKTST_USER;
		}
		i++;
	}

	q->rec_cnt = i;
	q->error = ESC_ERR_OK;

	return 0;
}

// split the tmd + cert blob into a tmd and list of certs
static int
_tmd_split(ec_title *title)
{
	ESV1TitleMeta *mp;
	u8 *cert_list;
	u32 cert_len;
	u32 cert_lens[ESC_CERT_FRAME_MAX];
	ESError ee;

	// Pull out the size of the TMD.
	mp = title->tmd_data;
	title->tmd_len = sizeof(ESV1TitleMeta) +
		(sizeof(ESV1ContentMeta) * cpu_to_be16(mp->head.numContents));
	
	cert_len = title->tmd_data_len - title->tmd_len;

	// Pull out the start of the certs
	cert_list = title->tmd_data;
	cert_list += title->tmd_len;

	// Pull out the certificates
	title->tmd_cert_cnt = ESC_CERT_FRAME_MAX;
	if ( (ee = ESI_ParseCertList(cert_list, cert_len,
				     (void **)&title->tmd_certs,
				     cert_lens, &title->tmd_cert_cnt)) ) {
		DBLOG("ESI_ParseCertList for certs failed - %d\n", ee);
		return -1;
	}

	if ( title->tmd_cert_cnt < 2 ) {
		DBLOG("Too few certs found - %d\n", title->tmd_cert_cnt);
		return -1;
	}

	return 0;
}

static int
_esh_data_title_access_chk(ec_title *title)
{
	ec_title *parent;
	hc_right *right;
	ESV1AccessTitleRecord *ar;
	ec_cache_node_t *cn;
	u32 i;

	// this is a data title with slight different rules
	// then game titles.
	title->is_data_title = true;

	// check the ticket for access rights.
	right = &title->ticket->rights[ES_ITEM_RIGHT_ACCESS_TITLE];
	if ( right->data == NULL ) {
		DBLOG("Title ticket had no title access rights\n");
		return -1;
	}

	// We "currently" don't know exactly which title is requesting this
	// data title. We could "guess" but that's a bad hack. I'd rather we
	// check to see if any mounted title has the necessary rights.
	ar = (ESV1AccessTitleRecord *)right->data;
	for( cn = __ttl_cache.ch_active ; cn ; cn = cn->cn_next ) {
		parent = cn->cn_data;
		// Walk through the table of access rights
		for( i = 0 ; i < right->cnt ; i++ ) {
			DBLOG("mask "FMT0x64" - id "FMT0x64"\n",
			      cpu_to_be64(ar[i].accessTitleMask),
			      cpu_to_be64(ar[i].accessTitleId));
			if ( (parent->title_id & 
				~cpu_to_be64(ar[i].accessTitleMask)) !=
			     cpu_to_be64(ar[i].accessTitleId) ) {
				continue;
			}

			// We found a title with access rights.
			goto found;
		}
	}

	DBLOG("Didn't find a title with access\n");
	return -1;

found:
	DBLOG("Title "FMT0x64" has access to "FMT0x64"\n", parent->title_id,
	      title->title_id);

	// link this into the parents list of data titles
	title->data_link = parent->data_link;
	parent->data_link = title;
	title->parent_link = parent;
	title->data_is_referenced = true;
	return 0;
}

static s32
_esh_hmac_sha1_gen(ec_ticket_t *sign, u8 *buf, u32 buf_len, IOSCSigRsa2048 *sig)
{
	IOSCHashContext context;
	IOSCError error = IOSC_ERROR_OK;

	memset(sig, 0, sizeof(*sig));
	error = IOSC_GenerateBlockMAC(context, 0, 0, 0, 0, sign->key,
				      IOSC_MAC_FIRST, sig->sig);
	if ( error ) {
		DBLOG("Failed IOSC_MAC_FIRST - %d\n", error);
		return -1;
	}
	error = IOSC_GenerateBlockMAC(context, buf, buf_len, 0, 0, sign->key,
				      IOSC_MAC_LAST, sig->sig);
	if ( error ) {
		DBLOG("Failed IOSC_MAC_FIRST\n");
		return -1;
	}
	sig->sigType = IOSC_SIG_HMAC_SHA1;

	dbg_bytes_dump("Signature:", sig->sig, _GSS_TMD_SIG_LEN);

	return 0;
}

typedef struct {
	IOSCSigRsa2048 sig;
} _esh_gss_tmd_verify_big_locals;

static s32
_esh_gss_tmd_verify(ec_title *title, ec_ticket_t *tkt_sig)
{
	u8 *tmd_data;
	ESV1TitleMeta *mp;
	s32 reterr = 0;

	_esh_gss_tmd_verify_big_locals *big_locals;
#ifdef __KERNEL__
	big_locals = kmalloc(sizeof(*big_locals), GFP_KERNEL);
	if (!big_locals)
		return -1;
#else
	_esh_gss_tmd_verify_big_locals _big_locals;
	big_locals = &_big_locals;
#endif

	// generate the signature
	tmd_data = title->tmd_data;
	if (_esh_hmac_sha1_gen(tkt_sig, &tmd_data[_GSS_TMD_SIG_BYTE_START],
			       _GSS_TMD_SIG_DATA_LEN, &big_locals->sig) ) {
		DBLOG("Failed generating signature on TMD\n");
		reterr = -1;
		goto end;
	}

	// compare the sigs
	mp = (ESV1TitleMeta *)title->tmd_data;
	if ( memcmp(big_locals->sig.sig, mp->sig.sig, _GSS_TMD_SIG_LEN) ) {
		DBLOG("Signature check failed\n");
		reterr = -1;
		goto end;
	}
 end:
#ifdef __KERNEL__
	kfree(big_locals);
#endif
	return reterr;
}

//
// exported to the challenge support code
// This should probably removed and force challenge code to use
// the gi routine.
//
s32
eshi_title_import(u64 tid, u32 cookie, u8 **tmd_datap, u32 tmd_len,
		  ec_ticket_t *tkt_sig, ec_ticket_t *tkt_data, ec_title **titlep)
{
	s32 svc_err = ESC_ERR_UNSPECIFIED;
	ec_title *title = NULL;
	ESV1TitleMeta *mp;
	ESTitleType tt;
	ESGVMTmdPlatformData *pd;

	DBLOG("Importing title "FMT0x64" cookie 0x%08x tkt 0x%08x\n",
	      tid, cookie, tkt_data ? tkt_data->handle : 0);

	// allocate a title structure to hold this import
	title = ALLOC(ec_title);
	if ( title == NULL ) {
		DBLOG("Title alloc failed\n");
		svc_err = ESC_ERR_NOMEM;
		goto end;
	}

	title->cookie = cookie;
	title->ticket = tkt_data;
	title->tmd_data_len = tmd_len;
	// At this point tmd_data is no longer owned by the caller.
	title->tmd_data = *tmd_datap;
	*tmd_datap = NULL;
	title->dec_streams.next = title->dec_streams.last = &title->dec_streams;

	// if this is a data title make sure 
	// Make sure this is a GVM title
	mp = title->tmd_data;
	tt = cpu_to_be32(mp->head.type);
	pd = (ESGVMTmdPlatformData *)&mp->head.customData;

	// Make sure this is a GVM Title
	if ( !(tt & ES_TITLE_TYPE_GVM_TITLE) ) {
		DBLOG("Title is not a GVM Title - 0x%08x\n", tt);
		svc_err = ESC_ERR_TITLE_GVM;
		goto end;
	}

	if ( (tt & ES_TITLE_TYPE_DATA) &&
	     _esh_data_title_access_chk(title) ) {
		DBLOG("Data title failed access check\n");
		svc_err = ESC_ERR_NO_RIGHT;
		goto end;
	}

	if ( cpu_to_be16(pd->flags) & ES_GVM_TMD_FLAG_IS_GSS ) {
		title->is_gss = true;

		// GSS must have a sign ticket in order to verify the TMD
		if ( tkt_sig == NULL ) {
			DBLOG("No signature ticket specified for GSS TMD\n");
			svc_err = ESC_ERR_NO_RIGHT;
			goto end;
		}

		if ( !tkt_sig->is_user_sign ) {
			DBLOG("Ticket specified is not a user signature\n");
			svc_err = ESC_ERR_NO_RIGHT;
			goto end;
		}
	}
#ifdef EC_DEBUG
	if ( title->is_gss ) {
		DBLOG("Title is a gss\n");
	}
	else if (title->is_data_title) {
		DBLOG("Title is a data title\n");
	}
	else {
		DBLOG("Title is a game title\n");
	}
#endif // EC_DEBUG

	// pull out the GSS Size
	if ( !title->is_data_title ) {
		if ( pd->gssSize ) {
			title->gss_size = cpu_to_be16(pd->gssSize);
		}
		else {
			DBLOG("*** GSS size is hardcoded\n");
			title->gss_size = _GSS_DFLT_SIZE;
		}
		DBLOG("gss size is %d MiB\n", title->gss_size);
	}

	// now create our TMD reader
	if ( iStorageMemInit(&title->tmd_rdr, title->tmd_data,
			     tmd_len) ) {
		DBLOG("TMD reader init failed\n");
		svc_err = ESC_ERR_NOMEM;
		goto end;
	}

	// The TMD for a game/data title  is actually a blob composed of the 
	// TMD + certs. This needs to be split up for the verification.
	// However, the GSS TMD is not a blob. There are no certs
	// associated with its signature. Don't split it up.
	if ( !title->is_gss && _tmd_split(title) ) {
		DBLOG("TMD Split failed\n");
		svc_err = ESC_ERR_TMD_VERIFY;
		goto end;
	}

	DBLOG("TMD has %d certs\n", title->tmd_cert_cnt);

	// GSS TMD isn't signed, yet.
	if ( title->is_gss ) {
		memcpy(&title->title_meta, mp, sizeof(*mp));
		if ( _esh_gss_tmd_verify(title, tkt_sig) ) {
			DBLOG("GSS TMD verify failed\n");
			svc_err = ESC_ERR_TMD_VERIFY;
			goto end;
		}
	}
	else if ( esVerifyTmd(title->tmd_rdr,
		         (const void **)title->tmd_certs,
			 title->tmd_cert_cnt, 0,
			 &title->title_meta, NULL) != ES_ERR_OK ) {
		DBLOG("TMD verify failed\n");
		svc_err = ESC_ERR_TMD_VERIFY;
		goto end;
	}

	// Make sure title ids match
 	title->title_id = cpu_to_be64(title->title_meta.head.titleId);
	if ( tid && (title->title_id != tid) ) {
		DBLOG("TMD title ids didn't match\n");
		svc_err = ESC_ERR_TITLE_MATCH;
		goto end;
	}

	DBLOG("Title ID = "FMT0x64"\n", title->title_id);

	// GSS versus Titles
	if ( title->is_gss ) {
		// This better be a user ticket
		if ( !tkt_data->is_user_key ) {
			DBLOG("GSS passed a non-user ticket\n");
			svc_err = ESC_ERR_TKT_RIGHTS;
			goto end;
		}

		// Ticket user ID must match GSS TMD user ID.
		title->user_id = cpu_to_be64(pd->userId);
		if ( tkt_data->user_id != title->user_id ) {
			DBLOG("uid mismatch tmd "FMT0x64" tkt "FMT0x64"\n",
			      title->user_id, tkt_data->user_id);
			svc_err = ESC_ERR_USER_ID;
			goto end;
		}
		DBLOG("GSS user id "FMT0x64"\n", title->user_id);

	}
	else if ( tkt_data ) {
		// Make sure the ticket type is correct.
		if ( tkt_data->title_id == 0 ) {
			DBLOG("Title/Data TMD passed a user ticket\n");
			svc_err = ESC_ERR_TKT_RIGHTS;
			goto end;
		}
		// ticket title id needs to match tmd id
		if ( title->title_id != title->ticket->title_id ) {
			DBLOG("ttl id "FMT0x64" - tkt ttl id "FMT0x64"\n",
			      title->title_id,
			      title->ticket->title_id);
			svc_err = ESC_ERR_TITLE_MATCH;
			goto end;
		}
	}

	if ( cache_add(&__ttl_cache, title, &title->handle) ) {
		DBLOG("ttl cache unexpectedly full?\n");
		svc_err = ESC_ERR_NOMEM;
		goto end;
	}

	DBLOG("title_handle 0x%08x\n", title->handle);

	if ( title->ticket ) {
		title->ticket->ref_cnt++;
	}

	*titlep = title;

	// done, kind of...
	svc_err = ESC_ERR_OK;
end:
	// clean up from errors
	if ( svc_err ) {
		if (title) {
			if ( title->tmd_rdr ) {
				iStorageMemFree(&title->tmd_rdr);
			}
			if ( title->tmd_data ) {
				free(title->tmd_data);
			}
			free(title);
		}
	}

	return svc_err;
}

s32
eshi_tmd_load(u32 *frames, u32 tmd_len, u8 **tmd_datap)
{
	s32 err = 0;
	IInputStream *tmd_rdr;
	u8 *tmd_data = NULL;
	bool tmd_rdr_is_setup = false;
	u32 xfer_cnt;

	// allocate a buffer for the TMD
	tmd_data = malloc(tmd_len);
	if ( tmd_data == NULL ) {
		DBLOG("TMD data alloc failed\n");
		err = ESC_ERR_NOMEM;
		goto end;
	}

	// set up the storage reader for the tmd
	if (iStoragePfnInit(&tmd_rdr, frames, tmd_len)) {
		DBLOG("TMD GVM reader init failed\n");
		err = ESC_ERR_BUF_ACCESS;
		goto end;
	}
	tmd_rdr_is_setup = true;

	// copy the data into our storage
	if ( rdrTryRead(tmd_rdr, &xfer_cnt, tmd_data, tmd_len)
		|| (xfer_cnt != tmd_len)) {
		DBLOG("TMD GVM read failed\n");
		err = ESC_ERR_BUF_ACCESS;
		goto end;
	}

	*tmd_datap =  tmd_data;
	tmd_data = NULL;

end:
	// general cleanup from here on.
	if ( tmd_rdr_is_setup ) {
		iStoragePfnFree(&tmd_rdr);
	}

	// clean up from errors
	if ( tmd_data ) {
		free(tmd_data);
	}

	return err;
}

int
esc_title_import(esc_title_t *t)
{
	s32 svc_err = ESC_ERR_UNSPECIFIED;
	ec_title *title = NULL;
	ec_ticket_t *tkt_data = NULL;
	ec_ticket_t *tkt_sig = NULL;
	u8 *tmd_data = NULL;

	if (!esc_initialized) {
		t->error = ESC_ERR_NOT_YET_INIT;
		return -1;
	}

	// see if there's room
	if ( CACHE_IS_FULL(&__ttl_cache) ) {
		DBLOG("__title_cache full\n");
		svc_err = ESC_ERR_NOMEM;
		goto end;
	}

	// sanity check
	if ( _NUM_PAGES(t->tmd_size) > ESC_TMD_FRAME_MAX ) {
		DBLOG("TMD size invalid!\n");
		svc_err = ESC_ERR_PARMS;
		goto end;
	}

	DBLOG("Importing title "FMT0x64" cookie 0x%08x tkts 0x%08x:0x%08x\n",
	      t->title_id, t->cookie, t->ticket_sig_handle,
	      t->ticket_data_handle);

	// Look up the tickets via the handle.
	if ( t->ticket_sig_handle ) {
		svc_err = cache_find(&__tkt_cache, t->ticket_sig_handle,
				     (void **)&tkt_sig);
		if ( svc_err ) {
			DBLOG("ticket 0x%08x find failed\n",
			      t->ticket_sig_handle);
			goto end;
		}
	}

	svc_err = cache_find(&__tkt_cache, t->ticket_data_handle,
			     (void **)&tkt_data);
	if ( svc_err ) {
		DBLOG("ticket 0x%08x find failed\n", t->ticket_data_handle);
		goto end;
	}

	svc_err = eshi_tmd_load(t->tmd_frames, t->tmd_size, &tmd_data);
	if ( svc_err ) {
		DBLOG("Failed TMD load\n");
		goto end;
	}

	svc_err = eshi_title_import(t->title_id, t->cookie, &tmd_data,
				    t->tmd_size, tkt_sig, tkt_data, &title);
	if ( svc_err ) {
		DBLOG("Failed title import\n");
		goto end;
	}

	t->title_handle = title->handle;

	t->error = ESC_ERR_OK;
end:
	// clean up from errors
	if ( tmd_data ) {
		free(tmd_data);
	}

	t->error = svc_err;
	
	return svc_err ? -1 : 0;
}

static int
_esh_hash_gen(void *buf, u32 len, IOSCHash256 hash)
{
	IOSCHashContext hash_ctx;
	int rv = -1;

	// Initialize the hash context
	if ( IOSC_GenerateHash(hash_ctx, NULL, 0, IOSC_SHA256_INIT, NULL)
			!= IOSC_ERROR_OK ) {
		DBLOG("hash init failed\n");
		goto end;
	}

	// Generate the final hash
	if ( IOSC_GenerateHash(hash_ctx, buf, len, IOSC_SHA256_FINAL, hash)
			!= IOSC_ERROR_OK ) {
		DBLOG("hash final failed\n");
		goto end;
	}

	rv = 0;
end:
	return rv;
}

static s32
_esh_tmd_sign(ec_ticket_t *sign, u8 *tmd_data, u32 tmd_len, u8 *cfm_data,
	      u32 cfm_len)
{
	ESV1TitleMeta *mp;
	ESV1ContentMeta *cmp;
	IOSCHash256 hash;

	mp = (ESV1TitleMeta *)tmd_data;
	cmp = (ESV1ContentMeta *)(&mp[1]);

	// compute the hash of the CFM
	if ( _esh_hash_gen(cfm_data, cfm_len, hash) ) {
		DBLOG("Failed computing hash of the CFM\n");
		return -1;
	}
	memcpy(&cmp->hash, &hash, sizeof(hash));

	// Update the content meta-data
	if ( _esh_hash_gen(cmp, sizeof(*cmp), hash) ) {
		DBLOG("Failed computing hash of the group hash\n");
		return -1;
	}
	memcpy(&mp->v1Head.cmdGroups[0].groupHash, &hash, sizeof(hash));

	// Update the V1 TMD header
	if ( _esh_hash_gen(mp->v1Head.cmdGroups, sizeof(mp->v1Head.cmdGroups),
			   hash) ) {
		DBLOG("Failed computing hash of the v1 tmd header\n");
		return -1;
	}
	memcpy(&mp->v1Head.hash, &hash, sizeof(hash));

	// Sign the TMD
	if (_esh_hmac_sha1_gen(sign, &tmd_data[_GSS_TMD_SIG_BYTE_START],
			       _GSS_TMD_SIG_DATA_LEN, &mp->sig) ) {
		DBLOG("Failed generating signature on TMD\n");
		return -1;
	}

	return 0;
}

int
esc_title_export(esc_title_export_t *req)
{
	s32 svc_err = ESC_ERR_UNSPECIFIED;
	ec_title *title = NULL;
	IOutputStream *tmd_wrtr = NULL;
	u32 xfer_cnt;
	ec_ticket_t *tkt_sig;
	ec_cfm *cfm;
	bool just_size;

	if (!esc_initialized) {
		req->error = ESC_ERR_NOT_YET_INIT;
		return -1;
	}

	// Look up the content.
	if ( (svc_err = cache_find(&__ttl_cache, req->title_handle,
				   (void **)&title)) ) {
		goto end;
	}

	// Make sure this is an exportable TMD
	if ( !title->is_gss ) {
		DBLOG("TMD is not available for export\n");
		svc_err = ESC_ERR_READONLY;
		goto end;
	}

	// See if they're just requesting the size
	just_size = (req->tmd_size == 0);
	req->tmd_size = title->tmd_data_len;
	if ( just_size ) {
		svc_err = ESC_ERR_OK;
		goto end;
	}

	// Make sure the buffer is the correct size
	if ( req->tmd_size != title->tmd_data_len ) {
		DBLOG("Buffer size mismatch\n");
		svc_err = ESC_ERR_SIZE_MISMATCH;
		goto end;
	}

	// look up the ticket
	if ( (svc_err = cache_find(&__tkt_cache, req->ticket_handle,
				   (void **)&tkt_sig)) ) {
		goto end;
	}

	// The ticket must represent a user ticket
	if ( !tkt_sig->is_user_key ) {
		DBLOG("passed a non-user ticket for GSS export - 0x%08x\n",
			req->ticket_handle);
		svc_err = ESC_ERR_TKT_RIGHTS;
		goto end;
	}

	// The user id of the ticket must match the user id of the GSS.
	if ( tkt_sig->user_id != title->user_id ) {
		DBLOG("uid mismatch tmd "FMT0x64" tkt "FMT0x64"\n",
		      title->user_id, tkt_sig->user_id);
		svc_err = ESC_ERR_USER_ID;
		goto end;
	}

	// Make sure this title has a content
	if ( (cfm = title->cfm_list) == NULL ) {
		DBLOG("TMD has no content!\n");
		svc_err = ESC_ERR_NO_CONTENT;
		goto end;
	}

#ifdef EC_DEBUG
	if ( cfm->is_dirty ) {
		DBLOG("Title exported with a dirty CFM.\n");
	}
#endif // EC_DEBUG

	// Sign the TMD
	svc_err = _esh_tmd_sign(tkt_sig, title->tmd_data,
				title->tmd_data_len, cfm->cfm_data,
				cfm->cfm_len);
	if ( svc_err ) {
		DBLOG("Failed signing the TMD\n");
		goto end;
	}

	// create a storage writer for this thing
	if ( iStoragePfnInit(&tmd_wrtr, req->tmd_frames,
			     req->tmd_size) ) {
		svc_err = ESC_ERR_BUF_ACCESS;
		DBLOG("TMD writer init failed\n");
		goto end;
	}

	// copy this thing out.
	if ( wrtrTryWrite(tmd_wrtr, &xfer_cnt, title->tmd_data,
		          req->tmd_size) || (xfer_cnt != req->tmd_size)) {
		svc_err = ESC_ERR_BUF_ACCESS;
		DBLOG("TMD write failed\n");
		goto end;
	}

	// See if we can clear the dirty bit.
	title->is_dirty = cfm->is_dirty;

	svc_err = ESC_ERR_OK;

end:
	if ( tmd_wrtr ) {
		iStoragePfnFree(&tmd_wrtr);
	}

	req->error = svc_err;

	return (svc_err) ? -1 : 0;
}

int
esc_title_release(esc_resource_rel *rel)
{
	if (!esc_initialized) {
		rel->error = ESC_ERR_NOT_YET_INIT;
		return -1;
	}

	rel->error = eshi_title_release(rel->handle);

	return (rel->error) ? -1 : 0;
}

int
esc_title_query(esc_title_query_t *q)
{
	ec_cache_node_t *cn;
	ec_title *title;
	u32 i;

	if (!esc_initialized) {
		q->error = ESC_ERR_NOT_YET_INIT;
		return -1;
	}

	// Walk thru the list of imported titles. A cookie of 0 matches
	// any imported title.
	i = 0;
	for( cn = __ttl_cache.ch_active; cn ; cn = cn->cn_next ) {
		title = (ec_title *)cn->cn_data;
		if ( q->title_id && (q->title_id != title->title_id) ) {
			continue;
		}
		// are we interested in this title?
		if ( q->cookie != (title->cookie & q->mask) ) {
			continue;
		}

		q->recs[i].title_id = title->title_id;
		q->recs[i].cookie = title->cookie;
		q->recs[i].title_handle = title->handle;
		// Place holder for read/write, dirty state.
		q->recs[i].state = 0;
		if ( title->is_gss ) {
			q->recs[i].state |= ESC_TTLST_GSS;
		}
		else if ( title->is_data_title ) {
			q->recs[i].state |= ESC_TTLST_DATA;
		}
		else {
			q->recs[i].state |= ESC_TTLST_GAME;
		}
		if ( title->is_dirty ) {
			q->recs[i].state |= ESC_TTLST_DIRTY;
		}
		q->recs[i].ticket_handle = title->ticket->handle;
		i++;
	}

	q->rec_cnt = i;
	q->error = ESC_ERR_OK;

	return 0;
}

int
esc_block_decrypt(esc_block_t *blk)

{
	ec_cfm *cfm;
	u32 i;
	void *in_map[ESC_BLOCK_PAGE_MAX];
	void *out_map[ESC_BLOCK_PAGE_MAX];
	hc_aes_iv *iv = (hc_aes_iv *)__iv_buf;
	bool in_is_mapped = false;
	bool out_is_mapped = false;
	s32 svc_err = ESC_ERR_UNSPECIFIED;
    	IOSCHash256 hash;
	bool is_zero_blk;
	u32 blk_page;
	ec_ticket_t *tkt;

	if (!esc_initialized) {
		blk->error = ESC_ERR_NOT_YET_INIT;
		return -1;
	}

	// Make sure it's from valid imported content
	if ( (svc_err = cache_find(&__cfm_cache, blk->content_handle,
				   (void **)&cfm)) ) {
		DBLOG("Invalid CFM handle 0x%08x\n", blk->content_handle);
		goto end;
	}

	// make sure data titles are still properly referenced
	if ( cfm->title->is_data_title && 
	     !cfm->title->data_is_referenced ) {
		DBLOG("data title "FMT0x64" has no valid reference\n",
		      cfm->title->title_id);
		svc_err = ESC_ERR_NO_RIGHT;
		goto end;
	}

	// sanity check 1.
	if ( blk->page_cnt > ESC_BLOCK_PAGE_MAX ) {
		DBLOG("invalid page cnt %d\n", blk->page_cnt);
		svc_err = ESC_ERR_PARMS;
		goto end;
	}

	if ( blk->block_number >= cfm->blk_cnt ) {
		DBLOG("invalid block_number 0x%08x\n", blk->block_number);
		svc_err = ESC_ERR_PARMS;
		goto end;
	}

	blk_page = blk->block_number * blk->page_cnt;

	// see if this is a zero block
	is_zero_blk = (cfm->zero_bits && 
		BIT_IS_SET(cfm->zero_bits, blk->block_number));

	// XXX pull out the content information - not necessary yet.
	// This is where we'd pull out the size of the block and determine
	// the number of pages in a block.

	// map the output buffer
	if ( pfns_map(out_map, blk->frames_out, blk->page_cnt) ) {
		DBLOG("out map failed\n");
		svc_err = ESC_ERR_NOMEM;
		goto end;
	}
	out_is_mapped = true;

	// Special case for zero blks
	if ( is_zero_blk ) {
		// DBLOG("Blk 0x%08x zero\n", blk->block_number);
		for( i = 0 ; i < blk->page_cnt ; i++ ) {
			if ( out_map[i] == NULL ) {
				continue;
			}
			memset(out_map[i], 0, VM_PAGE_SIZE);
		}
		svc_err = ESC_ERR_OK;
		goto end;
	}

	// map the input buffer
	if ( pfns_map(in_map, blk->frames_in, blk->page_cnt) ) {
		DBLOG("in map failed\n");
		svc_err = ESC_ERR_NOMEM;
		goto end;
	}
	in_is_mapped = true;

	tkt = cfm->title->ticket;

	// Set up the iv
	iv->hav_title_id = cpu_to_be64(cfm->title->title_id);
	iv->hav_content_index = cpu_to_be32(cfm->content_index);
	iv->hav_block_number = cpu_to_be32(blk->block_number);

	// decrypt the block
	for( i = 0 ; i < blk->page_cnt ; i++ ) {
		// decrypt the block
		if ( IOSC_Decrypt(tkt->key,
				  __iv_buf, in_map[i], VM_PAGE_SIZE,
				  &__codec_buf[i * VM_PAGE_SIZE])
				!= IOSC_ERROR_OK ) {
			DBLOG("Decrypt of page %d failed\n", i);
			svc_err = ESC_ERR_UNSPECIFIED;
			goto end;
		}
	}

	// We can't copy the data out until a hash check has been performed
	// on the data.
	if (_esh_hash_gen(__codec_buf, VM_PAGE_SIZE * blk->page_cnt, hash)) {
		DBLOG("hash gen failed\n");
		svc_err = ESC_ERR_BLK_HASH;
		goto end;
	}

	// Compare the hashes
	if ( memcmp(hash, cfm->hashes[blk->block_number], sizeof(hash)) 
	     != 0 ) {
		DBLOG("Hash check failed\n");
		DBLOG("title "FMT0x64" - cid 0x%08x - blk 0x%08x\n",
		      cfm->title->title_id, cfm->content_id,
		      blk->block_number);
		dbg_bytes_dump("Wanted hash:",
			       (u8 *)cfm->hashes[blk->block_number],
			       sizeof(hash));
		dbg_bytes_dump("Got hash:", (u8 *)hash, sizeof(hash));
		svc_err = ESC_ERR_BLK_HASH;
		goto end;
	}

	// copy the data out to the requestor
	for( i = 0 ; i < blk->page_cnt ; i++ ) {
		if ( out_map[i] == NULL ) {
			continue;
		}
		memcpy(out_map[i], &__codec_buf[i*VM_PAGE_SIZE], VM_PAGE_SIZE);
	}


	// success
	svc_err = ESC_ERR_OK;

end:
	if ( out_is_mapped ) {
		pfns_unmap(blk->frames_out, blk->page_cnt);
	}
	if ( in_is_mapped ) {
		pfns_unmap(blk->frames_in, blk->page_cnt);
	}

	blk->error = svc_err;
	return svc_err ? -1 : 0;
}

int
esc_block_encrypt(esc_block_t *blk)
{
	ec_cfm *cfm;
	u32 i, j;
	void *in_map[ESC_BLOCK_PAGE_MAX];
	void *out_map[ESC_BLOCK_PAGE_MAX];
	hc_aes_iv *iv = (hc_aes_iv *)__iv_buf;
	bool in_is_mapped = false;
	bool out_is_mapped = false;
	s32 svc_err = ESC_ERR_UNSPECIFIED;
	IOSCHashContext hash_ctx;
    	IOSCHash256 hash;
	u8 *bp;

	if (!esc_initialized) {
		blk->error = ESC_ERR_NOT_YET_INIT;
		return -1;
	}

	// Note: This will only work when the the input buffer is complete.
	// In the decrypt case the output buffer can be incomplete.
	// The easiest way to ensure this is to make sure that block size
	// for this content == page_size.

	// Make sure it's from valid imported content
	if ( (svc_err = cache_find(&__cfm_cache, blk->content_handle,
				   (void **)&cfm)) ) {
		DBLOG("Invalid cfm handle 0x%08x\n", blk->content_handle);
		goto end;
	}

	// make sure this CFM is read/write
	if ( !cfm->is_rw ) {
		DBLOG("Attempt to write to read-only Title\n");
		svc_err = ESC_ERR_READONLY;
		goto end;
	}

	// make sure data titles are still properly referenced
	if ( cfm->title->is_data_title && 
	     !cfm->title->data_is_referenced ) {
		DBLOG("data title "FMT0x64" has no valid reference\n",
		      cfm->title->title_id);
		svc_err = ESC_ERR_NO_RIGHT;
		goto end;
	}

	// sanity check 1.
	if ( blk->page_cnt > ESC_BLOCK_PAGE_MAX ) {
		DBLOG("invalid page cnt %d\n", blk->page_cnt);
		svc_err = ESC_ERR_PARMS;
		goto end;
	}

	if ( blk->block_number >= cfm->blk_cnt ) {
		DBLOG("invalid block_number 0x%08x\n", blk->block_number);
		svc_err = ESC_ERR_PARMS;
		goto end;
	}

	// In order for this to work the entire buffer for the block
	// must be present for input and output
	for( i = 0 ; i < blk->page_cnt ; i++ ) {
		if ( (blk->frames_in[i] == 0) ||
		     (blk->frames_out[i] == 0) ) {
			DBLOG("incomplete block being updated\n");
			svc_err = ESC_ERR_PARMS;
			goto end;
		}
	}

	// map the input buffer
	if ( pfns_map(in_map, blk->frames_in, blk->page_cnt) ) {
		DBLOG("in map failed\n");
		svc_err = ESC_ERR_NOMEM;
		goto end;
	}
	in_is_mapped = true;

	// see if the input is all zero.
	for( i = 0 ; i < blk->page_cnt ; i++ ) {
		bp = in_map[i];
		for( j = 0 ; j < VM_PAGE_SIZE ; j++ ) {
			if ( *bp++ ) {
				goto not_zero;
			}
		}
	}
	// DBLOG("Found a zero block - skipping copy out\n");
	cfm->zero_bits[BIT_IND(blk->block_number)] |=
		BIT_BYTEMASK(blk->block_number);
	memset(cfm->hashes[blk->block_number], 0, sizeof(hash));
	goto success;

not_zero:

	// map the output buffer
	if ( pfns_map(out_map, blk->frames_out, blk->page_cnt) ) {
		DBLOG("out map failed\n");
		svc_err = ESC_ERR_NOMEM;
		goto end;
	}
	out_is_mapped = true;

	// Set up the iv
	iv->hav_title_id = cpu_to_be64(cfm->title->title_id);
	iv->hav_content_index = cpu_to_be32(cfm->content_index);
	iv->hav_block_number = cpu_to_be32(blk->block_number);

	// Initialize the hash context
	if ( IOSC_GenerateHash(hash_ctx, NULL, 0, IOSC_SHA256_INIT, NULL)
			!= IOSC_ERROR_OK ) {
		DBLOG("Hash init failed\n");
		svc_err = ESC_ERR_UNSPECIFIED;
		goto end;
	}

	// encrypt the block
	for( i = 0 ; i < blk->page_cnt ; i++ ) {
		// update the hash
		if ( IOSC_GenerateHash(hash_ctx, in_map[i], VM_PAGE_SIZE,
				       IOSC_SHA256_UPDATE, NULL)
				!= IOSC_ERROR_OK ) {
			DBLOG("Hash update %d failed\n", i);
			svc_err = ESC_ERR_UNSPECIFIED;
			goto end;
		}
		// encrypt the block
		if ( IOSC_Encrypt(cfm->title->ticket->key, __iv_buf,
				  in_map[i], VM_PAGE_SIZE,
				  &__codec_buf[i * VM_PAGE_SIZE])
				!= IOSC_ERROR_OK ) {
			DBLOG("page encrypt %d failed\n", i);
			svc_err = ESC_ERR_UNSPECIFIED;
			goto end;
		}
	}

	// Generate the final hash
	if ( IOSC_GenerateHash(hash_ctx, NULL, 0, IOSC_SHA256_FINAL, hash)
			!= IOSC_ERROR_OK ) {
		DBLOG("Hash final failed\n");
		svc_err = ESC_ERR_UNSPECIFIED;
		goto end;
	}

	// Update the hash in the CFM
	memcpy(cfm->hashes[blk->block_number], hash, sizeof(hash));

	// clear the zero bit 
	cfm->zero_bits[BIT_IND(blk->block_number)] &=
		~BIT_BYTEMASK(blk->block_number);

	// copy the data out to the requestor
	for( i = 0 ; i < blk->page_cnt ; i++ ) {
		memcpy(out_map[i], &__codec_buf[i * VM_PAGE_SIZE],
		       VM_PAGE_SIZE);
	}

success:
	cfm->is_dirty = cfm->title->is_dirty = true;
	svc_err = ESC_ERR_OK;

end:
	blk->error = svc_err;

	if ( out_is_mapped ) {
		pfns_unmap(blk->frames_out, blk->page_cnt);
	}
	if ( in_is_mapped ) {
		pfns_unmap(blk->frames_in, blk->page_cnt);
	}
	return (svc_err) ? -1 : 0;
}

static s32
_content_stream_get(ec_title *title, u32 content_id, ec_dec_stream_t **streamp)
{
	s32 svc_err = ESC_ERR_UNSPECIFIED;
	ESV1ContentMeta cm;
	ec_dec_stream_t *stream = NULL;
	u16 *cind;

	// 1st see if the stream already exists
	for( stream = title->dec_streams.next ; stream != &title->dec_streams ;
			stream = stream->next ) {
		if ( content_id != stream->content_id ) {
			continue;
		}

		*streamp = stream;
		return ESC_ERR_OK;
	}

	// Look up the content ID. Pull out its hash at the same time.
	DBLOG("Content ID is 0x%08x\n", content_id);
	if ( esVerifyTmdContent(title->tmd_rdr, content_id,
				&title->title_meta, &cm) != ES_ERR_OK) {
		DBLOG("TMD Verify Content failed\n");
		svc_err = ESC_ERR_CONTENT_MATCH;
		goto end;
	}

	DBLOG("Creating stream for cid 0x%08x", content_id);

	// Need to create a new one
	stream = ALLOC(ec_dec_stream_t);
	if ( stream == NULL ) {
		svc_err = ESC_ERR_NOMEM;
		goto end;
	}

	stream->content_id = content_id;
	stream->offset = 0;
	stream->size = cpu_to_be64(cm.size);
	memcpy(stream->tmd_hash, cm.hash, sizeof(stream->tmd_hash));
	memset(stream->ivec, 0, sizeof(stream->ivec));
	// The ivec for this type of content is the content index in
	// 2byte network order, the rest all 0
	// Note: the index is likely to be zero as well. d'oh
	cind = (u16*)stream->ivec;
	*cind = cm.index;

	svc_err = IOSC_GenerateHash(stream->hash_ctx, NULL, 0,
				    IOSC_SHA256_INIT, NULL);
	if ( svc_err != IOSC_ERROR_OK ) {
		DBLOG("hash init failed\n");
		goto end;
	}

	// link it in
	stream->next = title->dec_streams.next;
	stream->last = &title->dec_streams;
	stream->next->last = stream->last->next = stream;

	*streamp = stream;
	svc_err = ESC_ERR_OK;

end:
	return svc_err;
}

#define MIN_DECRYPT_LEN 	16
int
esc_decrypt(esc_crypt_t *req)
{
	s32 svc_err = ESC_ERR_UNSPECIFIED;
	ec_title *title = NULL;
	void *in_map[ESC_DECRYPT_FRAME_MAX];
	void *out_map[ESC_DECRYPT_FRAME_MAX];
	bool in_is_mapped = false;
	bool out_is_mapped = false;
    	IOSCHash256 hash;
	u32 tot_size;
	bool del_stream = true;
	ec_dec_stream_t *stream = NULL;
	int i;

	if (!esc_initialized) {
		req->error = ESC_ERR_NOT_YET_INIT;
		return -1;
	}
	
	// Look up the title.
	if ( (svc_err = cache_find(&__ttl_cache, req->title_handle,
				   (void **)&title)) ) {
		goto end;
	}

	// more sanity checks
	if ( req->frame_cnt > ESC_DECRYPT_FRAME_MAX ) {
		DBLOG("invalid frame cnt %d\n", req->frame_cnt);
		svc_err = ESC_ERR_PARMS;
		goto end;
	}

	// convert an index to an id
	if ( (req->content_ind != ESC_INVALID_IND) &&
		_esh_content_ind_2_id(title, req->content_ind,
			&req->content_id)) {
		DBLOG("Failed translating content index 0x%08x for "FMT0x64"\n",
		      req->content_ind, title->title_id);
		svc_err = ESC_ERR_PARMS;
		goto end;
	}

	// pull out the stream from the title
	if ((svc_err = _content_stream_get(title, req->content_id, &stream))) {
		DBLOG("Failed getting the content decryption stream\n");
		goto end;
	}

	// verify the size of the IO, this can be off because of encryption
	// padding
	if ( ((req->size + stream->offset) > stream->size) &&
		(((req->size + stream->offset) - stream->size) > 16)) {
		DBLOG("data stream bigger than content\n");
		svc_err = ESC_ERR_PARMS;
		goto end;
	}

	// map the input buffer
	if ( pfns_map(in_map, req->frames_in, req->frame_cnt) ) {
		DBLOG("in map failed\n");
		svc_err = ESC_ERR_NOMEM;
		goto end;
	}
	in_is_mapped = true;

	// map the output buffer
	if ( pfns_map(out_map, req->frames_out, req->frame_cnt) ) {
		DBLOG("out map failed\n");
		svc_err = ESC_ERR_NOMEM;
		goto end;
	}
	out_is_mapped = true;


	// decrypt the data	
	tot_size = req->size;
	for( i = 0 ; i < req->frame_cnt ; i++ ) {
		u32 dec_size;

		dec_size = MIN(VM_PAGE_SIZE, tot_size);
		if ( dec_size < MIN_DECRYPT_LEN ) {
			DBLOG("Invalid dec_size %d\n", dec_size);
			svc_err = ESC_ERR_UNSPECIFIED;
			goto end;
		}

		if ( IOSC_Decrypt(title->ticket->key, stream->ivec,
				  in_map[i], dec_size, out_map[i])
				!= IOSC_ERROR_OK ) {
			DBLOG("Decrypt of frame %d failed\n", i);
			svc_err = ESC_ERR_UNSPECIFIED;
			goto end;
		}

		tot_size -= dec_size;
		stream->offset += dec_size;

		// Hash only content, omit any padding
		if ( stream->offset >= stream->size ) {
			dec_size -= stream->offset - stream->size;
		}

		// update the hash
		// Generate the final hash
		if ( IOSC_GenerateHash(stream->hash_ctx, out_map[i], dec_size,
				IOSC_SHA256_UPDATE, NULL) != IOSC_ERROR_OK ) {
			DBLOG("hash update failed\n");
			svc_err = ESC_ERR_UNSPECIFIED;
			goto end;
		}
	}

	if ( stream->offset >= stream->size ) {
		if ( IOSC_GenerateHash(stream->hash_ctx, NULL, 0,
				IOSC_SHA256_FINAL, hash) != IOSC_ERROR_OK ) {
			DBLOG("hash final failed\n");
			svc_err = ESC_ERR_UNSPECIFIED;
			goto end;
		}
		if ( memcmp(hash, stream->tmd_hash, sizeof(hash) != 0) ) {
			DBLOG("Hash check failed");
			svc_err = ESC_ERR_CONTENT_HASH;
			goto end;
		}
	}
	else {
		del_stream = false;
	}

	svc_err = ESC_ERR_OK;
end:
	if ( del_stream && stream ) {
		DBLOG("deleting stream for cid 0x%08x", stream->content_id);
		stream->next->last = stream->last;
		stream->last->next = stream->next;
		free(stream);
	}
	if ( out_is_mapped ) {
		pfns_unmap(req->frames_out, req->frame_cnt);
	}
	if ( in_is_mapped ) {
		pfns_unmap(req->frames_in, req->frame_cnt);
	}
	req->error = svc_err;
	return (svc_err) ? -1 : 0;	
}

// 
// Parse the CFM Hash Header
//
static int
_esh_cfm_parse_hash(CfmHashSectionHeader *hdr, IOSCHash256 *hashes, ec_cfm *cfm)
{
	u32 num_blks = cpu_to_be32(hdr->numRecords);

	// make sure all sections agree on the number of blocks
	if ( cfm->blk_cnt == 0 ) {
		cfm->blk_cnt = num_blks;
	}
	else if ( cfm->blk_cnt != num_blks ) {
		DBLOG("CFM Hash - num blocks inconsistent 0x08%x vs 0x%08x\n",
		      cfm->blk_cnt, num_blks);
		return -1;
	}
	cfm->hashes = hashes;

	// sanity check this section
	if ( hdr->hashType != CFM_SHA256_HASH ) {
		DBLOG("CFM Hash type invalid = %d\n", hdr->hashType);
		return -1;
	}

	if ( hdr->recordSize != sizeof(IOSCHash256) ) {
		DBLOG("CFM Hash Record size invalid - %d\n", hdr->recordSize);
		return -1;
	}

	cfm->blk_size = (1 << hdr->log2BlockSize);

	return 0;
}

// 
// Parse the CFM Zero Header
//
static int
_esh_cfm_parse_zero(CfmZeroSectionHeader *hdr, u8 *bits, ec_cfm *cfm)
{
	u32 num_blks = cpu_to_be32(hdr->numRecords) * 8;
	u32 page_size;

	// make sure all sections agree on the number of blocks
	if ( cfm->blk_cnt == 0 ) {
		cfm->blk_cnt = num_blks;
	}
	else if ( cfm->blk_cnt != num_blks ) {
		DBLOG("CFM zero - num blocks inconsistent 0x08%x vs 0x%08x\n",
		      cfm->blk_cnt, num_blks);
		return -1;
	}

	page_size = 1 << hdr->log2BlockSize;
	// make sure all sections agree on the number of blocks
	if ( cfm->page_size == 0 ) {
		cfm->page_size = page_size;
	}
	else if ( cfm->page_size != page_size ) {
		DBLOG("CFM zero - page size inconsistent 0x08%x vs 0x%08x\n",
		      cfm->page_size, page_size);
		return -1;
	}
	cfm->zero_bits = bits;

	// sanity check this section
	if ( hdr->recordSize != sizeof(u8) ) {
		DBLOG("CFM zero Record size invalid - %d\n", hdr->recordSize);
		return -1;
	}

	return 0;
}

//
// parse the CFM in one shot.
//
static int
_esh_cfm_parse(ec_cfm *cfm)
{
	u32 i;
	CfmHeader *cfm_hdr;
	u8 *buf;
	CfmSectionHeader *sec_hdr;
	u32 sec_offset;
	void *dp;
	void *hp;

	cfm_hdr = (CfmHeader *)cfm->cfm_data;
	buf = (u8 *)cfm_hdr;

	// Do the version check here
	if ( cpu_to_be16(cfm_hdr->version) != CFM_VERSION) {
		DBLOG("CFM: Version mismatch %d - %d\n",
		      cpu_to_be16(cfm_hdr->version), CFM_VERSION);
		return -1;
	}

	sec_offset = cpu_to_be32(cfm_hdr->sectionOffset);

	// look for the section
	DBLOG("CFM has %d sections\n", cfm_hdr->numSections);
	for( i = 0 ; i < cfm_hdr->numSections ; i++ ) {
		sec_hdr = (CfmSectionHeader *)&buf[sec_offset];
		// adjust the section header
		sec_offset = cpu_to_be32(sec_hdr->nextSectionOffset);
		hp = &buf[cpu_to_be32(sec_hdr->payloadHeaderOffset)];
		dp = &buf[cpu_to_be32(sec_hdr->payloadDataOffset)];
		
		// now adjust the section specific headers
		switch (sec_hdr->sectionType) {
		case CFM_HASH_SECTION:
			if ( _esh_cfm_parse_hash(hp, dp, cfm) ) {
				return -1;
			}
			break;
		case CFM_ZERO_SECTION:
			if ( _esh_cfm_parse_zero(hp, dp, cfm) ) {
				return -1;
			}
			break;
		default:
			DBLOG("Skipping unknown CFM Section Header type %d\n",
			      sec_hdr->sectionType);
			break;
		}
	}

	// Make sure we have a hash section
	if ( cfm->hashes == NULL ) {
		DBLOG("CFM - Missing Hash Section!\n");
		return -1;
	}
	DBLOG("CFM: blk cnt 0x%08x - blk size 0x%08x\n",
	      cfm->blk_cnt, cfm->blk_size);

	if ( cfm->page_size == 0 ) {
		DBLOG("No sections containing page size.\n");
		cfm->page_size = VM_PAGE_SIZE;
	}
	else if ( cfm->page_size != VM_PAGE_SIZE ) {
		DBLOG("Invalid page size specified = %d\n", cfm->page_size);
		return -1;
	}
	DBLOG("CFM: page size 0x%08x\n", cfm->page_size);

	cfm->pages_per_blk = (cfm->blk_size / cfm->page_size);

	if ( cfm->page_size > cfm->blk_size ) {
		DBLOG("CFM Invalid page size\n");
		return -1;
	}

	return 0;
}

//
// Look up this content in the content access mask of the ticket
// to see if we have rights to access this content.
//
static int
_esh_content_access_chk(ec_title *title, u32 index)
{
	u32 i;
	u32 rec_off;
	hc_right *hr;
	ESV1ContentRecord *cr;
	u32 mask_off, mask_ind, mask_bit;
	int rv = -1;

	hr = &title->ticket->rights[ES_ITEM_RIGHT_CONTENT];

	if ( hr->data == NULL ) {
		DBLOG("No content access information found\n");
		return -1;
	}

	// walk through the masks
	cr = (ESV1ContentRecord *)hr->data;
	for( i = 0 ; i < hr->cnt ; i++ ) {
		rec_off = cpu_to_be32(cr->offset);

		DBLOG("rec_off = 0x%08x\n", rec_off);

		// See if the content index is inside this record
		if ( (index < rec_off) || 
		     (index > (rec_off + (8 * sizeof(cr->accessMask)))) ) {
			continue;
		}

		mask_off = index - rec_off;
		mask_ind = mask_off / 8;
		mask_bit = mask_off % 8;

		DBLOG("index 0x%08x - rec_off 0x%08x\n", index, rec_off);
		DBLOG("mask_ind 0x%08x mask_bit 0x%08x\n", mask_ind, mask_bit);
		DBLOG("mask[%d]: 0x%08x\n", i, cr->accessMask[mask_ind]);
	
		// if the bit's set, we're good.
		if (cr->accessMask[mask_ind] & (1 << mask_bit)) {
			rv = 0;
		}

		break;
	}

	return rv;
}

static s32
_esh_content_import(ec_title *title, u32 content_id, u8 **cfm_datap,
		    u32 cfm_len, ec_cfm **cfmp)
{
	ec_cfm *cfm = NULL;
    	IOSCHash256 hash;
	ESV1ContentMeta cm;
	s32 svc_err = ESC_ERR_UNSPECIFIED;
	u32 tmd_cfm_len;

	// see if it already exists
	if ( _esh_content_find_id(title, content_id) ) {
		DBLOG("content id 0x%08x already exists\n", content_id);
		svc_err = ESC_ERR_EXISTS;
		goto end;
	}

	// make sure data titles are still properly referenced
	if ( title->is_data_title && !title->data_is_referenced ) {
		DBLOG("data title "FMT0x64" has no valid reference\n",
		      title->title_id);
		svc_err = ESC_ERR_NO_RIGHT;
		goto end;
	}

	// Look up the content ID. Pull out its hash at the same time.
	DBLOG("Content ID is 0x%08x\n", content_id);
	if ( esVerifyTmdContent(title->tmd_rdr, content_id,
				&title->title_meta, &cm) != ES_ERR_OK) {
		DBLOG("TMD Verify Content failed\n");
		svc_err = ESC_ERR_CONTENT_MATCH;
		goto end;
	}

	tmd_cfm_len = (u32)cpu_to_be64(cm.size);
	DBLOG("Content Index is 0x%08x\n", cpu_to_be16(cm.index));

	// base sanity check
	// The two sizes "might" differ depending upon whether it was
	// padded for an encryption pass.
	if ( (tmd_cfm_len > cfm_len) || ((cfm_len - tmd_cfm_len) > 16) ) {
		DBLOG("Content size mismatch - 0x%08x : 0x%08x\n",
			tmd_cfm_len, cfm_len);
		svc_err = ESC_ERR_PARMS;
		goto end;
	}

	// verify we have rights to access it.
	if ( _esh_content_access_chk(title, cpu_to_be16(cm.index)) ) {
		DBLOG("Content access check failed\n");
		svc_err = ESC_ERR_NO_RIGHT;
		goto end;
	}

	// allocate a buffer to hold it.
	cfm = ALLOC(ec_cfm);
	if ( cfm == NULL ) {
		DBLOG("CFM alloc failed\n");
		svc_err = ESC_ERR_NOMEM;
		goto end;
	}
	
	// allocate a buffer to hold it
	cfm->cfm_data = *cfm_datap;
	*cfm_datap = NULL;
	cfm->cfm_len = cfm_len;


	// Compute the hash of the CFM data
	// Note: Must be careful here. cfm_data may have been rounded up
	// for an encryption pass that was tossed. The hash was done on the
	// unpadded data so we must get the size of the CFM from the TMD.
	// Unclear which size I'm being given.
	if (_esh_hash_gen(cfm->cfm_data, tmd_cfm_len, hash)) {
		DBLOG("CFM hash gen failed\n");
		goto end;
	}

	// see if it matches the TMD hash.
	if ( memcmp(hash, cm.hash, sizeof(hash)) ) {
		DBLOG("CFM hash check failed\n");
		svc_err = ESC_ERR_CONTENT_HASH;
		goto end;
	}

	// parse the CFM
	if ( _esh_cfm_parse(cfm) ) {
		DBLOG("CFM Parse failed\n");
		svc_err = ESC_ERR_CONTENT_INVALID;
		goto end;
	}

	// GSS must have a valid zero mask section
	if ( title->is_gss && (cfm->zero_bits == NULL) ) {
		DBLOG("GSS CFM missing zero mask\n");
		svc_err = ESC_ERR_CONTENT_INVALID;
		goto end;
	}

	cfm->content_id = content_id;
	cfm->content_index = cpu_to_be16(cm.index);
	cfm->title = title;
	cfm->is_rw = title->is_gss;

	// add it to the cache
	if ( cache_add(&__cfm_cache, cfm, &cfm->content_handle) ) {
		svc_err = ESC_ERR_NOMEM;
		DBLOG("Content cache unexpectedly full!\n");
		goto end;
	}

	// add it to the title's list
	cfm->next = title->cfm_list;
	if ( cfm->next ) {
		cfm->next->last = cfm;
	}
	title->cfm_list = cfm;

	DBLOG("content_handle 0x%08x -> title 0x%08x\n", cfm->content_handle,
	      title->handle);

	*cfmp = cfm;

	cfm = NULL;
	svc_err = ESC_ERR_OK;

end:
	if ( cfm ) {
		if ( cfm->cfm_data ) {
			free(cfm->cfm_data);
		}
		free(cfm);
	}

	return svc_err;
}

int
esc_content_import(esc_content_t *req)
{
	IInputStream *cfm_rdr = NULL;
	u32 xfer_cnt;
	ec_cfm *cfm = NULL;
	ec_title *title;
	s32 svc_err = ESC_ERR_UNSPECIFIED;
	u8 *cfm_data = NULL;
	u32 cfm_len;

	if (!esc_initialized) {
		req->error = ESC_ERR_NOT_YET_INIT;
		return -1;
	}

	// see if there's room
	if ( CACHE_IS_FULL(&__cfm_cache) ) {
		DBLOG("__cfm_cache full\n");
		svc_err = ESC_ERR_NOMEM;
		goto end;
	}

	// XXX Make sure the req is complete computed by the size

	// Look up the title.
	if ( (svc_err = cache_find(&__ttl_cache, req->title_handle,
				   (void **)&title)) ) {
		goto end;
	}

	// allocate a buffer to hold it
	cfm_len = req->size;
	cfm_data = (u8 *)malloc(cfm_len);
	if ( cfm_data == NULL ) {
		DBLOG("CFM Data alloc failed\n");
		svc_err = ESC_ERR_NOMEM;
		goto end;
	}

	// set up a storage reader to suck it in
	if ( iStoragePfnInit(&cfm_rdr, req->frames, cfm_len) ) {
		DBLOG("CFM reader init failed\n");
		svc_err = ESC_ERR_BUF_ACCESS;
		goto end;
	}

	// copy this thing in.
	if ( rdrTryRead(cfm_rdr, &xfer_cnt, cfm_data, cfm_len) ||
		(xfer_cnt != cfm_len)) {
		DBLOG("CFM read failed\n");
		svc_err = ESC_ERR_BUF_ACCESS;
		goto end;
	}

	svc_err = _esh_content_import(title, req->content_id, &cfm_data,
				      cfm_len, &cfm);
	if ( svc_err ) {
		DBLOG("Failed import of content\n");
		goto end;
	}

	// copy out the content handle
	req->content_handle = cfm->content_handle;
	svc_err = ESC_ERR_OK;

end:
	if ( cfm_rdr ) {
		iStoragePfnFree(&cfm_rdr);
	}
	if ( cfm_data ) {
		free(cfm_data);
	}
	req->error = svc_err;
	return svc_err ? -1 : 0;
}

//
// The following was roughly bent to our purposes from
// TitleMetaData::FindContentInfos()
//
static int
_esh_content_ind_2_id(ec_title *title, u32 ind, u32 *idp)
{
    ESError rv = ES_ERR_OK;
    ESV1TitleMetaHeader *v1Head;
    ESContentMetaSearchByIndex cmdSearchByIndex;
    ESContentIndex c_ind = ind;
    ESContentInfo c_info;
    u32 i;


    v1Head = &title->title_meta.v1Head;
    for (i = 0; i < ES_MAX_CMD_GROUPS; i++) {
        if (ntohs(v1Head->cmdGroups[i].nCmds == 0)) {
            continue;
        }

        // Check if any of the specified indexes are in this group
        if (ntohs(v1Head->cmdGroups[i].offset) > ind) {
	    continue;
        }

        if (i == (ES_MAX_CMD_GROUPS - 1)) {
              continue;
        }

        if (ntohs(v1Head->cmdGroups[i + 1].nCmds) != 0 &&
		ntohs(v1Head->cmdGroups[i + 1].offset) <= ind) {
            continue;
        }

        // At least one of the specified indexes is in this group 
        cmdSearchByIndex.indexes = &c_ind;
        cmdSearchByIndex.nIndexes = 1;
        cmdSearchByIndex.outInfos = &c_info;

        if ((rv = esVerifyCmdGroup(title->tmd_rdr, v1Head->cmdGroups, i,
			false, &cmdSearchByIndex, NULL, NULL)) != ES_ERR_OK) {
            goto end;
        }

        if ( cmdSearchByIndex.nFound == 1 ) {
		*idp = c_info.id;
		return 0;
        }
    }

end:
	return -1;
}

int
esc_content_import_ind(esc_content_ind_t *req)
{
	ec_title *title;
	s32 svc_err = ESC_ERR_UNSPECIFIED;

	if (!esc_initialized) {
		req->error = ESC_ERR_NOT_YET_INIT;
		return -1;
	}

	// Look up the title.
	if ( (svc_err = cache_find(&__ttl_cache, req->title_handle,
				   (void **)&title)) ) {
		req->error = svc_err;
		return 0;
	}

	// Convert the index into an ID.
	if ( _esh_content_ind_2_id(title, req->content_ind,
				  &req->content_ind) ) {
		DBLOG("Failed translating content index 0x%08x for "FMT0x64"\n",
		      req->content_ind, title->title_id);
		req->error = ESC_ERR_PARMS;
		return 0;
	}

	return esc_content_import((esc_content_t *)req);
}

int
esc_content_export(esc_content_export_t *req)
{
	s32 svc_err = ESC_ERR_UNSPECIFIED;
	ec_cfm *cfm = NULL;
	IOutputStream *cfm_wrtr = NULL;
	u32 xfer_cnt;

	if (!esc_initialized) {
		req->error = ESC_ERR_NOT_YET_INIT;
		return -1;
	}

	// Look up the content.
	if ( (svc_err = cache_find(&__cfm_cache, req->content_handle,
				   (void **)&cfm)) ) {
		goto end;
	}

	// Make sure this is an exportable CFM
	if ( !cfm->title->is_gss ) {
		DBLOG("CFM is not available for export\n");
		svc_err = ESC_ERR_READONLY;
		goto end;
	}

	// See if they're just requesting the size
	if ( req->size == 0 ) {
		svc_err = ESC_ERR_OK;
		goto success;
	}

	// Make sure the buffer is the correct size
	if ( req->size != cfm->cfm_len ) {
		DBLOG("Buffer size mismatch\n");
		svc_err = ESC_ERR_SIZE_MISMATCH;
		goto end;
	}

	// XXX Make sure the req is complete computed by the size

	// create a storage writer for this thing
	if ( iStoragePfnInit(&cfm_wrtr, req->frames, req->size) ) {
		svc_err = ESC_ERR_BUF_ACCESS;
		DBLOG("CFM writer init failed\n");
		goto end;
	}

	// copy this thing out.
	if ( wrtrTryWrite(cfm_wrtr, &xfer_cnt, cfm->cfm_data, req->size)
	     || (xfer_cnt != req->size)) {
		svc_err = ESC_ERR_BUF_ACCESS;
		DBLOG("CFM write failed\n");
		goto end;
	}

	// clear the dirty bit
	cfm->is_dirty = false;

success:
	req->size = cfm->cfm_len;
	svc_err = ESC_ERR_OK;

end:
	if ( cfm_wrtr ) {
		iStoragePfnFree(&cfm_wrtr);
	}
	req->error = svc_err;

	return (svc_err) ? -1 : 0;
}

int
esc_content_release(esc_resource_rel *req)
{
	if (!esc_initialized) {
		req->error = ESC_ERR_NOT_YET_INIT;
		return -1;
	}

	req->error = _esh_content_release(req->handle);

	return req->error ? -1 : 0;
}

int
esc_content_query(esc_content_query_t *q)
{
	ec_cfm *cfm;
	ec_cache_node_t *cn;
	u32 i;

	if (!esc_initialized) {
		q->error = ESC_ERR_NOT_YET_INIT;
		return -1;
	}

	// Walk thru the list of imported contents. A handle of ANY matches
	// any imported title.
	i = 0;
	for( cn = __cfm_cache.ch_active ; cn ; cn = cn->cn_next ) {
		cfm = (ec_cfm *)cn->cn_data;
		// are we interested in this title?
		if ( (q->title_handle != ESC_MATCH_ANY) &&
		     (q->title_handle != cfm->title->handle) ) {
			continue;
		}

		q->recs[i].title_handle = cfm->title->handle;
		q->recs[i].content_id = cfm->content_id;
		q->recs[i].content_handle = cfm->content_handle;
		q->recs[i].content_state = 0;
		if ( cfm->is_rw ) {
			q->recs[i].content_state |= ESC_CFMST_RW;
		}
		if ( cfm->is_dirty ) {
			q->recs[i].content_state |= ESC_CFMST_DIRTY;
		}
		i++;
	}

	q->rec_cnt = i;
	q->error = ESC_ERR_OK;

	return 0;
}

static inline u8
mylog2(unsigned int x)
{
    u8  l = 0;
    while (x > 1) {
        x /= 2;
        l++;
    }
    return l;
}

static s32
_esh_cfm_create(u32 size, u32 blk_size, u8 **cfm_datap, u32 *cfm_lenp)
{
	u8 *cfm_data;
	u32 cfm_len;
	u32 blk_cnt;
	u32 hash_size;
	u32 zero_size;
	CfmHeader *cfm;
	CfmSectionHeader *sect;
	CfmZeroSectionHeader *zero;
	CfmHashSectionHeader *hash;
	u8 *data;
	u32 cur_pos;

	// The size is in MiB. Convert to bytes.
	size *= (1024 * 1024);

	// compute the full size of the CFM.
	blk_cnt = size / blk_size;
	hash_size = blk_cnt * sizeof(IOSCHash256);
	zero_size = blk_cnt / 8; // 8 bits per byte

	// CFM consists of 2 sections
	// 1st section is the zero mask
	// 2nd section is the hashes
	cfm_len = sizeof(CfmHeader) + 2 * sizeof(CfmSectionHeader) +
		  sizeof(CfmZeroSectionHeader) + sizeof(CfmHashSectionHeader) +
		  hash_size + zero_size;

	// XXX - _might_ need to round up to a mult of 16.

	cfm_data = ARRAY(u8, cfm_len);
	if ( cfm_data == NULL ) {
		DBLOG("Failed to allocate 0x%08x bytes\n", cfm_len);
		return ESC_ERR_NOMEM;
	}

	// fill in pieces
	// set up the CFM header
	cfm = (CfmHeader *)cfm_data;
	cfm->version = cpu_to_be16(CFM_VERSION);
	cfm->numSections = 2;
	cur_pos = sizeof(*cfm);
	cfm->sectionOffset = cpu_to_be32(cur_pos);

	// set up the zero mask
	sect = (CfmSectionHeader *)(&cfm_data[cur_pos]);
	sect->sectionType = CFM_ZERO_SECTION;
	cur_pos += sizeof(*sect) ;

	sect->payloadHeaderOffset = cpu_to_be32(cur_pos);
	zero = (CfmZeroSectionHeader *)(&cfm_data[cur_pos]);
	cur_pos += sizeof(*zero);

	sect->payloadDataOffset = cpu_to_be32(cur_pos);
	data = &cfm_data[cur_pos];
	cur_pos += zero_size;

	sect->nextSectionOffset = cpu_to_be32(cur_pos);

	// mark every block as zero/unuse
	zero->numRecords = cpu_to_be32(zero_size);
	zero->log2BlockSize = mylog2(blk_size);
	zero->recordSize = sizeof(u8);
	memset(data, 0xff, zero_size);

	// set up the hash list
	sect = (CfmSectionHeader *)(&cfm_data[cur_pos]);
	sect->sectionType = CFM_HASH_SECTION;
	cur_pos += sizeof(*sect);

	sect->payloadHeaderOffset = cpu_to_be32(cur_pos);
	hash = (CfmHashSectionHeader *)(&cfm_data[cur_pos]);
	cur_pos += sizeof(*hash);

	sect->payloadDataOffset = cpu_to_be32(cur_pos);
	data = &cfm_data[cur_pos];
	cur_pos += hash_size;

	// sect->nextSectionOffset = cpu_to_be32(cur_pos);
	sect->nextSectionOffset = 0;

	// mark every block as zero/unuse
	hash->numRecords = cpu_to_be32(blk_cnt);
	hash->log2BlockSize = mylog2(blk_size);
	hash->hashType = CFM_SHA256_HASH;
	hash->recordSize = sizeof(IOSCHash256);
	memset(data, 0, hash_size);

	// return the results;
	*cfm_lenp = cfm_len;
	*cfm_datap = cfm_data;

	return ESC_ERR_OK;
}

static s32
_esh_tmd_create(ec_title *parent, ec_ticket_t *tkt_sig, u32 content_id,
		u8 *cfm_data, u32 cfm_len, u8 **tmd_datap, u32 *tmd_lenp)
{
	u8 *tmd_data = NULL;
	u32 tmd_len;
	ESV1TitleMeta *mp;
	ESV1ContentMeta *cmp;
	s32 err = 0;
	ESGVMTmdPlatformData *pd;

	// create an initial TMD from a template
	// allocate the tmd
	// Note: There are no certs associated with this TMD because
	// the signaure is HMAC-SHA1.
	DBLOG("*** Certs not allocated for GSS TMD\n");
	tmd_len = sizeof(tmd_template);
	tmd_data = ARRAY(u8, tmd_len);
	if ( tmd_data == NULL ) {
		DBLOG("Failed allocating tmd_data\n");
		err = ESC_ERR_NOMEM;
		goto end;
	}

	// copy in the template
	memcpy(tmd_data, tmd_template, sizeof(tmd_template));

	// Fill in the title id, type, user_id, etc
	mp = (ESV1TitleMeta *)tmd_data;
	mp->head.titleId = cpu_to_be64(parent->title_id);
	pd = (ESGVMTmdPlatformData *)&mp->head.customData;

	// title type and IS_GSS "could" be set by the template
	mp->head.type = cpu_to_be32(ES_TITLE_TYPE_GVM_TITLE);
	pd->flags = cpu_to_be16(ES_GVM_TMD_FLAG_IS_GSS);
	pd->userId = cpu_to_be64(tkt_sig->user_id);
	pd->gssSize = cpu_to_be16(parent->gss_size);

	// Update the content meta-data
	// content ID, content type
	cmp = (ESV1ContentMeta *)(&mp[1]);
	cmp->cid = cpu_to_be32(content_id);
	cmp->index = cpu_to_be16(0);
	cmp->type = cpu_to_be16(ES_CONTENT_TYPE_ENCRYPTED |
				ES_CONTENT_TYPE_CFM);
	cmp->size = cpu_to_be64(cfm_len);

	// Sign the TMD
	if ( _esh_tmd_sign(tkt_sig, tmd_data, tmd_len, cfm_data, cfm_len) ) {
		DBLOG("Failed signing the TMD\n");
		err = ESC_ERR_GSS_SIGN;
		goto end;
	}
	
	*tmd_datap = tmd_data;
	*tmd_lenp = tmd_len;

	err = ESC_ERR_OK;
end:
	if ( err ) {
		if ( tmd_data ) {
			free(tmd_data);
		}
	}

	return err;
}

//
// Create a TMD and CFM, sign it and then do a regular
// import operation.
//
int
esc_gss_create(esc_gss_create_t *req)
{
	s32 svc_err = ESC_ERR_UNSPECIFIED;
	ec_ticket_t *tkt_sig = NULL;
	ec_ticket_t *tkt_data = NULL;
	ec_title *parent = NULL;
	ec_title *title = NULL;
	ec_cfm *cfm = NULL;
	u8 *cfm_data = NULL;
	u8 *tmd_data = NULL;
	u32 cfm_len;
	u32 tmd_len;

	if (!esc_initialized) {
		req->error = ESC_ERR_NOT_YET_INIT;
		return -1;
	}

	// see if there's room
	if ( CACHE_IS_FULL(&__ttl_cache) ) {
		DBLOG("__title_cache full\n");
		svc_err = ESC_ERR_NOMEM;
		goto end;
	}

	// see if there's room
	if ( CACHE_IS_FULL(&__cfm_cache) ) {
		DBLOG("__cfm_cache full\n");
		svc_err = ESC_ERR_NOMEM;
		goto end;
	}

	// look up the parent title
	svc_err = cache_find(&__ttl_cache, req->title_handle,
			     (void **)&parent);
	if ( svc_err ) {
		DBLOG("title 0x%08x find failed\n", req->title_handle);
		goto end;
	}

	// parent can't be a GSS
	if ( parent->is_gss || parent->is_data_title ) {
		DBLOG("Parent is not a game title\n");
		svc_err = ESC_ERR_TITLE_GVM;
		goto end;
	}

	// look up the sign ticket. We need it to make a real TMD
	// That we're going to fake up into an import
	svc_err = cache_find(&__tkt_cache, req->ticket_sign_handle,
			     (void **)&tkt_sig);
	if ( svc_err ) {
		DBLOG("ticket 0x%08x find failed\n",
		      req->ticket_sign_handle);
		goto end;
	}

	// look up the data ticket
	svc_err = cache_find(&__tkt_cache, req->ticket_data_handle,
			     (void **)&tkt_data);
	if ( svc_err ) {
		DBLOG("ticket 0x%08x find failed\n",
		      req->ticket_data_handle);
		goto end;
	}

	// Verify that both tickets represent user tickets.
	if ( !tkt_sig->is_user_key || !tkt_data->is_user_key ) {
		DBLOG("sig or data ticket is not a user ticket\n");
		svc_err = ESC_ERR_TKT_RIGHTS;
		goto end;
	}

	// verify that the user ids on the tickets match
	// And match the requested user id.
	if ( (tkt_sig->user_id != tkt_data->user_id) ||
	     (req->user_id != tkt_sig->user_id) ) {
		DBLOG("Ticket user ID don't match\n");
		svc_err = ESC_ERR_USER_ID;
		goto end;
	}

	// create a TMD and CFM for this guy.
	svc_err = _esh_cfm_create(parent->gss_size, _GSS_BLK_SIZE, &cfm_data,
				  &cfm_len);
	if ( svc_err ) {
		DBLOG("Failed creating initial CFM\n");
		goto end;
	}

	svc_err = _esh_tmd_create(parent, tkt_sig, req->content_id,
				  cfm_data, cfm_len, &tmd_data, &tmd_len);
	if ( svc_err ) {
		DBLOG("Failed creating title from template\n");
		goto end;
	}

	// Now, import the title.
	svc_err = eshi_title_import(parent->title_id, req->cookie,
				    &tmd_data, tmd_len, tkt_sig, tkt_data,
				    &title);
	if ( svc_err ) {
		DBLOG("Failed import of generated GSS Title\n");
		goto end;
	}

	// Now, import the CFM.
	svc_err = _esh_content_import(title, req->content_id, &cfm_data,
				      cfm_len, &cfm);
	if ( svc_err ) {
		DBLOG("Failed import of generated GSS CFM\n");
		goto end;
	}

	// return the title handle
	req->return_title_handle = title->handle;
	// return the content handle
	req->return_content_handle = cfm->content_handle;

	svc_err = ESC_ERR_OK;

end:
	if ( tmd_data ) {
		free(tmd_data);
	}
	if ( cfm_data ) {
		free(cfm_data);
	}
	
	req->error = svc_err;

	return svc_err ? -1 : 0;
}

int
esc_mem_usage(esc_mem_usage_t *req)
{
	// There's no data coming in, actually.
	memset(req, 0, sizeof(*req));

	req->mem_total = __mem_pool_size;
	req->mem_free = _mem_avail();

	return 0;
}

int
esc_rand_seed_set(esc_randseed_t *rs)
{
	s32 svc_err = ESC_ERR_UNSPECIFIED;
	static bool _rs_is_init = false;

	if ( _rs_is_init ) {
		DBLOG("Attempt to initialize the seeds twice?\n");
		goto end;
	}

	init_by_array64(rs->seeds, sizeof(rs->seeds));

	DBLOG("RS "FMT0x64":"FMT0x64":"FMT0x64":"FMT0x64"\n",
		rs->seeds[0], rs->seeds[1], rs->seeds[2],
		rs->seeds[3]);

	svc_err = ESC_ERR_OK;

end:
	rs->error = svc_err;

	return (svc_err) ? -1 : 0;
}

// The following lines are needed by Emacs to maintain the format the file started in.
// Local Variables:
// c-basic-offset: 8
// tab-width: 8
// indent-tabs-mode: t
// End:
